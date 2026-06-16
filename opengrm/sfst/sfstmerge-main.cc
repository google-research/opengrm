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

#include <iostream>
#include <memory>  // NOLINT(misc-include-cleaner)
#include <string>

#include "openfst/compat/init.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/fst.h"         // NOLINT(misc-include-cleaner)
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/merge.h"

ABSL_FLAG(std::string, method, "linear", "Merging method: linear, bayes");
ABSL_FLAG(double, alpha, 0.5, "Mixing weight for the first model");
ABSL_FLAG(double, beta, 0.5, "Mixing weight for the second model");

int sfstmerge_main(int argc, char** argv) {
  std::string usage = "Merge two stochastic FSTs.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in1.fst in2.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 3 || argc > 4) {
    std::cerr << usage << "\n";
    return 1;
  }

  std::unique_ptr<fst::StdFst> fst1(fst::StdFst::Read(argv[1]));
  if (!fst1) {
    LOG(ERROR) << argv[0] << ": Read failed: " << argv[1];
    return 1;
  }

  std::unique_ptr<fst::StdFst> fst2(fst::StdFst::Read(argv[2]));
  if (!fst2) {
    LOG(ERROR) << argv[0] << ": Read failed: " << argv[2];
    return 1;
  }

  std::string out_name = argc > 3 ? argv[3] : "";
  fst::StdVectorFst out_fst;

  bool ret = false;
  if (absl::GetFlag(FLAGS_method) == "linear") {
    ret = sfst::LinearMerge(*fst1, *fst2, absl::GetFlag(FLAGS_alpha),
                            absl::GetFlag(FLAGS_beta), &out_fst);
  } else if (absl::GetFlag(FLAGS_method) == "bayes") {
    ret = sfst::BayesMerge(*fst1, *fst2, absl::GetFlag(FLAGS_alpha),
                           absl::GetFlag(FLAGS_beta), &out_fst);
  } else {
    LOG(ERROR) << argv[0]
               << ": unknown merging method: " << absl::GetFlag(FLAGS_method);
    return 1;
  }

  if (!ret) {
    LOG(ERROR) << argv[0] << ": merging failed";
    return 1;
  }

  if (!out_fst.Write(out_name)) {
    LOG(ERROR) << argv[0] << ": failed to write output FST";
    return 1;
  }

  return 0;
}
