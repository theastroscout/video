#include <iostream>

// #ifndef VIDEO_CONVERTER_HPP
// #define VIDEO_CONVERTER_HPP

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavfilter/avfilter.h>
	#include <libavutil/avutil.h>
	#include <libavutil/opt.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
	#include <libavfilter/buffersrc.h>
	#include <libavfilter/buffersink.h>
}

#include <string>

class VideoConverter {
public:
	VideoConverter(const std::string& inputFilename, const std::string& outputFilename);
	~VideoConverter();

	bool configureInput();
	bool configureOutput();
	bool configureFilters();
	bool performConversion();

	bool flushEncoder();
    bool finalizeOutputFile();
    void cleanupFFmpeg();

private:
	std::string inputFilename;
	std::string outputFilename;

	AVFormatContext* inputFormatCtx = nullptr;
	AVFormatContext* outputFormatCtx = nullptr;
	AVCodecContext* inputCodecCtx = nullptr;
	AVCodecContext* outputCodecCtx = nullptr;
	AVFilterGraph* filterGraph = nullptr;

	AVFilterContext* buffersrc_ctx = nullptr;
    AVFilterContext* buffersink_ctx = nullptr;

	bool initFFmpeg();
	bool openInput();
	bool openOutput();
	bool setupDecoder(AVCodecContext*& codecCtx, AVStream* stream);
	bool setupEncoder(AVCodecContext*& codecCtx, AVStream* stream);
	bool initFilters();
	bool encodeAndWrite(AVFrame* frame);
	bool decodeAndFilter(AVFrame* frame);
	bool processFrame(AVFrame* frame);
	void logError(const std::string& error);
};

VideoConverter::VideoConverter(const std::string& inputFilename, const std::string& outputFilename)
    : inputFilename(inputFilename), outputFilename(outputFilename) {
    // Initialize FFmpeg components without deprecated calls
    initFFmpeg();
}

VideoConverter::~VideoConverter() {
    // Perform any necessary clean-up that isn't already handled
}

bool VideoConverter::initFFmpeg() {
    inputFormatCtx = avformat_alloc_context();
    outputFormatCtx = avformat_alloc_context();
    if (!inputFormatCtx || !outputFormatCtx) {
        logError("Failed to allocate format context");
        return false;
    }
    return true;
}

bool VideoConverter::configureInput() {
    if (!openInput()) {
        logError("Failed to open input file");
        return false;
    }

    // Assume video stream is always the first stream
    AVStream* inputStream = inputFormatCtx->streams[0];
    if (!setupDecoder(inputCodecCtx, inputStream)) {
        logError("Failed to set up decoder for input stream");
        return false;
    }

    return true;
}

bool VideoConverter::configureOutput() {
    if (!openOutput()) {
        logError("Failed to open output file");
        return false;
    }

    AVStream* outputStream = avformat_new_stream(outputFormatCtx, nullptr);
    if (!outputStream) {
        logError("Failed to create a new stream for output");
        return false;
    }

    if (!setupEncoder(outputCodecCtx, outputStream)) {
        logError("Failed to set up encoder for output stream");
        return false;
    }

    return true;
}

bool VideoConverter::configureFilters() {
    if (!initFilters()) {
        logError("Failed to initialize filters");
        return false;
    }
    return true;
}

bool VideoConverter::performConversion() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        logError("Failed to allocate frame");
        return false;
    }

    int ret;
    while (true) {
        ret = decodeAndFilter(frame);
        if (ret < 0) {  // decodeAndFilter should return an FFmpeg error code on failure
            if (ret == AVERROR_EOF) {  // Check for end of file
                break;
            }
            char error_buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, error_buf, AV_ERROR_MAX_STRING_SIZE);  // Get the error message
            logError("Error during frame processing: " + std::string(error_buf));
            av_frame_free(&frame);
            return false;
        }
    }

    av_frame_free(&frame); // Clean up the allocated frame after processing
    return true;
}






