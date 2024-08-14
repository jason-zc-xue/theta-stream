/*
  Copyright 2020 K. Takeo. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.
  3. Neither the name of the author nor other contributors may be
  used to endorse or promote products derived from this software
  without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/select.h>
#include <unistd.h>

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include "libuvc/libuvc.h"
#include "thetauvc.h"

#define MAX_PIPELINE_LEN 1024

struct gst_src {
    GstElement* pipeline;
    GstElement* appsrc;

    GMainLoop* loop;
    GTimer* timer;
    guint framecount;
    guint id;
    guint bus_watch_id;
    uint32_t dwFrameInterval;
    uint32_t dwClockFrequency;
};

struct gst_src src;

/**
 * @brief A callback function that is called when the GStreamer main loop
 * publishes a message to GstBus (usually just errors).
 *
 * @param bus
 * @param message
 * @param data
 * @return gboolean
 */
static gboolean gst_bus_cb(GstBus* bus, GstMessage* message, gpointer data) {
    GError* err;
    gchar* dbg;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(message, &err, &dbg);
            g_print("Error: %s\n", err->message);
            /* ----- BEGIN JCODE ----- */
            // Output debug info if available
            g_print("Debug: %s\n", (dbg) ? dbg : "none");
            /* ----- END JCODE ----- */
            g_error_free(err);
            g_free(dbg);
            g_main_loop_quit(src.loop);
            break;

        default:
            break;
    }

    return TRUE;
}

/**
 * @brief This function completes the GStreamer pipeline using the `char *pipeline`
 * string to tack onto the end of the appsrc processing needed in gst_viewer.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param pipeline The GStreamer pipeline to run
 * @return int Whether the pipeline succeeded
 */
int gst_src_init(int* argc, char*** argv, char* pipeline) {
    GstCaps* caps;
    GstBus* bus;
    char pipeline_str[MAX_PIPELINE_LEN];

    /* ----- BEGIN JCODE ----- */
    /*
     * "appsrc" is the element used to import a video stream from a non-default
     * source, such as libuvc in this case.
     *
     * The "h264parse" element here is used to make the video stream caps
     * (capabilities) compatible with the pads of the following elements in the
     * pipeline. Without h264parse, most elements (except fakesink and other
     * non-picky elements like queue) will throw the "Internal Data Stream"
     * error when directly following the appsrc.
     */
    snprintf(pipeline_str, MAX_PIPELINE_LEN,
             "appsrc name=ap ! queue  ! h264parse ! %s ", pipeline);
    /* ----- END JCODE ----- */

    gst_init(argc, argv);
    src.timer = g_timer_new();
    src.loop = g_main_loop_new(NULL, TRUE);
    src.pipeline = gst_parse_launch(pipeline_str, NULL);

    g_assert(src.pipeline);
    if (src.pipeline == NULL) {
        return FALSE;
    }
    gst_pipeline_set_clock(GST_PIPELINE(src.pipeline),
                           gst_system_clock_obtain());

    src.appsrc = gst_bin_get_by_name(GST_BIN(src.pipeline), "ap");

    caps = gst_caps_new_simple("video/x-h264", "framerate", GST_TYPE_FRACTION,
                               30000, 1001, "stream-format", G_TYPE_STRING,
                               "byte-stream", "profile", G_TYPE_STRING,
                               "constrained-baseline", NULL);
    gst_app_src_set_caps(GST_APP_SRC(src.appsrc), caps);

    bus = gst_pipeline_get_bus(GST_PIPELINE(src.pipeline));
    src.bus_watch_id = gst_bus_add_watch(bus, gst_bus_cb, NULL);
    gst_object_unref(bus);
    return TRUE;
}

/**
 * @brief Awaits a keypress (any, but more specifically, a LF/CRLF will trigger
 * this) which then closes the GStreamer blocking loop and ends the program.
 *
 * @param arg Value of the pressed key
 * @return void* ???
 */
void* keywait(void* arg) {
    struct gst_src* s;
    char keyin[4];

    read(1, keyin, 1);

    s = (struct gst_src*)arg;
    g_main_loop_quit(s->loop);
    return nullptr;
}

/**
 * @brief A callback that pushes UVC data to the GStreamer pipeline through the
 * appsrc element one frame at a time (when recieved).
 *
 * @param frame Video frame
 * @param ptr ???
 */
