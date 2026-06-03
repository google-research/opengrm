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

// A statement is a generic command to be executed. Currently, we support only
// return statements and rule (assignment) statements.

#ifndef OPENGRM_THRAX_AST_STATEMENT_NODE_H_
#define OPENGRM_THRAX_AST_STATEMENT_NODE_H_

#include <memory>

#include "opengrm/thrax/ast/node.h"

namespace thrax {

class AstWalker;

class StatementNode : public Node {
 public:
  enum StatementNodeType {
    RULE_STATEMENTNODE,
    RETURN_STATEMENTNODE,
  };

  explicit StatementNode(StatementNodeType type);

  ~StatementNode() override = default;

  StatementNodeType GetType() const;

  void Set(std::unique_ptr<Node> statement);

  Node* Get() const;

  void Accept(AstWalker* walker) override;

 private:
  const StatementNodeType type_;
  std::unique_ptr<Node> statement_;

  StatementNode(const StatementNode&) = delete;
  StatementNode& operator=(const StatementNode&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_STATEMENT_NODE_H_
