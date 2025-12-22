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

#include <cstdint>
#include <cstdio>

// Describes the data layout of a single channel
// in a single pixel
struct channel {

	unsigned int length = 0;
	unsigned int offset = 0;
	unsigned int mask = 0;

	channel() = default;
	channel(unsigned int length, unsigned int offset);

	// Check if this channel in any way contributes to the format (has
	// non-zero mask)
	bool is_used() const;

	// Encodes a single value into the format described by this channel
	size_t encode(uint8_t value) const;

	// Given an encoded pixel return back the channel value
	uint8_t decode(size_t value) const;

	// Print simple overview to the standard output
	void dump(const char* name) const;
};

// Describes the data layout in a single pixel
// can be used to convert RGB data to that layout
struct format {

	unsigned int bits;
	channel r, g, b, a;

	format(unsigned int bits, channel r, channel g, channel b, channel a);

	// Does this format have RGB channels
	bool pseudocolor() const;

	// Does this format have separate RGB channels
	bool color() const;

	// Encode RGB data into the format
	size_t encode_rgb(uint8_t sr, uint8_t sg, uint8_t sb) const;

	// Encode transparency data into the format
	size_t encode_alpha(uint8_t alpha) const;

	// Print simple overview to the standard output
	void dump() const;

	// How many bytes a single pixel takes in this format
	size_t bytes() const;

	// given an encoded pixel computes the stored RGB values
	void decode_rgb(size_t pixel, uint8_t* r, uint8_t* g, uint8_t* b) const;
};