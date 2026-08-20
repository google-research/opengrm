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

// Unit tests for normalization operations and checks for stochastic FSTs.

#include "opengrm/sfst/normalize.h"

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

const float kAlgoDelta = 1.0e-7;  // Algorithm delta
const float kTestDelta = 1.0e-2;  // Check delta

// Test class.
class NormalizeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // earnest LM
    const std::string sfst1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/earnest.mod");
    sfst1_.reset(fst::Fst<Arc>::Read(sfst1_name));

    // earnest LM U earnest LM but with the initial-added epsilon replaced with
    // a vocabulary word (word 100, which is not o.w. initial).
    const std::string sfst2_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/earnest2.mod");
    sfst2_.reset(fst::Fst<Arc>::Read(sfst2_name));

    // closure+ of earnest LM.
    const std::string sfst3_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/earnest_plus.mod");
    sfst3_.reset(fst::Fst<Arc>::Read(sfst3_name));
  }

  std::unique_ptr<fst::Fst<Arc>> sfst1_;
  std::unique_ptr<fst::Fst<Arc>> sfst2_;
  std::unique_ptr<fst::Fst<Arc>> sfst3_;
};

// Checks canonical topology
TEST_F(NormalizeTest, IsCanonicalTest) {
  ASSERT_TRUE(IsCanonical(*sfst1_, 0));
  ASSERT_TRUE(IsCanonical(*sfst2_, 0));
  ASSERT_FALSE(IsCanonical(*sfst3_, 0));
}

// Checks state order
TEST_F(NormalizeTest, StateOrderTest) {
  std::vector<int> state_order;
  int max_order = PhiStateOrder(*sfst1_, 0, &state_order);
  ASSERT_EQ(max_order, 5);       // 5-gram model
  ASSERT_EQ(state_order[0], 1);  // backoff state
  ASSERT_EQ(state_order[1], 2);  // initial state
}

// Checks weight normalization
TEST_F(NormalizeTest, IsNormalizedTest) {
  ASSERT_TRUE(IsNormalized(*sfst1_, 0, kTestDelta));
  ASSERT_FALSE(IsNormalized(*sfst2_, 0, kTestDelta));
  ASSERT_FALSE(IsNormalized(*sfst3_, 0, kTestDelta));
}

// Globally normalizes an FST
TEST_F(NormalizeTest, GlobalNormalize) {
  {
    fst::VectorFst<Arc> sfst(*sfst1_);
    ASSERT_TRUE(GlobalNormalize(&sfst, 0, kAlgoDelta));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst2_);
    ASSERT_TRUE(GlobalNormalize(&sfst, 0, kAlgoDelta));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst3_);
    ASSERT_FALSE(GlobalNormalize(&sfst, 0, kAlgoDelta));
    ASSERT_FALSE(IsNormalized(sfst, 0, kTestDelta));
  }
}

// Locally normalizes an FST
TEST_F(NormalizeTest, LocalNormalize) {
  {
    fst::VectorFst<Arc> sfst(*sfst1_);
    ASSERT_TRUE(LocalNormalize(&sfst));
    ASSERT_TRUE(IsNormalized(sfst, fst::kNoLabel, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst2_);
    ASSERT_TRUE(LocalNormalize(&sfst));
    ASSERT_TRUE(IsNormalized(sfst, fst::kNoLabel, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst3_);
    ASSERT_FALSE(LocalNormalize(&sfst));
    ASSERT_FALSE(IsNormalized(sfst, fst::kNoLabel, kTestDelta));
  }
}

// Normalizes the failure transitions in an FST
TEST_F(NormalizeTest, PhiNormalize) {
  {
    fst::VectorFst<Arc> sfst(*sfst1_);
    ASSERT_TRUE(PhiNormalize(&sfst, 0));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst2_);
    ASSERT_TRUE(PhiNormalize(&sfst, 0));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst3_);
    ASSERT_FALSE(PhiNormalize(&sfst, 0));
    ASSERT_FALSE(IsNormalized(sfst, 0, kTestDelta));
  }
}

// Globally consitioning an FST
TEST_F(NormalizeTest, Condition) {
  {
    fst::VectorFst<Arc> sfst(*sfst1_);
    ASSERT_TRUE(Condition(&sfst, 0, .3));
    ASSERT_TRUE(GlobalNormalize(&sfst, 0, kAlgoDelta));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst2_);
    ASSERT_TRUE(Condition(&sfst, 0, .3));
    ASSERT_TRUE(GlobalNormalize(&sfst, 0, kAlgoDelta));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
  {
    fst::VectorFst<Arc> sfst(*sfst3_);
    ASSERT_TRUE(Condition(&sfst, 0, .3));
    ASSERT_FALSE(GlobalNormalize(&sfst, 0, kAlgoDelta));
    ASSERT_FALSE(IsNormalized(sfst, 0, kTestDelta));
  }
}

// Recalculates failure transitions without rescaling non-failure weights
TEST_F(NormalizeTest, RecalcBackoff) {
  {
    fst::VectorFst<Arc> sfst(*sfst1_);
    ASSERT_TRUE(RecalcBackoff(&sfst, 0));
    ASSERT_TRUE(IsNormalized(sfst, 0, kTestDelta));
  }
}

TEST_F(NormalizeTest, SafeMinus) {
  fst::Log64Weight one = fst::Log64Weight::One();  // 0.0
  fst::Log64Weight half(0.69314718);               // -ln(0.5)

  // 1.0 - 0.5 = 0.5 (-ln(0.5) = 0.69314718)
  EXPECT_NEAR(SafeMinus(one, half).Value(), 0.69314718, 1e-5);

  // Subtrahend >= minuend in prob domain => kApproxZeroWeight
  EXPECT_EQ(SafeMinus(one, one), kApproxZeroWeight);
  EXPECT_EQ(SafeMinus(one, fst::Log64Weight(-0.5)), kApproxZeroWeight);

  // Numerical approximate equality within delta => kApproxZeroWeight
  fst::Log64Weight w_close(1.0e-16);
  EXPECT_EQ(SafeMinus(one, w_close, /*delta=*/1.0e-15), kApproxZeroWeight);
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
