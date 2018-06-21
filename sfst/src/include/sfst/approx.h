
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2018 Google, Inc.
// appox.h

// Algorithm to approximate a stochastic FST as a backoff FST.
// The backoff FST topology is provided. The result is the backoff
// FST weighted and normalized to approximate the input.

#ifndef SFST_APPROX_H_
#define SFST_APPROX_H_

#include <string>
#include <vector>

#include <fst/log.h>
#include <fst/fst.h>
#include <sfst/backoff.h>
#include <sfst/count.h>
#include <sfst/sfst.h>
#include <sfst/normalize.h>
#include <sfst/trim.h>

namespace sfst {

// A float delta for SFST approximation algorithms.
constexpr float kApproxDelta = 1e-9;

// Approximates a stochastic FSA as a backoff FSA.  The input 'ifst'
// should be a canonical stochastic FSA (see canonical.h). If it is
// cyclic, it should be normalized (see normalize.h - not
// checked). Assumes input has no (non-phi) epsilons (or treats such
// epsilons w.r.t. the failure semantics as if they were regular,
// uniquely-labeled symbols).  The topology FST is provided in 'ofst'
// (with incoming weights ignored) and must be a backoff FSA (see
// backoff.h). The 'phi_label' is the failure label (defaults to
// kNoLabel -> None). The 'delta' parameter controls the degree of
// algorithm convergence.  The result is the backoff FSA weighted and
// normalized to approximate the input.  The algorithm computes
// (smoothed) counts and then normalizes those counts. See
// sfst::CountNormType for the normalization variants. Returns true on
// success.
template <class Arc>
bool Approx(const fst::Fst<Arc> &ifst,
            fst::MutableFst<Arc> *ofst,
            typename Arc::Label phi_label = fst::kNoLabel,
            float delta = kApproxDelta,
            CountNormType norm_type = NORM_SUMMED) {
  namespace f = fst;
  using Label = typename Arc::Label;

  {  // Counts the n-grams.
    Counter<Arc> counter(phi_label, delta, ofst);
    counter.Count(ifst);
    counter.Finalize();
  }
  if (ofst->Properties(f::kError, false))
      return false;

  return CountNormalize(ofst, phi_label,  f::kDelta, norm_type);
}

}  // namespace sfst

#endif  // SFST_APPROX_H_
