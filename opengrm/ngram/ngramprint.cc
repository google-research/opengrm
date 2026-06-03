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

ABSL_FLAG(bool, ARPA, false, "Print in ARPA format");
ABSL_FLAG(bool, backoff, false,
          "Show epsilon backoff transitions when printing");
ABSL_FLAG(bool, backoff_inline, false,
          "Show epsilon backoffs transitions inline with context as a third "
          "field if --backoff are being printed");
ABSL_FLAG(bool, negativelogs, false,
          "Show negative log probs/counts when printing");
ABSL_FLAG(bool, integers, false, "Show just integer counts when printing");
ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(bool, check_consistency, false, "Check model consistency");
ABSL_FLAG(std::string, context_pattern, "", "Pattern of contexts to print");
ABSL_FLAG(bool, include_all_suffixes, false, "Include suffixes of contexts");
ABSL_FLAG(std::string, symbols, "",
          "Symbol table file. If not empty, causes it to be loaded from the"
          " specified file instead of using the one inside the input FST.");

int ngramprint_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramprint_main(argc, argv); }
