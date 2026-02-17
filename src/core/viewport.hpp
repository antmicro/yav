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

#pragma once

#include <initializer_list>

struct position {
	int x = 0;
	int y = 0;
};

struct constraint {
	position min;
	position max;

	constraint() = default;
	constraint(int x, int y, int w, int h);

	int width() const;
	int height() const;

	position offset(const constraint& other) const;
};

struct viewport {
	int w, h;

	float ax, ay;
	float ox, oy;

	viewport();
	viewport(float ax, float ay);

	position get_position(constraint canvas) const;
	constraint get_constraint(constraint canvas) const;

	int width() const;
	int height() const;
};

constraint get_constraint_intersection(std::initializer_list<constraint> boxes);
