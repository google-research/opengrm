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

// Algorithm to count from a stochastic FST w.r.t. a backoff-complete FST whose
// topology is provided.  Result is FST with backoff-complete topology weighted
// by expected counts derived from the input stochastic FST.

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
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "opengrm/sfst/approx.h"
#include "opengrm/sfst/count.h"
#include "opengrm/sfst/normalize.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(double, delta);

int sfstcount_main(int argc, char** argv) {
  std::string usage = "Algorithm to count from stochastic FST w.r.t. a";
  usage += " backoff-complete FST whose topology is provided.\n\n  Usage: ";
  usage += argv[0];
  usage += " sfst.fst top.fst [out.fst]\n";

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

  std::unique_ptr<fst::StdFst> ifst(fst::StdFst::Read(in1_name));
  if (!ifst) return 1;

  std::unique_ptr<fst::StdMutableFst> ofst(
      fst::StdMutableFst::Read(in2_name, true));
  if (!ofst) return 1;

  if (ifst->Properties(fst::kCyclic, true) &&
      !sfst::IsNormalized(*ifst, absl::GetFlag(FLAGS_phi_label))) {
    LOG(ERROR) << argv[0] << ": First input is not a normalized stochastic FST";
    return 1;
  }

  sfst::Counter<fst::StdArc> counter(absl::GetFlag(FLAGS_phi_label),
                                     absl::GetFlag(FLAGS_delta), ofst.get());
  counter.Count(*ifst);
  counter.Finalize();

  if (ofst->Properties(fst::kError, false)) return 1;

  if (!ofst->Write(out_name)) return 1;

  return 0;
}
