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

// Unit tests for Baum-Welch expectation tables.

#include "opengrm/baumwelch/expectation-table.h"

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/baumwelch/log-adder.h"

namespace fst {
namespace {

TEST(UnweightedArcTest, ConstructorsAndAccessors) {
  const StdArc arc(1, 2, StdArc::Weight(0.5), 3);

  // From Arc.
  const internal::UnweightedArc<StdArc> uarc1(arc);
  EXPECT_EQ(uarc1.ilabel, 1);
  EXPECT_EQ(uarc1.olabel, 2);
  EXPECT_EQ(uarc1.nextstate, 3);

  // Explicit constructor.
  const internal::UnweightedArc<StdArc> uarc2(1, 2, 3);
  EXPECT_EQ(uarc2.ilabel, 1);
  EXPECT_EQ(uarc2.olabel, 2);
  EXPECT_EQ(uarc2.nextstate, 3);

  // Default constructor (used for final weight).
  const internal::UnweightedArc<StdArc> uarc_default;
  EXPECT_EQ(uarc_default.ilabel, kNoLabel);
  EXPECT_EQ(uarc_default.olabel, kNoLabel);
  EXPECT_EQ(uarc_default.nextstate, kNoStateId);
}

TEST(UnweightedArcTest, EqualityAndHashing) {
  const internal::UnweightedArc<StdArc> arc1(1, 2, 3);
  const internal::UnweightedArc<StdArc> arc2(1, 2, 3);
  const internal::UnweightedArc<StdArc> arc3(1, 2, 4);
  const internal::UnweightedArc<StdArc> arc4(1, 5, 3);
  const internal::UnweightedArc<StdArc> arc5(6, 2, 3);

  EXPECT_EQ(arc1, arc2);
  EXPECT_NE(arc1, arc3);
  EXPECT_NE(arc1, arc4);
  EXPECT_NE(arc1, arc5);

  const internal::UnweightedArcHash<internal::UnweightedArc<StdArc>> hasher;
  EXPECT_EQ(hasher(arc1), hasher(arc2));
  EXPECT_NE(hasher(arc1), hasher(arc3));
}

using ArcTypes = ::testing::Types<StdArc, LogArc, Log64Arc>;

template <typename Arc>
class ExpectationTableTest : public ::testing::Test {};

TYPED_TEST_SUITE(ExpectationTableTest, ArcTypes, );

TYPED_TEST(ExpectationTableTest, StateExpectationTableZeroOrUnobserved) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  channel.SetStart(s0);

  const StateExpectationTable<Arc> table(channel);

  // Unobserved state / arc should return Zero().
  const Arc unobserved_arc(1, 2, Weight::One(), s1);
  EXPECT_EQ(table.Backward(s0, unobserved_arc), Weight::Zero());
  EXPECT_EQ(table.Backward(s0), Weight::Zero());
  EXPECT_EQ(table.Backward(s1, unobserved_arc), Weight::Zero());
}

TYPED_TEST(ExpectationTableTest, StateExpectationTableSingleArc) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  channel.SetStart(s0);

  StateExpectationTable<Arc> table(channel);
  const Weight w(1.5);
  table.Forward(s0, 10, 20, w, s1);

  const Arc arc(10, 20, Weight::One(), s1);
  // Expectation normalized by total likelihood should equal
  // Weight::One() (prob 1.0).
  EXPECT_EQ(table.Backward(s0, arc), Weight::One());
  // Final weight for s0 was not observed, so it's Zero().
  EXPECT_EQ(table.Backward(s0), Weight::Zero());
}

