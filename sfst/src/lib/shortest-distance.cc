
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
// shortest-distance.cc
//
// Computes the shortest distance with failure transitions.

#include <sfst/shortest-distance.h>

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fst/log.h>
#include <fst/fst.h>
#include <fst/shortest-distance.h>
#include <fst/signed-log-weight.h>
#include <fst/vector-fst.h>
#include <fst/weight.h>
#include <sfst/sfst.h>
#include <unordered_map>

namespace sfst {

// This queue is used with SLShortestDistance to correctly compute the
// shortest distance on the signed-log semiring when used with
// the output of SLRmPhi.
class SLShortestDistanceQueue final :
      public fst::QueueBase<fst::SignedLog64Arc::StateId> {
 public:
  using Arc = fst::SignedLog64Arc;
  using StateId = Arc::StateId;
  using Label = Arc::Label;
  using Weight = Arc::Weight;
  using MMap = std::unordered_multimap<StateId, StateId>;

  // For each state q >= astart, q' = astates[q - astart] is its
  // 'anti-state'; the q must be dequeued right before q'. This
  // ensures that paths with negatively weighted transitions are
  // matched up suitably with the corresponding paths of positive
  // weight in the SLRmPhi construction. The astart value should be
  // the number of states in the output of SLRmPhi.
  SLShortestDistanceQueue(const fst::Fst<Arc> &fst,
                          const std::vector<Weight> &distance,
                          const std::vector<StateId> &astates,
                          size_t astart, bool reverse = false)
      : QueueBase<StateId>(fst::OTHER_QUEUE),
        astates_(astates),
        astart_(reverse ? (astart + 1) : astart),
        reverse_(reverse) {
    namespace f = fst;
    if (fst.Properties(f::kAcyclic, true)) {
      f::AnyArcFilter<Arc> arc_filter;
      lev1_queue_.reset(new f::TopOrderQueue<StateId>(fst, arc_filter));
    } else {
      lev1_queue_.reset(new f::FifoQueue<StateId>());
    }
  }

  StateId Head() const override {
    if (lev2_queue_.Empty())
      FillLev2Queue();
    return lev2_queue_.Head();
  }

  void Enqueue(StateId s) override {
    // The level1 queue is the base queue for states less than astart_.
    if (s < astart_) {
      lev1_queue_->Enqueue(s);
    } else {
      StateId as = astates_[s - astart_];
      if (reverse_) ++as;   // reverse FST has super-initial state 0
      map_.insert(std::make_pair(as, s));
    }
  }

  void Dequeue() override {
    // Top queue is level2 queue.
    if (lev2_queue_.Empty())
      FillLev2Queue();
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
    auto iter = map_.find(as);
    while (iter != map_.end() && iter->first == as) {
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

  const std::vector<StateId> &astates_;      // s -> anti-s
  size_t astart_;                            // astates offset
  bool reverse_;

  mutable MMap map_;                         // anti-s -> s
  // For when s < astart_ queue; this queue can be changed to any discipline.
  std::unique_ptr<fst::QueueBase<StateId>> lev1_queue_;
  // For when s >= astart_ queue; this must be FIFO.
  mutable fst::FifoQueue<StateId> lev2_queue_;


  SLShortestDistanceQueue(const SLShortestDistanceQueue &) = delete;
  SLShortestDistanceQueue &operator=(const SLShortestDistanceQueue &) = delete;
};

struct AMapHash {
  using StateId = fst::SignedLog64Arc::StateId;
  size_t operator()(const std::pair<StateId, StateId> &p) const {
    static constexpr auto prime = 7853;
    return p.first + p.second * prime;
  }
};

void SLShortestDistance::BalancePaths(fst::MutableFst<Arc> *fst) {
  namespace f = fst;

  if (!astates_.empty()) return;

  if (phi_label_ == f::kNoLabel || fst->Properties(f::kAcyclic, true))
    return;

  std::unordered_map<std::pair<StateId, StateId>, StateId, AMapHash> amap;

  for (StateId s = 0; s < astart_; ++s) {
    StateId as = f::kNoStateId;
    std::unordered_map<StateId, Weight> ns_weight;
    // Finds negative multiarcs
    for (f::ArcIterator<f::MutableFst<Arc>> aiter(*fst, s);
         !aiter.Done(); aiter.Next()) {
      const Arc &arc = aiter.Value();
      // TODO(riley): 'as' could be on a phi PATH
      if (arc.ilabel == 0 && arc.olabel == phi_label_)
        as = arc.nextstate;  // the 'anti-state' for any added states
      auto it = ns_weight.find(arc.nextstate);
      if (it != ns_weight.end()) {
        it->second = Plus(it->second, arc.weight);
      } else {
        ns_weight[arc.nextstate] = arc.weight;
      }
    }

    if (as == f::kNoStateId) continue;
    for (f::MutableArcIterator<f::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      // Negative arc and multiarc
      if (Less(arc.weight, Weight::Zero()) &&
          (!Less(Weight::Zero(), ns_weight[arc.nextstate]))) {
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
        }  else {
          arc.nextstate = it->second;
        }
        aiter.SetValue(arc);
      }
    }
  }
}

bool SLShortestDistance::ComputeDistance(
    std::vector<fst::SignedLog64Weight> *distance, bool reverse) {
  namespace f = fst;

  distance->clear();
  f::AnyArcFilter<Arc> arc_filter;

  if (reverse) {
    f::VectorFst<Arc> rfst;
    f::Reverse(fst_, &rfst);
    std::vector<Weight> rdistance;
    SLShortestDistanceQueue queue(rfst, rdistance,
                                  astates_, astart_, true);
    f::ShortestDistanceOptions<Arc, f::QueueBase<StateId>,
                               f::AnyArcFilter<Arc>>
        opts(&queue, arc_filter);
    opts.delta = delta_;
    ShortestDistance(rfst, &rdistance, opts);

    while (distance->size() < rdistance.size() - 1)
      distance->push_back(rdistance[distance->size() + 1]);
  } else {
    SLShortestDistanceQueue queue(fst_, *distance,
                                  astates_, astart_, false);
    f::ShortestDistanceOptions<Arc, f::QueueBase<StateId>,
                               f::AnyArcFilter<Arc>>
        opts(&queue, arc_filter);
    opts.delta = delta_;
    ShortestDistance(fst_, distance, opts);
  }

  if (!distance->empty() && !(*distance)[0].Member()) {
    LOG(ERROR) << "SLShortestDistance: shortest distance computation failed";
    return false;
  }
  return true;
}

}  // namespace sfst
