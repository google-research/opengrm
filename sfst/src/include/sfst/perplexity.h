
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
// perplextiy.h
//
// Computes perplexity for a stochastic FST.

#ifndef SFST_PERPLEXITY_H_
#define SFST_PERPLEXITY_H_

#include <stddef.h>

#include <cmath>

#include <fst/log.h>
#include <fst/expectation-weight.h>
#include <sfst/intersect.h>
#include <sfst/normalize.h>
#include <sfst/rmphi.h>
#include <sfst/sfst.h>
#include <sfst/shortest-distance.h>

namespace fst {

// The 'entropy' semiring (p, e) is encoded as (-log p, e)
// over the log and real semirings resp.
using Entropy64Weight = ExpectationWeight<SignedLog64Weight,
                                          Real64Weight>;

// We define these operations to match our encoding.

inline Real64Weight Times(SignedLog64Weight w1, Real64Weight w2) {
  using Limits = fst::FloatLimits<double>;
  if (w1 == SignedLog64Weight::Zero() && w2.Value() == Limits::PosInfinity())
    return Real64Weight::Zero();
  double s1 = w1.Value1().Value();
  double l1 = w1.Value2().Value();
  double p1 = s1 * exp(-l1);
  double e = w2.Value();
  return Real64Weight(p1 * e);
}

inline Real64Weight Times(Real64Weight w1, SignedLog64Weight w2) {
  using Limits = fst::FloatLimits<double>;
  if (w2 == SignedLog64Weight::Zero() && w1.Value() == Limits::PosInfinity())
    return Real64Weight::Zero();
  double e = w1.Value();
  double s2 = w2.Value1().Value();
  double l2 = w2.Value2().Value();
  double p2 = s2 * exp(-l2);
  return Real64Weight(e * p2);
}

// We use this to transform the weights on an SFST into the entropy semiring.
template <class Weight>
class WeightTransform {
 public:
  using SLWeight = SignedLog64Weight;
  using RWeight = Real64Weight;
  using EWeight = Entropy64Weight;

  // How to transform an SFST's weights depends on how it will be used.
  using EntropyType = enum {
    ENTROPY,               // for (self) entropy p in -\sum p log p
    CROSS_ENTROPY_SOURCE,  // for cross entropy p in - \sum p log q
    CROSS_ENTROPY_TARGET   // for cross entropy q in - \sum p log q
  };

  explicit WeightTransform(EntropyType type = ENTROPY) : type_(type) { }

  EWeight operator()(const Weight &w) const {
    if (w == Weight::Zero())
      return EWeight::Zero();

    SLWeight slw = to_sl_(w);
    RWeight rw = slw.Value2().Value();

    switch (type_) {
      case ENTROPY:
        return EWeight(slw, Times(slw, rw));        // (p, -p log p)
      case CROSS_ENTROPY_SOURCE:
        return EWeight(slw, RWeight::Zero());        // (p, 0)
      default:
      case CROSS_ENTROPY_TARGET:
        return EWeight(SLWeight::One(), rw);        // (1, -log q)
    }
  }

 private:
  EntropyType type_;
  fst::WeightConvert<Weight, SLWeight> to_sl_;
};

// We use this to project onto the first component of the entropy weight.
template <>
struct WeightConvert<Entropy64Weight, Log64Weight> {
  using LWeight = Log64Weight;
  using EWeight = Entropy64Weight;

  LWeight operator()(const EWeight &w) const {
    return w.Value1().Value2();
  }
};

// We use this to promote to an entropy weight (p, 0)
template <>
struct WeightConvert<Log64Weight, Entropy64Weight> {
  using LWeight = Log64Weight;
  using SLWeight = SignedLog64Weight;
  using RWeight = Real64Weight;
  using EWeight = Entropy64Weight;

  EWeight operator()(const LWeight &w) const {
    SLWeight slw(1.0, w.Value());
    return EWeight(slw, RWeight::Zero());
  }
};

// We need and have a ring (for ShortestDistance).
inline Entropy64Weight Minus(Entropy64Weight w1, Entropy64Weight w2) {
  const SignedLog64Weight slz = SignedLog64Weight::Zero();
  const Real64Weight rlz = Real64Weight::Zero();
  return Plus(w1, Entropy64Weight(Minus(slz, w2.Value1()),
                                  Minus(rlz, w2.Value2())));
}

// Has a negative component?
inline bool IsNegative(Entropy64Weight w) {
  using SLWeight = fst::SignedLog64Weight;
  using RWeight = fst::Real64Weight;
  return sfst::Less(w.Value1(), SLWeight::Zero()) ||
      sfst::Less(w.Value2(), RWeight::Zero());
}

class Entropy64WeightApproxEqual {
 public:
  using SLWeight = SignedLog64Weight;
  using RWeight = Real64Weight;
  using EWeight = Entropy64Weight;

  explicit Entropy64WeightApproxEqual(float delta)
      : slw_equal_(delta),
        rw_equal_(delta) { }

