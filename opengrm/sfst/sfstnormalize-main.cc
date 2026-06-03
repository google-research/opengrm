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

// Gives an FST the weight distribution of a stochastic FST (where possible).

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/sfst/normalize.h"

ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(double, effective_zero);
ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(int64_t, max_iterations);
ABSL_DECLARE_FLAG(std::string, method);

int sfstnormalize_main(int argc, char** argv) {
  std::string usage =
      "Gives an FST the weight distribution of a stochastic FST";
  usage += " where possible\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdMutableFst> fst(
      fst::StdMutableFst::Read(in_name, true));
  if (!fst) return 1;

  bool ret;
  if (absl::GetFlag(FLAGS_method) == "global") {
    ret = sfst::GlobalNormalize(fst.get(), absl::GetFlag(FLAGS_phi_label),
                                absl::GetFlag(FLAGS_delta));
  } else if (absl::GetFlag(FLAGS_method) == "local") {
    if (absl::GetFlag(FLAGS_phi_label) != fst::kNoLabel) {
      LOG(ERROR) << argv[0]
                 << ": no phi label allowed with local normalization";
      return 1;
    }
    ret = sfst::LocalNormalize(fst.get());
  } else if (absl::GetFlag(FLAGS_method) == "kl_min" ||
             absl::GetFlag(FLAGS_method) ==
                 "marginally_constrained" /* deprecated */) {
    ret = sfst::CountNormalize(
        fst.get(), absl::GetFlag(FLAGS_phi_label), sfst::NORM_KL_MIN, false,
        absl::GetFlag(FLAGS_delta), absl::GetFlag(FLAGS_effective_zero),
        absl::GetFlag(FLAGS_max_iterations));
  } else if (absl::GetFlag(FLAGS_method) == "kl_min_approximated" ||
             absl::GetFlag(FLAGS_method) ==
                 "marginally_approximated" /* deprecated */) {
    ret = sfst::CountNormalize(fst.get(), absl::GetFlag(FLAGS_phi_label),
                               sfst::NORM_KL_MIN_APPROXIMATED, false,
                               absl::GetFlag(FLAGS_delta),
                               absl::GetFlag(FLAGS_effective_zero),
                               absl::GetFlag(FLAGS_max_iterations));
  } else if (absl::GetFlag(FLAGS_method) == "phi") {
    ret = sfst::PhiNormalize(fst.get(), absl::GetFlag(FLAGS_phi_label));
  } else if (absl::GetFlag(FLAGS_method) == "summed") {
    ret = sfst::CountNormalize(
        fst.get(), absl::GetFlag(FLAGS_phi_label), sfst::NORM_SUMMED, false,
        absl::GetFlag(FLAGS_delta), absl::GetFlag(FLAGS_effective_zero),
        absl::GetFlag(FLAGS_max_iterations));
  } else {
    LOG(ERROR) << argv[0] << ": unknown normalization method: "
               << absl::GetFlag(FLAGS_method);
    return 1;
  }

  if (!ret) {
    LOG(ERROR) << argv[0] << ": normalization failed";
    return 1;
  }

  if (!fst->Write(out_name)) return 1;

  return 0;
}
