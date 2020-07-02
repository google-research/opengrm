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

#ifndef BAUMWELCH_RANDOMIZE_H_
#define BAUMWELCH_RANDOMIZE_H_

#include <cmath>
#include <random>

#include <fst/types.h>
#include <fst/mutable-fst.h>

namespace fst {
namespace internal {

// Random weight generator in the (real) interval [kDelta, 1).
template <class Weight>
class LogUniformGenerator {
 public:
  using ValueType = typename Weight::ValueType;

  explicit LogUniformGenerator(uint64 seed = std::random_device()())
      : rand_(seed), dist_(kDelta, 1.0) {}

  Weight operator()() const { return Weight(-std::log(dist_(rand_))); }

 private:
  mutable std::mt19937_64 rand_;
  mutable std::uniform_real_distribution<ValueType> dist_;
};

}  // namespace internal

template <class Arc>
void RandomizeBaumWelch(MutableFst<Arc> *fst,
                        uint64 seed = std::random_device()()) {
  using Weight = typename Arc::Weight;
  const internal::LogUniformGenerator<Weight> generator(seed);
  for (StateIterator<MutableFst<Arc>> siter(*fst); !siter.Done();
       siter.Next()) {
    const auto state = siter.Value();
    // Arcs leaving this state.
    for (MutableArcIterator<MutableFst<Arc>> aiter(fst, state); !aiter.Done();
         aiter.Next()) {
      auto arc = aiter.Value();
      arc.weight = generator();
      aiter.SetValue(arc);
    }
    // Final weight.
    if (fst->Final(state) != Weight::Zero()) fst->SetFinal(state, generator());
  }
}

}  // namespace fst

#endif  // BAUMWELCH_RANDOMIZE_H_

