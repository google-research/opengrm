// ngram-model.h
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
// NGram model class

#ifndef NGRAM_NGRAM_MODEL_H__
#define NGRAM_NGRAM_MODEL_H__

#include <iostream>
#include <vector>

#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/mutable-fst.h>
#include <fst/compose.h>

namespace ngram {

using std::vector;
using std::ostream;

using fst::Fst;
using fst::StdFst;
using fst::StdExpandedFst;
using fst::StdMutableFst;

using fst::ArcIterator;
using fst::MutableArcIterator;

using fst::StdArc;
using fst::LogArc;
using fst::Matcher;
using fst::MATCH_INPUT;
using fst::MATCH_NONE;

using fst::kNoLabel;
using fst::kNoStateId;

static const double kNormEps = 0.000001;

class NGramModel {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;

  // Construct an NGramModel object, consisting of the FST and some
  // information about the states under the assumption that the FST is
  // a model. Ownership of the FST is retained by the caller. The
  // 'backoff_label' is what is followed when there is no word match
  // at a given order. The 'norm_eps' is the epsilon used in checking
  // weight normalization.  If 'state_ngrams' is true, this class
  // explicitly finds, checks the consistency of and stores the ngram that
  // must be read to reach each state (normally false to save some time
  // and space).
  explicit NGramModel(StdMutableFst *infst, Label backoff_label = 0,
		      double norm_eps = kNormEps, bool state_ngrams = false)
    : fst_(infst), backoff_label_(backoff_label), norm_eps_(norm_eps),
    have_state_ngrams_(state_ngrams) { InitModel(); }

  // Size of ngram model is the sum of the number of states and number of arcs
  int64 GetSize() const {
    int64 size = 0;
    for (StateId st = 0; st < fst_->NumStates(); ++st)
      size += fst_->NumArcs(st) + 1;  // number of arcs + 1 state
    return size;
  }

  // Returns highest order
  int HiOrder() const { return hi_order_; }

  // Returns order of a given state
  int StateOrder(StateId state) const {
    if (state >= 0 && state < fst_->NumStates())
      return state_orders_[state];
    else return -1;
  }

  // Returns n-gram that must be read to reach 'state'.
  // Constructor argument 'state_ngrams' must be true.
  const vector<Label> &StateNGram(StateId state) const {
    if (!have_state_ngrams_)
      LOG(FATAL) << "NGramModel: state ngrams not available";
    return state_ngrams_[state];
  }

  // Unigram state
  StateId UnigramState() const { return unigram_; }

  // Returns the unigram cost of requested symbol
  double GetSymbolUnigramCost(Label symbol);

  // Label of backoff transitions
  Label BackoffLabel() const { return backoff_label_; }

  // Find the backoff state for a given state st, and provide bocost if req'd
  StateId GetBackoff(StateId st, double *bocost) const;

  bool CheckTopology() const {
    ascending_ngrams_ = 0;
    // Checks state topology
    for (StateId st = 0; st < fst_->NumStates(); ++st)
      if (!CheckTopologyState(st)) return false;
    // All but start and unigram state should have a unique ascending ngram arc
    if (unigram_ != -1 && ascending_ngrams_ != fst_->NumStates() - 2) {
      VLOG(1) << "Incomplete # of ascending n-grams: " << ascending_ngrams_;
      return false;
    }
    return true;
  }

  // Iterate through all states and validate that they are fully normalized
  bool CheckNormalization() const {
    for (StateId st = 0; st < fst_->NumStates(); ++st)
      if (!CheckNormalizationState(st)) return false;
    return true;
  }

  // Expanded Fst const reference
  const StdExpandedFst& GetFst() const { return *fst_; }

  // Mutable Fst pointer
  StdMutableFst* GetMutableFst() { return fst_; }

  // Called at construction. If the model topology is mutated, this should
  // be re-called prior to any member function that depends on it.
  void InitModel();

  // For all states, recalculate backoff cost, assign to backoff arc
  // (if exists)
  void RecalcBackoff() {
    for (StateId st = 0; st < fst_->NumStates(); ++st) {
      double hi_neglog_sum, low_neglog_sum;
      if (CalcBONegLogSums(st, &hi_neglog_sum, &low_neglog_sum)) {
	UpdateBackoffCost(st, hi_neglog_sum, low_neglog_sum);
      }
    }
  }

