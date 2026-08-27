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

// Unit tests for shrinking algorithms.

#include "opengrm/sfst/shrink.h"

#include <cmath>
#include <cstddef>
#include <set>     // NOLINT(misc-include-cleaner)
#include <string>  // NOLINT(misc-include-cleaner)
#include <vector>  // NOLINT(misc-include-cleaner)

#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/arcsort.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/smooth.h"

namespace sfst {

using Arc = fst::StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

template <class Arc>
size_t CountArcs(const fst::Fst<Arc>& fst) {
  size_t count = 0;
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    count += fst.NumArcs(siter.Value());
  }
  return count;
}

class ShrinkTest : public testing::Test {
 protected:
  void SetUp() override {
    fst_.AddState();
    fst_.SetStart(0);
    fst_.AddState();
    fst_.AddState();
    fst_.AddState();  // Backoff state.
    fst_.AddState();
    fst_.AddState();
    fst_.AddArc(0, Arc(1, 1, Weight(-std::log(10.0)), 1));
    fst_.AddArc(0, Arc(2, 2, Weight(-std::log(5.0)), 2));
    fst_.AddArc(0, Arc(0, 0, Weight(-std::log(15.0)), 3));  // Phi arc.
    fst_.AddArc(3, Arc(1, 1, Weight(-std::log(5.0)), 4));
    fst_.AddArc(3, Arc(2, 2, Weight(-std::log(2.0)), 5));
    fst_.SetFinal(1, Weight::One());
    fst_.SetFinal(2, Weight::One());
    fst_.SetFinal(3, Weight::One());
    fst_.SetFinal(4, Weight::One());
    fst_.SetFinal(5, Weight::One());
    fst::ArcSort(&fst_, fst::StdILabelCompare());
  }

  fst::VectorFst<Arc> fst_;
};

TEST_F(ShrinkTest, StolckeShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  const size_t initial_arcs = sfst::CountArcs(fst);
  ASSERT_TRUE(StolckeShrink(&fst, 0, 0.1));
  const size_t final_arcs = sfst::CountArcs(fst);
  EXPECT_LE(final_arcs, initial_arcs);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, StolckeThetaForMaxNGramsTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, /*phi_label=*/0));
  const double theta = StolckeThetaForMaxNGrams(fst, /*phi_label=*/0,
                                                /*target_number_of_ngrams=*/2);
  EXPECT_GT(theta, 0.0);
  ASSERT_TRUE(StolckeShrink(&fst, /*phi_label=*/0, theta));
  EXPECT_TRUE(IsNormalized(fst, /*phi_label=*/0));
}

TEST_F(ShrinkTest, SeymoreShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  const size_t initial_arcs = sfst::CountArcs(fst);
  ASSERT_TRUE(SeymoreShrink(&fst, 0, 0.1, 100.0));
  const size_t final_arcs = sfst::CountArcs(fst);
  EXPECT_LE(final_arcs, initial_arcs);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, AbsoluteSeymoreShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  const size_t initial_arcs = sfst::CountArcs(fst);
  ASSERT_TRUE(AbsoluteSeymoreShrink(&fst, 0, 0.1, 100.0));
  const size_t final_arcs = sfst::CountArcs(fst);
  EXPECT_LE(final_arcs, initial_arcs);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, RestrictedRelEntropyShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  const size_t initial_arcs = sfst::CountArcs(fst);
  ASSERT_TRUE(RestrictedRelEntropyShrink(&fst, 0, 0.1));
  const size_t final_arcs = sfst::CountArcs(fst);
  EXPECT_LE(final_arcs, initial_arcs);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, SymmetrizedRelEntropyShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  const size_t initial_arcs = sfst::CountArcs(fst);
  ASSERT_TRUE(SymmetrizedRelEntropyShrink(&fst, 0, 0.1));
  const size_t final_arcs = sfst::CountArcs(fst);
  EXPECT_LE(final_arcs, initial_arcs);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, SignificanceShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  const size_t initial_arcs = sfst::CountArcs(fst);
  ASSERT_TRUE(SignificanceShrink(&fst, 0));
  const size_t final_arcs = sfst::CountArcs(fst);
  EXPECT_LE(final_arcs, initial_arcs);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, WordShrinkTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(WittenBell(&fst, 0));
  absl::flat_hash_set<Label> word_set = {1};
  ASSERT_TRUE(WordShrink(&fst, 0, word_set));
  bool found_label_1 = false;
  bool found_label_2 = false;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 0); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == 1) found_label_1 = true;
    if (arc.ilabel == 2) found_label_2 = true;
  }
  EXPECT_TRUE(found_label_1);
  EXPECT_FALSE(found_label_2);
}

