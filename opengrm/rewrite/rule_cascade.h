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

// RuleCascade is an implementation of BaseRuleCascade that wraps around a
// RewriteManager. RuleCascade can be used to apply a cascade of rules in a FAR
// to strings.

#ifndef OPENGRM_REWRITE_RULE_CASCADE_H_
#define OPENGRM_REWRITE_RULE_CASCADE_H_

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/rewrite/base_rule_cascade.h"
#include "opengrm/rewrite/rewrite_manager.h"

namespace rewrite {
namespace internal {

// Helper class for rule triples (rule name, optional PDT parentheses rule name,
// optional MPDT assignment rule name).
class RuleTriple {
 public:
  explicit RuleTriple(absl::string_view rule_def)
      : RuleTriple(RuleTriple::ParseRuleDef(rule_def)) {}

  const std::string& Rule() const { return rules_[0]; }

  bool HasPdtParens() const { return !rules_[1].empty(); }

  const std::string& PdtParens() const { return rules_[1]; }

  bool HasMPdtAssignments() const { return !rules_[2].empty(); }

  const std::string& MPdtAssignments() const { return rules_[2]; }

 private:
  explicit RuleTriple(std::array<std::string, 3> rules)
      : rules_(std::move(rules)) {}

  // Actually parses the rules into the pieces we need.
  static std::array<std::string, 3> ParseRuleDef(absl::string_view rule_def) {
    std::array<std::string, 3> result;
    auto main_pos = rule_def.find('$');
    if (main_pos == std::string::npos) main_pos = rule_def.find(':');
    result[0] = rule_def.substr(0, main_pos);
    if (main_pos == std::string::npos) return result;
    auto pdt_parens_pos = rule_def.find('$', main_pos + 1);
    if (pdt_parens_pos == std::string::npos) {
      pdt_parens_pos = rule_def.find(':', main_pos + 1);
    }
    if (pdt_parens_pos == std::string::npos) {
      result[1] = rule_def.substr(main_pos + 1);
      return result;
    }
    result[1] = rule_def.substr(main_pos + 1, pdt_parens_pos - main_pos - 1);
    result[2] = rule_def.substr(pdt_parens_pos + 1);
    return result;
  }

  const std::array<std::string, 3> rules_;
};

}  // namespace internal

template <class Arc>
class RuleCascade : public BaseRuleCascade<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using SymbolTable = ::fst::SymbolTable;

  // Do not use the manager until FSTs are loaded (with Load) and rules are
  // set (with SetRules).
  explicit RuleCascade(::fst::TokenType token_type = ::fst::TokenType::BYTE)
      : BaseRuleCascade<Arc>(token_type), manager_(token_type) {}

  // Loads rules from a FAR.
  absl::Status LoadWithStatus(absl::string_view filename);

  // Sets up rule spec; can be called repeatedly.
  absl::Status SetRulesWithStatus(absl::Span<const std::string> rules);


  [[deprecated("Use SetRulesWithStatus instead.")]]
  bool SetRules(absl::Span<const std::string> rules);


  bool Rewrite(
      const typename BaseRuleCascade<Arc>::Transducer& input,
      typename BaseRuleCascade<Arc>::MutableTransducer* output) const final;

  const SymbolTable* GeneratedSymbols() const final {
    return manager_.GeneratedSymbols();
  }

 private:
  absl::Status ValidateRules();

  RewriteManager<Arc> manager_;
  std::vector<internal::RuleTriple> triples_;
};

template <class Arc>
absl::Status RuleCascade<Arc>::LoadWithStatus(absl::string_view filename) {
  return manager_.LoadWithStatus(filename);
}

template <class Arc>
absl::Status RuleCascade<Arc>::SetRulesWithStatus(
    absl::Span<const std::string> rules) {
  triples_.clear();
  for (const auto& rule : rules) triples_.emplace_back(rule);
  return ValidateRules();
}


template <class Arc>
bool RuleCascade<Arc>::SetRules(absl::Span<const std::string> rules) {
  if (const absl::Status status = SetRulesWithStatus(rules); !status.ok()) {
    LOG(ERROR) << status;
    return false;
  }
  return true;
}


template <class Arc>
bool RuleCascade<Arc>::Rewrite(
    const typename BaseRuleCascade<Arc>::Transducer& input,
    typename BaseRuleCascade<Arc>::MutableTransducer* output) const {
  *output = input;
  for (const auto& triple : triples_) {
    if (!manager_.RewriteLattice(triple.Rule(), *output, output,
                                 triple.PdtParens(),
                                 triple.MPdtAssignments())) {
      return false;
    }
  }
  return true;
}

// Private methods.

template <class Arc>
absl::Status RuleCascade<Arc>::ValidateRules() {
  if (triples_.empty()) {
    return absl::InvalidArgumentError("No rules defined");
  }
  for (const auto& triple : triples_) {
    if (!manager_.GetFst(triple.Rule())) {
      return absl::InvalidArgumentError(
          absl::StrCat("Cannot find rule: ", triple.Rule()));
    }
    if (triple.HasPdtParens() && !manager_.GetFst(triple.PdtParens())) {
      return absl::InvalidArgumentError(
          absl::StrCat("Cannot find PDT parens: ", triple.PdtParens()));
    }
    if (triple.HasMPdtAssignments() &&
        !manager_.GetFst(triple.MPdtAssignments())) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Cannot find MPDT assignments: ", triple.MPdtAssignments()));
    }
  }
  return absl::OkStatus();
}

using StdRuleCascade = RuleCascade<::fst::StdArc>;

}  // namespace rewrite

#endif  // OPENGRM_REWRITE_RULE_CASCADE_H_
