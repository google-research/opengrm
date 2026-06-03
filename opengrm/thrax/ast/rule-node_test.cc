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

#include "opengrm/thrax/ast/rule-node.h"

#include <memory>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "opengrm/thrax/ast/fst-node.h"
#include "opengrm/thrax/ast/identifier-node.h"

namespace thrax {
namespace {

class RuleNodeTest : public ::testing::Test {};

TEST_F(RuleNodeTest, BasicTest) {
  RuleNode exported_rule(std::make_unique<IdentifierNode>("exported.name"),
                         std::make_unique<FstNode>(FstNode::CONCAT_FSTNODE),
                         RuleNode::EXPORT);
  EXPECT_EQ("exported.name", exported_rule.GetName()->Get());
  EXPECT_EQ(FstNode::CONCAT_FSTNODE,
            absl::down_cast<FstNode*>(exported_rule.Get())->GetType());
  EXPECT_TRUE(exported_rule.ShouldExport());

  RuleNode nonexported_rule(
      std::make_unique<IdentifierNode>("non.exported.name"),
      std::make_unique<FstNode>(FstNode::COMPOSITION_FSTNODE),
      RuleNode::DO_NOT_EXPORT);
  EXPECT_EQ("non.exported.name", nonexported_rule.GetName()->Get());
  EXPECT_EQ(FstNode::COMPOSITION_FSTNODE,
            absl::down_cast<FstNode*>(nonexported_rule.Get())->GetType());
  EXPECT_FALSE(nonexported_rule.ShouldExport());
}

}  // namespace
}  // namespace thrax
