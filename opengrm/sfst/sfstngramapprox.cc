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

// Algorithm to approximate a stochastic FST as an n-gram model.
// The output is a canonical and normalized OpenGrm ngram model.

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "opengrm/sfst/approx.h"

ABSL_FLAG(int64_t, order, 3, "Set maximal order of ngram model");
ABSL_FLAG(int64_t, phi_label, 0, "Specifies failure label (default: 0)");
ABSL_FLAG(double, delta, sfst::kApproxDelta, "Convergence delta");
ABSL_FLAG(std::string, norm_type, "kl_min",
          "Normalization type, one of: "
          "\"summed\", \"kl_min\", \"kl_min_approximated");

int sfstngramapprox_main(int argc, char** argv);
int main(int argc, char** argv) { return sfstngramapprox_main(argc, argv); }
