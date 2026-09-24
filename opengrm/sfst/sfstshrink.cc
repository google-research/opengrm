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

// Thin wrapper for sfstshrink.

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"

// ABSL_FLAG uses ABSL_CONST_INIT, which expands to C++20 `constinit` when
// analyzed in C++20 mode and falls back to C++17-compatible attributes under
// `-std=c++17`.
// NOLINTBEGIN(clang-diagnostic-pre-c++20-compat)
ABSL_FLAG(double, theta, 0.0, "Threshold for shrinking");
ABSL_FLAG(int64_t, target_number_of_ngrams, -1,
          "Maximum number of ngrams to leave in model after pruning. "
          "Value less than zero means no target number, just use theta.");
ABSL_FLAG(int32_t, min_order_to_prune, 2, "Minimum n-gram order to prune");
ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: kNoLabel)");
ABSL_FLAG(std::string, method, "stolcke", "Shrinking method");
ABSL_FLAG(double, total_unigram_count, -1.0,
          "Total unigram count (for Seymore)");
ABSL_FLAG(std::string, count_pattern, "", "Count pattern (for count prune)");
ABSL_FLAG(std::string, list_file, "", "File containing n-grams to prune");
ABSL_FLAG(std::string, count_fst, "", "Count FST for significance pruning");
ABSL_FLAG(std::string, word_set, "",
          "Words for word_shrink (file or comma-separated)");
// NOLINTEND(clang-diagnostic-pre-c++20-compat)

int sfstshrink_main(int argc, char** argv);

int main(int argc, char** argv) { return sfstshrink_main(argc, argv); }
