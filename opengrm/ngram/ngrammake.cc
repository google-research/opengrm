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
#include <string>

#include "absl/flags/flag.h"
#include "opengrm/ngram/ngram-model.h"

ABSL_FLAG(double, witten_bell_k, 1, "Witten-Bell hyperparameter K");
ABSL_FLAG(double, discount_D, -1, "Absolute discount value D to use");
ABSL_FLAG(std::string, method, "katz",
          "One of: \"absolute\", \"katz\", \"kneser_ney\", "
          "\"presmoothed\", \"unsmoothed\", \"katz_frac\", "
          "\"witten_bell\"");
ABSL_FLAG(bool, backoff, false,
          "Use backoff smoothing (default: method dependent)");
ABSL_FLAG(bool, interpolate, false,
          "Use interpolated smoothing (default: method dependent)");
ABSL_FLAG(int64_t, bins, -1, "Number of bins for katz or absolute discounting");
ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(double, norm_eps, ngram::kNormEps, "Normalization check epsilon");
ABSL_FLAG(bool, check_consistency, false, "Check model consistency");
ABSL_FLAG(std::string, count_of_counts, "", "Read count-of-counts from file");

int ngrammake_main(int argc, char** argv);
int main(int argc, char** argv) { return ngrammake_main(argc, argv); }
