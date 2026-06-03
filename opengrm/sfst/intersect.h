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

// Algorithm to intersect two canonical stochastic FSAs.
// The second FSA should be input-epsilon free (when phi_label != 0).

#ifndef OPENGRM_SFST_INTERSECT_H_
#define OPENGRM_SFST_INTERSECT_H_

#include <sys/types.h>

#include "openfst/lib/compose.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/sfst/phi2matcher.h"
#include "opengrm/sfst/trim.h"

namespace sfst {

// Intersects two canonical stochastic FSAs.
// The second FSA must be input-epsilon free (when phi_label != 0).
template <class Arc>
bool Intersect(const fst::Fst<Arc>& ifst1, const fst::Fst<Arc>& ifst2,
               fst::MutableFst<Arc>* ofst,
               typename Arc::Label phi_label = fst::kNoLabel, bool trim = true,
               TrimType trim_type = TRIM_NEEDED_FINAL) {
  using PM = Phi2Matcher<fst::Matcher<fst::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  fst::ComposeFstOptions<Arc, PM, PF> copts;
  copts.gc_limit = 0;
  copts.matcher1 = new PM(ifst1, fst::MATCH_OUTPUT, phi_label);
  copts.matcher2 = new PM(ifst2, fst::MATCH_INPUT, phi_label);
  *ofst = fst::ComposeFst<Arc>(ifst1, ifst2, copts);

  if (trim && !Trim(ofst, phi_label, trim_type)) return false;
  return true;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_INTERSECT_H_
