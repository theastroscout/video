
#include <gst/gst.h>
#include <iostream>
extern "C" {
// Function to link elements with dynamic pads
static void on_pad_added(GstElement* src, GstPad* new_pad, gpointer data) {
	GstPad* sink_pad = gst_element_get_static_pad((GstElement*)data, "sink");
	GstPadLinkReturn ret;
	GstCaps* new_pad_caps = NULL;
	GstStructure* new_pad_struct = NULL;
	const gchar* new_pad_type = NULL;

	if (gst_pad_is_linked(sink_pad)) {
		g_object_unref(sink_pad);
		return;
	}

	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);

	if (g_str_has_prefix(new_pad_type, "video/x-raw")) {
		ret = gst_pad_link(new_pad, sink_pad);
		if (GST_PAD_LINK_FAILED(ret)) {
			std::cerr << "Type is " << new_pad_type << " but link failed.\n";
		} else {
			std::cout << "Link succeeded (type " << new_pad_type << ").\n";
		}
	}

	if (new_pad_caps != NULL)
		gst_caps_unref(new_pad_caps);

	gst_object_unref(sink_pad);
}

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	GstElement *pipeline, *source, *decodebin, *scaler, *filter, *converter, *encoder, *muxer, *sink;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	bool is_active = true;

	// Initialize elements
	pipeline = gst_pipeline_new("video-converter");
	source = gst_element_factory_make("filesrc", "source");
	decodebin = gst_element_factory_make("decodebin", "decodebin");
	scaler = gst_element_factory_make("videoscale", "scaler");
	filter = gst_element_factory_make("capsfilter", "filter");
	converter = gst_element_factory_make("videoconvert", "converter");
	encoder = gst_element_factory_make("vp9enc", "encoder");
	muxer = gst_element_factory_make("webmmux", "muxer");
	sink = gst_element_factory_make("filesink", "sink");

	if (!pipeline || !source || !decodebin || !scaler || !filter || !converter || !encoder || !muxer || !sink) {
		g_printerr("Not all elements could be created.\n");
		return -1;
	}

	g_object_set(G_OBJECT(source), "location", "input.mov", NULL);
	g_object_set(G_OBJECT(sink), "location", "output.webm", NULL);
	g_object_set(G_OBJECT(encoder), "target-bitrate", 1500000, "cpu-used", 8, "deadline", 1, NULL);

	// Scale



	auto* crop = gst_element_factory_make("videocrop", "crop");

	// Add it to the pipeline
	gst_bin_add(GST_BIN(pipeline), crop);

	// Insert it into the existing element chain
	// gst_element_link_many(scaler, filter, crop, converter, NULL);

	// Set up the caps for scaling
	
	auto* scale_caps = gst_caps_new_simple("video/x-raw",
										   "width", G_TYPE_INT, 1080,
										   "height", G_TYPE_INT, gst_util_uint64_scale_int(1080, 16, 9), // Dynamic height based on aspect ratio
										   "pixel-aspect-ratio", GST_TYPE_FRACTION, 9, 16,
										   NULL);
	
	g_object_set(G_OBJECT(filter), "caps", scale_caps, NULL);
	gst_caps_unref(scale_caps);

	// Set the properties for cropping (to crop the video after scaling)
	/*
	g_object_set(G_OBJECT(crop),
				 "top", G_TYPE_INT, 0,
				 "left", G_TYPE_INT, 0,
				 "right", G_TYPE_INT, 0,
				 "bottom", G_TYPE_INT, gst_util_uint64_scale_int(1080, 9, 16) - 1920, // Adjust based on your requirements
				 NULL);
	*/
	/*
	g_object_set(G_OBJECT(crop),
		"top", G_TYPE_INT, 0,
		"left", G_TYPE_INT, (gst_util_uint64_scale_int(1920, 9, 16) - 1080) / 2, // Center crop the width
		"right", G_TYPE_INT, (gst_util_uint64_scale_int(1920, 9, 16) - 1080) / 2,
		"bottom", G_TYPE_INT, 0,
		NULL);*/











	gst_bin_add_many(GST_BIN(pipeline), source, decodebin, scaler, filter, converter, encoder, muxer, sink, NULL);
	gst_element_link(source, decodebin);
	gst_element_link_many(scaler, filter, converter, encoder, muxer, sink, NULL);

	g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), scaler);

	ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Failed to start the pipeline.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	// Start the message loop
	bus = gst_element_get_bus(pipeline);
	do {
		msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
		if (msg != NULL) {
			GError *err;
			gchar *debug_info;

			switch (GST_MESSAGE_TYPE(msg)) {
				case GST_MESSAGE_ERROR:
					gst_message_parse_error(msg, &err, &debug_info);
					g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
					g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
					g_clear_error(&err);
					g_free(debug_info);
					is_active = false;
					break;
				case GST_MESSAGE_EOS:
					g_print("End-Of-Stream reached.\n");
					is_active = false;
					break;
				case GST_MESSAGE_STATE_CHANGED:
					// We are only interested in state-changed messages from the pipeline
					if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
						GstState old_state, new_state, pending_state;
						gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
						g_print("Pipeline state changed from %s to %s:\n",
								gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
					}
					break;
				default:
					g_printerr("Unexpected message received.\n");
					break;
			}
			gst_message_unref(msg);
		}
	} while (is_active);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(bus);
	gst_object_unref(pipeline);
	return 0;
}
}