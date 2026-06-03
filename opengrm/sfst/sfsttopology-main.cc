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

// Algorithms for constructing specific FST topologies.

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
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/topology.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(int64_t, order);

int sfsttopology_main(int argc, char** argv) {
  std::string usage =
      "Algorithms for constructing specific FST topologies.\n\n";
  usage += "  Usage: ";
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

  std::unique_ptr<fst::StdFst> ifst(fst::StdFst::Read(in_name));
  if (!ifst) return 1;

  fst::StdVectorFst ofst;

  if (absl::GetFlag(FLAGS_method) == "ngram") {
    sfst::NGramTopology<fst::StdArc> ngram(
        absl::GetFlag(FLAGS_order), absl::GetFlag(FLAGS_phi_label), &ofst);
    ngram.FindNGrams(*ifst);
  } else {
    LOG(ERROR) << argv[0]
               << ": unknown topology method: " << absl::GetFlag(FLAGS_method);
    return 1;
  }

  if (!ofst.Write(out_name)) return 1;

  return 0;
}
