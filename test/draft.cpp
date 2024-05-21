/*

g++ test.cpp -o test.app --std=c++20 -lavformat -lavcodec -lavfilter -lavdevice -lavutil -lswscale -lswresample

if CCACHE_DISABLE=1 g++ tiles.cpp -o tiles.app --std=c++20 -lsqlite3 -Werror -Wfatal-errors; then

ffmpeg -hwaccel cuda -hwaccel_output_format cuda -i "input_file.mp4"
-c:a copy -vf "scale_cuda=-2:480" -c:v h264_nvenc "output_file.mp4"


*/

#include <iostream>

extern "C" {
	#include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

int main() {
	avformat_network_init();
    AVFormatContext* pFormatContext = avformat_alloc_context();

    if (!pFormatContext) {
        // Handle error
        return -1;
    }

    AVPacket* pPacket = av_packet_alloc();
if (!pPacket) {
    // Handle error
    return -1;
}



    // Open output file
	AVFormatContext* pOutputFormatContext = NULL;
	if (avformat_alloc_output_context2(&pOutputFormatContext, NULL, NULL, "output.mp4") < 0) {
	    // Handle error
	    return -1;
	}

	// Find or create the appropriate codec for the output format
	AVCodec* pOutputCodec = avcodec_find_encoder(pOutputFormatContext->oformat->video_codec);
	if (!pOutputCodec) {
	    // Handle error
	    return -1;
	}

	// Create a new stream for the output file
	AVStream* pOutputStream = avformat_new_stream(pOutputFormatContext, pOutputCodec);
	if (!pOutputStream) {
	    // Handle error
	    return -1;
	}

	// Set codec parameters for the output stream
	AVCodecContext* pOutputCodecContext = avcodec_alloc_context3(pOutputCodec);
	if (!pOutputCodecContext) {
	    // Handle error
	    return -1;
	}
	// Set parameters such as width, height, bitrate, etc. for the output codec context

	// Open the codec for the output stream
	if (avcodec_open2(pOutputCodecContext, pOutputCodec, NULL) < 0) {
	    // Handle error
	    return -1;
	}

	// Write the header for the output file
	if (avformat_write_header(pOutputFormatContext, NULL) < 0) {
	    // Handle error
	    return -1;
	}












    // Open input file
    if (avformat_open_input(&pFormatContext, "input.mp4", NULL, NULL) != 0) {
        // Handle error
        return -1;
    }

    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        avformat_close_input(&pFormatContext);
        return -1;
    }

    int video_stream_index = -1;
    for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        avformat_close_input(&pFormatContext);
        return -1;
    }

    AVCodecParameters* codecParams = pFormatContext->streams[video_stream_index]->codecpar;
    AVCodec* pCodec = avcodec_find_decoder(codecParams->codec_id);
    if (!pCodec) {
        avformat_close_input(&pFormatContext);
        return -1;
    }

    AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        avformat_close_input(&pFormatContext);
        return -1;
    }

    if (avcodec_parameters_to_context(pCodecContext, codecParams) < 0) {
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }

    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }

    SwsContext* swsCtx = sws_getContext(
        pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt,
        640, 480, AV_PIX_FMT_YUV420P, // Change to desired dimensions and pixel format
        SWS_BILINEAR, NULL, NULL, NULL
    );

    AVPacket* packet = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameRGB = av_frame_alloc();
    if (!packet || !pFrame || !pFrameRGB) {
        av_packet_free(&packet);
        av_frame_free(&pFrame);
        av_frame_free(&pFrameRGB);
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 640, 480, 32);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_YUV420P, 640, 480, 32);

    while (av_read_frame(pFormatContext, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            if (avcodec_send_packet(pCodecContext, packet) == 0) {
                while (avcodec_receive_frame(pCodecContext, pFrame) == 0) {
                    sws_scale(swsCtx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0,
                              pCodecContext->height, pFrameRGB->data, pFrameRGB->linesize);
                    
                    // Here you would encode the frame and write it to an output file.




                    // Encode the frame
	                if (avcodec_send_frame(pOutputCodecContext, pFrameRGB) < 0) {
	                    // Handle error
	                    break; // Break from the inner loop and continue to the next frame
	                }

	                // Write encoded data to the output file
	                while (avcodec_receive_packet(pOutputCodecContext, pPacket) == 0) {
	                    av_interleaved_write_frame(pOutputFormatContext, pPacket);
	                    av_packet_unref(pPacket);
	                }





                }
            }
        }
        av_packet_unref(packet);
    }

    av_free(buffer);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrame);
    av_packet_free(&packet);
    sws_freeContext(swsCtx);
    avcodec_free_context(&pCodecContext);
    avformat_close_input(&pFormatContext);
    avformat_network_deinit();

    std::cout << "Done" << std::endl;

    // avformat_close_input(&pFormatContext);
    // avformat_free_context(pFormatContext);



    // Write the trailer for the output file
	av_write_trailer(pOutputFormatContext);

	// Clean up resources
	avcodec_free_context(&pOutputCodecContext);
	avformat_free_context(pOutputFormatContext);

    return 0;
}