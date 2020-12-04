// Copyright 2005-2020 Google LLC
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
//
// A statement is a generic command to be executed. Currently, we support only
// return statements and rule (assignment) statements.

#ifndef THRAX_STATEMENT_NODE_H_
#define THRAX_STATEMENT_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;

class StatementNode : public Node {
 public:
  enum StatementNodeType {
    RULE_STATEMENTNODE,
    RETURN_STATEMENTNODE,
  };

  explicit StatementNode(StatementNodeType type);

  ~StatementNode() override;

  StatementNodeType GetType() const;

  void Set(Node* statement);

  Node* Get() const;

  void Accept(AstWalker* walker) override;

 private:
  StatementNodeType type_;
  Node* statement_;

  StatementNode(const StatementNode&) = delete;
  StatementNode& operator=(const StatementNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_STATEMENT_NODE_H_
