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

// This header defines functions for decoding outputs. Both FST and
// FAR inputs/outputs are supported.
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
#include <baumwelch/a-star.h>
#include <baumwelch/cascade.h>
#include <baumwelch/data.h>

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
void ShortestPathString(const Fst<Arc> &ifst, VectorFst<Arc> *ofst) {
  ShortestPath(ifst, ofst);
  MakeString(ofst);
}

// Computes the shortest path string over a semiring without the path property
// using lazily determinization with A* search.
template <class Arc,
          typename std::enable_if<!IsPath<typename Arc::Weight>::value>::type
              * = nullptr>
void ShortestPathString(const Fst<Arc> &ifst, VectorFst<Arc> *ofst) {
  AStarSingleShortestPath(ifst, ofst);
  MakeString(ofst);
}

// Computes the shortest path input string; we project and remove epsilons
// beforehand.
template <class Arc>
void DeciphermentShortestPathString(const Fst<Arc> &ifst,
                                    VectorFst<Arc> *ofst) {
  VectorFst<Arc> lattice;
  Project(ifst, &lattice, PROJECT_INPUT);
  RmEpsilon(&lattice);
  ShortestPathString(lattice, ofst);
}

// Paired data construction; outputs the highest-scoring alignment.
template <class Arc>
void DecodeBaumWelch(PairedData<Arc> *data, const Fst<Arc> &channel,
                     FarWriter<Arc> *hypotext) {
  for (; !data->Done(); data->Next()) {
    const SingleStateCascade<Arc> cascade(data->GetInput(), data->GetOutput(),
                                          channel);
    VectorFst<Arc> ofst;
    ShortestPathString(cascade.GetFst(), &ofst);
    hypotext->Add(data->GetKey(), CompactUnweightedFst<Arc>(ofst));
  }
}

// Decipherment construction; outputs the highest-scoring input string.
template <class Arc>
void DecodeBaumWelch(DeciphermentData<Arc> *data, const Fst<Arc> &channel,
                     FarWriter<Arc> *hypotext) {
  for (; !data->Done(); data->Next()) {
    const SingleStateCascade<Arc> cascade(data->GetInput(), data->GetOutput(),
                                          channel);
    VectorFst<Arc> ofst;
    DeciphermentShortestPathString(cascade.GetFst(), &ofst);
    hypotext->Add(data->GetKey(), CompactStringFst<Arc>(ofst));
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

