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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file

#include <deque>

#include <fst/arcsort.h>
#include <fst/vector-fst.h>

#include <ngram/ngram-model.h>

namespace ngram {

using std::deque;

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
  if (fst_->Start() == kNoLabel)
    LOG(FATAL) << "NGramModel: Empty automaton";
  uint64 need_props = kAcceptor | kIDeterministic | kILabelSorted;
  uint64 have_props = fst_->Properties(need_props, true);
  if (!(have_props & kAcceptor))
    LOG(FATAL) << "NGramModel: input not an acceptor";
  if (!(have_props & kIDeterministic))
    LOG(FATAL) << "NGramModel: input not deterministic";
  if (!(have_props & kILabelSorted))
    LOG(FATAL) << "NGramModel: input not label sorted";
  if (!fst_->InputSymbols())
    LOG(FATAL) << "NGramModel: no input symbol table";
  if (!fst_->OutputSymbols())
    LOG(FATAL) << "NGramModel: no output symbol table";
  unigram_ = GetBackoff(fst_->Start(), 0);  // set the unigram state
  ComputeStateOrders();
  if (!CheckTopology())
    LOG(FATAL) << "NGramModel: bad ngram model topology";
}

// Calculate and assign backoff cost from neglog sums of hi and low order arcs
void NGramModel::UpdateBackoffCost(StateId st, double hi_neglog_sum,
				    double low_neglog_sum) {
  double alpha;  // backoff cost
  // if higher order or lower order prob mass approximately one, unsmoothed
  if (low_neglog_sum - norm_eps_ <= 0 || hi_neglog_sum - norm_eps_ <= 0) {
    alpha = StdArc::Weight::Zero().Value();  // backoff cost infinite
  } else {
    alpha = -log(1 - exp(-hi_neglog_sum));
    alpha += log(1 - exp(-low_neglog_sum));
  }
  MutableArcIterator<StdMutableFst> aiter(fst_, st);
  if (FindMutableArc(&aiter, backoff_label_)) {
    StdArc arc = aiter.Value();
    arc.weight = alpha;
    aiter.SetValue(arc);
  } else {
    LOG(FATAL) << "NGramModel: No backoff arc found: " << st;
  }
}

// Iterate through arcs, accumulate neglog probs from arcs and their backoffs
void NGramModel::CalcArcNegLogSums(StateId st, StateId bo,
				   double *hi_sum, double *low_sum) const {
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);  // for querying backoff
  matcher.SetState(bo);
  for (ArcIterator<StdMutableFst> aiter(*fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_) continue;
    if (matcher.Find(arc.ilabel)) {
      StdArc barc = matcher.Value();
      (*hi_sum) =  // sum of higher order probs
	NegLogSum((*hi_sum), arc.weight.Value());
      (*low_sum) =  // sum of lower order probs of the same labels
	NegLogSum((*low_sum), barc.weight.Value());
    } else {
      LOG(FATAL) << "NGramModel: No arc label match in backoff state: " << st;
    }
  }
}

// Sum final + arc probs out of state and for same transitions out of backoff
bool NGramModel::CalcBONegLogSums(StateId st, double *hi_neglog_sum,
				  double *low_neglog_sum) const {
  StateId bo = GetBackoff(st, 0);
  if (bo < 0) return false;  // only for states that backoff to another state
  (*low_neglog_sum) = (*hi_neglog_sum) =  // final costs initialize the sum
    fst_->Final(st).Value();
  if ((*hi_neglog_sum) != StdArc::Weight::Zero().Value())  // if st is final
    (*low_neglog_sum) = fst_->Final(bo).Value();  // re-initialize lower sum
  CalcArcNegLogSums(st, bo, hi_neglog_sum, low_neglog_sum);
  return true;
}

