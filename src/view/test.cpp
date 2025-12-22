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

#include "core/framebuffer.hpp"

#define ASSERT(...)                                            \
	if (!(__VA_ARGS__)) {                                      \
		printf("Assertion failed: " __FILE__ ":%d", __LINE__); \
		exit(1);                                               \
	}

static void test_framebuffer_channel() {

	channel channel{6, 3};

	ASSERT(channel.is_used())

	ASSERT(channel.decode(0b111111'101) == 255)
	ASSERT(channel.decode(0b011111'101) == 125)
	ASSERT(channel.decode(0b111'000000'111) == 0)

	ASSERT(channel.encode(255) == 0b111111'000)
	ASSERT(channel.encode(125) == 0b011110'000)
	ASSERT(channel.encode(0) == 0)
}

static void test_framebuffer_format() {

	format fmt{16, {5, 11}, {6, 5}, {5, 0}, {}};

	ASSERT(fmt.color())
	ASSERT(fmt.encode_rgb(255, 125, 0) == 0b11111'011110'00000)

	uint8_t r, g, b;
	fmt.decode_rgb(0b11111'011111'00000, &r, &g, &b);

	ASSERT(r == 255)
	ASSERT(g == 125)
	ASSERT(b == 0)
}

int main() {
	test_framebuffer_channel();
	test_framebuffer_format();
}