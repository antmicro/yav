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

#include "image.hpp"

class screen {

public:

	virtual ~screen() = default;

	// Write image into the screen
	virtual void blit(const image& img) = 0;

	// Clear the screen contents
	virtual void clear() = 0;

	// Print generic information about this screen to the standard output
	virtual void dump() = 0;
};
