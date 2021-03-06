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
  static const RmWeightMapper<Arc> rmweight;
  ArcMap(fst, rmweight);
}

// Pair decoding.
template <class Arc>
CompactUnweightedFst<Arc> DecodePair(const Fst<Arc> &ifst) {
  VectorFst<Arc> ofst;
  if constexpr (IsPath<typename Arc::Weight>::value) {
    ShortestPath(ifst, &ofst);
  } else {
    AStarSingleShortestPath(ifst, &ofst);
  }
  MakeString(&ofst);
  return CompactUnweightedFst<Arc>(ofst);
}

// Decipherment decoding.
template <class Arc>
CompactStringFst<Arc> DecodeDecipherment(const Fst<Arc> &ifst) {
  VectorFst<Arc> lattice;
  Project(ifst, &lattice, ProjectType::INPUT);
  RmEpsilon(&lattice);
  VectorFst<Arc> ofst;
  if constexpr (IsPath<typename Arc::Weight>::value) {
    ShortestPath(lattice, &ofst);
  } else {
    AStarSingleShortestPath(lattice, &ofst);
  }
  MakeString(&ofst);
  return CompactStringFst<Arc>(ofst);
}

}  // namespace internal

// Full decipherment setup.
template <class Arc>
void DecodeBaumWelch(FarReader<Arc> *input, FarReader<Arc> *output,
                     const Fst<Arc> &model, FarWriter<Arc> *hypotext) {
  while (!input->Done() && !output->Done()) {
    const SimpleCascade<Arc> cascade(*input->GetFst(), *output->GetFst(),
                                     model);
    if (input->Type() == FarType::FST) {
      hypotext->Add(output->GetKey(),
                    internal::DecodeDecipherment(cascade.GetFst()));
    } else {
      hypotext->Add(input->GetKey() + "_" + output->GetKey(),
                    internal::DecodePair(cascade.GetFst()));
      input->Next();
    }
    output->Next();
  }
}

}  // namespace fst

#endif  // BAUMWELCH_DECODE_H_

