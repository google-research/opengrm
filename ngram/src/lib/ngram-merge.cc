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
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// NGram base model class for merging n-gram FSTs

#include <map>
#include <fst/arcsort.h>
#include <ngram/ngram-merge.h>

namespace ngram {

using std::map;
using fst::StdILabelCompare;

// Perform merger with NGram model specified by the FST argument.
void NGramMerge::MergeNGramModels(const StdFst &infst2, bool norm) {
  delete ngram2_;
  ngram2_ = new NGramModel(infst2, BackoffLabel(), NormEps(),
                                   check_consistency_);
  ngram1_ns_ = GetExpandedFst().NumStates();
  ngram2_ns_ = ngram2_->NumStates();
  MergeWordLists();  // merge symbol tables
  SetupMergeMaps();  // setup state mappings between two FSTs
  MergeFsts();       // combines Fsts; not necessarily normalized
  if (norm) {   // normalize and recalculate backoff weights if required
    NormStates();  // normalization
    RecalcBackoff();  // calculate backoff costs
    if (!CheckNormalization())  // ensure model normalized
      LOG(FATAL) << "NGramMerge: Merged model not fully normalized";
  }
}

// Merging word lists, relabeling arc labels in incoming ngram2
void NGramMerge::MergeWordLists() {
  if (fst::CompatSymbols(GetFst().InputSymbols(),
                         ngram2_->GetFst().InputSymbols(), false))
    return;  // nothing to do; symbol tables match

  if (!GetFst().InputSymbols() || !ngram2_->GetFst().InputSymbols())
    LOG(FATAL) << "NGramMerge: only one LM has symbol tables";

  delete fst2_;
  fst2_ = new StdVectorFst(ngram2_->GetFst());


  NGramMutableModel *mutable_ngram2 =
      new NGramMutableModel(fst2_, BackoffLabel(), NormEps(),
                            check_consistency_);
  delete ngram2_;
  ngram2_ = mutable_ngram2;

  map<int64, int64> symbol_map;  // mapping symbols in symbol lists
  symbol_map[ngram2_->BackoffLabel()] = BackoffLabel();  // ensure bkoff map
  for (StateId st = 0; st < ngram2_ns_; ++st) {
    for (MutableArcIterator<StdMutableFst>
	   aiter(mutable_ngram2->GetMutableFst(), st);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      // find symbol in symbol table
      if (symbol_map.find(arc.ilabel) == symbol_map.end())
	symbol_map[arc.ilabel] =
	  NewWordKey(ngram2_->GetFst().InputSymbols()->Find(arc.ilabel),
		     arc.ilabel);
      if (arc.ilabel != symbol_map[arc.ilabel]) {  // maps to a different idx
	arc.ilabel = arc.olabel = symbol_map[arc.ilabel];  // relabel arc
	aiter.SetValue(arc);
      }
    }
  }
  ArcSort(mutable_ngram2->GetMutableFst(), StdILabelCompare());
}

// Finds word key if in symbol table, otherwise adds (for merging wordlists)
int64 NGramMerge::NewWordKey(string symbol, int64 key2) {
  int64 key1 = GetMutableFst()->InputSymbols()->Find(symbol);
  if (key1 < 0) {  // Returns key2 if free, o.w. next available key.
    key1 = GetMutableFst()->InputSymbols()->Find(key2).empty()
      ? key2 : GetMutableFst()->InputSymbols()->AvailableKey();
    GetMutableFst()->MutableInputSymbols()->AddSymbol(symbol, key1);
    GetMutableFst()->MutableOutputSymbols()->AddSymbol(symbol, key1);
  }                // ... else returns existing key
  return key1;
}

// Maps states representing identical n-gram histories
void NGramMerge::SetupMergeMaps() {
  // Initializes the maps between models
  bool first_setup = exact_map_1to2_.empty();
  bool bo_from1 = MergeUnshared(true);

  if (first_setup) {
    for (StateId j = 0; j < ngram1_ns_; ++j)
      exact_map_1to2_.push_back(-1);
  } else {
    for (StateId ist = 0; ist < exact_map_2to1_.size(); ++ist)
      exact_map_1to2_[exact_map_2to1_[ist]] = -1;
  }

  if (bo_from1) {
    backoff_map_1to2_.clear();
    for (StateId j = 0; j < ngram1_ns_; ++j) {
      backoff_map_1to2_.push_back(-1);
    }
  }

  exact_map_2to1_.clear();
  backoff_map_2to1_.clear();
  for (StateId j = 0; j < ngram2_ns_; ++j) {
    exact_map_2to1_.push_back(-1);
    backoff_map_2to1_.push_back(-1);
  }

  StateId start1 = GetFst().Start();
  StateId start2 = ngram2_->GetFst().Start();
  StateId unigram1 = UnigramState();
  StateId unigram2 = ngram2_->UnigramState();

  if (unigram2 != kNoStateId) {
    if (unigram1 != kNoStateId) {       // merging two order > 1
      MergeStateMaps(unigram1, unigram2, true, bo_from1, true);
      MergeStateMaps(start1, start2, true, bo_from1, true);
    } else {                            // merging order > 1 into unigram
      MergeStateMaps(start1, unigram2, true, bo_from1, true);
      MergeStateMaps(start1, start2, false, false, true);
    }
  } else if (unigram1 != kNoStateId) {  // merging unigram into order > 1
    MergeStateMaps(unigram1, start2, true, bo_from1, true);
    MergeStateMaps(start1, start2, false, bo_from1, false);
  } else {                              // merging two unigrams
    MergeStateMaps(start1, start2, true, bo_from1, true);
  }

  if (!bo_from1 && first_setup)
    MergeBackedOffToMap();
}

// Creates a state map from ngram2 to equivalent states in ngram1
void NGramMerge::MergeExactStateMap(StateId st, StateId ist) {
  exact_map_1to2_[st] = ist;  // collect source state for state match
  exact_map_2to1_[ist] = st;  // collect target state for state match

  Matcher<StdFst> matcher(GetFst(), MATCH_INPUT);
  matcher.SetState(st);
  for (ArcIterator<StdFst> biter(ngram2_->GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    if (barc.ilabel == BackoffLabel()) continue;
    if (matcher.Find(barc.ilabel)) {  // found match in ngram1
      StdArc arc = matcher.Value();
      if (StateOrder(arc.nextstate) >
	  StateOrder(st) &&
	  ngram2_->StateOrder(barc.nextstate) >
	  ngram2_->StateOrder(ist))  // if both next states higher order
	MergeExactStateMap(arc.nextstate, barc.nextstate);  // map those too
    }
  }
}

// Creates a map from a state in one ngram model to closest backed-off
// context in other model
void NGramMerge::MergeBackoffStateMap(StateId st, StateId ist, bool from1) {
  // ngram1 or ngram2
  const NGramModel *ngram = from1 ? this : ngram2_;
  UpdateBackoffMap(st, ist, from1);  // update state map

  for (ArcIterator<StdFst> biter(ngram->GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    StdArc barc = biter.Value();
    if (barc.ilabel == BackoffLabel() ||
	ngram->StateOrder(barc.nextstate) <= ngram->StateOrder(ist))
      continue;
    MergeBackoffStateMap(MergeBackoffDest(st, barc.ilabel, from1, 0),
                         barc.nextstate, from1);
  }
}

// Creates a map from state s and label l in ngram1, with state s
// having label l on an outgoing arc to destination d, to the set of
// states s', backing off to s, that also have an arc labeled with l
// and going to destination d. Computed only for non-ascending arcs.
void NGramMerge::MergeBackedOffToMap() {
  for (StateId st = 0; st < ngram1_ns_; ++st) {
    StateId bo = GetBackoff(st, 0);
    if (bo < 0)
      continue;
    for (ArcIterator<StdFst> aiter(GetFst(), st);
         !aiter.Done(); aiter.Next()) {
      const StdArc &arc = aiter.Value();
      if (arc.ilabel == BackoffLabel())
        continue;
      if (StateOrder(st) > StateOrder(arc.nextstate)) {
        pair<StateId, Label> pr(bo, arc.ilabel);
        backed_off_to_.insert(make_pair(pr, st));
      }
    }
  }
}

// Combines Fsts; not necessarily normalized. Virtual allows
// e.g. model to merge states of different orders.
void NGramMerge::MergeFsts() {
  bool merge_unshared1 = MergeUnshared(true);

  // Adds all states from ngram2 not in ngram1
  for (StateId ist = 0; ist < ngram2_ns_; ++ist) {  // all states in ngram2
    if (exact_map_2to1_[ist] < 0) {  // no matching state in ngram1
      StateId st = GetMutableFst()->AddState();
      UpdateState(st, ngram2_->StateOrder(ist), false,
                  check_consistency_ ? &(ngram2_->StateNGram(ist)) : 0);
      exact_map_1to2_.push_back(ist);
      exact_map_2to1_[ist] = st;
      if (merge_unshared1)
        backoff_map_1to2_.push_back(-1);
    }
  }

  // If merging non-unigram into unigram, updates start and unigram info.
  if (UnigramState() == kNoStateId && ngram2_->UnigramState() != kNoStateId) {
    StateId new_start = exact_map_2to1_[ngram2_->GetFst().Start()];
    StateId new_unigram = exact_map_2to1_[ngram2_->UnigramState()];
    GetMutableFst()->SetStart(new_start);
    UpdateState(new_unigram, 1, true,
                check_consistency_ ? &StateNGram(new_unigram) : 0);
  }

  // Merges arcs from shared and unshared states
  set<Label> shared;                // shared arcs between st and ist
  for (int order = HiOrder(); order > 0; --order) {
    for (StateId ist = 0; ist < ngram2_ns_; ++ist) {
      if (ngram2_->StateOrder(ist) == order) {
        StateId st = exact_map_2to1_[ist];
        shared.clear();
        if (st < ngram1_ns_) {                    // shared state
          MergeSharedArcs(st, ist, &shared);      // n-grams in both ngram1,2
          if (merge_unshared1)
            MergeUnsharedArcs1(st, ist, shared);  // n-grams just in ngram1
        }
        MergeUnsharedArcs2(st, ist, shared);      // n-grams just in ngram2
      }
    }
    if (merge_unshared1) {
      shared.clear();
      for (StateId st = 0; st < ngram1_ns_; ++st) {
        if (StateOrder(st) == order && exact_map_1to2_[st] < 0)
          MergeUnsharedArcs1(st, -1, shared);
      }
    }
  }
}

// For n-gram arcs shared in common, combines weight, sets correct destination
void  NGramMerge::MergeSharedArcs(StateId st, StateId ist,
				  set<Label> *shared) {
  MutableArcIterator<StdMutableFst> aiter(GetMutableFst(), st);
  if (!aiter.Done()) {
    StdArc arc = aiter.Value();
    for (ArcIterator<StdFst> biter(ngram2_->GetFst(), ist);
	 !biter.Done();
	 biter.Next()) {
      const StdArc &barc = biter.Value();
      // Can't use matcher for Mutable fst (full copy made), use iterator
      if (FindMutableArc(&aiter, barc.ilabel)) {   // found in ngram1
	arc = aiter.Value();
	if (barc.ilabel != BackoffLabel() && StateOrder(arc.nextstate)
	    < ngram2_->StateOrder(barc.nextstate)) {  // needs new destination
	  if (!MergeUnshared(true)) {
	    MergeDests1(st, arc.ilabel, arc.nextstate,
			exact_map_2to1_[barc.nextstate]);
	  }
	  arc.nextstate = exact_map_2to1_[barc.nextstate];
	}
	shared->insert(arc.ilabel);  // marks word shared
	arc.weight = MergeWeights(st, ist, arc.ilabel, arc.weight.Value(),
				  barc.weight.Value(), true, true);
	aiter.SetValue(arc);
      }
    }
  }

  // Superfinal arc
  Weight final1 =  GetFst().Final(st);
  Weight final2 =  ngram2_->GetFst().Final(ist);
  if (final1 != Weight::Zero() && final2 != Weight::Zero()) {
    Weight merge_final = MergeWeights(st, ist, kNoLabel, final1.Value(),
				      final2.Value(), true, true);
    GetMutableFst()->SetFinal(st, merge_final);
    shared->insert(kNoLabel);      // marks superfinal word shared
  }
}

// Merges n-gram arcs not found in the new model
// Applies when MergeUnshared(true) is true.
void  NGramMerge::MergeUnsharedArcs1(StateId st, StateId ist,
                                     const set<Label> &shared) {
  StateId bst = backoff_map_1to2_[st];
  for (MutableArcIterator<StdMutableFst> aiter(GetMutableFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    double cost = Weight::Zero().Value();
    if (shared.count(arc.ilabel) == 0) {  // not found
      if (arc.ilabel != BackoffLabel()) {
        StateId dest = MergeBackoffDest(bst, arc.ilabel, true, &cost);
        if (ngram2_->StateOrder(dest) > StateOrder(arc.nextstate))
          arc.nextstate = exact_map_2to1_[dest];  // needs a new destination
      }
      arc.weight =
	MergeWeights(st, bst, arc.ilabel, arc.weight.Value(),
		     cost, true, false);
      aiter.SetValue(arc);
    }
  }

  // Superfinal arc
  if (shared.count(kNoLabel) == 0) {  // not found
    Weight final1 =  GetFst().Final(st);
    if (final1 != Weight::Zero()) {
      int order;
      double cost = ngram2_->FinalCostInModel(bst, &order);
      Weight merge_final = MergeWeights(st, bst, kNoLabel, final1.Value(),
					cost, true, false);
      GetMutableFst()->SetFinal(st, merge_final);
    }
  }
}

// Merges n-gram arcs not found in the original model
void  NGramMerge::MergeUnsharedArcs2(StateId st, StateId ist,
                                     const set<Label> &shared) {
  StateId bst = backoff_map_2to1_[ist];
  StateId ibo = ngram2_->GetBackoff(ist, 0);
  StateId bo = ibo >= 0 ? exact_map_2to1_[ibo] : -1;
  bool arcsort = false;
  for (ArcIterator<StdFst> biter(ngram2_->GetFst(), ist);
       !biter.Done();
       biter.Next()) {
    const StdArc &barc = biter.Value();
    double cost = Weight::Zero().Value();
    if (shared.count(barc.ilabel) == 0) {  // not found
      StdArc arc = barc;
      arc.nextstate = exact_map_2to1_[barc.nextstate];
      if (barc.ilabel != BackoffLabel()) {
        StateId dest = MergeBackoffDest(bst, barc.ilabel, false, &cost);
        if (StateOrder(dest) > ngram2_->StateOrder(barc.nextstate))
          arc.nextstate = dest;  // needs a new destination state
        if (!MergeUnshared(true) && bo >= 0 &&
            StateOrder(st) > StateOrder(arc.nextstate)) {
          pair<StateId, Label> pr(bo, arc.ilabel);
          backed_off_to_.insert(make_pair(pr, st));
        }
      }
      arc.weight = MergeWeights(bst, ist, arc.ilabel, cost,
				arc.weight.Value(), false, true);
      GetMutableFst()->AddArc(st, arc);
      arcsort = true;
    }
  }

  if (arcsort)
    SortArcs(st);

  if (shared.count(kNoLabel) == 0) {  // not found
    Weight final2 =  ngram2_->GetFst().Final(ist);
    if (final2 != Weight::Zero()) {
      int order;
      double cost = FinalCostInModel(bst, &order);
      Weight merge_final = MergeWeights(bst, ist, kNoLabel, cost,
					final2.Value(), false, true);
      GetMutableFst()->SetFinal(st, merge_final);
    }
  }
}

// Merges n-gram arcs not found in the new model by (solely) performing any
// necessary destination state changes from 'old_dest' to 'new_dest'.
// Looks up arcs that backoff to state 'low_src' and through label 'label'
// to 'old_dest'. Applies when MergeUnshared(true) is false.
void NGramMerge::MergeDests1(StateId low_src, Label label,
                             StateId old_dest, StateId new_dest) {
  pair<StateId, Label> pr(low_src, label);
  multimap< pair<StateId, Label>, StateId>::iterator it =
    backed_off_to_.find(pr);

  bool non_ascending = StateOrder(low_src) >= StateOrder(new_dest);

  while (it != backed_off_to_.end() && it->first == pr) {
    // Is backed_off_to_ entry still needed when we're done here?
    bool needed = non_ascending;
    StateId hi_src = it->second;
    MutableArcIterator<StdMutableFst> aiter(GetMutableFst(), hi_src);
    CHECK(FindMutableArc(&aiter, label));
    StdArc arc = aiter.Value();

    if (arc.nextstate == old_dest) {  // updates the destination state
      MergeDests1(hi_src, label, old_dest, new_dest);
      arc.nextstate = new_dest;
      aiter.SetValue(arc);
    } else if (arc.nextstate != new_dest) {
      needed = false;                 // no longer shares a destination state
    }

    if (needed) {
      ++it;
    } else {
      backed_off_to_.erase(it++);
    }
  }
}

// Finds the destination state with label from a backed-off model (assign cost)
NGramMerge::StateId
NGramMerge::MergeBackoffDest(StateId st, Label label,
                             bool from1, double *cost) {
  // ngram1 or ngram2
  const NGramModel *ngram = from1 ? ngram2_ : this;
  if (st < 0)
    LOG(FATAL) << "MergeBackoffDest: bad state: " << st;
  if (cost != 0)
    *cost = 0.0;
  Matcher<StdFst> matcher(ngram->GetFst(), MATCH_INPUT);
  matcher.SetState(st);
  while (!matcher.Find(label)) {  // while no match found
    double thiscost;
    st = ngram->GetBackoff(st, &thiscost);
    if (st < 0) {
      if (cost != 0)
	(*cost) = StdArc::Weight::Zero().Value();
      return ngram->UnigramState() < 0 ? ngram->GetFst().Start()
	: ngram->UnigramState();
    }
    if (cost != 0)
      (*cost) += thiscost;
    matcher.SetState(st);
  }
  if (cost != 0)
    (*cost) += matcher.Value().weight.Value();
  return matcher.Value().nextstate;
}

// Calculates correct normalization constant for each state and normalize
void NGramMerge::NormStates() {
  for (StateId ist = 0; ist < ngram2_ns_; ++ist) {
    StateId st = exact_map_2to1_[ist];
    if (st < ngram1_ns_) { 	            // state found in both models
      NormState(st, true, true);
    } else if (MergeUnshared(false)) {      // state only found in ngram2
      NormState(st, false, true);
    }
  }

  for (StateId st = 0; st < ngram1_ns_; ++st) {
    if (exact_map_1to2_[st] < 0)       // state only found in ngram1
      NormState(st, true, false);
  }
}

// Applies normalization constant to arcs and final cost at state.
void NGramMerge::NormState(StateId st, bool in_fst1, bool in_fst2) {
  double norm = NormWeight(in_fst1, in_fst2);
  GetMutableFst()->SetFinal(st, Times(GetFst().Final(st),
  					       norm));
  for (MutableArcIterator<StdMutableFst>
  	 aiter(GetMutableFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != BackoffLabel()) {
      arc.weight = Times(arc.weight, norm);
      aiter.SetValue(arc);
    }
  }
}

}  // namespace ngram
