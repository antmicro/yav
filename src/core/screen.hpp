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

#include "format.hpp"
#include "image.hpp"

class screen {

protected:

	// Pointer to the start of underlying data, in screen-specific format
	virtual void* data() const = 0;

	// Write image into the screen
	void blit_frame(const image& img, int frame);

public:

	virtual ~screen() = default;

	// Write image into the screen
	void blit(const image& img);

	// Clear the screen contents
	void clear();

	// Print generic information about this screen to the standard output
	virtual void dump() = 0;

	// Width, in pixels, of the screen
	virtual int width() const = 0;

	// Height, in pixels, of the screen
	virtual int height() const = 0;

	// Get data format fused by this screen
	virtual format form() const = 0;

	// Flush screen content
	virtual void flush() const = 0;
};
