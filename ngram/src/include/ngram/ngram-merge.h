
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
// NGram base model class for merging n-gram FSTs.

#ifndef NGRAM_NGRAM_MERGE_H_
#define NGRAM_NGRAM_MERGE_H_

#include <map>
#include <set>

#include <fst/arcsort.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-mutable-model.h>

namespace ngram {

using fst::VectorFst;
using std::set;

using std::map;
using std::multimap;

using fst::ILabelCompare;

template <class Arc>
class NGramMerge : public NGramMutableModel<Arc> {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  using NGramModel<Arc>::Error;
  using NGramModel<Arc>::SetError;
  using NGramMutableModel<Arc>::GetFst;
  using NGramMutableModel<Arc>::GetMutableFst;
  using NGramMutableModel<Arc>::GetExpandedFst;
  using NGramMutableModel<Arc>::BackoffLabel;
  using NGramMutableModel<Arc>::UnigramState;
  using NGramMutableModel<Arc>::NormEps;
  using NGramMutableModel<Arc>::CheckNormalization;
  using NGramMutableModel<Arc>::HiOrder;
  using NGramMutableModel<Arc>::RecalcBackoff;
  using NGramMutableModel<Arc>::GetBackoff;
  using NGramMutableModel<Arc>::FindMutableArc;
  using NGramMutableModel<Arc>::StateOrder;
  using NGramMutableModel<Arc>::StateNGram;
  using NGramMutableModel<Arc>::SortArcs;
  using NGramMutableModel<Arc>::UpdateState;
  using NGramMutableModel<Arc>::FinalCostInModel;
  using NGramMutableModel<Arc>::ScaleWeight;
  using NGramMutableModel<Arc>::SetAllowInfiniteBO;
  using NGramMutableModel<Arc>::ScalarValue;

