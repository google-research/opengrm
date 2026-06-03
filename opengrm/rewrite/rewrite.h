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

// Functions for applying rewrite rules to strings or FSTs. Unlike the naive
// approach, the lattices produced by composing the string and the FST rule
// are optimized (e.g., with epsilon-removal and pruned determinization) so
// that the output strings are unique.

#ifndef OPENGRM_REWRITE_REWRITE_H_
#define OPENGRM_REWRITE_REWRITE_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "openfst/extensions/mpdt/compose.h"
#include "openfst/extensions/pdt/compose.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/intersect.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/paths/paths.h"
#include "opengrm/string/stringprint.h"

// Generic rewrite utilities for string inputs.

namespace rewrite {
namespace internal {

// Post-composition check and cleanup.
template <class Arc>
inline bool CheckNonEmptyAndCleanup(::fst::MutableFst<Arc>* lattice) {
  if (lattice->Start() == ::fst::kNoStateId) return false;
  // Projects output side if not already known to be an acceptor.
  if (lattice->Properties(::fst::kAcceptor, /*test=*/false) !=
      ::fst::kAcceptor) {
    ::fst::Project(lattice, ::fst::ProjectType::OUTPUT);
  }
  // Removes epsilons if not already known to be epsilon-free.
  if (lattice->Properties(::fst::kNoEpsilons, /*test=*/false) !=
      ::fst::kNoEpsilons) {
    ::fst::RmEpsilon(lattice);
  }
  return true;
}

}  // namespace internal

// Constructs a weighted, epsilon-free lattice of output strings given a
// input FST and a rule FST.
//
// Callers may wish to arc-sort the input side of the rule ahead of time.
template <class Arc>
bool RewriteLattice(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                    ::fst::MutableFst<Arc>* lattice) {
  static const ::fst::ComposeOptions opts(true, ::fst::ALT_SEQUENCE_FILTER);
  ::fst::Compose(input, rule, lattice, opts);
  return internal::CheckNonEmptyAndCleanup(lattice);
}

// Same as above but supports PDT composition.
template <class Arc>
bool RewriteLattice(
    const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
    ::fst::MutableFst<Arc>* lattice,
    const std::vector<std::pair<typename Arc::Label, typename Arc::Label>>&
        pdt_parens) {
  static const ::fst::PdtComposeOptions opts(true,
                                             ::fst::PdtComposeFilter::EXPAND);
  ::fst::Compose(input, rule, pdt_parens, lattice, opts);
  return internal::CheckNonEmptyAndCleanup(lattice);
}

// Same as above but supports MPDT composition.
template <class Arc>
bool RewriteLattice(
    const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
    ::fst::MutableFst<Arc>* lattice,
    const std::vector<std::pair<typename Arc::Label, typename Arc::Label>>&
        pdt_parens,
    const std::vector<typename Arc::Label>& mpdt_assignments) {
  static const ::fst::MPdtComposeOptions opts(true,
                                              ::fst::PdtComposeFilter::EXPAND);
  ::fst::Compose(input, rule, pdt_parens, mpdt_assignments, lattice, opts);
  return internal::CheckNonEmptyAndCleanup(lattice);
}

// Given an epsilon-free lattice of output strings (such as produced by
// RewriteLattice), attempts to determinize it, pruning non-optimal paths if
// `optimal_only` is true. This is only valid in a semiring with the path
// property.
//
// To prevent unexpected blowup during determinization, a state threshold is
// also used and a warning is logged if this exact threshold is reached. The
// threshold is a multiplier of the size of the input lattice (by default, 4),
// plus a small constant factor. This is intended to be a sensible default
// and is not an inherently meaningful value in and of itself.
template <class Arc>
void LatticeToDfa(::fst::MutableFst<Arc>* lattice, bool optimal_only,
                  typename Arc::StateId state_multiplier = 4) {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  const auto& weight_threshold = optimal_only ? Weight::One() : Weight::Zero();
  const StateId state_threshold = 256 + state_multiplier * lattice->NumStates();
  const ::fst::DeterminizeOptions<Arc> opts(::fst::kDelta, weight_threshold,
                                            state_threshold);
  ::fst::Determinize(*lattice, lattice, opts);
  // Warns if we actually hit the state threshold; if so, we do not have the
  // full set of (optimal) rewrites; there may be cycles of unweighted
  // insertions, or the state threshold may just be too low.
  if (lattice->NumStates() == state_threshold) {
    LOG(WARNING) << "Unexpectedly hit state threshold; consider a higher value "
                    "for state_multiplier";
  }
}

// Given an epsilon-free lattice of output strings (such as produced by
// RewriteLattice), extracts n-shortest unique strings. This is only valid in a
// semiring with the path property.
template <class Arc>
void LatticeToShortest(::fst::MutableFst<Arc>* lattice, int32_t nshortest = 1) {
  ::fst::VectorFst<Arc> shortest;
  // By requesting unique solutions we request on-the-fly determinization.
  ::fst::ShortestPath(*lattice, &shortest, nshortest, /*unique=*/true);
  *lattice = shortest;
}

// Given an epsilon-free lattice of output strings (such as produced by
// RewriteLattice), extracts a single top string. This is only valid in a
// semiring with the path property.
template <class Arc>
bool LatticeToTopString(const ::fst::Fst<Arc>& lattice, std::string* output,
                        ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                        const ::fst::SymbolTable* syms = nullptr) {
  ::fst::VectorFst<Arc> ofst;
  ::fst::ShortestPath(lattice, &ofst);
  return ::fst::StringPrint(ofst, output, ttype, syms);
}

// Same as above but overloaded to also compute the path weight as a float.
template <class Arc>
bool LatticeToTopString(const ::fst::Fst<Arc>& lattice, std::string* output,
                        float* weight,
                        ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                        const ::fst::SymbolTable* syms = nullptr) {
  ::fst::VectorFst<Arc> ofst;
  ::fst::ShortestPath(lattice, &ofst);
  return ::fst::StringPrint(ofst, output, weight, ttype, syms);
}

// Attempts to extract a single top rewrite from a optimized DFA, logging a
// warning and returning false if there's a tie. This is only valid in a
// semiring with the path property.
template <class Arc>
bool LatticeToOneTopString(const ::fst::Fst<Arc>& lattice, std::string* output,
                           ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                           const ::fst::SymbolTable* syms = nullptr) {
  ::fst::StringPathIterator<Arc> paths(lattice, ttype, syms,
                                       /*check_acyclic=*/false);
  if (paths.Error() || paths.Done()) return false;
  *output = paths.OString();
  // Checks for uniqueness.
  paths.Next();
  if (!paths.Done()) {
    LOG(ERROR) << "Multiple top rewrites found: '" << *output << "' and '"
               << paths.OString() << "' (weight: " << paths.Weight() << ")";
    return false;
  }
  return true;
}

// Same as above but overloaded to also compute the path weight as a float.
template <class Arc>
bool LatticeToOneTopString(const ::fst::Fst<Arc>& lattice, std::string* output,
                           float* weight,
                           ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                           const ::fst::SymbolTable* syms = nullptr) {
  ::fst::StringPathIterator<Arc> paths(lattice, ttype, syms,
                                       /*check_acyclic=*/false);
  if (paths.Error() || paths.Done()) return false;
  *output = paths.OString();
  *weight = paths.Weight().Value();
  // Checks for uniqueness.
  paths.Next();
  if (!paths.Done()) {
    LOG(ERROR) << "Multiple top rewrites found: '" << *output << "' and '"
               << paths.OString() << "' (weight: " << paths.Weight() << ")";
    return false;
  }
  return true;
}

// Clears vector and writes lattice labels to it.
template <class Arc>
bool LatticeToLabels(const ::fst::Fst<Arc>& lattice,
                     std::vector<std::vector<typename Arc::Label>>* output) {
  output->clear();
  ::fst::PathIterator<Arc> paths(lattice);
  if (paths.Error()) return false;
  for (; !paths.Done(); paths.Next()) output->emplace_back(paths.OLabels());
  return true;
}

// Clears vector and writes lattice strings to it.
template <class Arc>
bool LatticeToStrings(const ::fst::Fst<Arc>& lattice,
                      std::vector<std::string>* output,
                      ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                      const ::fst::SymbolTable* syms = nullptr) {
  output->clear();
  // We have to do this check manually since StringPathIterator's check is
  // potentially fatal.
  if (lattice.Properties(::fst::kAcyclic, true) != ::fst::kAcyclic) {
    LOG(ERROR) << "Lattice is unexpectedly cyclic";
    return false;
  }
  // Input token type and symbol table will be ignored.
  ::fst::StringPathIterator<Arc> paths(lattice, ttype, syms,
                                       /*check_acyclic=*/false);
  if (paths.Error()) return false;
  for (; !paths.Done(); paths.Next()) {
    // Constructs these in-place.
    output->emplace_back();
    paths.OString(&output->back());
  }
  return true;
}


// Same as above but overloaded to also compute the path weights as floats.
template <class Arc>
bool LatticeToStrings(const ::fst::Fst<Arc>& lattice,
                      std::vector<std::pair<std::string, float>>* output,
                      ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                      const ::fst::SymbolTable* syms = nullptr) {
  output->clear();
  // We have to do this check manually since StringPathIterator's check is
  // potentially fatal.
  if (lattice.Properties(::fst::kAcyclic, true) != ::fst::kAcyclic) {
    LOG(ERROR) << "Lattice is unexpectedly cyclic";
    return false;
  }
  // Input token type and symbol table will be ignored.
  ::fst::StringPathIterator<Arc> paths(lattice, ttype, syms,
                                       /*check_acyclic=*/false);
  if (paths.Error()) return false;
  for (; !paths.Done(); paths.Next()) {
    output->emplace_back(paths.OString(), paths.Weight().Value());
  }
  return true;
}

// Top rewrite.
template <class Arc>
bool TopRewrite(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                std::string* output,
                ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                const ::fst::SymbolTable* syms = nullptr) {
  ::fst::VectorFst<Arc> lattice;
  return RewriteLattice(input, rule, &lattice) &&
         LatticeToTopString(lattice, output, ttype, syms);
}

// Same as above but overloaded to also compute the path weight as a float.
// Top rewrite.
template <class Arc>
bool TopRewrite(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                std::string* output, float* weight,
                ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                const ::fst::SymbolTable* syms = nullptr) {
  ::fst::VectorFst<Arc> lattice;
  return RewriteLattice(input, rule, &lattice) &&
         LatticeToTopString(lattice, output, weight, ttype, syms);
}

// Top rewrite, returning false and logging if there's a tie.
template <class Arc>
bool OneTopRewrite(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                   std::string* output,
                   ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                   const ::fst::SymbolTable* syms = nullptr,
                   typename Arc::StateId state_multiplier = 4) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToDfa(&lattice, /*optimal_only=*/true, state_multiplier);
  return LatticeToOneTopString(lattice, output, ttype, syms);
}

