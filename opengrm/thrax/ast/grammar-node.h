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

// The main grammar is the top node (i.e., root) of the AST. A grammar consists
// of a list of imports, a list of functions, and a list of body functions.

#ifndef OPENGRM_THRAX_AST_GRAMMAR_NODE_H_
#define OPENGRM_THRAX_AST_GRAMMAR_NODE_H_

#include <memory>

#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/node.h"

namespace thrax {

class AstWalker;

class GrammarNode : public Node {
 public:
  GrammarNode(std::unique_ptr<CollectionNode> imports,
              std::unique_ptr<CollectionNode> functions,
              std::unique_ptr<CollectionNode> statements);

  ~GrammarNode() override = default;

  CollectionNode* GetImports() const;

  CollectionNode* GetFunctions() const;

  CollectionNode* GetStatements() const;

  void Accept(AstWalker* walker) override;

 private:
  std::unique_ptr<CollectionNode> imports_;
  std::unique_ptr<CollectionNode> functions_;
  std::unique_ptr<CollectionNode> statements_;

  GrammarNode(const GrammarNode&) = delete;
  GrammarNode& operator=(const GrammarNode&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_GRAMMAR_NODE_H_
