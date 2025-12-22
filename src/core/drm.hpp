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

#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <memory>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "screen.hpp"

class drm {

	drm_mode_create_dumb dumb;
	drmModeCrtcPtr crtc;
	drmModeModeInfoPtr mode;
	drmModeConnectorPtr conn;
	uint32_t id;
	int fd;
	void* buffer;

	static drmModeResPtr get_resource(int fd);
	static drmModeConnectorPtr pick_connector(int fd, drmModeResPtr resource);
	static drmModeModeInfoPtr pick_mode(drmModeConnectorPtr connector);
	static drmModeCrtcPtr get_crtc(int fd, drmModeConnectorPtr connector);

	void map();
	void create_framebuffer(int depth, int bits_per_pixel);
	void init(int fd);

public:

	drm(const char* path);
	~drm();

	// Update the screen to match the buffer
	void flush();

	// Get width, in pixels
	int width() const;

	// get height, in pixels
	int height() const;

	// Get pointer to ARGB data buffer
	void* data() const;

	// Print some data about DRM
	void dump() const;
};

class drm_screen : public screen {

	std::unique_ptr<drm> fb;

protected:

	void* data() const override;

public:

	drm_screen(const std::string& path);

	void dump() override;
	int width() const override;
	int height() const override;
	format form() const override;
	void flush() const;
};
