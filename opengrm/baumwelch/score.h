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

#include <algorithm>
#include <cstddef>
#include <vector>

#include "absl/log/check.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"

namespace fst {

// Computes number of errors between two sequences, assuming the trivial
// left-to-right alignment.
template <class T>
size_t HammingDistance(absl::Span<const T> gld, absl::Span<const T> hyp) {
  // We assume the former is at least as long as the latter. If this is not
  // the case, re-invoke with the arguments swapped.
  if (gld.size() < hyp.size()) return HammingDistance(hyp, gld);
  size_t dist = gld.size() - hyp.size();
  for (size_t i = 0; i < hyp.size(); ++i) {
    if (gld[i] != hyp[i]) ++dist;
  }
  return dist;
}

// Same, for byte strings.
inline size_t HammingDistance(absl::string_view gld, absl::string_view hyp) {
  return HammingDistance(absl::Span<const char>(gld.data(), gld.size()),
                         absl::Span<const char>(hyp.data(), hyp.size()));
}

// Same, for string FSTs, comparing non-epsilon output label sequences directly.
template <class Arc>
size_t HammingDistance(const Fst<Arc>& gld, const Fst<Arc>& hyp) {
  using Label = typename Arc::Label;
  CHECK(CompatSymbols(gld.OutputSymbols(), hyp.OutputSymbols()))
      << "Output symbol table mismatch";
  std::vector<Label> gld_labels;
  CHECK(StringFstToOutputLabels(gld, &gld_labels)) << "Label extraction failed";
  std::vector<Label> hyp_labels;
  CHECK(StringFstToOutputLabels(hyp, &hyp_labels)) << "Label extraction failed";
  gld_labels.erase(std::remove(gld_labels.begin(), gld_labels.end(), 0),
                   gld_labels.end());
  hyp_labels.erase(std::remove(hyp_labels.begin(), hyp_labels.end(), 0),
                   hyp_labels.end());
  return HammingDistance(absl::MakeConstSpan(gld_labels),
                         absl::MakeConstSpan(hyp_labels));
}

// Same, but summing across all FSTs in a pair of FAR inputs. This is only
// sensible when both FARs have the same number of entries and a consistent
// iteration order.
template <class Arc>
size_t HammingDistance(FarReader<Arc>& gld, FarReader<Arc>& hyp) {
  size_t dist = 0;
  while (!gld.Done() && !hyp.Done()) {
    dist += HammingDistance(*gld.GetFst(), *hyp.GetFst());
    gld.Next();
    hyp.Next();
  }
  CHECK(gld.Done() && hyp.Done()) << "FAR reader size mismatch";
  gld.Reset();
  hyp.Reset();
  return dist;
}

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_SCORE_H_
