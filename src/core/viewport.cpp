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

#include "viewport.hpp"

#include <algorithm>

// region constraint

constraint::constraint(int x, int y, int w, int h)
	: min(x, y), max(x + w, y + h) {
}

int constraint::width() const {
	return max.x - min.x;
}

int constraint::height() const {
	return max.y - min.y;
}

position constraint::offset(const constraint& other) const {
	return {other.min.x - min.x, other.min.y - min.y};
}

constraint get_constraint_intersection(std::initializer_list<constraint> boxes) {
	constraint inter = *boxes.begin();

	for (auto& box : boxes) {
		inter.min.x = std::max(inter.min.x, box.min.x);
		inter.min.y = std::max(inter.min.y, box.min.y);
		inter.max.x = std::min(inter.max.x, box.max.x);
		inter.max.y = std::min(inter.max.y, box.max.y);
	}

	return inter;
}

// region viewport

viewport::viewport()
	: viewport(0.0, 0.0) {
}

viewport::viewport(float ax, float ay)
	: w(-1), h(-1), ax(ax), ay(ay), ox(0), oy(0) {
}

position viewport::get_position(constraint canvas) const {
	position placed;

	// [ax, ay] describe the position in screen space coordinates (in range [0,
	// 1] the placement of the image's matching point, where (0, 0) is the top
	// left screen corner. (so for (-1, -1) top-left image corner in top-left
	// screen corner, and for (1, 1) bottom-right image corner in bottom-right
	// screen corner). We add the [ox, oy], to allow for-fine tuning the image's
	// position in pixels.
	placed.x = ox + static_cast<float>(canvas.width()) * ax - static_cast<float>(w) * ax + canvas.min.x;
	placed.y = oy + static_cast<float>(canvas.height()) * ay - static_cast<float>(h) * ay + canvas.min.y;

	return placed;
}

constraint viewport::get_constraint(constraint canvas) const {
	constraint c;

	c.min = get_position(canvas);
	c.max = {c.min.x + w, c.min.y + h};

	return c;
}

int viewport::width() const {
	return w;
}

int viewport::height() const {
	return h;
}
