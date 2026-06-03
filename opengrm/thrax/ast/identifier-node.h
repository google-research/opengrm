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

// An identifier is a variable name, essentially. This node parses module
// namespaces, splitting on the dots and provides iteration capabilities over
// the identifier.

#ifndef OPENGRM_THRAX_AST_IDENTIFIER_NODE_H_
#define OPENGRM_THRAX_AST_IDENTIFIER_NODE_H_

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/node.h"

namespace thrax {

class AstWalker;

class IdentifierNode : public Node {
 public:
  explicit IdentifierNode(absl::string_view name);

  IdentifierNode(absl::string_view name, int begin_pos);

  ~IdentifierNode() override = default;

  // Return the entire identifier as originally used in the source.
  const std::string& Get() const;

  // Returns the actual identifier - the last component.  For example, if the
  // identifier is foo.bar.baz, this will return baz.
  const std::string& GetIdentifier() const;

  // Returns the beginning byte position of the identifier in the source.
  int GetBeginPos() const;

  // Returns true if there are any namespace qualifiers and false otherwise.
  bool HasNamespaces() const;

  // STL-style iterator-based accessors of the namespace list.
  typedef std::vector<std::string>::const_iterator const_iterator;

  const_iterator begin() const;

  const_iterator end() const;

  // Returns true if the identifier provided is valid.  We check for:
  //   - No empty components.
  //   - No components that are fully numeric or fully underscore.
  //   - Contains no invalid characters (only underscores, alphabet characters,
  //     and numbers are allowed).
  bool IsValid() const;

  void Accept(AstWalker* walker) override;

 private:
  bool CalculateValidity();

  const std::string full_name_;
  std::string identifier_;
  // The full_name_ broken up by namespaces.
  std::vector<std::string> namespaces_;
  const int begin_pos_;
  bool valid_;

  IdentifierNode(const IdentifierNode&) = delete;
  IdentifierNode& operator=(const IdentifierNode&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_IDENTIFIER_NODE_H_
