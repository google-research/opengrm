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

// Computes the stationary distribution of a stochastic FST.

#ifndef OPENGRM_SFST_STATIONARY_DISTRIB_H_
#define OPENGRM_SFST_STATIONARY_DISTRIB_H_

#include <cstddef>
#include <vector>

#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/rmphi.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {

namespace internal {

// Corresponds to a re-entry prob = 99.9999%
const fst::SignedLog64Weight kReEntryWeight(fst::SignedLog64Weight::W1(1.0),
                                            fst::SignedLog64Weight::W2(1.0e-6));
// Maximum number of stationary distribution iterations
constexpr size_t kMaxSDIters = 100;

// Computes the stationary distribution of the Markov chain consisting
// of the closure of the input stochastic FST with re-entry weight
// 'alpha'. This version is on the signed log semiring. The
// convergence is controlled by 'delta' and the number of iterations
// by 'maxiters' Epsilons (but no epsilon cycles) are
// permitted. Returns true on convergence.
//
// For cyclic input and 'negative' weights, any convergence is, in
// general, conditional and not absolute, so it will depend on the
// specific input.
bool SignedStationaryDistrib(
    const fst::Fst<fst::SignedLog64Arc>& fst,
    std::vector<fst::SignedLog64Weight>* weight,
    fst::SignedLog64Weight alpha = internal::kReEntryWeight,
    float delta = fst::kDelta, size_t maxiters = internal::kMaxSDIters);

}  // namespace internal

// Computes the stationary distribution of the Markov chain consisting
// of the closure of the input stochastic FST with re-entry weight
// 'alpha'. This version handles failure transitions when present.
// The convergence is controlled by 'delta' and the number of
// iterations by 'maxiters'. Returns true on convergence.  Assumes
// (but does not check) that the input has the canonical topology (see
// canonical.h).  Also assumes input has no (non-phi) epsilons (or
// treats such epsilons w.r.t. the failure semantics as if they were
// regular, uniquely-labeled symbols).
template <class Arc>
bool StationaryDistrib(const fst::Fst<Arc>& fst,
                       std::vector<typename Arc::Weight>* weight,
                       typename Arc::Weight alpha,
                       typename Arc::Label phi_label = fst::kNoLabel,
                       float delta = fst::kDelta,
                       size_t max_iters = internal::kMaxSDIters) {
  using Weight = typename Arc::Weight;
  using SLArc = fst::SignedLog64Arc;
  using SLWeight = SLArc::Weight;

  fst::VectorFst<SLArc> slfst;
  internal::RmPhi(fst, &slfst, phi_label);
  fst::WeightConvert<Weight, SLWeight> to_sl_convert;
  auto slalpha = to_sl_convert(alpha);

  std::vector<SLWeight> slweight;
  if (!internal::SignedStationaryDistrib(slfst, &slweight, slalpha, delta,
                                         max_iters))
    return false;

  fst::WeightConvert<SLWeight, Weight> from_sl_convert;
  weight->clear();
  for (size_t i = 0; i < slweight.size(); ++i) {
    auto w = slweight[i];
    if (Less(w, SLWeight::Zero())) w = SLWeight::Zero();
    weight->push_back(from_sl_convert(w));
  }
  return true;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_STATIONARY_DISTRIB_H_
