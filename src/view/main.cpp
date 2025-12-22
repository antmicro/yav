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

#include <algorithm>
#include <core/logger.hpp>
#include <filesystem>
#include <vector>

#include "core/framebuffer.hpp"

static void usage() {
	printf("Usage: yav [--image <path>] [--anchor <x> <y>] [--offset <x> <y>]\n");
	printf("           [-v] [--fbdev <path>] [-c|--clear] [-h|--help] [-b|--blend]\n");
}

static void help() {
	usage();
	printf("Options:\n");
	printf("  -h, --help           : Show this help page and exit\n");
	printf("  -v                   : Verbose mode\n");
	printf("      --dev <d[:path]> : Device type (Must be 'fb') and, optionally, path\n");
	printf("      --image <path>   : Image file path\n");
	printf("      --anchor <x> <y> : Anchor as fractions in range 0 to 1\n");
	printf("      --offset <x> <y> : Offset in pixels\n");
	printf("  -b, --blend          : Enable alpha-blending\n");
	printf("  -c, --clear          : Clear the framebuffer\n");
	printf("\nExample:\n");
	printf("  yav --image example/tuxan.png --anchor 0.5 0.5\n");
	printf("  yav --image example/tuxan.png --anchor 1 1 --offset -100 -100\n");
	printf("  yav --image example/splash.png --anchor 0.5 0.5 --blend\n");
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

	if (device == "fb") {
		return std::make_unique<framebuffer_screen>(path);
	}

	throw std::runtime_error("Unknown device, expected 'fb'");
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

	if (get_flag("--clear") != args.end() || get_flag("-c") != args.end()) {
		screen->clear();
	}

	if (auto it = get_flag("--image"); it != args.end()) {
		image img{next_value(it)};

		if (auto it = get_flag("--anchor"); it != args.end()) {
			img.sx = std::stof(next_value(it));
			img.sy = std::stof(next_value(it));
		}

		if (auto it = get_flag("--offset"); it != args.end()) {
			img.ox = std::stoi(next_value(it));
			img.oy = std::stoi(next_value(it));
		}

		if (get_flag("-b") != args.end() || get_flag("--blend") != args.end()) {
			img.blend = true;
		}

		screen->blit(img);
	}
}

int main(int argc, char* argv[]) {

	std::vector<std::string> args;
	args.reserve(argc);

	for (int i = 1; i < argc; i++) {
		args.emplace_back(argv[i]);
	}

	try {
		entry(args);
	} catch (const std::exception& e) {
		LOG_ERROR("%s\n", e.what());
	}

	return 0;
}