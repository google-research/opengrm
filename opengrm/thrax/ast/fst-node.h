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

// This node represents basic (primitive) types in the language. This name is
// currently misleading. The held type can be anything represented in a
// DataType essentially: FST, symbol table, or string.

#ifndef OPENGRM_THRAX_AST_FST_NODE_H_
#define OPENGRM_THRAX_AST_FST_NODE_H_

#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {

class AstWalker;

class FstNode : public Node {
 public:
  enum FstNodeType {
    COMPOSITION_FSTNODE,
    CONCAT_FSTNODE,
    DIFFERENCE_FSTNODE,
    FUNCTION_FSTNODE,
    IDENTIFIER_FSTNODE,
    REPETITION_FSTNODE,
    REWRITE_FSTNODE,
    STRING_FSTNODE,
    UNION_FSTNODE,
    UNKNOWN_FSTNODE,
  };

  static absl::string_view FstNodeTypeToString(FstNodeType type);

  explicit FstNode(FstNodeType type);

  ~FstNode() override = default;

  void AddArgument(std::unique_ptr<Node> arg);

  bool SetWeight(std::unique_ptr<StringNode> weight);

  FstNodeType GetType() const;

  int NumArguments() const;

  Node* GetArgument(int index) const;

  bool HasWeight() const;

  const std::string& GetWeight() const;

  const bool ShouldOptimize() const;

  void SetOptimize();

  void Accept(AstWalker* walker) override;

 protected:
  const FstNodeType type_;
  std::vector<std::unique_ptr<Node>> arguments_;
  std::unique_ptr<StringNode> weight_;  // nullptr = default weight.
  bool optimize_;

 private:
  FstNode(const FstNode&) = delete;
  FstNode& operator=(const FstNode&) = delete;
};

// A specialization to string FSTs, containing parse information. If we should
// parse the text using a symbol table, then the symbol table identifier should
// be in arguments_[1].
class StringFstNode : public FstNode {
 public:
  enum ParseMode {
    BYTE,
    UTF8,
    SYMBOL_TABLE,
  };

  explicit StringFstNode(ParseMode parse_mode);

  ~StringFstNode() override = default;

  ParseMode GetParseMode() const;

  void Accept(AstWalker* walker) override;

 private:
  const ParseMode parse_mode_;

  StringFstNode(const StringFstNode&) = delete;
  StringFstNode& operator=(const StringFstNode&) = delete;
};

class RepetitionFstNode : public FstNode {
 public:
  enum RepetitionFstNodeType {
    STAR = 0,
    PLUS = 1,
    QUESTION = 2,
    RANGE = 3,
  };

  static absl::string_view RepetitionFstNodeTypeToString(
      RepetitionFstNodeType type);

  explicit RepetitionFstNode(RepetitionFstNodeType type);

  ~RepetitionFstNode() override = default;

  RepetitionFstNodeType GetRepetitionType() const;

  void SetRange(int min, int max);

  void GetRange(int* min, int* max) const;

  void Accept(AstWalker* walker) override;

 private:
  const RepetitionFstNodeType repetition_type_;
  int range_min_;
  int range_max_;

  RepetitionFstNode(const RepetitionFstNode&) = delete;
  RepetitionFstNode& operator=(const RepetitionFstNode&) = delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_AST_FST_NODE_H_
