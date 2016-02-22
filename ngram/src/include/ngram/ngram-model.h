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
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// NGram model class

#ifndef NGRAM_NGRAM_MODEL_H__
#define NGRAM_NGRAM_MODEL_H__

#include <iostream>
#include <set>
#include <vector>

#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/compose.h>

namespace ngram {

using std::ostream;
using std::set;
using std::vector;

using fst::Fst;
using fst::StdFst;

using fst::ArcIterator;

using fst::StdArc;
using fst::LogArc;
using fst::Matcher;
using fst::MATCH_INPUT;
using fst::MATCH_NONE;

using fst::kNoLabel;
using fst::kNoStateId;

// Default normalization constant (e.g., for checks)
const double kNormEps = 0.001;
const double kFloatEps = 0.000001;
const double kInfBackoff = 99.00;

class NGramModel {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  // Construct an NGramModel object, consisting of the FST and some
  // information about the states under the assumption that the FST is
  // a model. The 'backoff_label' is what is followed when there is no
  // word match at a given order. The 'norm_eps' is the epsilon used
  // in checking weight normalization.  If 'state_ngrams' is true,
  // this class explicitly finds, checks the consistency of and stores
  // the ngram that must be read to reach each state (normally false
  // to save some time and space).
  explicit NGramModel(const StdFst &infst, Label backoff_label = 0,
		      double norm_eps = kNormEps, bool state_ngrams = false)
    : fst_(infst), backoff_label_(backoff_label), norm_eps_(norm_eps),
    have_state_ngrams_(state_ngrams) { InitModel(); }

  // Number of states in the LM fst
  StateId NumStates() const { return nstates_; }

  // Size of ngram model is the sum of the number of states and number of arcs
  int64 GetSize() const {
    int64 size = 0;
    for (StateId st = 0; st < nstates_; ++st)
      size += fst_.NumArcs(st) + 1;  // number of arcs + 1 state
    return size;
  }

  // Returns highest order
  int HiOrder() const { return hi_order_; }

  // Returns order of a given state
  int StateOrder(StateId state) const {
    if (state >= 0 && state < nstates_)
      return state_orders_[state];
    else return -1;
  }

  // Returns n-gram that must be read to reach 'state'. '0' signifies
  // super-initial 'word'. Constructor argument 'state_ngrams' must be true.
  const vector<Label> &StateNGram(StateId state) const {
    if (!have_state_ngrams_)
      LOG(FATAL) << "NGramModel: state ngrams not available";
    return state_ngrams_[state];
  }

  // Unigram state
  StateId UnigramState() const { return unigram_; }

  // Returns the unigram cost of requested symbol
  double GetSymbolUnigramCost(Label symbol) const;

  // Label of backoff transitions
  Label BackoffLabel() const { return backoff_label_; }

  // Finds the backoff state for a given state st, and provide bocost if req'd
  StateId GetBackoff(StateId st, double *bocost) const;

  // Verifies LM topology is sane.
  bool CheckTopology() const {
    ascending_ngrams_ = 0;
    // Checks state topology
    for (StateId st = 0; st < nstates_; ++st)
      if (!CheckTopologyState(st)) return false;
    // All but start and unigram state should have a unique ascending ngram arc
    if (unigram_ != -1 && ascending_ngrams_ != nstates_ - 2) {
      VLOG(1) << "Incomplete # of ascending n-grams: " << ascending_ngrams_;
      return false;
    }
    return true;
  }

  // Iterates through all states and validate that they are fully normalized
  bool CheckNormalization() const {
    for (StateId st = 0; st < nstates_; ++st)
      if (!CheckNormalizationState(st)) return false;
    return true;
  }

  // Calculates backoff cost from neglog sums of hi and low order arcs
  double CalculateBackoffCost(double hi_neglog_sum, double low_neglog_sum, 
			      bool infinite_backoff = 0) const;

  // Calculates backoff cost from neglog sums of hi and low order arcs
  bool CalculateBackoffFactors(double hi_neglog_sum, double low_neglog_sum,
			       double *nlog_backoff_num,
			       double *nlog_backoff_denom,
			       bool infinite_backoff = 0) const;

  // Fst const reference
  const StdFst& GetFst() const { return fst_; }

  // Called at construction. If the model topology is mutated, this should
  // be re-called prior to any member function that depends on it.
  void InitModel();


  // Accessor function for the norm_eps_ parameter
  double NormEps() const {
    return norm_eps_;
  }

