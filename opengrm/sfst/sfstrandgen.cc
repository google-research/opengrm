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

// Generates random paths through a stochastic FST. The FST must be a
// normalized.

#include <climits>
#include <cstdint>
#include <optional>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"

ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: none)");
ABSL_FLAG(int32_t, max_length, INT_MAX, "Maximum path length");
ABSL_FLAG(int64_t, npath, 1, "Number of paths to generate");
ABSL_FLAG(std::optional<uint64_t>, seed, std::nullopt, "Random seed");
ABSL_FLAG(bool, weighted, false,
          "Output tree weighted by path count vs. unweighted paths");
ABSL_FLAG(bool, remove_total_weight, false,
          "Remove total weight when output weighted");
ABSL_FLAG(bool, minimal, false,
          "Epsilon/phi-remove and minimize when output is weighted");
ABSL_FLAG(bool, stochastic, false,
          "Same as --weighted --remove_total_weight --minimal");

int sfstrandgen_main(int argc, char** argv);
int main(int argc, char** argv) { return sfstrandgen_main(argc, argv); }
