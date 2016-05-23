
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
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
// An Fst arc type for histograms.

#ifndef NGRAM_HIST_ARC_H_
#define NGRAM_HIST_ARC_H_

#include <cctype>
#include <cmath>
#include <list>
#include <sstream>
#include <string>

#include <fst/types.h>
#include <fst/arc.h>

namespace ngram {

// Number of bins in histogram arc weights.  Set to 2 more than the highest
// count that receives a Katz backoff estimate, to allow for bins representing
// the 0 count and counts higher than the cutoff.
const int kHistogramBins = 7;

}  // namespace ngram

namespace fst {

// Histogram Arc is a cartesian product of Std Arcs.
struct HistogramArc : public PowerArc<StdArc, ngram::kHistogramBins> {
  HistogramArc(Label i, Label o, Weight w, StateId s)
      : ilabel(i), olabel(o), weight(w), nextstate(s) {}

  HistogramArc() {}

  static const string &Type() {  // Arc type name
    static const string type = "hist";
    return type;
  }

  Label ilabel;       // Transition input label
  Label olabel;       // Transition output label
  Weight weight;      // Transition weight
  StateId nextstate;  // Transition destination state
};

}  // namespace fst

#endif  // NGRAM_HIST_ARC_H_
