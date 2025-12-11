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

#pragma once

#include <linux/fb.h>
#include <memory>
#include <string>

#include "core/screen.hpp"

class image;

class framebuffer {

	int m_handle = -1;
	unsigned char* m_buffer = nullptr;

	void init(int handle);

public:

	// Create a new framebuffer object, the default framebuffer device
	// can be overridden by providing a custom path. In case the provided
	// device can't be found this constructor will try the default ones.
	framebuffer(const char* path);
	~framebuffer();

	// Describes the data layout of a single channel
	// in a single pixel
	class channel {

		unsigned int m_length = 0;
		unsigned int m_offset = 0;
		unsigned long m_mask = 0;

	public:

		channel() = default;
		channel(unsigned int length, unsigned int offset);
		channel(fb_bitfield bf);

		// Check if this channel in any way contributes to the format (has
		// non-zero mask)
		bool is_used() const;

		// Encodes a single value into the format described by this channel
		size_t encode(unsigned char value) const;

		// Print simple overview to the standard output
		void dump(const char* name) const;

		// Get the required left-bit-shift to form the desired value
		unsigned int offset() const;

		// Convert back to linux fb_bitfield
		fb_bitfield as_bitfield() const;
	};

	// Describes the data layout in a single pixel
	// can be used to convert RGB data to that layout
	struct format {

		unsigned int bits;
		channel r, g, b, a;

		format(unsigned int bits, channel r, channel g, channel b, channel a);
		format(const fb_var_screeninfo& var);

		// Does this format have RGB channels
		bool pseudocolor() const;

		// Does this format have separate RGB channels
		bool color() const;

		// Encode RGB data into the format
		size_t encode_rgb(unsigned char* rgb) const;

		// Encode transparency data into the format
		size_t encode_alpha(unsigned char alpha) const;

		// Print simple overview to the standard output
		void dump() const;

		// How many bytes a single pixel takes in this format
		size_t bytes() const;
	};

	class info {

		friend class framebuffer;

		fb_var_screeninfo var{};
		fb_fix_screeninfo fix{};

	public:

		info() = default;
		info(fb_var_screeninfo& var, fb_fix_screeninfo& fix)
			: var(var), fix(fix) {}

		const char* name() const;

		// Check if the framebuffer uses a FOURCC code, we don't support
		// them and need to switch to a different mode if it does
		bool has_fourcc() const;

		// Check if this framebuffer has colored output configured
		bool has_color() const;

		// Get width in pixels
		unsigned int width() const;

		// get height in pixels
		unsigned int height() const;

		// Get format of individual pixel in the buffer
		format get_format() const;

		// Update current format
		void set_format(const format& fmt);

		// Print simple overview to the standard output
		void dump() const;

		// Get the size of the framebuffer in bytes
		size_t size() const;
	};

	// Get file handle to the framebuffer
	int handle() const;

	// Load framebuffer metadata into the provided object
	void load(info& info) const;

	// Update the framebuffers configuration using the provided metadata object
	void store(const info& info);

	// Copy image into the framebuffer
	void blit(const image& img);

	// Clear the internal buffer
	void clear();

private:

	info m_info;
};

class framebuffer_screen : public screen {

	std::unique_ptr<framebuffer> fb;

public:

	framebuffer_screen(const std::string& path);

	void blit(const image& img) override;
	void clear() override;
	void dump() override;
};
