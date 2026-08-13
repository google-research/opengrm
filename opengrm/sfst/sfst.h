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

// Common definitions for stochastic FSTs.

#ifndef OPENGRM_SFST_SFST_H_
#define OPENGRM_SFST_SFST_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/signed-log-weight.h"
#include "opengrm/sfst/ngram-context.h"

namespace sfst {

const fst::Log64Weight kApproxZeroWeight(99.0);

constexpr double kNormEps = 0.001;
constexpr double kFloatEps = 0.000001;
constexpr double kInfBackoff = 99.0;
constexpr int64_t kDefaultNGramOrder = 3;

// Threshold where exp(-delta) drops below double machine epsilon
// (std::numeric_limits<double>::digits * M_LN2 ~ 36.04).
constexpr double kMaxNegLogDiffDelta =
    std::numeric_limits<double>::digits * M_LN2;

// Calculates -log(exp(a - b) + 1) for use in high precision NegLogSum.
inline double NegLogDeltaValue(double a, double b, double* c) {
  const double x = std::exp(a - b);
  double delta = -std::log(x + 1);
  if (x < kNormEps) {
    delta = -x;
    for (int j = 2; j <= 4; ++j) delta += std::pow(-x, j) / j;
  }
  if (c) delta -= *c;
  return delta;
}

// Precision method for summing reals and saving negative logs.
inline double NegLogSum(double a, double b, double* c) {
  if (a == fst::StdArc::Weight::Zero().Value()) return b;
  if (b == fst::StdArc::Weight::Zero().Value()) return a;
  if (a > b) return NegLogSum(b, a, c);
  const double delta = NegLogDeltaValue(a, b, c), val = a + delta;
  if (c) *c = (val - a) - delta;
  return val;
}

inline double NegLogSum(double a, double b) { return NegLogSum(a, b, nullptr); }

// Negative log of difference: -log(exp^{-a} - exp^{-b}).
inline double NegLogDiff(double a, double b, bool* error = nullptr) {
  if (b == fst::StdArc::Weight::Zero().Value()) return a;
  if (a >= b) {
    if (a - b >= kNormEps) {
      if (error) *error = true;
    }
    return fst::StdArc::Weight::Zero().Value();
  }
  const double delta = b - a;
  // For delta > -ln(eps), exp(-delta) is below machine epsilon, so
  // -log(exp(-a) - exp(-b)) = a to full precision, and avoids exp() overflow.
  if (delta > kMaxNegLogDiffDelta) {
    return a;
  }
  return b - std::log(std::expm1(delta));
}

// Order w.r.t. probability: exp(-weight)
template <class T>
inline bool Less(fst::LogWeightTpl<T> weight1, fst::LogWeightTpl<T> weight2) {
  return weight1.Value() > weight2.Value();
}

// Order w.r.t. probability: exp(-weight) for the Tropical weight.
template <class T>
inline bool Less(fst::TropicalWeightTpl<T> weight1,
                 fst::TropicalWeightTpl<T> weight2) {
  return weight1.Value() > weight2.Value();
}

// Order w.r.t. probability: weight
template <class T>
inline bool Less(fst::RealWeightTpl<T> weight1, fst::RealWeightTpl<T> weight2) {
  return weight1.Value() < weight2.Value();
}

template <class T>
inline bool Less(fst::SignedLogWeightTpl<T> weight1,
                 fst::SignedLogWeightTpl<T> weight2) {
  bool s1 = weight1.Value1().Value() > 0.0;
  bool s2 = weight2.Value1().Value() > 0.0;
  if (!s1 && s2) {
    return true;
  } else if (s1 && !s2) {
    return false;
  } else if (s1 && s2) {
    return Less(weight1.Value2(), weight2.Value2());
  } else {
    return Less(weight2.Value2(), weight1.Value2());
  }
}

template <class Weight>
bool LessOrEqual(Weight w1, Weight w2) {
  return Less(w1, w2) || w1 == w2;
}

inline bool IsNegative(fst::SignedLog64Weight w) {
  using SLWeight = fst::SignedLog64Weight;
  return Less(w, SLWeight::Zero());
}

inline bool ApproxZero(fst::Log64Weight weight,
                       fst::Log64Weight approx_zero = kApproxZeroWeight) {
  return LessOrEqual(weight, approx_zero);
}

inline bool ApproxZero(
    fst::SignedLog64Weight weight,
    fst::Log64Weight pos_approx_zero = kApproxZeroWeight,
    fst::Log64Weight neg_approx_zero = fst::Log64Weight(10.0)) {
  if (weight.Value1().Value() > 0.0) {
    return LessOrEqual(weight.Value2(), pos_approx_zero);
  } else {
    return LessOrEqual(weight.Value2(), neg_approx_zero);
  }
}

// Safely subtracts two Log64Weights (w1 - w2 in Log domain, i.e. p1 - p2),
// returning kApproxZeroWeight if p1 <= p2 (w1 <= w2 in probability domain)
// or if w1 and w2 are approximately equal within delta.
inline fst::Log64Weight SafeMinus(fst::Log64Weight w1, fst::Log64Weight w2,
                                  float delta = 1.0e-15) {
  if (LessOrEqual(w1, w2) || ApproxEqual(w1, w2, delta)) {
    return kApproxZeroWeight;
  }
  return fst::Minus(w1, w2);
}

// Compares w.r.t. exponentiated values ('probabilities' vs
// '- log probabilities'). Appropriate from SignedLog(64) weights.
template <class Weight>
class SignedLogWeightApproxEqual {
 public:
  explicit SignedLogWeightApproxEqual(float delta) : delta_(delta) {}

