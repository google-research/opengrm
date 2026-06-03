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

// The generic, basic node for our AST.

#ifndef OPENGRM_THRAX_AST_NODE_H_
#define OPENGRM_THRAX_AST_NODE_H_

namespace thrax {

class AstWalker;

class Node {
 public:
  Node();

  virtual ~Node() = default;

  virtual void Accept(AstWalker* walker) = 0;

  // Set and get the line number where this node is stored.

  void SetLine(int line);

  int GetLine() const;

 protected:
  int line_number_;  // Where in the file this node is found.

 private:
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_NODE_H_
