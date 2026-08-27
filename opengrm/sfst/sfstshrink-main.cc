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

// Binary to shrink SFST models.

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "openfst/compat/init.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/sfst/shrink.h"

ABSL_DECLARE_FLAG(double, theta);
ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(double, total_unigram_count);
ABSL_DECLARE_FLAG(std::string, count_pattern);
ABSL_DECLARE_FLAG(std::string, list_file);
ABSL_DECLARE_FLAG(std::string, count_fst);
ABSL_DECLARE_FLAG(std::string, word_set);

int sfstshrink_main(int argc, char** argv) {
  std::string usage = "Shrink SFST models.\n\n";
  usage += "  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    std::cerr << usage << "\n";
    return 1;
  }

  const std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  const std::string out_name = (argc > 2) ? argv[2] : "";

  std::unique_ptr<fst::MutableFst<fst::StdArc>> fst(
      fst::MutableFst<fst::StdArc>::Read(in_name, true));
  if (!fst) {
    LOG(ERROR) << argv[0] << ": Read failed: " << in_name;
    return 1;
  }

  const double theta = absl::GetFlag(FLAGS_theta);
  const int64_t phi_label = absl::GetFlag(FLAGS_phi_label);
  const double total_unigram_count = absl::GetFlag(FLAGS_total_unigram_count);
  const std::string method = absl::GetFlag(FLAGS_method);
  bool success = false;

  if (method == "stolcke" || method == "relative_entropy") {
    success = sfst::StolckeShrink(fst.get(), phi_label, theta);
  } else if (method == "restricted_stolcke" ||
             method == "restricted_relative_entropy") {
    success = sfst::RestrictedRelEntropyShrink(fst.get(), phi_label, theta);
  } else if (method == "symmetrized_relative_entropy") {
    success = sfst::SymmetrizedRelEntropyShrink(fst.get(), phi_label, theta);
  } else if (method == "seymore") {
    success =
        sfst::SeymoreShrink(fst.get(), phi_label, theta, total_unigram_count);
  } else if (method == "absolute_seymore") {
    success = sfst::AbsoluteSeymoreShrink(fst.get(), phi_label, theta,
                                          total_unigram_count);
  } else if (method == "count_prune") {
    success = sfst::CountPrune(fst.get(), phi_label,
                               absl::GetFlag(FLAGS_count_pattern));
  } else if (method == "significance") {
    const std::string count_fst_name = absl::GetFlag(FLAGS_count_fst);
    std::unique_ptr<fst::ExpandedFst<fst::StdArc>> count_fst;
    if (!count_fst_name.empty()) {
      count_fst.reset(fst::ExpandedFst<fst::StdArc>::Read(count_fst_name));
      if (!count_fst) {
        LOG(ERROR) << "Failed to read count FST: " << count_fst_name;
        return 1;
      }
    }
    success = sfst::SignificanceShrink(fst.get(), phi_label, count_fst.get(),
                                       total_unigram_count);
  } else if (method == "word_shrink") {
    absl::flat_hash_set<fst::StdArc::Label> words;
    sfst::ReadWordSet(absl::GetFlag(FLAGS_word_set), fst->InputSymbols(),
                      &words);
    success = sfst::WordShrink(fst.get(), phi_label, words);
  } else if (method == "list_prune") {
    const std::string list_file = absl::GetFlag(FLAGS_list_file);
    if (list_file.empty()) {
      LOG(ERROR) << "list_file required for list_prune";
      return 1;
    }
    std::set<std::vector<fst::StdArc::Label>> ngram_list;
    sfst::ReadNGramList(list_file, fst->InputSymbols(), &ngram_list);
    success = sfst::ListPrune(fst.get(), phi_label, ngram_list);
  } else {
    LOG(ERROR) << "Unknown method: " << method;
    return 1;
  }

  if (!success) {
    LOG(ERROR) << "Shrinking failed";
    return 1;
  }

  if (!fst->Write(out_name)) {
    LOG(ERROR) << argv[0] << ": write of FST failed: " << out_name;
    return 1;
  }

  return 0;
}
