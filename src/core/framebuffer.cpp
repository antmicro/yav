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
	try {
		m_handle = handle;
		load(m_info);

		void* map = mmap(nullptr, m_info.size(), PROT_READ | PROT_WRITE, MAP_SHARED, handle, 0);
		m_buffer = static_cast<unsigned char*>(map);
	} catch (const std::exception& e) {
		throw std::runtime_error{"framebuffer init failed: " + std::string(e.what())};
	}
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

const framebuffer::info& framebuffer::cached_info() const {
	return m_info;
}

void* framebuffer::data() const {
	return m_buffer;
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

format framebuffer::info::get_format() const {
	auto r = var.red;
	auto g = var.green;
	auto b = var.blue;
	auto a = var.transp;

	return format{
		var.bits_per_pixel,
		{r.length, r.offset},
		{g.length, g.offset},
		{b.length, b.offset},
		{a.length, a.offset}};
}

void framebuffer::info::set_format(const format& fmt) {
	var.red = {fmt.r.offset, fmt.r.length, 0};
	var.green = {fmt.g.offset, fmt.g.length, 0};
	var.blue = {fmt.b.offset, fmt.b.length, 0};
	var.transp = {fmt.a.offset, fmt.a.length, 0};
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

// region framebuffer_screen

framebuffer_screen::framebuffer_screen(const std::string& path)
	: fb(std::make_unique<framebuffer>(path.empty() ? nullptr : path.c_str())) {

	framebuffer::info info;
	fb->load(info);

	if (!info.has_color() || info.has_fourcc()) {
		LOG_WARN("Framebuffer doesn't have color enabled, trying to switch format to RGB...\n");

		format fmt{32, {8, 0}, {8, 8}, {8, 16}, {}};
		info.set_format(fmt);

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

void framebuffer_screen::dump() {
	framebuffer::info info;
	fb->load(info);
	info.dump();
}

int framebuffer_screen::width() const {
	return fb->cached_info().width();
}

int framebuffer_screen::height() const {
	return fb->cached_info().height();
}

void* framebuffer_screen::data() const {
	return fb->data();
}

format framebuffer_screen::form() const {
	return fb->cached_info().get_format();
}

void framebuffer_screen::flush() const {
	// do nothing
}
