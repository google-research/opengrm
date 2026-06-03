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

#ifndef OPENGRM_THRAX_WALKER_RULE_DEPENDENCY_EXTRACTOR_H_
#define OPENGRM_THRAX_WALKER_RULE_DEPENDENCY_EXTRACTOR_H_

#include <deque>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/walker/walker.h"

namespace thrax {

// An AST walker that extracts the (direct) dependencies between rules. If rule
// X includes as part of its construction rule Y, then the dependency X -> Y
// will be added to a table.
class AstRuleDependencyExtractor : public AstWalker {
 public:
  explicit AstRuleDependencyExtractor(
      absl::flat_hash_set<std::string>* dependency_set,
      absl::flat_hash_map<std::string, std::string>* pdt_map);
  ~AstRuleDependencyExtractor() override;

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

 private:
  // A scoped stack that keeps track of the (direct) dependencies between rules
  class ScopedStack {
   public:
    ScopedStack(absl::string_view rule_name,
                std::deque<std::string>* rule_stack,
                absl::flat_hash_set<std::string>* dependency_set);
    ~ScopedStack();

   private:
    absl::flat_hash_set<std::string>* dependency_set_;
    std::deque<std::string>* rule_stack_;
  };

  // Checks if this rule is a PDT, and if it is, adds the relevant information
  // to the pdt_set_.
  // TODO: Do the same for the much less used MpdtCompose.
  void FindPdtComponents(RuleNode* node, absl::string_view rule_name);

  absl::flat_hash_set<std::string>* dependency_set_;
  std::deque<std::string> rule_stack_;
  // The set of PDTs, if any.
  absl::flat_hash_map<std::string, std::string>* pdt_map_;

  AstRuleDependencyExtractor(const AstRuleDependencyExtractor&) = delete;
  AstRuleDependencyExtractor& operator=(const AstRuleDependencyExtractor&) =
      delete;
};

}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_RULE_DEPENDENCY_EXTRACTOR_H_
