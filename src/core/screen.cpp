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

#include "screen.hpp"

#include <cstring>
#include <unistd.h>

// region screen

void screen::blit_frame(const image& img, int frame) {
	const int screen_width = width();
	const int screen_height = height();

	const int img_width = img.width();
	const int img_height = img.height();

	const format fmt = form();

	// save a few cycles by encoding alpha only once
	const size_t alpha = fmt.encode_alpha(0xff);
	const size_t bytes = std::min(fmt.bytes(), 8UL);

	auto* dst = reinterpret_cast<unsigned char*>(data());
	const bool blending = img.blend;

	// calculate final image offset
	// [sx, sy] describe the position in screen space coordinates (in range [0,
	// 1] the placement of the image's matching point, where (0, 0) is the top
	// left screen corner. (so for (-1, -1) top-left image corner in top-left
	// screen corner, and for (1, 1) bottom-right image corner in bottom-right
	// screen corner). We add the [ox, oy], to allow for-fine tuning the image's
	// position in pixels.
	const int fx = img.ox + static_cast<float>(screen_width) * img.sx - static_cast<float>(img_width) * img.sx;
	const int fy = img.oy + static_cast<float>(screen_height) * img.sy - static_cast<float>(img_height) * img.sy;

	//  clamp source dimensions so that it fits in the buffer
	size_t bx = std::max(fx, 0);
	size_t by = std::max(fy, 0);

	// ... here we make sure we don't fall outside on the right/bottom
	size_t w = std::min(fx + img_width, screen_width) - fx;
	size_t h = std::min(fy + img_height, screen_height) - fy;

	// we iterate over the clamped range of source image pixels
	for (size_t x = 0; x < w; x++) {
		for (size_t y = 0; y < h; y++) {
			void* target = dst + (bx + x + (by + y) * screen_width) * bytes;
			unsigned char* source = img.data(frame) + (x + y * img_width) * 4;

			uint8_t sr = source[0];
			uint8_t sg = source[1];
			uint8_t sb = source[2];
			uint8_t sa = source[3];

			// skip fully transparent pixels
			if (sa == 0) {
				continue;
			}

			if (blending) {
				const float foreground = *(source + 3) / 255.0f;
				const float background = 1 - foreground;

				uint8_t r, g, b;
				size_t pixel = 0;
				memcpy(&pixel, target, bytes);

				fmt.decode_rgb(pixel, &r, &g, &b);

				sr = sr * foreground + r * background;
				sg = sg * foreground + g * background;
				sb = sb * foreground + b * background;
			}

			const size_t color = fmt.encode_rgb(sr, sg, sb) | alpha;
			memcpy(target, &color, bytes);
		}
	}

	flush();
}

void screen::blit(const image& img) {
	int count = img.loops;

	while (count) {
		auto last = img.frame_count() - 1;

		for (int frame = 0; frame <= last; frame++) {
			blit_frame(img, frame);

			// only sleep if there will be another frame
			if (frame != last) {
				usleep(img.mspt);
			}
		}

		// setting loop to -1 puts it into an infinite loop
		if (count > 0) {
			count--;
		}
	}
}

void screen::clear() {
	const size_t bytes = std::min(form().bytes(), 8UL);
	const size_t size = width() * height() * bytes;

	memset(data(), 0, size);
}