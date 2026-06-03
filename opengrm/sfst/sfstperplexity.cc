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

// Computes the perplexity for a stochastic FST. The FST must be normalized.
// The FST must be topologically sorted and have a single path.

#include <cstdint>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"
#include "opengrm/sfst/perplexity.h"

ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: none)");
ABSL_FLAG(int64_t, unknown_label, fst::kNoLabel,
          "Unknown symbol label (determines OOV handling)");
ABSL_FLAG(bool, detailed, false, "Compute perplexity per source");
ABSL_FLAG(double, delta, fst::kDelta, "Comparison delta");
ABSL_FLAG(double, entropy_delta, sfst::kEntropyDelta,
          "Convergence delta for entropy/perplexity algorithms");

int sfstperplexity_main(int argc, char** argv);
int main(int argc, char** argv) { return sfstperplexity_main(argc, argv); }