TYPED_TEST(ExpectationTableTest, StateExpectationTableMultipleArcsAndFinal) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  const auto s2 = channel.AddState();
  channel.SetStart(s0);

  StateExpectationTable<Arc> table(channel);

  const Weight w1(1.0);
  const Weight w2(2.0);
  const Weight wf(0.5);

  table.Forward(s0, 1, 10, w1, s1);
  table.Forward(s0, 2, 20, w2, s2);
  table.Forward(s0, wf);

  const Arc arc1(1, 10, Weight::One(), s1);
  const Arc arc2(2, 20, Weight::One(), s2);

  const Weight bw1 = table.Backward(s0, arc1);
  const Weight bw2 = table.Backward(s0, arc2);
  const Weight bwf = table.Backward(s0);

  // Check that Backward weights sum to Weight::One() via LogAdder.
  LogAdder<Weight> sum;
  sum.Add(bw1);
  sum.Add(bw2);
  sum.Add(bwf);
  EXPECT_NEAR(sum.Sum().Value(), Weight::One().Value(), 1e-5);
}

TYPED_TEST(ExpectationTableTest, StateExpectationTableAccumulation) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  channel.SetStart(s0);

  StateExpectationTable<Arc> table(channel);

  const Weight w1(1.0);
  const Weight w2(2.0);

  // Forward same arc twice.
  table.Forward(s0, 1, 10, w1, s1);
  table.Forward(s0, 1, 10, w2, s1);

  const Arc arc(1, 10, Weight::One(), s1);
  // Total expectation for arc is LogAdder(w1, w2), total likelihood
  // is also LogAdder(w1, w2), so Backward should be Weight::One().
  EXPECT_EQ(table.Backward(s0, arc), Weight::One());
}

TYPED_TEST(ExpectationTableTest, StateILabelExpectationTableZeroOrUnobserved) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  channel.SetStart(s0);

  const StateILabelExpectationTable<Arc> table(channel);

  const Arc unobserved_arc(1, 2, Weight::One(), s1);
  EXPECT_EQ(table.Backward(s0, unobserved_arc), Weight::Zero());
  EXPECT_EQ(table.Backward(s0), Weight::Zero());
}

TYPED_TEST(ExpectationTableTest,
           StateILabelExpectationTableConditioningOnILabel) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  const auto s2 = channel.AddState();
  channel.SetStart(s0);

  StateILabelExpectationTable<Arc> table(channel);

  // Two arcs with ilabel = 1, different olabels/nextstates.
  const Weight w1_a(1.0);
  const Weight w1_b(2.0);
  table.Forward(s0, 1, 10, w1_a, s1);
  table.Forward(s0, 1, 20, w1_b, s2);

  // One arc with ilabel = 2.
  const Weight w2(0.5);
  table.Forward(s0, 2, 30, w2, s1);

  // Final weight (ilabel = kNoLabel).
  const Weight wf(1.5);
  table.Forward(s0, wf);

  const Arc arc1_a(1, 10, Weight::One(), s1);
  const Arc arc1_b(1, 20, Weight::One(), s2);
  const Arc arc2(2, 30, Weight::One(), s1);

  const Weight bw1_a = table.Backward(s0, arc1_a);
  const Weight bw1_b = table.Backward(s0, arc1_b);
  const Weight bw2 = table.Backward(s0, arc2);
  const Weight bwf = table.Backward(s0);

  // bw1_a and bw1_b should be normalized against ilabel=1 total likelihood.
  LogAdder<Weight> sum_ilabel1;
  sum_ilabel1.Add(bw1_a);
  sum_ilabel1.Add(bw1_b);
  EXPECT_NEAR(sum_ilabel1.Sum().Value(), Weight::One().Value(), 1e-5);

  // bw2 is only arc with ilabel=2, so should be Weight::One().
  EXPECT_EQ(bw2, Weight::One());

  // bwf is only final weight for s0, so should be Weight::One().
  EXPECT_EQ(bwf, Weight::One());
}

TYPED_TEST(ExpectationTableTest, StateILabelExpectationTableAccumulation) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> channel;
  const auto s0 = channel.AddState();
  const auto s1 = channel.AddState();
  channel.SetStart(s0);

  StateILabelExpectationTable<Arc> table(channel);

  const Weight w1(1.0);
  const Weight w2(2.0);

  table.Forward(s0, 1, 10, w1, s1);
  table.Forward(s0, 1, 10, w2, s1);

  const Arc arc(1, 10, Weight::One(), s1);
  EXPECT_EQ(table.Backward(s0, arc), Weight::One());
}

}  // namespace
}  // namespace fst
