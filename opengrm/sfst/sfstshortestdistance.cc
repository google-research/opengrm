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

// Computes the shortest distance with failure transitions in a stochastic FST.

#include <cstdint>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/shortest-distance.h"

ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: none)");
ABSL_FLAG(bool, reverse, false, "Perform in the reverse direction");
ABSL_FLAG(double, delta, fst::kShortestDelta, "Convergence delta");

int sfstshortestdistance_main(int argc, char** argv);
int main(int argc, char** argv) {
  return sfstshortestdistance_main(argc, argv);
}