  bool operator()(const EWeight &w1, const EWeight &w2) const {
    return slw_equal_(w1.Value1(), w2.Value1()) &&
        rw_equal_(w1.Value2(), w2.Value2());
  }

 private:
  sfst::SignedLogWeightApproxEqual<SLWeight> slw_equal_;
  WeightApproxEqual rw_equal_;
};

};  // namespace fst


namespace sfst {

// A float delta for SFST entropy/perplexity algorithms.
constexpr float kEntropyDelta = 1e-8;

// Computes the cross perplexity for a stochastic target FST q given one or
// more source FSTs p. For a single source and target (and L(p) \subseteq L(q))
// the cross entropy between between the two FSTs i H = - \sum p log q
// and the perplexity (per symbol) is e^(H/l) where l is the expect path
// length. When multiple source p's are provided, average values are returned.
// If the target FST is not provided, self perplexity is computed.
// The inputs must all be normalized stochastic FSTs.
template <class Arc>
class Perplexity {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  using SLArc = fst::SignedLog64Arc;
  using RWeight = fst::Real64Weight;
  using SLWeight = fst::SignedLog64Weight;
  using EArc = fst::ExpectationArc<SLArc, RWeight>;
  using EWeight = fst::Entropy64Weight;
  using WEq = fst::Entropy64WeightApproxEqual;
  using WT = fst::WeightTransform<Weight>;
  using WCM = fst::WeightConvertMapper<Arc, EArc, WT>;
  using WCM1 = fst::WeightConvertMapper<EArc, fst::Log64Arc>;

  // For computing cross perplexity.
  explicit Perplexity(const fst::Fst<Arc> &fst,
                      Label phi_label = fst::kNoLabel,
                      Label unknown_label =  fst::kNoLabel,
                      float delta = fst::kDelta,
                      float entropy_delta = kEntropyDelta)
      : phi_label_(phi_label),
        unknown_label_(unknown_label),
        delta_(delta),
        entropy_delta_(entropy_delta),
        error_(false),
        sent_count_(0),
        oov_count_(0) {
    SetTarget(fst);
  }

  // Computes cross perplexity if SetTarget is called.
  // O.w. self perplexity is computed.
  explicit Perplexity(Label phi_label = fst::kNoLabel,
                      Label unknown_label =  fst::kNoLabel,
                      float delta = fst::kDelta,
                      float entropy_delta = kEntropyDelta)
      : phi_label_(phi_label),
        unknown_label_(unknown_label),
        delta_(delta),
        entropy_delta_(entropy_delta),
        error_(false),
        sent_count_(0),
        oov_count_(0) { }


  // Sets target FST for cross-entropy computation. Invokes
  // Reset. Called by the first constructor.
  void SetTarget(const fst::Fst<Arc> &ifst);

  // Applies a source FST to the perplexity computation.
  bool Apply(const  fst::Fst<Arc> &fst);

  // Returns cross/self entropy per source FST.
  double GetEntropy() const {
    double te = GetTotalEntropy();
    double sc = GetSourceCount();
    return te / sc;
  }

  // Returns perplexity per symbol.
  double GetPerplexity() const {
    double te = GetTotalEntropy();
    double sc = GetTotalStateCount();
    return exp(te / sc);
  }

  // Returns total # of source FSTs since construction
  // or last reset.
  size_t NumSources() const { return sent_count_; }

  // Returns count of skipped whole or partial source FSTs.
  // since construction or last reset. Paths can be skipped
  // when L(p) \subsetneq L(q) and the count indicates the
  // probability mass of the source p's that are skipped.
  double SkipCount() const {
    return sent_count_ - GetSourceCount();
  }

  // Returns # of OOVs since construction or last reset (ignoring
  // fully-skipped sources).
  size_t NumOOVs() const { return oov_count_; }

  // Resets perplexity computation.
  void Reset() {
    entropy_.Reset();
    state_count_.Reset();
    sent_count_ = 0;
    oov_count_ = 0;
  }

 private:
  // Finds in-vocabulary set.
  void FindVocabSet(const fst::Fst<Arc> &sfst) {
    namespace f = fst;
    vocab_.clear();

    for (f::StateIterator<f::Fst<Arc>> siter(sfst);
         !siter.Done();
         siter.Next()) {
      StateId s = siter.Value();
      for (f::ArcIterator<f::Fst<Arc>> aiter(sfst, s);
           !aiter.Done();
           aiter.Next()) {
        const Arc &arc = aiter.Value();
        if (arc.ilabel != phi_label_ && arc.ilabel != 0)
          vocab_.insert(arc.ilabel);
      }
    }
  }

  // Converts a source FST to entropy semiring and converts OOVs to
  // unknown_label when defined.
  void PrepareSource(const fst::Fst<Arc> &ifst,
                     fst::MutableFst<EArc> *ofst);

  // Self or cross entropy being computed?
  bool IsSelfEntropy() const {
    namespace f = fst;
    return qfst_.Start() == f::kNoStateId;
  }

