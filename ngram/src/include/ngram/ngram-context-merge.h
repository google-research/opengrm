// ngram-context-merge.h
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
// NGram model class for merging specified contexts from one
// FST into another.

#ifndef NGRAM_NGRAM_CONTEXT_MERGE_H__
#define NGRAM_NGRAM_CONTEXT_MERGE_H__

#include <ngram/ngram-context.h>
#include <ngram/ngram-merge.h>

namespace ngram {

class NGramContextMerge : public NGramMerge {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;

  // Constructs an NGramContextMerge object consisting of ngram model to be merged.
  // Ownership of FST is retained by the caller.
  NGramContextMerge(StdMutableFst *infst1, Label backoff_label = 0,
                    double norm_eps = kNormEps, bool check_consistency = false)
      : NGramMerge(infst1, backoff_label, norm_eps, true),
        context_(0) { }

  ~NGramContextMerge() { delete context_; }


  // Perform context merger with n-gram model specified by the FST
  // argument and 'context_pattern' string. These contexts taken from
  // the second FST and added to the first FST, replacing any existing
  // shared arcs.  See 'ngram-context.h' for meaning of the context
  // specification.
  void MergeNGramModels(const StdFst &infst2, string context_pattern,
                        bool norm = false) {
    if (context_) delete context_;
    context_ = new NGramContext(context_pattern, HiOrder());
    NGramMerge::MergeNGramModels(infst2, norm);
  }

  // Perform context merger with n-gram model specified by the FST argument
  // and 'context_begin' and 'context_end' vectors. These contexts
  // taken from the second FST and added to the first FST, replacing any
  // existing shared arcs.  See 'ngram-context.h' for meaning of the
  // context specification.
  void MergeNGramModels(const StdFst &infst2,
                        const std::vector<Label> context_begin,
                        const std::vector<Label> context_end,
                        bool norm = false) {
    if (context_) delete context_;
    context_ = new NGramContext(context_begin, context_end, HiOrder());
    NGramMerge::MergeNGramModels(infst2, norm);
  }

 protected:
  // Specifies resultant weight when combining a weight from each FST
  virtual double MergeWeights(StateId s1, StateId s2, Label label,
			      double w1, double w2,
                              bool in_fst1, bool in_fst2) const {
    if (in_fst1 && in_fst2) {
      const std::vector<Label> &ngram = NGram2().StateNGram(s2);
      return context_->HasContext(ngram) ? w2 : w1;
    } else if (in_fst1) {
      return w1;
    } else {
      return w2;
    }
  }

  // Specifies if unshared arcs/final weights between the two
  // FSTs in a merge have a non-trivial merge. In particular, this
  // means MergeWeights() changes the arc or final weights; any
  // destination state changes are not relevant here. When false, more
  // efficient merging may be performed. If the arc/final_weight
  // comes from the first FST, then 'in_fst1' is true.
  virtual bool MergeUnshared(bool in_fst1) const { return false; }

 private:
  NGramContext *context_;
  DISALLOW_COPY_AND_ASSIGN(NGramContextMerge);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_CONTEXT_MERGE_H__
