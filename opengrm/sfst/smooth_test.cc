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

// Unit tests for smoothing algorithms.

#include "opengrm/sfst/smooth.h"

#include <cmath>  // NOLINT(misc-include-cleaner)
#include <vector>

#include "openfst/compat/init.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/arcsort.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/normalize.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

class SmoothTest : public testing::Test {
 protected:
  void SetUp() override {
    // Create a simple FST with counts.
    // State 0: start.
    // Arc 0->1 with label 1, count 10 (log domain: -log(10) = -2.3)
    // Arc 0->2 with label 2, count 5 (log domain: -log(5) = -1.6)
    // Phi arc 0->3 with label 0, count 15 (total count).
    fst_.AddState();
    fst_.SetStart(0);
    fst_.AddState();
    fst_.AddState();
    fst_.AddState();  // Backoff state
    fst_.AddState();  // State 4
    fst_.AddState();  // State 5
    fst_.AddArc(0, Arc(1, 1, Weight(-std::log(10.0)), 1));
    fst_.AddArc(0, Arc(2, 2, Weight(-std::log(5.0)), 2));
    fst_.AddArc(0, Arc(0, 0, Weight(-std::log(15.0)), 3));  // Phi arc
    fst_.AddArc(3, Arc(1, 1, Weight(-std::log(5.0)), 4));
    fst_.AddArc(3, Arc(2, 2, Weight(-std::log(2.0)), 5));
    // State 1: final.
    fst_.SetFinal(1, Weight::One());
    // State 2: final.
    fst_.SetFinal(2, Weight::One());
    // State 3: backoff state.
    fst_.SetFinal(3, Weight::One());
    // State 4: final.
    fst_.SetFinal(4, Weight::One());
    // State 5: final.
    fst_.SetFinal(5, Weight::One());
    fst::ArcSort(&fst_, fst::StdILabelCompare());
  }

  // Verifies that all transitions in the FST have non-zero and finite (non-NaN)
  // weights.
  static void CheckValidWeights(const fst::Fst<Arc>& fst) {
    for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
         siter.Next()) {
      StateId s = siter.Value();
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        EXPECT_NE(aiter.Value().weight, Weight::Zero());
        EXPECT_FALSE(std::isnan(aiter.Value().weight.Value()));
      }
    }
  }

  fst::VectorFst<Arc> fst_;
};

