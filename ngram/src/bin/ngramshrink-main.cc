
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Shrinks an input n-gram model using given pruning criteria.

#include <fstream>
#include <memory>
#include <string>

#include <ngram/ngram-list-prune.h>
#include <ngram/ngram-shrink.h>

DECLARE_double(total_unigram_count);
DECLARE_double(theta);
DECLARE_int64(target_number_of_ngrams);
DECLARE_int32(min_order_to_prune);
DECLARE_string(method);
DECLARE_string(list_file);
DECLARE_string(count_pattern);
DECLARE_string(context_pattern);
DECLARE_int32(shrink_opt);
DECLARE_int64(backoff_label);
DECLARE_double(norm_eps);
DECLARE_bool(check_consistency);

int ngramshrink_main(int argc, char **argv) {
  std::string usage = "Shrink n-gram model from input model file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdMutableFst> fst(
      fst::StdMutableFst::Read(in_name, true));
  if (!fst) return 1;

  std::set<std::vector<fst::StdArc::Label>> ngram_list;
  if (FLAGS_method == "list_prune") {
    if (FLAGS_list_file.empty()) {
      LOG(WARNING) << "list_file parameter empty, no n-grams given";
      return 1;
    }
    std::ifstream ifstrm(FLAGS_list_file);
    if (!ifstrm) {
      LOG(WARNING) << "NGramShrink: Can't open " << FLAGS_list_file
                   << " for reading";
      return 1;
    }
    std::string line;
    std::vector<std::string> ngrams_to_prune;
    while (std::getline(ifstrm, line)) {
      ngrams_to_prune.push_back(line);
    }
    ifstrm.close();
    ngram::GetNGramListToPrune(ngrams_to_prune, fst->InputSymbols(),
                               &ngram_list);
  }
  if (!ngram::NGramShrinkModel(
          fst.get(), FLAGS_method, ngram_list, FLAGS_total_unigram_count,
          FLAGS_theta, FLAGS_target_number_of_ngrams, FLAGS_min_order_to_prune,
          FLAGS_count_pattern, FLAGS_context_pattern, FLAGS_shrink_opt,
          FLAGS_backoff_label, FLAGS_norm_eps, FLAGS_check_consistency))
    return 1;

  fst->Write(out_name);

  return 0;
}
