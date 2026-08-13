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

// Unit tests for core SFST mathematical functions in sfst.h.

#include "opengrm/sfst/sfst.h"

#include <cmath>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"

namespace sfst {
namespace {

TEST(NegLogDiffTest, BasicDifference) {
  // P_a = 0.8, P_b = 0.3 => P_{diff} = 0.5
  double a = -std::log(0.8);
  double b = -std::log(0.3);
  double expected = -std::log(0.5);
  EXPECT_NEAR(NegLogDiff(a, b), expected, 1e-12);
}

TEST(NegLogDiffTest, ZeroWeightAsSecondOperand) {
  double a = 2.5;
  double b = fst::StdArc::Weight::Zero().Value();
  EXPECT_DOUBLE_EQ(NegLogDiff(a, b), a);
}

TEST(NegLogDiffTest, SmallDifferencePrecision) {
  double a = 1.0;
  double b = a + 1e-8;
  double result = NegLogDiff(a, b);
  double delta = b - a;
  double expected = a - std::log(-std::expm1(-delta));
  EXPECT_NEAR(result, expected, 1e-12);
}

TEST(NegLogDiffTest, LargeDifferenceNoOverflow) {
  // delta = 800 (would overflow std::exp(800) to +inf in naive implementation)
  double a = 1.0;
  double b = a + 800.0;
  EXPECT_DOUBLE_EQ(NegLogDiff(a, b), a);
}

TEST(NegLogDiffTest, NonPositiveDifferenceSetsError) {
  bool error = false;
  double zero_val = fst::StdArc::Weight::Zero().Value();
  // a == b
  EXPECT_DOUBLE_EQ(NegLogDiff(2.0, 2.0, &error), zero_val);
  EXPECT_FALSE(error);

  // a > b by more than kNormEps
  EXPECT_DOUBLE_EQ(NegLogDiff(2.5, 2.0, &error), zero_val);
  EXPECT_TRUE(error);
}

TEST(NegLogSumTest, BasicSum) {
  // P_a = 0.3, P_b = 0.4 => P_{sum} = 0.7
  double a = -std::log(0.3);
  double b = -std::log(0.4);
  double expected = -std::log(0.7);
  EXPECT_NEAR(NegLogSum(a, b), expected, 1e-12);
}

TEST(NegLogSumTest, ZeroWeightOperands) {
  double a = 2.5;
  double zero_val = fst::StdArc::Weight::Zero().Value();
  EXPECT_DOUBLE_EQ(NegLogSum(a, zero_val), a);
  EXPECT_DOUBLE_EQ(NegLogSum(zero_val, a), a);
}

}  // namespace
}  // namespace sfst
