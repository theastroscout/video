#include <iostream>
#include <gst/gst.h>

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstPad *sinkpad;
    GstElement *pipeline = (GstElement *)data;
    GstElement *videoconvert = gst_bin_get_by_name(GST_BIN(pipeline), "videoconvert");
    GstElement *audio_convert = gst_bin_get_by_name(GST_BIN(pipeline), "audioconvert");

    GstCaps *caps = gst_pad_query_caps(pad, NULL);
    const GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);

    if (g_str_has_prefix(name, "video/x-raw")) {
        sinkpad = gst_element_get_static_pad(videoconvert, "sink");
    } else if (g_str_has_prefix(name, "audio/x-raw")) {
        sinkpad = gst_element_get_static_pad(audio_convert, "sink");
    } else {
        gst_object_unref(videoconvert);
        gst_object_unref(audio_convert);
        return;
    }

    if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link decoder to %s.\n", name);
    }

    gst_object_unref(sinkpad);
    gst_object_unref(videoconvert);
    gst_object_unref(audio_convert);
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            g_free(debug);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    GstElement *pipeline, *source, *decoder, *videoconvert, *videoscale, *capsfilter, *audio_convert, *video_encoder, *audio_encoder, *muxer, *sink;
    GstBus *bus;
    GMainLoop *loop;
    GstCaps *caps;

    // Create GStreamer elements
    pipeline = gst_pipeline_new("video-convert-pipeline");
    source = gst_element_factory_make("filesrc", "source");
    decoder = gst_element_factory_make("decodebin", "decoder");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    videoscale = gst_element_factory_make("videoscale", "videoscale");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    audio_convert = gst_element_factory_make("audioconvert", "audioconvert");
    video_encoder = gst_element_factory_make("vp8enc", "video_encoder");
    audio_encoder = gst_element_factory_make("vorbisenc", "audio_encoder");
    muxer = gst_element_factory_make("webmmux", "muxer");
    sink = gst_element_factory_make("filesink", "sink");

    // Check if all elements were created successfully
    if (!pipeline || !source || !decoder || !videoconvert || !videoscale || !capsfilter || !audio_convert || !video_encoder || !audio_encoder || !muxer || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // Set element properties
    g_object_set(source, "location", "input.mov", NULL);
    g_object_set(sink, "location", "output.webm", NULL);
    g_object_set(video_encoder, "target-bitrate", 1000, NULL); // 1M bitrate

    // Set the caps filter to resize the video
    caps = gst_caps_new_simple("video/x-raw",
                               "width", G_TYPE_INT, 1080,
                               "height", G_TYPE_INT, 1920,
                               NULL);
    g_object_set(capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, videoconvert, videoscale, capsfilter, audio_convert, video_encoder, audio_encoder, muxer, sink, NULL);

    // Link the source and decoder
    if (!gst_element_link(source, decoder)) {
        g_printerr("Source and decoder could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Link video elements
    if (!gst_element_link_many(videoconvert, videoscale, capsfilter, video_encoder, muxer, NULL)) {
        g_printerr("Video elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Link audio elements
    if (!gst_element_link_many(audio_convert, audio_encoder, muxer, NULL)) {
        g_printerr("Audio elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Link muxer to sink
    if (!gst_element_link(muxer, sink)) {
        g_printerr("Muxer and sink could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Connect the pad-added signal for dynamic linking
    g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), pipeline);

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Create a GLib Main Loop and set up the bus
    loop = g_main_loop_new(NULL, FALSE);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    // Run the main loop
    g_main_loop_run(loop);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
    return 0;
}
