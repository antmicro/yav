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

// region image

image::image(const std::string& path)
	: owner(STB), w(0), h(0) {

	// number of channels in the *file*, the loaded
	// buffer will always use 4 channels (RGBA)
	int unused_channel_count = 0;

	pixels = stbi_load(path.c_str(), &w, &h, &unused_channel_count, 4);

	if (pixels == nullptr) {
		throw std::runtime_error("failed to load image '" + path + "'");
	}
}

image::image(unsigned char* pixels, int w, int h)
	: pixels(pixels), w(w), h(h) {
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

unsigned char* image::data() const {
	return pixels;
}

void image::dump() const {
	printf("Image w: %d, h: %d\n", w, h);
}