TEST_F(SmoothTest, WittenBellTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, AbsoluteDiscountingTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(AbsoluteDiscounting(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, UnsmoothedTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(Unsmoothed(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, PreSmoothedTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(PreSmoothed(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, KneserNeyTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(KneserNey(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, KatzTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(Katz(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, KatzDegenerateCountsTest) {
  // Construct an FST where N_1 = (bins + 1) * N_{bins + 1}, producing
  // rnorm = 1.0. With bins = 5: N_1 = 6, N_6 = 1.
  fst::VectorFst<Arc> fst;
  fst.AddState();  // State 0 (start).
  fst.SetStart(0);
  fst.AddState();  // State 1 (backoff).
  for (int i = 2; i <= 9; ++i) fst.AddState();

  // 6 arcs with count 1.0 (N_1 = 6).
  for (int l = 1; l <= 6; ++l) {
    fst.AddArc(0, Arc(l, l, Weight(-std::log(1.0)), l + 1));
    fst.SetFinal(l + 1, Weight::One());
  }
  // 1 arc with count 6.0 (N_6 = 1).
  fst.AddArc(0, Arc(7, 7, Weight(-std::log(6.0)), 8));
  fst.SetFinal(8, Weight::One());

  // Phi arc to backoff state with total count.
  fst.AddArc(0, Arc(0, 0, Weight(-std::log(12.0)), 1));
  fst.AddArc(1, Arc(1, 1, Weight(-std::log(5.0)), 9));
  fst.SetFinal(1, Weight::One());
  fst.SetFinal(9, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());

  // Smoothing must not crash or leak NaN when rnorm == 1.0 (1.0 - rnorm == 0).
  EXPECT_TRUE(Katz(&fst, 0, /*bins=*/5));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, KatzZeroSingletonsTest) {
  // Construct an FST with zero singletons (N_1 = 0, e.g. count pruned).
  fst::VectorFst<Arc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddState();  // Backoff state 1.
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.AddArc(0, Arc(1, 1, Weight(-std::log(4.0)), 2));
  fst.AddArc(0, Arc(2, 2, Weight(-std::log(4.0)), 3));
  fst.AddArc(0, Arc(0, 0, Weight(-std::log(8.0)), 1));
  fst.AddArc(1, Arc(1, 1, Weight(-std::log(2.0)), 4));
  fst.SetFinal(1, Weight::One());
  fst.SetFinal(2, Weight::One());
  fst.SetFinal(3, Weight::One());
  fst.SetFinal(4, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());

  EXPECT_TRUE(Katz(&fst, 0, /*bins=*/5));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, WittenBellNormalizationAndCustomKTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(WittenBell(&fst, 0, /*k=*/1.0));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);

  // Custom k = 0.5
  fst = fst_;
  EXPECT_TRUE(WittenBell(&fst, 0, /*k=*/0.5));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);

  // Custom k = 2.0
  fst = fst_;
  EXPECT_TRUE(WittenBell(&fst, 0, /*k=*/2.0));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, AbsoluteDiscountingNormalizationAndCustomDTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(AbsoluteDiscounting(&fst, 0, /*D=*/0.75));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);

  // Custom D = 0.5
  fst = fst_;
  EXPECT_TRUE(AbsoluteDiscounting(&fst, 0, /*D=*/0.5));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);

  // Custom D = 0.9
  fst = fst_;
  EXPECT_TRUE(AbsoluteDiscounting(&fst, 0, /*D=*/0.9));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, KneserNeyNormalizationAndCustomDTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(KneserNey(&fst, 0, /*D=*/0.75));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));

  // Custom D = 0.5
  fst = fst_;
  EXPECT_TRUE(KneserNey(&fst, 0, /*D=*/0.5));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));

  // Custom D = 0.9
  fst = fst_;
  EXPECT_TRUE(KneserNey(&fst, 0, /*D=*/0.9));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
}

TEST_F(SmoothTest, ModifiedKneserNeyTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(ModifiedKneserNey(&fst, 0, /*bins=*/3));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);

  // Custom bins = 2
  fst = fst_;
  EXPECT_TRUE(ModifiedKneserNey(&fst, 0, /*bins=*/2));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, PreSmoothedNormalizationTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(PreSmoothed(&fst, 0));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
}

TEST_F(SmoothTest, UnsmoothedNormalizationTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(Unsmoothed(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
}

TEST_F(SmoothTest, KatzNormalizationTest) {
  fst::VectorFst<Arc> fst(fst_);
  EXPECT_TRUE(Katz(&fst, 0, /*bins=*/3));
  EXPECT_TRUE(PhiNormalize(&fst, 0));
  EXPECT_TRUE(IsNormalized(fst, 0, 1e-4));
}

TEST_F(SmoothTest, AbsoluteDiscountingCountLessThanDiscountTest) {
  // Arcs with count 0.5 where discount D = 0.75.
  fst::VectorFst<Arc> fst;
  fst.AddState();  // State 0 (unigram).
  fst.SetStart(0);
  fst.AddState();  // State 1.
  fst.AddState();  // State 2 (higher-order state).
  fst.AddState();  // State 3.

  // State 0 (unigram): word 1 with count 10.0.
  fst.AddArc(0, Arc(1, 1, Weight(-std::log(10.0)), 1));
  fst.SetFinal(0, Weight::One());
  fst.SetFinal(1, Weight::One());

  // State 2: count 0.5 (< D = 0.75), phi backoff to state 0 with count 1.0.
  fst.AddArc(2, Arc(1, 1, Weight(-std::log(0.5)), 3));
  fst.AddArc(2, Arc(0, 0, Weight(-std::log(1.0)), 0));  // phi arc
  fst.SetFinal(3, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());

  EXPECT_TRUE(AbsoluteDiscounting(&fst, 0, /*D=*/0.75));
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 2); !aiter.Done();
       aiter.Next()) {
    EXPECT_FALSE(std::isnan(aiter.Value().weight.Value()));
  }
}

TEST_F(SmoothTest, PreSmoothedZeroBackoffMassTest) {
  // State where outgoing arc counts sum exactly to state count c_h
  // (backoff_mass = 0).
  fst::VectorFst<Arc> fst;
  fst.AddState();  // State 0 (unigram).
  fst.SetStart(0);
  fst.AddState();  // State 1.
  fst.AddState();  // State 2 (higher order).
  fst.AddState();  // State 3.

  fst.AddArc(0, Arc(1, 1, Weight(-std::log(10.0)), 1));
  fst.SetFinal(0, Weight::One());
  fst.SetFinal(1, Weight::One());

  // State 2: outgoing arc count 5.0, phi total count 5.0 => remainder = 0.
  fst.AddArc(2, Arc(1, 1, Weight(-std::log(5.0)), 3));
  fst.AddArc(2, Arc(0, 0, Weight(-std::log(5.0)), 0));  // phi arc
  fst.SetFinal(3, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());

  EXPECT_TRUE(PreSmoothed(&fst, 0));
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 2); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    EXPECT_FALSE(std::isnan(arc.weight.Value()));
    if (arc.ilabel == 0) {
      // Backoff arc receives Weight::Zero() because backoff mass was 0.
      EXPECT_EQ(arc.weight, Weight::Zero());
    }
  }
}