  // Calculates number of n-grams at state
  int NumNGrams(StateId st) {
    int num_ngrams = fst_.NumArcs(st);  // arcs are n-grams
    if (GetBackoff(st, 0) >= 0)         // except one arc, backoff arc
      num_ngrams--;
    if (fst_.Final(st) != StdArc::Weight::Zero())  // </s> n-gram
      num_ngrams++;
    return num_ngrams;
  }

  // Mimics a phi matcher: follow backoff links until final state found
  double FinalCostInModel(StateId mst, int *order) const;

  // Calculate marginal state probs.  By default, uses the product of
  // the order-ascending ngram transition probabilities. If
  // 'stationary' is true, instead computes the stationary
  // distribution of the Markov chain.
  void CalculateStateProbs(vector<double> *log_probs,
                           bool stationary = false) const;

  // Change data for a state that would normally be computed
  // by InitModel; this allows incremental updates
  void UpdateState(StateId st, int order, bool unigram_state,
                   const vector<Label> *ngram = 0) {
    if (have_state_ngrams_ && !ngram)
      LOG(FATAL) << "NGramModel::UpdateState: no ngram provides";

    if (state_orders_.size() < st)
      LOG(FATAL) << "NGramModel::UpdateState: bad state: " << st;

    if (order > hi_order_)
      hi_order_ = order;

    if (state_orders_.size() == st) {  // add state info
      state_orders_.push_back(order);
      if (ngram)
	state_ngrams_.push_back(*ngram);
      ++nstates_;
    } else {                          // modifies state info
      state_orders_[st] = order;
      if (ngram)
	state_ngrams_.push_back(*ngram);
    }

    if (unigram_state)
      unigram_ = nstates_;
  }

 protected:
  // Calculate - log( exp(a - b) + 1 ) for use in high precision NegLogSum
  static double NegLogDeltaValue(double a, double b, double *c) {
    double x = exp(a - b), delta = - log(x + 1);
    if (x < kNormEps) {  // for small x, use Mercator Series to calculate
      delta = -x;
      for (int j = 2; j <= 4; ++j)
	delta += pow(-x, j) / j;
    }
    if (c) delta -= (*c);  // Sum correction from Kahan formula (if using)
    return delta;
  }

  // Precision method for summing reals and saving negative logs
  // -log( exp(-a) + exp(-b) ) = a - log( exp(a - b) + 1 )
  // Uses Mercator series and Kahan formula for additional numerical stability
  static double NegLogSum(double a, double b, double *c) {
    if (a == StdArc::Weight::Zero().Value()) return b;
    if (b == StdArc::Weight::Zero().Value()) return a;
    if (a > b) return NegLogSum(b, a, c);
    double delta = NegLogDeltaValue(a, b, c), val = a + delta;
    if (c) (*c) = (val - a) - delta;  // update sum correction for Kahan formula
    return val;
  }

