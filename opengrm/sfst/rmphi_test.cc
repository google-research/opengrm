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

// Unit tests for RmPhi mapping and correction in stochastic FSTs.

#include "opengrm/sfst/rmphi.h"

#include <cmath>

#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/globals.h"
#include "openfst/lib/arc-range.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace sfst {
namespace {

using fst::ArcIterator;
using fst::SignedLog64Arc;
using fst::SignedLog64Weight;
using fst::StdArc;
using fst::VectorFst;

TEST(RmPhiTest, RmPhiNoLabel) {
  VectorFst<StdArc> ifst;
  ifst.AddState();
  ifst.AddState();
  ifst.SetStart(0);
  ifst.AddArc(0, StdArc(1, 1, StdArc::Weight(-std::log(0.5)), 1));
  ifst.SetFinal(1, StdArc::Weight::One());

  VectorFst<SignedLog64Arc> ofst;
  internal::RmPhi(ifst, &ofst, fst::kNoLabel);
  ASSERT_TRUE(fst::Verify(ofst));
  ASSERT_EQ(ofst.NumStates(), 2);

  ArcIterator<VectorFst<SignedLog64Arc>> aiter(ofst, 0);
  ASSERT_FALSE(aiter.Done());
  const auto& arc = aiter.Value();
  EXPECT_EQ(arc.ilabel, 1);
  EXPECT_EQ(arc.olabel, 1);
  EXPECT_EQ(arc.nextstate, 1);
  EXPECT_NEAR(arc.weight.Value2().Value(), -std::log(0.5), 1e-4);
}

TEST(RmPhiTest, RmPhiSingleStateTransition) {
  VectorFst<StdArc> ifst;
  ifst.AddState();
  ifst.AddState();
  ifst.SetStart(0);
  const StdArc::Label kPhi = 10;
  ifst.AddArc(0, StdArc(kPhi, kPhi, StdArc::Weight(-std::log(0.5)), 1));
  ifst.SetFinal(1, StdArc::Weight::One());

  VectorFst<SignedLog64Arc> ofst;
  internal::RmPhi(ifst, &ofst, kPhi, fst::MATCHER_REWRITE_ALWAYS);
  ASSERT_TRUE(fst::Verify(ofst));
  ASSERT_EQ(ofst.NumStates(), 2);

  ArcIterator<VectorFst<SignedLog64Arc>> aiter(ofst, 0);
  ASSERT_FALSE(aiter.Done());
  const auto& arc = aiter.Value();
  // Failure label kPhi rewritten as epsilon (0)
  EXPECT_EQ(arc.ilabel, 0);
  EXPECT_EQ(arc.olabel, 0);
  EXPECT_EQ(arc.nextstate, 1);
}

TEST(RmPhiTest, RmPhiCorrectionWeightSubtraction) {
  VectorFst<StdArc> ifst;
  ifst.AddState();
  ifst.AddState();
  ifst.AddState();
  ifst.SetStart(0);

  const StdArc::Label kPhi = 10;
  // State 0 -> State 1 via regular arc with prob 0.4
  ifst.AddArc(0, StdArc(1, 1, StdArc::Weight(-std::log(0.4)), 1));
  // State 0 -> State 2 via phi transition with prob 0.5
  ifst.AddArc(0, StdArc(kPhi, kPhi, StdArc::Weight(-std::log(0.5)), 2));

  // State 2 -> State 1 via regular arc with prob 0.2
  ifst.AddArc(2, StdArc(1, 1, StdArc::Weight(-std::log(0.2)), 1));
  ifst.SetFinal(1, StdArc::Weight::One());

  VectorFst<SignedLog64Arc> ofst;
  internal::RmPhi(ifst, &ofst, kPhi, fst::MATCHER_REWRITE_AUTO);
  ASSERT_TRUE(fst::Verify(ofst));
  ASSERT_EQ(ofst.NumStates(), 3);

  // State 0 should have two outgoing arcs:
  // 1) The regular arc 0 -> 1, whose weight is corrected from 0.4 to:
  //    0.4 - (phi_prob * fail_arc_prob) = 0.4 - (0.5 * 0.2) = 0.3
  // 2) The phi arc 0 -> 2 rewritten as epsilon (0)
  bool found_corrected_arc = false;
  bool found_epsilon_arc = false;
  for (const auto& arc : fst::GetArcs(ofst, 0)) {
    if (arc.ilabel == 1 && arc.nextstate == 1) {
      EXPECT_NEAR(arc.weight.Value2().Value(), -std::log(0.3), 1e-4);
      found_corrected_arc = true;
    } else if (arc.ilabel == 0 && arc.nextstate == 2) {
      EXPECT_NEAR(arc.weight.Value2().Value(), -std::log(0.5), 1e-4);
      found_epsilon_arc = true;
    }
  }
  EXPECT_TRUE(found_corrected_arc);
  EXPECT_TRUE(found_epsilon_arc);
}

TEST(RmPhiTest, RmPhiFinalWeightCorrection) {
  VectorFst<StdArc> ifst;
  ifst.AddState();
  ifst.AddState();
  ifst.SetStart(0);

  const StdArc::Label kPhi = 10;
  // State 0 has final weight prob 0.5
  ifst.SetFinal(0, StdArc::Weight(-std::log(0.5)));
  // State 0 -> State 1 via phi with prob 0.4
  ifst.AddArc(0, StdArc(kPhi, kPhi, StdArc::Weight(-std::log(0.4)), 1));
  // State 1 has final weight prob 0.25
  ifst.SetFinal(1, StdArc::Weight(-std::log(0.25)));

  VectorFst<SignedLog64Arc> ofst;
  internal::RmPhi(ifst, &ofst, kPhi);
  ASSERT_TRUE(fst::Verify(ofst));

  // Corrected final weight of State 0:
  // 0.5 - (0.4 * 0.25) = 0.5 - 0.10 = 0.40
  SignedLog64Weight s0_final = ofst.Final(0);
  EXPECT_NEAR(s0_final.Value2().Value(), -std::log(0.40), 1e-4);
}

TEST(RmPhiTest, RmPhiNegativeCorrectionArcAdded) {
  VectorFst<StdArc> ifst;
  ifst.AddState();
  ifst.AddState();
  ifst.AddState();
  ifst.AddState();
  ifst.SetStart(0);

  const StdArc::Label kPhi = 10;
  // State 0 -> State 1 via regular arc with prob 0.1
  ifst.AddArc(0, StdArc(1, 1, StdArc::Weight(-std::log(0.1)), 1));
  // State 0 -> State 2 via phi transition with prob 0.5
  ifst.AddArc(0, StdArc(kPhi, kPhi, StdArc::Weight(-std::log(0.5)), 2));

  // State 2 -> State 3 via regular arc with prob 0.4
  ifst.AddArc(2, StdArc(1, 1, StdArc::Weight(-std::log(0.4)), 3));
  ifst.SetFinal(1, StdArc::Weight::One());
  ifst.SetFinal(3, StdArc::Weight::One());

  VectorFst<SignedLog64Arc> ofst;
  internal::RmPhi(ifst, &ofst, kPhi);
  ASSERT_TRUE(fst::Verify(ofst));

  // Since 0.1 - (0.5 * 0.4) = 0.1 - 0.2 = -0.1 (< 0), RmPhiMapper cannot
  // subtract from the 0 -> 1 arc. Instead, it adds a negative corrective arc
  // with weight -0.2 (where Value1 is -1.0 indicating negative sign).
  bool found_neg_arc = false;
  for (const auto& arc : fst::GetArcs(ofst, 0)) {
    if (arc.ilabel == 1 && arc.nextstate == 3) {
      EXPECT_LT(arc.weight.Value1().Value(), 0.0);
      EXPECT_NEAR(arc.weight.Value2().Value(), -std::log(0.2), 1e-4);
      found_neg_arc = true;
    }
  }
  EXPECT_TRUE(found_neg_arc);
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