bool VideoConverter::openInput() {
    if (avformat_open_input(&inputFormatCtx, inputFilename.c_str(), nullptr, nullptr) != 0) {
        logError("Could not open input file");
        return false;
    }

    if (avformat_find_stream_info(inputFormatCtx, nullptr) < 0) {
        logError("Failed to find stream information");
        return false;
    }

    // Find the primary video stream
    int videoStreamIndex = -1;
    for (unsigned i = 0; i < inputFormatCtx->nb_streams; i++) {
        if (inputFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        logError("Could not find a video stream in the file");
        return false;
    }

    // Set up the codec context for the video stream
    AVStream* stream = inputFormatCtx->streams[videoStreamIndex];
    inputCodecCtx = avcodec_alloc_context3(nullptr);
    if (!inputCodecCtx) {
        logError("Failed to allocate codec context");
        return false;
    }

    if (avcodec_parameters_to_context(inputCodecCtx, stream->codecpar) < 0) {
        logError("Failed to copy codec parameters to codec context");
        return false;
    }

    AVCodec* codec = avcodec_find_decoder(inputCodecCtx->codec_id);
    if (!codec) {
        logError("Failed to find the codec");
        return false;
    }

    if (avcodec_open2(inputCodecCtx, codec, nullptr) < 0) {
        logError("Failed to open codec");
        return false;
    }

    return true;
}


bool VideoConverter::openOutput() {
    avformat_alloc_output_context2(&outputFormatCtx, nullptr, nullptr, outputFilename.c_str());
    if (!outputFormatCtx) {
        logError("Could not create output context");
        return false;
    }

    return true;
}

bool VideoConverter::setupDecoder(AVCodecContext*& codecCtx, AVStream* stream) {
    AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        logError("Decoder not found");
        return false;
    }

    codecCtx = avcodec_alloc_context3(decoder);
    if (!codecCtx) {
        logError("Failed to allocate the codec context");
        return false;
    }

    if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0) {
        logError("Failed to copy codec parameters to codec context");
        return false;
    }

    if (avcodec_open2(codecCtx, decoder, nullptr) < 0) {
        logError("Failed to open codec");
        return false;
    }

    return true;
}

/*

Setup Encoder
Configures the encoder for the output stream.

*/

bool VideoConverter::setupEncoder(AVCodecContext*& codecCtx, AVStream* stream) {
    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_VP9);
    if (!encoder) {
        logError("Encoder not found");
        return false;
    }

    codecCtx = avcodec_alloc_context3(encoder);
    if (!codecCtx) {
        logError("Failed to allocate the codec context");
        return false;
    }

    codecCtx->height = 1920;
    codecCtx->width = 1080;
    codecCtx->sample_aspect_ratio = stream->sample_aspect_ratio; // Keep original aspect ratio
    codecCtx->bit_rate = 1000000;  // Set bitrate to 1 Mbit/s or
    codecCtx->pix_fmt = encoder->pix_fmts[0];
    codecCtx->time_base = {1, 29};
    codecCtx->global_quality = av_q2d({20, 1}); // Set CRF to 20 using a quality scale


    if (avcodec_open2(codecCtx, encoder, nullptr) < 0) {
        logError("Failed to open encoder");
        return false;
    }

    return true;
}

bool VideoConverter::initFilters() {
    char args[512];
    filterGraph = avfilter_graph_alloc();
    if (!filterGraph) {
        logError("Unable to create filter graph");
        return false;
    }

    AVFilterContext* scale_ctx = nullptr;
    AVFilterContext* crop_ctx = nullptr;
    AVFilterContext* fps_ctx = nullptr;

    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    const AVFilter* scale = avfilter_get_by_name("scale");
    const AVFilter* crop = avfilter_get_by_name("crop");
    const AVFilter* fps = avfilter_get_by_name("fps");

    if (inputCodecCtx->width <= 0 || inputCodecCtx->height <= 0 || inputCodecCtx->pix_fmt == AV_PIX_FMT_NONE) {
	    logError("Invalid or uninitialized codec context parameters");
	    return false;
	}

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
             inputCodecCtx->width, inputCodecCtx->height, inputCodecCtx->pix_fmt,
             inputCodecCtx->time_base.num, inputCodecCtx->time_base.den);

    if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, nullptr, filterGraph) < 0) {
        logError("Cannot create buffer source");
        return false;
    }

    if (avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", nullptr, nullptr, filterGraph) < 0) {
        logError("Cannot create buffer sink");
        return false;
    }

    snprintf(args, sizeof(args), "scale=1080:-1");
    if (avfilter_graph_create_filter(&scale_ctx, scale, "scale", args, nullptr, filterGraph) < 0) {
        logError("Cannot create scale filter");
        return false;
    }

    snprintf(args, sizeof(args), "crop=1080:1920:0:0");
    if (avfilter_graph_create_filter(&crop_ctx, crop, "crop", args, nullptr, filterGraph) < 0) {
        logError("Cannot create crop filter");
        return false;
    }

    snprintf(args, sizeof(args), "fps=fps=29");
    if (avfilter_graph_create_filter(&fps_ctx, fps, "fps", args, nullptr, filterGraph) < 0) {
        logError("Cannot create fps filter");
        return false;
    }

    if (avfilter_link(buffersrc_ctx, 0, scale_ctx, 0) < 0 ||
        avfilter_link(scale_ctx, 0, crop_ctx, 0) < 0 ||
        avfilter_link(crop_ctx, 0, fps_ctx, 0) < 0 ||
        avfilter_link(fps_ctx, 0, buffersink_ctx, 0) < 0) {
        logError("Error connecting filters");
        return false;
    }

    if (avfilter_graph_config(filterGraph, nullptr) < 0) {
        logError("Error configuring the filter graph");
        return false;
    }

    return true;
}

