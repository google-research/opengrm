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

// Unit tests for sfst::NGramCounter.

#include "opengrm/sfst/ngram-count.h"

#include <cmath>
#include <cstddef>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc-range.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {
namespace {

using ::fst::Log64Arc;
using ::fst::LogArc;
using ::fst::StdArc;
using ::fst::VectorFst;

// Helper to construct a string FST from label IDs.
VectorFst<StdArc> MakeStringFst(const std::vector<int>& labels,
                                double weight = 0.0) {
  VectorFst<StdArc> fst;
  auto current = fst.AddState();
  fst.SetStart(current);
  for (int label : labels) {
    auto next = fst.AddState();
    fst.AddArc(current, StdArc(label, label, StdArc::Weight(0.0), next));
    current = next;
  }
  fst.SetFinal(current, StdArc::Weight(weight));
  return fst;
}

TEST(NGramCounterTest, InvalidOrderSetError) {
  NGramCounter<LogArc::Weight> counter(/*order=*/0);
  EXPECT_TRUE(counter.Error());

  VectorFst<LogArc> example_fst;
  EXPECT_FALSE(counter.Count(example_fst));
}

TEST(NGramCounterTest, CountUnigramStringFst) {
  VectorFst<StdArc> string_fst = MakeStringFst({1, 2});

  NGramCounter<LogArc::Weight> counter(/*order=*/1);
  EXPECT_FALSE(counter.Error());
  EXPECT_TRUE(counter.Count(string_fst));

  VectorFst<StdArc> out_fst;
  counter.GetFst(&out_fst);

  EXPECT_GT(out_fst.NumStates(), 0u);
  EXPECT_NE(out_fst.Start(), fst::kNoStateId);
}

TEST(NGramCounterTest, CountBigramStringFst) {
  VectorFst<StdArc> string_fst = MakeStringFst({1, 2});

  NGramCounter<LogArc::Weight> counter(/*order=*/2);
  EXPECT_TRUE(counter.Count(string_fst));

  VectorFst<StdArc> out_fst;
  counter.GetFst(&out_fst);

  EXPECT_GT(out_fst.NumStates(), 0u);
  EXPECT_NE(out_fst.Start(), fst::kNoStateId);
}

TEST(NGramCounterTest, CountTopSortedFst) {
  VectorFst<Log64Arc> example_fst;
  auto start = example_fst.AddState();
  example_fst.SetStart(start);
  auto end = example_fst.AddState();
  example_fst.SetFinal(end, Log64Arc::Weight::One());

  // Path 1: "1 2" with prob 0.5.
  auto state1 = example_fst.AddState();
  example_fst.AddArc(start, Log64Arc(1, 1, -log(0.5), state1));
  example_fst.AddArc(state1, Log64Arc(2, 2, Log64Arc::Weight::One(), end));

  // Path 2: "3" with prob 0.5.
  example_fst.AddArc(start, Log64Arc(3, 3, -log(0.5), end));

  NGramCounter<LogArc::Weight> counter(/*order=*/2);
  EXPECT_TRUE(counter.Count(example_fst));

  VectorFst<StdArc> out_fst;
  counter.GetFst(&out_fst);

  EXPECT_GT(out_fst.NumStates(), 0u);
}

TEST(NGramCounterTest, GetFstWithPhiLabel) {
  VectorFst<StdArc> string_fst = MakeStringFst({1, 2});

  NGramCounter<LogArc::Weight> counter(/*order=*/2);
  EXPECT_TRUE(counter.Count(string_fst));

  VectorFst<StdArc> out_fst;
  constexpr int kPhiLabel = 99;
  counter.GetFst(&out_fst, kPhiLabel);

  bool found_phi = false;
  for (size_t s = 0; s < out_fst.NumStates(); ++s) {
    for (const auto& arc : fst::GetArcs(out_fst, s)) {
      if (arc.ilabel == kPhiLabel) {
        found_phi = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_phi);
}

TEST(NGramCounterTest, EpsilonAsBackoff) {
  VectorFst<StdArc> fst;
  auto s0 = fst.AddState();
  auto s1 = fst.AddState();
  auto s2 = fst.AddState();
  auto s3 = fst.AddState();
  fst.SetStart(s0);
  fst.AddArc(s0, StdArc(1, 1, 0.0, s1));
  fst.AddArc(s1, StdArc(0, 0, 0.0, s2));
  fst.AddArc(s2, StdArc(2, 2, 0.0, s3));
  fst.SetFinal(s3, StdArc::Weight::One());

  NGramCounter<LogArc::Weight> counter(/*order=*/2,
                                       /*epsilon_as_backoff=*/true);
  EXPECT_TRUE(counter.Count(fst));

  VectorFst<StdArc> out_fst;
  counter.GetFst(&out_fst);
  EXPECT_GT(out_fst.NumStates(), 0u);
}

TEST(NGramCounterTest, NonCoaccessibleInput) {
  VectorFst<StdArc> fst;
  auto s0 = fst.AddState();
  auto s1 = fst.AddState();
  auto s_dead = fst.AddState();  // Dead state (no path to final).
  fst.SetStart(s0);
  fst.AddArc(s0, StdArc(1, 1, 0.0, s1));
  fst.AddArc(s0, StdArc(2, 2, 0.0, s_dead));
  fst.SetFinal(s1, StdArc::Weight::One());

  NGramCounter<LogArc::Weight> counter(/*order=*/2);
  // Count(MutableFst*) should automatically connect (remove dead states).
  EXPECT_TRUE(counter.Count(&fst));

  VectorFst<StdArc> out_fst;
  counter.GetFst(&out_fst);
  EXPECT_GT(out_fst.NumStates(), 0u);
}

}  // namespace
}  // namespace sfst
