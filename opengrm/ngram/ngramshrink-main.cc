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

// Shrinks an input n-gram model using given pruning criteria.

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/ngram/ngram-list-prune.h"
#include "opengrm/ngram/ngram-shrink.h"

ABSL_DECLARE_FLAG(double, total_unigram_count);
ABSL_DECLARE_FLAG(double, theta);
ABSL_DECLARE_FLAG(int64_t, target_number_of_ngrams);
ABSL_DECLARE_FLAG(int32_t, min_order_to_prune);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(std::string, list_file);
ABSL_DECLARE_FLAG(std::string, count_pattern);
ABSL_DECLARE_FLAG(std::string, context_pattern);
ABSL_DECLARE_FLAG(int32_t, shrink_opt);
ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(double, norm_eps);
ABSL_DECLARE_FLAG(bool, check_consistency);
ABSL_DECLARE_FLAG(bool, retry_downcase);

int ngramshrink_main(int argc, char** argv) {
  std::string usage = "Shrink n-gram model from input model file.\n\n  Usage: ";
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

  std::set<std::vector<fst::StdArc::Label>> ngram_list;
  if (absl::GetFlag(FLAGS_method) == "list_prune") {
    if (absl::GetFlag(FLAGS_list_file).empty()) {
      LOG(WARNING) << "list_file parameter empty, no n-grams given";
      return 1;
    }
    std::ifstream ifstrm(absl::GetFlag(FLAGS_list_file));
    if (!ifstrm) {
      LOG(WARNING) << "NGramShrink: Can't open "
                   << absl::GetFlag(FLAGS_list_file) << " for reading";
      return 1;
    }
    std::string line;
    std::vector<std::string> ngrams_to_prune;
    while (std::getline(ifstrm, line)) {
      ngrams_to_prune.push_back(line);
    }
    ifstrm.close();
    ngram::GetNGramListToPrune(ngrams_to_prune, fst->InputSymbols(),
                               &ngram_list,
                               absl::GetFlag(FLAGS_retry_downcase));
  }
  if (!ngram::NGramShrinkModel(
          fst.get(), absl::GetFlag(FLAGS_method), ngram_list,
          absl::GetFlag(FLAGS_total_unigram_count), absl::GetFlag(FLAGS_theta),
          absl::GetFlag(FLAGS_target_number_of_ngrams),
          absl::GetFlag(FLAGS_min_order_to_prune),
          absl::GetFlag(FLAGS_count_pattern),
          absl::GetFlag(FLAGS_context_pattern), absl::GetFlag(FLAGS_shrink_opt),
          absl::GetFlag(FLAGS_backoff_label), absl::GetFlag(FLAGS_norm_eps),
          absl::GetFlag(FLAGS_check_consistency)))
    return 1;

  fst->Write(out_name);

  return 0;
}
