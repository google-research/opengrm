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

// Computes the shortest distance with failure transitions in a stochastic FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/shortest-distance.h"
#include "opengrm/sfst/shortest-distance.h"
#include "opengrm/sfst/state-weights.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(bool, reverse);
ABSL_DECLARE_FLAG(double, delta);

int sfstshortestdistance_main(int argc, char** argv) {
  std::string usage = "Computes the shortest distance with failure transitions";
  usage += " in a stochastic FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  const std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdFst> fst(fst::StdFst::Read(in_name));
  if (!fst) return 1;

  std::vector<fst::StdArc::Weight> distance;
  auto total_weight = sfst::ShortestDistance(
      *fst, &distance, absl::GetFlag(FLAGS_phi_label),
      absl::GetFlag(FLAGS_reverse), absl::GetFlag(FLAGS_delta));
  if (!total_weight.Member()) return 1;

  if (!sfst::WriteWeights(out_name, distance)) return 1;

  return 0;
}
