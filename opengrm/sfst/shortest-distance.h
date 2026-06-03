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

// Computes the shortest distance with failure transitions.

#ifndef OPENGRM_SFST_SHORTEST_DISTANCE_H_
#define OPENGRM_SFST_SHORTEST_DISTANCE_H_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/container/node_hash_map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/rmphi.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {

namespace internal {

// This queue is used with SignedShortestDistance to correctly compute the
// shortest distance when used with the output of RmPhi.
template <class Arc>
class SignedShortestDistanceQueue final
    : public fst::QueueBase<typename Arc::StateId> {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using MMap = std::unordered_multimap<StateId, StateId>;

  // For each state q >= astart, q' = astates[q - astart] is its
  // 'anti-state'; the q must be dequeued right before q'. This
  // ensures that paths with negatively weighted transitions are
  // matched up suitably with the corresponding paths of positive
  // weight in the RmPhi construction. The astart value should be
  // the number of states in the output of RmPhi.
  SignedShortestDistanceQueue(const fst::Fst<Arc>& fst,
                              const std::vector<Weight>& distance,
                              const std::vector<StateId>& astates,
                              size_t astart, bool reverse = false)
      : fst::QueueBase<StateId>(fst::OTHER_QUEUE),
        astates_(astates),
        astart_(reverse ? (astart + 1) : astart),
        reverse_(reverse) {
    if (fst.Properties(fst::kAcyclic, true)) {
      fst::AnyArcFilter<Arc> arc_filter;
      lev1_queue_ =
          std::make_unique<fst::TopOrderQueue<StateId>>(fst, arc_filter);
    } else {
      lev1_queue_ = std::make_unique<fst::FifoQueue<StateId>>();
    }
  }

  StateId Head() const override {
    if (lev2_queue_.Empty()) FillLev2Queue();
    return lev2_queue_.Head();
  }

  void Enqueue(StateId s) override {
    // The level1 queue is the base queue for states less than astart_.
    if (s < astart_) {
      lev1_queue_->Enqueue(s);
    } else {
      StateId as = astates_[s - astart_];
      if (reverse_) ++as;  // reverse FST has super-initial state 0
      map_.insert(std::make_pair(as, s));
    }
  }

  void Dequeue() override {
    // Top queue is level2 queue.
    if (lev2_queue_.Empty()) FillLev2Queue();
    lev2_queue_.Dequeue();
  }

  void Update(StateId s) override {
    if (s < astart_) lev1_queue_->Update(s);
  }

  bool Empty() const override {
    return lev1_queue_->Empty() && lev2_queue_.Empty();
  }

  void Clear() override {
    lev1_queue_->Clear();
    lev2_queue_.Clear();
    map_.clear();
  }

 private:
  // This dequeues a state 'as' from the level1 queue and enqueues it in
  // the level2 queue but only after enqueuing any states s > astart_
  // specified by the map_ that must be dequeued with 'as'.
  void FillLev2Queue() const {
    StateId as = lev1_queue_->Head();
    const auto [range_begin, range_end] = map_.equal_range(as);
    for (auto iter = range_begin; iter != range_end;) {
      StateId s = iter->second;
      // Enqueues in lev2_queue and dequeues from map_[as] (we can't use
      // map_ directly as an active queue since an iterator to it could
      // be invalidated by this->Enqueue())
      lev2_queue_.Enqueue(s);
      map_.erase(iter++);
    }
    // Finally enqueues 'as' in level2 queue.
    lev1_queue_->Dequeue();
    lev2_queue_.Enqueue(as);
  }

  const std::vector<StateId>& astates_;  // s -> anti-s
  size_t astart_;                        // astates offset
  bool reverse_;

  mutable MMap map_;  // anti-s -> s
  // For when s < astart_ queue; this queue can be changed to any discipline.
  std::unique_ptr<fst::QueueBase<StateId>> lev1_queue_;
  // For when s >= astart_ queue; this must be FIFO.
  mutable fst::FifoQueue<StateId> lev2_queue_;

  SignedShortestDistanceQueue(const SignedShortestDistanceQueue&) = delete;
  SignedShortestDistanceQueue& operator=(const SignedShortestDistanceQueue&) =
      delete;
};

// This version of shortest distance computes the shortest distance on
// a ring.  The 'Arc' weight (e.g. SignedLog64Arc) must have a Minus()
// operation (forming a ring).
template <class Arc, class WeightEqual = fst::WeightApproxEqual>
class SignedShortestDistance {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  // For cyclic input and 'negative' weights, any convergence is, in
  // general, conditional and not absolute, so it will depend on the
  // specific input.
  explicit SignedShortestDistance(const fst::Fst<Arc>& fst,
                                  float delta = fst::kShortestDelta)
      : fst_(fst), phi_label_(fst::kNoLabel), delta_(delta) {
    astart_ = fst::CountStates(fst);
  }

