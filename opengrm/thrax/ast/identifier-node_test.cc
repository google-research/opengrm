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

#include "opengrm/thrax/ast/identifier-node.h"

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"

namespace thrax {
namespace {

class IdentifierNodeTest : public ::testing::Test {
 protected:
  void CheckValidity(absl::string_view name, bool expected_validity) {
    IdentifierNode n(name);
    EXPECT_EQ(expected_validity, n.IsValid());
  }
};

TEST_F(IdentifierNodeTest, BasicTest) {
  IdentifierNode node("abc");
  EXPECT_EQ("abc", node.Get());
  EXPECT_EQ("abc", node.GetIdentifier());

  EXPECT_FALSE(node.HasNamespaces());
  EXPECT_TRUE(node.begin() == node.end());  // EXPECT_EQ can't print iterators.
}

TEST_F(IdentifierNodeTest, NamespaceTest) {
  IdentifierNode node("ab.cd.ef");

  EXPECT_EQ("ab.cd.ef", node.Get());
  EXPECT_EQ("ef", node.GetIdentifier());

  EXPECT_TRUE(node.HasNamespaces());
  IdentifierNode::const_iterator ns = node.begin();
  EXPECT_EQ("ab", *ns++);
  EXPECT_EQ("cd", *ns++);
  EXPECT_TRUE(ns == node.end());
}

TEST_F(IdentifierNodeTest, EmptyNamespaceTest) {
  IdentifierNode node("..aaa..bbb..");  // Yes, this identifier is invalid.

  EXPECT_EQ("..aaa..bbb..", node.Get());
  EXPECT_EQ("", node.GetIdentifier());

  auto it = node.begin();
  EXPECT_EQ("", *it++);
  EXPECT_EQ("", *it++);
  EXPECT_EQ("aaa", *it++);
  EXPECT_EQ("", *it++);
  EXPECT_EQ("bbb", *it++);
  EXPECT_EQ("", *it++);
  EXPECT_TRUE(it == node.end());
}

TEST_F(IdentifierNodeTest, ValidityTest) {
  CheckValidity("abc", true);
  CheckValidity("_abc", true);
  CheckValidity("abc1", true);
  CheckValidity("_1", true);
  CheckValidity("abc._abc.abc1._1", true);

  CheckValidity("1abc", false);
  CheckValidity("", false);
  CheckValidity("abc..abc", false);
  CheckValidity("a*", false);
  CheckValidity("abc.!@#.abc", false);
}

}  // namespace
}  // namespace thrax
