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

// Unit test for stochastic FST perplexity computation.

#include "opengrm/sfst/perplexity.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/canonical.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

const double kSourceCrossEntropy = 9.69104;
const double kCrossPerplexity = 2.30541;
const double kSourceSelfEntropy = 23.7232482;
const double kSelfPerplexity = 8.01492662;
const int kSources = 1688;
const int kSkipped = 0;
const double kOOVSourceCrossEntropy = 9.68903;
const double kOOVCrossPerplexity = 2.30583;
const double kOOVSkipped = 1;
const double kOOVs = 1;
const double kUnk = 2304;
const double kCmpDelta = fst::kDelta;

class PerplexityTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string targ_fst_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/earnest.mod");
    const std::string src_fst_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/min_test.fst");
    const std::string oov_fst_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/min_oov_test.fst");
    const std::string overflow1_fst_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/overflow1.fst");
    const std::string overflow2_fst_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/overflow2.fst");

    src_far_name_ =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/test.far");
    oov_far_name_ = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/oov_test.far");

    targ_fst_.reset(fst::Fst<Arc>::Read(targ_fst_name));
    src_fst_.reset(fst::Fst<Arc>::Read(src_fst_name));
    oov_fst_.reset(fst::Fst<Arc>::Read(oov_fst_name));
    overflow1_fst_.reset(fst::Fst<Arc>::Read(overflow1_fst_name));
    overflow2_fst_.reset(fst::Fst<Arc>::Read(overflow2_fst_name));
  }

  std::unique_ptr<fst::Fst<Arc>> targ_fst_;
  std::unique_ptr<fst::Fst<Arc>> src_fst_;
  std::unique_ptr<fst::Fst<Arc>> oov_fst_;
  std::unique_ptr<fst::Fst<Arc>> overflow1_fst_;
  std::unique_ptr<fst::Fst<Arc>> overflow2_fst_;
  std::string src_far_name_;
  std::string oov_far_name_;
};

// Sources are unweighted strings in a FAR. No OOVs.
TEST_F(PerplexityTest, FarPerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0);
  std::unique_ptr<fst::FarReader<Arc>> far_reader(
      fst::FarReader<Arc>::Open(src_far_name_));
  for (; !far_reader->Done(); far_reader->Next())
    ASSERT_TRUE(perp.Apply(*far_reader->GetFst()));

  ASSERT_NEAR(perp.GetEntropy(), kSourceCrossEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kCrossPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), kSources);
  ASSERT_NEAR(perp.SkipCount(), kSkipped, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), 0);
}

// Source is weighted FST (equivalent to the FAR contents above).
TEST_F(PerplexityTest, FstPerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0);
  ASSERT_TRUE(perp.Apply(*src_fst_));

  ASSERT_NEAR(perp.GetEntropy(), kSourceCrossEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kCrossPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), 1);
  ASSERT_NEAR(perp.SkipCount(), kSkipped / kSources, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), 0);
}

// Sources are unweighted strings in a FAR.
// One OOV (word 2304 remapped to OOV 2305) in last string.
// Unknown label unset.
TEST_F(PerplexityTest, OOVFarPerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0);
  std::unique_ptr<fst::FarReader<Arc>> far_reader(
      fst::FarReader<Arc>::Open(oov_far_name_));
  for (; !far_reader->Done(); far_reader->Next())
    ASSERT_TRUE(perp.Apply(*far_reader->GetFst()));

  ASSERT_NEAR(perp.GetEntropy(), kOOVSourceCrossEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kOOVCrossPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), kSources);
  ASSERT_NEAR(perp.SkipCount(), kOOVSkipped, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), 0);
}

// Source is weighted FST (equivalent to the OOV FAR contents above).
// One OOV (word 2304 remapped to OOV 2305) in last string.
// Unknown label unset.
TEST_F(PerplexityTest, OOVFstPerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0);
  ASSERT_TRUE(perp.Apply(*oov_fst_));

  ASSERT_NEAR(perp.GetEntropy(), kOOVSourceCrossEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kOOVCrossPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), 1);
  ASSERT_NEAR(perp.SkipCount(), kOOVSkipped / kSources, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), 0);
}

// Sources are unweighted strings in a FAR.
// One OOV (word 2304 remapped to OOV 2305) in last string.
// Unknown label set (to 2304).
TEST_F(PerplexityTest, UnkFarPerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0, kUnk);
  std::unique_ptr<fst::FarReader<Arc>> far_reader(
      fst::FarReader<Arc>::Open(oov_far_name_));
  for (; !far_reader->Done(); far_reader->Next())
    ASSERT_TRUE(perp.Apply(*far_reader->GetFst()));

  ASSERT_NEAR(perp.GetEntropy(), kSourceCrossEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kCrossPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), kSources);
  ASSERT_NEAR(perp.SkipCount(), kSkipped, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), kOOVs);
}

// Source is weighted FST (equivalent to the OOV FAR contents above).
// One OOV (word 2304 remapped to OOV 2305) in last string.
// Unknown label set (to 2304).
TEST_F(PerplexityTest, UnkFstPerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0, kUnk);
  ASSERT_TRUE(perp.Apply(*oov_fst_));

  ASSERT_NEAR(perp.GetEntropy(), kSourceCrossEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kCrossPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), 1);
  ASSERT_NEAR(perp.SkipCount(), kSkipped / kSources, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), kOOVs);
}

// Source is LM FST
TEST_F(PerplexityTest, SelfPerplexityTest) {
  Perplexity<Arc> perp(0);
  ASSERT_TRUE(perp.Apply(*targ_fst_));

  ASSERT_NEAR(perp.GetEntropy(), kSourceSelfEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kSelfPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), 1);
  ASSERT_NEAR(perp.SkipCount(), kSkipped, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), 0);
}

// Source and target is LM FST
TEST_F(PerplexityTest, PerplexityTest) {
  Perplexity<Arc> perp(*targ_fst_, 0);
  ASSERT_TRUE(perp.Apply(*targ_fst_));
  ASSERT_NEAR(perp.GetEntropy(), kSourceSelfEntropy, kCmpDelta);
  ASSERT_NEAR(perp.GetPerplexity(), kSelfPerplexity, kCmpDelta);
  ASSERT_EQ(perp.NumSources(), 1);
  ASSERT_NEAR(perp.SkipCount(), kSkipped, kCmpDelta);
  ASSERT_EQ(perp.NumOOVs(), 0);
}

TEST_F(PerplexityTest, AsanHeapBufferOverflowTest) {
  ASSERT_TRUE(sfst::IsCanonical(*overflow1_fst_, 0));
  ASSERT_TRUE(sfst::IsCanonical(*overflow2_fst_, 0));
  sfst::Perplexity<Arc> perp(*overflow1_fst_, 0);
  ASSERT_TRUE(perp.Apply(*overflow2_fst_));
}

TEST_F(PerplexityTest, EmptyTargetTest) {
  fst::VectorFst<Arc> empty_fst;
  Perplexity<Arc> perp(empty_fst, 0);
  EXPECT_FALSE(perp.Apply(*src_fst_));
}

}  // namespace sfst

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
