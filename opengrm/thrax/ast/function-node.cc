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
#include <utility>

#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/walker/walker.h"

namespace thrax {

FunctionNode::FunctionNode(std::unique_ptr<IdentifierNode> name,
                           std::unique_ptr<CollectionNode> arguments,
                           std::unique_ptr<CollectionNode> body)
    : Node(),
      name_(std::move(name)),
      arguments_(std::move(arguments)),
      body_(std::move(body)) {}

IdentifierNode* FunctionNode::GetName() const { return name_.get(); }

CollectionNode* FunctionNode::GetArguments() const { return arguments_.get(); }

CollectionNode* FunctionNode::GetBody() const { return body_.get(); }

void FunctionNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
