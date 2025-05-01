all:
	g++ main.cpp -o main -std=c++20 -g -lglfw -lpthread -lX11 -ldl -lXrandr -lGLEW -lGL -DGL_SILENCE_DEPRECATION -DGLM_ENABLE_EXPERIMENTAL -I. `pkg-config --cflags --libs freetype2`
