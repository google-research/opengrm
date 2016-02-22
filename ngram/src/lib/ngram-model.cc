// ngram-model.cc
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

#include <deque>

#include <fst/arcsort.h>
#include <fst/vector-fst.h>

#include <ngram/ngram-model.h>

namespace ngram {

using fst::StdMutableFst;

using std::deque;
using std::vector;

using fst::VectorFst;
using fst::StdILabelCompare;

using fst::kAcceptor;
using fst::kIDeterministic;
using fst::kILabelSorted;

// Called at construction. If the model topology is mutated, this should
// be re-called prior to any member function that depends on it.
void NGramModel::InitModel() {
  // unigram state is set to -1 for unigram models (in which case start
  // state is the unigram state, no need to store here)
  if (fst_.Start() == kNoLabel)
    LOG(FATAL) << "NGramModel: Empty automaton";
  uint64 need_props = kAcceptor | kIDeterministic | kILabelSorted;
  uint64 have_props = fst_.Properties(need_props, true);
  if (!(have_props & kAcceptor))
    LOG(FATAL) << "NGramModel: input not an acceptor";
  if (!(have_props & kIDeterministic))
    LOG(FATAL) << "NGramModel: input not deterministic";
  if (!(have_props & kILabelSorted))
    LOG(FATAL) << "NGramModel: input not label sorted";

  if (!fst::CompatSymbols(fst_.InputSymbols(), fst_.OutputSymbols()))
    LOG(FATAL) << "NGramModel: input and output symbol tables do not match";

  nstates_ = CountStates(fst_);
  unigram_ = GetBackoff(fst_.Start(), 0);  // set the unigram state
  ComputeStateOrders();
  if (!CheckTopology())
    LOG(FATAL) << "NGramModel: bad ngram model topology";
}

// Iterate through arcs, accumulate neglog probs from arcs and their backoffs
// Used in case the more efficient method fails to produce a sane value
double NGramModel::CalcBruteLowSum(StateId st, StateId bo,
				   double start_low) const {
  double low_sum = start_low, KahanVal = 0;
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);  // for querying backoff
  matcher.SetState(bo);
  ArcIterator<StdFst> biter(fst_, bo);
  StdArc barc;
  for (ArcIterator<StdFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_) continue;
    barc = biter.Value();
    while (!biter.Done() && barc.ilabel < arc.ilabel) {  // linear scan
      if (barc.ilabel != backoff_label_)
	low_sum =  // sum of lower order probs of different labels
	  NegLogSum(low_sum, barc.weight.Value(), &KahanVal);      
      biter.Next();
      barc = biter.Value();
    }
    if (!biter.Done() && barc.ilabel == arc.ilabel) {
      biter.Next();
      barc = biter.Value();
    }
  }
  while (!biter.Done()) {  // linear scan
    if (barc.ilabel != backoff_label_)
      low_sum =  // sum of lower order probs of different labels
	NegLogSum(low_sum, barc.weight.Value(), &KahanVal);      
    biter.Next();
    barc = biter.Value();
  }
  return NegLogDiff(0.0, low_sum);
}

// Iterate through arcs, accumulate neglog probs from arcs and their backoffs
void NGramModel::CalcArcNegLogSums(StateId st, StateId bo,
				   double *hi_sum, double *low_sum,
				   bool infinite_backoff) const {
  double KahanVal1 = 0, KahanVal2 = 0;  // correction values for Kahan summation
  double init_low = (*low_sum);
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);  // for querying backoff
  if (bo >= 0)
    matcher.SetState(bo);
  for (ArcIterator<StdFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_) continue;
    if (bo < 0 || matcher.Find(arc.ilabel)) {
      if (bo >= 0) {
	StdArc barc = matcher.Value();
	(*low_sum) =  // sum of lower order probs of the same labels
	  NegLogSum((*low_sum), barc.weight.Value(), &KahanVal2);
      }
      (*hi_sum) =  // sum of higher order probs
	NegLogSum((*hi_sum), arc.weight.Value(), &KahanVal1);
    } else {
      LOG(FATAL) << "NGramModel: No arc label match in backoff state: " << st;
    }
  }
  if (bo >= 0 && infinite_backoff && (*low_sum) == 0.0)  // ok for unsmoothed
    return;
  if (bo >= 0 && (*low_sum) <= 0.0) {
    VLOG(1) << "lower order sum less than zero: " << st << " " << (*low_sum);
    double start_low = StdArc::Weight::Zero().Value();
    if (init_low == start_low)
      start_low = fst_.Final(bo).Value();
    (*low_sum) = CalcBruteLowSum(st, bo, start_low);
    VLOG(1) << "new lower order sum: " << st << " " << (*low_sum);
  }
}

