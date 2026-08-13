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

// Unit tests for stochastic FST stationary distribution algorithms.

#include <cmath>
#include <vector>

#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/globals.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/stationary-distrib.h"

namespace sfst {
namespace {

using fst::StdArc;
using fst::VectorFst;

TEST(StationaryDistribTest, SingleStateSelfLoop) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  // Self-loop on state 0 with probability 1.0 (weight 0.0).
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 0));
  fst.SetFinal(0, StdArc::Weight::Zero());

  std::vector<StdArc::Weight> weights;
  EXPECT_TRUE(StationaryDistrib(fst, &weights, StdArc::Weight::One()));
  ASSERT_EQ(weights.size(), 1);
  // p = 1.0 corresponds to StdArc::Weight(0.0).
  EXPECT_NEAR(weights[0].Value(), 0.0, 1e-4);
}

TEST(StationaryDistribTest, TwoStateDeterministicLoop) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);
  // State 0 -> State 1 with probability 1.0.
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  fst.SetFinal(0, StdArc::Weight::Zero());
  // State 1 -> State 0 with probability 1.0.
  fst.AddArc(1, StdArc(2, 2, StdArc::Weight::One(), 0));
  fst.SetFinal(1, StdArc::Weight::Zero());

  std::vector<StdArc::Weight> weights;
  EXPECT_TRUE(StationaryDistrib(fst, &weights, StdArc::Weight::One()));
  ASSERT_EQ(weights.size(), 2);
  // Both states should have stationary probability 0.5 (-log(0.5)).
  EXPECT_NEAR(weights[0].Value(), -std::log(0.5), 1e-4);
  EXPECT_NEAR(weights[1].Value(), -std::log(0.5), 1e-4);
}

TEST(StationaryDistribTest, TwoStateWithFinalAndAlpha) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);
  // State 0 -> State 1 with probability 1.0.
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  fst.SetFinal(0, StdArc::Weight::Zero());
  // State 1 has no outgoing arcs, but has probability 1.0 final weight.
  fst.SetFinal(1, StdArc::Weight::One());

  std::vector<StdArc::Weight> weights;
  // Using alpha = One() causes re-entry into start state 0 with probability 1.
  EXPECT_TRUE(StationaryDistrib(fst, &weights, StdArc::Weight::One()));
  ASSERT_EQ(weights.size(), 2);
  EXPECT_NEAR(weights[0].Value(), -std::log(0.5), 1e-4);
  EXPECT_NEAR(weights[1].Value(), -std::log(0.5), 1e-4);
}

TEST(StationaryDistribTest, ThreeStateMarkovChain) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // State 0: self-loop p=0.5, transition to state 1 with p=0.5.
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight(-std::log(0.5)), 0));
  fst.AddArc(0, StdArc(2, 2, StdArc::Weight(-std::log(0.5)), 1));
  fst.SetFinal(0, StdArc::Weight::Zero());

  // State 1: transition to state 0 with p=0.25, to state 2 with p=0.75.
  fst.AddArc(1, StdArc(3, 3, StdArc::Weight(-std::log(0.25)), 0));
  fst.AddArc(1, StdArc(4, 4, StdArc::Weight(-std::log(0.75)), 2));
  fst.SetFinal(1, StdArc::Weight::Zero());

  // State 2: transition to state 0 with p=1.0.
  fst.AddArc(2, StdArc(5, 5, StdArc::Weight::One(), 0));
  fst.SetFinal(2, StdArc::Weight::Zero());

  std::vector<StdArc::Weight> weights;
  EXPECT_TRUE(StationaryDistrib(fst, &weights, StdArc::Weight::One(),
                                fst::kNoLabel, 1e-6, 1000));
  ASSERT_EQ(weights.size(), 3);

  // Theoretical stationary distribution: (8/15, 4/15, 3/15).
  EXPECT_NEAR(weights[0].Value(), -std::log(8.0 / 15.0), 1e-4);
  EXPECT_NEAR(weights[1].Value(), -std::log(4.0 / 15.0), 1e-4);
  EXPECT_NEAR(weights[2].Value(), -std::log(3.0 / 15.0), 1e-4);
}

TEST(StationaryDistribTest, EpsilonTransitionCheck) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);
  // State 0 -> State 1 via epsilon (ilabel 0) with probability 1.0.
  fst.AddArc(0, StdArc(0, 1, StdArc::Weight::One(), 1));
  fst.SetFinal(0, StdArc::Weight::Zero());
  // State 1 -> State 0 via regular arc with probability 1.0.
  fst.AddArc(1, StdArc(1, 1, StdArc::Weight::One(), 0));
  fst.SetFinal(1, StdArc::Weight::Zero());

  std::vector<StdArc::Weight> weights;
  EXPECT_TRUE(StationaryDistrib(fst, &weights, StdArc::Weight::One()));
  ASSERT_EQ(weights.size(), 2);
  // Because State 1 only receives an epsilon transition (ilabel 0), probability
  // mass is pushed directly through State 1 to State 0.
  EXPECT_NEAR(weights[0].Value(), 0.0, 1e-4);
  EXPECT_EQ(weights[1], StdArc::Weight::Zero());
}

TEST(StationaryDistribTest, EpsilonCycleShouldFail) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);
  // State 0 -> State 1 via epsilon.
  fst.AddArc(0, StdArc(0, 1, StdArc::Weight::One(), 1));
  fst.SetFinal(0, StdArc::Weight::Zero());
  // State 1 -> State 0 via epsilon (creates epsilon cycle).
  fst.AddArc(1, StdArc(0, 1, StdArc::Weight::One(), 0));
  fst.SetFinal(1, StdArc::Weight::Zero());

  std::vector<StdArc::Weight> weights;
  EXPECT_FALSE(StationaryDistrib(fst, &weights, StdArc::Weight::One()));
}

TEST(StationaryDistribTest, SignedStationaryDistribDirect) {
  VectorFst<fst::SignedLog64Arc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, fst::SignedLog64Arc(
                    1, 1,
                    fst::SignedLog64Weight(fst::SignedLog64Weight::W1(1.0),
                                           fst::SignedLog64Weight::W2(0.0)),
                    1));
  fst.AddArc(1, fst::SignedLog64Arc(
                    2, 2,
                    fst::SignedLog64Weight(fst::SignedLog64Weight::W1(1.0),
                                           fst::SignedLog64Weight::W2(0.0)),
                    0));

  std::vector<fst::SignedLog64Weight> weights;
  EXPECT_TRUE(internal::SignedStationaryDistrib(
      fst, &weights, internal::kReEntryWeight, fst::kDelta, 100));
  ASSERT_EQ(weights.size(), 2);
  EXPECT_NEAR(weights[0].Value2().Value(), -std::log(0.5), 1e-4);
  EXPECT_NEAR(weights[1].Value2().Value(), -std::log(0.5), 1e-4);
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
