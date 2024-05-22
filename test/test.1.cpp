/*

g++ test.cpp -o test.app --std=c++20 -lavformat -lavcodec -lavfilter -lavdevice -lavutil -lswscale -lswresample

if CCACHE_DISABLE=1 g++ tiles.cpp -o tiles.app --std=c++20 -lsqlite3 -Werror -Wfatal-errors; then

ffmpeg -hwaccel cuda -hwaccel_output_format cuda -i "input_file.mp4"
-c:a copy -vf "scale_cuda=-2:480" -c:v h264_nvenc "output_file.mp4"

-vf "scale=1080:-1, crop=1080:1920"


ffmpeg -i 6609d069be61940bcaf7e77b.1716283172891.mov -vf "scale=1080:-1, crop=1080:1920" -c:v libx264 -crf 23 -preset medium -c:a aac -b:a 128k output.ios.mp4
ffmpeg -i 6609d069be61940bcaf7e77b.1716283172891.mov -vf "scale=1080:-1, crop=1080:1920" -c:v libvpx-vp9 -crf 30 -b:v 0 -c:a libvorbis -threads 4 output.ios.webm
ffmpeg -i 6609d069be61940bcaf7e77b.1716283172891.mov -vf "scale=1080:-1, crop=1080:1920" -c:v libvpx-vp9 -crf 40 -b:v 1.5M -c:a libvorbis -b:a 128k -threads 6 output.ios.3.webm

ffmpeg -i 6609d069be61940bcaf7e77b.1716283172891.mov -vf "scale=1080:-1, crop=1080:1920" -c:v libvpx-vp9 -crf 20 -b:v 1.5M -c:a libvorbis -b:a 128k output.ios.6.webm

ffmpeg -i 6609d069be61940bcaf7e77b.1716283172891.mov -vf "scale=1080:-1, crop=1080:1920, fps=29" -c:v libvpx-vp9 -crf 10 -b:v 1.5M -c:a libvorbis -b:a 128k -cpu-used 4 -threads 4 output.ios.8.webm

ffmpeg -i 6609d069be61940bcaf7e77b.1716283172891.mov -vf "scale=1080:-1, crop=1080:1920, fps=29" -c:v libvpx-vp9 -crf 20 -b:v 1M -c:a libvorbis -b:a 128k -cpu-used 6 -threads 6 output.ios.11.webm



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