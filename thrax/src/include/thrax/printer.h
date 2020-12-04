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
#ifndef THRAX_PRINTER_H_
#define THRAX_PRINTER_H_

#include <iostream>
#include <string>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>
#include <thrax/walker.h>

namespace thrax {

class CollectionNode;
class FstNode;
class FunctionNode;
class GrammarNode;
class IdentifierNode;
class ImportNode;
class RepetitionFstNode;
class ReturnNode;
class RuleNode;
class StatementNode;
class StringFstNode;
class StringNode;

// An AST walker that prints out each node as it visits it.
class AstPrinter : public AstWalker {
 public:
  AstPrinter();
  explicit AstPrinter(std::ostream& output_stream);
  ~AstPrinter() override;

  void Visit(CollectionNode* node) override;
  void Visit(FstNode* node) override;
  void Visit(FunctionNode* node) override;
  void Visit(GrammarNode* node) override;
  void Visit(IdentifierNode* node) override;
  void Visit(ImportNode* node) override;
  void Visit(RepetitionFstNode* node) override;
  void Visit(ReturnNode* node) override;
  void Visit(RuleNode* node) override;
  void Visit(StatementNode* node) override;
  void Visit(StringFstNode* node) override;
  void Visit(StringNode* node) override;

  // If true, put line numbers into ast.
  bool include_line_numbers = false;

 private:
  // Returns the spacing prefix for the current nested level, plus the line
  // number if line_numbers is true.
  std::string Spaces(Node* node) const;

  // A scoped counter that increments and decrements the spacing as necessary.
  class ScopedSpaceCounter {
   public:
    explicit ScopedSpaceCounter(int* num_spaces);
    ~ScopedSpaceCounter();

   private:
    int* num_spaces_;
  };

  int num_spaces_;

  mutable bool argument_;

  // This is the actual stream to which we write out, which we do not own.
  std::ostream& out;

  AstPrinter(const AstPrinter&) = delete;
  AstPrinter& operator=(const AstPrinter&) = delete;
};

}  // namespace thrax

#endif  // THRAX_PRINTER_H_
