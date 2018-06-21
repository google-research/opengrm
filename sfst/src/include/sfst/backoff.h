
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
// Copyright 2018 Google, Inc.
// backoff.h
//
// Algorithms for 'backoff' automata.

#ifndef SFST_BACKOFF_H_
#define SFST_BACKOFF_H_

#include <stddef.h>
#include <sys/types.h>
#include <utility>
#include <vector>

#include <fst/log.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <sfst/canonical.h>
#include <sfst/sfst.h>
#include <unordered_map>

namespace sfst {

// A backoff transducer is a canonical SFST (see canonical.h) that
// additionally has the properties:
//
// (1) it is (input) deterministic and (non-phi) epsilon-free
//
// (2) if there is a failure transition from state q to 'backoff
// state' q', then every transition from q has a 'backed-off'
// transition from q'. I.e. if a transition from q is (input) labeled
// with x, then there is a transition from q' also (input) labeled
// with x.
//
// This class provides convenient access to the backoff states and
// backed off arcs.
template <class Arc>
class Backoff {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using Matr = fst::SortedMatcher<fst::Fst<Arc>>;

  Backoff(const fst::Fst<Arc> &fst, Label phi_label);

  // Returns the number of states in the input.
  int NumStates() const { return states_.size(); }

  // The maximum number of states (length + 1) in a failure path.
  int MaxOrder() const { return order_; }

  // The number of states (length + 1) in a failure path from
  // that state.
  int Order(StateId s) const { return states_[s].order; }

  // As returned by canonical.h::PhiTopOrder().
  StateId GetPhiTopOrder(StateId i) const { return top_order_[i]; }

  // Gives the state that is failed to from s (or fst::kNoStateId).
  StateId GetBackoffState(StateId s) const { return states_[s].backoff_state; }

  // Gives the position of the failure arc from s (or -1).
  StateId GetBackoffPosition(StateId s) const {
    return states_[s].backoff_position;
  }

  // Returns the position at the 'backed-off' arc for the arc
  // at position pos from state s. Returns -1 if none (e.g. for the
  // phi arc).
  ssize_t GetBackedOffArc(StateId s, ssize_t pos) const {
    auto iter = arc_map_.find(std::make_pair(s, pos));
    return iter != arc_map_.end() ? iter->second : -1;
  }

  // Returns true if in a bad state.
  bool Error() const { return error_; }

 private:
  // Locates the backoff states and backed-off arcs.
  void FindBackedOffArcs(StateId s);

  void SetError() const { error_ = true; }

  // Backoff state data
  struct BackoffState {
    StateId backoff_state;     // backoff state ID for the referenced state
    ssize_t backoff_position;  // arc position of backoff arc
    int order;                 // order of the referenced state

    BackoffState(StateId s, ssize_t p, int o)
        : backoff_state(s), backoff_position(p), order(o) {}

    BackoffState()
        : backoff_state(fst::kNoStateId), backoff_position(-1), order(1) {}
  };

  // (state, arc position)
  using Pair = std::pair<StateId, ssize_t>;

  struct PairHash {
    size_t operator()(const Pair &p) const {
      return (static_cast<size_t>(p.first) * 55697) ^
             (static_cast<size_t>(p.second) * 54631);
    }
  };

  using PairArcMap = std::unordered_map<Pair, ssize_t, PairHash>;

  const fst::Fst<Arc> &fst_;      // backoff FST
  Label phi_label_;                   // failure label
  int order_;                         // maximal phi-order
  std::vector<StateId> top_order_;    //
  std::vector<BackoffState> states_;  // maps from state ID to backoff info
  PairArcMap arc_map_;                // maps from state id and position to
                                      // ... backed-off arc position
  mutable bool error_;

