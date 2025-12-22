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
#include "format.hpp"

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

	// get cached info
	const info& cached_info() const;

	// get pointer to raw pixel data in framebuffer-specific format
	void* data() const;

private:

	info m_info;
};

class framebuffer_screen : public screen {

	std::unique_ptr<framebuffer> fb;

protected:

	void* data() const override;

public:

	framebuffer_screen(const std::string& path);

	void dump() override;
	int width() const override;
	int height() const override;
	format form() const override;
	void flush() const override;
};
