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
#include "openfst/lib/properties.h"
#include "opengrm/sfst/trim.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(bool, phi_trim);
ABSL_DECLARE_FLAG(bool, weight_trim);
ABSL_DECLARE_FLAG(bool, sum_weight_trim);
ABSL_DECLARE_FLAG(bool, include_phi);
ABSL_DECLARE_FLAG(bool, connect);
ABSL_DECLARE_FLAG(std::string, trim_type);
ABSL_DECLARE_FLAG(double, weight);

int sfsttrim_main(int argc, char** argv) {
  std::string usage = "Removes useless states and transitions in stochastic ";
  usage += " automata.\n\n  Usage: ";
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

  sfst::TrimType trim_type;
  if (absl::GetFlag(FLAGS_trim_type) == "needed_trim") {
    trim_type = sfst::TRIM_NEEDED_TRIM;
  } else if (absl::GetFlag(FLAGS_trim_type) == "needed_final") {
    trim_type = sfst::TRIM_NEEDED_FINAL;
  } else if (absl::GetFlag(FLAGS_trim_type) == "needed_nonfinal") {
    trim_type = sfst::TRIM_NEEDED_NONFINAL;
  } else {
    LOG(ERROR) << argv[0]
               << ": Bad trim type: " << absl::GetFlag(FLAGS_trim_type);
    return 1;
  }

  sfst::Trimmer<fst::StdArc> trim(fst.get(), absl::GetFlag(FLAGS_phi_label),
                                  trim_type);

  if (absl::GetFlag(FLAGS_phi_trim)) trim.PhiTrim();

  if (absl::GetFlag(FLAGS_weight_trim))
    trim.WeightTrim(absl::GetFlag(FLAGS_include_phi),
                    typename fst::StdArc::Weight(absl::GetFlag(FLAGS_weight)));

  if (absl::GetFlag(FLAGS_sum_weight_trim))
    trim.SumWeightTrim(
        absl::GetFlag(FLAGS_include_phi),
        typename fst::StdArc::Weight(absl::GetFlag(FLAGS_weight)));

  if (absl::GetFlag(FLAGS_connect)) trim.Connect();

  trim.Finalize();

  if (fst->Properties(fst::kError, false)) {
    LOG(ERROR) << argv[0] << ": trimming failed";
    return 2;
  }

  if (!fst->Write(out_name)) return 1;

  return 0;
}
