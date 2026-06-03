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

// Intersects two canonical stochastic FSAs.
// The second FSA must be input-epsilon free (when phi_label != 0).

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
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/intersect.h"
#include "opengrm/sfst/trim.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(bool, trim);
ABSL_DECLARE_FLAG(std::string, trim_type);

int sfstintersect_main(int argc, char** argv) {
  std::string usage = "Intersects two canonical stochastic FSAs.\n\n Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 4) {
    LOG(INFO) << absl::ProgramUsageMessage();
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

  std::unique_ptr<fst::StdFst> ifst1(fst::StdFst::Read(in1_name));
  if (!ifst1) return 1;

  std::unique_ptr<fst::StdFst> ifst2(fst::StdFst::Read(in2_name));
  if (!ifst2) return 1;

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

  fst::StdVectorFst ofst;
  if (!sfst::Intersect(*ifst1, *ifst2, &ofst, absl::GetFlag(FLAGS_phi_label),
                       absl::GetFlag(FLAGS_trim), trim_type))
    return 1;

  if (!ofst.Write(out_name)) return 1;

  return 0;
}
