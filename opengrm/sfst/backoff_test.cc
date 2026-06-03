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

// Unit tests for stochastic FST backoff algorithms.

#include "opengrm/sfst/backoff.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/canonical.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

class BackoffTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string backoff1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/count1.fst");
    const std::string backoff2_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/backoff1.fst");

    bfst1_.reset(fst::VectorFst<Arc>::Read(backoff1_name));
    bfst2_.reset(fst::VectorFst<Arc>::Read(backoff2_name));
  }

  std::unique_ptr<fst::VectorFst<Arc>> bfst1_;
  std::unique_ptr<fst::VectorFst<Arc>> bfst2_;
};

TEST_F(BackoffTest, IsBackoffTest) {
  ASSERT_TRUE(IsCanonical(*bfst1_, 0));
  ASSERT_TRUE(IsBackoffComplete(*bfst1_, 0));
}

TEST_F(BackoffTest, SumBackoffTest) {
  fst::VectorFst<Arc> bfst2(*bfst1_);
  SumBackoff(&bfst2, 0);
  ASSERT_TRUE(IsCanonical(bfst2, 0));
  ASSERT_TRUE(fst::Equal(*bfst2_, bfst2));
}

TEST_F(BackoffTest, DiffBackoffTest) {
  fst::VectorFst<Arc> bfst2(*bfst1_);
  SumBackoff(&bfst2, 0);
  DiffBackoff(&bfst2, 0);
  ASSERT_TRUE(IsCanonical(bfst2, 0));
  ASSERT_TRUE(fst::Equal(*bfst1_, bfst2));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
