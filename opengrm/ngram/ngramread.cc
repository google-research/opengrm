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

#include <string>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"

ABSL_FLAG(bool, ARPA, false, "Read model from ARPA format");
ABSL_FLAG(bool, renormalize_arpa, false,
          "If true, attempts to renormalize an unnormalized ARPA format "
          "model by normalizing the unigram state and recomputing the "
          "backoff weights.  Only used if --ARPA=true.");
ABSL_FLAG(std::string, symbols, "", "Label symbol table");
ABSL_FLAG(std::string, epsilon_symbol, "<epsilon>",
          "Label for epsilon transitions");
ABSL_FLAG(std::string, OOV_symbol, "<UNK>", "Class label for OOV symbols");
ABSL_DECLARE_FLAG(std::string, start_symbol);  // defined in ngram-output.cc
ABSL_DECLARE_FLAG(std::string, end_symbol);    // defined in ngram-output.cc

int ngramread_main(int argc, char** argv);
int main(int argc, char** argv) { return ngramread_main(argc, argv); }
