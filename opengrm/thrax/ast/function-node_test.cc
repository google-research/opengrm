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

#include "opengrm/thrax/ast/function-node.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "absl/memory/memory.h"
#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {
namespace {

class FunctionNodeTest : public ::testing::Test {
 protected:
  const std::string& GetString(Node* node) {
    auto* snode = absl::down_cast<StringNode*>(node);
    return snode->Get();
  }
};

TEST_F(FunctionNodeTest, BasicTest) {
  auto* name = new IdentifierNode("my_function");

  auto* args = new CollectionNode();
  args->AddFront(std::make_unique<StringNode>("arg2"));
  args->AddFront(std::make_unique<StringNode>("arg1"));

  auto* body = new CollectionNode();
  body->AddFront(std::make_unique<StringNode>("stmt2"));
  body->AddFront(std::make_unique<StringNode>("stmt1"));

  FunctionNode function(absl::WrapUnique(name), absl::WrapUnique(args),
                        absl::WrapUnique(body));

  EXPECT_EQ("my_function", function.GetName()->Get());
  EXPECT_EQ(2, function.GetArguments()->Size());
  EXPECT_EQ("arg1", GetString(function.GetArguments()->Get(0)));
  EXPECT_EQ("arg2", GetString(function.GetArguments()->Get(1)));
  EXPECT_EQ(2, function.GetBody()->Size());
  EXPECT_EQ("stmt1", GetString(function.GetBody()->Get(0)));
  EXPECT_EQ("stmt2", GetString(function.GetBody()->Get(1)));
}

}  // namespace
}  // namespace thrax
