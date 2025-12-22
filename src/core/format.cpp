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

#include "format.hpp"

// region channel

channel::channel(unsigned int length, unsigned int offset)
	: offset(offset), length(length), mask((1 << length) - 1) {
}

bool channel::is_used() const {
	return mask != 0;
}

size_t channel::encode(uint8_t value) const {
	const size_t mapped = (value * mask) / 255;
	return (mapped & mask) << offset;
}

uint8_t channel::decode(size_t value) const {
	const size_t field = (value >> offset) & mask;
	return (field * 255) / mask;
}

void channel::dump(const char* name) const {
	printf("%s=%02x@%d ", name, mask, offset);
}

// region format

format::format(unsigned int bits, channel r, channel g, channel b, channel a)
	: bits(bits), r(r), g(g), b(b), a(a) {
}

bool format::pseudocolor() const {
	return r.is_used() && g.is_used() && b.is_used();
}

bool format::color() const {
	return pseudocolor() && (r.offset != g.offset) && (g.offset != b.offset) && (r.offset != b.offset);
}

size_t format::encode_rgb(uint8_t sr, uint8_t sg, uint8_t sb) const {
	return r.encode(sr) | g.encode(sg) | b.encode(sb);
}

size_t format::encode_alpha(uint8_t alpha) const {
	return a.encode(alpha);
}

void format::dump() const {
	r.dump("red");
	g.dump("green");
	b.dump("blue");
	a.dump("alpha");
}

size_t format::bytes() const {
	return bits / 8;
}

void format::decode_rgb(size_t pixel, uint8_t* dr, uint8_t* dg, uint8_t* db) const {
	*dr = r.decode(pixel);
	*dg = g.decode(pixel);
	*db = b.decode(pixel);
}