  // Constructs an NGramMerge object consisting of ngram model to be merged.
  // Ownership of FST is retained by the caller.
  explicit NGramMerge(MutableFst<Arc> *infst1, Label backoff_label = 0,
                      double norm_eps = kNormEps,
                      bool check_consistency = false)
      : NGramMutableModel<Arc>(infst1, backoff_label, norm_eps,
                               check_consistency),
        check_consistency_(check_consistency) {
    // set switch if inf backoff costs
    NGramMutableModel<Arc>::SetAllowInfiniteBO();
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

  // Perform merger with NGram model specified by the FST argument.
  bool MergeNGramModels(const Fst<Arc> &infst2, bool norm = false) {
    ngram2_.reset(new NGramModel<Arc>(infst2, BackoffLabel(), NormEps(),
                                      check_consistency_));
    if (ngram2_->Error()) return false;
    ngram1_ns_ = GetExpandedFst().NumStates();
    ngram2_ns_ = ngram2_->NumStates();
    if (!MergeWordLists()) return false;  // merge symbol tables
    SetupMergeMaps();   // setup state mappings between two FSTs
    MergeFsts();        // combines Fsts; not necessarily normalized
    if (Error()) return false;
    if (norm) {         // normalize and recalculate backoff weights if required
      NormStates();     // normalization
      RecalcBackoff();  // calculate backoff costs
      if (Error()) return false;
      if (!CheckNormalization()) {  // ensure model normalized
        NGRAMERROR() << "NGramMerge: Merged model not fully normalized";
        SetError();
        return false;
      }
    }
    return true;
  }

  // Specifies the resultant weight when combining a weight from each
  // FST.  If the weight comes from a shared arc (or final weight),
  // then in_fst1 and in_fst2, are true. O.w., if the arc comes from
  // the first FST, then 'in_fst1' is true, 'w1' is the arc weight,
  // and 'w2' is the backed-off weight to the matching arc on the
  // second FST from backed-off state 's2'. Similarly if the arc comes
  // from the second FST.
  virtual Weight MergeWeights(StateId s1, StateId s2, Label label, Weight w1,
                              Weight w2, bool in_fst1, bool in_fst2) const = 0;

  // Specifies the normalization constant per state 'st' depending whether the
  // state was present in one or boths FSTs.
  virtual double NormWeight(StateId st, bool in_fst1, bool in_fst2) const {
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
  const NGramModel<Arc> &NGram2() const { return *ngram2_; }

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
  // Merges word lists, relabeling arc labels in incoming ngram2.
  // Returns false on error.
  bool MergeWordLists() {
    if (fst::CompatSymbols(GetFst().InputSymbols(),
                               ngram2_->GetFst().InputSymbols(), false))
      return true;  // nothing to do; symbol tables match

    if (!GetFst().InputSymbols() || !ngram2_->GetFst().InputSymbols()) {
      NGRAMERROR() << "NGramMerge: only one LM has symbol tables";
      SetError();
      return false;
    }

    fst2_.reset(new VectorFst<Arc>(ngram2_->GetFst()));

    std::unique_ptr<NGramMutableModel<Arc>> mutable_ngram2(
        new NGramMutableModel<Arc>(fst2_.get(), BackoffLabel(), NormEps(),
                                   check_consistency_));

    map<int64, int64> symbol_map;  // mapping symbols in symbol lists
    symbol_map[mutable_ngram2->BackoffLabel()] = BackoffLabel();
    for (StateId st = 0; st < ngram2_ns_; ++st) {
      for (MutableArcIterator<MutableFst<Arc>> aiter(
               mutable_ngram2->GetMutableFst(), st);
           !aiter.Done(); aiter.Next()) {
        Arc arc = aiter.Value();
        // find symbol in symbol table
        if (symbol_map.find(arc.ilabel) == symbol_map.end())
          symbol_map[arc.ilabel] = NewWordKey(
              ngram2_->GetFst().InputSymbols()->Find(arc.ilabel), arc.ilabel);
        if (arc.ilabel != symbol_map[arc.ilabel]) {  // maps to a different idx
          arc.ilabel = arc.olabel = symbol_map[arc.ilabel];  // relabel arc
          aiter.SetValue(arc);
        }
      }
    }
    ArcSort(mutable_ngram2->GetMutableFst(), ILabelCompare<Arc>());
    mutable_ngram2->InitModel();
    ngram2_ = std::move(mutable_ngram2);
    return ngram2_->Error() ? false : true;
  }

  // Finds word key if in symbol table, otherwise adds (for merging wordlists)
  int64 NewWordKey(string symbol, int64 key2) {
    int64 key1 = GetMutableFst()->InputSymbols()->Find(symbol);
    if (key1 < 0) {  // Returns key2 if free, o.w. next available key.
      key1 = GetMutableFst()->InputSymbols()->Find(key2).empty()
                 ? key2
                 : GetMutableFst()->InputSymbols()->AvailableKey();
      GetMutableFst()->MutableInputSymbols()->AddSymbol(symbol, key1);
      GetMutableFst()->MutableOutputSymbols()->AddSymbol(symbol, key1);
    }  // ... else returns existing key
    return key1;
  }

  // Maps states representing identical n-gram histories
  void SetupMergeMaps() {
    // Initializes the maps between models
    bool first_setup = exact_map_1to2_.empty();
    bool bo_from1 = MergeUnshared(true);

    if (first_setup) {
      for (StateId j = 0; j < ngram1_ns_; ++j) exact_map_1to2_.push_back(-1);
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
      if (unigram1 != kNoStateId) {  // merging two order > 1
        MergeStateMaps(unigram1, unigram2, true, bo_from1, true);
        MergeStateMaps(start1, start2, true, bo_from1, true);
      } else {  // merging order > 1 into unigram
        MergeStateMaps(start1, unigram2, true, bo_from1, true);
        MergeStateMaps(start1, start2, false, false, true);
      }
    } else if (unigram1 != kNoStateId) {  // merging unigram into order > 1
      MergeStateMaps(unigram1, start2, true, bo_from1, true);
      MergeStateMaps(start1, start2, false, bo_from1, false);
    } else {  // merging two unigrams
      MergeStateMaps(start1, start2, true, bo_from1, true);
    }

    if (!bo_from1 && first_setup) MergeBackedOffToMap();
  }

  // Creates exact and backoff state maps. Maps for exact state context
  // corrspondence created if 'exact' is true. Map that finds state in
  // ngram2 that is the closest backed-off context from corresponding
  // state in ngrams is created if 'bo_from1' is true. Similarly,
  // for 'bo_from2'.
  void MergeStateMaps(StateId st, StateId ist, bool exact, bool bo_from1,
                      bool bo_from2) {
    if (exact) MergeExactStateMap(st, ist);
    if (bo_from1) MergeBackoffStateMap(ist, st, true);
    if (bo_from2) MergeBackoffStateMap(st, ist, false);
  }

  // Creates a state map from ngram2 to equivalent states in ngram1
  void MergeExactStateMap(StateId st, StateId ist) {
    exact_map_1to2_[st] = ist;  // collect source state for state match
    exact_map_2to1_[ist] = st;  // collect target state for state match

    Matcher<Fst<Arc>> matcher(GetFst(), MATCH_INPUT);
    matcher.SetState(st);
    for (ArcIterator<Fst<Arc>> biter(ngram2_->GetFst(), ist); !biter.Done();
         biter.Next()) {
      Arc barc = biter.Value();
      if (barc.ilabel == BackoffLabel()) continue;
      if (matcher.Find(barc.ilabel)) {  // found match in ngram1
        Arc arc = matcher.Value();
        if (StateOrder(arc.nextstate) > StateOrder(st) &&
            ngram2_->StateOrder(barc.nextstate) >
                ngram2_->StateOrder(ist))  // if both next states higher order
          MergeExactStateMap(arc.nextstate, barc.nextstate);  // map those too
      }
    }
  }

  // Creates a map from a state in one ngram model to closest backed-off
  // context in other model
  void MergeBackoffStateMap(StateId st, StateId ist, bool from1) {
    if (Error()) return;
    const NGramModel<Arc> *ngram = from1 ? this : ngram2_.get();
    UpdateBackoffMap(st, ist, from1);  // update state map

    for (ArcIterator<Fst<Arc>> biter(ngram->GetFst(), ist); !biter.Done();
         biter.Next()) {
      Arc barc = biter.Value();
      if (barc.ilabel == BackoffLabel() ||
          ngram->StateOrder(barc.nextstate) <= ngram->StateOrder(ist))
        continue;
      MergeBackoffStateMap(MergeBackoffDest(st, barc.ilabel, from1, 0),
                           barc.nextstate, from1);
      if (Error()) return;
    }
  }

  // Creates a map from state s and label l in ngram1, with state s
  // having label l on an outgoing arc to destination d, to the set of
  // states s', backing off to s, that also have an arc labeled with l
  // and going to destination d. Computed only for non-ascending arcs.
  void MergeBackedOffToMap() {
    for (StateId st = 0; st < ngram1_ns_; ++st) {
      StateId bo = GetBackoff(st, 0);
      if (bo < 0) continue;
      for (ArcIterator<Fst<Arc>> aiter(GetFst(), st); !aiter.Done();
           aiter.Next()) {
        const Arc &arc = aiter.Value();
        if (arc.ilabel == BackoffLabel()) continue;
        if (StateOrder(st) > StateOrder(arc.nextstate)) {
          std::pair<StateId, Label> pr(bo, arc.ilabel);
          backed_off_to_.insert(std::make_pair(pr, st));
        }
      }
    }
  }

  // Combines Fsts; not necessarily normalized.
  void MergeFsts() {
    bool merge_unshared1 = MergeUnshared(true);

    // Adds all states from ngram2 not in ngram1
    for (StateId ist = 0; ist < ngram2_ns_; ++ist) {  // all states in ngram2
      if (exact_map_2to1_[ist] < 0) {  // no matching state in ngram1
        StateId st = GetMutableFst()->AddState();
        UpdateState(st, ngram2_->StateOrder(ist), false,
                    check_consistency_ ? &(ngram2_->StateNGram(ist)) : 0);
        if (Error()) return;
        exact_map_1to2_.push_back(ist);
        exact_map_2to1_[ist] = st;
        if (merge_unshared1) backoff_map_1to2_.push_back(-1);
      }
    }

    // If merging non-unigram into unigram, updates start and unigram info.
    if (UnigramState() == kNoStateId && ngram2_->UnigramState() != kNoStateId) {
      StateId new_start = exact_map_2to1_[ngram2_->GetFst().Start()];
      StateId new_unigram = exact_map_2to1_[ngram2_->UnigramState()];
      GetMutableFst()->SetStart(new_start);
      UpdateState(new_unigram, 1, true,
                  check_consistency_ ? &StateNGram(new_unigram) : 0);
      if (Error()) return;
    }

    // Merges arcs from shared and unshared states
    set<Label> shared;  // shared arcs between st and ist
    for (int order = HiOrder(); order > 0; --order) {
      for (StateId ist = 0; ist < ngram2_ns_; ++ist) {
        if (ngram2_->StateOrder(ist) == order) {
          StateId st = exact_map_2to1_[ist];
          shared.clear();
          if (st < ngram1_ns_) {                // shared state
            MergeSharedArcs(st, ist, &shared);  // n-grams in both ngram1,2
            if (Error()) return;
            if (merge_unshared1) {
              MergeUnsharedArcs1(st, ist, shared);  // n-grams just in ngram1
              if (Error()) return;
            }
          }
          MergeUnsharedArcs2(st, ist, shared);  // n-grams just in ngram2
          if (Error()) return;
        }
      }
      if (merge_unshared1) {
        shared.clear();
        for (StateId st = 0; st < ngram1_ns_; ++st) {
          if (StateOrder(st) == order && exact_map_1to2_[st] < 0) {
            MergeUnsharedArcs1(st, -1, shared);
            if (Error()) return;
          }
        }
      }
    }
  }

  // For n-gram arcs shared in common, combines weight,
  // sets correct destination
  void MergeSharedArcs(StateId st, StateId ist, set<Label> *shared) {
    MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
    if (!aiter.Done()) {
      Arc arc = aiter.Value();
      for (ArcIterator<Fst<Arc>> biter(ngram2_->GetFst(), ist); !biter.Done();
           biter.Next()) {
        const Arc &barc = biter.Value();
        // Can't use matcher for Mutable fst (full copy made), use iterator
        if (FindMutableArc(&aiter, barc.ilabel)) {  // found in ngram1
          arc = aiter.Value();
          if (barc.ilabel != BackoffLabel() &&
              StateOrder(arc.nextstate) < ngram2_->StateOrder(barc.nextstate)) {
            // needs new destination
            if (!MergeUnshared(true)) {
              MergeDests1(st, arc.ilabel, arc.nextstate,
                          exact_map_2to1_[barc.nextstate]);
              if (Error()) return;
            }
            arc.nextstate = exact_map_2to1_[barc.nextstate];
          }
          shared->insert(arc.ilabel);  // marks word shared
          arc.weight = MergeWeights(st, ist, arc.ilabel, arc.weight,
                                    barc.weight, true, true);
          aiter.SetValue(arc);
        }
      }
    }

    // Superfinal arc
    Weight final1 = GetFst().Final(st);
    Weight final2 = ngram2_->GetFst().Final(ist);
    if (ScalarValue(final1) != ScalarValue(Weight::Zero()) &&
        ScalarValue(final2) != ScalarValue(Weight::Zero())) {
      Weight merge_final =
          MergeWeights(st, ist, kNoLabel, final1, final2, true, true);
      GetMutableFst()->SetFinal(st, merge_final);
      shared->insert(kNoLabel);  // marks superfinal word shared
    }
  }

  // Merges n-gram arcs not found in the new model
  // Applies when MergeUnshared(true) is true.
  void MergeUnsharedArcs1(StateId st, StateId ist, const set<Label> &shared) {
    StateId bst = backoff_map_1to2_[st];
    for (MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      Weight cost = Weight::Zero();
      if (shared.count(arc.ilabel) == 0) {  // not found
        if (arc.ilabel != BackoffLabel()) {
          StateId dest = MergeBackoffDest(bst, arc.ilabel, true, &cost);
          if (Error()) return;
          if (ngram2_->StateOrder(dest) > StateOrder(arc.nextstate))
            arc.nextstate = exact_map_2to1_[dest];  // needs a new destination
        }
        arc.weight =
            MergeWeights(st, bst, arc.ilabel, arc.weight, cost, true, false);
        aiter.SetValue(arc);
      }
    }

    // Superfinal arc
    if (shared.count(kNoLabel) == 0) {  // not found
      Weight final1 = GetFst().Final(st);
      if (ScalarValue(final1) != ScalarValue(Weight::Zero())) {
        int order;
        Weight cost = ngram2_->FinalCostInModel(bst, &order);
        Weight merge_final =
            MergeWeights(st, bst, kNoLabel, final1, cost, true, false);
        GetMutableFst()->SetFinal(st, merge_final);
      }
    }
  }

  // Merges n-gram arcs not found in the original model
  void MergeUnsharedArcs2(StateId st, StateId ist, const set<Label> &shared) {
    StateId bst = backoff_map_2to1_[ist];
    StateId ibo = ngram2_->GetBackoff(ist, 0);
    StateId bo = ibo >= 0 ? exact_map_2to1_[ibo] : -1;
    bool arcsort = false;
    for (ArcIterator<Fst<Arc>> biter(ngram2_->GetFst(), ist); !biter.Done();
         biter.Next()) {
      const Arc &barc = biter.Value();
      Weight cost = Weight::Zero();
      if (shared.count(barc.ilabel) == 0) {  // not found
        Arc arc = barc;
        arc.nextstate = exact_map_2to1_[barc.nextstate];
        if (barc.ilabel != BackoffLabel()) {
          StateId dest = MergeBackoffDest(bst, barc.ilabel, false, &cost);
          if (Error()) return;
          if (StateOrder(dest) > ngram2_->StateOrder(barc.nextstate))
            arc.nextstate = dest;  // needs a new destination state
          if (!MergeUnshared(true) && bo >= 0 &&
              StateOrder(st) > StateOrder(arc.nextstate)) {
            std::pair<StateId, Label> pr(bo, arc.ilabel);
            backed_off_to_.insert(std::make_pair(pr, st));
          }
        }
        arc.weight =
            MergeWeights(bst, ist, arc.ilabel, cost, arc.weight, false, true);
        GetMutableFst()->AddArc(st, arc);
        arcsort = true;
      }
    }

    if (arcsort) SortArcs(st);

    if (shared.count(kNoLabel) == 0) {  // not found
      Weight final2 = ngram2_->GetFst().Final(ist);
      if (ScalarValue(final2) != ScalarValue(Weight::Zero())) {
        int order;
        Weight cost = FinalCostInModel(bst, &order);
        Weight merge_final =
            MergeWeights(bst, ist, kNoLabel, cost, final2, false, true);
        GetMutableFst()->SetFinal(st, merge_final);
      }
    }
  }

  // Merges n-gram arcs not found in the new model by (solely) performing any
  // necessary destination state changes from 'old_dest' to 'new_dest'.
  // Looks up arcs that backoff to state 'low_src' and through label 'label'
  // to 'old_dest'. Applies when MergeUnshared(true) is false.
  void MergeDests1(StateId low_src, Label label, StateId old_dest,
                   StateId new_dest) {
    std::pair<StateId, Label> pr(low_src, label);

    auto it = backed_off_to_.find(pr);

    bool non_ascending = StateOrder(low_src) >= StateOrder(new_dest);

    while (it != backed_off_to_.end() && it->first == pr) {
      // Is backed_off_to_ entry still needed when we're done here?
      bool needed = non_ascending;
      StateId hi_src = it->second;
      MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), hi_src);
      if (!FindMutableArc(&aiter, label)) {
        NGRAMERROR() << "label not found";
        SetError();
        return;
      }
      Arc arc = aiter.Value();

      if (arc.nextstate == old_dest) {  // updates the destination state
        MergeDests1(hi_src, label, old_dest, new_dest);
        arc.nextstate = new_dest;
        aiter.SetValue(arc);
      } else if (arc.nextstate != new_dest) {
        needed = false;  // no longer shares a destination state
      }

      if (needed) {
        ++it;
      } else {
        backed_off_to_.erase(it++);
      }
    }
  }

