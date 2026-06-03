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

ABSL_FLAG(std::string, method, "counts",
          "One of: \"counts\", \"histograms\", \"count_of_counts\", "
          "\"count_of_histograms\"");
ABSL_FLAG(int64_t, order, 3, "Set maximal order of ngrams to be counted");

// For counting:
ABSL_FLAG(bool, round_to_int, false, "Round all counts to integers");
ABSL_FLAG(bool, output_fst, true, "Output counts as fst (otherwise strings)");
ABSL_FLAG(bool, require_symbols, true, "Require symbol tables? (default: yes)");
ABSL_FLAG(double, add_to_symbol_unigram_count, 0.0,
          "Adds this amount to the unigram count of each word in the symbol "
          "table");

// For counting and histograms:
ABSL_FLAG(bool, epsilon_as_backoff, false,
          "Treat epsilon in the input Fsts as backoff");

// For count-of-counting:
ABSL_FLAG(std::string, context_pattern, "", "Pattern of contexts to count");

// For merging:
ABSL_FLAG(double, alpha, 1.0, "Weight for first FST");
ABSL_FLAG(double, beta, 1.0, "Weight for second (and subsequent) FST(s)");
ABSL_FLAG(bool, normalize, false, "Normalize resulting model");
ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(double, norm_eps, ngram::kNormEps, "Normalization check epsilon");
ABSL_FLAG(bool, check_consistency, false, "Check model consistency");

int ngramcount_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramcount_main(argc, argv); }
