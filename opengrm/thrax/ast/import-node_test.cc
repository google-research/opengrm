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

#include "opengrm/thrax/ast/import-node.h"

#include <memory>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {
namespace {

class ImportNodeTest : public ::testing::Test {};

TEST_F(ImportNodeTest, BasicTest) {
  ImportNode node(std::make_unique<StringNode>("path/to/file.grm"),
                  std::make_unique<IdentifierNode>("localname"));

  EXPECT_EQ("path/to/file.grm",
            absl::down_cast<StringNode*>(node.GetPath())->Get());
  EXPECT_EQ("localname",
            absl::down_cast<IdentifierNode*>(node.GetAlias())->Get());
}

}  // namespace
}  // namespace thrax
