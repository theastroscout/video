
#include <iostream>
#include <gst/gst.h>

// Callback to link the decodebin dynamically created source pad
void on_pad_added(GstElement* element, GstPad* pad, gpointer data) {
    GstPad* sink_pad = gst_element_get_static_pad((GstElement*)data, "sink");
    if (gst_pad_is_linked(sink_pad) == FALSE) {
        gst_pad_link(pad, sink_pad);
    }
    gst_object_unref(sink_pad);
}


int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create the elements
    auto* pipeline = gst_pipeline_new("video-converter");
    auto* source = gst_element_factory_make("filesrc", "source");
    auto* decodebin = gst_element_factory_make("decodebin", "decodebin");
    auto* scaler = gst_element_factory_make("videoscale", "scaler");
    auto* filter = gst_element_factory_make("capsfilter", "filter");
    auto* converter = gst_element_factory_make("videoconvert", "converter");
    auto* encoder = gst_element_factory_make("vp9enc", "encoder");
    auto* muxer = gst_element_factory_make("webmmux", "muxer");
    auto* sink = gst_element_factory_make("filesink", "sink");

    // Check for element creation failures
    if (!pipeline || !source || !decodebin || !scaler || !filter || !converter || !encoder || !muxer || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // Set element properties
    g_object_set(G_OBJECT(source), "location", "input.mov", NULL);
    g_object_set(G_OBJECT(sink), "location", "output.webm", NULL);

    // Video filter caps
    auto* caps = gst_caps_new_simple("video/x-raw",
                                     "width", G_TYPE_INT, 1080,
                                     "height", G_TYPE_INT, 1920,
                                     "framerate", GST_TYPE_FRACTION, 29, 1,
                                     NULL);
    g_object_set(G_OBJECT(filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    // Encoder settings
    g_object_set(G_OBJECT(encoder),
                 "target-bitrate", 1500000,
                 "cpu-used", 8,
                 "deadline", 1,  // realtime mode
                 "end-usage", 1, // VBR mode
                 NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, decodebin, scaler, filter, converter, encoder, muxer, sink, NULL);
    gst_element_link_many(source, decodebin, NULL);
    gst_element_link_many(scaler, filter, converter, encoder, muxer, sink, NULL);

    // Connect the dynamic pad of decodebin
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), scaler);

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Free resources
    if (msg != nullptr)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}