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

// Algorithm to approximate a stochastic FST as an n-gram model.
// The output is a canonical and normalized OpenGrm ngram model.

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
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/approx.h"
#include "opengrm/sfst/ngramapprox.h"
#include "opengrm/sfst/normalize.h"

ABSL_DECLARE_FLAG(int64_t, order);
ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(std::string, norm_type);

int sfstngramapprox_main(int argc, char** argv) {
  std::string usage = "Algorithm to approximate a stochastic FST";
  usage += " as an n-gram model.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  sfst::CountNormType norm_type;
  if (absl::GetFlag(FLAGS_norm_type) == "summed") {
    norm_type = sfst::NORM_SUMMED;
  } else if (absl::GetFlag(FLAGS_norm_type) == "kl_min" ||
             absl::GetFlag(FLAGS_norm_type) ==
                 "marginally_constrained" /* deprecated */) {
    norm_type = sfst::NORM_KL_MIN;
  } else if (absl::GetFlag(FLAGS_norm_type) == "kl_min_approximated" ||
             absl::GetFlag(FLAGS_norm_type) == "marginally_approximated") {
    norm_type = sfst::NORM_KL_MIN_APPROXIMATED;
  } else {
    LOG(ERROR) << argv[0]
               << ": Bad norm type: " << absl::GetFlag(FLAGS_norm_type);
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdFst> ifst(fst::StdFst::Read(in_name));
  if (!ifst) return 1;

  if (ifst->Properties(fst::kCyclic, true) &&
      !sfst::IsNormalized(*ifst, absl::GetFlag(FLAGS_phi_label))) {
    LOG(ERROR) << argv[0] << ": Input is not a normalized stochastic FST";
    return 1;
  }

  fst::StdVectorFst ofst;
  if (!sfst::NGramApprox(*ifst, &ofst, absl::GetFlag(FLAGS_order),
                         absl::GetFlag(FLAGS_phi_label),
                         absl::GetFlag(FLAGS_delta), norm_type))
    return 1;

  if (!ofst.Write(out_name)) return 1;

  return 0;
}
