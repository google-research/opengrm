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

#include "openfst/compat/init.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/arcsort.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)

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

  fst::VectorFst<Arc> fst_;
};

TEST_F(SmoothTest, WittenBellTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  // We cannot easily use IsNormalized here because it expects a
  // backoff-complete FST or specific topology. Let's just check that it runs
  // and produces non-Zero weights.
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      ASSERT_NE(aiter.Value().weight, Weight::Zero());
    }
  }
}

TEST_F(SmoothTest, AbsoluteDiscountingTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(AbsoluteDiscounting(&fst, 0));
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      ASSERT_NE(aiter.Value().weight, Weight::Zero());
    }
  }
}

TEST_F(SmoothTest, UnsmoothedTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(Unsmoothed(&fst, 0));
}

TEST_F(SmoothTest, PreSmoothedTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(PreSmoothed(&fst, 0));
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      ASSERT_NE(aiter.Value().weight, Weight::Zero());
    }
  }
}

TEST_F(SmoothTest, KneserNeyTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(KneserNey(&fst, 0));
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      ASSERT_NE(aiter.Value().weight, Weight::Zero());
    }
  }
}

TEST_F(SmoothTest, KatzTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(Katz(&fst, 0));
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      ASSERT_NE(aiter.Value().weight, Weight::Zero());
      EXPECT_FALSE(std::isnan(aiter.Value().weight.Value()));
    }
  }
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
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      EXPECT_FALSE(std::isnan(aiter.Value().weight.Value()));
      EXPECT_NE(aiter.Value().weight, Weight::Zero());
    }
  }
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
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      EXPECT_FALSE(std::isnan(aiter.Value().weight.Value()));
    }
  }
}

}  // namespace sfst

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
