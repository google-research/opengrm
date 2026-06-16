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

// Counts n-grams from an input fst archive (FAR) file and builds SFST topology.

#include <cstdint>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"

ABSL_FLAG(int64_t, order, 3, "Set maximal order of ngram model");
ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: kNoLabel)");
ABSL_FLAG(bool, epsilon_as_backoff, false,
          "Treat epsilons as backoff transitions in input Fsts");
ABSL_FLAG(bool, require_symbols, true, "Require symbol tables? (default: yes)");

int sfstngramcount_main(int argc, char** argv);
int main(int argc, char** argv) { return sfstngramcount_main(argc, argv); }
