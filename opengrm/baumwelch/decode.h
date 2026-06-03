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

// This header defines functions for various types of decoding.
//
// We use whatever compaction is appropriate for the output FARs.

#ifndef OPENGRM_BAUMWELCH_DECODE_H_
#define OPENGRM_BAUMWELCH_DECODE_H_

#include <numeric>
#include <vector>

#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/project.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/statesort.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/baumwelch/a-star.h"
#include "opengrm/baumwelch/cascade.h"

namespace fst {
namespace internal {

// Helper function that reverses the state numbering of the output of single
// ShortestPath. This is necessary because the single shortest path produces
// an automaton with descendingly ordered state numbering, and this in turn
// is not compatible with the definition of the kString property assumed by
// string compactor.
template <class Arc>
void ReverseStateNumbering(MutableFst<Arc>* fst) {
  using StateId = typename Arc::StateId;
  // TODO: use std::ranges::iota_view once C++20 is more widely available.
  std::vector<StateId> order(fst->NumStates());
  std::iota(order.rbegin(), order.rend(), 0);
  StateSort(fst, order);
}

// Helper function that decides whether to use Viterbi or A* decoding, then
// reverses the state numbering.
template <class Arc>
void ShortestPathOrString(const Fst<Arc>& ifst, MutableFst<Arc>* ofst) {
  if constexpr (IsPath<typename Arc::Weight>::value) {
    ShortestPath(ifst, ofst);
  } else {
    AStarSingleShortestString(ifst, ofst);
  }
  ReverseStateNumbering(ofst);
}

// Decipherment decoding.
template <class Arc>
void Decode(const Fst<Arc>& plaintext_model, FarReader<Arc>& ciphertext,
            const Fst<Arc>& channel_model, FarWriter<Arc>& hypotext) {
  for (; !ciphertext.Done(); ciphertext.Next()) {
    const SimpleCascade<Arc> cascade(plaintext_model, *ciphertext.GetFst(),
                                     channel_model);
    VectorFst<Arc> ofst;
    {
      VectorFst<Arc> tfst;
      Project(cascade.GetFst(), &tfst, ProjectType::INPUT);
      RmEpsilon(&tfst);
      ShortestPathOrString(tfst, &ofst);
    }
    hypotext.Add(ciphertext.GetKey(), CompactWeightedStringFst<Arc>(ofst));
  }
}

// Pair decoding; if an EncodeMapper is provided, weight removal, label
// encoding, and compaction is applied to prepare the data for pair n-gram
// model fitting.
template <class Arc>
void Decode(FarReader<Arc>& input, FarReader<Arc>& output,
            const Fst<Arc>& model, FarWriter<Arc>& hypotext,
            EncodeMapper<Arc>* encoder = nullptr) {
  while (!input.Done() && !output.Done()) {
    const SimpleCascade<Arc> cascade(*input.GetFst(), *output.GetFst(), model);
    VectorFst<Arc> tfst;
    ShortestPathOrString(cascade.GetFst(), &tfst);
    if (encoder) {
      static RmWeightMapper<Arc> rmweight;
      ArcMap(&tfst, rmweight);
      RmEpsilon(&tfst);
      Encode(&tfst, encoder);
      hypotext.Add(input.GetKey() + "_" + output.GetKey(),
                   CompactStringFst<Arc>(tfst));
    } else {
      hypotext.Add(input.GetKey() + "_" + output.GetKey(), ConstFst<Arc>(tfst));
    }
    input.Next();
    output.Next();
  }
}

}  // namespace internal

// Selects the right function.
template <class Arc>
void Decode(FarReader<Arc>& input, FarReader<Arc>& output,
            const Fst<Arc>& model, FarWriter<Arc>& hypotext,
            EncodeMapper<Arc>* encoder = nullptr) {
  if (input.Type() == FarType::FST) {
    internal::Decode(*input.GetFst(), output, model, hypotext);
  } else {
    internal::Decode(input, output, model, hypotext, encoder);
  }
}

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_DECODE_H_
