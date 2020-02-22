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

#ifndef BAUMWELCH_A_STAR_H_
#define BAUMWELCH_A_STAR_H_

#include <type_traits>
#include <vector>

#include <fst/arc-map.h>
#include <fst/determinize.h>
#include <fst/fst-decl.h>
#include <fst/shortest-path.h>
#include <baumwelch/util.h>

namespace fst {

// Computes the shortest path string over a semiring without the path property:
//
// * Compute the NFA beta.
// * Lazily determinize and compute the DFA beta
// * Lazily cast into the tropical semiring
// * Compute the shortest path using the DFA beta as an A* heuristic
// * Cast back into the input semiring
//
// Due to limitations of determinization, this only works for acceptors.
template <class Arc,
          typename std::enable_if<!IsPath<typename Arc::Weight>::value>::type
              * = nullptr>
void AStarSingleShortestPath(const Fst<Arc> &ifst, MutableFst<Arc> *ofst,
                             float delta = kShortestDelta) {
  using Weight = typename Arc::Weight;
  std::vector<Weight> nfa_beta;
  ShortestDistance(ifst, &nfa_beta, /*reverse=*/true);
  std::vector<Weight> dfa_beta;
  static const DeterminizeFstOptions<Arc> dopts;
  const DeterminizeFst<Arc> dfa(ifst, &nfa_beta, &dfa_beta, dopts);
  // Computing shortest path.
  using StateId = typename Arc::StateId;
  using PathWeight = TropicalWeightTpl<typename Weight::ValueType>;
  using PathArc = ArcTpl<PathWeight>;
  using ToPathMapper = WeightConvertMapper<Arc, PathArc>;
  using FromPathMapper = WeightConvertMapper<PathArc, Arc>;
  const auto path_dfa = MakeArcMapFst(dfa, ToPathMapper());
  VectorFst<PathArc> path_shortest;
  {
    using MyEstimate = NaturalAStarEstimate<StateId, PathWeight>;
    using MyQueue = NaturalAStarQueue<StateId, PathWeight, MyEstimate>;
    using MyArcFilter = AnyArcFilter<PathArc>;
    using MyShortestPathOptions =
        ShortestPathOptions<PathArc, MyQueue, MyArcFilter>;
    const MyEstimate estimate(
        reinterpret_cast<const std::vector<PathWeight> &>(dfa_beta));
    std::vector<PathWeight> dfa_alpha;
    MyQueue queue(dfa_alpha, estimate);
    static const MyArcFilter arc_filter;
    const MyShortestPathOptions sopts(
        &queue, arc_filter,
        /*nshortest=*/1,        // 1-best.
        /*unique=*/false,       // Lattice is already deterministic.
        /*has_distance=*/true,  // Precomputed..
        /*delta=*/delta,
        /*first_path=*/true);  // Heuristic is admissible.
    ShortestPath(path_dfa, &path_shortest, &dfa_alpha, sopts);
    VLOG(1) << internal::CountExploredStates(dfa_alpha) << " states explored";
  }
  static const FromPathMapper from_mapper;
  ArcMap(path_shortest, ofst, from_mapper);
}

}  // namespace fst

#endif  // BAUMWELCH_A_STAR_H_

