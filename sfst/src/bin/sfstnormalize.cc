
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
// sfstnormalize.cc

// Gives an FST the weight distribution of a stochastic FST
// (where possible).

#include <string.h>

#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/fst-decl.h>
#include <fst/mutable-fst.h>
#include <sfst/normalize.h>

constexpr double kNormDelta = 1.0e-5;
constexpr double kEffectiveZero = 35.0;
constexpr size_t kMaxNormIters = 1000;

DEFINE_double(delta, kNormDelta, "Convergence delta");
DEFINE_double(effective_zero, kEffectiveZero, "Effective zero (-log)");
DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_int64(max_iterations, kMaxNormIters,
             "Specifies maximum number of iterations for convergence");
DEFINE_string(method, "global",
              "Specifies normalization method, one of: "
              "\"global\", \"local\", \"kl_min (counts)\","
              "\"kl_min_approximated (counts)\", "
              "\"phi\", or \"summed (counts)\"");

int main(int argc, char **argv) {
  namespace f = fst;
  std::string usage =
      "Gives an FST the weight distribution of a stochastic FST";
  usage += " where possible\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  f::StdMutableFst *fst = f::StdMutableFst::Read(in_name, true);
  if (!fst) return 1;

  bool ret;
  if (FLAGS_method == "global") {
    ret = sfst::GlobalNormalize(fst, FLAGS_phi_label, FLAGS_delta);
  } else if (FLAGS_method == "local") {
    if (FLAGS_phi_label != fst::kNoLabel) {
      LOG(ERROR) << argv[0]
                 << ": no phi label allowed with local normalization";
      return 1;
    }
    ret = sfst::LocalNormalize(fst);
  } else if (FLAGS_method == "kl_min" ||
             FLAGS_method == "marginally_constrained" /* deprecated */) {
    ret = sfst::CountNormalize(fst, FLAGS_phi_label, sfst::NORM_KL_MIN,
                               false, FLAGS_delta, FLAGS_effective_zero,
                               FLAGS_max_iterations);
  } else if (FLAGS_method == "kl_min_approximated" ||
             FLAGS_method == "marginally_approximated" /* deprecated */) {
    ret = sfst::CountNormalize(fst, FLAGS_phi_label, sfst::NORM_KL_MIN,
                               false, FLAGS_delta, FLAGS_effective_zero,
                               FLAGS_max_iterations);
  } else if (FLAGS_method == "phi") {
    ret = sfst::PhiNormalize(fst, FLAGS_phi_label);
  } else if (FLAGS_method == "summed") {
    ret = sfst::CountNormalize(fst, FLAGS_phi_label, sfst::NORM_SUMMED,
                               false, FLAGS_delta, FLAGS_effective_zero,
                               FLAGS_max_iterations);
  } else {
    LOG(ERROR) << argv[0] << ": unknown normalization method: "
               << FLAGS_method;
    return 1;
  }

  if (!ret) {
    LOG(ERROR) << argv[0] << ": normalization failed";
    return 1;
  }

  if (!fst->Write(out_name))
    return 1;

  return 0;
}
