/* GStreamer
 * Copyright (C) 2011 Jon Nordby <jononor@gmail.com>
 * Copyright (C) 2021 Vasily Evseenko <svpcom@p2ptech.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#define _GNU_SOURCE

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include <cairo.h>
#include <cairo-gobject.h>

#include <glib.h>
#include "graphengine.h"

static gboolean
on_message (GstBus * bus, GstMessage * message, gpointer user_data)
{
    GMainLoop *loop = (GMainLoop *) user_data;

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    {
        GError *err = NULL;
        gchar *debug;

        gst_message_parse_error (message, &err, &debug);
        g_critical ("Got ERROR: %s (%s)", err->message, GST_STR_NULL (debug));
        g_main_loop_quit (loop);
        break;
    }

    case GST_MESSAGE_WARNING:
    {
        GError *err = NULL;
        gchar *debug;

        gst_message_parse_warning (message, &err, &debug);
        g_warning ("Got WARNING: %s (%s)", err->message, GST_STR_NULL (debug));
        g_main_loop_quit (loop);
        break;
    }

    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state;

        gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
        g_print ("gst %s: %s -> %s\n",
                 GST_OBJECT_NAME (message->src),
                 gst_element_state_get_name (old_state),
                 gst_element_state_get_name (new_state));
        break;
    }

    case GST_MESSAGE_EOS:
        g_main_loop_quit (loop);
        break;

    default:
        break;
    }

    return TRUE;
}

/* Datastructure to share the state we are interested in between
 * prepare and render function. */
typedef struct
{
    gboolean valid;
    GstVideoInfo vinfo;
} CairoOverlayState;

/* Store the information from the caps that we are interested in. */
static void
prepare_overlay (GstElement * overlay, GstCaps * caps, gpointer user_data)
{
    CairoOverlayState *state = (CairoOverlayState *) user_data;

    state->valid = gst_video_info_from_caps (&state->vinfo, caps);
}

/* Draw the overlay. */
static void
draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp,
              guint64 duration, gpointer user_data)
{
    CairoOverlayState *s = (CairoOverlayState *) user_data;

    if (!s->valid)
        return;

    pthread_mutex_lock(&video_mutex);

    cairo_surface_t *vctx = cairo_image_surface_create_for_data(video_buf_ext, CAIRO_FORMAT_ARGB32, GRAPHICS_WIDTH, GRAPHICS_HEIGHT, GRAPHICS_WIDTH * 4);
    if (cairo_surface_status(vctx) != CAIRO_STATUS_SUCCESS)
    {
        pthread_mutex_unlock(&video_mutex);
        fprintf(stderr, "Cairo surface error: %d\n", cairo_surface_status(vctx));
        exit(1);
    }

    cairo_set_source_surface(cr, vctx, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(vctx);

    pthread_mutex_unlock(&video_mutex);
}

static GstElement *
setup_gst_pipeline (CairoOverlayState * overlay_state, int rtp_port, char *codec, int rtp_jitter,
                    int use_xv, int screen_width, char *rtsp_url)
{
    char *pipeline_str;
    char *src_str;
    GstElement *pipeline;
    GstElement *cairo_overlay;
    GError *error = NULL;

    int screen_height = screen_width * 9 / 16;

    if(rtsp_url != NULL)
    {
        asprintf(&src_str,
                 "rtspsrc latency=%d location=\"%s\"", rtp_jitter, rtsp_url);
    } else {
        asprintf(&src_str,
                 "udpsrc port=%d caps=\"application/x-rtp,media=(string)video,  clock-rate=(int)90000, encoding-name=(string)H%s\" ! "
                 "rtpjitterbuffer latency=%d",
                 rtp_port, codec + 1, rtp_jitter);
    }

    asprintf(&pipeline_str,
             "%s ! "
             "rtp%sdepay ! "
             "%sparse config-interval=1 disable-passthrough=true ! "
             "avdec_%s ! "
             "glupload ! glcolorconvert ! glcolorscale ! "
             "glvideomixerelement background=black name=osd sink_0::width=%d sink_0::height=%d sink_1::width=%d sink_1::height=%d ! "
             "glcolorscale ! glcolorconvert ! gldownload ! "
             "%s sync=false "
             "videotestsrc pattern=solid-color foreground-color=0x00000000 is-live=true ! video/x-raw,width=%d,height=%d ! cairooverlay name=overlay ! glupload ! glcolorconvert ! osd. ",
             src_str, codec, codec, codec,
             screen_width, screen_height, screen_width, screen_height,
             use_xv ? "xvimagesink" : "autovideosink",
             GRAPHICS_WIDTH, GRAPHICS_HEIGHT);

    free(src_str);
    printf("GST pipeline: %s\n", pipeline_str);

    pipeline = gst_parse_launch(pipeline_str, &error);
    free(pipeline_str);

    if(error != NULL)
    {
        fprintf (stderr, "GST Error: %s\n", error->message);
        g_error_free (error);
        exit(1);
    }

    g_assert(pipeline);

    /* Hook up the necessary signals for cairooverlay */
    cairo_overlay = gst_bin_get_by_name(GST_BIN(pipeline), "overlay");
    g_signal_connect (cairo_overlay, "draw",
                      G_CALLBACK (draw_overlay), overlay_state);
    g_signal_connect (cairo_overlay, "caps-changed",
                      G_CALLBACK (prepare_overlay), overlay_state);

    gst_object_unref(cairo_overlay);
    return pipeline;
}

int gst_main(int rtp_port, char *codec, int rtp_jitter, int use_xv, int screen_width, char *rtsp_src)
{
    GMainLoop *loop;
    GstElement *pipeline;
    GstBus *bus;
    CairoOverlayState *overlay_state;

    gst_init (NULL, NULL);
    loop = g_main_loop_new (NULL, FALSE);

    /* allocate on heap for pedagogical reasons, makes code easier to transfer */
    overlay_state = g_new0 (CairoOverlayState, 1);

    pipeline = setup_gst_pipeline (overlay_state, rtp_port, codec, rtp_jitter, use_xv, screen_width, rtsp_src);

    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);
    gst_object_unref (GST_OBJECT (bus));

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);

    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

    g_free (overlay_state);
    return 0;
}
