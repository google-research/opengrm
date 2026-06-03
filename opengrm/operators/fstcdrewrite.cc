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

// Compiles a context-dependent rewrite rule.

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"

ABSL_FLAG(std::string, direction, "ltr",
          "Rewrite direction, one of: \"ltr\", \"rtl\", \"sim\"");
ABSL_FLAG(std::string, mode, "obl", "Rewrite mode, one of: \"obl\", \"opt\"");
ABSL_FLAG(int64_t, initial_boundary_marker, ::fst::kNoLabel, "BOS label");
ABSL_FLAG(int64_t, final_boundary_marker, ::fst::kNoLabel, "EOS label");

int fstcdrewrite_main(int argc, char** argv);

int main(int argc, char** argv) { return fstcdrewrite_main(argc, argv); }
