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

#ifndef OPENGRM_BAUMWELCH_RANDOMIZE_H_
#define OPENGRM_BAUMWELCH_RANDOMIZE_H_

#include <cmath>

#include "absl/random/bit_gen_ref.h"
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/weight.h"

namespace fst {
namespace internal {

// Random weight generator in the (real) interval [kDelta, 1).
template <class Weight>
Weight LogUniform(absl::BitGenRef bit_gen) {
  double p = absl::Uniform(bit_gen, kDelta, 1.0);
  return Weight(-std::log(p));
}

}  // namespace internal

template <class Arc>
void Randomize(absl::BitGenRef bit_gen, MutableFst<Arc>* fst) {
  using Weight = typename Arc::Weight;
  for (StateIterator<MutableFst<Arc>> siter(*fst); !siter.Done();
       siter.Next()) {
    const auto state = siter.Value();
    // Arcs leaving this state.
    for (MutableArcIterator<MutableFst<Arc>> aiter(fst, state); !aiter.Done();
         aiter.Next()) {
      auto arc = aiter.Value();
      arc.weight = internal::LogUniform<Weight>(bit_gen);
      aiter.SetValue(arc);
    }
    // Final weight.
    if (fst->Final(state) != Weight::Zero()) {
      fst->SetFinal(state, internal::LogUniform<Weight>(bit_gen));
    }
  }
}

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_RANDOMIZE_H_