bool VideoConverter::decodeAndFilter(AVFrame* frame) {
    AVPacket packet;
    int response;

    while (av_read_frame(inputFormatCtx, &packet) >= 0) {
        response = avcodec_send_packet(inputCodecCtx, &packet);
        if (response < 0) {
            logError("Failed to send packet to decoder");
            av_packet_unref(&packet); // Ensure packet is unreferenced even on failure
            return false;
        }

        while (response >= 0) {
            response = avcodec_receive_frame(inputCodecCtx, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            } else if (response < 0) {
                logError("Failed to receive frame from decoder");
                av_packet_unref(&packet); // Ensure packet is unreferenced even on failure
                return false;
            }

            if (!processFrame(frame)) {
                av_packet_unref(&packet); // Ensure packet is unreferenced even on failure
                return false;
            }
        }
        av_packet_unref(&packet);
    }
    return true;
}


bool VideoConverter::processFrame(AVFrame* frame) {
    if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        logError("Error adding frame to buffer source");
        return false;
    }

    AVFrame *filt_frame = av_frame_alloc();
    if (!filt_frame) {
        logError("Could not allocate filtered frame");
        return false;
    }

    while (true) {
        int ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break; // No more frames to process, exit loop
        }
        if (ret < 0) {
            av_frame_free(&filt_frame);
            logError("Error during filtering");
            return false;
        }

        if (!encodeAndWrite(filt_frame)) {
            av_frame_free(&filt_frame);
            return false;
        }
        av_frame_unref(filt_frame);
    }
    av_frame_free(&filt_frame); // Clean up the allocated frame
    return true;
}


bool VideoConverter::encodeAndWrite(AVFrame* frame) {
    AVPacket pkt;
    // av_init_packet(&pkt);
    pkt.data = nullptr;    // Packet data will be allocated by the encoder
    pkt.size = 0;

    int response = avcodec_send_frame(outputCodecCtx, frame);
    if (response < 0) {
        logError("Failed to send frame for encoding");
        return false;
    }

    while (response >= 0) {
        response = avcodec_receive_packet(outputCodecCtx, &pkt);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break; // No more packets to process from the encoder
        } else if (response < 0) {
            logError("Error during encoding");
            return false;
        }

        if (av_interleaved_write_frame(outputFormatCtx, &pkt) < 0) {
            av_packet_unref(&pkt);
            logError("Error while writing frame to output");
            return false;
        }
        av_packet_unref(&pkt);
    }
    return true;
}

bool VideoConverter::flushEncoder() {
    AVPacket pkt;
    // av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    // Send a NULL frame to the encoder to flush remaining frames
    int response = avcodec_send_frame(outputCodecCtx, nullptr);
    if (response < 0) {
        logError("Failed to send flush frame");
        return false;
    }

    while (response >= 0) {
        response = avcodec_receive_packet(outputCodecCtx, &pkt);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;  // No more packets to flush
        } else if (response < 0) {
            logError("Error during flushing encoder");
            return false;
        }

        if (av_interleaved_write_frame(outputFormatCtx, &pkt) < 0) {
            av_packet_unref(&pkt);
            logError("Error while writing flushed frame");
            return false;
        }
        av_packet_unref(&pkt);
    }
    return true;
}


bool VideoConverter::finalizeOutputFile() {
    if (av_write_trailer(outputFormatCtx) < 0) {
        logError("Error writing output file trailer");
        return false;
    }
    return true;
}


void VideoConverter::cleanupFFmpeg() {
    if (inputCodecCtx) {
        avcodec_free_context(&inputCodecCtx);
    }
    if (outputCodecCtx) {
        avcodec_free_context(&outputCodecCtx);
    }
    if (inputFormatCtx) {
        avformat_close_input(&inputFormatCtx);
    }
    if (outputFormatCtx) {
        if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&outputFormatCtx->pb);
        }
        avformat_free_context(outputFormatCtx);
    }
    if (filterGraph) {
        avfilter_graph_free(&filterGraph);
    }
}





void VideoConverter::logError(const std::string& error) {
    std::cerr << "Error: " << error << std::endl;
}

// #endif // VIDEO_CONVERTER_HPP

int main() {
	VideoConverter converter("input.mov", "output.webm");
	if (converter.configureInput() && converter.configureOutput() && converter.configureFilters()) {
	    if (converter.performConversion()) {
	        converter.flushEncoder();
	        converter.finalizeOutputFile();
	    }
	}
	converter.cleanupFFmpeg();
	return 0;
}