TEST_F(ShrinkTest, CountPruneTest) {
  fst::VectorFst<Arc> fst(fst_);
  ASSERT_TRUE(CountPrune(&fst, 0, "2:7"));
  bool found_label_1 = false;
  bool found_label_2 = false;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 0); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == 1) found_label_1 = true;
    if (arc.ilabel == 2) found_label_2 = true;
  }
  EXPECT_TRUE(found_label_1);
  EXPECT_FALSE(found_label_2);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST_F(ShrinkTest, CountPrunePatternValidationTest) {
  fst::VectorFst<Arc> fst(fst_);
  // Valid patterns
  EXPECT_TRUE(CountPrune(&fst, 0, "2:5"));
  fst = fst_;
  EXPECT_TRUE(CountPrune(&fst, 0, "2+:5"));
  fst = fst_;
  EXPECT_TRUE(CountPrune(&fst, 0, "1+:7"));
  bool found_label_2_s0 = false;
  bool found_label_2_s3 = false;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 0); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 2) found_label_2_s0 = true;
  }
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 3); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 2) found_label_2_s3 = true;
  }
  EXPECT_FALSE(found_label_2_s0);
  EXPECT_FALSE(found_label_2_s3);
  fst = fst_;
  EXPECT_TRUE(CountPrune(&fst, 0, "1:1; 2:7"));
  fst = fst_;
  EXPECT_TRUE(CountPrune(&fst, 0, "1:1;2:7;"));

  // Invalid patterns should safely return false without crashing
  fst = fst_;
  EXPECT_FALSE(CountPrune(&fst, 0, ""));
  EXPECT_FALSE(CountPrune(&fst, 0, ":7"));
  EXPECT_FALSE(CountPrune(&fst, 0, "2:"));
  EXPECT_FALSE(CountPrune(&fst, 0, "abc:7"));
  EXPECT_FALSE(CountPrune(&fst, 0, "2:abc"));
  EXPECT_FALSE(CountPrune(&fst, 0, "-1:5"));
  EXPECT_FALSE(CountPrune(&fst, 0, "0:5"));
  EXPECT_FALSE(CountPrune(&fst, 0, "2:5;invalid"));
}

TEST_F(ShrinkTest, ListPruneTest) {
  fst::VectorFst<Arc> fst(fst_);
  std::set<std::vector<Label>> ngrams_to_prune;
  ngrams_to_prune.insert({2});
  ASSERT_TRUE(ListPrune(&fst, 0, ngrams_to_prune));
  bool found_label_2_from_0 = false;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 0); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 2) found_label_2_from_0 = true;
  }
  EXPECT_FALSE(found_label_2_from_0);
  bool found_label_2_from_3 = false;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 3); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 2) found_label_2_from_3 = true;
  }
  EXPECT_FALSE(found_label_2_from_3);
  EXPECT_TRUE(IsNormalized(fst, 0));
}

TEST(NonCanonicalShrinkTest, OrphanBehaviorTest) {
  fst::VectorFst<Arc> fst;
  fst.AddState();
  fst.SetStart(0);  // Order 1.
  fst.AddState();   // State 1: Backoff node (order 1).
  fst.AddState();   // State 2: Higher order (order 2).
  fst.AddState();
  fst.AddState();  // Sinks.
  // 2. Enables probability flow 0 -> 2.
  fst.AddArc(0, Arc(10, 10, Weight(-std::log(100.0)), 2));
  // 3. State 1 (backoff) only has label 21.
  fst.AddArc(1, Arc(21, 21, Weight(-std::log(50.0)), 3));
  // 4. State 2 backs off to 1, but has orphan label 22.
  fst.AddArc(2, Arc(0, 0, Weight(-std::log(20.0)), 1));   // Phi.
  fst.AddArc(2, Arc(21, 21, Weight(-std::log(2.0)), 4));  // Paired (Weak).
  fst.AddArc(2, Arc(22, 22, Weight(-std::log(2.0)), 4));  // Orphan (Weak).
  fst.SetFinal(0, Weight::One());
  fst.SetFinal(1, Weight::One());
  fst.SetFinal(2, Weight::One());
  fst.SetFinal(3, Weight::One());
  fst.SetFinal(4, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());
  // Must skip Orphan 22 despite aggressive theta.
  ASSERT_TRUE(StolckeShrink(&fst, 0, 1.0));
  bool has_orphan = false;
  bool has_paired = false;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, 2); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 22) has_orphan = true;
    if (aiter.Value().ilabel == 21) has_paired = true;
  }
  // Automated defensive skip preserves the non-canonical orphan.
  EXPECT_TRUE(has_orphan);
  // Verifies paired arc WAS successfully pruned (eliminating false negatives).
  EXPECT_FALSE(has_paired);
}

TEST(NonCanonicalShrinkTest, CyclicTopologyRegressionTest) {
  fst::VectorFst<Arc> fst;
  fst.AddState();  // State 0.
  fst.AddState();  // State 1.
  fst.SetStart(0);
  // 0 <-> 1 phi loop.
  fst.AddArc(0, Arc(0, 0, Weight::One(), 1));
  fst.AddArc(1, Arc(0, 0, Weight::One(), 0));
  fst.SetFinal(0, Weight::One());
  fst.SetFinal(1, Weight::One());
  fst::ArcSort(&fst, fst::StdILabelCompare());
  // Verifies negative return status from shrinkers ensuring safety gates
  // operate.
  EXPECT_FALSE(StolckeShrink(&fst, 0, 1.0));
  EXPECT_FALSE(SeymoreShrink(&fst, 0, 1.0, 100.0));
}

}  // namespace sfst

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