// Same as above but overloaded to also compute the path weight as a float.
template <class Arc>
bool OneTopRewrite(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                   std::string* output, float* weight,
                   ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                   const ::fst::SymbolTable* syms = nullptr,
                   typename Arc::StateId state_multiplier = 4) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToDfa(&lattice, /*optimal_only=*/true, state_multiplier);
  return LatticeToOneTopString(lattice, output, weight, ttype, syms);
}

// All rewrites.
template <class Arc>
bool Rewrites(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
              std::vector<std::string>* output,
              ::fst::TokenType ttype = ::fst::TokenType::BYTE,
              const ::fst::SymbolTable* syms = nullptr,
              typename Arc::StateId state_multiplier = 4) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToDfa(&lattice, /*optimal_only=*/false, state_multiplier);
  return LatticeToStrings(lattice, output, ttype, syms);
}


// Same as above but overloaded to also compute the path weights as floats.
template <class Arc>
bool Rewrites(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
              std::vector<std::pair<std::string, float>>* output,
              ::fst::TokenType ttype = ::fst::TokenType::BYTE,
              const ::fst::SymbolTable* syms = nullptr,
              typename Arc::StateId state_multiplier = 4) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToDfa(&lattice, /*optimal_only=*/false, state_multiplier);
  return LatticeToStrings(lattice, output, ttype, syms);
}