  bool operator()(const Weight& w1, const Weight& w2) const {
    double sgn1 = w1.Value1().Value();
    double sgn2 = w2.Value1().Value();
    double val1 = w1.Value2().Value();
    double val2 = w2.Value2().Value();
    double exp1 = sgn1 * std::exp(-val1);
    double exp2 = sgn2 * std::exp(-val2);
    return std::abs(exp1 - exp2) < delta_;
  }

 private:
  const float delta_;
};

// Class to get information about the failure path leaving a state.
// Assumes (but does not check) input FST has a canonical topology
// (see canonical.h) for a stochastic FST (when match_input = true,
// o.w. fst^-1 is assumed canonical).
template <class Arc>
class FailurePath {
 public:
  using Label = typename Arc::Label;
  using Matcher = fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>>;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  FailurePath(const fst::Fst<Arc>& fst, Label phi_label, bool match_input)
      : fst_(fst),
        phi_label_(phi_label),
        matcher_(fst, match_input ? fst::MATCH_INPUT : fst::MATCH_OUTPUT),
        s_(fst::kNoStateId) {}

  // Sets the current state.
  void SetState(StateId s);

  // Length of the failure path from current state.
  // Same as the state order - 1.
  size_t Length() const { return faildest_.size(); }

  // Destination state of the ith transition on the current failure path.
  StateId GetNextState(size_t i) const { return faildest_[i]; }

  // Weight of the ith transition on the current failure path.
  Weight GetWeight(size_t i) const { return failweight_[i]; }

  // Arc position of the ith transition on the current failure path.
  size_t GetPosition(size_t i) const { return failpos_[i]; }

 private:
  // Finds failure arc for a state and returns the arc and arc position.
  // If no failure arc, uses (kNoLabel, kNoLabel, Zero(), kNoStateId)
  // and position -1.
  ssize_t GetFailureArc(StateId s, Arc* arc);

  const fst::Fst<Arc>& fst_;
  Label phi_label_;
  Matcher matcher_;

  StateId s_;

  std::vector<StateId> faildest_;   // phi destination states
  std::vector<Weight> failweight_;  // phi weights
  std::vector<size_t> failpos_;     // phi positions

  FailurePath(const FailurePath&) = delete;
  FailurePath& operator=(const FailurePath&) = delete;
};

template <class Arc>
void FailurePath<Arc>::SetState(StateId s) {
  if (s == s_) return;  // Already set up.
  s_ = s;
  faildest_.clear();
  failweight_.clear();
  failpos_.clear();
  Arc failarc;
  for (StateId r = s;; r = failarc.nextstate) {
    ssize_t pos = GetFailureArc(r, &failarc);
    if (failarc.nextstate == fst::kNoStateId) break;
    faildest_.push_back(failarc.nextstate);
    failweight_.push_back(failarc.weight);
    failpos_.push_back(pos);
  }
}

template <class Arc>
ssize_t FailurePath<Arc>::GetFailureArc(StateId s, Arc* failarc) {
  *failarc = Arc(fst::kNoLabel, fst::kNoLabel, Weight::Zero(), fst::kNoStateId);
  ssize_t pos = -1;
  if (phi_label_ == fst::kNoLabel) return pos;
  matcher_.SetState(s);
  if (matcher_.Find(phi_label_)) {
    *failarc = matcher_.Value();
    pos = matcher_.GetMatcher()->Position();
  }
  return pos;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_SFST_H_
