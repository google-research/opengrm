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

// Applies smoothed marginalization constraints to given model.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/ngram/ngram-marginalize.h"

ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(int32_t, max_bo_updates);
ABSL_DECLARE_FLAG(double, norm_eps);
ABSL_DECLARE_FLAG(bool, check_consistency);

int ngrammarginalize_main(int argc, char** argv) {
  std::string usage =
      "Marginalize n-gram model from input model file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdMutableFst> fst(
      fst::StdMutableFst::Read(in_name, /*convert=*/true));
  if (!fst) return 1;

  ngram::NGramMarginal ngramarg(fst.get(), absl::GetFlag(FLAGS_backoff_label),
                                absl::GetFlag(FLAGS_norm_eps),
                                absl::GetFlag(FLAGS_max_bo_updates),
                                absl::GetFlag(FLAGS_check_consistency));

  ngramarg.MarginalizeNGramModel();
  if (ngramarg.Error()) return 1;
  ngramarg.GetFst().Write(out_name);
  return 0;
}
