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
#include <utility>

#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/walker/walker.h"

namespace thrax {

GrammarNode::GrammarNode(std::unique_ptr<CollectionNode> imports,
                         std::unique_ptr<CollectionNode> functions,
                         std::unique_ptr<CollectionNode> statements)
    : Node(),
      imports_(std::move(imports)),
      functions_(std::move(functions)),
      statements_(std::move(statements)) {}

CollectionNode* GrammarNode::GetImports() const { return imports_.get(); }

CollectionNode* GrammarNode::GetFunctions() const { return functions_.get(); }

CollectionNode* GrammarNode::GetStatements() const { return statements_.get(); }

void GrammarNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
