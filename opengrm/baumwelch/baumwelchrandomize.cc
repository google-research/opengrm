// Copyright 2026 The OpenGrm Authors.
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

// Randomizes model weights.

#include <cstdint>

#include "absl/flags/flag.h"
#include "openfst/script/getters.h"

ABSL_FLAG(uint64_t, seed, 0, "Random seed");

int baumwelchrandomize_main(int argc, char** argv);

int main(int argc, char** argv) { return baumwelchrandomize_main(argc, argv); }
