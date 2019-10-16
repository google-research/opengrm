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
// Copyright 2017 and onwards Google, Inc.

#ifndef BAUMWELCH_FORWARD_BACKWARD_H_
#define BAUMWELCH_FORWARD_BACKWARD_H_

// Classes for computing and storing forward and backwards (alpha and beta,
// respectively) weights. Separate implementations of the estimation step are
// used for idempotent and non-idempotent semirings.

#include <type_traits>
#include <vector>

#include <fst/log.h>
#include <fst/compose.h>
#include <fst/shortest-distance.h>
#include <baumwelch/util.h>

namespace fst {
namespace internal {

// Computes alpha and beta for idempotent semirings, using A* search during the
// alpha computation. If a state is not visited during search the estimate is
// taken to be semiring zero. This estimate of alpha for a state has the true
// value as an upper bound, since some states not visited during the search will
// have true non-zero values because search terminates once the shortest path
// is found (due to the first_path=true).
template <class Arc, typename std::enable_if<IsIdempotent<
                         typename Arc::Weight>::value>::type * = nullptr>
void ComputeAlphaAndBeta(const ComposeFst<Arc> &ico,
                         std::vector<typename Arc::Weight> *alpha,
                         std::vector<typename Arc::Weight> *beta) {
  // Computes beta.
  ShortestDistance(ico, beta, /*reverse=*/true);
  // Computes alpha using an A* approximation.
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using MyEstimate = NaturalAStarEstimate<StateId, Weight>;
  using MyQueue = NaturalAStarQueue<StateId, Weight, MyEstimate>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestDistanceOptions =
      ShortestDistanceOptions<Arc, MyQueue, MyArcFilter>;
  const MyEstimate estimate(*beta);
  MyQueue queue(*alpha, estimate);
  const MyArcFilter arc_filter;
  const MyShortestDistanceOptions opts(
      &queue, arc_filter,
      /*source=*/kNoStateId,     // Default.
      /*delta=*/kShortestDelta,  // Default.
      /*first_path=*/true);      // Heuristic is admissible.
  ShortestDistance(ico, alpha, opts);
  VLOG(1) << CountExploredStates<Weight>(*alpha) << " alpha states explored";
}

// Computes alpha and beta for non-idempotent semirings, which requires full
// search.
template <class Arc, typename std::enable_if<!IsIdempotent<
                         typename Arc::Weight>::value>::type * = nullptr>
void ComputeAlphaAndBeta(const ComposeFst<Arc> &ico,
                         std::vector<typename Arc::Weight> *alpha,
                         std::vector<typename Arc::Weight> *beta) {
  // Computes beta.
  ShortestDistance(ico, beta, /*reverse=*/true);
  // Computes alpha.
  ShortestDistance(ico, alpha, /*reverse=*/false);
}

// Class storing forward and backwards weights
template <class Arc>
class ForwardBackward {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  explicit ForwardBackward(const ComposeFst<Arc> &ico) {
    ComputeAlphaAndBeta(ico, &alpha_, &beta_);
  }

  const Weight &Alpha(StateId s) const {
    return ForwardBackward::WeightOrZero(s, alpha_);
  }

  const Weight &Beta(StateId s) const {
    return ForwardBackward::WeightOrZero(s, beta_);
  }

 private:
  static constexpr Weight kZero = Weight::Zero();

  // Returns the shortest distance weight, or semiring zero if the state was
  // not visited during the respective shortest distance computation.
  static const Weight &WeightOrZero(StateId s,
                                    const std::vector<Weight> &weights) {
    return (s < weights.size()) ? weights[s] : kZero;
  }

  std::vector<Weight> alpha_;
  std::vector<Weight> beta_;
};

template <class Arc>
constexpr typename ForwardBackward<Arc>::Weight ForwardBackward<Arc>::kZero;

}  // namespace internal
}  // namespace fst

#endif  // BAUMWELCH_FORWARD_BACKWARD_H_