// Sum final + arc probs out of state and for same transitions out of backoff
bool NGramModel::CalcBONegLogSums(StateId st, double *hi_neglog_sum,
				  double *low_neglog_sum, bool infinite_backoff,
				  bool unigram) const {
  StateId bo = GetBackoff(st, 0);
  if (bo < 0 && !unigram) return false;  // only calc for states that backoff
  (*low_neglog_sum) = (*hi_neglog_sum) =  // final costs initialize the sum
    fst_.Final(st).Value();
  // if st is final
  if (bo >= 0 && (*hi_neglog_sum) != StdArc::Weight::Zero().Value())
    (*low_neglog_sum) = fst_.Final(bo).Value();  // re-initialize lower sum
  CalcArcNegLogSums(st, bo, hi_neglog_sum, low_neglog_sum, infinite_backoff);
  return true;
}

// Traverse n-gram fst and record each state's n-gram order, return highest
void NGramModel::ComputeStateOrders() {
  state_orders_.clear();
  state_orders_.resize(nstates_, -1);

  if (have_state_ngrams_) {
    state_ngrams_.clear();
    state_ngrams_.resize(nstates_);
  }

  hi_order_ = 1;  // calculate highest order in the model
  deque<StateId> state_queue;
  if (unigram_ != kNoStateId) {
    state_orders_[unigram_] = 1;
    state_queue.push_back(unigram_);
    state_orders_[fst_.Start()] = hi_order_ = 2;
    state_queue.push_back(fst_.Start());
    if (have_state_ngrams_)
      state_ngrams_[fst_.Start()].push_back(0);  // initial context
  } else {
    state_orders_[fst_.Start()] = 1;
    state_queue.push_back(fst_.Start());
  }

  while (!state_queue.empty()) {
    StateId state = state_queue.front();
    state_queue.pop_front();
    for (ArcIterator<Fst<StdArc> > aiter(fst_, state);
	 !aiter.Done();
	 aiter.Next()) {
      const StdArc &arc = aiter.Value();
      if (state_orders_[arc.nextstate] == -1) {
	state_orders_[arc.nextstate] = state_orders_[state] + 1;
        if (have_state_ngrams_) {
          state_ngrams_[arc.nextstate] = state_ngrams_[state];
          state_ngrams_[arc.nextstate].push_back(arc.ilabel);
        }
	if (state_orders_[state] >= hi_order_)
	  hi_order_ = state_orders_[state] + 1;
	state_queue.push_back(arc.nextstate);
      }
    }
  }
}
// Find the backoff state for a given state st, and provide bocost if req'd
NGramModel::StateId NGramModel::GetBackoff(StateId st, double *bocost) const {
  StateId backoff = -1;
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);
  matcher.SetState(st);
  if (matcher.Find(backoff_label_)) {
    for (; !matcher.Done(); matcher.Next()) {
      StdArc arc = matcher.Value();
      if (arc.ilabel == kNoLabel) continue;  // non-consuming symbol
      backoff = arc.nextstate;
      if (bocost != 0)
	bocost[0] = arc.weight.Value();
    }
  }
  return backoff;
}

