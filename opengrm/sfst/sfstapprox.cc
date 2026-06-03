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

// Algorithm to approximate a stochastic FST as a backoff-complete FST.
// The backoff-complete FST topology is provided. The result is the
// backoff-complete FST weighted and normalized to approximate the input.

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"
#include "opengrm/sfst/approx.h"

ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: none)");
ABSL_FLAG(double, delta, sfst::kApproxDelta, "Convergence delta");
ABSL_FLAG(std::string, norm_type, "kl_min",
          "Normalization type, one of: "
          "\"summed\", \"kl_min\", \"kl_min_approximated");

int sfstapprox_main(int argc, char** argv);
int main(int argc, char** argv) { return sfstapprox_main(argc, argv); }
