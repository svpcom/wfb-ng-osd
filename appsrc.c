/*
 * Сopyright (C) 2024 Vasily Evseenko <svpcom@p2ptech.org>
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

// Local camera overlay
#define LOCAL_CAMERA_SUPPORT 0

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <glib.h>

#include "graphengine.h"

// For gstreamer < 1.18
GstClockTime gst_element_get_current_running_time (GstElement * element);


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

static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data)
{
    GMainLoop *loop = (GMainLoop *) user_data;

    pthread_mutex_lock(&video_mutex);
    GstBuffer *buffer = render();
    pthread_mutex_unlock(&video_mutex);

    GST_BUFFER_PTS (buffer) = gst_element_get_current_running_time(appsrc);

    // set to min supported fps,
    // but low value will increase latency.
    // If fps lower than selected then cpu usage will increase a lot
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 30);
    GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_LIVE);
    GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DROPPABLE);

    //GstFlowReturn ret = gst_app_src_push_buffer(appsrc, buffer);
    GstFlowReturn ret;
    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref (buffer);


    if (ret != GST_FLOW_OK) {
        /* something wrong, stop pushing */
        g_main_loop_quit (loop);
    }
}

static const char* select_osd_render(osd_render_t osd_render)
{
    switch(osd_render)
    {
    case OSD_RENDER_XV:
        return osd_debug ? \
            "glcolorconvert ! gldownload ! clockoverlay text=XV valignment=center ! xvimagesink" : \
            "glcolorconvert ! gldownload ! xvimagesink";

    case OSD_RENDER_GL:
        return osd_debug ? \
            "clockoverlay text=GL valignment=center ! glimagesink" : \
            "glimagesink";

    case OSD_RENDER_AUTO:
    default:
        return osd_debug ? \
            "glcolorconvert ! gldownload ! clockoverlay text=Auto valignment=center ! autovideosink" : \
            "glcolorconvert ! gldownload ! autovideosink";
    }
}


int gst_main(int rtp_port, char *codec, int rtp_jitter, osd_render_t osd_render, int screen_width, char *rtsp_src)
{
    int screen_height = screen_width * 9 / 16;

    // Fix for intel hd graphics
    setenv("GST_GL_PLATFORM", "egl", 0);

    /* init GStreamer */
    gst_init (NULL, NULL);

    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    GstElement *pipeline = NULL;

    /* setup pipeline */
    {
        char *pipeline_str = NULL;
        char *src_str = NULL;
        GError *error = NULL;

        if(rtsp_src != NULL)
        {
            asprintf(&src_str,
                     "rtspsrc latency=%d location=\"%s\"", rtp_jitter, rtsp_src);
        } else {
            asprintf(&src_str,
                     "udpsrc port=%d caps=\"application/x-rtp,media=(string)video,  clock-rate=(int)90000, encoding-name=(string)H%s\"",
                     rtp_port, codec + 1);
        }

        char *codecs[] = {"nv%sdec", "avdec_%s", "v4l2%sdec"};
        char *decoder = NULL;

        for(int i = 0; i < sizeof(codecs) / sizeof(codecs[0]); i++)
        {
            char *buf = NULL;
            asprintf(&buf, codecs[i], codec);
            GstElement *tmp = gst_element_factory_make(buf, "decoder");

            if(tmp != NULL)
            {
                gst_object_unref(tmp);
                decoder = buf;
                break;
            }

            free(buf);
        }

        if(decoder == NULL)
        {
            fprintf(stderr, "No decoder for %s was found\n", codec);
            exit(1);
        }

        asprintf(&pipeline_str,
                 "%s ! "
                 "rtp%sdepay ! "
                 "%sparse config-interval=1 disable-passthrough=true ! "
                 "%s qos=false ! "
                 "queue leaky=downstream max-size-buffers=1 max-size-bytes=0 ! "
                 "glupload ! glcolorconvert ! "
                 "glvideomixerelement emit-signals=true start-time-selection=1 name=osd_mixer "
                 "sink_0::emit-signals=true sink_0::width=%d sink_0::height=%d sink_0::zorder=-2 "
                 "sink_1::emit-signals=true sink_1::width=%d sink_1::height=%d sink_1::zorder=0 "
#if LOCAL_CAMERA_SUPPORT
                 "sink_2::emit-signals=true sink_2::width=640 sink_2::height=360 sink_2::zorder=-1 "
#endif
                 "! %s sync=true "
                 "appsrc name=osd_src stream-type=0 format=time min-latency=0 ! "
                 "video/x-raw,format=RGBA,width=%d,height=%d,framerate=0/1 ! glupload ! glcolorconvert ! osd_mixer. "
#if LOCAL_CAMERA_SUPPORT
                 "v4l2src device=/dev/video2 ! video/x-raw,width=640,height=360,framerate=30/1 ! queue ! glupload ! glcolorconvert ! osd_mixer."
#endif
                 ,
                 src_str, codec, codec, decoder,
                 screen_width, screen_height, screen_width, screen_height,
                 select_osd_render(osd_render),
                 GRAPHICS_WIDTH, GRAPHICS_HEIGHT);

        free(src_str);
        free(decoder);

        printf("GST pipeline: %s\n", pipeline_str);

        pipeline = gst_parse_launch(pipeline_str, &error);
        free(pipeline_str);

        if(error != NULL)
        {
            fprintf (stderr, "GST Error: %s\n", error->message);
            g_error_free (error);
            exit(1);
        }
    }

    g_assert(pipeline);

    /* setup */
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "osd_src");
    g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), loop);

    // Set message handler
    {
        GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
        gst_bus_add_signal_watch (bus);
        g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);
        gst_object_unref (GST_OBJECT (bus));
    }

    // main loop
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);
    gst_element_set_state (pipeline, GST_STATE_NULL);

    gst_object_unref (pipeline);

    return 0;
}
