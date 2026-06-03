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

#ifndef OPENGRM_SFST_INFO_H_
#define OPENGRM_SFST_INFO_H_

#include <cstddef>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "openfst/lib/fst.h"
#include "openfst/lib/util.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/count.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/trim.h"

namespace sfst {

template <class Arc>
void SfstInfo(const fst::Fst<Arc>& fst, std::ostream& ostrm,
              typename Arc::Label phi_label = fst::kNoLabel,
              float delta = fst::kDelta) {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  StateId start = fst.Start();
  StateId nstates = 0;
  size_t narcs = 0;
  size_t nphis = 0;
  size_t nfinal = 0;
  bool canonical = IsCanonical(fst, phi_label);
  bool trim = IsTrim(fst, phi_label);
  bool backoff = IsBackoffComplete(fst, phi_label);
  bool conservative = IsConservative(fst, delta);
  bool norm = IsNormalized(fst, phi_label, delta);
  std::vector<int> state_order;
  int max_order = 1;
  if (canonical) max_order = PhiStateOrder(fst, phi_label, &state_order);
  std::vector<size_t> order_counts(max_order, 0);
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    ++nstates;
    StateId s = siter.Value();
    if (fst.Final(s) != Weight::Zero()) ++nfinal;
    if (canonical) ++order_counts[state_order[s] - 1];
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      ++narcs;
      const Arc& arc = aiter.Value();
      if (arc.ilabel == phi_label) ++nphis;
    }
  }
  const auto old = ostrm.setf(std::ios::left);
  fst::PrintField(ostrm, "# of states", nstates);
  fst::PrintField(ostrm, "# of arcs", narcs);
  fst::PrintField(ostrm, "# of failure transitions", nphis);
  fst::PrintField(ostrm, "initial state", start);
  fst::PrintField(ostrm, "# of final states", nfinal);
  if (canonical) {
    fst::PrintField(ostrm, "max state order", max_order);
    for (int order = 1; order <= max_order; ++order) {
      std::stringstream label;
      label << "# of order-" << order << " states";
      fst::PrintField(ostrm, label.str(), order_counts[order - 1]);
    }
  }
  fst::PrintField(ostrm, "canonical", (canonical ? 'y' : 'n'));
  fst::PrintField(ostrm, "backoff-complete", (backoff ? 'y' : 'n'));
  fst::PrintField(ostrm, "trim", (trim ? 'y' : 'n'));
  fst::PrintField(ostrm, "conservative", (conservative ? 'y' : 'n'));
  fst::PrintField(ostrm, "normalized", (norm ? 'y' : 'n'));
  fst::PrintField(ostrm, "symbols", (fst.InputSymbols() ? 'y' : 'n'));
  ostrm.setf(old);
}

}  // namespace sfst

#endif  // OPENGRM_SFST_INFO_H_