  // Finds the destination state with label
  // from a backed-off model (assign cost)
  StateId MergeBackoffDest(StateId st, Label label, bool from1, Weight *cost) {
    // ngram1 or ngram2
    const NGramModel<Arc> *ngram = from1 ? ngram2_.get() : this;
    if (st < 0) {
      NGRAMERROR() << "MergeBackoffDest: bad state: " << st;
      SetError();
      return st;
    }
    if (cost != 0) *cost = Arc::Weight::One();
    Matcher<Fst<Arc>> matcher(ngram->GetFst(), MATCH_INPUT);
    matcher.SetState(st);
    while (!matcher.Find(label)) {  // while no match found
      Weight thiscost;
      st = ngram->GetBackoff(st, &thiscost);
      if (st < 0) {
        if (cost != 0) (*cost) = Arc::Weight::Zero();
        return ngram->UnigramState() < 0 ? ngram->GetFst().Start()
                                         : ngram->UnigramState();
      }
      if (cost != 0) (*cost) = Times(*cost, thiscost);
      matcher.SetState(st);
    }
    if (cost != 0) (*cost) = Times(*cost, matcher.Value().weight);
    return matcher.Value().nextstate;
  }

  // Calculates correct normalization constant for each state and normalize
  void NormStates() {
    for (StateId ist = 0; ist < ngram2_ns_; ++ist) {
      StateId st = exact_map_2to1_[ist];
      if (st < ngram1_ns_) {  // state found in both models
        NormState(st, true, true);
      } else if (MergeUnshared(false)) {  // state only found in ngram2
        NormState(st, false, true);
      }
    }

    for (StateId st = 0; st < ngram1_ns_; ++st) {
      if (exact_map_1to2_[st] < 0)  // state only found in ngram1
        NormState(st, true, false);
    }
  }

