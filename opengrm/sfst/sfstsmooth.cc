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

// Thin wrapper for sfstsmooth.

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"

ABSL_FLAG(std::string, method, "witten_bell",
          "Smoothing method: witten_bell, absolute, unsmoothed, kneser_ney, "
          "modified_kneser_ney, katz, presmoothed");
ABSL_FLAG(double, witten_bell_k, 1.0, "Witten-Bell hyperparameter K");
ABSL_FLAG(double, discount_D, 0.75,
          "Discount constant D for absolute discounting");
ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: kNoLabel)");
ABSL_FLAG(int64_t, bins, 5,
          "Number of bins for Katz (default: 5) and Modified Kneser-Ney "
          "(default: 3) smoothing");

int sfstsmooth_main(int argc, char** argv);

int main(int argc, char** argv) { return sfstsmooth_main(argc, argv); }
