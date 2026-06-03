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

// Sorts an ngram LM in lexicographic state context order.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/ngram/ngram-mutable-model.h"

ABSL_DECLARE_FLAG(bool, check_consistency);
ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(double, norm_eps);

int ngramsort_main(int argc, char** argv) {
  std::string usage =
      "Sorts an ngram LM in lexicographic state context order.\n\n  Usage: ";
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
      fst::StdMutableFst::Read(in_name, true));
  if (!fst) return 1;

  ngram::NGramMutableModel<fst::StdArc> ngramlm(
      fst.get(), absl::GetFlag(FLAGS_backoff_label),
      absl::GetFlag(FLAGS_norm_eps),
      /* state_ngrams= */ true, /* infinite_backoff= */ false);
  ngramlm.SortStates();
  ngramlm.InitModel();
  ngramlm.GetFst().Write(out_name);

  return 0;
}