  // Fill a vector with the counts of each state, based on prefix count
  void FillStateCounts(vector<double> *state_counts) {
    for (int i = 0; i < fst_->NumStates(); i++)
      state_counts->push_back(StdArc::Weight::Zero().Value());
    WalkStatesForCount(state_counts);
  }

  // Scale weights in the whole model
  void ScaleWeights(double scale) {
    for (StateId st = 0; st < fst_->NumStates(); ++st)
      ScaleStateWeight(st, scale);
  }

  // Accessor function for the norm_eps_ parameter
  double NormEps() {
    return norm_eps_;
  }
 protected:
  // Modify n-gram weights according to printing parameters
  static double WeightRep(double wt, bool neglogs, bool intcnts) {
    if (!neglogs || intcnts)
      wt = exp(-wt);
    if (intcnts)
      wt = round(wt);
    return wt;
  }

  // log accumulator for double
  static double NegLogSum(double a, double b) {
    if (a == StdArc::Weight::Zero().Value()) return b;
    if (b == StdArc::Weight::Zero().Value()) return a;
    if (a > b) return NegLogSum(b, a);
    return a - log(exp(a - b) + 1);
  }

  // negative log of difference: -log(exp^{-a} - exp^{-b})
  //   FRAGILE: assumes exp^{-a} >= exp^{-b}
  static double NegLogDiff(double a, double b) {
    if (b == StdArc::Weight::Zero().Value()) return a;
    if (a >= b) {
      if (a - b < kNormEps) // equal within fp error
	return StdArc::Weight::Zero().Value();
      LOG(FATAL) << "NegLogDiff: undefined " << a << " " << b;
    }
    return b - log(exp(b - a) - 1);
  }

  // Collect backoff arc weights in a vector
  void FillBackoffArcWeights(StateId st, StateId bo,
			     vector<double> *bo_arc_weight) const;

  // Return the backoff cost for state st
  double GetBackoffCost(StateId st) const {
    double bocost;
    StateId bo = GetBackoff(st, &bocost);
    if (bo < 0)  // if no backoff arc found
      bocost = StdArc::Weight::Zero().Value();
    return bocost;
  }

  double GetBackoffFinalCost(StateId st) const {
    if (fst_->Final(st) != StdArc::Weight::Zero())
      return fst_->Final(st).Value();
    double fcost;
    StateId bo = GetBackoff(st, &fcost);
    fcost += GetBackoffFinalCost(bo);
    if (fcost != StdArc::Weight::Zero().Value())
      fst_->SetFinal(st, fcost);
    return fcost;
  }

  // Mimic a phi matcher: follow backoff arcs until label found or no backoff
  bool FindNGramInModel(StateId *mst, int *order,
			Label label, double *cost) const;

  // Mimic a phi matcher: follow backoff links until final state found
  double FinalCostInModel(StateId mst, int *order) const;

  // Sum final + arc probs out of state and for same transitions out of backoff
  bool CalcBONegLogSums(StateId st, double *hi_neglog_sum,
			double *low_neglog_sum) const;

  // Find word key if in list, otherwise add to list (for merging wordlists)
  int64 NewWordKey(string symbol) {
    int64 key = fst_->InputSymbols()->Find(symbol);
    if (key < 0) {
      fst_->MutableInputSymbols()->AddSymbol(symbol);
      fst_->MutableOutputSymbols()->AddSymbol(symbol);
      key = fst_->InputSymbols()->Find(symbol);
    }
    return key;
  }

  // Use iterator in place of matcher for mutable arc iterators,
  // avoid full copy
  bool FindMutableArc(MutableArcIterator<StdMutableFst> *biter,
		      Label label) const {
    StdArc barc = biter->Value();
    while (!biter->Done() && barc.ilabel < label) {
      biter->Next();
      barc = biter->Value();
    }
    if (biter->Done() || barc.ilabel != label) return false;
    return true;
  }

  // Scale weights by some factor, for normalizing and use in model merging
  void ScaleStateWeight(StateId st, double scale);

