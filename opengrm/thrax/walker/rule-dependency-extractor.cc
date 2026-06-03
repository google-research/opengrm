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

#include "opengrm/thrax/walker/rule-dependency-extractor.h"

#include <deque>
#include <map>
#include <set>
#include <string>

#include "absl/base/casts.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/fst-node.h"
#include "opengrm/thrax/ast/function-node.h"
#include "opengrm/thrax/ast/grammar-node.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/import-node.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/ast/return-node.h"
#include "opengrm/thrax/ast/rule-node.h"
#include "opengrm/thrax/ast/statement-node.h"
#include "opengrm/thrax/ast/string-node.h"

namespace thrax {

AstRuleDependencyExtractor::AstRuleDependencyExtractor(
    absl::flat_hash_set<std::string>* dependency_set,
    absl::flat_hash_map<std::string, std::string>* pdt_map)
    : dependency_set_(dependency_set), pdt_map_(pdt_map) {}

AstRuleDependencyExtractor::~AstRuleDependencyExtractor() = default;

void AstRuleDependencyExtractor::Visit(CollectionNode* node) {
  for (int i = 0; i < node->Size(); ++i) {
    (*node)[i]->Accept(this);
  }
}

void AstRuleDependencyExtractor::Visit(FstNode* node) {
  if (node->GetType() == FstNode::IDENTIFIER_FSTNODE) {
    ScopedStack stack(
        absl::down_cast<IdentifierNode*>(node->GetArgument(0))->Get(),
        &rule_stack_, dependency_set_);
  }
  if (node->NumArguments() > 0) {
    for (int i = 0; i < node->NumArguments(); ++i) {
      node->GetArgument(i)->Accept(this);
    }
  }
}

void AstRuleDependencyExtractor::Visit(FunctionNode* node) {
  node->GetName()->Accept(this);
  node->GetArguments()->Accept(this);
  node->GetBody()->Accept(this);
}

void AstRuleDependencyExtractor::Visit(GrammarNode* node) {
  node->GetImports()->Accept(this);
  node->GetFunctions()->Accept(this);
  node->GetStatements()->Accept(this);
}

void AstRuleDependencyExtractor::Visit(IdentifierNode* node) {}

void AstRuleDependencyExtractor::Visit(ImportNode* node) {
  node->GetPath()->Accept(this);
  node->GetAlias()->Accept(this);
}

void AstRuleDependencyExtractor::Visit(RepetitionFstNode* node) {
  // Use the base FstNode version.
  Visit(absl::implicit_cast<FstNode*>(node));
}

void AstRuleDependencyExtractor::Visit(ReturnNode* node) {
  node->Get()->Accept(this);
}

void AstRuleDependencyExtractor::Visit(RuleNode* node) {
  const std::string& rule_name = node->GetName()->Get();
  ScopedStack stack(rule_name, &rule_stack_, dependency_set_);
  node->GetName()->Accept(this);
  node->Get()->Accept(this);
  FindPdtComponents(node, rule_name);
}

void AstRuleDependencyExtractor::Visit(StatementNode* node) {
  node->Get()->Accept(this);
}

void AstRuleDependencyExtractor::Visit(StringFstNode* node) {
  // Use the base FstNode version.
  Visit(absl::implicit_cast<FstNode*>(node));
}

void AstRuleDependencyExtractor::Visit(StringNode* node) {}

void AstRuleDependencyExtractor::FindPdtComponents(
    RuleNode* node, absl::string_view rule_name) {
  // Walks the rather overly articulated tree of nodes to collect all of the
  // needed information---name of the parens, directionality of the PDT---and if
  // the collection is successful, it places the information into the pdt_map_.
  FstNode* fst_node = absl::down_cast<FstNode*>(node->Get());
  CHECK(fst_node) << "FindPdtComponents: Cannot find fstnode.";
  if (fst_node->GetType() == FstNode::FUNCTION_FSTNODE) {
    std::string node_name =
        absl::down_cast<IdentifierNode*>(fst_node->GetArgument(0))->Get();
    // The node is a PDT. In that case the third argument of the associated
    // CollectionNode is an IDENTIFIER_FSTNODE, representing the parens, and the
    // fourth argument gives the direction of the composition. Note that while
    // in theory one could provide the parens via an FST constructed within this
    // expression, that would not be of much use as a PDT since in any case to
    // use it as a PDT, one would need to have the parens exported as a
    // variable.
    if (node_name == "PdtCompose") {
      CollectionNode* collection_node =
          absl::down_cast<CollectionNode*>(fst_node->GetArgument(1));
      if (collection_node) {
        CHECK_EQ(collection_node->Size(), 4)
            << "FindPdtComponents: Collection has wrong size.";
        FstNode* parens = absl::down_cast<FstNode*>(collection_node->Get(2));
        CHECK(parens) << "FindPdtComponents: Cannot find parens.";
        if (parens->GetType() == FstNode::IDENTIFIER_FSTNODE) {
          FstNode* parens_fst =
              absl::down_cast<FstNode*>(collection_node->Get(2));
          CHECK(parens_fst) << "FindPdtComponents: Cannot find parens fst.";
          std::string parens_name =
              absl::down_cast<IdentifierNode*>(parens_fst->GetArgument(0))
                  ->Get();
          std::string direction =
              absl::down_cast<IdentifierNode*>(collection_node->Get(3))->Get();
          LOG(INFO) << "Found PDT rule " << rule_name
                    << " with parens=" << parens_name << " and direction "
                    << direction << ".";
          pdt_map_->emplace(rule_name,
                            absl::StrCat(parens_name, " ", direction));
        }
      }
    }
  }
}

AstRuleDependencyExtractor::ScopedStack::ScopedStack(
    absl::string_view rule_name, std::deque<std::string>* rule_stack,
    absl::flat_hash_set<std::string>* dependency_set)
    : dependency_set_(dependency_set), rule_stack_(rule_stack) {
  rule_stack_->push_front(std::string(rule_name));
}

AstRuleDependencyExtractor::ScopedStack::~ScopedStack() {
  if (rule_stack_->empty()) return;
  std::string dependent = rule_stack_->front();
  rule_stack_->pop_front();
  if (rule_stack_->empty()) return;
  std::string head = rule_stack_->front();
  // No rule can be dependent on itself, and if these are equal it's an artifact
  // of the way in which rule names are encoded as identity nodes.
  if (head == dependent) return;
  dependency_set_->insert(absl::StrCat(head, " ", dependent));
}

}  // namespace thrax
