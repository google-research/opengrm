// ngram-merge.h
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
// NGram base model class for merging n-gram FSTs

#ifndef NGRAM_NGRAM_MERGE_H__
#define NGRAM_NGRAM_MERGE_H__

#include <map>
#include <set>

#include <fst/vector-fst.h>
#include <ngram/ngram-mutable-model.h>

namespace ngram {

using fst::StdVectorFst;
using std::multimap;
using std::set;

class NGramMerge : public NGramMutableModel {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  // Constructs an NGramMerge object consisting of ngram model to be merged.
  // Ownership of FST is retained by the caller.
  NGramMerge(StdMutableFst *infst1, Label backoff_label = 0,
             double norm_eps = kNormEps, bool check_consistency = false)
      : NGramMutableModel(infst1, backoff_label, norm_eps, check_consistency),
        ngram2_(0),
        check_consistency_(check_consistency),
        fst2_(0) {
    NGramMutableModel::SetAllowInfiniteBO();  // set switch if inf backoff costs
  }

  virtual ~NGramMerge() {
    delete ngram2_;
    delete fst2_;
  }

 protected:
  // Some terminology used in the merge class:
  //  'shared arc': this is an arc that is found in both input FSTs leaving
  //     the exact same state context.
  //  'unshared arc': not a shared arc
  //  'backoff destination': the destination state found by backing off from a
  //     given state that matches a label
  //  'backoff weight' the accumulated weight to the 'backoff destination'
  //    including the matching arc weight

  // Perform merger with n-gram model specified by the FST argument.
  // Ownership of FST is retained by the caller.
  void MergeNGramModels(const StdFst &infst2, bool norm = false);

  // Specifies the resultant weight when combining a weight from each
  // FST.  If the weight comes from a shared arc (or final weight),
  // then in_fst1 and in_fst2, are true. O.w., if the arc comes from
  // the first FST, then 'in_fst1' is true, 'w1' is the arc weight,
  // and 'w2' is the backed-off weight to the matching arc on the
  // second FST from backed-off state 's2'. Similarly if the arc comes
  // from the second FST.
  virtual double MergeWeights(StateId s1, StateId s2, Label label,
                              double w1, double w2,
                              bool in_fst1, bool in_fst2) const = 0;

  // Specifies the normalization constant per state depending whether the
  // state was present in one or boths FSTs.
  virtual double NormWeight(bool in_fst1, bool in_fst2) const {
    return 0.0;
  }

  // Specifies if unshared arcs/final weights between the two
  // FSTs in a merge have a non-trivial merge. In particular, this
  // means MergeWeights() changes the arc or final weights; any
  // destination state changes are not relevant here. When false, more
  // efficient merging may be performed. If the arc/final_weight
  // comes from the first FST, then 'in_fst1' is true.
  virtual bool MergeUnshared(bool in_fst1) const { return true; }

  // model to be mixed in into ngram1
  const NGramModel &NGram2() const { return *ngram2_; }

  // original number of states in ngram1
  size_t NumStates1() const { return ngram1_ns_; }
  // original number of states in ngram2
  size_t NumStates2() const { return ngram2_ns_; }

  // Maps from a state to its exact same context in the other model.
  // These include states that have been added to NGram1.
  StateId ExactMap1To2(StateId s1) const { return exact_map_1to2_[s1]; }
  StateId ExactMap2To1(StateId s2) const { return exact_map_2to1_[s2]; }

  // Maps from a state to its closest backed-off context in the other model
  // These are wrt original model before any states have been added to NGram1
  StateId BackoffMap1To2(StateId s1) const { return backoff_map_1to2_[s1]; }
  StateId BackoffMap2To1(StateId s2) const { return backoff_map_2to1_[s2]; }

 private:
  // Merging word lists, relabeling arc labels in incoming ngram2
  void MergeWordLists();

  // Finds word key if in list, otherwise adds to list (for merging wordlists)
  int64 NewWordKey(string symbol, int64 key);

  // Maps states representing identical n-gram histories
  void SetupMergeMaps();

