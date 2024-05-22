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
	#include <libavutil/imgutils.h>
	#include <libavutil/samplefmt.h>
}

#include <stdexcept>
#include <string>
#include <memory>
#include <iostream>

class VideoConverter {
public:
	VideoConverter(const std::string& inputFilename, const std::string& outputFilename);
	~VideoConverter();

	bool convert();

private:
	const std::string inputFilename;
	const std::string outputFilename;

	AVFormatContext* inputFormatCtx = nullptr;
	AVCodecContext* inputCodecCtx = nullptr;

	AVFormatContext* outputFormatCtx = nullptr;	
	AVCodecContext* outputCodecCtx = nullptr;

	AVFilterGraph* filterGraph = nullptr;


	void initFFmpeg();

	// Input
	bool openInput();
	bool setupDecoder(AVCodecContext*& codecCtx, AVStream* stream);

	// Output
	bool openOutput();	
	bool setupEncoder(AVCodecContext*& codecCtx, AVStream* stream);

	bool initFilters();
	bool processFrames();

	// Service
	void logError(const std::string& error);
};

VideoConverter::VideoConverter(const std::string& inputFilename, const std::string& outputFilename)
	: inputFilename(inputFilename), outputFilename(outputFilename) {
	
	initFFmpeg();

}

void VideoConverter::initFFmpeg() {
	avformat_network_init();  // Only necessary if network streaming is involved

	// Open input file
	if (avformat_open_input(&inputFormatCtx, inputFilename.c_str(), nullptr, nullptr) < 0) {
		throw std::runtime_error("Failed to open input file: " + inputFilename);
	}

	// Retrieve stream information
	if (avformat_find_stream_info(inputFormatCtx, nullptr) < 0) {
		avformat_close_input(&inputFormatCtx);
		throw std::runtime_error("Failed to find stream information for file: " + inputFilename);
	}

	// Open output file
	AVOutputFormat* outputFmt = av_guess_format(nullptr, outputFilename.c_str(), nullptr);
	if (!outputFmt) {
		throw std::runtime_error("Failed to determine output format for file: " + outputFilename);
	}

	outputFormatCtx = avformat_alloc_context();
	if (!outputFormatCtx) {
		throw std::runtime_error("Failed to allocate output format context");
	}

	outputFormatCtx->oformat = outputFmt;
	snprintf(outputFormatCtx->filename, sizeof(outputFormatCtx->filename), "%s", outputFilename.c_str());

	if (!(outputFmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&outputFormatCtx->pb, outputFilename.c_str(), AVIO_FLAG_WRITE) < 0) {
			throw std::runtime_error("Failed to open output file: " + outputFilename);
		}
	}
}

VideoConverter::~VideoConverter() {
	if (inputFormatCtx) {
		avformat_close_input(&inputFormatCtx);
	}
	if (outputFormatCtx) {
		if (outputFormatCtx->pb) {
			avio_closep(&outputFormatCtx->pb);
		}
		avformat_free_context(outputFormatCtx);
	}
	avformat_network_deinit();
}

bool VideoConverter::convert() {
	/*
	try {
		if (!initFFmpeg()) {
			throw std::runtime_error("Failed to initialise FFmpeg");
		}
		if (!openInput()) {
			throw std::runtime_error("Failed to open input file");
		}
		if (!setupDecoder(inputCodecCtx, inputFormatCtx->streams[video_stream_index])) {
			throw std::runtime_error("Failed to setup decoder");
		}
		if (!openOutput()) {
			throw std::runtime_error("Failed to open output file");
		}
		if (!setupEncoder(outputCodecCtx, outputFormatCtx->streams[video_stream_index])) {
			throw std::runtime_error("Failed to setup encoder");
		}
		if (!initFilters()) {
			throw std::runtime_error("Failed to initialise filters");
		}
		if (!processFrames()) {
			throw std::runtime_error("Failed to process frames");
		}
	} catch (const std::runtime_error& e) {
		logError(e.what());
		return false;
	}
	*/
	return true;
}


int main() {
	// VideoConverter converter("input.mov", "output.webm");
	
	return 0;
}