// All optimal rewrites.
template <class Arc>
bool TopRewrites(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                 std::vector<std::string>* output,
                 ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                 const ::fst::SymbolTable* syms = nullptr,
                 typename Arc::StateId state_multiplier = 4) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToDfa(&lattice, /*optimal_only=*/true, state_multiplier);
  return LatticeToStrings(lattice, output, ttype, syms);
}


// Same as above but overloaded to also compute the path weights as floats.
template <class Arc>
bool TopRewrites(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                 std::vector<std::pair<std::string, float>>* output,
                 ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                 const ::fst::SymbolTable* syms = nullptr,
                 typename Arc::StateId state_multiplier = 4) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToDfa(&lattice, /*optimal_only=*/true, state_multiplier);
  return LatticeToStrings(lattice, output, ttype, syms);
}

// The top n rewrites.
template <class Arc>
bool TopRewrites(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                 int32_t nshortest, std::vector<std::string>* output,
                 ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                 const ::fst::SymbolTable* syms = nullptr) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToShortest(&lattice, nshortest);
  return LatticeToStrings(lattice, output, ttype, syms);
}


// Same as above but overloaded to also compute the path weights as floats.
template <class Arc>
bool TopRewrites(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& rule,
                 int32_t nshortest,
                 std::vector<std::pair<std::string, float>>* output,
                 ::fst::TokenType ttype = ::fst::TokenType::BYTE,
                 const ::fst::SymbolTable* syms = nullptr) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  LatticeToShortest(&lattice, nshortest);
  return LatticeToStrings(lattice, output, ttype, syms);
}

// Determines whether a rule allows an input/output pair.
template <class Arc>
bool Matches(const ::fst::Fst<Arc>& input, const ::fst::Fst<Arc>& output,
             const ::fst::Fst<Arc>& rule) {
  ::fst::VectorFst<Arc> lattice;
  if (!RewriteLattice(input, rule, &lattice)) return false;
  static const ::fst::OLabelCompare<Arc> ocomp;
  ::fst::ArcSort(&lattice, ocomp);
  static const ::fst::IntersectOptions opts(true, ::fst::SEQUENCE_FILTER);
  ::fst::Intersect(lattice, output, &lattice, opts);
  return lattice.Start() != ::fst::kNoStateId;
}

}  // namespace rewrite

#endif  // OPENGRM_REWRITE_REWRITE_H_
