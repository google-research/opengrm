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

// Provides a total sort of the arcs for tests where the sorting of the arc
// labels is not significant.

#ifndef OPENGRM_THRAX_TEST_TOTAL_SORT_H_
#define OPENGRM_THRAX_TEST_TOTAL_SORT_H_

#include <cstdint>
#include <tuple>

#include "openfst/lib/arcsort.h"
#include "openfst/lib/equal.h"

namespace thrax {

template <class Arc>
class TotalOrdering {
 public:
  bool operator()(const Arc& arc1, const Arc& arc2) const {
    // NB: The use of `Value()` will fail to compile in general for non-float
    // weights.
    return std::forward_as_tuple(arc1.ilabel, arc1.olabel, arc1.nextstate,
                                 arc1.weight.Value()) <
           std::forward_as_tuple(arc2.ilabel, arc2.olabel, arc2.nextstate,
                                 arc2.weight.Value());
  }

  uint64_t Properties(uint64_t props) const {
    using ::fst::kAcceptor;
    using ::fst::kArcSortProperties;
    using ::fst::kILabelSorted;
    using ::fst::kOLabelSorted;
    return (props & kArcSortProperties) | kILabelSorted |
           (props & kAcceptor ? kOLabelSorted : 0);
  }
};

template <class Arc>
bool EqualUnordered(const ::fst::Fst<Arc>& fst1, const ::fst::Fst<Arc>& fst2,
                    float delta = ::fst::kDelta) {
  using ::fst::ArcSortFst;
  using ::fst::Equal;
  static const TotalOrdering<Arc> torder;
  return Equal(ArcSortFst<Arc, TotalOrdering<Arc>>(fst1, torder),
               ArcSortFst<Arc, TotalOrdering<Arc>>(fst2, torder), delta);
}

}  // namespace thrax

#endif  // OPENGRM_THRAX_TEST_TOTAL_SORT_H_
