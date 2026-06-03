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

ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(std::string, contexts, "", "Context patterns file");
ABSL_FLAG(std::string, method, "count_split",
          "One of: \"count_split\", \"histogram_split\"");
ABSL_FLAG(double, norm_eps, ngram::kNormEps, "Normalization check epsilon");
ABSL_FLAG(bool, complete, false, "Complete partial models");
// TODO: Change the default `far_type` from this rather strange
// empty string implying to create a bunch of FSTs to the literal `"default"`.
// Note that the disttests depend on this default implicitly, so they will have
// to be provided `--far_type=""`.
ABSL_FLAG(std::string, far_type, "",
          "Type of far to compile (not FAR if empty):"
          " one of: \"default\", "
          "\"stlist\", \"sttable\"");

int ngramsplit_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramsplit_main(argc, argv); }
