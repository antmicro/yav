// Copyright 2026 Antmicro
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

#include "color.hpp"

#include <cstring>
#include <stdexcept>

static uint8_t parse_nibble(const char digit) {
	if (digit <= '9' && digit >= '0') {
		return digit - '0';
	}

	if (digit <= 'f' && digit >= 'a') {
		return digit - 'a' + 10;
	}

	if (digit <= 'F' && digit >= 'A') {
		return digit - 'A' + 10;
	}

	std::string wrong{};

	if (digit <= '~' && digit >= ' ') {
		wrong.push_back('\'');
		wrong.push_back(digit);
		wrong.push_back('\'');
	} else {
		wrong += "ASCII ";
		wrong += std::to_string((int)digit);
	}

	throw std::runtime_error{"Invalid hex digit " + wrong + ", expected [0-9a-fA-F]!"};
}

static uint8_t parse_byte(const char* pair) {
	return (parse_nibble(pair[0]) << 4) | parse_nibble(pair[1]);
}

// region color

color color::parse(const char* code) {

	if (code[0] == 0) {
		return {};
	}

	if (code[0] == '#') {
		code += 1;
	} else if (code[0] == '0' && code[1] == 'x') {
		code += 2;
	}

	color c{};
	int length = strlen(code);

	if (length == 8) {
		c.a = parse_byte(code);
		code += 2;
	}

	else if (length != 6) {
		throw std::runtime_error{"Invalid color code, expected 6 or 8 digits!"};
	}

	c.r = parse_byte(code + 0);
	c.g = parse_byte(code + 2);
	c.b = parse_byte(code + 4);

	return c;
}

color color::from_rgba(const uint8_t* pixel) {
	color c{};

	c.r = pixel[0];
	c.g = pixel[1];
	c.b = pixel[2];
	c.a = pixel[3];

	return c;
}