  // This version is designed to work with the output of RmPhi
  // (called with MATCHER_REWRITE_NEVER). It may have to add states
  // and epsilon transitions, but this ensures convergence and a
  // correct result when the shortest distance is defined and finite.
  // The phi_label is passed since it is kept on the output label by
  // RmPhi in this case.
  explicit SignedShortestDistance(fst::MutableFst<Arc>* fst,
                                  typename Arc::Label phi_label = fst::kNoLabel,
                                  float delta = fst::kShortestDelta)
      : fst_(*fst), phi_label_(phi_label), delta_(delta) {
    astart_ = fst->NumStates();
    BalancePaths(fst);
  }

  // This computes the shortest distance to the final states when 'reverse =
  // true', o.w. computes it from the initial state.  Convergence within 'delta'
  // w.r.t. weights ('-log probs') unless 'cmp_exp_weights = true' ('probs').
  // The former is more generally applicable (e.g. for normalization), the
  // latter can be faster (e.g. for counting). Returns false on error. An
  // unvisited state S has distance Zero(), which will be stored in the
  // 'distance' vector if S is less than the maximum visited state. Additional
  // states may have be added if constructed with the second constructor above.
  bool ComputeDistance(std::vector<Weight>* distance, bool reverse = false);

 private:
  struct AMapHash {
    size_t operator()(const std::pair<StateId, StateId>& p) const {
      static constexpr auto prime = 7853;
      return p.first + p.second * prime;
    }
  };

  // This transforms a ring-weighted FST, generated
  // by RmPhi, so that when used with the appropriate
  // queue, the shortest distance will be correctly
  // computed. This construction may add states and epsilon
  // transitions.
  void BalancePaths(fst::MutableFst<Arc>* fst);

  const fst::Fst<Arc>& fst_;
  Label phi_label_;
  float delta_;
  // Any states >= this value are newly added.
  size_t astart_;
  // This vector is used as an argument to the shortest-path queue. It is used
  // to ensure oppositely signed 'corresponding' paths are dequeued adjacent. In
  // particular, q' = astates_[q - astart_] is the 'anti-state' for added state
  // q; q must be dequeued right before the q'.
  std::vector<StateId> astates_;

  SignedShortestDistance(const SignedShortestDistance&) = delete;
  SignedShortestDistance& operator=(const SignedShortestDistance&) = delete;
};

template <class Arc, class WeightEqual>
void SignedShortestDistance<Arc, WeightEqual>::BalancePaths(
    fst::MutableFst<Arc>* fst) {
  if (!astates_.empty()) return;

  if (phi_label_ == fst::kNoLabel || fst->Properties(fst::kAcyclic, true))
    return;

  absl::node_hash_map<std::pair<StateId, StateId>, StateId, AMapHash> amap;

  for (StateId s = 0; s < astart_; ++s) {
    StateId as = fst::kNoStateId;
    absl::node_hash_map<StateId, Weight> ns_weight;
    // Finds negative multiarcs
    for (fst::ArcIterator<fst::MutableFst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      // TODO: 'as' could be on a phi PATH
      if (arc.ilabel == 0 && arc.olabel == phi_label_)
        as = arc.nextstate;  // the 'anti-state' for any added states
      auto it = ns_weight.find(arc.nextstate);
      if (it != ns_weight.end()) {
        it->second = Plus(it->second, arc.weight);
      } else {
        ns_weight[arc.nextstate] = arc.weight;
      }
    }

    if (as == fst::kNoStateId) continue;
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      // Negative arc and multiarc
      if (IsNegative(arc.weight) &&
          (IsNegative(ns_weight[arc.nextstate]) ||
           ns_weight[arc.nextstate] == Weight::Zero())) {
        // Creates/reuses a shared state and epsilon arc that
        // lengthens any negative arc that goes to arc.nextstate and
        // has 'anti-state' as. This 'balances' the oppositely signed
        // path lengths which facilitates the queue management.
        std::pair<StateId, StateId> p(as, arc.nextstate);
        auto it = amap.find(p);
        if (it == amap.end()) {
          StateId t = fst->AddState();
          fst->AddArc(t, Arc(0, 0, Weight::One(), arc.nextstate));
          amap[p] = t;
          astates_.push_back(as);
          arc.nextstate = t;
        } else {
          arc.nextstate = it->second;
        }
        aiter.SetValue(arc);
      }
    }
  }
}

