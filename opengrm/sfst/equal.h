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

// Equal and Isomorphic for normalized stochastic FSTs.

#ifndef OPENGRM_SFST_EQUAL_H_
#define OPENGRM_SFST_EQUAL_H_

#include <cstdint>

#include "openfst/lib/arc-map.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/normalize.h"

namespace sfst {

namespace internal {

// Mapper to map weights on phi-labeled arcs to One()
template <class Arc>
class RmPhiWeightMapper {
 public:
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using FromArc = Arc;
  using ToArc = Arc;

  explicit RmPhiWeightMapper(Label phi_label) : phi_label_(phi_label) {}

  Arc operator()(const Arc& arc) const {
    return Arc(arc.ilabel, arc.olabel,
               arc.ilabel == phi_label_ ? Weight::One() : arc.weight,
               arc.nextstate);
  }

  constexpr fst::MapFinalAction FinalAction() const {
    return fst::MAP_NO_SUPERFINAL;
  }

  constexpr fst::MapSymbolsAction InputSymbolsAction() const {
    return fst::MAP_COPY_SYMBOLS;
  }

  constexpr fst::MapSymbolsAction OutputSymbolsAction() const {
    return fst::MAP_COPY_SYMBOLS;
  }

  uint64_t Properties(uint64_t props) const {
    return (props & fst::kWeightInvariantProperties) | fst::kUnweighted;
  }

 private:
  Label phi_label_;
};

}  // namespace internal

// Equal for normalized stochastic FSTs that deals with uninteresting
// variations in the failure weights. In particular, if the
// FSTs are normalized, comparing failure weights can be skipped since
// they are determined by the other weights.
template <class Arc>
bool Equal(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
           typename Arc::Label phi_label, float delta = fst::kDelta,
           uint32_t etype = fst::kEqualFsts) {
  using Mapper = internal::RmPhiWeightMapper<Arc>;

  if (!IsNormalized(fst1, phi_label, delta)) {
    FSTERROR() << "Equal: Input FST1 is not normalized";
    return false;
  }
  if (!IsNormalized(fst2, phi_label, delta)) {
    FSTERROR() << "Equal: Input FST2 is not normalized";
    return false;
  }

  if (phi_label == fst::kNoLabel) {
    return fst::Equal(fst1, fst2, delta, etype);
  } else {
    // Maps all failure weights to One() before comparing.
    Mapper mapper(phi_label);
    fst::ArcMapFst mfst1(fst1, mapper);
    fst::ArcMapFst mfst2(fst2, mapper);
    return fst::Equal(mfst1, mfst2, delta, etype);
  }
}

// Isomorphic for normalized stochastic FSTs that deals with uninteresting
// variations in the failure weights. In particular, if the
// FSTs are normalized, comparing failure weights can be skipped since
// they are determined by the other weights.
template <class Arc>
bool Isomorphic(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                typename Arc::Label phi_label, float delta = fst::kDelta) {
  using Mapper = internal::RmPhiWeightMapper<Arc>;

  if (!IsNormalized(fst1, phi_label, delta)) {
    FSTERROR() << "Isomorphic: Input FST1 is not normalized";
    return false;
  }
  if (!IsNormalized(fst2, phi_label, delta)) {
    FSTERROR() << "Isomorphic: Input FST2 is not normalized";
    return false;
  }

  if (phi_label == fst::kNoLabel) {
    return fst::Isomorphic(fst1, fst2, delta);
  } else {
    // Maps all failure weights to One() before comparing
    Mapper mapper(phi_label);
    fst::ArcMapFst mfst1(fst1, mapper);
    fst::ArcMapFst mfst2(fst2, mapper);
    return fst::Isomorphic(mfst1, mfst2, delta);
  }
}

}  // namespace sfst

#endif  // OPENGRM_SFST_EQUAL_H_
