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

#include "drm.hpp"

#include <stdexcept>

#include "logger.hpp"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define DRM_DEV_1 "/dev/dri/card0"

// region drm

drmModeResPtr drm::get_resource(int fd) {
	auto resource = drmModeGetResources(fd);

	if (resource) {
		return resource;
	}

	throw std::runtime_error{"Unable to get DRM resources!"};
}

drmModeConnectorPtr drm::pick_connector(int fd, drmModeResPtr resource) {
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

		return connector;
	}

	throw std::runtime_error{"No valid connection found!"};
}

drmModeModeInfoPtr drm::pick_mode(drmModeConnectorPtr connector) {
	size_t pixels = 0;
	drmModeModeInfoPtr selected = nullptr;

	for (int i = 0; i < connector->count_modes; i++) {
		const auto mode = &connector->modes[i];

		if (mode->type & DRM_MODE_TYPE_PREFERRED) {
			return mode;
		}

		const size_t size = mode->vdisplay * mode->hdisplay;
		printf("Found %dx%d at %d\n", mode->vdisplay, mode->hdisplay, i);

		// pick the mode with the highest resolution
		if (size > pixels) {
			pixels = size;
			selected = mode;
		}
	}

	if (selected == nullptr) {
		throw std::runtime_error{"No valid mode found for connection!"};
	}

	return selected;
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

drm::drm(const char* path) {

	if (path != nullptr) {
		int fh = open(path, O_RDWR);
		if (fh > 0) {
			init(fh);
			return;
		}

		LOG_WARN("Failed to open user-provided path '%s'!\n", path);
	}

	if (DRM_DEV_1) {
		int fh = open(DRM_DEV_1, O_RDWR);
		if (fh > 0) {
			init(fh);
			return;
		}

		LOG_WARN("Failed to open '%s'!\n", DRM_DEV_1);
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

void drm::init(int fd) {

	try {
		auto resource = get_resource(fd);
		this->fd = fd;

		this->conn = pick_connector(fd, resource);
		this->mode = pick_mode(conn);
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

drm_screen::drm_screen(const std::string& path)
	: fb(std::make_unique<drm>(path.empty() ? nullptr : path.c_str())) {
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