  // Applies normalization constant to arcs and final cost at state.
  // If, after application of the normalization constant, the probabilities
  // sum to greater than 1 (perhaps due to float imprecision) then a second
  // round of brute force normalization is applied.
  void NormState(StateId st, bool in_fst1, bool in_fst2) {
    double norm = NormWeight(st, in_fst1, in_fst2);
    GetMutableFst()->SetFinal(st, ScaleWeight(GetFst().Final(st), norm));
    double kahan_factor = 0;
    double tot_neg_log_prob = ScalarValue(GetFst().Final(st));
    for (MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      if (arc.ilabel != BackoffLabel()) {
        arc.weight = ScaleWeight(arc.weight, norm);
        aiter.SetValue(arc);
        tot_neg_log_prob =
            NegLogSum(tot_neg_log_prob, ScalarValue(arc.weight), &kahan_factor);
      }
    }
    if (tot_neg_log_prob < 0.0) {
      // Normalizes directly since total probability mass greater than one.
      GetMutableFst()->SetFinal(
          st, ScaleWeight(GetFst().Final(st), -tot_neg_log_prob));
      for (MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
           !aiter.Done(); aiter.Next()) {
        Arc arc = aiter.Value();
        if (arc.ilabel != BackoffLabel()) {
          arc.weight = ScaleWeight(arc.weight, -tot_neg_log_prob);
          aiter.SetValue(arc);
        }
      }
    }
  }