  Backoff(const Backoff &) = delete;
  Backoff &operator=(const Backoff &) = delete;
};

template <class Arc>
Backoff<Arc>::Backoff(const fst::Fst<Arc> &fst, Label phi_label)
    : fst_(fst), phi_label_(phi_label), order_(1), error_(false) {
  namespace f = fst;

  if (fst_.Start() == f::kNoStateId) {
    FSTERROR() << "Backoff: FST has no states";
    SetError();
    return;
  }

  if (!fst.Properties(f::kILabelSorted, true)) {
    FSTERROR() << "Backoff: FST is not canonical (ilabel-sorted)";
    SetError();
    return;
  }

  bool acyclic = PhiTopOrder(fst_, phi_label_, &top_order_);
  if (!acyclic) {
    FSTERROR() << "Backoff: FST is not canonical (phi-cyclic)";
    SetError();
    return;
  }

  states_.resize(top_order_.size());

  if (phi_label_ == f::kNoStateId) return;

  for (StateId i = top_order_.size() - 1; i >= 0; --i) {
    StateId s = top_order_[i];  // ith state in reverse phi-top order
    BackoffState &bo_state = states_[s];
    Matr matcher(fst_, f::MATCH_INPUT);
    matcher.SetState(s);
    if (matcher.Find(phi_label_)) {
      for (; !matcher.Done(); matcher.Next()) {
        const Arc &arc = matcher.Value();
        // Continues on implicit match.
        if (arc.ilabel == f::kNoLabel) continue;
        // Error if phi non-determinism.
        if (bo_state.backoff_state != f::kNoStateId) {
          FSTERROR() << "Backoff: FST is not canonical (phi-non-determinism)";
          SetError();
          continue;
        }
        bo_state.backoff_state = arc.nextstate;
        bo_state.backoff_position = matcher.Position();
        bo_state.order = states_[bo_state.backoff_state].order + 1;
        if (bo_state.order > order_) order_ = bo_state.order;
        FindBackedOffArcs(s);
      }
    }
  }
}

template <class Arc>
void Backoff<Arc>::FindBackedOffArcs(StateId s) {
  namespace f = fst;
  Matr matcher(fst_, f::MATCH_INPUT);
  matcher.SetState(states_[s].backoff_state);
  for (f::ArcIterator<f::Fst<Arc>> aiter(fst_, s); !aiter.Done();
       aiter.Next()) {
    const Arc &arc = aiter.Value();
    if (arc.ilabel == phi_label_) continue;
    if (arc.ilabel == 0) {
      FSTERROR() << "Backoff: non-failure epsilons disallowed";
      SetError();
      return;
    }
    if (matcher.Find(arc.ilabel)) {
      Pair p(s, aiter.Position());
      arc_map_[p] = matcher.Position();
    } else {
      FSTERROR() << "Backoff: no backed-off arc with label " << arc.ilabel
                 << " from state " << s;
      SetError();
      return;
    }
  }
}

// 'Phi-sums' a backoff SFST: adds the higher-order arc weights
// of a backoff automaton onto lower-order backed-off transitions
// Returns true on success.
template <class Arc, class CompWeight = fst::Log64Weight>
bool SumBackoff(fst::MutableFst<Arc> *fst, typename Arc::Label phi_label) {
  namespace f = fst;
  using Weight = typename Arc::Weight;
  using StateId = typename Arc::StateId;
  fst::WeightConvert<CompWeight, Weight> from_compute_weight;
  fst::WeightConvert<Weight, CompWeight> to_compute_weight;

  Backoff<Arc> backoff(*fst, phi_label);
  for (StateId i = 0; i < fst->NumStates(); ++i) {
    // ith state in phi-top order
    StateId s = backoff.GetPhiTopOrder(i);
    StateId bos = backoff.GetBackoffState(s);
    if (bos == f::kNoStateId) continue;
    Weight final = fst->Final(s);
    if (final != Weight::Zero()) {
      Weight bo_final = fst->Final(bos);
      CompWeight w1 = to_compute_weight(bo_final);
      CompWeight w2 = to_compute_weight(final);
      fst->SetFinal(bos, from_compute_weight(Plus(w1, w2)));
    }
    f::MutableArcIterator<f::MutableFst<Arc>> miter(fst, bos);
    for (f::ArcIterator<f::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      ssize_t pos = aiter.Position();
      if (arc.ilabel == phi_label) continue;
      ssize_t bo_pos = backoff.GetBackedOffArc(s, pos);
      CHECK_NE(bo_pos, -1);
      miter.Seek(bo_pos);
      Arc bo_arc = miter.Value();
      CompWeight w1 = to_compute_weight(bo_arc.weight);
      CompWeight w2 = to_compute_weight(arc.weight);
      bo_arc.weight = from_compute_weight(Plus(w1, w2));
      miter.SetValue(bo_arc);
    }
  }

  return !backoff.Error();
}

// Undoes the 'phi-summation' of a backoff SFST: subtracts the
// higher-order arc weights a backoff automaton from lower-order
// backed-off transitions The bo_zero weight is used for effectively
// zero backoffed-off weights.  This should be non-Zero() if a
// returned backoff topology is to be ensured (cf. super-final
// weights). Returns true on success.
template <class Arc>
bool DiffBackoff(fst::MutableFst<Arc> *fst, typename Arc::Label phi_label,
                 typename Arc::Weight bo_zero = Arc::Weight::Zero()) {
  namespace f = fst;
  using Weight = typename Arc::Weight;
  using StateId = typename Arc::StateId;
  f::WeightConvert<f::Log64Weight, Weight> from_log;
  f::WeightConvert<Weight, f::Log64Weight> to_log;

  Backoff<Arc> backoff(*fst, phi_label);
  for (StateId i = fst->NumStates() - 1; i >= 0; --i) {
    // ith state in reverse phi-top order
    StateId s = backoff.GetPhiTopOrder(i);
    StateId bos = backoff.GetBackoffState(s);
    if (bos == f::kNoStateId) continue;
    Weight final = fst->Final(s);
    if (final != Weight::Zero()) {
      Weight bo_final = fst->Final(bos);
      f::Log64Weight w1 = to_log(bo_final);
      f::Log64Weight w2 = to_log(final);
      fst->SetFinal(bos, Less(w2, w1) ? from_log(Minus(w1, w2)) : bo_zero);
    }
    f::MutableArcIterator<f::MutableFst<Arc>> miter(fst, bos);
    for (f::ArcIterator<f::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      ssize_t pos = aiter.Position();
      if (arc.ilabel == phi_label) continue;
      ssize_t bo_pos = backoff.GetBackedOffArc(s, pos);
      CHECK_NE(bo_pos, -1);
      miter.Seek(bo_pos);
      Arc bo_arc = miter.Value();
      f::Log64Weight w1 = to_log(bo_arc.weight);
      f::Log64Weight w2 = to_log(arc.weight);
      bo_arc.weight = Less(w2, w1) ? from_log(Minus(w1, w2)) : bo_zero;
      miter.SetValue(bo_arc);
    }
  }

  return !backoff.Error();
}

}  // namespace sfst

#endif  // SFST_BACKOFF_H_
