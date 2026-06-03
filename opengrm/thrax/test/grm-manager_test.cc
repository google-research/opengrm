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

#include "opengrm/thrax/grm-manager.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/vector-fst.h"

namespace thrax {
namespace {

using ::fst::StdArc;
using ::fst::StdVectorFst;

class GrmManagerTestBase : public ::testing::Test {
 public:
  // The following two functions are just helper functions that test for
  // successful or failing rewrites.  They're public so that we can use them in
  // callbacks.

  // Rewrite input to expected_output using the specified rule.  This expects a
  // successful rewrite.
  void RunRule(absl::string_view rule, absl::string_view input,
               absl::string_view expected_output) {
    std::string actual_output;
    ASSERT_TRUE(grm_manager_->RewriteBytes(rule, input, &actual_output));
    EXPECT_EQ(expected_output, actual_output);
  }

  // Attempt to rewrite input using the specified rule.  This expects a failure
  // (non-match).
  void RunFailingRule(absl::string_view rule, absl::string_view input) {
    std::string temp;
    ASSERT_FALSE(grm_manager_->RewriteBytes(rule, input, &temp));
  }

 protected:
  void SetUpGrmManager(bool use_deprecated_load = false) {
    grm_manager_ = std::make_unique<GrmManagerSpec<StdArc>>();
    const std::string far_path = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/cap.sttable.far");
    if (use_deprecated_load) {
      ASSERT_TRUE(grm_manager_->LoadArchive(far_path));
    } else {
      ABSL_ASSERT_OK(grm_manager_->LoadArchiveWithStatus(far_path));
    }
  }

  std::unique_ptr<GrmManagerSpec<StdArc>> grm_manager_;
};

struct GrmManagerTestParams {
  bool use_deprecated_load = false;
};
class GrmManagerTest
    : public GrmManagerTestBase,
      public ::testing::WithParamInterface<GrmManagerTestParams> {
 protected:
  void SetUp() override { SetUpGrmManager(GetParam().use_deprecated_load); }
};

TEST_P(GrmManagerTest, SimpleTest) {
  RunRule("cap", "a", "A");
  RunRule("cap", "b", "B");
  RunFailingRule("cap", "z");

  RunRule("cap_sorted", "a", "A");
  RunRule("cap_sorted", "b", "B");
  RunFailingRule("cap_sorted", "z");
}

TEST_P(GrmManagerTest, GetAndSet) {
  RunRule("cap", "a", "A");
  RunRule("test", "A", "a");
  RunFailingRule("test", "a");

  // Check that we can get "cap" and set "test" to a copy of "cap".
  const ::fst::StdVectorFst fst(*grm_manager_->GetFst("cap"));
  ASSERT_TRUE(grm_manager_->SetFst("test", fst));
  RunRule("test", "a", "A");
  RunFailingRule("test", "A");

  // Check that we cannot call SetFst if the rule doesn't exist.
  ASSERT_FALSE(grm_manager_->SetFst("doesnotexist", fst));
}

INSTANTIATE_TEST_SUITE_P(GrmManagerTest, GrmManagerTest,
                         ::testing::ValuesIn<GrmManagerTestParams>({
                             {.use_deprecated_load = false},
                             {.use_deprecated_load = true},
                         }));

}  // namespace
}  // namespace thrax