void cb(uvc_frame_t* frame, void* ptr) {
    struct gst_src* s;
    GstBuffer* buffer;
    GstFlowReturn ret;
    GstMapInfo map;
    gdouble ms;
    uint32_t pts;

    s = (struct gst_src*)ptr;
    ms = g_timer_elapsed(s->timer, NULL);

    buffer = gst_buffer_new_allocate(NULL, frame->data_bytes, NULL);
    GST_BUFFER_PTS(buffer) = frame->sequence * s->dwFrameInterval * 100;
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = s->dwFrameInterval * 100;
    GST_BUFFER_OFFSET(buffer) = frame->sequence;
    s->framecount++;

    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy(map.data, frame->data, frame->data_bytes);
    gst_buffer_unmap(buffer, &map);

    g_signal_emit_by_name(s->appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        fprintf(stderr, "pushbuffer errorn");
    }
    return;
}

/**
 * @brief Prints help text for the executable.
 */
void print_help() {
    std::cout
        << "\n"
        << "Usage: gst_viewer -p <PROFILE> [OPTIONS]\n"
        << "Options:\n"
        << "  -h, --help          Show this help message.\n"
        << "  -a, --addr ADDR     Which host to stream to (e.g., IP, device).\n"
        << "  -s, --stream STR    Which streaming protocol to use.\n"
        << "                        (select from: auto, rtsp, udp, v4l2)\n"
        << "  -p, --profile STR   Which preset pipeline profile to use.\n"
        << "                        (select from: loopback, viewer, streamer)\n"
        << "  -P, --pipeline      Specify a custom pipeline.\n";
}

/**
 * @brief Main function of the executable.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return int Return status, where 1: invalid option, 0: success, -1: error
 */
