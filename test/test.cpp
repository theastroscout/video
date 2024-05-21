#ifndef CONVERTER_HPP
#define CONVERTER_HPP

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavfilter/avfilter.h>
	#include <libavutil/opt.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/time.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/pixfmt.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#include <string>
#include <stdexcept>

class Converter {
public:
	Converter(const std::string& inputFilePath, const std::string& outputFilePath)
		: inputFilePath(inputFilePath), outputFilePath(outputFilePath), crf(20), cpuUsage(6), threadCount(6) {
		init();
	}
	
	void setVideoFilter(const std::string& filter) {
		videoFilter = filter;
	}
	
	void setVideoCodec(const std::string& codec, int crf, const std::string& bitrate) {
		videoCodec = codec;
		this->crf = crf;
		videoBitrate = bitrate;
	}
	
	void setAudioCodec(const std::string& codec, const std::string& bitrate) {
		audioCodec = codec;
		audioBitrate = bitrate;
	}
	
	void setCpuUsage(int usage) {
		cpuUsage = usage;
	}
	
	void setThreads(int threadCount) {
		this->threadCount = threadCount;
	}
	
	void convert() {
		// Conversion logic using FFmpeg goes here
	}
	
	~Converter() {
		cleanup();
	}

private:
	std::string inputFilePath;
	std::string outputFilePath;
	std::string videoFilter;
	std::string videoCodec;
	int crf;
	std::string videoBitrate;
	std::string audioCodec;
	std::string audioBitrate;
	int cpuUsage;
	int threadCount;

	void init() {
		// Initialize FFmpeg library
		avformat_network_init();

		// Open input file and allocate format context
		if (avformat_open_input(&inputFormatContext, inputFilePath.c_str(), nullptr, nullptr) < 0) {
			throw std::runtime_error("Could not open input file.");
		}

		// Retrieve stream information
		if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
			throw std::runtime_error("Could not find stream information.");
		}

		// Dump input information
		av_dump_format(inputFormatContext, 0, inputFilePath.c_str(), 0);

		// Find the best video stream
		videoStreamIndex = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
		if (videoStreamIndex < 0) {
			throw std::runtime_error("Could not find video stream in the input file.");
		}

		// Find the best audio stream
		audioStreamIndex = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioDecoder, 0);
		if (audioStreamIndex < 0) {
			throw std::runtime_error("Could not find audio stream in the input file.");
		}

		// Allocate codec contexts
		videoDecoderContext = avcodec_alloc_context3(videoDecoder);
		if (!videoDecoderContext) {
			throw std::runtime_error("Could not allocate video codec context.");
		}
		audioDecoderContext = avcodec_alloc_context3(audioDecoder);
		if (!audioDecoderContext) {
			throw std::runtime_error("Could not allocate audio codec context.");
		}

		// Copy codec parameters from input stream to output codec context
		if (avcodec_parameters_to_context(videoDecoderContext, inputFormatContext->streams[videoStreamIndex]->codecpar) < 0) {
			throw std::runtime_error("Could not copy video codec parameters.");
		}
		if (avcodec_parameters_to_context(audioDecoderContext, inputFormatContext->streams[audioStreamIndex]->codecpar) < 0) {
			throw std::runtime_error("Could not copy audio codec parameters.");
		}

		// Initialize the video decoder
		if (avcodec_open2(videoDecoderContext, videoDecoder, nullptr) < 0) {
			throw std::runtime_error("Could not open video decoder.");
		}
		// Initialize the audio decoder
		if (avcodec_open2(audioDecoderContext, audioDecoder, nullptr) < 0) {
			throw std::runtime_error("Could not open audio decoder.");
		}
	}
	
	void cleanup() {
		// Cleanup logic goes here
	}
};

#endif // CONVERTER_HPP

int main() {
	return 0;
}