// Ensure correct n-gram topology for a given state.
bool NGramModel::CheckTopologyState(StateId st) const {
  if (unigram_ == -1) {  // unigram model
    if (fst_.Final(fst_.Start()) == StdArc::Weight::Zero()) {
      VLOG(1) << "CheckTopology: bad final weight for start state";
      return false;
    } else {
      return true;
    }
  }

  StateId bos = GetBackoff(st, 0);
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);  // for querying backoff

  if (st == unigram_) {  // unigram state
    if (fst_.Final(unigram_) == StdArc::Weight::Zero()) {
      VLOG(1) << "CheckTopology: bad final weight for unigram state: "
	      << unigram_;
      return false;
    } else if (have_state_ngrams_ && !state_ngrams_[unigram_].empty()) {
      VLOG(1) << "CheckTopology: bad unigram state: " << unigram_;
      return false;
    }
  } else {  // non-unigram state
    if (bos == -1) {
      VLOG(1) << "CheckTopology: no backoff state: " << st;
      return false;
    }

    if (fst_.Final(st) != StdArc::Weight::Zero() &&
        fst_.Final(bos) == StdArc::Weight::Zero()) {
      VLOG(1) << "CheckTopology: bad final weight for backoff state: " << st;
      return false;
    }

    if (StateOrder(st) != StateOrder(bos) + 1) {
      VLOG(1) << "CheckTopology: bad backoff arc from: " << st
              << " with order: " << StateOrder(st)
              << " to state: " << bos
              << " with order: " << StateOrder(bos);
      return false;
    }
    matcher.SetState(bos);
  }

  for (ArcIterator<StdFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();

    if (StateOrder(st) < StateOrder(arc.nextstate))
      ++ascending_ngrams_;

    if (have_state_ngrams_ && !CheckStateNGrams(st, arc)) {
      VLOG(1) << "CheckTopology: inconsistent n-gram states: "
	      << st << " -- " << arc.ilabel
	      << "/" << arc.weight
	      << " -> " << arc.nextstate;
      return false;
    }

    if (st != unigram_) {
      if (arc.ilabel == backoff_label_) continue;
      if (!matcher.Find(arc.ilabel)) {
        VLOG(1) << "CheckTopology: unmatched arc at backoff state: "
                << arc.ilabel << "/" << arc.weight
                << " for state: " << st;
        return false;
      }
    }
  }
  return true;
}

// Checks state ngrams for consistency
bool NGramModel::CheckStateNGrams(StateId st, const StdArc &arc) const {
  vector<Label> state_ngram;
  bool boa = arc.ilabel == backoff_label_;

  int j = state_orders_[st] - state_orders_[arc.nextstate] + (boa ? 0 : 1);
  if (j < 0)
    return false;

  for (int i = j; i < state_ngrams_[st].size(); ++i)
    state_ngram.push_back(state_ngrams_[st][i]);
  if (!boa && j <= state_ngrams_[st].size())
    state_ngram.push_back(arc.ilabel);

  return state_ngram == state_ngrams_[arc.nextstate];
}

// Prints state ngram to a stream
bool NGramModel::PrintStateNGram(StateId st, ostream &ostrm) const {
  ostrm << "state: " << st
	<< " order: " << state_orders_[st]
	<< " ngram: ";
  for (int i = 0; i < state_ngrams_[st].size(); ++i)
    ostrm << state_ngrams_[st][i] << " ";
  ostrm << "\n";
  return true;
}

// Ensure normalization for a given state to error epsilon
// sum of state probs + exp(-backoff_cost) - sum of arc backoff probs = 1
bool NGramModel::CheckNormalizationState(StateId st) const {
  double Norm, Norm1, bocost;
  StateId bo = GetBackoff(st, &bocost);
  Norm = Norm1 = fst_.Final(st).Value();  // final costs initialize the sum
  if (bo >= 0 && Norm != StdArc::Weight::Zero().Value())  // if st is final
    Norm1 = fst_.Final(bo).Value();  // re-initialize lower sum
  CalcArcNegLogSums(st, bo, &Norm, &Norm1, (bocost == kInfBackoff));
  return EvaluateNormalization(st, bo, bocost, Norm, Norm1);  // Normalized?
}

// For accumulated negative log probabilities, test for normalization
bool NGramModel::EvaluateNormalization(StateId st, StateId bo, double bocost,
				       double norm, double norm1) const {
  double newnorm = norm;
  if (bo >= 0) {
    newnorm = NegLogSum(norm, bocost);
    if (newnorm < norm1 + bocost)
      newnorm = NegLogDiff(newnorm, norm1 + bocost);
    else newnorm = NegLogDiff(norm1 + bocost, newnorm);
  }
  // NOTE: can we automatically derive an appropriate epsilon?
  if (fabs(newnorm) > norm_eps_ &&  // not normalized
      (bo < 0 || !ReevaluateNormalization(st, bocost, norm, norm1))) {
    VLOG(1) << "State ID: " << st << "; " << fst_.NumArcs(st) <<
      " arcs;" << "  -log(sum(P)) = " << newnorm << ", should be 0";
    VLOG(1) << norm << " " << norm1;
    return false;
  }
  return true;
}