  // Collects target state for backoff state map
  void UpdateBackoffMap(StateId st, StateId ist, bool from1) {
    if (from1) {
      backoff_map_1to2_[ist] = st;
    } else {
      backoff_map_2to1_[ist] = st;
    }
  }

  // NB: ngram1 is *this
  std::unique_ptr<const NGramModel<Arc>> ngram2_;  // model to mix into ngram1
  bool check_consistency_;

  // Maps from a state to its exact same context in the other model
  // These include states that have been added to NGram1.
  vector<StateId> exact_map_1to2_;  // mapping ngram1 states to ngram2 states
  vector<StateId> exact_map_2to1_;  // mapping ngram2 states to ngram1 states

  // Maps from a state to its closest backed-off context in the other model
  // These are wrt original model before any states have been added to NGram1
  vector<StateId> backoff_map_1to2_;  // mapping ngram1 states to ngram2 states
  vector<StateId> backoff_map_2to1_;  // mapping ngram2 states to ngram1 states

  // Given a state s and a label l on an outgoing arc to destination d
  // in ngram1, returns the set of states backing off to s, that also
  // have an arc labeled with l and going to destination d.  Computed
  // only for non-ascending arcs.
  multimap<std::pair<StateId, Label>, StateId> backed_off_to_;

  size_t ngram1_ns_;  // original number of states in ngram1
  size_t ngram2_ns_;  // original number of states in ngram2

  std::unique_ptr<VectorFst<Arc>> fst2_;  // copy of FST2 if needed.
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_MERGE_H_
