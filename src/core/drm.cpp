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

#include "drm.hpp"

#include <cstdlib>
#include <stdexcept>
#include <vector>

#include "logger.hpp"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "config.hpp"

// region drm

drmModeResPtr drm::get_resource(int fd) {
	auto resource = drmModeGetResources(fd);

	if (resource) {
		return resource;
	}

	throw std::runtime_error{"Unable to get DRM resources!"};
}

drmModeConnectorPtr drm::pick_connector(int fd, drmModeResPtr resource, size_t index) {

	std::vector<drmModeConnectorPtr> connectors;

	for (int i = 0; i < resource->count_connectors; i++) {
		const auto connector = drmModeGetConnectorCurrent(fd, resource->connectors[i]);

		if (connector == nullptr) {
			continue;
		}

		if (connector->count_modes == 0) {
			continue;
		}

		if (connector->connection == DRM_MODE_DISCONNECTED) {
			continue;
		}

		connectors.push_back(connector);
	}

	if (index >= connectors.size()) {
		throw std::runtime_error{"Can't select output #" + std::to_string(index) + ", as there are only " + std::to_string(connectors.size()) + " outputs"};
	}

	for (size_t i = 0; i < connectors.size(); i++) {
		if (i != index) {
			drmModeFreeConnector(connectors[i]);
		}
	}

	return connectors[index];
}

drmModeModeInfoPtr drm::pick_mode(drmModeConnectorPtr connector, const uint16_t hdisplay_hint, const uint16_t vdisplay_hint, const uint32_t vrefresh_hint) {
	size_t pixels = 0;
	drmModeModeInfoPtr preferred = nullptr, highest = nullptr;

	for (int i = 0; i < connector->count_modes; i++) {
		const auto mode = &connector->modes[i];
		if (hdisplay_hint == mode->hdisplay && vdisplay_hint == mode->vdisplay) {
			if (vrefresh_hint == -1 || vrefresh_hint == mode->vrefresh) {
				return mode;
			}
		}

		if (mode->type & DRM_MODE_TYPE_PREFERRED) {
			preferred = mode;
		}

		const size_t size = mode->vdisplay * mode->hdisplay;

		// pick the mode with the highest resolution
		if (size > pixels) {
			pixels = size;
			highest = mode;
		}
	}

	if (preferred) {
		return preferred;
	}

	if (highest) {
		return highest;
	}

	throw std::runtime_error{"No valid mode found for connection!"};
}

drmModeCrtcPtr drm::get_crtc(int fd, drmModeConnectorPtr connector) {
	auto encoder = drmModeGetEncoder(fd, connector->encoder_id);
	if (!encoder) {
		throw std::runtime_error{"Unable to get encoder!"};
	}

	auto crtc = drmModeGetCrtc(fd, encoder->crtc_id);
	drmModeFreeEncoder(encoder);

	if (!crtc) {
		throw std::runtime_error{"Unable to get CRTC!"};
	}

	return crtc;
}

bool drm::try_using(const char* path, size_t conn, uint16_t hdisplay_hint, uint16_t vdisplay_hint, uint32_t vrefresh_hint) {
	if (path != nullptr) {
		int fh = open(path, O_RDWR);
		if (fh > 0) {
			init(fh, conn, hdisplay_hint, vdisplay_hint, vrefresh_hint);
			return true;
		}

		LOG_WARN("Failed to open '%s'!\n", path);
	}

	return false;
}

drm::drm(const char* path, uint16_t width, uint16_t height, uint32_t refresh) {

	size_t output = 0;

	const char* default_conn = std::getenv(DRM_ENV_CONN);

	if (default_conn != nullptr) {
		output = std::stoull(default_conn);
	}

	if (path != nullptr) {
		bool begin_display = false;
		std::string wrapped{path};
		std::string file, display;

		for (char c : wrapped) {
			if (begin_display) {
				display.push_back(c);
			} else {
				if (c == '@') {
					begin_display = true;
					continue;
				}

				file.push_back(c);
			}
		}

		if (!display.empty()) {
			output = std::stoull(display.c_str());
		}

		if (!file.empty() && try_using(file.c_str(), output, width, height, refresh)) {
			return;
		}
	}

	if (try_using(std::getenv(DRM_ENV_PATH), output, width, height, refresh)) {
		return;
	}

	if (try_using(DRM_DEV_1, output, width, height, refresh)) {
		return;
	}

	throw std::runtime_error("out of ideas, unable to open any framebuffer");
}

