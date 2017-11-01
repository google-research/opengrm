
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Mapper to/from histogram arc.

#ifndef NGRAM_HIST_MAPPER_H_
#define NGRAM_HIST_MAPPER_H_

#include <fst/fst.h>
#include <fst/map.h>
#include <ngram/hist-arc.h>

using ngram::kHistogramBins;

namespace fst {

// Mapper (from HistogramArc to StdArc)
// The value at index 0 of HistogramArc is a raw expected count
// which becomes the value of the mapped StdArc.
struct ToStdArcMapper {
  typedef HistogramArc FromArc;
  typedef StdArc ToArc;

  ToArc operator()(const FromArc &arc) const {
    return ToArc(arc.ilabel, arc.olabel, arc.weight.Value(0).Value(),
                 arc.nextstate);
  }

  fst::MapFinalAction FinalAction() const {
    return fst::MAP_NO_SUPERFINAL;
  }
  fst::MapSymbolsAction InputSymbolsAction() const {
    return fst::MAP_COPY_SYMBOLS;
  }
  fst::MapSymbolsAction OutputSymbolsAction() const {
    return fst::MAP_COPY_SYMBOLS;
  }
  uint64 Properties(uint64 props) const { return props; }
};

// Mapper (from Arc to HistogramArc)
// The value at index 0 of HistogramArc is the raw exptected count
// (weight value of the original arc). Note that HistogramArc stores
// frequencies of 0,1,2,...K at indices 1,2,3,...,K+1.
// For instance, if expected count is 1.1, then under this mapping
// HistogramArc will contain the following.
// value:  -log(1.1) -log(0) -log(1.1-1) -log(1.1-1) -log(0) ...
// index:         0        1           2           3       4 ...
template <class Arc>
class ToHistogramMapper {
 public:
  typedef Arc FromArc;
  typedef HistogramArc ToArc;

  ToArc operator()(const Arc &arc) const {
    std::vector<TropicalWeight> v(kHistogramBins, TropicalWeight::Zero());
    double val = arc.weight.Value();
    v[0] = val;
    double round_down = floor(exp(-val));
    double round_up = round_down + 1;
    int index = static_cast<int>(round_up);
    if (index < kHistogramBins - 1) {
      v[index + 1] = -log(exp(-val) - round_down);
    }
    if (index && index < kHistogramBins) {
      v[index] = -log(round_up - exp(-val));
    }
    return ToArc(
        arc.ilabel, arc.olabel,
        PowerWeight<TropicalWeight, kHistogramBins>(v.begin(), v.end()),
        arc.nextstate);
  }
  fst::MapFinalAction FinalAction() const {
    return fst::MAP_NO_SUPERFINAL;
  }
  fst::MapSymbolsAction InputSymbolsAction() const {
    return fst::MAP_COPY_SYMBOLS;
  }
  fst::MapSymbolsAction OutputSymbolsAction() const {
    return fst::MAP_COPY_SYMBOLS;
  }
  uint64 Properties(uint64 props) const { return props; }
};

}  // namespace fst

#endif  // NGRAM_HIST_MAPPER_H_
