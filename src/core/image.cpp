// Copyright 2025 Antmicro
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "image.hpp"
#include <fstream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define RGBA_CHANNELS 4

// region image

static bool is_gif(const uint8_t* buffer, int len) {
	stbi__context s;
	stbi__start_mem(&s, buffer, len);

	return stbi__gif_test(&s);
}

static uint8_t* read_file_into_buffer(const std::string& path, int* bytes) {
	FILE* file = stbi__fopen(path.c_str(), "rb");
	if (!file)
		throw std::runtime_error("Failed to open image '" + path + "'");

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	*bytes = size;

	if (size == 0) {
		fclose(file);
		throw std::runtime_error("Failed to read image '" + path + "'");
	}

	uint8_t* buffer = (uint8_t*)malloc(size);

	while (size) {
		size -= fread(buffer, 1, size, file);
	}

	fclose(file);
	return buffer;
}

image::image(const std::string& path)
	: owner(STB), w(0), h(0), frames(1) {

	// number of channels in the *file*, the loaded
	// buffer will always use 4 channels (RGBA)
	int unused_channel_count = 0;

	// read file into buffer, or throw on error
	int bytes;
	uint8_t* buffer = read_file_into_buffer(path, &bytes);

	if (is_gif(buffer, bytes)) {
		pixels = stbi_load_gif_from_memory(buffer, bytes, nullptr, &w, &h, &frames, &unused_channel_count, RGBA_CHANNELS);
	} else {
		pixels = stbi_load_from_memory(buffer, bytes, &w, &h, &unused_channel_count, RGBA_CHANNELS);
	}

	free(buffer);

	if (pixels == nullptr) {
		throw std::runtime_error("Failed to load image '" + path + "'");
	}
}

image::image(unsigned char* pixels, int w, int h)
	: pixels(pixels), w(w), h(h), frames(1) {
}

image::~image() {
	if (owner == STB) {
		stbi_image_free(pixels);
	}

	pixels = nullptr;
}

int image::width() const {
	return w;
}

int image::height() const {
	return h;
}

unsigned char* image::data(int frame) const {
	if (frame >= frames) {
		throw std::runtime_error("Image frame out of range");
	}

	return pixels + frame * (w * h * 4);
}

void image::dump() const {
	printf("Image w: %d, h: %d\n", w, h);
}

int image::frame_count() const {
	return frames;
}
