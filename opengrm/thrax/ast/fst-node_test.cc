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

#include "opengrm/thrax/ast/fst-node.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {
namespace {

class FstNodeTest : public ::testing::Test {
 protected:
  const std::string& GetString(Node* node) {
    auto* snode = absl::down_cast<StringNode*>(node);
    return snode->Get();
  }
};

TEST_F(FstNodeTest, BasicTest) {
  FstNode node(FstNode::UNION_FSTNODE);
  EXPECT_EQ(FstNode::UNION_FSTNODE, node.GetType());

  node.AddArgument(std::make_unique<StringNode>("abc"));
  node.AddArgument(std::make_unique<StringNode>("xyz"));
  EXPECT_EQ("abc", GetString(node.GetArgument(0)));
  EXPECT_EQ("xyz", GetString(node.GetArgument(1)));

  EXPECT_FALSE(node.HasWeight());
  EXPECT_TRUE(node.SetWeight(std::make_unique<StringNode>("weight")));
  EXPECT_FALSE(node.SetWeight(std::make_unique<StringNode>("new weight")));
  EXPECT_TRUE(node.HasWeight());
  EXPECT_EQ("weight", node.GetWeight());

  EXPECT_FALSE(node.ShouldOptimize());
  node.SetOptimize();
  EXPECT_TRUE(node.ShouldOptimize());
}

}  // namespace
}  // namespace thrax
