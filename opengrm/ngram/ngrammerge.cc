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

ABSL_FLAG(double, alpha, 1.0, "Weight for first FST");
ABSL_FLAG(double, beta, 1.0, "Weight for second (and subsequent) FST(s)");
ABSL_FLAG(std::string, context_pattern, "", "Context pattern for second FST");
ABSL_FLAG(std::string, contexts, "", "Context patterns file (all FSTs)");
ABSL_FLAG(bool, normalize, false, "Normalize resulting model");
ABSL_FLAG(std::string, method, "count_merge",
          "One of: \"context_merge\", \"count_merge\", \"model_merge\" "
          "\"bayes_model_merge\", \"histogram_merge\", \"replace_merge\"");
ABSL_FLAG(int32_t, max_replace_order, -1,
          "Maximum order to replace in replace_merge, ignored if < 1.");
ABSL_FLAG(std::string, ofile, "", "Output file");
ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(double, norm_eps, ngram::kNormEps, "Normalization check epsilon");
ABSL_FLAG(bool, check_consistency, false, "Check model consistency");
ABSL_FLAG(bool, complete, false, "Complete partial models");
ABSL_FLAG(bool, round_to_int, false, "Round all merged counts to integers");

int ngrammerge_main(int argc, char** argv);
int main(int argc, char** argv) { return ngrammerge_main(argc, argv); }
