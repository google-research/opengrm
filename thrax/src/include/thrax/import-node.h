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
// An import statement is one that loads up another module. This node contains
// the path and alias for an import.

#ifndef THRAX_IMPORT_NODE_H_
#define THRAX_IMPORT_NODE_H_

#include <memory>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class IdentifierNode;
class StringNode;

class ImportNode : public Node {
 public:
  // TODO(wolfsonkin): Make this interface use  std::unique_ptr.
  ImportNode(StringNode* path, IdentifierNode* alias);

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

#endif  // THRAX_IMPORT_NODE_H_