  // Creates exact and backoff state maps. Maps for exact state context
  // corrspondence created if 'exact' is true. Map that finds state in
  // ngram2 that is the closest backed-off context from corresponding
  // state in ngrams is created if 'bo_from1' is true. Similarly,
  // for 'bo_from2'.
  void MergeStateMaps(StateId st, StateId ist, bool exact,
                      bool bo_from1, bool bo_from2) {
    if (exact)
      MergeExactStateMap(st, ist);
    if (bo_from1)
      MergeBackoffStateMap(ist, st, true);
    if (bo_from2)
      MergeBackoffStateMap(st, ist, false);
  }

  // Creates a state map from ngram2 to equivalent states in ngram1
  void MergeExactStateMap(StateId st, StateId ist);

  // Creates a map from a state in one ngram model to closest backed-off
  // context in other model
  void MergeBackoffStateMap(StateId st, StateId ist, bool from1);

  // Creates a map from state s and label l in ngram1, with state s
  // having label l on an outgoing arc to destination d, to the set of
  // states s', backing off to s, that also have an arc labeled with l
  // and going to destination d. Computed only for 'NonAscending()' arcs.
  void MergeBackedOffToMap();

  // Combines Fsts; not necessarily normalized.
  void MergeFsts();

  // For n-gram arcs shared in common, combines weight, sets correct dest.
  void MergeSharedArcs(StateId st, StateId ist, set<Label> *shared);

  // Merges n-gram arcs not found in the new model
  // Applies when MergeUnshared(true) is true.
  void MergeUnsharedArcs1(StateId st, StateId ist, const set<Label> &shared);

  // Merges n-gram arcs not found in the original model
  void MergeUnsharedArcs2(StateId st, StateId ist, const set<Label> &shared);

  // Merges n-gram arcs not found in the new model by (solely) performing any
  // necessary destination state changes from 'old_dest' to 'new_dest'.
  // Looks up arcs that backoff to state 'low_src' and through label 'label'
  // to 'old_dest'. Applies when MergeUnshared(true) is false.
  void MergeDests1(StateId low_src, Label label, StateId old_dest,
                   StateId new_dest);

  // Finds the destination state with label from a backoff model (assign cost)
  StateId MergeBackoffDest(StateId st, Label label, bool from1, double *cost);

  // Calculates correct normalization constant for each state and normalize.
  void NormStates();

  // Applies normalization constant to arcs and final cost at state
  void NormState(StateId st, bool in_fst1, bool in_fst2);

  // Collects target state for backoff state map
  void UpdateBackoffMap(StateId st, StateId ist, bool from1) {
    if (from1) {
      backoff_map_1to2_[ist] = st;
    } else {
      backoff_map_2to1_[ist] = st;
    }
  }

  // NB: ngram1 is *this
  const NGramModel *ngram2_;          // model to be mixed in into ngram1
  bool check_consistency_;

  // Maps from a state to its exact same context in the other model
  // These include states that have been added to NGram1.
  vector<StateId> exact_map_1to2_;   // mapping ngram1 states to ngram2 states
  vector<StateId> exact_map_2to1_;   // mapping ngram2 states to ngram1 states

  // Maps from a state to its closest backed-off context in the other model
  // These are wrt original model before any states have been added to NGram1
  vector<StateId> backoff_map_1to2_;  // mapping ngram1 states to ngram2 states
  vector<StateId> backoff_map_2to1_;  // mapping ngram2 states to ngram1 states

  // Given a state s and a label l on an outgoing arc to destination d
  // in ngram1, returns the set of states backing off to s, that also
  // have an arc labeled with l and going to destination d.  Computed
  // only for non-ascending arcs.
  multimap< pair<StateId, Label>, StateId> backed_off_to_;

  size_t ngram1_ns_;    // original number of states in ngram1
  size_t ngram2_ns_;    // original number of states in ngram2

  StdVectorFst *fst2_;  // copy of FST2 (non-0 only when needed)

  DISALLOW_COPY_AND_ASSIGN(NGramMerge);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_MERGE_H__
