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

// Unit tests for Baum-Welch randomization functions.

#include "opengrm/baumwelch/randomize.h"

#include <cmath>

#include "gtest/gtest.h"
#include "absl/random/random.h"
#include "openfst/compat/seed_sequences.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"

namespace fst {
namespace {

using ArcTypes = ::testing::Types<StdArc, LogArc, Log64Arc>;

template <typename Arc>
class RandomizeTest : public ::testing::Test {};

TYPED_TEST_SUITE(RandomizeTest, ArcTypes, );

TYPED_TEST(RandomizeTest, LogUniformInExpectedRange) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  absl::BitGen bit_gen(absl::SeedSeq{1, 2, 3, 4, 5});
  const double max_expected = -std::log(kDelta) + 1e-5;

  int different_count = 0;
  const Weight first_w = internal::LogUniform<Weight>(bit_gen);

  for (int i = 0; i < 100; ++i) {
    const Weight w = internal::LogUniform<Weight>(bit_gen);
    EXPECT_FALSE(std::isnan(w.Value()));
    EXPECT_FALSE(std::isinf(w.Value()));
    EXPECT_GE(w.Value(), 0.0);
    EXPECT_LE(w.Value(), max_expected);
    if (w != first_w) {
      ++different_count;
    }
  }

  EXPECT_GT(different_count, 90);
}

TYPED_TEST(RandomizeTest, RandomizeEmptyFst) {
  using Arc = TypeParam;
  VectorFst<Arc> fst;
  absl::BitGen bit_gen(absl::SeedSeq{1, 2, 3, 4, 5});
  Randomize(bit_gen, &fst);
  EXPECT_EQ(fst.NumStates(), 0);
}

TYPED_TEST(RandomizeTest, RandomizeUpdatesArcAndFinalWeights) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  fst.SetStart(s0);

  // s0: non-final, 2 outgoing arcs.
  fst.AddArc(s0, Arc(1, 10, Weight(0.0), s1));
  fst.AddArc(s0, Arc(2, 20, Weight(5.0), s2));

  // s1: final, 1 outgoing arc.
  fst.AddArc(s1, Arc(1, 10, Weight(0.0), s1));
  fst.SetFinal(s1, Weight::One());

  // s2: non-final, 0 outgoing arcs.
  fst.SetFinal(s2, Weight::Zero());

  absl::BitGen bit_gen(absl::SeedSeq{1, 2, 3, 4, 5});
  Randomize(bit_gen, &fst);

  // Verify s0 arcs and final weight.
  EXPECT_EQ(fst.Final(s0), Weight::Zero());
  ArcIterator<VectorFst<Arc>> aiter0(fst, s0);
  EXPECT_FALSE(aiter0.Done());
  const Weight w0_0 = aiter0.Value().weight;
  EXPECT_GE(w0_0.Value(), 0.0);
  EXPECT_LE(w0_0.Value(), -std::log(kDelta) + 1e-5);

  aiter0.Next();
  EXPECT_FALSE(aiter0.Done());
  const Weight w0_1 = aiter0.Value().weight;
  EXPECT_GE(w0_1.Value(), 0.0);
  EXPECT_LE(w0_1.Value(), -std::log(kDelta) + 1e-5);

  // Verify s1 arc and final weight.
  ArcIterator<VectorFst<Arc>> aiter1(fst, s1);
  EXPECT_FALSE(aiter1.Done());
  const Weight w1_0 = aiter1.Value().weight;
  EXPECT_GE(w1_0.Value(), 0.0);

  const Weight f1 = fst.Final(s1);
  EXPECT_NE(f1, Weight::Zero());
  EXPECT_GE(f1.Value(), 0.0);
  EXPECT_LE(f1.Value(), -std::log(kDelta) + 1e-5);

  // Verify s2 final weight remains Zero.
  EXPECT_EQ(fst.Final(s2), Weight::Zero());
}

}  // namespace
}  // namespace fst
