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

#include <algorithm>
#include <core/interrupt.hpp>
#include <core/logger.hpp>
#include <filesystem>
#include <vector>

#ifdef HAS_LIBDRM
#include "core/drm.hpp"
#endif

#include "core/framebuffer.hpp"

static void printo(const std::string_view& text, bool stop_on_colon = true) {
	bool bold = false;
	bool faint = false;
	bool enabled = true;

	for (char c : text) {
		if (enabled) {
			if (c == '-' && !bold) {
				printf("\033[1m");
				bold = true;
			} else if ((c == ' ' || c == ',') && bold) {
				printf("\033[0m");
				bold = false;
			}
		}

		if (c == ':') {
			enabled = !stop_on_colon;
		}

		putc(c, stdout);
	}
}

static void usage() {
	printo("Usage: yav [--image <path>] [--anchor <x> <y>] [--offset <x> <y>]\n", false);
	printo("           [-v] [--dev <d[:cfg]>] [-c|--clear [color]] [-h|--help]\n", false);
	printo("           [-b|--blend] [-s|--static] [--time <mspf>] [--loop [times]]\n");
	printo("           [--view <x> <y> <w> <h>] [--view-anchor <x> <y>]\n");
}

static void help() {
	usage();
	printf("\nOptions:\n");
	printo("  -h, --help                 : Show this help page and exit\n");
	printo("  -v                         : Verbose mode\n");
	printo("      --dev <device[:cfg]>   : Device type ('fb', 'drm') and config, use '--dev <d>:?' for more info.\n");
	printo("      --image <path>         : Image file path\n");
	printo("      --anchor <x> <y>       : Anchor as fractions in range 0 to 1\n");
	printo("      --offset <x> <y>       : Offset in pixels\n");
	printo("      --time <mspf>          : Milliseconds per animation frame\n");
	printo("      --loop [times]         : Specify infinite or exact loop count\n");
	printo("  -c, --clear [color]        : Clear the framebuffer, where color is [0x|#][aa]rrggbb\n");
	printo("  -s, --static               : Disable animations if present\n");
	printo("  -b, --blend                : Enable alpha-blending\n");
	printo("      --view <x> <y> <w> <h> : Configure viewport area\n");
	printo("      --view-anchor <x> <y>  : Viewport anchor as fractions in range 0 to 1\n");
	printf("\nExamples:\n");
	printf("  yav --image example/tuxan.png --anchor 0.5 0.5 --clear ffffff\n");
	printf("  yav --image example/tuxan.png --anchor 1 1 --offset -100 -100\n");
	printf("  yav --image example/splash.png --anchor 0.5 0.5 --blend\n");
	printf("  yav --image example/earth.png --loop\n");
	printf("  yav --view 0 0 200 10 --clear ff0000\n");
}

static std::unique_ptr<screen> make_screen(const std::string& descriptor) {

	if (descriptor.empty()) {
		return std::make_unique<framebuffer_screen>("");
	}

	bool begin_path = false;
	std::string device;
	std::string path;

	// split descriptor to device[:path]
	for (char c : descriptor) {
		if (begin_path) {
			path.push_back(c);
		} else {
			if (c == ':') {
				begin_path = true;
				continue;
			}

			device.push_back(c);
		}
	}

	// Linux framebuffer device
	if (device == "fb") {
		if (path == "?") {
			printf("Usage: --dev fb[:path]\n\n");
			printf("Use framebuffer device, this is the default mode of operation,\n");
			printf("the optional path given after ':' can be used to point YAV to a specific\n");
			printf("framebuffer device driver to use. By default yav will try both /dev/fb0 and /dev/fb/0.\n\n");
			exit(0);
		}

		return std::make_unique<framebuffer_screen>(path);
	}

	// Linux DRM device
	if (device == "drm") {
#ifdef HAS_LIBDRM
		if (path == "?") {
			printf("Usage: --dev dev[:[path][@screen]]\n\n");
			printf("Use Linux Direct Rendering Manager (DRM) device,\n");
			printf("the optional path given after ':' can be used to point YAV to a specific\n");
			printf("DRM device driver to use. By default YAV will try to use /dev/dri/card0.\n");
			printf("The screen is an optional integer given after '@' that specifies the DRM connector to use, by default\n");
			printf("the value is read from environment variable 'DRM_CONNECTOR', if that is missing '0' is used.\n");
			printf("As the path is optional '--dev drm:@1' is a valid descriptor.\n\n");
			exit(0);
		}

		return std::make_unique<drm_screen>(path);
#else
		printf("This VAV build was compiled without DRM support, use --dev fb[:path]!");
		exit(1);
#endif
	}

	throw std::runtime_error("Unknown device '" + device + "' (expected 'fb', 'drm'), did you forget the ':'?");
}