  // Returns accumulated cross/self entropy since construction
  // or last reset.
  double GetTotalEntropy() const {
    return entropy_.Sum().Value2().Value();
  }

  // Returns count of partial or whole source FSTs used
  // in the entropy computation since construction or last
  // reset. This can be fractional if L(p) \subsetneq L(q). This
  // is used to normalize the total entropy to a per source FST
  // entropy.
  double GetSourceCount() const {
    double sign_count = entropy_.Sum().Value1().Value1().Value();
    double mag_count = entropy_.Sum().Value1().Value2().Value();
    return sign_count * exp(-mag_count);
  }

  // Returns the accumulated state count mass since construction or last
  // reset. This is equal to the expected length * source count, which
  // is the normalizaton factor needed for perplexity. Note that the expected
  // length of each accepted string includes the super-final label.
  double GetTotalStateCount() const {
    double sign_count = state_count_.Sum().Value1().Value();
    double mag_count = state_count_.Sum().Value2().Value();
    return sign_count * exp(-mag_count);
  }

  Label phi_label_;
  Label unknown_label_;
  float delta_;
  float entropy_delta_;
  bool error_;
  fst::VectorFst<EArc> qfst_;
  fst::Adder<EWeight> entropy_;
  fst::Adder<SLWeight> state_count_;
  size_t sent_count_;
  size_t oov_count_;
  size_t fst_oov_count_;
  std::unordered_set<Label> vocab_;

  Perplexity(const Perplexity &) = delete;
  Perplexity &operator=(const Perplexity &) = delete;
};

template <class Arc>
void Perplexity<Arc>::SetTarget(const fst::Fst<Arc> &ifst) {
  namespace f = fst;

  if (ifst.Start() == f::kNoLabel) {
    LOG(ERROR) << "Perplexity: target FST has no states";
    error_ = true;
    return;
  }

  if (!IsNormalized(ifst, phi_label_, delta_)) {
    LOG(ERROR) << "Perplexity: target is not a normalized stochastic FST";
    error_ = true;
    return;
  }

  WT to_e(WT::CROSS_ENTROPY_TARGET);
  WCM wc_mapper(to_e);
  f::ArcMap(ifst, &qfst_, wc_mapper);

  if (unknown_label_ != f::kNoLabel)
    FindVocabSet(ifst);

  Reset();
}

// Converts FST to entropy semiring and converts OOVs to unknown_label
// when defined.
template <class Arc>
void Perplexity<Arc>::PrepareSource(const fst::Fst<Arc> &ifst,
                                    fst::MutableFst<EArc> *ofst) {
  namespace f = fst;

  if (!IsNormalized(ifst, phi_label_, delta_)) {
    LOG(ERROR) << "Perplexity: source (" << sent_count_
               << ") is not a normalized stochastic FST";
    error_ = true;
    return;
  }

  const auto etype =
      (IsSelfEntropy() ? WT::ENTROPY : WT::CROSS_ENTROPY_SOURCE);

  WT to_e(etype);
  WCM wc_mapper(to_e);
  f::ArcMap(ifst, ofst, wc_mapper);
  fst_oov_count_ = 0;

  if (unknown_label_ == f::kNoLabel || IsSelfEntropy())
    return;

  for (StateId s = 0; s < ofst->NumStates(); ++s) {
    for (f::MutableArcIterator<f::MutableFst<EArc> > aiter(ofst, s);
         !aiter.Done();
         aiter.Next()) {
      EArc arc = aiter.Value();
      if (arc.ilabel != 0) {
        if (arc.ilabel == unknown_label_ || vocab_.count(arc.ilabel) == 0) {
          arc.ilabel = arc.olabel = unknown_label_;
          aiter.SetValue(arc);
          ++fst_oov_count_;
        }
      }
    }
  }
}

template <class Arc>
bool Perplexity<Arc>::Apply(const  fst::Fst<Arc> &fst) {
  namespace f = fst;
  if (error_)
    return false;

  f::VectorFst<EArc> pfst, plogq_fst;
  if (IsSelfEntropy()) {
    PrepareSource(fst, &plogq_fst);
  } else {
    PrepareSource(fst, &pfst);
    Intersect(pfst, qfst_, &plogq_fst, phi_label_, true);
  }

  sent_count_ += 1;
  std::vector<EWeight> distance;
  EWeight entropy =
      ShortestDistance<EArc, EArc, WEq>(plogq_fst, &distance, phi_label_,
                                        false, entropy_delta_);
  if (entropy.Member() && entropy != EWeight::Zero()) {
    entropy_.Add(entropy);
    oov_count_ += fst_oov_count_;
    // This removes the incoming failure mass to a state, which we
    // do not want to count here. This is equivalent to having
    // done the shortest distance computation of the (explicitly)
    // phi-removed automaton.
    DiffStateWeights(plogq_fst, &distance, phi_label_, true);
    for (auto w : distance)
      state_count_.Add(w.Value1());
  }
  return true;
}

}  // namespace sfst

#endif  // SFST_PERPLEXITY_H_
