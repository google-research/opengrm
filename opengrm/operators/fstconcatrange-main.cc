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

// Creates the generalized closure of an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/script/fst-class.h"
#include "opengrm/operators/concatrangescript.h"

ABSL_DECLARE_FLAG(int32_t, lower);
ABSL_DECLARE_FLAG(int32_t, upper);

int fstconcatrange_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::script::MutableFstClass;

  std::string usage = "Creates the generalized closure of an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name = argc > 2 ? argv[2] : "";

  const std::unique_ptr<MutableFstClass> fst(
      MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  s::ConcatRange(fst.get(), absl::GetFlag(FLAGS_lower),
                 absl::GetFlag(FLAGS_upper));

  return !fst->Write(out_name);
}
