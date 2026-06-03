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

// Algorithm to approximate a stochastic FST as a backoff FST.
// The backoff FST topology is provided. The result is the backoff
// FST weighted and normalized to approximate the input.

#ifndef OPENGRM_SFST_APPROX_H_
#define OPENGRM_SFST_APPROX_H_

#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "opengrm/sfst/count.h"
#include "opengrm/sfst/normalize.h"

namespace sfst {

// A float delta for SFST approximation algorithms.
constexpr float kApproxDelta = 1e-10;

// Approximates a stochastic FSA as a backoff FSA.  The input 'ifst'
// should be a canonical stochastic FSA (see canonical.h). If it is
// cyclic, it should be normalized (see normalize.h - not
// checked). Assumes input has no (non-phi) epsilons (or treats such
// epsilons w.r.t. the failure semantics as if they were regular,
// uniquely-labeled symbols).  The topology FST is provided in 'ofst'
// (with incoming weights ignored) and must be a backoff-complete FSA (see
// backoff.h). The 'phi_label' is the failure label (defaults to
// kNoLabel -> None). The 'delta' parameter controls the degree of
// and algorithm convergence.  The result is the backoff-complete FSA weighted
// normalized to approximate the input.  The algorithm computes (smoothed)
// counts and then normalizes those counts. See sfst::CountNormType
// for the normalization variants. Returns true on success.
template <class Arc>
bool Approx(const fst::Fst<Arc>& ifst, fst::MutableFst<Arc>* ofst,
            typename Arc::Label phi_label = fst::kNoLabel,
            float delta = kApproxDelta, CountNormType norm_type = NORM_KL_MIN) {
  {  // Counts the n-grams.
    Counter<Arc> counter(phi_label, delta, ofst);
    counter.Count(ifst);
    counter.Finalize();
    if (ofst->Properties(fst::kError, false)) return false;
  }

  return CountNormalize(ofst, phi_label, norm_type, true);
}

}  // namespace sfst

#endif  // OPENGRM_SFST_APPROX_H_
