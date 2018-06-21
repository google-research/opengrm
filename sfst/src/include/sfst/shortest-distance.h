
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
// shortest-distance.h;
//
// Computes the shortest distance with failure transitions.

#ifndef SFST_SHORTEST_DISTANCE_H_
#define SFST_SHORTEST_DISTANCE_H_

#include <stddef.h>
#include <algorithm>
#include <vector>

#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/shortest-distance.h>
#include <fst/vector-fst.h>
#include <sfst/rmphi.h>
#include <sfst/sfst.h>

namespace sfst {

// This version of shortest distance computes the shortest distance on
// the signed-log semiring.
class SLShortestDistance {
 public:
  using Arc = fst::SignedLog64Arc;
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  // For cyclic input and 'negative' weights, any convergence is, in
  // general, conditional and not absolute, so it will depend on the
  // specific input.
  explicit SLShortestDistance(
      const fst::Fst<Arc> &fst,
      float delta = fst::kShortestDelta)
      : fst_(fst),
        phi_label_(fst::kNoLabel),
        delta_(delta) {
    namespace f = fst;
    astart_ = f::CountStates(fst);
  }

  // This version is designed to work with the output of SLRmPhi
  // (called with MATCHER_REWRITE_NEVER). It may have to add states
  // and epsilon transitions, but this ensures convergence and a
  // correct result when the shortest distance is defined and finite.
  // The phi_label is passed since it is kept on the output label by
  // SLRMPhi in this case.
  explicit SLShortestDistance(
      fst::MutableFst<Arc> *fst,
      typename Arc::Label phi_label = fst::kNoLabel,
      float delta = fst::kShortestDelta)
      : fst_(*fst),
        phi_label_(phi_label),
        delta_(delta) {
    astart_ = fst->NumStates();
    BalancePaths(fst);
  }

  // This computes the shortest distance to the final states when
  // reverse = true, o.w. computes it from the initial state.  Returns
  // false on error. An unvisited state S has distance Zero(), which
  // will be stored in the 'distance' vector if S is less than the
  // maximum visited state. Additional states may have be added if
  // constructed with the second constructor above.
  bool ComputeDistance(
      std::vector<fst::SignedLog64Weight> *distance, bool reverse = false);

 private:
  // This transforms a signed-log weighted FST, generated
  // by SLRmPhi, so that when used with the appropriate
  // signed-log queue, the shortest distance will be correctly
  // computed. This construction may add states and epsilon
  // transitions.
  void BalancePaths(fst::MutableFst<Arc> *fst);

  const fst::Fst<Arc> &fst_;
  Label phi_label_;
  float delta_;
  // Any states >= this value are newly added.
  size_t astart_;
  // This vector is used as an argument to the signed-log
  // shortest-path queue. It is used to ensure oppositely signed
  // 'corresponding' paths are dequeued adjacent. In particular, q' =
  // astates_[q - astart_] is the 'anti-state' for added state q;
  // q must be dequeued right before the q'.
  std::vector<StateId> astates_;

  SLShortestDistance(const SLShortestDistance &) = delete;
  SLShortestDistance &operator=(const SLShortestDistance &) = delete;
};


// This version of shortest distance computes the shortest distance
// when failure transitions may be present.  It computes the shortest
// distance to the final states when reverse = true, o.w. computes it
// from the initial state. Returns false on error.  An unvisited state
// S has distance Zero(), which will be stored in the 'distance'
// vector if S is less than the maximum visited state.  Assumes (but
// does not check) that the input has the canonical topology (see
// canonical.h).  Also assumes input has no (non-phi) epsilons (or
// treats such epsilons w.r.t. the failure semantics as if they were
// regular, uniquely-labeled symbols).
template <class Arc>
bool PhiShortestDistance(
    const fst::Fst<Arc> &fst,
    std::vector<typename Arc::Weight> *distance,
    typename Arc::Label phi_label = fst::kNoLabel,
    bool reverse = false, float delta = fst::kShortestDelta) {
  namespace f = fst;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using SLArc = f::SignedLog64Arc;
  using SLStateId = SLArc::StateId;
  using SLWeight = SLArc::Weight;

  f::VectorFst<SLArc> slfst;
  SLRmPhi(fst, &slfst, phi_label, fst::MATCHER_REWRITE_NEVER);
  size_t ins = slfst.NumStates();
  SLShortestDistance sdist(&slfst, phi_label, delta);

  std::vector<SLWeight> sldistance;
  if (!sdist.ComputeDistance(&sldistance, reverse))
    return false;

  f::WeightConvert<SLWeight, Weight> from_sl_convert;
  distance->clear();
  for (size_t i = 0; i < sldistance.size(); ++i) {
    auto dist = sldistance[i];
    if (Less(dist, SLWeight::Zero()))
      dist = SLWeight::Zero();
    distance->push_back(from_sl_convert(dist));
  }

  // Removes any added states in the SL construction
  distance->resize(std::min(ins, distance->size()));

  return true;
}

}  // namespace sfst

#endif  // SFST_SHORTEST_DISTANCE_H_
