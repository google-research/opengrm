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

#include "opengrm/thrax/ast/collection-node.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {
namespace {

class CollectionNodeTest : public ::testing::Test {
 protected:
  const std::string& GetString(Node* node) {
    auto* snode = absl::down_cast<StringNode*>(node);
    return snode->Get();
  }
};

TEST_F(CollectionNodeTest, BasicTest) {
  CollectionNode node;
  node.AddFront(std::make_unique<StringNode>("a"));
  node.AddFront(std::make_unique<StringNode>("b"));

  EXPECT_EQ(2, node.Size());
  EXPECT_EQ("b", GetString(node.Get(0)));
  EXPECT_EQ("a", GetString(node[1]));
}

}  // namespace
}  // namespace thrax
