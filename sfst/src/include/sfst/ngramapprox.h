
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
// ngramappox.h

// Algorithm to approximate a stochastic FST as an n-gram model.
// Output is a canonical and normalized OpenGrm ngram model.

#ifndef SFST_NGRAMAPPROX_H_
#define SFST_NGRAMAPPROX_H_

#include <string>
#include <vector>

#include <fst/log.h>
#include <fst/fst.h>
#include <fst/relabel.h>
#include <sfst/approx.h>
#include <sfst/sfst.h>
#include <sfst/topology.h>

namespace sfst {

// Approximates a stochastic FSA as an n-gram model of order 'order'.  The input
// FST should be a canonical stochastic FSA (see canonical.h).  If it is cyclic,
// it should be normalized (see normalize.h - not checked).  Assumes input has
// no (non-phi) epsilons (or treats such epsilons w.r.t. the failure semantics
// as if they were regular, uniquely-labeled symbols).  The 'phi_label' is the
// failure label (defaults to OpenGrm NGram backoff label of 0). The 'delta'
// parameter controls the degree of algorithm convergence. The result is a
// canonical and normalized OpenGrm ngram model FST.  The algorithm computes
// (smoothed) counts and then normalizes those counts. See sfst::CountNormType
// for the normalization variants. Returns true on success.
template <class Arc>
bool NGramApprox(const fst::Fst<Arc> &ifst,
                 fst::MutableFst<Arc> *ofst, int order,
                 typename Arc::Label phi_label = 0,
                 float delta = sfst::kApproxDelta,
                 CountNormType norm_type = NORM_KL_MIN) {
  namespace f = fst;
  using Label = typename Arc::Label;

  {  // Finds the n-gram topology.
    NGramTopology<Arc> ngram(order, phi_label, ofst);
    ngram.FindNGrams(ifst);
    if (ofst->Properties(f::kError, false)) return false;
  }

  return Approx(ifst, ofst, phi_label, delta, norm_type);
}

}  // namespace sfst

#endif  // SFST_NGRAMAPPROX_H_
