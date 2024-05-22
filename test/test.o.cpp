/*

g++ -o test.o.app test.o.cpp `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0`

emcc test.o.cpp -o test.wasm --no-entry -s EXPORTED_FUNCTIONS="['_check']"

emcc -o your_output.js your_source_file.cpp -I/home/user/gst-wasm/include/gstreamer-1.0 -L/home/user/gst-wasm/lib -lgstreamer-1.0

emcc test.o.cpp -o test.wasm --no-entry -s EXPORTED_FUNCTIONS="['_check']" -I/usr/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include

emcc test.o.cpp -o test.wasm --no-entry -s EXPORTED_FUNCTIONS="['_check']" \
-I /usr/include/gstreamer-1.0 \
-I /usr/include/glib-2.0 \
-I /usr/lib/x86_64-linux-gnu/glib-2.0/include

clang -target wasm32-unknown-unknown -O3 -o test.wasm test.o.cpp `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0`
clang -std=c++20 -m64 -target wasm32-unknown-unknown -o test.wasm test.o.cpp `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0` -I/usr/include

g++ test.o.cpp -std=c++20 -o test.wasm `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0`
g++ source_file.cpp -std=c++20 -o output_file.wasm -nostdlib -Wl,--no-entry,--export=__wasm_call_ctors

g++ -nostdlib -nostartfiles -Wl,--no-entry -Wl,--export-all -o test.wasm test.o.cpp

clang++ --target=wasm32-unknown-unknown-wasm -nostdlib -Wl,--no-entry -Wl,--export-all -o test.wasm test.o.cpp




*/

// #include "emscripten.h"
// #include <gst/gst.h>
// #include <iostream> 

extern "C" {
	int check() {
		return 0;
	}

	int main() {
		// std::cout << "Hey!" << std::endl;
		return 0;
	}
}