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
// Return statements tell us what FST to return from a function, allowing
// binding to a variable in the main program.

#ifndef THRAX_RETURN_NODE_H_
#define THRAX_RETURN_NODE_H_

#include <memory>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;

class ReturnNode : public Node {
 public:
  // TODO(wolfsonkin): Make this interface use  std::unique_ptr.
  explicit ReturnNode(Node* node);

  ~ReturnNode() override = default;

  Node* Get() const;

  void Accept(AstWalker* walker) override;

 private:
  std::unique_ptr<Node> node_;

  ReturnNode(const ReturnNode&) = delete;
  ReturnNode& operator=(const ReturnNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_RETURN_NODE_H_
