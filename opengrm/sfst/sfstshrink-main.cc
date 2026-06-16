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

#include <cstdint>
#include <iostream>
#include <memory>  // NOLINT(misc-include-cleaner)
#include <set>     // NOLINT(misc-include-cleaner)
#include <string>
#include <vector>  // NOLINT(misc-include-cleaner)

#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/mutable-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/shrink.h"

ABSL_DECLARE_FLAG(double, theta);
ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(double, total_unigram_count);
ABSL_DECLARE_FLAG(std::string, count_pattern);
ABSL_DECLARE_FLAG(std::string, list_file);

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

  std::string in_name = (argc > 1) ? argv[1] : "";
  std::string out_name = (argc > 2) ? argv[2] : "";
  std::unique_ptr<fst::MutableFst<fst::StdArc>> fst(
      fst::MutableFst<fst::StdArc>::Read(in_name, true));
  if (!fst) {
    LOG(ERROR) << argv[0] << ": Read failed: " << in_name;
    return 1;
  }

  double theta = absl::GetFlag(FLAGS_theta);
  int64_t phi_label = absl::GetFlag(FLAGS_phi_label);
  std::string method = absl::GetFlag(FLAGS_method);

  if (method == "stolcke") {
    if (!sfst::StolckeShrink(fst.get(), phi_label, theta)) {
      LOG(ERROR) << "Stolcke shrinking failed";
      return 1;
    }
  } else if (method == "seymore") {
    double total_unigram_count = absl::GetFlag(FLAGS_total_unigram_count);
    if (!sfst::SeymoreShrink(fst.get(), phi_label, theta,
                             total_unigram_count)) {
      LOG(ERROR) << "Seymore shrinking failed";
      return 1;
    }
  } else if (method == "count_prune") {
    std::string count_pattern = absl::GetFlag(FLAGS_count_pattern);
    if (!sfst::CountPrune(fst.get(), phi_label, count_pattern)) {
      LOG(ERROR) << "Count pruning failed";
      return 1;
    }
  } else if (method == "list_prune") {
    std::string list_file = absl::GetFlag(FLAGS_list_file);
    if (list_file.empty()) {
      LOG(ERROR) << "list_file required for list_prune";
      return 1;
    }
    std::set<std::vector<fst::StdArc::Label>> ngram_list;
    sfst::ReadNGramList(list_file, fst->InputSymbols(), &ngram_list);
    if (!sfst::ListPrune(fst.get(), phi_label, ngram_list)) {
      LOG(ERROR) << "List pruning failed";
      return 1;
    }
  } else {
    LOG(ERROR) << "Unknown method: " << method;
    return 1;
  }

  if (!fst->Write(out_name)) {
    LOG(ERROR) << argv[0] << ": write of FST failed: " << out_name;
    return 1;
  }

  return 0;
}
