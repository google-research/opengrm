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

ABSL_FLAG(double, total_unigram_count, -1.0, "Total unigram count");
ABSL_FLAG(double, theta, 0.0, "Pruning threshold theta");
ABSL_FLAG(int64_t, target_number_of_ngrams, -1,
          "Maximum number of ngrams to leave in model after pruning. "
          "Value less than zero means no target number, just use theta.");
ABSL_FLAG(int32_t, min_order_to_prune, 2, "Minimum n-gram order to prune");
ABSL_FLAG(std::string, method, "seymore",
          "One of: \"context_prune\", \"count_prune\", "
          "\"relative_entropy\", \"seymore\", \"list_prune\"");
ABSL_FLAG(std::string, list_file, "", "File with list of n-grams to prune");
ABSL_FLAG(std::string, count_pattern, "", "Pattern of counts to prune");
ABSL_FLAG(std::string, context_pattern, "", "Pattern of contexts to prune");
ABSL_FLAG(int32_t, shrink_opt, 0,
          "Optimization level: Range 0 (fastest) to 2 (most accurate)");
ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(double, norm_eps, ngram::kNormEps, "Normalization check epsilon");
ABSL_FLAG(bool, check_consistency, false, "Check model consistency");
ABSL_FLAG(bool, retry_downcase, false,
          "If a pruned symbol is not found in the FST, automatically tries the "
          "lower-cased variant of this symbol. Only useful in list_prune "
          "mode.");

int ngramshrink_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramshrink_main(argc, argv); }
