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

#include "opengrm/rewrite/rule_cascade.h"

#include <string>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/string.h"

namespace rewrite {
namespace {

constexpr absl::string_view kTestDir =
    "opengrm/thrax/test/testdata";

TEST(RuleCascadeTest, FST) {
  StdRuleCascade cascade;
  ABSL_ASSERT_OK(cascade.LoadWithStatus(
      fst::JoinPath(std::string("."), kTestDir, "cap.sttable.far")));
  std::string result;

  ABSL_ASSERT_OK(cascade.SetRulesWithStatus({"cap", "test"}));
  EXPECT_TRUE(cascade.TopRewrite("a", &result));
  EXPECT_EQ("a", result);
  EXPECT_TRUE(cascade.TopRewrite("b", &result));
  EXPECT_EQ("b", result);
  EXPECT_TRUE(cascade.Matches("a", "a"));
  EXPECT_FALSE(cascade.Matches("a", "A"));

  ABSL_ASSERT_OK(cascade.SetRulesWithStatus({"test", "cap"}));
  EXPECT_TRUE(cascade.TopRewrite("A", &result));
  EXPECT_EQ("A", result);
  EXPECT_TRUE(cascade.TopRewrite("B", &result));
  EXPECT_EQ("B", result);
}

TEST(RuleCascadeTest, FSTDeprecatedBoolReturningMethods) {
  StdRuleCascade cascade;
  ABSL_ASSERT_OK(cascade.LoadWithStatus(
      fst::JoinPath(std::string("."), kTestDir, "cap.sttable.far")));
  std::string result;

  ASSERT_TRUE(cascade.SetRules({"cap", "test"}));
  EXPECT_TRUE(cascade.TopRewrite("a", &result));
  EXPECT_EQ("a", result);
  EXPECT_TRUE(cascade.TopRewrite("b", &result));
  EXPECT_EQ("b", result);
  EXPECT_TRUE(cascade.Matches("a", "a"));
  EXPECT_FALSE(cascade.Matches("a", "A"));

  ASSERT_TRUE(cascade.SetRules({"test", "cap"}));
  EXPECT_TRUE(cascade.TopRewrite("A", &result));
  EXPECT_EQ("A", result);
  EXPECT_TRUE(cascade.TopRewrite("B", &result));
  EXPECT_EQ("B", result);
}

TEST(RuleCascadeTest, FSTWithTrivialUtf8) {
  StdRuleCascade cascade(::fst::TokenType::UTF8);
  ABSL_ASSERT_OK(cascade.LoadWithStatus(
      fst::JoinPath(std::string("."), kTestDir, "cap.sttable.far")));
  std::string result;

  ASSERT_TRUE(cascade.SetRules({"cap", "test"}));
  EXPECT_TRUE(cascade.TopRewrite("a", &result));
  EXPECT_EQ("a", result);
  EXPECT_TRUE(cascade.TopRewrite("b", &result));
  EXPECT_EQ("b", result);
  EXPECT_TRUE(cascade.Matches("a", "a"));
  EXPECT_FALSE(cascade.Matches("a", "A"));

  ASSERT_TRUE(cascade.SetRules({"test", "cap"}));
  EXPECT_TRUE(cascade.TopRewrite("A", &result));
  EXPECT_EQ("A", result);
  EXPECT_TRUE(cascade.TopRewrite("B", &result));
  EXPECT_EQ("B", result);
}

TEST(RuleCascadeTest, PDT) {
  StdRuleCascade cascade;
  ABSL_ASSERT_OK(cascade.LoadWithStatus(
      fst::JoinPath(std::string("."), kTestDir, "pdt/pdt.far")));
  std::string result;

  ASSERT_TRUE(cascade.SetRules({"CURRENCY$PARENS"}));
  ASSERT_TRUE(cascade.TopRewrite("$30", &result));
  EXPECT_EQ("thirty dollars", result);

  ASSERT_TRUE(cascade.SetRules({"CURRENCY:PARENS"}));
  ASSERT_TRUE(cascade.TopRewrite("$30", &result));
  EXPECT_EQ("thirty dollars", result);
}

TEST(RuleCascadeTest, MPDT) {
  StdRuleCascade cascade;
  ABSL_ASSERT_OK(cascade.LoadWithStatus(
      fst::JoinPath(std::string("."), kTestDir, "mpdt/mpdt.far")));
  std::string result;

  ASSERT_TRUE(cascade.SetRules({"REDUPLICATOR$PARENS$ASSIGNMENTS"}));
  ASSERT_TRUE(cascade.TopRewrite("aab", &result));
  EXPECT_EQ("aabaab", result);

  ASSERT_TRUE(cascade.SetRules({"REDUPLICATOR:PARENS:ASSIGNMENTS"}));
  ASSERT_TRUE(cascade.TopRewrite("aab", &result));
  EXPECT_EQ("aabaab", result);
}

}  // namespace
}  // namespace rewrite
