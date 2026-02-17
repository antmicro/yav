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

#pragma once
#include <string>

#include "viewport.hpp"

class image : public viewport {

	enum ownership {
		STB,
		EXTERNAL
	};

	ownership owner = EXTERNAL;
	unsigned char* pixels = nullptr;

public:

	bool blend = false;
	int frames = 1;
	int mspt = 41'666;
	int loops = 1;

	image(const std::string& path);
	image(unsigned char* pixels, int w, int h);
	~image();

	unsigned char* data(int frame) const;
	void dump() const;
	int frame_count() const;
};
