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

#ifndef BAUMWELCH_DECODE_H_
#define BAUMWELCH_DECODE_H_

// This header defines functions for decoding output into input using
// the Viterbi algorithm. Both FST and FAR inputs/outputs are supported.
//
// If the semiring (arc) does not have the path property, then there must exist
// a weight converter to and from the tropical semiring (arc). (Weight
// properties must also be known at compile time.)
//
// Decoding proceeds as follows:
//
// 1. Compose the channel model and the output.
// 2. Project onto the input (input) side of (1).
// 3. Compose the input with the result of (2) to construct a weighted
//    lattice.
// 4. If the input semiring is does not have the path property (i.e., is not
//    kPath), attempt to convert the weighted lattice to the tropical semiring.
// 5. Compute the shortest path over the weighted lattice (3-4).
// 6. If the input semiring does not have the path property (i.e., is not
//    kPath), attempt to convert the shortest path (5) back to the input
//    semiring.
// 7. Project onto the input, remove epsilon arcs, and remove weights.
//
// Note that if a uniform channel model is provided, this is equivalent to
// decoding with the input model alone.
//
// By convention, operations in this library that work with FarReader input
// reset the FAR to its initial position upon completion.

#include <algorithm>
#include <memory>
#include <numeric>
#include <type_traits>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fst/arc-map.h>
#include <fst/fst-decl.h>
#include <fst/project.h>
#include <fst/rmepsilon.h>
#include <fst/shortest-path.h>
#include <fst/statesort.h>
#include <baumwelch/cascade.h>
#include <baumwelch/data.h>
#include <baumwelch/util.h>

namespace fst {
namespace internal {

// Helper function that reverses the state numbering of the output of single
// ShortestPath and removes weights. This produces an unweighted FST, and
// if the input consists of a single-path acceptor, a "kString" FST.
template <class Arc>
void MakeString(MutableFst<Arc> *fst) {
  using StateId = typename Arc::StateId;
  std::vector<StateId> order(fst->NumStates());
  std::iota(order.rbegin(), order.rend(), 0);
  StateSort(fst, order);
  ArcMap(fst, RmWeightMapper<Arc>());
}

// Computes the shortest path string over a semiring with the path property.
template <class Arc, typename std::enable_if<
                         IsPath<typename Arc::Weight>::value>::type * = nullptr>
void ShortestPathString(const Fst<Arc> &ico, VectorFst<Arc> *shortest) {
  ShortestPath(ico, shortest);
  MakeString(shortest);
}

// Computes the shortest path string over a semiring without the path property:
//
// * Lazily determinize and lazily compute the A* heuristic
// * Lazily cast into the tropical semiring
// * Compute the shortest path using A* search
// * Eagerly cast back into the input semiring
// * Optimize to yield a kString FST
template <class Arc,
          typename std::enable_if<!IsPath<typename Arc::Weight>::value>::type
              * = nullptr>
void ShortestPathString(const Fst<Arc> &ico, VectorFst<Arc> *shortest) {
  // A* search heuristic.
  using Weight = typename Arc::Weight;
  std::vector<Weight> nfa_beta;
  ShortestDistance(ico, &nfa_beta, /*reverse=*/true);
  std::vector<Weight> dfa_beta;
  const DeterminizeFstOptions<Arc> opts;
  const DeterminizeFst<Arc> dfa(ico, &nfa_beta, &dfa_beta, opts);
  // Computing shortest path.
  using StateId = typename Arc::StateId;
  using PathWeight = TropicalWeightTpl<typename Weight::ValueType>;
  using PathArc = ArcTpl<PathWeight>;
  using ToPathMapper = WeightConvertMapper<Arc, PathArc>;
  using FromPathMapper = WeightConvertMapper<PathArc, Arc>;
  const ArcMapFst<Arc, PathArc, ToPathMapper> path_dfa(dfa, ToPathMapper());
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
    const MyArcFilter arc_filter;
    const MyShortestPathOptions opts(
        &queue, arc_filter,
        /*nshortest=*/1,           // 1-best.
        /*unique=*/false,          // Lattice is already deterministic.
        /*has_distance=*/true,     // Heuristic is precomputed.
        /*delta=*/kShortestDelta,  // Default.
        /*first_path=*/true);      // Heuristic is admissible.
    ShortestPath(path_dfa, &path_shortest, &dfa_alpha, opts);
    VLOG(1) << CountExploredStates(dfa_alpha) << " states explored";
  }
  ArcMap(path_shortest, shortest, FromPathMapper());
  MakeString(shortest);
}

// Computes the shortest path input string; we project and remove epsilons
// beforehand.
template <class Arc>
void DeciphermentShortestPathString(const Fst<Arc> &ico,
                                    VectorFst<Arc> *shortest) {
  VectorFst<Arc> lattice;
  Project(ico, &lattice, PROJECT_INPUT);
  RmEpsilon(&lattice);
  ShortestPathString(lattice, shortest);
}

// Paired data construction; outputs the highest-scoring alignment.
template <class Arc>
void DecodeBaumWelch(PairedData<Arc> *data, const Fst<Arc> &channel,
                     FarWriter<Arc> *hypotext) {
  for (; !data->Done(); data->Next()) {
    const SingleStateCascade<Arc> cascade(data->GetInput(), data->GetOutput(),
                                          channel);
    VectorFst<Arc> shortest;
    ShortestPathString(cascade.GetFst(), &shortest);
    hypotext->Add(data->GetKey(), CompactUnweightedFst<Arc>(shortest));
  }
}

// Decipherment construction; outputs the highest-scoring input string.
template <class Arc>
void DecodeBaumWelch(DeciphermentData<Arc> *data, const Fst<Arc> &channel,
                     FarWriter<Arc> *hypotext) {
  for (; !data->Done(); data->Next()) {
    const SingleStateCascade<Arc> cascade(data->GetInput(), data->GetOutput(),
                                          channel);
    VectorFst<Arc> shortest;
    DeciphermentShortestPathString(cascade.GetFst(), &shortest);
    hypotext->Add(data->GetKey(), CompactStringFst<Arc>(shortest));
  }
}

}  // namespace internal

// This instantiates the paired construction.
template <class Arc>
void DecodeBaumWelch(FarReader<Arc> *input, FarReader<Arc> *output,
                     const Fst<Arc> &channel, FarWriter<Arc> *writer) {
  internal::PairedData<Arc> data(input, output);
  internal::DecodeBaumWelch(&data, channel, writer);
}

// This instantiates the decipherment construction.
template <class Arc>
void DecodeBaumWelch(const Fst<Arc> &input, FarReader<Arc> *output,
                     const Fst<Arc> &channel, FarWriter<Arc> *writer) {
  internal::DeciphermentData<Arc> data(input, output);
  internal::DecodeBaumWelch(&data, channel, writer);
}

}  // namespace fst

#endif  // BAUMWELCH_DECODE_H_

