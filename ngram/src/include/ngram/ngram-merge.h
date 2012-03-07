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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// 
// NGram model class for merging two models

#ifndef NGRAM_NGRAM_MERGE_H__
#define NGRAM_NGRAM_MERGE_H__

#include <ngram/ngram-model.h>

namespace ngram {

class NGramMerge : public NGramModel {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;

  // Construct an NGramMerge object, consisting of two ngram models
  // and their scaling weights, as well as other info that informs the
  // merger.  The larger of the FSTs is modified by default.
  // Ownership of FSTs is retained by the caller.

  // NB: the self model (*this) is the merged model and is referred to as
  // 'ngram1' below. The mixed-in model is referred to as 'ngram2'.
  // 'infst1' and 'infst2' might first be swapped based on their
  // relative sizes and the 'fixed_order' option.
  NGramMerge(StdMutableFst *infst1,StdMutableFst*infst2,
	     double alpha, double beta, bool norm = false,
	     bool use_smoothing = false, bool fixed_order = false,
	     Label backoff_label = 0, double norm_eps = kNormEps,
             bool check_consistency = false);

  // Incorporate n-gram model ngram2 into ngram1, scaling with weights
  void MergeNGramModels();

 private:
  static bool Swap(StdExpandedFst &fst1,StdExpandedFst&fst2,
		   bool fixed_order) {
    return !fixed_order && fst1.NumStates() < fst2.NumStates();
  }

  // Merging word lists, relabeling arc labels in incoming ngram2
  void MergeWordList();

  // Map states representing identical n-gram histories, add remaining states
  void MergeAddStates();

  // First add all states from ngram2 not in ngram1, then add backoff arcs
  void MergeCreateStatesAndBackoffs();

  // For n-gram arcs shared in common, combine weight, set correct destination
  void MergeSharedArcs(StateId st, StateId ist, vector<bool> *arc_upd);

  // Assign mixture weight to arcs that are in just one of the models
  // Unshared arcs won't be used as backoffs in assigning these smoothed
  // weights. Thus, there are no ordering concerns for this particular
  // function.
  void MergeSmoothUnsharedArcs() {
    for (StateId st = 0; st < ngram2_ns_; ++st) {
      bool exact_state = new_states_[st] >= 0 ? 1 : 0;  // states exact match
      MergeSmoothUnsharedArcState(st, smooth_map_2to1_[st], exact_state, 0);
    }
    for (StateId st = 0; st < ngram1_ns_; ++st) {
      bool exact_state = (st == new_states_[smooth_map_1to2_[st]]) ? 1 : 0;
      MergeSmoothUnsharedArcState(st, smooth_map_1to2_[st], exact_state, 1);
    }
  }

  // For a state and its smoothed state, combine probs for unshared arcs
  void MergeSmoothUnsharedArcState(StateId ist, StateId st,
				   bool exact, bool from1);

  // Add n-gram arcs not found in the original model
  void MergeNewArcs(StateId st, StateId ist, vector<bool> *arc_upd) ;

  // For a given state, merge the shared arcs, add any remaining arcs req'd
  void MergeState(StateId ist);

  void MergeStateMaps(StateId st, StateId ist) {
    MergeStateMap(st, ist);
    if (use_smoothing_) {
      MergeSmoothStateMap(st, ist, 0);
      MergeSmoothStateMap(ist, st, 1);
    }
  }

  // Create a state map from ngram2 to equivalent states in ngram1
  void MergeStateMap(StateId st, StateId ist);

  // Create a map from states in one ngram model to smoothed states in other
  void MergeSmoothStateMap(StateId st, StateId ist, bool from1);

  // Collect target state for smoothed state map
  void UpdateSmoothMap(StateId st, StateId ist, bool from1) {
    if (from1) {
      smooth_map_1to2_[ist] = st;
    } else {
      smooth_map_2to1_[ist] = st;
    }
  }

  // Find the destination state with label from a smoothed model (assign cost)
  StateId MergeSmoothDest(StateId st, Label label, bool from1, double *cost);

  // Apply normalization constant to arcs and final cost at state
  void NormState(StateId st, double norm);

  // Calculate correct normalization constant for each state and normalize
  void NormStates();

  // NB: ngram1 is *this
  NGramModel ngram2_;  // model to be mixed in into ngram1
  double alpha_;  // weight to scale model ngram1
  double beta_;  // weight to scale model ngram2
  bool norm_;  // whether or not to treat the model as normalized
  bool use_smoothing_;  // whether or not to use the smoothing when merging
  vector<StateId> new_states_;  // mapping ngram2 states to ngram1 states
  vector<StateId> smooth_map_1to2_;  // mapping to states in ngram2
  vector<StateId> smooth_map_2to1_;  // mapping to states in ngram1
  Label backoff_label_;  // backoff_label_ in ngram1
  size_t ngram1_ns_;  // original number of states in ngram1
  size_t ngram2_ns_;  // original number of states in ngram2

  DISALLOW_COPY_AND_ASSIGN(NGramMerge);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_MERGE_H__
