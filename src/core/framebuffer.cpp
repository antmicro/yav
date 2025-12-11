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

#include "framebuffer.hpp"
#include "logger.hpp"

#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "image.hpp"

// to disable set to nullptr
#define FB_DEV_1 "/dev/fb0"
#define FB_DEV_2 "/dev/fb/0"

// region framebuffer

framebuffer::framebuffer(const char* path) {

	if (path != nullptr) {
		int fh = open(path, O_RDWR);
		if (fh > 0) {
			init(fh);
			return;
		}

		LOG_WARN("Failed to open user-provided path '%s'!\n", path);
	}

	if (FB_DEV_1) {
		int fh = open(FB_DEV_1, O_RDWR);
		if (fh > 0) {
			init(fh);
			return;
		}

		LOG_WARN("Failed to open '%s'!\n", FB_DEV_1);
	}

	if (FB_DEV_2) {
		int fh = open(FB_DEV_2, O_RDWR);
		if (fh > 0) {
			init(fh);
			return;
		}

		LOG_WARN("Failed to open '%s'!\n", FB_DEV_2);
	}

	throw std::runtime_error("out of ideas, unable to open any framebuffer");
}

void framebuffer::init(int handle) {
	m_handle = handle;
	load(m_info);

	void* map = mmap(nullptr, m_info.size(), PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
	m_buffer = static_cast<unsigned char*>(map);
}

framebuffer::~framebuffer() {
	munmap(m_buffer, m_info.size());
	close(m_handle);
}

int framebuffer::handle() const {
	return m_handle;
}

void framebuffer::load(info& info) const {

	info.var = {};
	info.fix = {};

	if (ioctl(m_handle, FBIOGET_VSCREENINFO, &info.var) != 0) {
		throw std::runtime_error("failed to load variable frame buffer info");
	}

	if (ioctl(m_handle, FBIOGET_FSCREENINFO, &info.fix) != 0) {
		throw std::runtime_error("failed to load fixed frame buffer info");
	}
}

void framebuffer::store(const info& info) {

	if (ioctl(m_handle, FBIOPUT_VSCREENINFO, info.var)) {
		throw std::runtime_error("failed to update variable frame buffer info");
	}

	// update local info
	load(m_info);
}

void framebuffer::blit(const image& img) {
	const int screen_width = m_info.width();
	const int screen_height = m_info.height();

	const int img_width = img.width();
	const int img_height = img.height();

	const format fmt = m_info.get_format();

	// save a few cycles by encoding alpha only once
	const size_t alpha = fmt.encode_alpha(0xff);
	const size_t bytes = std::min(fmt.bytes(), 8UL);

	unsigned char* dst = m_buffer;
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
			unsigned char* source = img.data() + (x + y * img_width) * 4;

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
}

void framebuffer::clear() {
	const size_t bytes = std::min(m_info.get_format().bytes(), 8UL);
	const size_t size = m_info.width() * m_info.height() * bytes;

	memset(m_buffer, 0, size);
}

// region framebuffer::info

const char* framebuffer::info::name() const {
	return fix.id;
}

bool framebuffer::info::has_fourcc() const {
	return var.grayscale > 1;
}

bool framebuffer::info::has_color() const {
	return var.grayscale == 0 && get_format().color();
}

unsigned int framebuffer::info::width() const {
	return var.xres;
}

unsigned int framebuffer::info::height() const {
	return var.yres;
}

framebuffer::format framebuffer::info::get_format() const {
	return format{var};
}

void framebuffer::info::set_format(const format& fmt) {
	var.red = fmt.r.as_bitfield();
	var.green = fmt.g.as_bitfield();
	var.blue = fmt.b.as_bitfield();
	var.transp = fmt.a.as_bitfield();
	var.bits_per_pixel = fmt.bits;
}

void framebuffer::info::dump() const {
	printf("Using framebuffer '%s' (%dx%d) %s format: ", name(), width(), height(), has_color() ? "color" : "grayscale");
	get_format().dump();
	printf("\n");
}

size_t framebuffer::info::size() const {
	const size_t page = getpagesize();
	const size_t mask = page - 1;
	const size_t bytes = fix.smem_len;

	// align up
	if (bytes & mask) {
		return (bytes & ~mask) + page;
	}

	return bytes;
}

// region framebuffer::format

framebuffer::format::format(unsigned int bits, channel r, channel g, channel b, channel a)
	: bits(bits), r(r), g(g), b(b), a(a) {
}

framebuffer::format::format(const fb_var_screeninfo& var)
	: format(var.bits_per_pixel, var.red, var.green, var.blue, var.transp) {
}

bool framebuffer::format::pseudocolor() const {
	return r.is_used() && g.is_used() && b.is_used();
}

bool framebuffer::format::color() const {
	return pseudocolor() && (r.offset() != g.offset()) && (g.offset() != b.offset()) && (r.offset() != b.offset());
}

size_t framebuffer::format::encode_rgb(uint8_t sr, uint8_t sg, uint8_t sb) const {
	return r.encode(sr) | g.encode(sg) | b.encode(sb);
}

size_t framebuffer::format::encode_alpha(uint8_t alpha) const {
	return a.encode(alpha);
}

void framebuffer::format::dump() const {
	r.dump("red");
	g.dump("green");
	b.dump("blue");
	a.dump("alpha");
}

size_t framebuffer::format::bytes() const {
	return bits / 8;
}

void framebuffer::format::decode_rgb(size_t pixel, uint8_t* dr, uint8_t* dg, uint8_t* db) const {
	*dr = r.decode(pixel);
	*dg = g.decode(pixel);
	*db = b.decode(pixel);
}

// region framebuffer::channel

framebuffer::channel::channel(unsigned int length, unsigned int offset)
	: m_offset(offset), m_length(length), m_mask((1 << length) - 1) {
}

framebuffer::channel::channel(fb_bitfield bf)
	: channel(bf.length, bf.offset) {
}

bool framebuffer::channel::is_used() const {
	return m_mask != 0;
}

size_t framebuffer::channel::encode(uint8_t value) const {
	return (((value * m_mask) / 255) & m_mask) << m_offset;
}

uint8_t framebuffer::channel::decode(size_t value) const {
	return (((value * 255) / m_mask) >> m_offset) & m_mask;
}

void framebuffer::channel::dump(const char* name) const {
	printf("%s=%02x@%d ", name, m_mask, m_offset);
}

unsigned int framebuffer::channel::offset() const {
	return m_offset;
}

fb_bitfield framebuffer::channel::as_bitfield() const {
	return {m_offset, m_length, 0};
}

// region framebuffer_screen

framebuffer_screen::framebuffer_screen(const std::string& path)
	: fb(std::make_unique<framebuffer>(path.empty() ? nullptr : path.c_str())) {

	framebuffer::info info;
	fb->load(info);

	if (!info.has_color() || info.has_fourcc()) {
		LOG_WARN("Framebuffer doesn't have color enabled, trying to switch format to RGB...\n");

		framebuffer::format format{32, {8, 0}, {8, 8}, {8, 16}, {}};
		info.set_format(format);

		try {
			fb->store(info);
		} catch (const std::exception& e) {
			LOG_ERROR("Failed to enable color support! %s\n", e.what());
		}
	}

	fb->load(info);

	if (!info.has_color()) {
		throw std::runtime_error("Failed to enable color support!");
	}
}

void framebuffer_screen::blit(const image& img) {
	fb->blit(img);
}

void framebuffer_screen::clear() {
	fb->clear();
}

void framebuffer_screen::dump() {
	framebuffer::info info;
	fb->load(info);
	info.dump();
}
