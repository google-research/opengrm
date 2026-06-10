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

#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "opengrm/thrax/abstract-grm-manager.h"
#include "opengrm/thrax/grm-manager.h"

namespace thrax {
namespace {

using ::fst::StdArc;

constexpr absl::string_view kTestDir =
    "opengrm/thrax/test/testdata";

TEST(RuleCascadeTest, FST) {
  GrmManager grm;
  ASSERT_TRUE(grm.LoadArchive(
      fst::JoinPath(std::string("."), kTestDir, "cap.sttable.far")));
  RuleCascade<StdArc> lower;
  RuleCascade<StdArc> upper;
  ASSERT_TRUE(lower.InitFromDefs(&grm, {"cap", "test"}));
  ASSERT_TRUE(upper.InitFromDefs(&grm, {"test", "cap"}));
  std::string result;
  ASSERT_TRUE(lower.RewriteBytes("a", &result));
  EXPECT_EQ("a", result);
  ASSERT_TRUE(lower.RewriteBytes("b", &result));
  EXPECT_EQ("b", result);
  ASSERT_TRUE(upper.RewriteBytes("A", &result));
  EXPECT_EQ("A", result);
  ASSERT_TRUE(upper.RewriteBytes("B", &result));
  EXPECT_EQ("B", result);
}

TEST(RuleCascadeTest, PDT) {
  GrmManager grm;
  ASSERT_TRUE(grm.LoadArchive(
      fst::JoinPath(std::string("."), kTestDir, "pdt/pdt.far")));
  RuleCascade<StdArc> cascade;
  ASSERT_TRUE(cascade.InitFromDefs(&grm, {"CURRENCY$PARENS"}));
  std::string result;
  ASSERT_TRUE(cascade.RewriteBytes("$30", &result));
  EXPECT_EQ("thirty dollars", result);
}

TEST(RuleCascadeTest, PDT_colon) {
  GrmManager grm;
  ASSERT_TRUE(grm.LoadArchive(
      fst::JoinPath(std::string("."), kTestDir, "pdt/pdt.far")));
  RuleCascade<StdArc> cascade;
  ASSERT_TRUE(cascade.InitFromDefs(&grm, {"CURRENCY:PARENS"}));
  std::string result;
  ASSERT_TRUE(cascade.RewriteBytes("$30", &result));
  EXPECT_EQ("thirty dollars", result);
}

TEST(RuleCascadeTest, MPDT) {
  GrmManager grm;
  ASSERT_TRUE(grm.LoadArchive(
      fst::JoinPath(std::string("."), kTestDir, "mpdt/mpdt.far")));
  RuleCascade<StdArc> cascade;
  ASSERT_TRUE(cascade.InitFromDefs(&grm, {"REDUPLICATOR$PARENS$ASSIGNMENTS"}));
  std::string result;
  ASSERT_TRUE(cascade.RewriteBytes("aab", &result));
  EXPECT_EQ("aabaab", result);
}

TEST(RuleCascadeTest, MPDT_colon) {
  GrmManager grm;
  ASSERT_TRUE(grm.LoadArchive(
      fst::JoinPath(std::string("."), kTestDir, "mpdt/mpdt.far")));
  RuleCascade<StdArc> cascade;
  ASSERT_TRUE(cascade.InitFromDefs(&grm, {"REDUPLICATOR:PARENS:ASSIGNMENTS"}));
  std::string result;
  ASSERT_TRUE(cascade.RewriteBytes("aab", &result));
  EXPECT_EQ("aabaab", result);
}

}  // namespace
}  // namespace thrax