  // Replace backoff weight with -log p(backoff)
  void DeBackoffNGramModel();

  // Prints a state ngram to a stream
  bool PrintStateNGram(StateId st, ostream &ostrm = cerr) const;

 private:
  // Calculate and assign backoff cost from neglog sums of hi and low
  // order arcs
  void UpdateBackoffCost(StateId st, double hi_neglog_sum,
			 double low_neglog_sum);

  // Iterate through arcs, accumulate neglog probs from arcs and their backoffs
  void CalcArcNegLogSums(StateId st, StateId bo,
			 double *hi_sum, double *low_sum) const;

  // Traverse n-gram fst and record each state's n-gram order, return highest
  void ComputeStateOrders();

  // Ensure correct topology for a given state: existence of backoff
  // transition to backoff state with matching backed-off arcs (if not unigram)
  bool CheckTopologyState(StateId st) const;

  // Checks state ngrams for consistency
  bool CheckStateNGrams(StateId st, const StdArc &arc) const;

  // Ensure normalization for a given state to error epsilon
  // sum of state probs + exp(-backoff_cost) - sum of arc backoff probs = 1
  bool CheckNormalizationState(StateId st) const;

  // For accumulated negative log probabilities, test for normalization
  bool EvaluateNormalization(StateId st, StateId bo,
			     double Norm, double Norm1) const;

  // Find the arc weight of the backoff
  double FindArcWeight(StateId st, Label label) const {
    double cost = StdArc::Weight::Zero().Value();
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);  // for querying backoff
    matcher.SetState(st);
    if (matcher.Find(label)) {
      StdArc arc = matcher.Value();
      cost = arc.weight.Value();
    }
    return cost;
  }

  // Collect prefix counts for arcs out of a specific state
  void CollectPrefixCounts(vector<double> *state_counts, StateId st) {
    for (MutableArcIterator<StdMutableFst> aiter(fst_, st);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      if (arc.ilabel != backoff_label_ &&  // only counting non-backoff arcs
	  state_orders_[st] < state_orders_[arc.nextstate]) {  // that + order
	(*state_counts)[arc.nextstate] = arc.weight.Value();
	CollectPrefixCounts(state_counts, arc.nextstate);
      }
    }
  }
  
  // Walk model automaton to collect prefix counts for each state
  void WalkStatesForCount(vector<double> *state_counts) {
    if (unigram_ != -1) {
      (*state_counts)[fst_->Start()] = fst_->Final(unigram_).Value();
      CollectPrefixCounts(state_counts, unigram_);
    }
    CollectPrefixCounts(state_counts, fst_->Start());
  }

  // Scan arcs and remove lower order from arc weight
  void UnSumState(StateId st);

  // Test to see if model came from pre-summing a mixture
  // Should have: backoff weights > 0; higher order always higher prob (summed)
  bool MixtureConsistent() const;

 StdMutableFst*fst_;
  StdArc::StateId unigram_;      // unigram state
  StdArc::Label backoff_label_;  // label of backoff transitions
  int hi_order_;                 // highest order in the model
  double norm_eps_;              // epsilon diff allowed to ensure normalized
  vector<int> state_orders_;     // order of each state
  bool have_state_ngrams_;       // compute and store state n-gram info
  mutable size_t ascending_ngrams_;  // # of n-gram arcs that increase order
  vector< vector<Label> > state_ngrams_;  // n-gram always read to reach state
  DISALLOW_COPY_AND_ASSIGN(NGramModel);
};

// Casts Fst to a MutableFst if possible, otherwise copies
// into a VectorFst deleting the input.
template <class Arc>
fst::MutableFst<Arc> *MutableFstConvert(fst::Fst<Arc> *ifst) {
  fst::MutableFst<Arc> *ofst = 0;
  if (ifst->Properties(fst::kMutable, false)) {
    ofst = fst::down_cast<fst::MutableFst<Arc> *>(ifst);
  } else {
    ofst = new fst::VectorFst<Arc>(*ifst);
    delete ifst;
  }
  return ofst;
}

}  // namespace ngram

#endif  // NGRAM_NGRAM_MODEL_H__
