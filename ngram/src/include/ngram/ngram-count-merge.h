// ngram-count-merge.h
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
//
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// NGram model class for merging count FSTs

#ifndef NGRAM_NGRAM_COUNT_MERGE_H__
#define NGRAM_NGRAM_COUNT_MERGE_H__

#include <ngram/ngram-merge.h>

namespace ngram {

class NGramCountMerge : public NGramMerge {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;

  // Constructs an NGramCountMerge object consisting of ngram model
  // to be merged.
  // Ownership of FST is retained by the caller.
  NGramCountMerge(StdMutableFst *infst1, Label backoff_label = 0,
                  double norm_eps = kNormEps, bool check_consistency = false)
      : NGramMerge(infst1, backoff_label, norm_eps, check_consistency) {
  }

  // Perform count-model merger with n-gram model specified by the FST argument
  // and mixing weights alpha and beta.
  void MergeNGramModels(const StdFst &infst2, double alpha, double beta,
                        bool norm = false) {
    alpha_ = -log(alpha);
    beta_ = -log(beta);
    NGramMerge::MergeNGramModels(infst2, norm);
  }

 protected:
  // Specifies resultant weight when combining a weight from each FST.
  virtual double MergeWeights(StateId s1, StateId s2, Label Label,
			      double w1, double w2,
                              bool in_fst1, bool in_fst2) const {
    if (in_fst1 && in_fst2) {
      return NegLogSum(w1 + alpha_, w2 + beta_);
    } else if (in_fst1) {
      return w1 + alpha_;
    } else {
      return w2 + beta_;
    }
  }

  // Specifies the normalization constant per state depending whether
  // state was present in one or boths FSTs.
  virtual double NormWeight(bool in_fst1, bool in_fst2) const {
    if (in_fst1 && in_fst2) {
      return -NegLogSum(alpha_, beta_);
    } else if (in_fst1) {
      return -alpha_;
    } else {
      return -beta_;
    }
  }

  // Specifies if unshared arcs/final weights between the two
  // FSTs in a merge have a non-trivial merge. In particular, this
  // means MergeWeights() changes the arc or final weights; any
  // destination state changes are not relevant here. When false, more
  // efficient merging may be performed. If the arc/final_weight
  // comes from the first FST, then 'in_fst1' is true.
  virtual bool MergeUnshared(bool in_fst1) const {
    return (in_fst1) ? (alpha_ != 0.0) : (beta_ != 0.0);
  }

 private:
  double alpha_;  // weight to scale model ngram1
  double beta_;   // weight to scale model ngram2
  DISALLOW_COPY_AND_ASSIGN(NGramCountMerge);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_COUNT_MERGE_H__
