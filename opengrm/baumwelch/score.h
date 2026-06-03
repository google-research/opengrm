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

// Primitive string scoring functions.
//
// By convention, operations in this library that work with FarReader input
// reset the FAR to its initial position upon completion.

#ifndef OPENGRM_BAUMWELCH_SCORE_H_
#define OPENGRM_BAUMWELCH_SCORE_H_

#include <cstddef>
#include <string>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/string.h"
#include "opengrm/string/stringprint.h"

namespace fst {

// Computes number of errors, assuming the trivial left-to-right alignment.
inline size_t HammingDistance(const std::string& gld, const std::string& hyp) {
  // We assume the former is at least as long as the latter. If this is not
  // the case, re-invoke with the arguments swapped.
  if (gld.size() < hyp.size()) return HammingDistance(hyp, gld);
  size_t dist = gld.size() - hyp.size();
  for (size_t i = 0; i < hyp.size(); ++i) {
    if (gld[i] != hyp[i]) ++dist;
  }
  return dist;
}

// Same, but with FST inputs.
template <class Arc>
size_t HammingDistance(const Fst<Arc>& gld, const Fst<Arc>& hyp,
                       TokenType ttype = TokenType::BYTE,
                       const SymbolTable* syms = nullptr) {
  std::string gld_str;
  CHECK(StringPrint(gld, &gld_str, ttype, syms)) << "String printing failed";
  std::string hyp_str;
  CHECK(StringPrint(hyp, &hyp_str, ttype, syms)) << "String printing failed";
  return HammingDistance(gld_str, hyp_str);
}

// Same, but summing across all FSTs in a pair of FAR inputs. This is only
// sensible when both FARs have the same number of entries and a consistent
// iteration order.
template <class Arc>
size_t HammingDistance(FarReader<Arc>& gld, FarReader<Arc>& hyp,
                       TokenType ttype = TokenType::BYTE,
                       const SymbolTable* syms = nullptr) {
  size_t dist = 0;
  while (!gld.Done() || !hyp.Done()) {
    dist += HammingDistance(*gld.GetFst(), *hyp.GetFst(), ttype, syms);
    gld.Next();
    hyp.Next();
  }
  gld.Reset();
  hyp.Reset();
  return dist;
}

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_SCORE_H_