int main(int argc, char** argv) {
    /* ----- BEGIN JCODE ----- */
    std::string pre;  // only set based on `profile`

    std::string stream;       // user input
    bool needs_addr = false;  // derived from `stream`
    std::string addr_type;    // derived from `stream`
    std::string addr;         // user input
    std::string port_type;    // derived from `stream`
    std::string port;         // default=5000, but only set based on `stream`
    std::string stream_opts;  // only set based on `profile`

    bool profile_given = false;   // derived from
    std::string custom_pipeline;  // user input (overrides all others)

    for (int i = 1; i < argc; ++i) {
        // Get option name (and value, if it exists)
        std::string arg(argv[i]);
        std::string val;
        if (i + 1 < argc) {
            val = (argv[++i]);
        }

        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;

        } else if ((arg == "-a" || arg == "--addr")) {
            addr = val;

        } else if ((arg == "-s" || arg == "--stream")) {
            if (val == "auto") {
                // Note: addr_type is not used by autovideosink
                val += "video";  // full name is "autovideo"
            } else if (val == "rtsp") {
                needs_addr = true;
                addr_type = "location=";
            } else if (val == "udp") {
                needs_addr = true;
                addr_type = "host=";
                port_type = "port=";
                port = "5000";  // default
            } else if (val == "v4l2") {
                needs_addr = true;
                addr_type = "device=";
            } else {
                std::cerr << "[ERROR] Unknown streaming protocol: " << val
                          << "\n";
                print_help();
                return 1;
            }
            stream = val + "sink";

        } else if ((arg == "-p" || arg == "--profile")) {
            if (val == "loopback") {
                // Used to sink the parsed H264 stream into a v4l2loopback virtual video device
                pre = "queue";
                stream = "v4l2sink";
                addr_type = "device=";
                addr = "/dev/video1";
                stream_opts = "sync=false";
            } else if (val == "viewer") {
                // Used to test basic functionality
                pre = "queue ! decodebin";
                stream = "autovideosink";
                stream_opts = "sync=false";
            } else if (val == "streamer") {
                // Creates an RTSP server
                pre = "video/x-h264,width=3840,height=1920,framerate=30000/"
                      "1001 ! rtspclientsink";
                stream = "udpsink";
                addr_type = "location=";
                addr = "rtsp://192.168.43.90:8554/stream";
                stream_opts = "";
            } else {
                std::cerr << "[ERROR] Unknown profile: " << val << "\n";
                print_help();
                return 1;
            }
            profile_given = true;
            break;  // ignore any other options and exit loop

        } else if (arg == "-P" || arg == "--pipeline") {
            custom_pipeline = val;

        } else {
            std::cerr << "[ERROR] Unknown option: " << arg << "\n";
            print_help();
            return 1;
        }
    }

    std::string pipeline;
    if (!custom_pipeline.empty()) {
        // Skip further input processing if a custom pipeline was provided
        pipeline = custom_pipeline;
    } else {
        // Check mutually-inclusive variables
        if (!profile_given && stream.empty()) {
            std::cerr << "[ERROR] Missing required args!\n"
                      << "Please specify either a profile or a streaming "
                         "protocol + address\n";
            print_help();
            return 1;
        }
        if (needs_addr && addr.empty()) {
            std::cerr << "[ERROR] Streamer \"" << stream
                      << "\" requires an address!\n";
            print_help();
            return 1;
        }

        // Set defaults
        if (stream.empty()) {
            std::string val("udp");
            std::cout
                << "[WARN] Streaming protocol not specified; defaulting to "
                << stream << "\n";
            stream = val + "sink";
        }
        if (addr.empty() && stream != "autovideosink") {
            addr_type = "host=";
            addr = "192.168.43.111";
            port_type = "port=";
            port = "5000";  // default
            std::cout << "[WARN] Sink address not specified; defaulting to "
                      << addr << ":" << port << "\n";
        }

        // Form the pipeline string
        std::ostringstream oss;
        oss << " ";
        // - Preamble
        if (!pre.empty()) {
            oss << pre << " ! ";
        }
        // - Streaming
        oss << stream;
        if (!addr.empty()) {
            oss << " " << addr_type << addr;
        }
        if (!port.empty()) {
            oss << " " << port_type << port;
        }
        if (!stream_opts.empty()) {
            oss << " " << stream_opts;
        }
        pipeline = oss.str();
    }

    // FOR DEBUGGING PURPOSES ONLY
    std::cout << "[DEBUG] Using the following pipeline ending:\n\"" << pipeline
              << "\"\n";
    /* ----- END JCODE ----- */

    uvc_context_t* ctx;
    uvc_device_t* dev;
    uvc_device_t** devlist;
    uvc_device_handle_t* devh;
    uvc_stream_ctrl_t ctrl;
    uvc_error_t res;

    pthread_t thr;
    pthread_attr_t attr;

    struct gst_src* s;
    int idx;
    /* ----- BEGIN JCODE ----- */
    // Only placing this here so as to not break up the variable initialization
    // of the original program.
    char* pipe_proc = new char[pipeline.size() + 1];
    strcpy(pipe_proc, pipeline.c_str());
    /* ----- END JCODE ----- */
    char* cmd_name;

    cmd_name = rindex(argv[0], '/');
    if (cmd_name == NULL) {
        cmd_name = argv[0];
    } else {
        cmd_name++;
    }

    if (!gst_src_init(&argc, &argv, pipe_proc)) {
        return -1;
    }

    res = uvc_init(&ctx, NULL);
    if (res != UVC_SUCCESS) {
        uvc_perror(res, "[ERROR] The uvc_init call failed!");
        return res;
    }

    src.framecount = 0;
    res = thetauvc_find_device(ctx, &dev, 0);
    if (res != UVC_SUCCESS) {
        std::cerr << "[ERROR] THETA not found!\n"
                  << "Check whether the THETA is connected, powered on, and on "
                     "LIVE mode.\n";
        return res;
    }

    res = uvc_open(dev, &devh);
    if (res != UVC_SUCCESS) {
        // NOTE: libuvc-theta by ricohapi is different from libuvc, but the
        // README.md in the libuvc-theta repo is -directly- copied from libuvc.
        // So, funnily enough, even the git clone url is incorrect for use with
        // the THETA V camera.
        std::cerr << "[ERROR] Can't open THETA!\n"
                  << "Ensure libuvc-theta is being used rather than standard "
                     "libuvc.\n";
        uvc_exit(ctx);
        return res;
    }

    gst_element_set_state(src.pipeline, GST_STATE_PLAYING);
    pthread_create(&thr, NULL, keywait, &src);

    res = thetauvc_get_stream_ctrl_format_size(devh, THETAUVC_MODE_UHD_2997,
                                               &ctrl);
    src.dwFrameInterval = ctrl.dwFrameInterval;
    src.dwClockFrequency = ctrl.dwClockFrequency;

    res = uvc_start_streaming(devh, &ctrl, cb, &src, 0);
    if (res == UVC_SUCCESS) {
        fprintf(stderr, "start, hit any key to stop\n");
        g_main_loop_run(src.loop);

        fprintf(stderr, "stop\n");
        uvc_stop_streaming(devh);

        gst_element_set_state(src.pipeline, GST_STATE_NULL);
        g_source_remove(src.bus_watch_id);
        g_main_loop_unref(src.loop);

        pthread_cancel(thr);
        pthread_join(thr, NULL);
    }

    uvc_close(devh);
    uvc_exit(ctx);
    return res;
}
