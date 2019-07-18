
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2018 Google, Inc.
// sfstshortestdistance.cc

// Computes the shortest distance with failure transitions in a
// stochastic FST.

#include <string.h>

#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/fst-decl.h>
#include <fst/mutable-fst.h>
#include <sfst/shortest-distance.h>
#include <sfst/state-weights.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_bool(reverse, false, "Perform in the reverse direction");
DEFINE_double(delta, fst::kShortestDelta, "Convergence delta");

int main(int argc, char **argv) {
  namespace f = fst;
  std::string usage = "Computes the shortest distance with failure transitions";
  usage += " in a stochastic FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const std::string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0))
                              ? argv[1] : "";
  const std::string out_name = argc > 2 ? argv[2] : "";

  f::StdFst *fst = f::StdFst::Read(in_name);
  if (!fst) return 1;

  std::vector<f::StdArc::Weight> distance;
  if (!sfst::PhiShortestDistance(*fst, &distance, FLAGS_phi_label,
                                 FLAGS_reverse, FLAGS_delta))
    return 1;

  if (!sfst::WriteWeights(out_name, distance))
    return 1;

  return 0;
}