TEST_F(SmoothTest, KneserNeyUnigramModelTest) {
  // Model with order 1 only (no phi backoff arcs).
  fst::VectorFst<Arc> fst;
  fst.AddState();  // State 0.
  fst.SetStart(0);
  fst.AddState();  // State 1.
  fst.AddState();  // State 2.
  fst.AddArc(0, Arc(1, 1, Weight(-std::log(3.0)), 1));
  fst.AddArc(0, Arc(2, 2, Weight(-std::log(7.0)), 2));
  fst.SetFinal(1, Weight::One());
  fst.SetFinal(2, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());

  EXPECT_TRUE(KneserNey(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, WittenBellUnigramModelTest) {
  // Model with order 1 only (no phi backoff arcs).
  fst::VectorFst<Arc> fst;
  fst.AddState();  // State 0.
  fst.SetStart(0);
  fst.AddState();  // State 1.
  fst.AddState();  // State 2.
  fst.AddArc(0, Arc(1, 1, Weight(-std::log(4.0)), 1));
  fst.AddArc(0, Arc(2, 2, Weight(-std::log(6.0)), 2));
  fst.SetFinal(1, Weight::One());
  fst.SetFinal(2, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());

  EXPECT_TRUE(WittenBell(&fst, 0));
  CheckValidWeights(fst);
}

TEST_F(SmoothTest, NonCanonicalInputRejected) {
  // FST with a cycle on phi transitions (label 0) is non-canonical.
  fst::VectorFst<Arc> phi_cyclic_fst;
  phi_cyclic_fst.AddState();
  phi_cyclic_fst.AddState();
  phi_cyclic_fst.SetStart(0);
  phi_cyclic_fst.AddArc(0, Arc(0, 0, Weight(0.5), 1));
  phi_cyclic_fst.AddArc(1, Arc(0, 0, Weight(0.5), 0));
  phi_cyclic_fst.SetFinal(0, Weight::One());
  phi_cyclic_fst.SetFinal(1, Weight::One());

  fst::VectorFst<Arc> fst;
  fst = phi_cyclic_fst;
  EXPECT_FALSE(Unsmoothed(&fst, 0));
  fst = phi_cyclic_fst;
  EXPECT_FALSE(WittenBell(&fst, 0));
  fst = phi_cyclic_fst;
  EXPECT_FALSE(AbsoluteDiscounting(&fst, 0));
  fst = phi_cyclic_fst;
  EXPECT_FALSE(PreSmoothed(&fst, 0));
  fst = phi_cyclic_fst;
  EXPECT_FALSE(KneserNey(&fst, 0));
  fst = phi_cyclic_fst;
  EXPECT_FALSE(Katz(&fst, 0));
}

TEST(ComputeCountHistogramTest, AccumulatesHistogramBinsCorrectly) {
  using Arc = fst::StdArc;
  using Weight = Arc::Weight;
  fst::VectorFst<Arc> fst;

  fst.AddState();  // State 0: Order 2 state.
  fst.SetStart(0);
  fst.AddState();  // State 1: Order 1 (backoff) state.
  fst.AddState();  // State 2: Destination state.
  fst.AddState();  // State 3: Destination state.

  // State 0 (Order 2):
  // - Phi arc to State 1 (label 0, weight -log(10.0)) -> ignored by histogram
  // - Arc 1 (label 1, count 1.0) -> bin 1 (Order 2)
  // - Arc 2 (label 2, count 2.0) -> bin 2 (Order 2)
  // - Arc 3 (label 3, count 2.1) -> rounds to bin 2 (Order 2)
  // - Arc 4 (label 4, count 5.0) -> for bins=3, count 5 > bins+1=4, so ignored
  // - Final weight = -log(1.0) (count 1.0) -> bin 1 (Order 2)
  fst.AddArc(0, Arc(0, 0, Weight(-std::log(10.0)), 1));
  fst.AddArc(0, Arc(1, 1, Weight(-std::log(1.0)), 2));
  fst.AddArc(0, Arc(2, 2, Weight(-std::log(2.0)), 2));
  fst.AddArc(0, Arc(3, 3, Weight(-std::log(2.1)), 2));
  fst.AddArc(0, Arc(4, 4, Weight(-std::log(5.0)), 2));
  fst.SetFinal(0, Weight(-std::log(1.0)));

  // State 1 (Order 1):
  // - Arc 1 (label 1, count 1.0) -> bin 1 (Order 1)
  // - Arc 2 (label 2, count 3.0) -> bin 3 (Order 1)
  // - Arc 3 (label 3, count 4.0) -> bin 4 (Order 1)
  // - Final weight = -log(3.0) (count 3.0) -> bin 3 (Order 1)
  fst.AddArc(1, Arc(1, 1, Weight(-std::log(1.0)), 2));
  fst.AddArc(1, Arc(2, 2, Weight(-std::log(3.0)), 3));
  fst.AddArc(1, Arc(3, 3, Weight(-std::log(4.0)), 3));
  fst.SetFinal(1, Weight(-std::log(3.0)));

  // State 2 & 3: Final weights Zero (no counts)
  fst.SetFinal(2, Weight::Zero());
  fst.SetFinal(3, Weight::Zero());

  fst::ArcSort(&fst, fst::StdILabelCompare());

  std::vector<int> orders;
  int max_order = PhiStateOrder(fst, 0, &orders);
  EXPECT_EQ(max_order, 2);
  ASSERT_GE(orders.size(), 2);
  EXPECT_EQ(orders[0], 2);
  EXPECT_EQ(orders[1], 1);

  int bins = 3;
  std::vector<std::vector<double>> count_of_counts;
  internal::ComputeCountHistogram(fst, /*phi_label=*/0, bins, orders, max_order,
                                  &count_of_counts);

  // Dimensions: (max_order + 1) x (bins + 2) -> 3 x 5
  ASSERT_EQ(count_of_counts.size(), 3);
  ASSERT_EQ(count_of_counts[0].size(), 5);
  ASSERT_EQ(count_of_counts[1].size(), 5);
  ASSERT_EQ(count_of_counts[2].size(), 5);

  // Order 0: unused, all zeros.
  for (int r = 0; r <= bins + 1; ++r) {
    EXPECT_DOUBLE_EQ(count_of_counts[0][r], 0.0);
  }

  // Order 1:
  // r=1: 1 (arc count 1.0)
  // r=2: 0
  // r=3: 2 (arc count 3.0, final weight count 3.0)
  // r=4: 1 (arc count 4.0)
  EXPECT_DOUBLE_EQ(count_of_counts[1][0], 0.0);
  EXPECT_DOUBLE_EQ(count_of_counts[1][1], 1.0);
  EXPECT_DOUBLE_EQ(count_of_counts[1][2], 0.0);
  EXPECT_DOUBLE_EQ(count_of_counts[1][3], 2.0);
  EXPECT_DOUBLE_EQ(count_of_counts[1][4], 1.0);

  // Order 2:
  // r=1: 2 (arc count 1.0, final weight count 1.0)
  // r=2: 2 (arc count 2.0, arc count 2.1)
  // r=3: 0
  // r=4: 0 (arc count 5.0 is > bins+1=4, so not counted)
  EXPECT_DOUBLE_EQ(count_of_counts[2][0], 0.0);
  EXPECT_DOUBLE_EQ(count_of_counts[2][1], 2.0);
  EXPECT_DOUBLE_EQ(count_of_counts[2][2], 2.0);
  EXPECT_DOUBLE_EQ(count_of_counts[2][3], 0.0);
  EXPECT_DOUBLE_EQ(count_of_counts[2][4], 0.0);
}

TEST(ComputeAbsoluteDiscountsTest, AllDiscountingModes) {
  int max_order = 2;
  // count_of_counts[order][r]
  // Order 1: n1 = 100, n2 = 50, n3 = 20, n4 = 5
  // Order 2: sparse / zeros to test fallback
  std::vector<std::vector<double>> count_of_counts = {
      {},                             // Order 0 (unused)
      {0.0, 100.0, 50.0, 20.0, 5.0},  // Order 1
      {0.0, 0.0, 0.0, 0.0, 0.0},      // Order 2 (all zeros)
  };

  std::vector<std::vector<double>> discounts;

  // Mode 1: Constant discount (bins = 1, D = 0.75).
  internal::ComputeAbsoluteDiscounts(/*bins=*/1, /*D=*/0.75, max_order,
                                     count_of_counts, &discounts);
  ASSERT_EQ(discounts.size(), max_order + 1);
  EXPECT_NEAR(discounts[1][1], 0.75, 1e-6);
  EXPECT_NEAR(discounts[2][1], 0.75, 1e-6);

  // Mode 2: Data-driven Good-Turing estimate (bins = 1, D = -1.0).
  // For order 1: Y = 100 / (100 + 2*50) = 0.5.
  // d1 = 1.0 - 2.0 * 0.5 * 50 / 100 = 0.5.
  internal::ComputeAbsoluteDiscounts(/*bins=*/1, /*D=*/-1.0, max_order,
                                     count_of_counts, &discounts);
  EXPECT_NEAR(discounts[1][1], 0.5, 1e-6);
  // For order 2 (empty histogram), fallback is used (0.75 or finite in (0, 1)).
  EXPECT_GT(discounts[2][1], 0.0);
  EXPECT_LT(discounts[2][1], 1.0);

  // Mode 3: Modified Kneser-Ney 3-bin count-dependent discounting (bins = 3).
  // For order 1:
  // Y = 0.5
  // D1 = 1 - 2 * 0.5 * (50 / 100) = 0.5
  // D2 = 2 - 3 * 0.5 * (20 / 50) = 1.4
  // D3 = 3 - 4 * 0.5 * (5 / 20) = 2.5
  internal::ComputeAbsoluteDiscounts(/*bins=*/3, /*D=*/-1.0, max_order,
                                     count_of_counts, &discounts);
  ASSERT_GE(discounts[1].size(), 4);
  EXPECT_NEAR(discounts[1][1], 0.5, 1e-6);
  EXPECT_NEAR(discounts[1][2], 1.4, 1e-6);
  EXPECT_NEAR(discounts[1][3], 2.5, 1e-6);

  // For order 2 (zero histogram), fallbacks D1 in (0, 1), D2 in (0, 2), D3 in
  // (0, 3).
  EXPECT_GT(discounts[2][1], 0.0);
  EXPECT_LT(discounts[2][1], 1.0);
  EXPECT_GT(discounts[2][2], 0.0);
  EXPECT_LT(discounts[2][2], 2.0);
  EXPECT_GT(discounts[2][3], 0.0);
  EXPECT_LT(discounts[2][3], 3.0);
}

}  // namespace sfst

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