// For accumulated negative log probabilities, a 2nd test for normalization
// Intended for states with very high magnitude backoff cost, which makes 
// previous test unreliable
bool NGramModel::ReevaluateNormalization(StateId st, double bocost, 
					 double norm, double norm1) const {
  double newalpha = CalculateBackoffCost(norm, norm1);
  // NOTE: can we automatically derive an appropriate epsilon?
  VLOG(1) << "Required re-evaluation of normalization: state " << st << " " 
	  << norm << " " << norm1 << " " << newalpha << " " << norm_eps_;
  if (fabs(newalpha - bocost) > norm_eps_)
    return false;
  return true;
}

// Calculates the numerator and denominator for assigning backoff cost
bool NGramModel::CalculateBackoffFactors(double hi_neglog_sum,
					 double low_neglog_sum,
					 double *nlog_backoff_num,
					 double *nlog_backoff_denom,
					 bool infinite_backoff) const {
  if (infinite_backoff && hi_neglog_sum <= kFloatEps)  // unsmoothed and p=1 
    return true;
  if (hi_neglog_sum <= kFloatEps) {  // p=0 for lower order
    hi_neglog_sum = kFloatEps;  // give epsilon probability
    low_neglog_sum = kInfBackoff;  // close to zero mass from lower order also
  } else if (low_neglog_sum <= 0) {
    low_neglog_sum = kFloatEps / 10;  // give lower order mass smaller epsilon
  }
  // if higher order or lower order prob mass is one, unsmoothed
  if (low_neglog_sum <= 0 || hi_neglog_sum < kFloatEps) {
    return true;
  }
  (*nlog_backoff_num) = NegLogDiff(0.0, hi_neglog_sum);
  (*nlog_backoff_denom) = NegLogDiff(0.0, low_neglog_sum);
  return false;
}

// Calculate backoff cost from neglog sums of hi and low order arcs
double NGramModel::CalculateBackoffCost(double hi_neglog_sum,
					double low_neglog_sum, 
					bool infinite_backoff) const {
  double nlog_backoff_num, nlog_backoff_denom;  // backoff cost and factors
  bool return_inf = CalculateBackoffFactors(hi_neglog_sum, low_neglog_sum,
					    &nlog_backoff_num,
					    &nlog_backoff_denom,
					    infinite_backoff);
  if (return_inf)
    return kInfBackoff;  // backoff cost is 'infinite'
  return nlog_backoff_num - nlog_backoff_denom;
}

// Collect backoff arc weights in a vector
void NGramModel::FillBackoffArcWeights(StateId st, StateId bo,
				       vector<double> *bo_arc_weight) const {
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);  // for querying backoff
  matcher.SetState(bo);
  for (ArcIterator<StdFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_)
      continue;
    if (matcher.Find(arc.ilabel)) {
      StdArc barc = matcher.Value();
      bo_arc_weight->push_back(barc.weight.Value());
    } else {
      LOG(FATAL) << "NGramModel: lower order arc missing: " << st;
    }
  }
}

// Mimic a phi matcher: follow backoff arcs until label found or no backoff
bool NGramModel::FindNGramInModel(StateId *mst, int *order, Label label,
				  double *cost) const {
  if (label < 0)
    return 0;
  StateId currstate = (*mst);
  (*cost) = 0;
  (*mst) = -1;
  while ((*mst) < 0) {
    Matcher<StdFst> matcher(fst_, MATCH_INPUT);
    matcher.SetState(currstate);
    if (matcher.Find(label)) {  // arc found out of current state
      StdArc arc = matcher.Value();
      (*order) = state_orders_[currstate];
      (*mst) = arc.nextstate;  // assign destination as new model state
      (*cost) += arc.weight.Value();  // add cost to total
    } else if (matcher.Find(backoff_label_)) {  // follow backoff arc
      currstate = -1;
      for (; !matcher.Done(); matcher.Next()) {
	StdArc arc = matcher.Value();
	if (arc.ilabel == backoff_label_) {
	  currstate = arc.nextstate;  // make current state backoff state
	  (*cost) += arc.weight.Value();  // add in backoff cost
	}
      }
      if (currstate < 0)
	return 0;
    } else {
      return 0;  // Found label in symbol list, but not in model
    }
  }
  return 1;
}

