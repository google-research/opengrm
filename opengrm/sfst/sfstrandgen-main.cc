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

// Generates random paths through a stochastic FST. The FST must be a
// normalized.

#include <time.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/randgen.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(int32_t, max_length);
ABSL_DECLARE_FLAG(int64_t, npath);
ABSL_DECLARE_FLAG(std::optional<uint64_t>, seed);
ABSL_DECLARE_FLAG(bool, weighted);
ABSL_DECLARE_FLAG(bool, remove_total_weight);
ABSL_DECLARE_FLAG(bool, minimal);
ABSL_DECLARE_FLAG(bool, stochastic);

int sfstrandgen_main(int argc, char** argv) {
  std::string usage = "Generates random paths through an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (absl::GetFlag(FLAGS_stochastic)) {
    absl::SetFlag(&FLAGS_weighted, true);
    absl::SetFlag(&FLAGS_remove_total_weight, true);
    absl::SetFlag(&FLAGS_minimal, true);
  }

  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::optional<uint64_t> seed = absl::GetFlag(FLAGS_seed);
  VLOG(1) << argv[0] << ": Seed = "
          << (seed.has_value() ? absl::StrCat(*seed) : "nullopt");

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdFst> ifst(fst::StdFst::Read(in_name));
  if (!ifst) return 1;

  if (!sfst::IsNormalized(*ifst, absl::GetFlag(FLAGS_phi_label))) {
    LOG(ERROR) << argv[0] << ": Input is not a normalized stochastic FST";
    return 1;
  }

  fst::StdVectorFst ofst;

  sfst::SFstArcSelector<fst::StdArc> selector(absl::GetFlag(FLAGS_phi_label));
  fst::RandGenOptions<sfst::SFstArcSelector<fst::StdArc>> opts(
      selector, absl::GetFlag(FLAGS_max_length), absl::GetFlag(FLAGS_npath),
      absl::GetFlag(FLAGS_weighted), absl::GetFlag(FLAGS_remove_total_weight));
  fst::RandGen(*ifst, &ofst, opts, seed);
  if (absl::GetFlag(FLAGS_weighted) && absl::GetFlag(FLAGS_minimal))
    fst::RandMinimize(&ofst, absl::GetFlag(FLAGS_phi_label));
  if (!ofst.Write(out_name)) return 1;

  return 0;
}
