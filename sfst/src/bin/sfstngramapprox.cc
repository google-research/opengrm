
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
// sfstngramapprox.cc

// Algorithm to approximate a stochastic FST as an n-gram model.
// Output is a canonical and normalized OpenGrm ngram model.

#include <string.h>

#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/cache.h>
#include <fst/mutable-fst.h>
#include <fst/shortest-distance.h>
#include <sfst/approx.h>
#include <sfst/ngramapprox.h>
#include <sfst/normalize.h>

DEFINE_int64(order, 3, "Set maximal order of ngram model");
DEFINE_int64(phi_label, 0, "Specifies failure label (default: 0)");
DEFINE_double(delta, sfst::kApproxDelta, "Convergence delta");
DEFINE_string(norm_type, "kl_min",
              "Normalization type, one of: "
              "\"summed\", \"kl_min\", \"kl_min_approximated");

int main(int argc, char **argv) {
  namespace f = fst;
  std::string usage = "Algorithm to approximate a stochastic FST";
  usage += " as an n-gram model.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  sfst::CountNormType norm_type;
  if (FLAGS_norm_type == "summed") {
    norm_type = sfst::NORM_SUMMED;
  } else if (FLAGS_norm_type == "kl_min" ||
             FLAGS_norm_type == "marginally_constrained" /* deprecated */) {
    norm_type = sfst::NORM_KL_MIN;
  } else if (FLAGS_norm_type == "kl_min_approximated" ||
             FLAGS_norm_type == "marginally_approximated") {
    norm_type = sfst::NORM_KL_MIN_APPROXIMATED;
  } else {
    LOG(ERROR) << argv[0] << ": Bad norm type: " << FLAGS_norm_type;
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  f::StdFst *ifst = f::StdFst::Read(in_name);
  if (!ifst) return 1;

  if (ifst->Properties(fst::kCyclic, true) &&
      !sfst::IsNormalized(*ifst, FLAGS_phi_label)) {
    LOG(ERROR) << argv[0] << ": Input is not a normalized stochastic FST";
    return 1;
  }

  f::StdVectorFst ofst;
  if (!sfst::NGramApprox(*ifst, &ofst, FLAGS_order, FLAGS_phi_label,
                         FLAGS_delta, norm_type))
    return 1;

  if (!ofst.Write(out_name)) return 1;

  return 0;
}
