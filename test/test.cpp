/*

g++ test.cpp -o test.app --std=c++20 -lavformat -lavcodec -lavfilter -lavdevice -lavutil -lswscale -lswresample

if CCACHE_DISABLE=1 g++ tiles.cpp -o tiles.app --std=c++20 -lsqlite3 -Werror -Wfatal-errors; then

ffmpeg -hwaccel cuda -hwaccel_output_format cuda -i "input_file.mp4"
-c:a copy -vf "scale_cuda=-2:480" -c:v h264_nvenc "output_file.mp4"

It should -vf "scale=1080:-1, crop=1080:1920"

-i input.mp4: specifies the input file.
-vf "scale=640:360": resizes the video to 640x360 pixels.
-c:v libx264: sets the video codec to H.264.
-crf 23: controls the quality (lower is better quality; range is 0-51).
-preset medium: sets the compression speed (options: ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow).
-c:a aac: sets the audio codec to AAC.
-b:a 128k: sets the audio bitrate to 128 kbps.
output.mp4: specifies the output file name.

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
        return -1;
    }


    // Open input file
    if (avformat_open_input(&pFormatContext, "input.mp4", NULL, NULL) != 0) {
        // Handle error
        return -1;
    }

    std::cout << "Done" << std::endl;

    return 0;
}