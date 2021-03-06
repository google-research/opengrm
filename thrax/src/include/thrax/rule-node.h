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
// A rule node is an assignment of an FST or other object to a variable. The
// rule node contains the left and right hand sides of the assignment.

#ifndef THRAX_RULE_NODE_H_
#define THRAX_RULE_NODE_H_

#include <memory>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class IdentifierNode;

class RuleNode : public Node {
 public:
  enum ExportStatus {
    EXPORT,
    DO_NOT_EXPORT,
  };

  // TODO(wolfsonkin): Make this interface use  std::unique_ptr.
  RuleNode(IdentifierNode* name, Node* rhs, ExportStatus exp);

  ~RuleNode() override = default;

  IdentifierNode* GetName() const;

  Node* Get() const;

  bool ShouldExport() const;

  void Accept(AstWalker* walker) override;

 private:
  std::unique_ptr<IdentifierNode> name_;
  std::unique_ptr<Node> rhs_;
  ExportStatus export_;

  RuleNode(const RuleNode&) = delete;
  RuleNode& operator=(const RuleNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_RULE_NODE_H_
