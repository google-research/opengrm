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

// Unit tests for stochastic FST canonical topology checks and state ordering.

#include "opengrm/sfst/canonical.h"

#include <vector>

#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/globals.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {
namespace {

using fst::StdArc;
using fst::VectorFst;
using StateId = StdArc::StateId;

TEST(CanonicalTest, PhiTopOrderAcyclic) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // Regular transitions: 0 -> 1, 1 -> 2.
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(2, 2, StdArc::Weight::One(), 2));

  // Phi transitions (ilabel 0): 2 -> 1, 1 -> 0.
  fst.AddArc(2, StdArc(0, 0, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(0, 0, StdArc::Weight::One(), 0));

  std::vector<StateId> top_order;
  EXPECT_TRUE(PhiTopOrder(fst, 0, &top_order));
  ASSERT_EQ(top_order.size(), 3);
  // With phi arcs 2 -> 1 -> 0, state 2 must precede state 1, which precedes
  // state 0.
  EXPECT_LT(top_order[2], top_order[1]);
  EXPECT_LT(top_order[1], top_order[0]);
}

TEST(CanonicalTest, PhiTopOrderCyclic) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // Cycle of phi transitions: 0 -> 1 -> 0 with ilabel 0.
  fst.AddArc(0, StdArc(0, 0, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(0, 0, StdArc::Weight::One(), 0));

  std::vector<StateId> top_order;
  EXPECT_FALSE(PhiTopOrder(fst, 0, &top_order));
}

TEST(CanonicalTest, IsCanonicalNoPhiLabel) {
  VectorFst<StdArc> sorted_fst;
  sorted_fst.AddState();
  sorted_fst.AddState();
  sorted_fst.AddState();
  sorted_fst.SetStart(0);
  sorted_fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  sorted_fst.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 2));
  EXPECT_TRUE(IsCanonical(sorted_fst, fst::kNoLabel));

  VectorFst<StdArc> unsorted_fst;
  unsorted_fst.AddState();
  unsorted_fst.AddState();
  unsorted_fst.AddState();
  unsorted_fst.SetStart(0);
  unsorted_fst.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 2));
  unsorted_fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  EXPECT_FALSE(IsCanonical(unsorted_fst, fst::kNoLabel));
}

TEST(CanonicalTest, IsCanonicalValidWithPhiLabel) {
  constexpr StdArc::Label kPhiLabel = 100;
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // State 0: regular arc with ilabel 1, and phi arc with ilabel 100 -> State 1.
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  fst.AddArc(0, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));

  // State 1: regular arc with ilabel 1, and phi arc with ilabel 100 -> State 2.
  fst.AddArc(1, StdArc(1, 1, StdArc::Weight::One(), 2));
  fst.AddArc(1, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 2));

  // State 2: regular arc only.
  fst.AddArc(2, StdArc(1, 1, StdArc::Weight::One(), 2));

  std::vector<StateId> top_order;
  EXPECT_TRUE(IsCanonical(fst, kPhiLabel, &top_order));
  EXPECT_EQ(top_order.size(), 3);
  EXPECT_TRUE(IsCanonical(fst, kPhiLabel));
}

TEST(CanonicalTest, IsCanonicalMultiplePhiTransitions) {
  constexpr StdArc::Label kPhiLabel = 100;
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // Two outgoing phi arcs from State 0.
  fst.AddArc(0, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));
  fst.AddArc(0, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));

  EXPECT_FALSE(IsCanonical(fst, kPhiLabel));
}

TEST(CanonicalTest, IsCanonicalCyclicPhiTransitions) {
  constexpr StdArc::Label kPhiLabel = 100;
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // Create a 2-state cycle of phi transitions: State 0 -> State 1 -> State 0.
  fst.AddArc(0, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 0));

  EXPECT_FALSE(IsCanonical(fst, kPhiLabel));
}

TEST(CanonicalTest, PhiStateOrderNoPhiLabel) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  std::vector<int> state_order;
  EXPECT_EQ(PhiStateOrder(fst, fst::kNoLabel, &state_order), 1);
  ASSERT_EQ(state_order.size(), 3);
  EXPECT_EQ(state_order[0], 1);
  EXPECT_EQ(state_order[1], 1);
  EXPECT_EQ(state_order[2], 1);
}

TEST(CanonicalTest, PhiStateOrderPhiChain) {
  constexpr StdArc::Label kPhiLabel = 100;
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // Phi chain: State 2 -> State 1 -> State 0.
  fst.AddArc(2, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 0));

  std::vector<int> state_order;
  EXPECT_EQ(PhiStateOrder(fst, kPhiLabel, &state_order), 3);
  ASSERT_EQ(state_order.size(), 3);
  EXPECT_EQ(state_order[0], 1);
  EXPECT_EQ(state_order[1], 2);
  EXPECT_EQ(state_order[2], 3);
}

// Tests that calling PhiStateOrder on an FST containing cyclic phi transitions
// triggers a fatal error.
TEST(CanonicalTest, PhiStateOrderCyclicDeath) {
  constexpr StdArc::Label kPhiLabel = 100;
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // Create a 2-state cycle of phi transitions: State 0 -> State 1 -> State 0.
  fst.AddArc(0, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 0));

  std::vector<int> state_order;
  EXPECT_DEATH(PhiStateOrder(fst, kPhiLabel, &state_order),
               "PhiStateOrder: input FST not canonical");
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
