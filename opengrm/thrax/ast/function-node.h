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

// A function is a repeatable set of assignments and commands used to build a
// new FST from inputs. This node contains the name of the function along with
// the bound argument names and the body statements.

#ifndef OPENGRM_THRAX_AST_FUNCTION_NODE_H_
#define OPENGRM_THRAX_AST_FUNCTION_NODE_H_

#include <memory>

#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/node.h"

namespace thrax {

class AstWalker;

class FunctionNode : public Node {
 public:
  FunctionNode(std::unique_ptr<IdentifierNode> name,
               std::unique_ptr<CollectionNode> arguments,
               std::unique_ptr<CollectionNode> body);

  ~FunctionNode() override = default;

  IdentifierNode* GetName() const;

  CollectionNode* GetArguments() const;

  CollectionNode* GetBody() const;

  void Accept(AstWalker* walker) override;

 private:
  std::unique_ptr<IdentifierNode> name_;
  std::unique_ptr<CollectionNode> arguments_;
  std::unique_ptr<CollectionNode> body_;

  FunctionNode(const FunctionNode&) = delete;
  FunctionNode& operator=(const FunctionNode&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_FUNCTION_NODE_H_