  // Summing reals and saving negative logs, no Kahan formula (backwards compat)
  static double NegLogSum(double a, double b) {
    return NegLogSum(a, b, 0);
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

  // Fills a vector with the counts of each state, based on prefix count
  void FillStateCounts(vector<double> *state_counts) {
    for (int i = 0; i < nstates_; i++)
      state_counts->push_back(StdArc::Weight::Zero().Value());
    WalkStatesForCount(state_counts);
  }

  // Collects backoff arc weights in a vector
  void FillBackoffArcWeights(StateId st, StateId bo,
			     vector<double> *bo_arc_weight) const;

  // Returns the backoff cost for state st
  double GetBackoffCost(StateId st) const {
    double bocost;
    StateId bo = GetBackoff(st, &bocost);
    if (bo < 0)  // if no backoff arc found
      bocost = StdArc::Weight::Zero().Value();
    return bocost;
  }

  // Uses iterator in place of matcher for arc iterators; allows
  // getting Position(). NB: begins search from current position.
  bool FindArc(ArcIterator<StdFst> *biter, Label label) const {
    while (!biter->Done()) {  // scan through arcs
      StdArc barc = biter->Value();
      if (barc.ilabel == label) return true;  // if label matches, true
      else if (barc.ilabel < label)  // if less than value, go to next
	biter->Next();
      else return false;  // otherwise no match
    }
    return false;  // no match found
  }

  // Finds the arc weight associated with a label at a state
  double FindArcWeight(StateId st, Label label) const {
    double cost = StdArc::Weight::Zero().Value();
    Matcher<StdFst> matcher(fst_, MATCH_INPUT);
    matcher.SetState(st);
    if (matcher.Find(label)) {
      StdArc arc = matcher.Value();
      cost = arc.weight.Value();
    }
    return cost;
  }

  // Mimics a phi matcher: follow backoff arcs until label found or no backoff
  bool FindNGramInModel(StateId *mst, int *order,
			Label label, double *cost) const;

  // Sums final + arc probs out of state and for same transitions
  // out of backoff
  bool CalcBONegLogSums(StateId st, double *hi_neglog_sum,
			double *low_neglog_sum, bool infinite_backoff = false,
			bool unigram = false) const;

  // Prints a state ngram to a stream
  bool PrintStateNGram(StateId st, ostream &ostrm = std::cerr) const;

  // Modifies n-gram weights according to printing parameters
  static double WeightRep(double wt, bool neglogs, bool intcnts) {
    if (!neglogs || intcnts)
      wt = exp(-wt);
    if (intcnts)
      wt = round(wt);
    return wt;
  }

  // Estimate total unigram count based on probabilities in unigram state
  // The difference between two smallest probs should be 1/N, return reciprocal
  double EstimateTotalUnigramCount() const;

 private:
  // Iterates through arcs, accumulates neglog probs from arcs and
  // their backoffs
  void CalcArcNegLogSums(StateId st, StateId bo,
			 double *hi_sum, double *low_sum,
			 bool infinite_backoff = false) const;

  // Iterates through arcs, accumulates neglog probs from arcs and
  // their backoffs.  Used when efficient method fails to produce a sane value
  double CalcBruteLowSum(StateId st, StateId bo, double start_low) const;

  // Traverses n-gram fst and record each state's n-gram order, return highest
  void ComputeStateOrders();

  // Ensures correct topology for a given state: existence of backoff
  // transition to backoff state with matching backed-off arcs (if not unigram)
  bool CheckTopologyState(StateId st) const;

  // Checks state ngrams for consistency
  bool CheckStateNGrams(StateId st, const StdArc &arc) const;

  // Ensures normalization for a given state to error epsilon
  // sum of state probs + exp(-backoff_cost) - sum of arc backoff probs = 1
  bool CheckNormalizationState(StateId st) const;

  // For accumulated negative log probabilities, tests for normalization
  bool EvaluateNormalization(StateId st, StateId bo, double bocost,
			     double Norm, double Norm1) const;

  // For accumulated negative log probabilities, a 2nd test for normalization
  bool ReevaluateNormalization(StateId st, double bocost, double norm,
			       double norm1) const;

  // Collects prefix counts for arcs out of a specific state
  void CollectPrefixCounts(vector<double> *state_counts, StateId st) const {
    for (ArcIterator<StdFst> aiter(fst_, st);
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

  // Walks model automaton to collect prefix counts for each state
  void WalkStatesForCount(vector<double> *state_counts) const {
    if (unigram_ != -1) {
      (*state_counts)[fst_.Start()] = fst_.Final(unigram_).Value();
      CollectPrefixCounts(state_counts, unigram_);
    }
    CollectPrefixCounts(state_counts, fst_.Start());
  }

  // Tests to see if model came from pre-summing a mixture
  // Should have: backoff weights > 0; higher order always higher prob (summed)
  bool MixtureConsistent() const;

  // At a given state, calculate the marginal prob p(h) based on
  // the smoothed, order-ascending n-gram transition probabilities.
  void NGramStateProb(StateId st, vector<double> *prob) const;

  // Calculate marginal state probs as the product of the smoothed,
  // order-ascending ngram transition probablities: p(abc) =
  // p(a)p(b|a)p(c|ba) (odd w/KN)
  void NGramStateProbs(vector<double> *probs, bool norm = false) const;

  // At a given state, calculate one step of the power method
  // for the stationary distribution of the closure of the
  // LM with re-entry probability 'alpha'.
  void StationaryStateProb(StateId st,  vector<double> *init_probs,
                           vector<double> *probs, double alpha) const;

  // Calculate marginal state probs as the stationary distribution
  // of the Markov chain consisting of the closure of the LM
  // with re-entry probability 'alpha'. The convergence is controlled
  // by 'converge_eps'.
  void StationaryStateProbs(vector<double> *probs,
                            double alpha, double converge_eps) const;

  const StdFst &fst_;
  StateId unigram_;              // unigram state
  Label backoff_label_;          // label of backoff transitions
  StateId nstates_;              // number of states in LM
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
