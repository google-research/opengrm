// ngram-mutable-model.h
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
// NGram mutable model class

#ifndef NGRAM_NGRAM_MUTABLE_MODEL_H__
#define NGRAM_NGRAM_MUTABLE_MODEL_H__

#include <algorithm>
#include <vector>

#include <fst/mutable-fst.h>
#include <fst/statesort.h>
#include <ngram/ngram-model.h>

namespace ngram {

using fst::StdExpandedFst;
using fst::StdMutableFst;
using fst::MutableArcIterator;

class NGramMutableModel : public NGramModel {
 public:
  // Constructs an NGramMutableModel object, derived from NGramModel,
  // that adds mutable methods such as backoff normalization.
  // Ownership of the FST is retained by the caller.
  explicit NGramMutableModel(StdMutableFst *infst, Label backoff_label = 0,
                             double norm_eps = kNormEps,
			     bool state_ngrams = false,
			     bool infinite_backoff = false)
      : NGramModel(*infst, backoff_label, norm_eps, state_ngrams),
        infinite_backoff_(infinite_backoff), mutable_fst_(infst) {}

  // ExpandedFst const reference
  const StdExpandedFst& GetExpandedFst() const { return *mutable_fst_; }

  // Mutable Fst pointer
  StdMutableFst* GetMutableFst() { return mutable_fst_; }

  // For given state, recalculates backoff cost, assigns to backoff arc
  void RecalcBackoff(StateId st) {
    double hi_neglog_sum, low_neglog_sum;
    if (CalcBONegLogSums(st, &hi_neglog_sum, &low_neglog_sum,
			 infinite_backoff_)) {
      UpdateBackoffCost(st, hi_neglog_sum, low_neglog_sum);
    }
  }

  // For all states, recalculates backoff cost, assigns to backoff arc
  // (if exists)
  void RecalcBackoff() {
    for (StateId st = 0; st < mutable_fst_->NumStates(); ++st)
      RecalcBackoff(st);
  }

  // Scales weights in the whole model
  void ScaleWeights(double scale) {
    for (StateId st = 0; st < mutable_fst_->NumStates(); ++st)
      ScaleStateWeight(st, scale);
  }

  // Looks for infinite backoff cost in model, sets flag to allow if found
  void SetAllowInfiniteBO() {
    for (StateId s = 0; s < NumStates(); ++s) {
      double bocost;
      StateId bo = GetBackoff(s, &bocost);
      if (bo >= 0 && bocost >= kInfBackoff) {
	infinite_backoff_ = true;  // found an 'infinite' backoff, so true
	return;
      }
    }
  }

  // Sorts states in ngram-context lexicographic order.
  void SortStates() {
    vector<StateId> order(NumStates()), inv_order(NumStates());
    for (StateId s = 0; s < NumStates(); ++s)
      order[s] = s;
    sort(order.begin(), order.end(), StateCompare(*this));
    for (StateId s = 0; s < NumStates(); ++s)
      inv_order[order[s]] = s;
    StateSort(mutable_fst_, inv_order);
  }

 protected:
  double GetBackoffFinalCost(StateId st) const {
    if (mutable_fst_->Final(st) != StdArc::Weight::Zero())
      return mutable_fst_->Final(st).Value();
    double fcost;
    StateId bo = GetBackoff(st, &fcost);
    fcost += GetBackoffFinalCost(bo);
    if (fcost != StdArc::Weight::Zero().Value())
      mutable_fst_->SetFinal(st, fcost);
    return fcost;
  }

  // Uses iterator in place of matcher for mutable arc iterators,
  // avoids full copy and allows getting Position(). NB: begins
  // search from current position.
  bool FindMutableArc(MutableArcIterator<StdMutableFst> *biter,
		      Label label) const {
    while (!biter->Done()) {  // scan through arcs
      StdArc barc = biter->Value();
      if (barc.ilabel == label) return true;  // if label matches, true
      else if (barc.ilabel < label)  // if less than value, go to next
	biter->Next();
      else return false;  // otherwise no match
    }
    return false;  // no match found
  }

  // Scales weights by some factor, for normalizing and use in model merging
  void ScaleStateWeight(StateId st, double scale);

  // Sorts arcs in state in ilabel order.
  void SortArcs(StateId st);

  // Replaces backoff weight with -log p(backoff)
  void DeBackoffNGramModel();

 private:
  // Calculates and assigns backoff cost from neglog sums of hi and low
  // order arcs
  void UpdateBackoffCost(StateId st, double hi_neglog_sum,
			 double low_neglog_sum);

  // Sets alpha to kInfBackoff for states with every possible n-gram
  void AdjustCompleteStates(StateId st, double *alpha);

  // Scans arcs and removes lower order from arc weight
  void UnSumState(StateId st);

  class StateCompare {
   public:
    StateCompare(const NGramModel &ngramlm)
        : ngramlm_(ngramlm) { }

    bool operator() (StateId s1, StateId s2) const {
      vector<Label> ngram1 = ngramlm_.StateNGram(s1);
      vector<Label> ngram2 = ngramlm_.StateNGram(s2);
      return lexicographical_compare(ngram1.begin(), ngram1.end(),
                                     ngram2.begin(), ngram2.end());
    }

   private:
    const NGramModel &ngramlm_;
  };

  bool infinite_backoff_;
  StdMutableFst *mutable_fst_;

  DISALLOW_COPY_AND_ASSIGN(NGramMutableModel);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_MUTABLE_MODEL_H__
