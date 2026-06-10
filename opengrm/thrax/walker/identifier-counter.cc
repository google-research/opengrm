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

#include "opengrm/thrax/walker/identifier-counter.h"

#include <string>

#include "absl/base/casts.h"
#include "absl/container/node_hash_map.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/collection-node.h"
#include "opengrm/thrax/ast/fst-node.h"
#include "opengrm/thrax/ast/grammar-node.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/ast/rule-node.h"
#include "opengrm/thrax/ast/statement-node.h"

namespace thrax {

AstIdentifierCounter::AstIdentifierCounter()
    : next_identifier_exported_(false) {}

AstIdentifierCounter::~AstIdentifierCounter() = default;

void AstIdentifierCounter::Visit(CollectionNode* node) {
  for (int i = 0; i < node->Size(); ++i) (*node)[i]->Accept(this);
}

void AstIdentifierCounter::Visit(FstNode* node) {
  for (int i = 0; i < node->NumArguments(); ++i)
    node->GetArgument(i)->Accept(this);
}

void AstIdentifierCounter::Visit(GrammarNode* node) {
  // We only care about the actual statements, and not the imports or functions.
  node->GetStatements()->Accept(this);
}

void AstIdentifierCounter::Visit(IdentifierNode* node) {
  if (node->HasNamespaces())  // We only care about local variables.
    return;

  const std::string& name = node->Get();
  if (next_identifier_exported_) {
    references_[name] = -1;  // -1 = infinite
  } else {
    const auto [it, inserted] = references_.insert({name, 0});
    if (!inserted) {
      // Only increment the reference count if we had a pre-existing value.
      int& count = it->second;  // Previous value.
      if (count != -1) ++count;
    }
  }
}

void AstIdentifierCounter::Visit(RepetitionFstNode* node) {
  Visit(absl::implicit_cast<FstNode*>(node));
}

void AstIdentifierCounter::Visit(RuleNode* node) {
  next_identifier_exported_ = node->ShouldExport();
  node->GetName()->Accept(this);
  next_identifier_exported_ = false;

  node->Get()->Accept(this);
}

void AstIdentifierCounter::Visit(StatementNode* node) {
  node->Get()->Accept(this);
}

void AstIdentifierCounter::Visit(StringFstNode* node) {
  Visit(absl::implicit_cast<FstNode*>(node));
}

bool AstIdentifierCounter::Decrement(absl::string_view identifier) {
  absl::node_hash_map<std::string, int>::iterator where =
      references_.find(identifier);
  if (where == references_.end())
    LOG(FATAL) << "Identifier " << identifier << " not found";
  if (where->second < 0) return true;
  if (where->second == 0) return false;
  VLOG(3) << "Decrementing count for identifier " << identifier << " from "
          << where->second << " to " << where->second - 1 << ".";
  return --where->second;
}

}  // namespace thrax
