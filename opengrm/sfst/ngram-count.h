// Copyright 2026 The OpenGrm Authors.
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

// NGram counting class adapted for SFST.

#ifndef OPENGRM_SFST_NGRAM_COUNT_H_
#define OPENGRM_SFST_NGRAM_COUNT_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

#include "absl/container/node_hash_map.h"
#include "absl/log/log.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/topsort.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {

// NGramCounter class, adapted from ngram::NGramCounter.
template <class Weight, class Label = int32_t>
class NGramCounter {
 public:
  // Construct an NGramCounter object counting n-grams of order less or equal to
  // 'order'. When 'epsilon_as_backoff' is 'true', the epsilon transition in the
  // input Fst are treated as failure backoff transitions and would trigger the
  // length of the current context to be decreased by one ("pop front").
  explicit NGramCounter(int order, bool epsilon_as_backoff = false,
                        float delta = 1e-9F)
      : order_(order),
        epsilon_as_backoff_(epsilon_as_backoff),
        delta_(delta),
        error_(false) {
    if (order < 1) {
      LOG(ERROR) << "NGramCounter: order must be positive";
      SetError();
      return;
    }
    pair_arc_maps_.resize(order);
    backoff_ = states_.size();
    states_.push_back(CountState(-1, 1, Weight::Zero(), -1));
    if (order == 1) {
      initial_ = backoff_;
    } else {
      initial_ = states_.size();
      states_.push_back(CountState(backoff_, 2, Weight::Zero(), -1));
    }
  }

  // Extract counts from the input acyclic FST.  Return 'true' if counting
  // from the FST was successful and false otherwise. If necessary, a copy is
  // taken and this is sent to the MutableFst overload.
  template <class Arc>
  bool Count(const fst::Fst<Arc>& fst) {
    if (Error()) return false;
    if (fst.Start() == fst::kNoStateId) return false;
    if (fst.Properties(fst::kString, false) == fst::kString) {
      return CountFromStringFst(fst);
    } else if (fst.Properties(fst::kTopSorted, true)) {
      return CountFromTopSortedFst(fst);
    } else {
      fst::VectorFst<Arc> vfst(fst);
      return Count(&vfst);
    }
  }

  // Extract counts from input mutable acyclic FST, top-sorting the input FST
  // if necessary. Returns 'true' when the counting from
  // the Fst was successful and false otherwise.
  template <class Arc>
  bool Count(fst::MutableFst<Arc>* fst) {
    if (Error()) return false;
    if (fst->Start() == fst::kNoStateId) return false;
    if (fst->Properties(fst::kString, true) == fst::kString) {
      return CountFromStringFst(*fst);
    }
    if (fst->Properties(fst::kCoAccessible, true) != fst::kCoAccessible) {
      LOG(WARNING) << "NGramCounter::Count: input FST is not coaccessible";
      fst::Connect(fst);
      if (fst->Start() == fst::kNoStateId) return false;
    }
    if (fst->Properties(fst::kTopSorted, true) != fst::kTopSorted) {
      if (!fst::TopSort(fst)) {
        LOG(ERROR) << "NGramCounter::Count: input FST is not acyclic";
        return false;
      }
    }
    return CountFromTopSortedFst(*fst);
  }

  // Get an FST representation of the ngram counts.
  template <class Arc>
  void GetFst(fst::MutableFst<Arc>* fst, Label phi_label = -1) {
    fst->DeleteStates();
    if (Error()) return;
    for (size_t s = 0; s < states_.size(); ++s) {
      fst->AddState();
      fst->SetFinal(s, typename Arc::Weight(states_[s].final_count.Value()));
      if (states_[s].backoff_state != -1) {
        Label label = phi_label >= 0 ? phi_label : 0;
        fst->AddArc(s, Arc(label, label, Arc::Weight::Zero(),
                           states_[s].backoff_state));
      }
    }
    for (size_t a = 0; a < arcs_.size(); ++a) {
      const CountArc& arc = arcs_[a];
      fst->AddArc(arc.origin, Arc(arc.label, arc.label,
                                  typename Arc::Weight(arc.count.Value()),
                                  arc.destination));
    }
    fst->SetStart(initial_);
    StateCounts(fst, phi_label);
    fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  }

  bool Error() const { return error_; }

 protected:
  void SetError() { error_ = true; }

 private:
  // Data representation for a state.
  struct CountState {
    ssize_t backoff_state;  // ID of the backoff state for the current state.
    int order;              // N-gram order of the state (of the outgoing arcs).
    Weight final_count;     // Count for n-gram corresponding to superfinal arc.
    ssize_t first_arc;      // ID of the first outgoing arc at that state.

