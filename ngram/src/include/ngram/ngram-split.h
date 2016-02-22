// ngram-split.h
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
// Class for splitting an NGram model into multiple parts by context.

#ifndef NGRAM_NGRAM_SPLIT_H__
#define NGRAM_NGRAM_SPLIT_H__

#include <map>
#include <set>
#include <string>
#include <vector>

#include <ngram/ngram-context.h>
#include <ngram/ngram-model.h>

namespace ngram {

using std::map;
using std::set;
using std::vector;

using fst::StdArc;
using fst::StdFst;
using fst::StdMutableFst;
using fst::StdVectorFst;

// Splits NGram model into multiple parts by context.
// Assumes outer context encompasses input.
class NGramSplit {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  // Split based on context patterns (sse ngram-context.h for more
  // information).
  NGramSplit(const StdFst &fst, const vector<string> &context_patterns,
             Label backoff_label = 0, double norm_eps = kNormEps);

  // Split based on context begin and end vectors (sse ngram-context.h
  // for more information).
  NGramSplit(const StdFst &infst, const vector< vector<Label> > &contexts,
             Label backoff_label = 0, double norm_eps = kNormEps);

  ~NGramSplit() {
    for (int i = 0; i < contexts_.size(); ++i) delete contexts_[i];
    for (int i = 0; i < fsts_.size(); ++i) delete fsts_[i];
  }

  // Return next NGram component model.
  void NextNGramModel(StdMutableFst *outfst) {
    *outfst = *(fsts_[split_]);
    ++split_;
  }

  // Indicates if no more components to return.
  bool Done() const { return split_ >= fsts_.size(); }

 private:
  void SplitNGramModel(const StdFst &fst);

 private:
  NGramModel model_;
  vector<NGramContext*> contexts_;  // Ordered contexts to split
  vector<StdVectorFst*> fsts_;      // FST for ith binary split
  vector<map<StateId, StateId> > state_maps_;  // State mapping in each split
  vector<set<size_t> > state_splits_;  // Set of splits for each inpyut state
  size_t split_;

  DISALLOW_COPY_AND_ASSIGN(NGramSplit);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_SPLIT_H__
