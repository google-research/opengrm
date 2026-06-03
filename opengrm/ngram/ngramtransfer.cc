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

ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(std::string, context_pattern1, "", "Context pattern for first model");
ABSL_FLAG(std::string, context_pattern2, "",
          "Context pattern for second model");
ABSL_FLAG(std::string, contexts, "", "Context patterns files (all FSTs)");
ABSL_FLAG(std::string, ofile, "", "Output file (prefix)");
ABSL_FLAG(std::string, method, "count_transfer",
          "One of \"count_transfer\", \"histogram_transfer\"");
ABSL_FLAG(int32_t, index, -1, "Specifies one FST as the destination (source)");
ABSL_FLAG(bool, transfer_from, false,
          "Transfer from (to) other FSTS to indexed FST");
ABSL_FLAG(bool, normalize, false, "Recompute backoff weights after transfer");
ABSL_FLAG(bool, complete, false, "Complete partial models");

int ngramtransfer_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramtransfer_main(argc, argv); }
