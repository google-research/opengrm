
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
// sfstapprox.cc

// Algorithm to approximate a stochastic FST as a backoff-complete FST.
// The backoff-complete FST topology is provided. The result is the
// backoff-complete FST weighted and normalized to approximate the input.

#include <string.h>

#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/cache.h>
#include <fst/mutable-fst.h>
#include <fst/shortest-distance.h>
#include <sfst/approx.h>
#include <sfst/normalize.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_double(delta, sfst::kApproxDelta, "Convergence delta");
DEFINE_string(norm_type, "kl_min",
              "Normalization type, one of: "
              "\"summed\", \"kl_min\", \"kl_min_approximated");

int main(int argc, char **argv) {
  namespace f = fst;
  std::string usage = "Algorithm to approximate a stochastic FST as a";
  usage += " backoff-complete FST whose topology is provided.\n\n  Usage: ";
  usage += argv[0];
  usage += " sfst.fst top.fst [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 4) {
    ShowUsage();
    return 1;
  }

  const std::string in1_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string in2_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";
  const std::string out_name = argc > 3 ? argv[3] : "";

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input";
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

  f::StdFst *ifst = f::StdFst::Read(in1_name);
  if (!ifst) return 1;

  f::StdMutableFst *ofst = f::StdMutableFst::Read(in2_name, true);
  if (!ofst) return 1;

  if (ifst->Properties(fst::kCyclic, true) &&
      !sfst::IsNormalized(*ifst, FLAGS_phi_label)) {
    LOG(ERROR) << argv[0] << ": First input is not a normalized stochastic FST";
    return 1;
  }

  if (!sfst::Approx(*ifst, ofst, FLAGS_phi_label, FLAGS_delta, norm_type))
    return 1;

  if (!ofst->Write(out_name))
    return 1;

  return 0;
}
