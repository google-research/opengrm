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

// Removes useless states and transitions in stochastic automata.

#include <cstdint>
#include <string>

#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"

ABSL_FLAG(int64_t, phi_label, fst::kNoLabel,
          "Specifies failure label (default: none)");
ABSL_FLAG(bool, phi_trim, true,
          "Removes inaccessible transitions due to failure labels");
ABSL_FLAG(bool, weight_trim, false, "Removes ApproxZero() weight transitions");
ABSL_FLAG(bool, sum_weight_trim, false,
          "Removes ApproxZero() weight transitions wrt the phi-summed SFST");
ABSL_FLAG(bool, include_phi, false,
          "Include phi transitions when weight trimming");
ABSL_FLAG(bool, connect, true,
          "Removes inaccessible/non-accessible states treating"
          " failure labels as regular labels");
ABSL_FLAG(std::string, trim_type, "needed_final",
          "Trim type, one of: "
          "\"needed_trim\", \"needed_final\", "
          "\"needed_nonfinal");
ABSL_FLAG(double, weight, 99.0, "Weight threshold");

int sfsttrim_main(int argc, char** argv);
int main(int argc, char** argv) { return sfsttrim_main(argc, argv); }
