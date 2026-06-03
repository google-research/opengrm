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

// See sfst.opengrm.org for documentation of this library for
// stochastic finite-state transducers.

// SFst is a library for normalizing, sampling, combining, and
// approximating stochastic (or probabilistic) finite-state
// transducers. These are weighted finite-state transducers,
// represented in OpenFst library format, that have the following two
// properties:
//
// 1. normalized: the paths into the future from each state sum to
// Weight::One().
//
// 2. canonical topology:
//   * the states are sorted by input label
//   * there may be failure transitions (see phi-labeled transitions) but
//      * there is at most one such transition per state
//      * there are no failure-transition (and/or epsilon-transition) cycles
//   * no assumption is made of general determinism or what transitions must be
//     present on failure (unlike in a canonical n-gram model).
//
//
// For example, an n-gram model produced by the OpenGrm NGram Library
// is a stochastic FST (provided the phi_label is specified to match
// the backoff label, typically 0, of the n-gram model), but many
// other topologies are possible.
//
// If the input automaton is a transducer, then unless otherwise
// stated, the algorithms here only examine the input labels.
//
// Computation is done internally with Log64Weight (or sometimes
// SignedLog64Weight). Conversion from the input weight type is done
// using a fst::WeightConvert functor, pre-defined for common
// weight types like TropicalWeight and LogWeight.

// This convenience file includes all other SFST header files.

#ifndef OPENGRM_SFST_SFSTLIB_H_
#define OPENGRM_SFST_SFSTLIB_H_

// Do not let Include-What-You-Use suggest this file.
// IWYU pragma: private
#include "opengrm/sfst/approx.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/count.h"
#include "opengrm/sfst/equal.h"
#include "opengrm/sfst/intersect.h"
#include "opengrm/sfst/ngramapprox.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/perplexity.h"
#include "opengrm/sfst/phi2matcher.h"
#include "opengrm/sfst/randgen.h"
#include "opengrm/sfst/rmphi.h"
#include "opengrm/sfst/sfst.h"
#include "opengrm/sfst/shortest-distance.h"
#include "opengrm/sfst/state-weights.h"
#include "opengrm/sfst/stationary-distrib.h"
#include "opengrm/sfst/topology.h"
#include "opengrm/sfst/trim.h"

#endif  // OPENGRM_SFST_SFSTLIB_H_