template <class Arc, class WeightEqual>
bool SignedShortestDistance<Arc, WeightEqual>::ComputeDistance(
    std::vector<Weight>* distance, bool reverse) {
  using ArcFilter = fst::AnyArcFilter<Arc>;
  using Queue = fst::QueueBase<StateId>;

  distance->clear();
  ArcFilter arc_filter;
  std::unique_ptr<fst::Fst<Arc>> fst;

  if (reverse) {
    auto rfst = std::make_unique<fst::VectorFst<Arc>>();
    fst::Reverse(fst_, rfst.get());
    fst = std::move(rfst);
  } else {
    fst.reset(fst_.Copy());
  }

  internal::SignedShortestDistanceQueue<Arc> queue(*fst, *distance, astates_,
                                                   astart_, reverse);
  fst::ShortestDistanceOptions<Arc, Queue, ArcFilter> opts(&queue, arc_filter);
  opts.delta = delta_;

  fst::internal::ShortestDistanceState<Arc, Queue, ArcFilter, WeightEqual>
      sd_state(*fst, distance, opts, false);
  sd_state.ShortestDistance(opts.source);
  if (sd_state.Error()) return false;

  if (reverse) {
    std::vector<Weight> rdistance;
    while (rdistance.size() < distance->size() - 1)
      rdistance.push_back((*distance)[rdistance.size() + 1]);
    *distance = rdistance;
  }

  return true;
}

}  // namespace internal

// This version of shortest distance computes the shortest distance if failure
// transitions may be present.  It computes the shortest distance to the final
// states when reverse = true, o.w. computes it from the initial state. Returns
// false on error.  An unvisited state S has distance Zero(), which will be
// stored in the 'distance' vector if S is less than the maximum visited state.
// Assumes (but does not check) that the input has the canonical topology (see
// canonical.h).  Also assumes input has no (non-phi) epsilons (or treats such
// epsilons w.r.t. the failure semantics as if they were regular,
// uniquely-labeled symbols).  The 'SignedArc' weight must have a Minus()
// operation (forming a ring) and a WeightConvert method from 'Arc'.
template <class Arc, class SignedArc = fst::SignedLog64Arc,
          class WeightEqual = fst::WeightApproxEqual>
typename Arc::Weight ShortestDistance(
    const fst::Fst<Arc>& fst, std::vector<typename Arc::Weight>* distance,
    typename Arc::Label phi_label = fst::kNoLabel, bool reverse = false,
    float delta = fst::kShortestDelta) {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using SignedWeight = typename SignedArc::Weight;

  fst::VectorFst<SignedArc> sfst;
  internal::RmPhi(fst, &sfst, phi_label, fst::MATCHER_REWRITE_NEVER);
  size_t ins = sfst.NumStates();
  internal::SignedShortestDistance<SignedArc, WeightEqual> sdist(
      &sfst, phi_label, delta);
  std::vector<SignedWeight> sdistance;
  if (!sdist.ComputeDistance(&sdistance, reverse)) return Weight::NoWeight();
  fst::WeightConvert<SignedWeight, Weight> from_signed_convert;
  distance->clear();
  for (size_t i = 0; i < sdistance.size(); ++i) {
    auto dist = sdistance[i];
    if (IsNegative(dist)) dist = SignedWeight::Zero();
    distance->push_back(from_signed_convert(dist));
  }

  // Removes any added states in the construction
  distance->resize(std::min(ins, distance->size()));

  // Computes total weight. Note this is non-trivial from the distance vector
  // w/o 'sfst' since it may have different final states than 'fst' due to
  // phi-accessibility.
  fst::Adder<Weight> total_weight;
  if (reverse) {
    if (distance->size() > sfst.Start())
      total_weight.Add((*distance)[sfst.Start()]);
  } else {
    for (StateId s = 0; s < distance->size(); ++s) {
      Weight final_weight = from_signed_convert(sfst.Final(s));
      total_weight.Add(Times((*distance)[s], final_weight));
    }
  }

  return total_weight.Sum();
}

}  // namespace sfst

#endif  // OPENGRM_SFST_SHORTEST_DISTANCE_H_
