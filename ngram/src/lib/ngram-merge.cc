// ngram-merge.cc
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

#include <fst/arcsort.h>
#include <ngram/ngram-merge.h>

namespace ngram {

using fst::StdILabelCompare;


// Construct an NGramMerge object, consisting of two ngram models
// and their scaling weights, as well as other info that informs the
// merger.  The larger of the FSTs is modified by default.
// Ownership of FSTs is retained by the caller.

// NB: the self model (*this) is the merged model and is referred to as
// 'ngram1' below. The mixed-in model is referred to as 'ngram2'.
// 'infst1' and 'infst2' might first be swapped based on their
// relative sizes and the 'fixed_order' option.
NGramMerge::NGramMerge(StdMutableFst *infst1,
		      StdMutableFst*infst2,
		       double alpha, double beta, bool norm,
		       bool use_smoothing, bool fixed_order,
		       Label backoff_label, double norm_eps,
                       bool check_consistency)
  : NGramModel(Swap(*infst1, *infst2, fixed_order) ? infst2 : infst1,
	       backoff_label, norm_eps, check_consistency),
    ngram2_(Swap(*infst1, *infst2, fixed_order) ? infst1 : infst2,
	    backoff_label, norm_eps, check_consistency),
    alpha_(Swap(*infst1, *infst2, fixed_order) ? beta : alpha),
    beta_(Swap(*infst1, *infst2, fixed_order) ? alpha : beta),
    norm_(norm),
    use_smoothing_(use_smoothing) {

  if (use_smoothing && (!CheckNormalization() ||
			!ngram2_.CheckNormalization()))
    LOG(FATAL) << "NGramMerge: Models must be normalized to"
	       << " use smoothing in merging";
  ngram1_ns_ = GetFst().NumStates();  // orig. NumStates of ngram1
  ngram2_ns_ = ngram2_.GetFst().NumStates();  // orig. NumStates of ngram2
  backoff_label_ = BackoffLabel();  // ngram1 backoff_label_ as new
  for (StateId j = 0; j < ngram2_ns_; ++j)  // vector for storing state mapping
    new_states_.push_back(-1);
  if (use_smoothing) {  // need vectors for storing smoothed state mappings
    for (StateId j = 0; j < ngram2_ns_; ++j)
      smooth_map_2to1_.push_back(-1);  // smoothed state map from 2 to 1
    for (StateId j = 0; j < ngram1_ns_; ++j)
      smooth_map_1to2_.push_back(-1);  // smoothed state map from 1 to 2
  }
}

// Incorporate n-gram model ngram2 into ngram1, scaling with weights
void NGramMerge::MergeNGramModels() {
  ScaleWeights(alpha_);  // scale counts/probs in ngram1
  ngram2_.ScaleWeights(beta_);  // scale counts/probs in ngram2
  MergeWordList();  // merge symbol tables
  MergeAddStates();  // add new states and backoff arcs to ngram1 as needed
  // TODO BER: option to use model backoffs in combining weights (model mix)
  for (StateId st = 0; st < ngram2_ns_; ++st)  // add new arcs, upd. weights
    MergeState(st);
  ArcSort(GetMutableFst(), StdILabelCompare());
  Connect(GetMutableFst());  // ensure resulting dests are coherent
  InitModel();  // calculate state info
  if (norm_) {  // normalize and recalculate backoff weights if required
    NormStates();  // normalization
    RecalcBackoff();  // calculate backoff costs
    if (!CheckNormalization())  // ensure model normalized
      LOG(FATAL) << "NGramMerge: Merged model not fully normalized";
  }
}

// Merging word lists, relabeling arc labels in incoming ngram2
void NGramMerge::MergeWordList() {
  vector<int64> symbol_map;  // mapping symbols in symbol lists
  for (int64 j = 0; j < ngram2_.GetFst().InputSymbols()->NumSymbols(); ++j)
    symbol_map.push_back(-1);
  symbol_map[ngram2_.BackoffLabel()] = backoff_label_;  // ensure bkoff map
  for (StateId st = 0; st < ngram2_ns_; ++st) {
    for (MutableArcIterator<StdMutableFst>
	   aiter(ngram2_.GetMutableFst(), st);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      if (symbol_map[arc.ilabel] < 0)  // find symbol in symbol table
	symbol_map[arc.ilabel] =
	  NewWordKey(ngram2_.GetFst().InputSymbols()->Find(arc.ilabel));
      if (arc.ilabel != symbol_map[arc.ilabel]) {  // maps to a different idx
	arc.ilabel = arc.olabel = symbol_map[arc.ilabel];  // relabel arc
	aiter.SetValue(arc);
      }
    }
  }
  ArcSort(ngram2_.GetMutableFst(), StdILabelCompare());
}

// Map states representing identical n-gram histories, add remaining states
void NGramMerge::MergeAddStates() {
  bool bothstarts = true;  // true if Start() states represent same history
  if (ngram2_.UnigramState() != kNoStateId) {  // incoming fst order > 1

    if (UnigramState() == kNoStateId) {
      // merging order > 1 into unigram:
      // unigrams
      MergeStateMaps(GetFst().Start(), ngram2_.UnigramState());
      // no merge from both Start() states (history mismatch)
      bothstarts = false;
    }
    else {
      // merging two n-gram models of order > 1:
      // unigram states
      MergeStateMaps(UnigramState(), ngram2_.UnigramState());
    }
  }
  else if (UnigramState() != kNoStateId) {
    // merge unigram into order > 1:
    // unigrams
    MergeStateMaps(UnigramState(), ngram2_.GetFst().Start());
    bothstarts = false;  // no merge from both Start() states (hist. mismatch)
  }
  if (bothstarts)  //  Merging from both start states, Start() history match
    MergeStateMaps(GetFst().Start(), ngram2_.GetFst().Start());
  if (use_smoothing_) {
    MergeSmoothUnsharedArcs();
  }
  MergeCreateStatesAndBackoffs();  // Add states from ngram2 not in ngram1
}

// First add all states from ngram2 not in ngram1, then add backoff arcs
void NGramMerge::MergeCreateStatesAndBackoffs() {
  vector<StateId> add_st, boff_st;
  for (StateId st = 0; st < ngram2_ns_; ++st) {  // all states in ngram2
    if (new_states_[st] < 0) {  // no matching state in ngram1
      new_states_[st] = GetMutableFst()->AddState();
      add_st.push_back(st);
      boff_st.push_back(ngram2_.GetBackoff(st, 0));
    }
  }
  for (StateId i = 0; i < add_st.size(); ++i)  // add new state backoffs
    GetMutableFst()->AddArc(new_states_[add_st[i]],
			  StdArc(0, 0, StdArc::Weight::Zero(),
				 new_states_[boff_st[i]]));
}

// For n-gram arcs shared in common, combine weight, set correct destination
void  NGramMerge::MergeSharedArcs(StateId st, StateId ist,
				  vector<bool> *arc_upd) {
  MutableArcIterator<StdMutableFst> aiter(GetMutableFst(),
						 st);
  StdArc arc = aiter.Value();
  for (size_t arccnt = 0; arccnt < ngram2_.GetFst().NumArcs(ist); ++arccnt)
    arc_upd->push_back(false);
  size_t arccnt = 0;
  for (ArcIterator<StdExpandedFst> biter(ngram2_.GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    // Can't use matcher for Mutable fst (full copy made), use iterator
    if (barc.ilabel != backoff_label_ &&  // ignore backoff arc
	FindMutableArc(&aiter, barc.ilabel)) {  // found in ngram1
      arc = aiter.Value();
      if (StateOrder(arc.nextstate) <
	  ngram2_.StateOrder(barc.nextstate))  // needs a new destination
	arc.nextstate = new_states_[barc.nextstate];
      arc.weight =  // combine weights from both arcs
	NegLogSum(arc.weight.Value(), barc.weight.Value());
      aiter.SetValue(arc);
      (*arc_upd)[arccnt] = true;  // remember that arc weight was found
    }
    ++arccnt;
  }
}

// For a state and its smoothed state, combine probs for unshared arcs
void NGramMerge::MergeSmoothUnsharedArcState(StateId ist, StateId st,
					     bool exact, bool from1) {
  NGramModel *ngram = from1 ? this : &ngram2_;  // ngram1 or ngram2
  Matcher<StdFst> matcher(from1 ? ngram2_.GetFst()
			  : GetFst(), MATCH_INPUT);
  matcher.SetState(st);
  double cost;
  for (MutableArcIterator<StdMutableFst> biter(ngram->GetMutableFst(),
						      ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    if (barc.ilabel == backoff_label_) continue;
    if (!matcher.Find(barc.ilabel) || !exact) {  // Unshared arc
      cost = 0;
      MergeSmoothDest(st, barc.ilabel, from1, &cost);
      if (cost != StdArc::Weight::Zero().Value()) {
	barc.weight = NegLogSum(barc.weight.Value(), cost);
	biter.SetValue(barc);
      }
    }
  }
}

// Add n-gram arcs not found in the original model
void  NGramMerge::MergeNewArcs(StateId st, StateId ist,
			       vector<bool> *arc_upd) {
  size_t arccnt = 0;
  for (ArcIterator<StdExpandedFst> biter(ngram2_.GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    if (barc.ilabel != backoff_label_ && !(*arc_upd)[arccnt])  // not found
      GetMutableFst()->AddArc(st,
				       StdArc(barc.ilabel, barc.ilabel,
					      barc.weight,
					      new_states_[barc.nextstate]));
    ++arccnt;
  }
}

// For a given state, merge the shared arcs, add any remaining arcs req'd
void NGramMerge::MergeState(StateId ist) {
  StateId st = new_states_[ist];
  GetMutableFst()->SetFinal(st,
				     NegLogSum(
				      GetFst().Final(st).Value(),
				      ngram2_.GetFst().Final(ist).Value()));
  vector<bool> arc_upd;  // updated arc already?
  MergeSharedArcs(st, ist, &arc_upd);  // n-grams in both ngram1 and ngram2
  // Use smoothed estimate for existing arcs that were not
  MergeNewArcs(st, ist, &arc_upd);  // new n-grams just in ngram2
}

// Create a state map from ngram2 to equivalent states in ngram1
void NGramMerge::MergeStateMap(StateId st, StateId ist) {
  new_states_[ist] = st;  // collect target state for state match
  Matcher<StdFst> matcher(GetFst(), MATCH_INPUT);
  matcher.SetState(st);
  for (ArcIterator<StdExpandedFst> biter(ngram2_.GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    if (barc.ilabel == backoff_label_) continue;
    if (matcher.Find(barc.ilabel)) {  // found match in ngram1
      StdArc arc = matcher.Value();
      if (StateOrder(arc.nextstate) >
	  StateOrder(st) &&
	  ngram2_.StateOrder(barc.nextstate) >
	  ngram2_.StateOrder(ist))  // if both next states higher order
	MergeStateMap(arc.nextstate, barc.nextstate);  // map those as well
    }
  }
}

// Create a map from states in one ngram model to smoothed states in other
void NGramMerge::MergeSmoothStateMap(StateId st, StateId ist, bool from1) {
  NGramModel *ngram = from1 ? this : &ngram2_;  // ngram1 or ngram2
  UpdateSmoothMap(st, ist, from1);  // update state map

  for (ArcIterator<StdExpandedFst> biter(ngram->GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    if (barc.ilabel == backoff_label_ ||
	ngram->StateOrder(barc.nextstate) <= ngram->StateOrder(ist))
      continue;
    MergeSmoothStateMap(MergeSmoothDest(st, barc.ilabel, from1, 0),
			barc.nextstate, from1);
  }
}

// Find the destination state with label from a smoothed model (assign cost)
NGramMerge::StateId NGramMerge::MergeSmoothDest(StateId st, Label label,
						bool from1, double *cost) {
  NGramModel *ngram = from1 ? &ngram2_ : this;  // ngram1 or ngram2
  Matcher<StdFst> matcher(ngram->GetFst(), MATCH_INPUT);
  matcher.SetState(st);
  while (!matcher.Find(label)) {  // while no match found
    double thiscost;
    st = ngram->GetBackoff(st, &thiscost);
    if (cost != 0)
      (*cost) += thiscost;
    if (st < 0) {
      if (cost != 0)
	(*cost) = StdArc::Weight::Zero().Value();
      return ngram->UnigramState() < 0 ? ngram->GetFst().Start()
	: ngram->UnigramState();
    }
    matcher.SetState(st);
  }
  if (cost != 0)
    (*cost) += matcher.Value().weight.Value();
  return matcher.Value().nextstate;
}

// Apply normalization constant to arcs and final cost at state
void NGramMerge::NormState(StateId st, double norm) {
  GetMutableFst()->SetFinal(st, Times(GetFst().Final(st),
					       norm));
  for (MutableArcIterator<StdMutableFst>
	 aiter(GetMutableFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != backoff_label_) {
      arc.weight = Times(arc.weight, norm);
      aiter.SetValue(arc);
    }
  }
}

// Calculate correct normalization constant for each state and normalize
void NGramMerge::NormStates() {
  vector<bool> combined_state;  // which states in model are shared
  for (StateId st = 0; st < ngram1_ns_; ++st)  // orig. states in ngram1
    combined_state.push_back(0);
  for (StateId st = 0; st < ngram2_ns_; ++st) {  // orig. states in ngram2
    if (new_states_[st] < ngram1_ns_)  // mapped state in ngram1
      combined_state[new_states_[st]] = true;
  }
  double Norm1 = -beta_, Norm2 = -alpha_,
    CombinedNorm = -NegLogSum(-Norm1, -Norm2);
  if (use_smoothing_)  // When using smoothing, all states combined norm
    Norm1 = Norm2 = CombinedNorm;
  for (StateId st = 0; st < GetFst().NumStates(); ++st) {
    if (st >= ngram1_ns_) {  // state was only found in ngram2
      NormState(st, Norm1);  // normalize by the ngram2 scaling factor
    } else if (combined_state[st]) {  // if state was found in both models
      NormState(st, CombinedNorm);  // use combined normalization constant
    } else {  // state was only found in ngram1
      NormState(st, Norm2);  // normalize by the ngram1 scaling factor
    }
  }
}

}  // namespace ngram
