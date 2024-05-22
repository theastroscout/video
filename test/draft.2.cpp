#include <iostream>

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
		: inputFilePath(inputFilePath), outputFilePath(outputFilePath), crf(20), cpuUsage(6), threadCount(6),
		  inputFormatContext(nullptr), videoDecoderContext(nullptr), audioDecoderContext(nullptr),
		  videoStreamIndex(-1), audioStreamIndex(-1) {
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
		/*
		std::cout << "\n\n\n\n";
		std::cout << "Setting audio bitrate to: " << bitrate << std::endl;
		std::cout << std::stoi(bitrate) << std::endl;
		std::cout << "\n\n\n\n";
		*/

		audioCodec = codec;
		audioBitrate = bitrate;
		
		// audioEncCtx->bit_rate = avutil_parse_bitrate(audioBitrate.c_str());
	}
	
	void setCpuUsage(int usage) {
		cpuUsage = usage;
	}
	
	void setThreads(int threadCount) {
		this->threadCount = threadCount;
	}
	
	void convert() {
		AVPacket *packet = av_packet_alloc();
		if (!packet)
			throw std::runtime_error("Failed to allocate packet.");

		AVFrame *frame = av_frame_alloc();
		if (!frame)
			throw std::runtime_error("Failed to allocate frame.");

		AVFrame *filt_frame = av_frame_alloc();
		if (!filt_frame)
			throw std::runtime_error("Failed to allocate filtered frame.");

		int ret;
		// Open output file
		AVFormatContext *outputFormatContext = nullptr;
		avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFilePath.c_str());
		if (!outputFormatContext)
			throw std::runtime_error("Could not create output format context.");

		// Create and open the codec contexts for the encoder
		AVCodec *videoEncoder = avcodec_find_encoder_by_name(videoCodec.c_str());
		AVCodec *audioEncoder = avcodec_find_encoder_by_name(audioCodec.c_str());

		if (!videoEncoder)
			throw std::runtime_error("Video encoder not found.");
		if (!audioEncoder)
			throw std::runtime_error("Audio encoder not found.");

		AVCodecContext *videoEncCtx = avcodec_alloc_context3(videoEncoder);
		AVCodecContext *audioEncCtx = avcodec_alloc_context3(audioEncoder);
		if (!videoEncCtx || !audioEncCtx)
			throw std::runtime_error("Failed to allocate codec contexts.");

		// Set encoder parameters
		videoEncCtx->height = videoDecoderContext->height;
		videoEncCtx->width = videoDecoderContext->width;
		videoEncCtx->sample_aspect_ratio = videoDecoderContext->sample_aspect_ratio;
		// the frame rate needs to be adjusted to the input stream's rate
		videoEncCtx->framerate = av_guess_frame_rate(inputFormatContext, inputFormatContext->streams[videoStreamIndex], nullptr);
		videoEncCtx->time_base = av_inv_q(videoEncCtx->framerate);
		videoEncCtx->pix_fmt = videoEncoder->pix_fmts[0];
		videoEncCtx->bit_rate = std::stoi(videoBitrate);

		audioEncCtx->sample_rate = audioDecoderContext->sample_rate;
		audioEncCtx->channel_layout = audioDecoderContext->channel_layout;
		audioEncCtx->channels = av_get_channel_layout_nb_channels(audioEncCtx->channel_layout);
		audioEncCtx->sample_fmt = audioEncoder->sample_fmts[0];
		audioEncCtx->bit_rate = std::stoi(audioBitrate);
		// audioEncCtx->bit_rate = avutil_parse_bitrate(audioBitrate.c_str());

		audioEncCtx->time_base = { 1, audioEncCtx->sample_rate };

		if (avcodec_open2(videoEncCtx, videoEncoder, nullptr) < 0)
			throw std::runtime_error("Failed to open video encoder.");
		if (avcodec_open2(audioEncCtx, audioEncoder, nullptr) < 0) {
			/*char error_buffer[255];
		    av_strerror(ret, error_buffer, sizeof(error_buffer));
		    std::cerr << "Failed to open audio encoder: " << error_buffer << std::endl;
		    */
		    throw std::runtime_error("Failed to open audio encoder.");
			// throw std::runtime_error("Failed to open audio encoder.");
		}

		// Add streams to output context
		AVStream *videoStream = avformat_new_stream(outputFormatContext, nullptr);
		if (!videoStream)
			throw std::runtime_error("Failed to create video stream.");
		avcodec_parameters_from_context(videoStream->codecpar, videoEncCtx);

		AVStream *audioStream = avformat_new_stream(outputFormatContext, nullptr);
		if (!audioStream)
			throw std::runtime_error("Failed to create audio stream.");
		avcodec_parameters_from_context(audioStream->codecpar, audioEncCtx);

		// Open output file
		if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
			ret = avio_open(&outputFormatContext->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE);
			if (ret < 0)
				throw std::runtime_error("Could not open output file.");
		}

		// Write file header
		if (avformat_write_header(outputFormatContext, nullptr) < 0)
			throw std::runtime_error("Error occurred when opening output file.");

		// Main processing loop (simplified version)
		while (av_read_frame(inputFormatContext, packet) >= 0) {
			// Check the stream index and process accordingly
			if (packet->stream_index == videoStreamIndex) {
				// Process video packet
			} else if (packet->stream_index == audioStreamIndex) {
				// Process audio packet
			}
			av_packet_unref(packet);
		}

		// Write the trailer to mux everything correctly
		if (av_write_trailer(outputFormatContext) < 0)
			throw std::runtime_error("Error writing AV trailer to output file.");

		// Cleanup local allocs
		av_packet_free(&packet);
		av_frame_free(&frame);
		av_frame_free(&filt_frame);

	    // Close input and output codecs
	    avcodec_close(videoDecoderContext);
	    avcodec_close(audioDecoderContext);
	    avcodec_close(videoEncCtx);
	    avcodec_close(audioEncCtx);

	    // Close input and output files
	    if (inputFormatContext) {
	        avformat_close_input(&inputFormatContext);
	    }
	    if (outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
	        avio_closep(&outputFormatContext->pb);
	    }

	    // Free the context
	    avformat_free_context(outputFormatContext);

	    // Final cleanup
	    avcodec_free_context(&videoEncCtx);
	    avcodec_free_context(&audioEncCtx);
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

	AVFormatContext* inputFormatContext;
	AVCodecContext* videoDecoderContext;
	AVCodecContext* audioDecoderContext;
	AVCodec* videoDecoder;
	AVCodec* audioDecoder;
	int videoStreamIndex;
	int audioStreamIndex;

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
		// Close the codec contexts if they have been opened
	    if (videoDecoderContext) {
	        avcodec_free_context(&videoDecoderContext);
	    }
	    if (audioDecoderContext) {
	        avcodec_free_context(&audioDecoderContext);
	    }

	    // Close the format context
	    if (inputFormatContext) {
	        avformat_close_input(&inputFormatContext);
	    }

	    // Free the format context itself
	    avformat_free_context(inputFormatContext);
	}
};


#endif // CONVERTER_HPP

int main(int argc, char* argv[]) {
	if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>" << std::endl;
        return 1;
    }

    std::string inputFilePath = argv[1];
    std::string outputFilePath = argv[2];

    try {
        // Create a Converter instance
        Converter converter(inputFilePath, outputFilePath);

        // Set video and audio codec settings
        converter.setVideoCodec("libvpx-vp9", 20, "1000k");
        converter.setAudioCodec("libvorbis", "128000");

        // Set additional processing settings like video filter, CPU usage, and thread count
        converter.setVideoFilter("scale=1080:-1, crop=1080:1920, fps=29");
        converter.setCpuUsage(6);
        converter.setThreads(6);

        // Perform the conversion
        converter.convert();

        std::cout << "Conversion completed successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during conversion: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}