// Returns the unigram cost of requested symbol if found (inf otherwise)
double NGramModel::GetSymbolUnigramCost(Label symbol) const {
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);
  StateId st = unigram_;
  if (st < 0) st = fst_.Start();
  matcher.SetState(st);
  if (matcher.Find(symbol)) {
    StdArc arc = matcher.Value();
    return arc.weight.Value();
  } else {
    return StdArc::Weight::Zero().Value();
  }
}

// Mimic a phi matcher: follow backoff links until final state found
double NGramModel::FinalCostInModel(StateId mst, int *order) const {
  double cost = 0;
  while (fst_.Final(mst) == StdArc::Weight::Zero()) {
    Matcher<StdFst> matcher(fst_, MATCH_INPUT);
    matcher.SetState(mst);
    if (matcher.Find(backoff_label_)) {
      for (; !matcher.Done(); matcher.Next()) {
	StdArc arc = matcher.Value();
	if (arc.ilabel == backoff_label_) {
	  mst = arc.nextstate;  // make current state backoff state
	  cost += arc.weight.Value();  // add in backoff cost
	}
      }
    } else {
      LOG(FATAL) << "NGramModel: No final cost in model: "  << mst;
    }
  }
  (*order) = state_orders_[mst];
  cost += fst_.Final(mst).Value();
  return cost;
}

// Test to see if model came from pre-summing a mixture
// Should have: backoff weights > 0; higher order always higher prob (summed)
bool NGramModel::MixtureConsistent() const {
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);  // for querying backoff
  for (StateId st = 0; st < nstates_; ++st) {
    double bocost;
    StateId bo = GetBackoff(st, &bocost);
    if (bo >= 0) {  // if bigram or higher order
      if (bocost < 0)  // Backoff cost > 0 (can't happen with mixture)
	return 0;
      matcher.SetState(bo);
      for (ArcIterator<StdFst> aiter(fst_, st);
	   !aiter.Done();
	   aiter.Next()) {
	StdArc arc = aiter.Value();
	if (arc.ilabel == backoff_label_)
	  continue;
	if (matcher.Find(arc.ilabel)) {
	  StdArc barc = matcher.Value();
	  if (arc.weight.Value() > barc.weight.Value() + bocost) {
	    return 0;  // L P + (1-L) P' < (1-L) P' (can't happen w/mix)
	  }
	} else {
	  LOG(FATAL) << "NGramModel: lower order arc missing: "  << st;
	}
      }
      if (fst_.Final(st).Value() != StdArc::Weight::Zero().Value() &&
	  fst_.Final(st).Value() > fst_.Final(bo).Value() + bocost)
	return 0;  // final cost doesn't sum
    }
  }
  return 1;
}

// At a given state, calculate the marginal prob p(h) based on
// the smoothed, order-ascending n-gram transition probabilities.
void NGramModel::NGramStateProb(StateId st,
                                vector<double> *probs) const {
  for (ArcIterator<StdFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_) continue;
    if (state_orders_[arc.nextstate] > state_orders_[st]) {
      (*probs)[arc.nextstate] = (*probs)[st] * exp(-arc.weight.Value());
      NGramStateProb(arc.nextstate, probs);
    }
  }
}

// Calculate marginal state probs as the product of the smoothed,
// order-ascending ngram transition probablities: p(abc) =
// p(a)p(b|a)p(c|ba) (odd w/KN)
void NGramModel::NGramStateProbs(vector<double> *probs, bool norm) const {
  probs->clear();
  probs->resize(nstates_, 0.0);
  if (unigram_ < 0) {
    // p(unigram state) = 1
    (*probs)[fst_.Start()] = 1.0;
  } else {
    // p(unigram state) = 1
    (*probs)[unigram_] = 1.0;
    NGramStateProb(unigram_, probs);
    // p(<s>) = p(</s>)
    (*probs)[fst_.Start()] = exp(-fst_.Final(unigram_).Value());
  }
  NGramStateProb(fst_.Start(), probs);

  if (norm) {  // Normalize result, as a starting point for the power method
    double sum = 0.0;
    for (size_t st = 0; st < probs->size(); ++st)
      sum += (*probs)[st];
    for (size_t st = 0; st < probs->size(); ++st)
      (*probs)[st] /= sum;
  }
}