// Traverse n-gram fst and record each state's n-gram order, return highest
void NGramModel::ComputeStateOrders() {
  state_orders_.clear();
  state_orders_.resize(fst_->NumStates(), -1);

  if (have_state_ngrams_) {
    state_ngrams_.clear();
    state_ngrams_.resize(fst_->NumStates());
  }

  hi_order_ = 1;  // calculate highest order in the model
  deque<StateId> state_queue;
  if (unigram_ != kNoStateId) {
    state_orders_[unigram_] = 1;
    state_queue.push_back(unigram_);
    state_orders_[fst_->Start()] = hi_order_ = 2;
    state_queue.push_back(fst_->Start());
    if (have_state_ngrams_)
      state_ngrams_[fst_->Start()].push_back(kNoLabel);  // initial context
  } else {
    state_orders_[fst_->Start()] = 1;
    state_queue.push_back(fst_->Start());
  }

  while (!state_queue.empty()) {
    StateId state = state_queue.front();
    state_queue.pop_front();
    for (ArcIterator<Fst<StdArc> > aiter(*fst_, state);
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
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
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
    if (fst_->Final(fst_->Start()) == StdArc::Weight::Zero()) {
      VLOG(1) << "CheckTopology: bad final weight for start state";
      return false;
    } else {
      return true;
    }
  }

  StateId bos = GetBackoff(st, 0);
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);  // for querying backoff

  if (st == unigram_) {  // unigram state
    if (fst_->Final(unigram_) == StdArc::Weight::Zero()) {
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

    if (fst_->Final(st) != StdArc::Weight::Zero() &&
        fst_->Final(bos) == StdArc::Weight::Zero()) {
      VLOG(1) << "CheckTopology: bad final weight for backoff state: " << st;
      return false;
    }
    matcher.SetState(bos);
  }

  for (ArcIterator<StdMutableFst> aiter(*fst_, st);
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
  ostrm << endl;
  return true;
}

// Ensure normalization for a given state to error epsilon
// sum of state probs + exp(-backoff_cost) - sum of arc backoff probs = 1
bool NGramModel::CheckNormalizationState(StateId st) const {
  double Norm = fst_->Final(st).Value(), Norm1 = Norm, bocost;
  StateId bo = GetBackoff(st, &bocost);
  if (Norm != Norm) {  // test for nan
    VLOG(1) << "State ID: " << st << "; " << "final cost nan";
    return false;
  }
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);  // for querying backoff
  if (bo >= 0) {  // if bigram or higher order
    if (Norm != StdArc::Weight::Zero().Value())  // if st is a final state
      Norm1 = bocost + fst_->Final(bo).Value();  // Norm1 = lower order probs
    matcher.SetState(bo);
  }
  for (ArcIterator<StdMutableFst> aiter(*fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.weight.Value() != arc.weight.Value()) {  // test for nan
      VLOG(1) << "State ID: " << st << "; " << "arc cost nan" << endl;
      return false;
    }
    Norm = NegLogSum(Norm, arc.weight.Value());
    if (arc.ilabel != backoff_label_ && bo >= 0) {  // sum lower order prob
      if (matcher.Find(arc.ilabel)) {
	StdArc barc = matcher.Value();
	Norm1 = NegLogSum(Norm1, bocost + barc.weight.Value());
      } else {
	VLOG(1) << "CheckNormalizationState: lower order arc missing";
	return false;
      }
    }
  }
  return EvaluateNormalization(st, bo, Norm, Norm1);  // Normalized?
}

// For accumulated negative log probabilities, test for normalization
bool NGramModel::EvaluateNormalization(StateId st, StateId bo,
			   double norm, double norm1) const {
  if (bo >= 0)
    norm = -log(exp(-norm) - exp(-norm1));  // Subtract prob mass
  // NOTE: can we automatically derive an appropriate epsilon?
  if (fabs(norm) > norm_eps_) {  // insufficiently normalized
    VLOG(1) << "State ID: " << st << "; " << fst_->NumArcs(st) <<
      " arcs;" << "  -log(sum(P)) = " << norm << ", should be 0";
    return false;
  }
  return true;
}

// Scale weights by some factor, for normalizing and use in model merging
void NGramModel::ScaleStateWeight(StateId st, double scale) {
  if (fst_->Final(st) != StdArc::Weight::Zero())
    fst_->SetFinal(st, Times(fst_->Final(st), scale));
  for (MutableArcIterator<StdMutableFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != backoff_label_) {  // only scaling non-backoff arcs
      arc.weight = Times(arc.weight, scale);
      aiter.SetValue(arc);
    }
  }
}