    CountState(ssize_t s, size_t o, Weight c, ssize_t a)
        : backoff_state(s), order(o), final_count(c), first_arc(a) {}
  };

  // Data represention for an arc.
  struct CountArc {
    ssize_t origin;       // ID of the origin state for this arc.
    ssize_t destination;  // ID of the destination state for this arc.
    Label label;          // Label.
    Weight count;         // Count of the n-gram corresponding to this arc.
    ssize_t backoff_arc;  // ID of backoff arc.

    CountArc(ssize_t o, size_t d, Label l, Weight c, ssize_t b)
        : origin(o), destination(d), label(l), count(c), backoff_arc(b) {}
  };

  using Pair = std::pair<ssize_t, ssize_t>;

  struct PairHash {
    size_t operator()(const Pair& p) const {
      return (static_cast<size_t>(p.first) * 55697) ^
             (static_cast<size_t>(p.second) * 54631);
    }
  };

  using PairArcMap = absl::node_hash_map<Pair, size_t, PairHash>;

  struct Compare {
    bool operator()(const Pair& p1, const Pair& p2) const {
      return p1.first == p2.first ? p1.second > p2.second : p1.first > p2.first;
    }
  };

  // Given a state ID and a label, returns the ID of the corresponding
  // arc, creating the arc if it does not exist already.
  ssize_t FindArc(ssize_t state_id, Label label) {
    const CountState& count_state = states_[state_id];
    if (count_state.first_arc != -1) {
      if (arcs_[count_state.first_arc].label == label) {
        return count_state.first_arc;
      }
      const PairArcMap& arc_map = pair_arc_maps_[count_state.order - 1];
      const auto iter = arc_map.find(std::make_pair(label, state_id));
      if (iter != arc_map.end()) return iter->second;
    }
    return AddArc(state_id, label);
  }

  // Creates the arc corresponding to label 'label' out of the state
  // with ID 'state_id'.
  size_t AddArc(ssize_t state_id, Label label) {
    const auto count_state = states_[state_id];
    const ssize_t arc_id = arcs_.size();
    if (count_state.first_arc == -1) {
      states_[state_id].first_arc = arc_id;
    } else {
      pair_arc_maps_[count_state.order - 1].emplace(
          std::make_pair(label, state_id), arc_id);
    }
    arcs_.emplace_back(state_id, initial_, label, Weight::Zero(), -1);
    if (order_ == 1) return arc_id;
    const ssize_t backoff_arc = count_state.backoff_state == -1
                                    ? -1
                                    : FindArc(count_state.backoff_state, label);
    ssize_t destination;
    if (count_state.order == order_) {
      destination = arcs_[backoff_arc].destination;
    } else {
      destination = states_.size();
      CountState next_count_state(
          backoff_arc == -1 ? backoff_ : arcs_[backoff_arc].destination,
          count_state.order + 1, Weight::Zero(), -1);
      states_.push_back(next_count_state);
    }
    arcs_[arc_id].destination = destination;
    arcs_[arc_id].backoff_arc = backoff_arc;
    return arc_id;
  }

  // Increase the count of n-gram corresponding to the arc labeled 'label'
  // out of state of ID 'state_id' by 'count'.
  ssize_t UpdateCount(ssize_t state_id, Label label, Weight count) {
    ssize_t arc_id = FindArc(state_id, label);
    const ssize_t nextstate_id = arcs_[arc_id].destination;
    while (arc_id != -1) {
      arcs_[arc_id].count = Plus(arcs_[arc_id].count, count);
      arc_id = arcs_[arc_id].backoff_arc;
    }
    return nextstate_id;
  }

  // Increase the count of n-gram corresponding to the super-final arc
  // out of state of ID 'state_id' by 'count'.
  void UpdateFinalCount(ssize_t state_id, Weight count) {
    while (state_id != -1) {
      states_[state_id].final_count =
          Plus(states_[state_id].final_count, count);
      state_id = states_[state_id].backoff_state;
    }
  }

