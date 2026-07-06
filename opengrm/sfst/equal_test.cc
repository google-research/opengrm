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

// Unit tests for normalized stochastic FST equality and isomorphism algorithms.

#include "opengrm/sfst/equal.h"

#include <cmath>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/globals.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/normalize.h"

namespace sfst {
namespace {

using fst::StdArc;
using fst::VectorFst;
using StateId = StdArc::StateId;
using Weight = StdArc::Weight;
using Label = StdArc::Label;

class EqualTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // earnest LM (normalized sfst with phi_label = 0).
    const std::string sfst1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/earnest.mod");
    sfst1_.reset(fst::Fst<StdArc>::Read(sfst1_name));

    // earnest_plus LM (NOT normalized).
    const std::string sfst3_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/earnest_plus.mod");
    sfst3_.reset(fst::Fst<StdArc>::Read(sfst3_name));
  }

  std::unique_ptr<fst::Fst<StdArc>> sfst1_;
  std::unique_ptr<fst::Fst<StdArc>> sfst3_;
};

TEST_F(EqualTest, RmPhiWeightMapperTest) {
  constexpr Label kPhiLabel = 100;
  internal::RmPhiWeightMapper<StdArc> mapper(kPhiLabel);

  StdArc phi_arc(kPhiLabel, kPhiLabel, Weight(0.5), 1);
  StdArc mapped_phi = mapper(phi_arc);
  EXPECT_EQ(mapped_phi.weight, Weight::One());

  StdArc regular_arc(1, 1, Weight(0.5), 1);
  StdArc mapped_regular = mapper(regular_arc);
  EXPECT_EQ(mapped_regular.weight, Weight(0.5));
}

TEST_F(EqualTest, NoPhiLabelEqualAndUnequal) {
  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0, StdArc(1, 1, Weight(-std::log(0.5)), 1));
  fst1.AddArc(0, StdArc(2, 2, Weight(-std::log(0.5)), 2));
  fst1.SetFinal(1, Weight::One());
  fst1.SetFinal(2, Weight::One());
  fst::ArcSort(&fst1, fst::ILabelCompare<StdArc>());
  ASSERT_TRUE(LocalNormalize(&fst1));

  VectorFst<StdArc> fst2(fst1);
  EXPECT_TRUE(sfst::Equal(fst1, fst2, fst::kNoLabel));
  EXPECT_TRUE(sfst::Isomorphic(fst1, fst2, fst::kNoLabel));

  VectorFst<StdArc> fst3;
  fst3.AddState();
  fst3.AddState();
  fst3.AddState();
  fst3.SetStart(0);
  fst3.AddArc(0, StdArc(1, 1, Weight(-std::log(0.25)), 1));
  fst3.AddArc(0, StdArc(2, 2, Weight(-std::log(0.75)), 2));
  fst3.SetFinal(1, Weight::One());
  fst3.SetFinal(2, Weight::One());
  fst::ArcSort(&fst3, fst::ILabelCompare<StdArc>());
  ASSERT_TRUE(LocalNormalize(&fst3));

  EXPECT_FALSE(sfst::Equal(fst1, fst3, fst::kNoLabel));
  EXPECT_FALSE(sfst::Isomorphic(fst1, fst3, fst::kNoLabel));
}

TEST_F(EqualTest, WithPhiLabelEqualAndIsomorphic) {
  ASSERT_TRUE(sfst1_ != nullptr);
  VectorFst<StdArc> fst2(*sfst1_);

  EXPECT_TRUE(sfst::Equal(*sfst1_, fst2, 0));
  EXPECT_TRUE(sfst::Isomorphic(*sfst1_, fst2, 0));
}

TEST_F(EqualTest, UnnormalizedInputDeath) {
  ASSERT_TRUE(sfst1_ != nullptr);
  ASSERT_TRUE(sfst3_ != nullptr);

  EXPECT_DEATH(sfst::Equal(*sfst1_, *sfst3_, 0),
               "Equal: Input FST2 is not normalized");
  EXPECT_DEATH(sfst::Equal(*sfst3_, *sfst1_, 0),
               "Equal: Input FST1 is not normalized");
  EXPECT_DEATH(sfst::Isomorphic(*sfst1_, *sfst3_, 0),
               "Isomorphic: Input FST2 is not normalized");
  EXPECT_DEATH(sfst::Isomorphic(*sfst3_, *sfst1_, 0),
               "Isomorphic: Input FST1 is not normalized");
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
