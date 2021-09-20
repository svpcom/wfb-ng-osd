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


#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include <cairo.h>
#include <cairo-gobject.h>

#include <glib.h>
#include "graphengine.h"

// Comment it out to use xvimagesink
#define USE_VAAPI_OUTPUT

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

/* Draw the overlay.
 * This function draws a cute "beating" heart. */
static void
draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp,
              guint64 duration, gpointer user_data)
{
    CairoOverlayState *s = (CairoOverlayState *) user_data;
    int width, height;

    if (!s->valid)
        return;

    width = GST_VIDEO_INFO_WIDTH (&s->vinfo);
    height = GST_VIDEO_INFO_HEIGHT (&s->vinfo);

    pthread_mutex_lock(&video_mutex);

    cairo_surface_t *vctx = cairo_image_surface_create_for_data(video_buf_ext, CAIRO_FORMAT_ARGB32, GRAPHICS_WIDTH, GRAPHICS_HEIGHT, GRAPHICS_WIDTH * 4);
    if (cairo_surface_status(vctx) != CAIRO_STATUS_SUCCESS)
    {
        pthread_mutex_unlock(&video_mutex);
        fprintf(stderr, "Cairo surface error: %d\n", cairo_surface_status(vctx));
        exit(1);
    }

    cairo_scale(cr, (float)width/GRAPHICS_WIDTH, (float)height/GRAPHICS_HEIGHT);
    cairo_set_source_surface(cr, vctx, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(vctx);

    pthread_mutex_unlock(&video_mutex);
}

static GstElement *
setup_gst_pipeline (CairoOverlayState * overlay_state)
{
    GstElement *pipeline;
    GstElement *cairo_overlay;
    GstElement *udpsrc, *rtpdepay, *decoder, *videoconvert, *sink;
#ifndef USE_VAAPI_OUTPUT
    GstElement *adaptor2;
#endif
    GstCaps* caps = NULL;
    char *__rtp_port = getenv("RTP_PORT");
    int rtp_port = __rtp_port == NULL ? 5600 : atoi(__rtp_port);

    printf("RTP_PORT=%d\n", rtp_port);

    pipeline = gst_pipeline_new ("cairo-overlay-wfb");

    udpsrc = gst_element_factory_make ("udpsrc", "udpsrc");
    caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264");
    g_object_set(udpsrc, "do-timestamp", TRUE, "port", rtp_port, "caps", caps, NULL);

    rtpdepay = gst_element_factory_make ("rtph264depay", "rtpdepay");
    /* decoder = gst_element_factory_make ("vaapih264dec", "decoder"); */
    decoder = gst_element_factory_make ("avdec_h264", "decoder");

    /* Adaptors needed because cairooverlay only supports ARGB data */
    videoconvert = gst_element_factory_make ("videoconvert", "videoconvert");
    cairo_overlay = gst_element_factory_make ("cairooverlay", "overlay");

#ifdef USE_VAAPI_OUTPUT
    sink = gst_element_factory_make ("vaapisink", "sink");
    g_assert(sink);

    g_object_set(sink, "sync", FALSE, "fullscreen", TRUE, NULL);
#else
    adaptor2 = gst_element_factory_make ("videoconvert", "adaptor2");
    sink = gst_element_factory_make ("xvimagesink", "sink");
    g_assert(sink);

    g_object_set(sink, "sync", FALSE, NULL);
#endif

    /* If failing, the element could not be created */
    g_assert (cairo_overlay);

    /* Hook up the necessary signals for cairooverlay */
    g_signal_connect (cairo_overlay, "draw",
                      G_CALLBACK (draw_overlay), overlay_state);
    g_signal_connect (cairo_overlay, "caps-changed",
                      G_CALLBACK (prepare_overlay), overlay_state);

    gst_bin_add_many (GST_BIN (pipeline),
                      udpsrc, rtpdepay, decoder, videoconvert,
                      cairo_overlay,
#ifndef USE_VAAPI_OUTPUT
		      adaptor2,
#endif
		      sink, NULL);

    if (!gst_element_link_many (udpsrc, rtpdepay, decoder,
                                videoconvert, cairo_overlay,
#ifndef USE_VAAPI_OUTPUT
				adaptor2,
#endif
				sink, NULL))
    {
        g_warning ("Failed to link elements!");
    }

    return pipeline;
}

int gst_main (int argc, char **argv)
{
    GMainLoop *loop;
    GstElement *pipeline;
    GstBus *bus;
    CairoOverlayState *overlay_state;

    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);

    /* allocate on heap for pedagogical reasons, makes code easier to transfer */
    overlay_state = g_new0 (CairoOverlayState, 1);

    pipeline = setup_gst_pipeline (overlay_state);

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