static void entry(const std::vector<std::string>& args) {

	if (args.empty()) {
		usage();
		printf("Run 'yav --help' for more information!\n");
		return;
	}

	auto get_flag = [&](const char* option) {
		return std::find(args.begin(), args.end(), option);
	};

	auto get_either_flag = [&](const char* first, const char* second) {
		auto it = get_flag(first);

		if (it != args.end()) {
			return it;
		}

		return get_flag(second);
	};

	auto next_value = [&](decltype(args.begin())& it) -> std::string {
		++it;
		return (it == args.end()) ? "" : *it;
	};

	if (get_flag("--help") != args.end() || get_flag("-h") != args.end()) {
		help();
		return;
	}

	std::string fbdev_path;

	if (auto it = get_flag("--dev"); it != args.end()) {
		fbdev_path = next_value(it);
	}

	auto screen = make_screen(fbdev_path);

	if (get_flag("-v") != args.end()) {
		screen->dump();
	}

	if (auto it = get_flag("--view"); it != args.end()) {
		screen->view.ox = std::stoi(next_value(it));
		screen->view.oy = std::stoi(next_value(it));
		screen->view.w = std::stoi(next_value(it));
		screen->view.h = std::stoi(next_value(it));

		if (auto it = get_flag("--view-anchor"); it != args.end()) {
			screen->view.ax = std::stof(next_value(it));
			screen->view.ay = std::stof(next_value(it));
		}
	}

	if (auto it = get_either_flag("-c", "--clear"); it != args.end()) {
		auto next = it + 1;
		color c{};

		if ((next != args.end()) && !next->empty() && !next->starts_with("-")) {
			c = color::parse(next->c_str());
		}

		screen->clear(c);
	}

	if (auto it = get_flag("--image"); it != args.end()) {
		image img{next_value(it)};

		bool used_animation_flags = false;

		if (auto it = get_flag("--anchor"); it != args.end()) {
			img.ax = std::stof(next_value(it));
			img.ay = std::stof(next_value(it));
		}

		if (auto it = get_flag("--offset"); it != args.end()) {
			img.ox = std::stoi(next_value(it));
			img.oy = std::stoi(next_value(it));
		}

		if (auto it = get_flag("--time"); it != args.end()) {
			used_animation_flags = true;
			img.mspt = std::stoi(next_value(it)) * 1000; // micro to milli
		}

		if (auto it = get_flag("--loop"); it != args.end()) {
			used_animation_flags = true;
			int times = -1;
			auto next = it + 1;

			if (!next->empty() && std::isdigit(next->at(0))) {
				try {
					times = std::stoi(*(it + 1));
					++it;
				} catch (const std::exception&) {
					printf("Invalid value given to '--loop'!\n");
					return;
				}
			}

			img.loops = times;
		}

		if (get_flag("-b") != args.end() || get_flag("--blend") != args.end()) {
			img.blend = true;
		}

		if (get_flag("--static") != args.end() || get_flag("-s") != args.end()) {

			if (used_animation_flags) {
				printf("Option '--static' cannot be used with neither '--loop' nor '--time'!\n");
				return;
			}

			img.frames = 1;
			img.loops = 1;
		}

		screen->blit(img);
	}
}

int main(int argc, char* argv[]) {

	setup_interrupt_handlers();

	std::vector<std::string> args;
	args.reserve(argc);

	for (int i = 1; i < argc; i++) {
		args.emplace_back(argv[i]);
	}

	try {
		entry(args);
	} catch (const std::exception& e) {
		LOG_ERROR("%s\n", e.what());
		return 1;
	}

	return 0;
}
