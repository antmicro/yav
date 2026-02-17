// Copyright 2025, 2026 Antmicro
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

#include "interrupt.hpp"

#ifdef __GNUC__
#define YAV_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define YAV_FORCE_INLINE inline
#endif

static YAV_FORCE_INLINE void blend(color& s, const void* backbuffer_pixel, const format& fmt, const size_t bytes) {

	const float foreground = s.a / 255.0f;
	const float background = 1 - foreground;

	uint8_t r, g, b;
	size_t pixel = 0;
	memcpy(&pixel, backbuffer_pixel, bytes);

	fmt.decode_rgb(pixel, &r, &g, &b);

	s.r = s.r * foreground + r * background;
	s.g = s.g * foreground + g * background;
	s.b = s.b * foreground + b * background;
}

static YAV_FORCE_INLINE size_t get_offset(const position& offset, int x, int y, size_t line, size_t point) {
	return ((offset.x + x) + (offset.y + y) * line) * point;
}

// region screen

constraint screen::get_viewport(constraint scrc) const {
	viewport sized = view;

	if (sized.w == -1) {
		sized.w = scrc.width();
	}

	if (sized.h == -1) {
		sized.h = scrc.height();
	}

	return sized.get_constraint(scrc);
}

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
	constraint scrc{0, 0, screen_width, screen_height};
	constraint view = get_viewport(scrc);

	position placed = img.get_position(view);
	constraint imgc{placed.x, placed.y, img_width, img_height};

	constraint region = get_constraint_intersection({scrc, imgc, view});
	position so = scrc.offset(region);
	position io = imgc.offset(region);

	// we iterate over the clamped range of source image pixels
	for (int y = 0; y < region.height(); y++) {
		for (int x = 0; x < region.width(); x++) {
			const size_t screen_pixel = get_offset(so, x, y, screen_width, bytes);
			const size_t image_pixel = get_offset(io, x, y, img_width, 4);

			void* target = dst + screen_pixel;
			color s = color::from_rgba(img.data(frame) + image_pixel);

			// skip fully transparent pixels
			if (s.a == 0) {
				continue;
			}

			if (blending) {
				blend(s, target, fmt, bytes);
			}

			const size_t color = fmt.encode_rgb(s.r, s.g, s.b) | alpha;
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

			if (was_interrupted()) {
				return;
			}

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

void screen::clear(color c) {
	const int w = width();
	const int h = height();

	const format fmt = form();
	const size_t bytes = std::min(fmt.bytes(), 8UL);
	const size_t alpha = fmt.encode_alpha(0xff);

	auto* dst = reinterpret_cast<uint8_t*>(data());

	if (c.a == 0) {
		return;
	}

	constraint scrc{0, 0, width(), height()};
	constraint view = get_viewport(scrc);

	constraint region = get_constraint_intersection({scrc, view});
	position so = scrc.offset(region);

	for (int y = 0; y < region.height(); y++) {
		for (int x = 0; x < region.width(); x++) {
			void* target = dst + get_offset(so, x, y, w, bytes);
			color s = c;

			if (c.a != 255) {
				blend(s, target, fmt, bytes);
			}

			const size_t encoded = fmt.encode_rgb(s.r, s.g, s.b) | alpha;
			memcpy(target, &encoded, bytes);
		}
	}
}
