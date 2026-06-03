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

// Prints out various information about a stochastic FST.

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/info.h"

ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(int64_t, phi_label);

int sfstinfo_main(int argc, char** argv) {
  using fst::StdFst;

  std::string usage =
      "Prints out information about a stochastic FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  std::unique_ptr<const StdFst> ifst(StdFst::Read(in_name));
  if (!ifst) return 1;

  sfst::SfstInfo(*ifst, std::cout, absl::GetFlag(FLAGS_phi_label),
                 absl::GetFlag(FLAGS_delta));

  return 0;
}