  // Puts the sum of counts of non-backoff arcs leaving s on the backoff arc.
  template <class Arc>
  void StateCounts(fst::MutableFst<Arc>* fst, Label phi_label = -1) {
    Label actual_phi_label = phi_label >= 0 ? phi_label : 0;
    for (size_t s = 0; s < states_.size(); ++s) {
      Weight state_count = states_[s].final_count;
      if (states_[s].backoff_state != -1) {
        fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
        ssize_t bo_pos = -1;
        for (; !aiter.Done(); aiter.Next()) {
          const auto& arc = aiter.Value();
          if (arc.ilabel != actual_phi_label) {
            state_count = Plus(state_count, Weight(arc.weight.Value()));
          } else {
            bo_pos = aiter.Position();
          }
        }
        if (bo_pos >= 0) {
          aiter.Seek(bo_pos);
          auto arc = aiter.Value();
          arc.weight = typename Arc::Weight(state_count.Value());
          aiter.SetValue(arc);
        }
      }
    }
  }

  template <class Arc>
  bool CountFromStringFst(const fst::Fst<Arc>& fst) {
    if (!fst.Properties(fst::kString, false)) {
      LOG(ERROR) << "Input FST is not a string";
      return false;
    }
    ssize_t count_state = initial_;
    auto fst_state = fst.Start();
    Weight weight = fst.Properties(fst::kUnweighted, false)
                        ? Weight::One()
                        : Weight(fst::ShortestDistance(fst).Value());
    while (fst.Final(fst_state) == Arc::Weight::Zero()) {
      fst::ArcIterator<fst::Fst<Arc>> aiter(fst, fst_state);
      const auto& arc = aiter.Value();
      if (arc.ilabel) {
        count_state = UpdateCount(count_state, arc.ilabel, weight);
      } else if (epsilon_as_backoff_) {
        const ssize_t next_count_state = states_[count_state].backoff_state;
        count_state = next_count_state == -1 ? count_state : next_count_state;
      }
      fst_state = arc.nextstate;
      aiter.Next();
      if (!aiter.Done()) {
        LOG(ERROR) << "More than one arc leaving state " << fst_state;
        return false;
      }
    }
    UpdateFinalCount(count_state, weight);
    return true;
  }

  template <class Arc>
  bool CountFromTopSortedFst(const fst::Fst<Arc>& fst) {
    if (!fst.Properties(fst::kTopSorted, false)) {
      LOG(ERROR) << "Input not topologically sorted";
      return false;
    }
    std::vector<typename Arc::Weight> fdistance;
    fst::ShortestDistance(fst, &fdistance, true, delta_);
    std::vector<Pair> heap;
    absl::node_hash_map<Pair, typename Arc::Weight, PairHash> pair2weight;
    Compare compare;
    Pair start_pair = std::make_pair(fst.Start(), initial_);
    pair2weight[start_pair] = Arc::Weight::One();
    heap.push_back(start_pair);
    std::push_heap(heap.begin(), heap.end(), compare);
    while (!heap.empty()) {
      std::pop_heap(heap.begin(), heap.end(), compare);
      const auto current_pair = heap.back();
      const auto fst_state = current_pair.first;
      const ssize_t count_state = current_pair.second;
      auto current_weight = pair2weight[current_pair];
      pair2weight.erase(current_pair);
      heap.pop_back();
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, fst_state); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        Pair next_pair(arc.nextstate, count_state);
        if (arc.ilabel) {
          Weight count = Weight(
              Times(current_weight, Times(arc.weight, fdistance[arc.nextstate]))
                  .Value());
          next_pair.second = UpdateCount(count_state, arc.ilabel, count);
        } else if (epsilon_as_backoff_) {
          const ssize_t next_count_state = states_[count_state].backoff_state;
          next_pair.second =
              next_count_state == -1 ? count_state : next_count_state;
        }
        const auto next_weight = Times(current_weight, arc.weight);
        const auto iter = pair2weight.find(next_pair);
        if (iter == pair2weight.end()) {
          pair2weight[next_pair] = next_weight;
          heap.push_back(next_pair);
          std::push_heap(heap.begin(), heap.end(), compare);
        } else {
          iter->second = Plus(iter->second, next_weight);
        }
      }
      if (fst.Final(fst_state) != Arc::Weight::Zero()) {
        UpdateFinalCount(
            count_state,
            Weight(Times(current_weight, fst.Final(fst_state)).Value()));
      }
    }
    return true;
  }

  int order_;
  std::vector<CountState> states_;
  std::vector<CountArc> arcs_;
  ssize_t initial_;
  ssize_t backoff_;
  std::vector<PairArcMap> pair_arc_maps_;
  bool epsilon_as_backoff_;
  float delta_;
  bool error_;

  NGramCounter(const NGramCounter&) = delete;
  NGramCounter& operator=(const NGramCounter&) = delete;
};

}  // namespace sfst

#endif  // OPENGRM_SFST_NGRAM_COUNT_H_
