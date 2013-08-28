// lexicographic-map.h
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
//
// Copyright 2011 Richard Sproat.
// Author: rws@xoba.com (Richard Sproat)
//
// \file
//
// Implements the algorithm for using lexicographic semirings
// discussed in:
//
// Brian Roark, Richard Sproat and Izhak Shafran. 2011. "Lexicographic
// Semirings for Exact Automata Encoding of Sequence Models". ACL-HLT
// 2011, Portland, OR.
//
// The conversion back and forth between the W and the
// Lexicographic<W, W> semiring is handled by a map (fst::Map). Also
// provided is a lightweight class to perform the composition, epsilon
// removal and determinization required by the method.

#ifndef NGRAM_LEXICOGRAPHIC_MAP_H_
#define NGRAM_LEXICOGRAPHIC_MAP_H_

#include <fst/arc.h>
#include <fst/compose.h>
#include <fst/determinize.h>
#include <fst/fst.h>
#include <fst/lexicographic-weight.h>
#include <fst/map.h>
#include <fst/vector-fst.h>
#include <fst/rmepsilon.h>
#include <ngram/ngram-model.h>

namespace ngram {

using fst::kNoStateId;
using fst::Compose;
using fst::Determinize;
using fst::Fst;
using fst::kWeightInvariantProperties;
using fst::LexicographicArc;
using fst::LexicographicWeight;
using fst::Map;
using fst::MapFinalAction;
using fst::MapSymbolsAction;
using fst::MAP_NO_SUPERFINAL;
using fst::MAP_COPY_SYMBOLS;
using fst::MutableFst;
using fst::Power;
using fst::ProjectProperties;
using fst::RmEpsilon;
using fst::StdArc;
using fst::StdVectorFst;
using fst::VectorFst;

// The penalty for the first dimension of the lexicographic weight on
// the phi arc implemented as an epsilon arc. In the most common
// usage, where W=Tropical, this could just be 1 (or any positive
// number). But we make it 2 in case someone uses a semiring where
// Power is really power and not (as in the Tropical),
// multiplication.

static const int32 kBackoffPenalty = 2;

template <class A>
struct ToLexicographicMapper {
  typedef A FromArc;
  typedef typename A::Weight W;

  typedef LexicographicArc<W, W> ToArc;
  typedef typename ToArc::Weight LW;

  explicit ToLexicographicMapper(NGramModel* model) : model_(model) { }

  ToArc operator()(const FromArc &arc) const {
    // 'Super-non-final' arc
    if (arc.nextstate == kNoStateId && arc.weight == W::Zero()) {
      return ToArc(0, 0, LW(W::Zero(), arc.weight), kNoStateId);
    // 'Super-final' arc
    } else if (arc.nextstate == kNoStateId) {
      return ToArc(0, 0, LW(W::One(), arc.weight), kNoStateId);
    // Epsilon label: in this case if it's an LM we need to check the
    // order of the backoff, unless this is Zero(), which can happen
    // in some topologies.
    } else if (arc.ilabel == 0 && arc.olabel == 0 && model_) {
      if (arc.weight == W::Zero())
	return ToArc(arc.ilabel, arc.olabel,
		     LW(arc.weight, arc.weight),
		     arc.nextstate);
      int expt = model_->HiOrder() - model_->StateOrder(arc.nextstate);
      return ToArc(arc.ilabel, arc.olabel,
                   LW(Power<W>(kBackoffPenalty, expt), arc.weight),
                   arc.nextstate);
    // Real arc (called an "ngram" arc in Roark et al. 2011)
    } else {
      return ToArc(arc.ilabel, arc.olabel,
                   LW(W::One(), arc.weight), arc.nextstate);
    }
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

  uint64 Properties(uint64 props) const {
    return ProjectProperties(props, true) & kWeightInvariantProperties;
  }

  NGramModel* model_;
};

template <class A>
struct FromLexicographicMapper {
  typedef A ToArc;
  typedef typename A::Weight W;
  typedef LexicographicArc<W, W> FromArc;
  typedef typename FromArc::Weight LW;

  ToArc operator()(const FromArc &arc) const {
    // 'Super-final' arc and 'Super-non-final' arc
    if (arc.nextstate == kNoStateId)
      return ToArc(0, 0, W(arc.weight.Value2()), kNoStateId);
    else
      return ToArc(arc.ilabel, arc.olabel,
		   W(arc.weight.Value2()), arc.nextstate);
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

  uint64 Properties(uint64 props) const {
    return ProjectProperties(props, true) & kWeightInvariantProperties;
  }
};

template <class A>
class LexicographicRescorer {
 public:
  typedef ToLexicographicMapper<A> ToMapper;
  typedef FromLexicographicMapper<A> FromMapper;

  typedef typename A::Weight W;
  typedef typename ToMapper::ToArc ToArc;

  LexicographicRescorer(MutableFst<A>* lm, NGramModel* model) {
    Map(*lm, &lm_, ToMapper(model));
  }

  ~LexicographicRescorer() { }

  VectorFst<A>* Rescore(MutableFst<A>* lattice);

 private:
  VectorFst<ToArc> lm_;
  VectorFst<A> result_;
};


template <class A>
VectorFst<A>* LexicographicRescorer<A>::Rescore(MutableFst<A>* lattice) {
  VectorFst<ToArc> lexlat;
  Map(*lattice, &lexlat, ToMapper(NULL));
  VectorFst<ToArc> comp;
  Compose(lexlat, lm_, &comp);
  RmEpsilon(&comp);
  VectorFst<ToArc> det;
  Determinize(comp, &det);
  Map(det, &result_, FromMapper());
  return &result_;
}


typedef LexicographicRescorer<StdArc> StdLexicographicRescorer;


}  // namespace ngram

#endif  // NGRAM_LEXICOGRAPHIC_MAP_H_
