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

// Computes the stationary distribution of a stochastic FST

#include "opengrm/sfst/stationary-distrib.h"

#include <cmath>
#include <cstddef>
#include <vector>

#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/signed-log-weight.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {

namespace internal {

// At a given state, calculate one step of the power method
// for the stationary distribution of the closure of the
// input stochastic FST with re-entry weight 'alpha'.
// TODO: handle epsilons - pushed?
void SignedStationaryDistribState(
    const fst::Fst<fst::SignedLog64Arc>& fst, int st,
    std::vector<fst::SignedLog64Weight>* prev_weight,
    std::vector<fst::SignedLog64Weight>* weight, fst::SignedLog64Weight alpha) {
  using SLArc = fst::SignedLog64Arc;
  using SLWeight = SLArc::Weight;

  SLWeight prevw = (*prev_weight)[st];
  for (fst::ArcIterator<fst::Fst<SLArc>> aiter(fst, st); !aiter.Done();
       aiter.Next()) {
    const SLArc& arc = aiter.Value();
    SLWeight nextw = Times(prevw, arc.weight);
    if (arc.ilabel != 0) {
      (*weight)[arc.nextstate] = Plus((*weight)[arc.nextstate], nextw);
    } else {
      (*prev_weight)[arc.nextstate] =
          Plus((*prev_weight)[arc.nextstate], nextw);
    }
  }

  if (fst.Final(st) != SLWeight::Zero()) {
    (*weight)[fst.Start()] =
        Plus((*weight)[fst.Start()], Times(prevw, Times(fst.Final(st), alpha)));
  }
}

bool SignedStationaryDistrib(const fst::Fst<fst::SignedLog64Arc>& fst,
                             std::vector<fst::SignedLog64Weight>* weight,
                             fst::SignedLog64Weight alpha /* kReEntryWeight */,
                             float delta /* = fst::kDelta */,
                             size_t maxiters /* kMaxSDIters */) {
  using SLArc = fst::SignedLog64Arc;
  using StateId = SLArc::StateId;
  using SLWeight = SLArc::Weight;
  using LWeight = fst::Log64Weight;

  std::vector<StateId> top_order;
  if (!PhiTopOrder(fst, fst::kNoLabel, &top_order)) {  // epsilon top order
    LOG(ERROR) << "SignedStationaryDistrib: "
               << "FST has (input) epsilon cycles";
    return false;
  }

  size_t nstates = top_order.size();
  LWeight ldelta(-std::log(delta));

  std::vector<SLWeight> prev_weight, tmp_weight;
  // Initializes to the uniform distribution
  prev_weight.resize(
      nstates, SLWeight(SLWeight::W1(1.0),
                        SLWeight::W2(std::log(static_cast<float>(nstates)))));

  size_t changed;
  size_t niter = 0;
  do {
    weight->clear();
    weight->resize(nstates, SLWeight::Zero());
    tmp_weight = prev_weight;
    for (size_t i = 0; i < nstates; ++i) {
      StateId st = top_order[i];  // ith state in epsilon top order
      SignedStationaryDistribState(fst, st, &tmp_weight, weight, alpha);
    }

    changed = 0;
    for (size_t st = 0; st < nstates; ++st) {
      LWeight dw = Minus((*weight)[st], prev_weight[st]).Value2();
      LWeight dp = Times(ldelta, prev_weight[st].Value2());
      if (Less(dp, dw)) ++changed;
      prev_weight[st] = (*weight)[st];
    }
    ++niter;

    // Second clause tests for periodic states
    if (niter > maxiters ||
        (niter > nstates && ApproxZero((*weight)[fst.Start()])))
      return false;

    VLOG(2) << "SignedStationaryDistrib: state weights changed: " << changed;
  } while (changed > 0);

  return true;
}

}  // namespace internal

}  // namespace sfst
