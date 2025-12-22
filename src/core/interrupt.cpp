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

#include "interrupt.hpp"
#include <csignal>

static volatile bool is_interrupted = false;

void signal_handler(int ignored) {
	(void)ignored;
	is_interrupted = true;
}

void setup_interrupt_handlers() {
	signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGTERM, signal_handler);
}

bool was_interrupted() {
	return is_interrupted;
}