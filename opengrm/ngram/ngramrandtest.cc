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

#include <cstdint>
#include <optional>
#include <string>

#include "absl/flags/flag.h"

ABSL_FLAG(int32_t, max_length, 1000, "Maximum sentence length");
ABSL_FLAG(std::optional<uint64_t>, seed, std::nullopt, "Randomization seed");
ABSL_FLAG(int32_t, vocabulary_max, 5000, "maximum vocabulary size");
ABSL_FLAG(int32_t, mean_length, 100, "maximum mean string length");
ABSL_FLAG(int32_t, sample_max, 10000, "maximum sample corpus size");
ABSL_FLAG(int32_t, ngram_max, 3, "maximum n-gram order size");
ABSL_FLAG(std::string, directory, ".", "directory where files will be placed");
ABSL_FLAG(std::string, vars, "", "file name for outputting variable values");
ABSL_FLAG(double, thresh_max, 3, "maximum threshold size");

int ngramrandtest_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramrandtest_main(argc, argv); }