drm::~drm() {

	if (crtc) {
		drmSetMaster(fd);
		drmModeSetCrtc(fd, crtc->crtc_id, crtc->buffer_id, 0, 0, &conn->connector_id, 1, mode);
		drmModeFreeCrtc(crtc);
	}

	if (id) {
		drmModeFreeFB(drmModeGetFB(fd, id));
	}

	if (conn) {
		drmModeFreeConnector(conn);
	}

	if (dumb.handle) {
		ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, dumb.handle);
	}

	if (buffer) {
		munmap(buffer, dumb.size);
	}

	close(fd);
}

void drm::flush() {
	if (drmSetMaster(fd)) {
		throw std::runtime_error{"Unable to acquire master access!"};
	}

	drmModeSetCrtc(fd, crtc->crtc_id, id, 0, 0, &conn->connector_id, 1, mode);
	drmDropMaster(fd);
}

void drm::map() {
	drm_mode_map_dumb map{};

	memset(&map, 0, sizeof(map));
	map.handle = dumb.handle;

	int err = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
	if (err) {
		throw std::runtime_error{"Unable to mode-map dumb framebuffer (err: " + std::to_string(err) + ")!"};
	}

	buffer = mmap(nullptr, dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map.offset);
	if (buffer == nullptr) {
		throw std::runtime_error{"Unable to memory-map dumb framebuffer!"};
	}
}

void drm::create_framebuffer(int depth, int bits_per_pixel) {
	dumb.height = mode->vdisplay;
	dumb.width = mode->hdisplay;
	dumb.bpp = bits_per_pixel;

	int err = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb);
	if (err) {
		throw std::runtime_error{"Unable to create dumb framebuffer (err: " + std::to_string(err) + ")!"};
	}

	err = drmModeAddFB(fd, dumb.width, dumb.height, depth, dumb.bpp, dumb.pitch, dumb.handle, &id);
	if (err) {
		throw std::runtime_error{"Unable to add framebuffer (err: " + std::to_string(err) + ")!"};
	}
}

void drm::init(int fd, size_t output, uint16_t hdisplay_hint, uint16_t vdisplay_hint, uint32_t vrefresh_hint) {

	try {
		auto resource = get_resource(fd);
		this->fd = fd;

		this->conn = pick_connector(fd, resource, output);
		this->mode = pick_mode(conn, hdisplay_hint, vdisplay_hint, vrefresh_hint);
		this->crtc = get_crtc(fd, conn);

		create_framebuffer(24, 32);
		map();
	} catch (const std::exception& e) {
		throw std::runtime_error{"DMA init failed: " + std::string(e.what())};
	}
}

int drm::width() const {
	return dumb.width;
}

int drm::height() const {
	return dumb.height;
}

void* drm::data() const {
	return buffer;
}

void drm::dump() const {
	printf("Using DRM conn=%d, crtc=%d, type=%d (%dx%d)", conn->connector_id, crtc->crtc_id, conn->connector_type_id, width(), height());
}

// region drm_screen

drm_screen::drm_screen(const std::string& path, uint16_t hdisplay_hint, uint16_t vdisplay_hint, uint32_t vrefresh_hint)
	: fb(std::make_unique<drm>(path.empty() ? nullptr : path.c_str(), hdisplay_hint, vdisplay_hint, vrefresh_hint)) {
}

void drm_screen::dump() {
	fb->dump();

	printf(" color format: ");
	form().dump();
	printf("\n");
}

int drm_screen::width() const {
	return fb->width();
}

int drm_screen::height() const {
	return fb->height();
}

void* drm_screen::data() const {
	return fb->data();
}

format drm_screen::form() const {
	return {32, {8, 16}, {8, 8}, {8, 0}, {8, 24}};
}

void drm_screen::flush() const {
	fb->flush();
}