// Collect backoff arc weights in a vector
void NGramModel::FillBackoffArcWeights(StateId st, StateId bo,
				       vector<double> *bo_arc_weight) const {
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);  // for querying backoff
  matcher.SetState(bo);
  for (ArcIterator<StdMutableFst> aiter(*fst_, st);
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
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
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
double NGramModel::GetSymbolUnigramCost(Label symbol) {
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
  StateId st = unigram_;
  if (st < 0) st = fst_->Start();
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
  while (fst_->Final(mst) == StdArc::Weight::Zero()) {
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
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
  cost += fst_->Final(mst).Value();
  return cost;
}

// Scan arcs and remove lower order from arc weight
void NGramModel::UnSumState(StateId st) {
  double bocost;
  StateId bo = GetBackoff(st, &bocost);
  for (MutableArcIterator<StdMutableFst> aiter(fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == backoff_label_)
      continue;
    arc.weight = NegLogDiff(arc.weight.Value(),
			    FindArcWeight(bo, arc.ilabel) + bocost);
    aiter.SetValue(arc);
  }
  if (fst_->Final(st).Value() != StdArc::Weight::Zero().Value())
    fst_->SetFinal(st, NegLogDiff(fst_->Final(st).Value(),
				  fst_->Final(bo).Value() + bocost));
}

// Replace backoff weight with -log p(backoff)
void NGramModel::DeBackoffNGramModel() {
//    if (mixture_consistent) {  // Remove lower order contribution
//  for (int order = hi_order_; order > 1; --order) {  // for each order
//	for (StateId st = 0; st < fst_->NumStates(); ++st)  // scan states
//	  if (state_orders_[st] == order)  // if state is the current order
//	    UnSumState(st);  // Remove lower order prob from arc weights
//  }
//} else {
for (StateId st = 0; st < fst_->NumStates(); ++st) {  // scan states
  double hi_neglog_sum, low_neglog_sum;
  if (CalcBONegLogSums(st, &hi_neglog_sum, &low_neglog_sum)) {
    MutableArcIterator<StdMutableFst> aiter(fst_, st);
    if (FindMutableArc(&aiter, backoff_label_)) {
      StdArc arc = aiter.Value();
      arc.weight = -log(1 - exp(-hi_neglog_sum));
      aiter.SetValue(arc);
    }
    else {
      LOG(FATAL) << "NGramModel: No backoff arc found: "  << st;
    }
  }
}
//    }
}

// Test to see if model came from pre-summing a mixture
// Should have: backoff weights > 0; higher order always higher prob (summed)
bool NGramModel::MixtureConsistent() const {
  Matcher<StdFst> matcher(*fst_, MATCH_INPUT);  // for querying backoff
  for (StateId st = 0; st < fst_->NumStates(); ++st) {
    double bocost;
    StateId bo = GetBackoff(st, &bocost);
    if (bo >= 0) {  // if bigram or higher order
      if (bocost < 0)  // Backoff cost > 0 (can't happen with mixture)
	return 0;
      matcher.SetState(bo);
      for (ArcIterator<StdMutableFst> aiter(*fst_, st);
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
      if (fst_->Final(st).Value() != StdArc::Weight::Zero().Value() &&
	  fst_->Final(st).Value() > fst_->Final(bo).Value() + bocost)
	return 0;  // final cost doesn't sum
    }
  }
  return 1;
}

}  // namespace ngram
