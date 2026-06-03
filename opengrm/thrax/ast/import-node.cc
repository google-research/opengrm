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
#include <utility>

#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"
#include "opengrm/thrax/walker/walker.h"

namespace thrax {

ImportNode::ImportNode(std::unique_ptr<StringNode> path,
                       std::unique_ptr<IdentifierNode> alias)
    : Node(), path_(std::move(path)), alias_(std::move(alias)) {}

StringNode* ImportNode::GetPath() const { return path_.get(); }

IdentifierNode* ImportNode::GetAlias() const { return alias_.get(); }

void ImportNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
