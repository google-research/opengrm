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

// An import statement is one that loads up another module. This node contains
// the path and alias for an import.

#ifndef OPENGRM_THRAX_AST_IMPORT_NODE_H_
#define OPENGRM_THRAX_AST_IMPORT_NODE_H_

#include <memory>

#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {

class AstWalker;

class ImportNode : public Node {
 public:
  ImportNode(std::unique_ptr<StringNode> path,
             std::unique_ptr<IdentifierNode> alias);

  ~ImportNode() override = default;

  StringNode* GetPath() const;

  IdentifierNode* GetAlias() const;

  void Accept(AstWalker* walker) override;

 private:
  std::unique_ptr<StringNode> path_;
  std::unique_ptr<IdentifierNode> alias_;

  ImportNode(const ImportNode&) = delete;
  ImportNode& operator=(const ImportNode&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_IMPORT_NODE_H_
