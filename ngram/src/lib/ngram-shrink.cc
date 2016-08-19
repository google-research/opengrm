
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
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
#include <ngram/ngram-context-prune.h>
#include <ngram/ngram-count-prune.h>
#include <ngram/ngram-relentropy.h>
#include <ngram/ngram-seymore-shrink.h>
#include <ngram/ngram-shrink.h>

namespace ngram {

// Makes model from NGram model FST with StdArc counts.
bool NGramShrinkModel(fst::StdMutableFst *fst, const string &method,
                      double tot_uni, double theta, int64 target_num,
                      const string &count_pattern,
                      const string &context_pattern, int shrink_opt,
                      fst::StdArc::Label backoff_label,
                      double norm_eps, bool check_consistency) {
  bool full_context = context_pattern.empty();
  if (method == "context_prune") {
    if (target_num >= 0) {
      LOG(ERROR) << "Target number of ngrams requires \"relative_entropy\" or "
                 << "\"seymore\" shrinking";
    }
    NGramContextPrune ngramsh(fst, context_pattern, shrink_opt, tot_uni,
                              backoff_label, norm_eps, check_consistency);
    ngramsh.ShrinkNGramModel();
    return !ngramsh.Error();
  } else if (method == "count_prune") {
    if (target_num >= 0) {
      LOG(ERROR) << "Target number of ngrams requires \"relative_entropy\" or "
                 << "\"seymore\" shrinking";
    }
    if (full_context) {
      NGramCountPrune ngramsh(fst, count_pattern, shrink_opt, tot_uni,
                              backoff_label, norm_eps, check_consistency);
      ngramsh.ShrinkNGramModel();
      return !ngramsh.Error();
    } else {
      NGramContextCountPrune ngramsh(fst, count_pattern, context_pattern,
                                     shrink_opt, tot_uni, backoff_label,
                                     norm_eps, check_consistency);
      ngramsh.ShrinkNGramModel();
      return !ngramsh.Error();
    }
  } else if (method == "relative_entropy") {
    if (full_context) {
      NGramRelEntropy ngramsh(fst, theta, shrink_opt, tot_uni, backoff_label,
                              norm_eps, check_consistency);
      if (target_num >= 0) {
        ngramsh.CalculateTheta(target_num);
        if (ngramsh.Error()) return false;
      }
      ngramsh.ShrinkNGramModel();
      return !ngramsh.Error();
    } else {
      if (target_num >= 0)
        LOG(ERROR) << "Target number of ngrams requires a full context";
      NGramContextRelEntropy ngramsh(fst, theta, context_pattern, shrink_opt,
                                     tot_uni, backoff_label, norm_eps,
                                     check_consistency);
      ngramsh.ShrinkNGramModel();
      return !ngramsh.Error();
    }
  } else if (method == "seymore") {
    if (full_context) {
      NGramSeymoreShrink ngramsh(fst, theta, shrink_opt, tot_uni,
                                 backoff_label, norm_eps, check_consistency);
      if (target_num >= 0) {
        ngramsh.CalculateTheta(target_num);
        if (ngramsh.Error()) return false;
      }
      ngramsh.ShrinkNGramModel();
      return !ngramsh.Error();
    } else {
      if (target_num >= 0)
        LOG(ERROR) << "Target number of ngrams requires a full context";
      NGramContextSeymoreShrink ngramsh(fst, theta, context_pattern,
                                        shrink_opt, tot_uni, backoff_label,
                                        norm_eps, check_consistency);
      ngramsh.ShrinkNGramModel();
      return !ngramsh.Error();
    }
  }
  LOG(ERROR) << "Unknown shrink method: " << method;
  return false;
}

}  // namespace ngram
