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

#include "opengrm/thrax/ast/grammar-node.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "absl/memory/memory.h"
#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {
namespace {

class GrammarNodeTest : public ::testing::Test {
 protected:
  const std::string& GetString(Node* node) const {
    auto* snode = absl::down_cast<StringNode*>(node);
    return snode->Get();
  }
};

TEST_F(GrammarNodeTest, BasicTest) {
  auto* imports = new CollectionNode();
  imports->AddFront(std::make_unique<StringNode>("import 2"));
  imports->AddFront(std::make_unique<StringNode>("import 1"));

  auto* functions = new CollectionNode();
  functions->AddFront(std::make_unique<StringNode>("function 2"));
  functions->AddFront(std::make_unique<StringNode>("function 1"));

  auto* statements = new CollectionNode();
  statements->AddFront(std::make_unique<StringNode>("statement 2"));
  statements->AddFront(std::make_unique<StringNode>("statement 1"));

  GrammarNode grammar(absl::WrapUnique(imports), absl::WrapUnique(functions),
                      absl::WrapUnique(statements));

  EXPECT_EQ(2, grammar.GetImports()->Size());
  EXPECT_EQ("import 1", GetString(grammar.GetImports()->Get(0)));
  EXPECT_EQ("import 2", GetString(grammar.GetImports()->Get(1)));
  EXPECT_EQ(2, grammar.GetFunctions()->Size());
  EXPECT_EQ("function 1", GetString(grammar.GetFunctions()->Get(0)));
  EXPECT_EQ("function 2", GetString(grammar.GetFunctions()->Get(1)));
  EXPECT_EQ(2, grammar.GetStatements()->Size());
  EXPECT_EQ("statement 1", GetString(grammar.GetStatements()->Get(0)));
  EXPECT_EQ("statement 2", GetString(grammar.GetStatements()->Get(1)));
}

}  // namespace
}  // namespace thrax
