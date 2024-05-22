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

	std::unique_ptr<AVFormatContext, decltype(&avformat_free_context)> inputFormatCtx = {nullptr, avformat_free_context};
	std::unique_ptr<AVFormatContext, decltype(&avformat_free_context)> outputFormatCtx = {nullptr, avformat_free_context};
	
	std::unique_ptr<AVCodecContext, void (*)(AVCodecContext*)> inputCodecCtx{
		nullptr,
		[](AVCodecContext* ctx) {
			avcodec_free_context(&ctx);
		}
	};
	
	std::unique_ptr<AVCodecContext, void (*)(AVCodecContext*)> outputCodecCtx{
		nullptr,
		[](AVCodecContext* ctx) {
			avcodec_free_context(&ctx);
		}
	};

	std::unique_ptr<AVFilterGraph, void (*)(AVFilterGraph*)> filterGraph{
		nullptr,
		[](AVFilterGraph* ptr) {
			avfilter_graph_free(&ptr);
		}
	};

	AVFilterContext* buffersrc_ctx = nullptr;
	AVFilterContext* buffersink_ctx = nullptr;

	bool initFFmpeg();
	bool openInput();
	bool openOutput();
	bool setupDecoder(AVCodecContext*& codecCtx, AVStream* stream);
	bool setupEncoder(AVCodecContext*& codecCtx, AVStream* stream);
	bool initFilters();
	bool processFrames();
	void logError(const std::string& error);
};

VideoConverter::VideoConverter(const std::string& inputFilename, const std::string& outputFilename)
	: inputFilename(inputFilename), outputFilename(outputFilename) {
	avformat_network_init();  // This is ideally called once per application, consider relocating if multiple instances are used

	// Initialize input and output format contexts using the provided custom deleter
	inputFormatCtx.reset(avformat_alloc_context());
	outputFormatCtx.reset(avformat_alloc_context());

	if (!openInput()) {
		logError("Failed to open input file");
		throw std::runtime_error("Initialization failed at openInput");
	}

	if (!openOutput()) {
		logError("Failed to open output file");
		throw std::runtime_error("Initialization failed at openOutput");
	}

	if (!initFilters()) {
		logError("Failed to initialize filters");
		throw std::runtime_error("Initialization failed at initFilters");
	}
}


VideoConverter::~VideoConverter() {
	avformat_network_deinit();  // Consider global deinit if used across multiple converters

	if (inputFormatCtx) {
		AVFormatContext* tmpInputCtx = inputFormatCtx.get();  // Get the pointer
		avformat_close_input(&tmpInputCtx); // Pass address of the local variable
		inputFormatCtx.reset(); // Reset since the input is now closed
	}

	if (outputFormatCtx) {
		av_write_trailer(outputFormatCtx.get());
		AVFormatContext* tmpOutputCtx = outputFormatCtx.get();
		avformat_close_input(&tmpOutputCtx);                // Similarly, handle output format context
		outputFormatCtx.reset();                            // Reset the unique_ptr
	}
}

void VideoConverter::logError(const std::string& error) {
	// Log an error message to the standard error stream
	std::cerr << "Error: " << error << std::endl;
}


bool VideoConverter::openInput() {
    // Release the unique_ptr temporarily to avoid undefined behavior
    AVFormatContext* ctx = inputFormatCtx.release();

    // Attempt to open the input file
    if (avformat_open_input(&ctx, inputFilename.c_str(), nullptr, nullptr) != 0) {
        // If opening the file fails, reset the unique_ptr without modifying it
        inputFormatCtx.reset(ctx);
        return false;
    }

    // Reset the unique_ptr with the potentially modified context
    inputFormatCtx.reset(ctx);

    // Load stream information into the AVFormatContext
    if (avformat_find_stream_info(inputFormatCtx.get(), nullptr) < 0) {
        return false;
    }
    return true;
}

bool VideoConverter::openOutput() {
    // Temporary release the unique_ptr control
    AVFormatContext* ctx = outputFormatCtx.release();

    // Allocating the output context based on the output file name
    if (avformat_alloc_output_context2(&ctx, nullptr, nullptr, outputFilename.c_str()) < 0) {
        outputFormatCtx.reset(ctx);  // If failure, reset the unique_ptr to manage the context again
        return false;
    }

    // Reset the unique_ptr with the potentially modified context
    outputFormatCtx.reset(ctx);

    // Assuming here you also need to create an output stream based on the encoder
    AVStream* stream = avformat_new_stream(ctx, nullptr);
    if (!stream) {
        return false;  // Error handling if stream cannot be added
    }

    // Additional configurations here if necessary

    return true;
}


bool VideoConverter::initFilters() {
	char args[512];
	filterGraph.reset(avfilter_graph_alloc());
	if (!filterGraph) {
		return false;
	}

	snprintf(args, sizeof(args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			inputCodecCtx->width, inputCodecCtx->height, inputCodecCtx->pix_fmt,
			inputCodecCtx->time_base.num, inputCodecCtx->time_base.den,
			inputCodecCtx->sample_aspect_ratio.num, inputCodecCtx->sample_aspect_ratio.den);

	// Create buffer source and buffer sink, add to filter graph, etc.
	// Check FFmpeg documentation for detailed setup.
	return true;
}

bool VideoConverter::convert() {
	if (!initFFmpeg()) {
		logError("Failed to initialize FFmpeg components");
		return false;
	}

	// Decode, filter, and encode process
	// This will involve reading frames, sending them to the decoder,
	// processing them through filters, and encoding to the output.
	return true;
}



// #endif // VIDEO_CONVERTER_HPP

int main() {
	VideoConverter converter("input.mov", "output.webm");
	
	return 0;
}