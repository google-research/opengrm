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

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "openfst/lib/arc.h"
#include "opengrm/thrax/grm-manager.h"

namespace thrax {
namespace {

using ::fst::StdArc;

class AdvancedRegressionTest : public ::testing::Test {
 protected:
  AdvancedRegressionTest() {
    far_path_ = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/regression",
        "advanced.far");

    grm_ = std::make_unique<GrmManagerSpec<StdArc>>();
    CHECK(grm_->LoadArchive(far_path_));
  }

  std::string far_path_;
  std::unique_ptr<GrmManagerSpec<StdArc>> grm_;
};

TEST_F(AdvancedRegressionTest, MainTest) {
  std::string output;

  // [a-z] | [a-z][a-z][a-z]
  EXPECT_TRUE(grm_->RewriteBytes("one_or_three", "a", &output));
  EXPECT_TRUE(grm_->RewriteBytes("one_or_three", "j", &output));
  EXPECT_TRUE(grm_->RewriteBytes("one_or_three", "abc", &output));
  EXPECT_TRUE(grm_->RewriteBytes("one_or_three", "zzz", &output));
  EXPECT_FALSE(grm_->RewriteBytes("one_or_three", "ab", &output));
  EXPECT_FALSE(grm_->RewriteBytes("one_or_three", "", &output));
  EXPECT_FALSE(grm_->RewriteBytes("one_or_three", "tmnt", &output));

  // capitalize_vowels
  EXPECT_TRUE(grm_->RewriteBytes("cap_all_vowels", "abcdefg", &output));
  EXPECT_EQ("AbcdEfg", output);
  EXPECT_TRUE(grm_->RewriteBytes("cap_all_vowels", "plrbr", &output));
  EXPECT_EQ("plrbr", output);
  EXPECT_TRUE(grm_->RewriteBytes("cap_all_vowels", "aiya", &output));
  EXPECT_EQ("AIYA", output);
  EXPECT_FALSE(grm_->RewriteBytes("cap_all_vowels", "!$!@$#%@#", &output));
}

}  // namespace
}  // namespace thrax
