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
#include <memory>
#include <unistd.h>

#include "interrupt.hpp"

#ifdef __GNUC__
#define YAV_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define YAV_FORCE_INLINE inline
#endif

static YAV_FORCE_INLINE size_t get_offset(const position& offset, int x, int y, size_t line, size_t point) {
	return ((offset.x + x) + (offset.y + y) * line) * point;
}

static YAV_FORCE_INLINE void blend(color& front, const color& back) {
	const float foreground = front.a / 255.0f;
	const float background = 1 - foreground;

	front.r = front.r * foreground + back.r * background;
	front.g = front.g * foreground + back.g * background;
	front.b = front.b * foreground + back.b * background;
}

static YAV_FORCE_INLINE void blend(color& s, const void* backbuffer_pixel, const format& fmt, const size_t bytes) {

	const float foreground = s.a / 255.0f;
	const float background = 1 - foreground;

	size_t pixel = 0;
	memcpy(&pixel, backbuffer_pixel, bytes);

	color back;
	fmt.decode_rgb(pixel, &back.r, &back.g, &back.b);

	blend(s, back);
}

// region screen

color* screen::fetch_backbuffer(constraint region, position offset) {
	const int rw = region.width();
	const int rh = region.height();

	color* backbuffer = new color[rw * rh];

	const format fmt = form();
	const int screen_width = width();

	const size_t bytes = std::min(fmt.bytes(), 8UL);
	auto* src_buffer = reinterpret_cast<unsigned char*>(data());

	for (int y = 0; y < rh; y++) {
		for (int x = 0; x < rw; x++) {
			color* dst = backbuffer + (x + y * rw);
			void* src = src_buffer + get_offset(offset, x, y, screen_width, bytes);

			size_t pixel = 0;
			memcpy(&pixel, src, bytes);
			fmt.decode_rgb(pixel, &dst->r, &dst->g, &dst->b);
		}
	}

	return backbuffer;
}

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

void screen::blit_frame(const image& img, int frame, constraint region, position so, position io, color* backbuffer) {

	const int rw = region.width();
	const int rh = region.height();

	const int screen_width = width();
	const format fmt = form();

	// save a few cycles by encoding alpha only once
	const size_t alpha = fmt.encode_alpha(0xff);
	const size_t bytes = std::min(fmt.bytes(), 8UL);

	auto* dst_buffer = reinterpret_cast<unsigned char*>(data());

	for (int y = 0; y < rh; y++) {
		for (int x = 0; x < rw; x++) {
			void* dst = dst_buffer + get_offset(so, x, y, screen_width, bytes);
			color src = img.pixel(frame, io.x + x, io.y + y);

			if (backbuffer) {
				blend(src, backbuffer[x + y * rw]);
			}

			const size_t encoded = fmt.encode_rgb(src.r, src.g, src.b) | alpha;
			memcpy(dst, &encoded, bytes);
		}
	}

	flush();
}

void screen::blit(const image& img) {
	int count = img.loops;

	// calculate final image offset
	constraint scrc{0, 0, width(), height()};
	constraint view = get_viewport(scrc);

	position placed = img.get_position(view);
	constraint imgc{placed.x, placed.y, img.width(), img.height()};

	constraint region = get_constraint_intersection({scrc, imgc, view});
	position so = scrc.offset(region);
	position io = imgc.offset(region);

	std::unique_ptr<color[]> backbuffer;

	if (img.blend) {
		backbuffer.reset(fetch_backbuffer(region, so));
	}

	while (count) {
		auto last = img.frame_count() - 1;

		for (int frame = 0; frame <= last; frame++) {
			blit_frame(img, frame, region, so, io, backbuffer.get());

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
			color src = c;

			if (src.a != 255) {
				blend(src, target, fmt, bytes);
			}

			const size_t encoded = fmt.encode_rgb(src.r, src.g, src.b) | alpha;
			memcpy(target, &encoded, bytes);
		}
	}
}