// At a given state, calculate one step of the power method
// for the stationary distribution of the closure of the
// LM with re-entry probability 'alpha'.
void NGramModel::StationaryStateProb(StateId st,
                                     vector<double> *init_probs,
                                     vector<double> *probs,
                                     double alpha) const {
  Matcher<StdFst> matcher(fst_, MATCH_INPUT);  // for querying backoff
  double bocost;
  StateId bo = GetBackoff(st, &bocost);
  if (bo != -1) {
    // Treats backoff like an epsilon transition
    matcher.SetState(bo);
    (*init_probs)[bo] += (*init_probs)[st] * exp(-bocost);
  }

  for (ArcIterator<StdFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_)
      continue;
    (*probs)[arc.nextstate] += (*init_probs)[st] * exp(-arc.weight.Value());
    if (bo != -1 && matcher.Find(arc.ilabel)) {
      // Subtracts corrective weight for backed-off arc
      const StdArc &barc = matcher.Value();
      (*probs)[barc.nextstate] -=
          (*init_probs)[st] * exp(-barc.weight.Value() - bocost);
    }
  }

  if (fst_.Final(st) != Weight::Zero()) {
    (*probs)[fst_.Start()] +=
        (*init_probs)[st] * exp(-fst_.Final(st).Value()) * alpha;
    if (bo != -1) {
      // Subtracts corrective weight for backed-off superfinal arc
      (*probs)[fst_.Start()] -=
          (*init_probs)[st] * exp(-fst_.Final(bo).Value() - bocost) * alpha;
    }
  }
}

// Calculate marginal state probs as the stationary distribution
// of the Markov chain consisting of the closure of the LM
// with re-entry probability 'alpha'. The convergence is controlled
// by 'converge_eps'
void NGramModel::StationaryStateProbs(vector<double> *probs,
                                      double alpha,
                                      double converge_eps) const {
  vector<double> init_probs, last_probs;
  // Initialize based on ngram transition probabilities
  NGramStateProbs(&init_probs, true);
  last_probs = init_probs;

  size_t changed;
  do {
    probs->clear();
    probs->resize(nstates_, 0.0);
    for (int order = hi_order_; order > 0; --order) {
      for (size_t st = 0; st < nstates_; ++st) {
        if (state_orders_[st] == order)
          StationaryStateProb(st, &init_probs, probs, alpha);
      }
    }

    changed = 0;
    for (size_t st = 0; st < nstates_; ++st) {
      if (fabs((*probs)[st] - last_probs[st]) > converge_eps * last_probs[st])
        ++changed;
      last_probs[st] = init_probs[st] = (*probs)[st];
    }
    VLOG(1) << "NGramModel::StationaryStateProbs: state probs changed: "
            << changed;
  } while (changed > 0);
}

// Calculate marginal state probs.  By default, uses the product of
// the order-ascending ngram transition probabilities. If 'stationary'
// is true, instead computes the stationary distribution of the Markov
// chain.
void NGramModel::CalculateStateProbs(vector<double> *probs,
                                     bool stationary) const {
  if (stationary) {
    StationaryStateProbs(probs, .999999, norm_eps_);
  } else {
    NGramStateProbs(probs);
  }
  if (FLAGS_v > 1) {
    for (size_t st = 0; st < probs->size(); ++st)
      std::cerr << "st: " << st << " log_prob: " << log((*probs)[st]) << std::endl;
  }
}

// Estimate total unigram count based on probabilities in unigram state
// The difference between two smallest probs should be 1/N, return reciprocal
double NGramModel::EstimateTotalUnigramCount() const {
  StateId st = UnigramState();
  bool first = true;
  double max = LogArc::Weight::Zero().Value(), nextmax = max;
  if (st < 0) st = GetFst().Start();  // if model unigram, use Start()
  for (ArcIterator<StdFst> aiter(GetFst(), st); !aiter.Done(); aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == BackoffLabel()) continue;
    if (first || arc.weight.Value() > max) {  // maximum negative log prob
      nextmax = max;  // keep both max and nextmax (to calculate diff)
      max = arc.weight.Value();
      first = false;
    } else if (arc.weight.Value() < max && arc.weight.Value() > nextmax) {
      nextmax = arc.weight.Value();
    }
  }
  if (nextmax == LogArc::Weight::Zero().Value()) return exp(max);
  return exp(NegLogDiff(nextmax, max));
}

}  // namespace ngram
