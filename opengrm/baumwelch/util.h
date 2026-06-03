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

#ifndef OPENGRM_BAUMWELCH_UTIL_H_
#define OPENGRM_BAUMWELCH_UTIL_H_

// Utility functions.

#include <algorithm>
#include <cstddef>
#include <vector>

namespace fst {
namespace internal {

// Computes the number of explored states in a distance vector. This is used to
// log the expansion of the DFA in A* search.
template <class Weight>
size_t ExploredStates(const std::vector<Weight>& distance) {
  static const auto nonzero_weight = [](const Weight& weight) {
    return weight != Weight::Zero();
  };
  return std::count_if(distance.begin(), distance.end(), nonzero_weight);
}

}  // namespace internal
}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_UTIL_H_
