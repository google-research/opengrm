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
#ifndef THRAX_IDENTIFIER_COUNTER_H_
#define THRAX_IDENTIFIER_COUNTER_H_

#include <string>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/walker.h>
#include <unordered_map>

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

// A AST walker that counts the references to base level identifiers, not
// including the very first time when it's declared/defined.
class AstIdentifierCounter : public AstWalker {
 public:
  AstIdentifierCounter();
  ~AstIdentifierCounter() override;

  void Visit(CollectionNode* node) override;
  void Visit(FstNode* node) override;
  void Visit(GrammarNode* node) override;
  void Visit(IdentifierNode* node) override;
  void Visit(RepetitionFstNode* node) override;
  void Visit(RuleNode* node) override;
  void Visit(StatementNode* node) override;
  void Visit(StringFstNode* node) override;

  // The following functions have no useful work to be done, either because they
  // have no sub-nodes or because we don't care about their contents for
  // identifier counting.
  void Visit(FunctionNode* node) override {}
  void Visit(ImportNode* node) override {}
  void Visit(ReturnNode* node) override {}
  void Visit(StringNode* node) override {}

  // Returns the number of times the identifier was referenced and -1 if the
  // provided name is exported (with an infinite reference count). Crashes if
  // the provided name isn't found.
  int GetCount(const std::string& identifier);

  // Decrements the count of the identifier and returns true if there're still
  // available references remaining (and false otherwise). Crashes if the
  // provided identifier isn't found.
  bool Decrement(const std::string& identifier);

 private:
  // A map of references from the identifier name to the number of times it's
  // used.
  std::unordered_map<std::string, int> references_;
  // A boolean to tell us whether the next IdentifierNode we encounter is
  // exported (and thus should have a reference count of infinity (-1)).
  bool next_identifier_exported_;

  AstIdentifierCounter(const AstIdentifierCounter&) = delete;
  AstIdentifierCounter& operator=(const AstIdentifierCounter&) = delete;
};

}  // namespace thrax

#endif  // THRAX_IDENTIFIER_COUNTER_H_
