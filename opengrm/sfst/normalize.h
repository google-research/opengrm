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

// Algorithms to normalize stochastic FSTs and to check if one is
// normalized.

#ifndef OPENGRM_SFST_NORMALIZE_H_
#define OPENGRM_SFST_NORMALIZE_H_

#include <sys/types.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/push.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/sfst.h"
#include "opengrm/sfst/shortest-distance.h"
#include "opengrm/sfst/state-weights.h"
#include "opengrm/sfst/trim.h"

namespace sfst {

// Computes high-order and (if present) low-order sums at a state.
// Note that (non-failure) epsilons are treated as regular symbols
// where each instance behaves as if it is uniquely labeled (i.e.,
// they are not constrained by failure transitions). Assumes (but does
// not check) that the input has the canonical topology (see canonical.h).
template <class Arc>
void StateSums(const fst::Fst<Arc>& fst, typename Arc::StateId s,
               typename Arc::Label phi_label, fst::Log64Weight* high_sum,
               fst::Log64Weight* low_sum, fst::Log64Weight* phi_weight,
               ssize_t* phi_position) {
  using Label = typename Arc::Label;
  using Matcher = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;
  using Weight = typename Arc::Weight;
  fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  fst::Adder<fst::Log64Weight> high_adder(to_log64(fst.Final(s)));
  fst::Adder<fst::Log64Weight> low_adder;
  *phi_weight = fst::Log64Weight::Zero();
  *phi_position = -1;
  FailurePath<Arc> failpath(fst, phi_label, true);
  failpath.SetState(s);
  Weight fail_weight = Weight::One();
  for (size_t i = 0; i < failpath.Length(); ++i) {
    if (i == 0) {
      *phi_weight = to_log64(failpath.GetWeight(i));
      *phi_position = failpath.GetPosition(i);
      if (high_adder.Sum() == fst::Log64Weight::Zero()) break;
    } else {
      fail_weight = fst::Times(fail_weight, failpath.GetWeight(i));
    }
    Weight final = fst.Final(failpath.GetNextState(i));
    if (final != Weight::Zero()) {
      low_adder.Reset(to_log64(fst::Times(fail_weight, final)));
      break;
    }
  }
  Matcher matcher(fst, fst::MATCH_INPUT);
  Label prev_label = fst::kNoLabel;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next()) {
    const Arc& high_arc = aiter.Value();
    Label label = high_arc.ilabel;
    if (label != phi_label) {
      fst::Log64Weight high_weight = to_log64(high_arc.weight);
      high_adder.Add(high_weight);
      if (label != 0) {
        fail_weight = Weight::One();
        bool matched = label == prev_label;
        for (size_t i = 0; i < failpath.Length() && !matched; ++i) {
          matcher.SetState(failpath.GetNextState(i));
          if (i > 0) fail_weight = Times(fail_weight, failpath.GetWeight(i));
          for (matcher.Find(label); !matcher.Done(); matcher.Next()) {
            const Arc& low_arc = matcher.Value();
            fst::Log64Weight low_weight =
                to_log64(Times(fail_weight, low_arc.weight));
            low_adder.Add(low_weight);
            matched = true;
          }
        }
      }
    }
    prev_label = label;
  }
  *high_sum = high_adder.Sum();
  *low_sum = low_adder.Sum();
}

// Tests if a canonical input FST is normalized at state s.
template <class Arc>
bool IsNormalizedState(const fst::Fst<Arc>& fst, typename Arc::StateId s,
                       typename Arc::Label phi_label, float delta) {
  fst::Log64Weight high_sum, low_sum, phi_weight;
  ssize_t phi_position;
  StateSums(fst, s, phi_label, &high_sum, &low_sum, &phi_weight, &phi_position);
  // Checks if high_sum is a proper probability (<= 1).
  bool high_sum_le_one = Less(high_sum, fst::Log64Weight::One()) ||
                         ApproxEqual(high_sum, fst::Log64Weight::One(), delta);
  // Checks if high_sum, low_sum, and phi_weight are consistent:
  //  phi_weight = (1 - high_sum)/(1 - low_sum)
  bool phi_norm = fst::ApproxEqual(
      fst::Plus(high_sum, phi_weight),
      fst::Plus(fst::Times(phi_weight, low_sum), fst::Log64Weight::One()),
      delta);
  bool ret = high_sum_le_one && phi_norm;
  if (!ret) {
    VLOG(1) << "IsNormalized: State not normalized: " << s
            << " high_sum: " << high_sum << " low_sum: " << low_sum
            << " phi_weight: " << phi_weight;
  }
  return ret;
}

// Tests if an input FST has the canonical topology and the weight of
// the transitions (plus any final weight) leaving a state sums to
// Weight::One(). The summation follows failure transitions through
// to an actual transition accumulating the weight. If the input
// is trim (see trim.h), then this will correspond to the condition
// that the weight of the paths into the future sum to Weight::One().
template <class Arc>
bool IsNormalized(const fst::Fst<Arc>& fst,
                  typename Arc::Label phi_label = fst::kNoLabel,
                  float delta = fst::kDelta) {
  using StateId = typename Arc::StateId;
  if (!IsCanonical(fst, phi_label)) return false;
  for (fst::StateIterator<fst::Fst<Arc>> siter(fst); !siter.Done();
       siter.Next()) {
    StateId s = siter.Value();
    if (!IsNormalizedState(fst, s, phi_label, delta)) return false;
  }
  return true;
}

// Globally normalizes a weighted FST, when possible, as a stochastic
// FST. This preserves successful path weights up to a global
// constant. Normalization is possible when the sum of the weight of
// all successful paths from the initial state is finite. Always
// possible in the acyclic case. Returns true if the operation is
// successful. The 'delta' parameter controls the degree of
// convergence.
template <class Arc>
bool GlobalNormalize(fst::MutableFst<Arc>* fst,
                     typename Arc::Label phi_label = fst::kNoLabel,
                     float delta = fst::kDelta) {
  using Weight = typename Arc::Weight;
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "GlobalNormalize: input is not a canonical stochastic FST";
    return false;
  }
  // Reweights with the weights found above.
  std::vector<Weight> distance;
  const auto total_weight =
      ShortestDistance(*fst, &distance, phi_label, true, delta);
  if (!total_weight.Member()) return false;
  fst::Reweight(fst, distance, fst::REWEIGHT_TO_INITIAL);
  fst::RemoveWeight(fst, total_weight, false);
  return true;
}

// Locally normalizes a state of a weighted FST, when possible, as a stochastic
// FST. This rescales out-going arc weights (including super-final
// weight) from each state by a state-dependent constant. Any phi or
// epsilon labels are considered as regular symbols. Normalization is
// always possible when the sum of the weight of the out-going arcs of
// each state is non-Zero(). Returns true if the operation is
// successful.
template <class Arc>
bool LocalNormalizeState(typename Arc::StateId s, fst::MutableFst<Arc>* fst) {
  using Weight = typename Arc::Weight;
  fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  if (s < 0 || s >= fst->NumStates()) {
    return false;
  } else {
    Weight final = fst->Final(s);
    fst::Adder<fst::Log64Weight> adder(to_log64(final));
    for (fst::ArcIterator<fst::MutableFst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      fst::Log64Weight weight = to_log64(arc.weight);
      adder.Add(weight);
    }
    if (ApproxZero(adder.Sum())) return false;
    Weight sum = from_log64(adder.Sum());
    if (final != Weight::Zero()) fst->SetFinal(s, Divide(final, sum));
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      arc.weight = Divide(arc.weight, sum);
      aiter.SetValue(arc);
    }
    return true;
  }
}

// Locally normalizes a weighted FST, when possible, as a stochastic
// FST. This rescales out-going arc weights (including super-final
// weight) from each state by a state-dependent constant. Any phi or
// epsilon labels are considered as regular symbols. Normalization is
// always possible when the sum of the weight of the out-going arcs of
// each state is non-Zero(). Returns true if the operation is
// successful.
template <class Arc>
bool LocalNormalize(fst::MutableFst<Arc>* fst) {
  using StateId = typename Arc::StateId;
  if (!IsCanonical(*fst, fst::kNoLabel)) {
    LOG(ERROR) << "LocalNormalize: input is not a canonical stochastic FST";
    return false;
  }
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (!LocalNormalizeState(s, fst)) return false;
  }
  return true;
}

// Normalizes a state of a weighted FST, when possible, as a stochastic FST by
// computing the appropriate failure transition weights. The
// non-failure transition weights are assumed correct where possible,
// otherwise they are locally normalized. Returns true if the
// operation is successful.
template <class Arc>
bool PhiNormalizeState(typename Arc::StateId s, fst::MutableFst<Arc>* fst,
                       typename Arc::Label phi_label = fst::kNoLabel) {
  using Weight = typename Arc::Weight;
  constexpr float kNormDelta = 1.0e-15;
  fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  if (s < 0 || s >= fst->NumStates()) {
    return false;
  } else {
    fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
    fst::Log64Weight high_sum, low_sum, phi_weight;
    ssize_t phi_position;
    StateSums(*fst, s, phi_label, &high_sum, &low_sum, &phi_weight,
              &phi_position);
    // Only case where high_sum can be zero is if
    // there is a state with only a phi transition.
    if (ApproxZero(high_sum) && (phi_position == -1 || fst->NumArcs(s) != 1)) {
      return false;
    }
    bool low_sum_ge_one =
        Less(fst::Log64Weight::One(), low_sum) ||
        ApproxEqual(low_sum, fst::Log64Weight::One(), kNormDelta);
    bool high_sum_eq_one =
        ApproxEqual(high_sum, fst::Log64Weight::One(), kNormDelta);
    bool high_sum_gt_one = Less(fst::Log64Weight::One(), high_sum);
    // Locally normalizes if necessary.
    if (low_sum_ge_one || high_sum_gt_one ||
        (high_sum_eq_one && phi_position != -1) ||
        (!high_sum_eq_one && phi_position == -1)) {
      for (; !aiter.Done(); aiter.Next()) {
        Arc arc = aiter.Value();
        if (arc.ilabel != phi_label) {
          arc.weight = from_log64(Divide(to_log64(arc.weight), high_sum));
          aiter.SetValue(arc);
        }
      }
      Weight final = fst->Final(s);
      if (final != Weight::Zero()) {
        final = from_log64(Divide(to_log64(final), high_sum));
        fst->SetFinal(s, final);
      }
      high_sum = fst::Log64Weight::One();
    }
    if (phi_position != -1) {
      fst::Log64Weight numer = SafeMinus(fst::Log64Weight::One(), high_sum);
      fst::Log64Weight denom = SafeMinus(fst::Log64Weight::One(), low_sum);
      if (numer == kApproxZeroWeight || denom == kApproxZeroWeight) {
        phi_weight = kApproxZeroWeight;
      } else {
        phi_weight = fst::Divide(numer, denom);
      }
      aiter.Seek(phi_position);
      Arc arc = aiter.Value();
      arc.weight = from_log64(phi_weight);
      aiter.SetValue(arc);
    }
  }
  return true;
}

// Normalizes a weighted FST, when possible, as a stochastic FST by
// computing the appropriate failure transition weights. The
// non-failure transition weights are assumed correct where possible,
// otherwise they are locally normalized. Returns true if the
// operation is successful.
template <class Arc>
bool PhiNormalize(fst::MutableFst<Arc>* fst,
                  typename Arc::Label phi_label = fst::kNoLabel) {
  using StateId = typename Arc::StateId;
  std::vector<StateId> top_order;
  if (phi_label == fst::kNoLabel) return true;
  if (!IsCanonical(*fst, phi_label, &top_order)) {
    LOG(ERROR) << "PhiNormalize: input is not a canonical stochastic FST";
    return false;
  }
  for (StateId i = top_order.size() - 1; i >= 0; --i) {
    StateId s = top_order[i];  // ith state in reverse phi-top order
    if (!PhiNormalizeState(s, fst, phi_label)) {
      return false;
    }
  }
  return true;
}

// Recalculates failure transition weights without rescaling non-failure
// transition weights. Returns true if the operation is successful.
template <class Arc>
bool RecalcBackoffState(typename Arc::StateId s, fst::MutableFst<Arc>* fst,
                        typename Arc::Label phi_label = fst::kNoLabel) {
  using Weight = typename Arc::Weight;
  fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  if (s < 0 || s >= fst->NumStates()) {
    return false;
  } else {
    fst::Log64Weight high_sum, low_sum, phi_weight;
    ssize_t phi_position;
    StateSums(*fst, s, phi_label, &high_sum, &low_sum, &phi_weight,
              &phi_position);
    if (phi_position != -1) {
      fst::Log64Weight numer = SafeMinus(fst::Log64Weight::One(), high_sum);
      fst::Log64Weight denom = SafeMinus(fst::Log64Weight::One(), low_sum);
      if (numer == kApproxZeroWeight || denom == kApproxZeroWeight) {
        phi_weight = kApproxZeroWeight;
      } else {
        phi_weight = fst::Divide(numer, denom);
      }
      fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
      aiter.Seek(phi_position);
      Arc arc = aiter.Value();
      arc.weight = from_log64(phi_weight);
      aiter.SetValue(arc);
    }
  }
  return true;
}

// Recalculates failure transition weights for all states in an FST without
// rescaling non-failure transition weights. Returns true if successful.
template <class Arc>
bool RecalcBackoff(fst::MutableFst<Arc>* fst,
                   typename Arc::Label phi_label = fst::kNoLabel) {
  using StateId = typename Arc::StateId;
  std::vector<StateId> top_order;
  if (phi_label == fst::kNoLabel) return true;
  if (!IsCanonical(*fst, phi_label, &top_order)) {
    LOG(ERROR) << "RecalcBackoff: input is not a canonical stochastic FST";
    return false;
  }
  while (!top_order.empty()) {
    if (!RecalcBackoffState(top_order.back(), fst, phi_label)) {
      return false;
    }
    top_order.pop_back();
  }
  return true;
}

// See CountNormalize() below for an explantion of these values.
enum CountNormType {
  NORM_SUMMED = 0,
  NORM_KL_MIN = 1,
  NORM_MARGINALLY_CONSTRAINED ABSL_DEPRECATED("Use `NORM_KL_MIN` instead.") = 1,
  NORM_KL_MIN_APPROXIMATED = 2,
  NORM_MARGINALLY_APPROXIMATED ABSL_DEPRECATED(
      "Use `NORM_KL_MIN_APPROXIMATED` instead.") = 2,
};

namespace internal {

// Internal class to normalize a count SFST. See the public interface
// below for algorithm and argument description and for a reference.
// The formula used in comments are w.r.t the reference.
template <class Arc>
class CountNormalizer {
 public:
  using Label = typename Arc::Label;
  using Matcher = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;
  using SLWeight = fst::SignedLog64Weight;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  explicit CountNormalizer(Label phi_label, bool trim = false,
                           float delta = kNormDelta,
                           double effective_zero = kEffectiveZero,
                           size_t maxiters = kMaxNormIters)
      : phi_label_(phi_label),
        trim_(trim),
        delta_(delta),
        effective_zero_(SLWeight::W1(1.0), SLWeight::W2(effective_zero)),
        maxiters_(maxiters) {}

  // Performs the normalization.
  bool Normalize(CountNormType norm_type, fst::MutableFst<Arc>* fst);

  // Comparison delta for state normalization.
  static constexpr float kNormDelta = 1.0e-5;
  // Within epsilon of zero.
  static constexpr double kEffectiveZero = 35.0;
  // Maximum iterations for convergence of normalization.
  static constexpr size_t kMaxNormIters = 1000;

 private:
  // Internal state associated with the constrained marginalization of an
  // SFST state.
  struct NormState {
    NormState()
        : count(SLWeight::Zero()),
          fail_count(SLWeight::Zero()),
          denom(SLWeight::Zero()) {}
    SLWeight count;       // Sum of outgoing transitions weights from counts:
                          //   C(q)
    SLWeight fail_count;  // Failure transition weight from counts:
                          //   C(\varphi, q)
    SLWeight denom;       // Denominator of failure arc weight from this state:
                          //   d(q, q') where q \in B_1(q')
    std::vector<StateId> hi_states;  // States with a failure arc to this state.
  };

  // Initializes states in the KL minimization.
  void InitStates(const fst::ExpandedFst<Arc>& fst);

  // Iteratively calculates the arc weights at a state in the KL minimization.
  // This uses a DC (difference of convex functions) optimization.
  bool KLMinimizeState(StateId s, CountNormType norm_type,
                       fst::MutableFst<Arc>* fst);

  // Calculates the per arc normalization factor f(x,s,y^n)
  // used to minimize the KL divergence (see NormArcWeights).
  // Returns true on success.
  bool ComputeNormFactor(const fst::ExpandedFst<Arc>& fst, StateId s,
                         std::vector<SLWeight>* norm_factor) const;

  // Normalizes the arc count using the arc normalization factor
  // and the lambda Lagrange multiplier to compute
  // y^{n+1} = C(x, s)/(lambda - f(x, s, y^n))
  // in the minimization of the KL divergence (see LambdaSearch).
  // Returns the sum of these new weights at the state.
  SLWeight NormArcWeights(const fst::ExpandedFst<Arc>& fst, StateId s,
                          CountNormType norm_type,
                          absl::Span<const SLWeight> norm_factor,
                          SLWeight lambda,
                          std::vector<SLWeight>* arc_weights) const;

  // Search for the normalization that makes the arc weights a
  // prob distribution. Does so by a binary search on the lambda argument
  // to NormArcWeights(). Returns true on success.
  bool LambdaSearch(const fst::ExpandedFst<Arc>& fst, StateId s,
                    CountNormType norm_type,
                    absl::Span<const SLWeight> norm_factor,
                    std::vector<SLWeight>* arc_weights) const;

  // Initializes arc weights from counts. Returns true if computed arc weights
  // are solely an initialization; returns false if they are the final answer.
  bool InitArcWeights(const fst::ExpandedFst<Arc>& fst, StateId s,
                      std::vector<SLWeight>* arc_weights) const;

  // Calculates the denominator of the backoff weights given the arc weights.
  // Returns true on success.
  bool ComputeDenom(const fst::ExpandedFst<Arc>& fst, StateId s,
                    absl::Span<const SLWeight> arc_weights);

  // Returns number of arcs at a state including any super-final but
  // excluding any failure arcs.
  size_t NumNonPhiArcs(const fst::Fst<Arc>& fst, StateId s) const {
    size_t narcs = fst.NumArcs(s);
    if (fst.Final(s) != Weight::Zero()) ++narcs;
    if (backoff_->GetBackoffPosition(s) != -1) --narcs;
    return narcs;
  }

  // Tests within epsilon of zero; ensures approximation is on a closed set.
  bool IsEffectiveZero(SLWeight w) const {
    return ApproxZero(w, effective_zero_.Value2());
  }

  const Label phi_label_;
  bool trim_;
  const float delta_;
  const SLWeight effective_zero_;
  const size_t maxiters_;
  std::unique_ptr<Backoff<Arc>> backoff_;
  std::vector<NormState> norm_states_;
  fst::WeightConvert<SLWeight, Weight> from_log_;
  fst::WeightConvert<Weight, SLWeight> to_log_;

  CountNormalizer(const CountNormalizer&) = delete;
  CountNormalizer& operator=(const CountNormalizer&) = delete;
};

template <class Arc>
bool CountNormalizer<Arc>::Normalize(CountNormType norm_type,
                                     fst::MutableFst<Arc>* fst) {
  if (norm_type == NORM_SUMMED) {
    // Sums to lower orders.
    if (!SumBackoff(fst, phi_label_)) {
      LOG(ERROR) << "CountNormalize: backoff summation failed";
      return false;
    }
  }
  if (trim_) {
    // Trims negligible (non-phi) weight transitions while
    // preserving a backoff-complete input.
    const Weight trim_weight = from_log_(effective_zero_);
    Trimmer<Arc> trim(fst, phi_label_, TRIM_NEEDED_TRIM);
    if (norm_type == NORM_SUMMED) {
      trim.WeightTrim(false, trim_weight);
    } else {
      trim.SumWeightTrim(false, trim_weight);
    }
    trim.Finalize();
    if (fst->Properties(fst::kError, false)) return false;
  }
  if (phi_label_ == fst::kNoLabel || norm_type == NORM_SUMMED) {
    if (!LocalNormalize(fst)) {
      LOG(ERROR) << "CountNormalizer: local normalization failed";
      return false;
    }
  } else {  // Marginally-constrained case.
    InitStates(*fst);
    for (StateId i = 0; i < fst->NumStates(); ++i) {
      StateId s = backoff_->GetPhiTopOrder(i);  // ith state in phi-top order.
      if (!KLMinimizeState(s, norm_type, fst)) return false;
    }
  }
  if (phi_label_ != fst::kNoLabel && !PhiNormalize(fst, phi_label_)) {
    LOG(ERROR) << "CountNormalizer: phi normalization failed";
    return false;
  }
  return true;
}

template <class Arc>
void CountNormalizer<Arc>::InitStates(const fst::ExpandedFst<Arc>& fst) {
  backoff_ = std::make_unique<Backoff<Arc>>(fst, phi_label_);
  norm_states_.clear();
  norm_states_.resize(fst.NumStates());
  for (StateId i = fst.NumStates() - 1; i >= 0; --i) {
    const StateId s = backoff_->GetPhiTopOrder(i);
    NormState& state = norm_states_[s];
    fst::Adder<SLWeight> adder(to_log_(fst.Final(s)));
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      if (arc.ilabel == phi_label_) state.fail_count = to_log_(arc.weight);
      adder.Add(to_log_(arc.weight));
    }
    state.count = adder.Sum();
    const StateId bos = backoff_->GetBackoffState(s);
    if (bos != fst::kNoStateId) {
      NormState& bo_state = norm_states_[bos];
      // Records the immediately higher-order state sets.
      bo_state.hi_states.push_back(s);
    }
  }
}

// The computations are done in the signed log rather than the log semiring
// for numerical stability.
template <class Arc>
bool CountNormalizer<Arc>::KLMinimizeState(StateId s, CountNormType norm_type,
                                           fst::MutableFst<Arc>* fst) {
  // Stores new arc weights; last position is the super-final weight.
  std::vector<SLWeight> arc_weights(fst->NumArcs(s) + 1, SLWeight::Zero());
  std::vector<SLWeight> prev_arc_weights;
  std::vector<SLWeight> norm_factor(fst->NumArcs(s) + 1, SLWeight::Zero());
  size_t iters = 0;
  if (InitArcWeights(*fst, s, &arc_weights)) {
    if (!ComputeDenom(*fst, s, arc_weights)) return false;
    do {
      prev_arc_weights = arc_weights;
      if (!ComputeNormFactor(*fst, s, &norm_factor)) return false;
      if (!LambdaSearch(*fst, s, norm_type, norm_factor, &arc_weights)) {
        LOG(ERROR) << "CountNormalizer: lambda iterations failed";
        return false;
      }
      if (!ComputeDenom(*fst, s, arc_weights)) return false;
      if (++iters > maxiters_) {
        LOG(WARNING) << "CountNormalizer: max DC iterations exceeded:"
                     << " state: " << s;
        break;  // Allows sub-optimal solution (with warning).
      }
    } while (!ApproxEqualWeights(arc_weights, prev_arc_weights));
  }
  NormWeights(&arc_weights);  // Explicitly renormalizes sum to One().
  // Copies arc weights to FST and validates their values.
  ssize_t pos = 0;
  fst::Adder<SLWeight> adder;
  for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
       !aiter.Done(); aiter.Next(), ++pos) {
    Arc arc = aiter.Value();
    if (Less(arc_weights[pos], SLWeight::Zero())) {
      LOG(ERROR) << "CountNormalizer: bad arc weight: " << arc_weights[pos];
      return false;
    }
    arc.weight = from_log_(arc_weights[pos]);
    aiter.SetValue(arc);
    adder.Add(arc_weights[pos]);
  }
  // ...including the super-final arc.
  if (Less(arc_weights[pos], SLWeight::Zero())) {
    LOG(ERROR) << "CountNormalizer: bad final weight: " << arc_weights[pos];
    return false;
  }
  fst->SetFinal(s, from_log_(arc_weights[pos]));
  adder.Add(arc_weights[pos]);
  // Validates total mass.
  if (Less(SLWeight::One(), adder.Sum()) &&
      !ApproxEqual(SLWeight::One(), adder.Sum())) {
    LOG(ERROR) << "CountNormalizer: bad state sum: " << adder.Sum();
    return false;
  }
  return true;
}

template <class Arc>
bool CountNormalizer<Arc>::ComputeNormFactor(
    const fst::ExpandedFst<Arc>& fst, StateId s,
    std::vector<SLWeight>* norm_factor) const {
  // We assume any immediately higher NormStates are completed except
  // for the denom members which should have at least tentative values.
  const NormState& state = norm_states_[s];
  // Per-arc normalization factor.
  std::fill(norm_factor->begin(), norm_factor->end(), SLWeight::Zero());
  // The higher-order state probabilities w/ backoff.
  for (auto his : state.hi_states) {
    // If all arcs are backed-off, skip.
    if (NumNonPhiArcs(fst, s) == NumNonPhiArcs(fst, his)) continue;
    const NormState& hi_state = norm_states_[his];
    // C(\varphi, his) / d(his, s)
    const SLWeight hi_weight = Divide(hi_state.fail_count, hi_state.denom);
    for (size_t hipos = 0; hipos < fst.NumArcs(his); ++hipos) {
      const ssize_t pos = backoff_->GetBackedOffArc(his, hipos);
      // 1_{x \in L[s] \cap \Sigma}
      if (pos != -1) {
        (*norm_factor)[pos] = Plus((*norm_factor)[pos], hi_weight);
      }
    }
    // 1_{$ \in L[s] \cap \Sigma}
    if (fst.Final(his) != Weight::Zero()) {
      const ssize_t pos = fst.NumArcs(s);
      (*norm_factor)[pos] = Plus((*norm_factor)[pos], hi_weight);
    }
  }
  return true;
}

template <class Arc>
fst::SignedLog64Weight CountNormalizer<Arc>::NormArcWeights(
    const fst::ExpandedFst<Arc>& fst, StateId s, CountNormType norm_type,
    absl::Span<const SLWeight> norm_factor, SLWeight lambda,
    std::vector<SLWeight>* arc_weights) const {
  // Normalizes using arc normalization factors.
  size_t pos = 0;
  fst::Adder<SLWeight> adder;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next(), ++pos) {
    const Arc& arc = aiter.Value();
    // If norm type is NORM_KL_MIN_APPROXIMATED, then the failure
    // transition weight is not modified from its initial estimate.
    // C(x, s)
    if (arc.ilabel != phi_label_ || norm_type != NORM_KL_MIN_APPROXIMATED) {
      // \lambda - f(x, q, y^n)
      const SLWeight arc_weight = to_log_(arc.weight);
      if (IsEffectiveZero(arc_weight)) {
        // y^{n+1} = max(y^{n+1}, eps)
        (*arc_weights)[pos] = effective_zero_;
      } else {
        const SLWeight norm = Minus(lambda, norm_factor[pos]);
        // y^{n+1} = C(x, s)/(\lambda - f(x, q, y^n))
        (*arc_weights)[pos] = Divide(arc_weight, norm);
        if (IsEffectiveZero((*arc_weights)[pos])) {
          // y^{n+1} = max(y^{n+1}, eps)
          (*arc_weights)[pos] = effective_zero_;
        }
      }
    }
    adder.Add((*arc_weights)[pos]);
  }
  // ...including the super-final arc.
  if (fst.Final(s) != Weight::Zero()) {
    // C(x, s)
    const SLWeight final_weight = to_log_(fst.Final(s));
    // \lambda - f(x, q, y^n)
    const SLWeight norm = Minus(lambda, norm_factor[pos]);
    if (IsEffectiveZero(final_weight)) {
      // y^{n+1} = max(y^{n+1}, eps)
      (*arc_weights)[pos] = effective_zero_;
    } else {
      // y^{n+1} = C(x, s)/(\lambda - f(x, q, y^n))
      (*arc_weights)[pos] = Divide(final_weight, norm);
      // y^{n+1} = max(y^{n+1}, eps)
      if (IsEffectiveZero((*arc_weights)[pos]))
        (*arc_weights)[pos] = effective_zero_;
    }
    adder.Add((*arc_weights)[pos]);
  }
  return adder.Sum();
}

template <class Arc>
bool CountNormalizer<Arc>::LambdaSearch(
    const fst::ExpandedFst<Arc>& fst, StateId s, CountNormType norm_type,
    absl::Span<const SLWeight> norm_factor,
    std::vector<SLWeight>* arc_weights) const {
  // Chooses lambda lower bound as max_x (f(x, s, y^n) + C(x,s))
  // Chooses lambda higher bound as C(s) + max_x f(x, s, y^n)
  ssize_t pos = 0;
  SLWeight maxfx = SLWeight::Zero();
  SLWeight lambda_low = SLWeight::Zero();
  SLWeight lambda_hi = SLWeight::Zero();
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next(), ++pos) {
    SLWeight cx = to_log_(aiter.Value().weight);
    SLWeight fx = norm_factor[pos];
    SLWeight cfx = Plus(cx, fx);
    if (Less(maxfx, fx)) maxfx = fx;
    if (Less(lambda_low, cfx)) lambda_low = cfx;
    lambda_hi = Plus(lambda_hi, cx);
  }
  const SLWeight final_cx = to_log_(fst.Final(s));
  const SLWeight final_fx = norm_factor[pos];
  const SLWeight final_cfx = Plus(final_fx, final_cx);
  if (Less(maxfx, final_fx)) maxfx = final_fx;
  if (Less(lambda_low, final_cfx)) lambda_low = final_cfx;
  lambda_hi = Plus(Plus(lambda_hi, final_cx), maxfx);
  if (Less(lambda_hi, lambda_low)) {
    if (ApproxEqual(lambda_low, lambda_hi, kNormDelta)) {
      lambda_low = lambda_hi;
    } else {
      LOG(ERROR) << "CountNormalizer: bad lambda parameter limits:"
                 << " state: " << s << " lambda_low: " << lambda_low
                 << " lambda_hi: " << lambda_hi;
      return false;
    }
  }
  const SLWeight two = Plus(SLWeight::One(), SLWeight::One());
  size_t iters = 0;
  while (true) {
    const SLWeight lambda = Divide(Plus(lambda_hi, lambda_low), two);
    const SLWeight arc_sum =
        NormArcWeights(fst, s, norm_type, norm_factor, lambda, arc_weights);
    if (ApproxEqual(arc_sum, SLWeight::One(), delta_) ||
        ApproxEqual(lambda_low, lambda_hi, kNormDelta)) {
      return true;
    } else if (Less(arc_sum, SLWeight::One())) {
      lambda_hi = lambda;
    } else {
      lambda_low = lambda;
    }
    if (++iters > maxiters_) {
      LOG(ERROR) << "CountNormalizer: max (lambda) iterations exceeded:"
                 << " state: " << s << " lambda_low: " << lambda_low
                 << " lambda_hi: " << lambda_hi << " arc_sum: " << arc_sum;
      return false;
    }
  }
}

template <class Arc>
bool CountNormalizer<Arc>::InitArcWeights(
    const fst::ExpandedFst<Arc>& fst, StateId s,
    std::vector<SLWeight>* arc_weights) const {
  const NormState& state = norm_states_[s];
  // Initializes the initialization.
  std::fill(arc_weights->begin(), arc_weights->end(), SLWeight::Zero());
  // Finds the number of arcs (including super-final arc)
  ssize_t num_arcs = fst.NumArcs(s);
  if (fst.Final(s) != Weight::Zero()) ++num_arcs;
  const SLWeight num_arcs_weight(
      to_log_(Weight(-std::log(static_cast<double>(num_arcs)))));
  if (IsEffectiveZero(state.count)) {
    // Trivial case (no count mass at the state):
    // y^0 = 1/|L(s)| and return false.
    for (size_t pos = 0; pos < num_arcs; ++pos)
      (*arc_weights)[pos] = Divide(SLWeight::One(), num_arcs_weight);
    return false;
  } else {
    // General case:
    //  y^0 = C(x,s)/C(s) (1 - |L(s)| eps) + eps and return true.
    const SLWeight arc_mult =
        Minus(SLWeight::One(), Times(num_arcs_weight, effective_zero_));
    size_t pos = 0;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next(), ++pos) {
      const Arc& arc = aiter.Value();
      const SLWeight arc_weight = Divide(to_log_(arc.weight), state.count);
      (*arc_weights)[pos] = Plus(Times(arc_weight, arc_mult), effective_zero_);
    }
    if (fst.Final(s) != Weight::Zero()) {
      const SLWeight final_weight = Divide(to_log_(fst.Final(s)), state.count);
      (*arc_weights)[pos] =
          Plus(Times(final_weight, arc_mult), effective_zero_);
    }
    return true;
  }
}

template <class Arc>
bool CountNormalizer<Arc>::ComputeDenom(
    const fst::ExpandedFst<Arc>& fst, StateId s,
    absl::Span<const SLWeight> arc_weights) {
  NormState& state = norm_states_[s];
  for (auto his : state.hi_states) {
    NormState& hi_state = norm_states_[his];
    fst::Adder<SLWeight> adder;
    // Computes d(his,s) = 1 - \sum{x \in L[his]\cap\Sigma} y_x.
    for (size_t hipos = 0; hipos < fst.NumArcs(his); ++hipos) {
      const ssize_t pos = backoff_->GetBackedOffArc(his, hipos);
      if (pos != -1) adder.Add(arc_weights[pos]);
    }
    if (fst.Final(his) != Weight::Zero()) {
      const ssize_t pos = fst.NumArcs(s);
      adder.Add(arc_weights[pos]);
    }
    hi_state.denom = Minus(SLWeight::One(), adder.Sum());
    if (Less(hi_state.denom, effective_zero_)) {
      // Computes d(his,s) = \sum{x \notin L[his]\cap\Sigma} y_x. Equal
      // mathematically to above but possibly different numerically. This
      // ensures the low probability is not due to that on the few states that
      // get here.
      adder.Reset();
      Matcher matcher(fst, fst::MATCH_INPUT);
      matcher.SetState(his);
      ssize_t pos = 0;
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
           aiter.Next(), ++pos) {
        const Arc& arc = aiter.Value();
        if (arc.ilabel == phi_label_ || !matcher.Find(arc.ilabel))
          adder.Add(arc_weights[pos]);
      }
      if (fst.Final(his) == Weight::Zero()) adder.Add(arc_weights[pos]);
      hi_state.denom = adder.Sum();
    }
    if (IsEffectiveZero(hi_state.denom)) {
      // Probability mass at the lower order for arcs at this state is nearly
      // one, so that the denominator in backoff calculation is effectively
      // zero. This can happen for states with a large number of arcs and
      // nearly the same arcs at each state.
      hi_state.denom = effective_zero_;
    } else if (Less(hi_state.denom, SLWeight::Zero())) {
      LOG(ERROR) << "CountNormalizer: bad backoff denominator: "
                 << hi_state.denom << " state: " << s << " high state: " << his;
      return false;
    }
  }
  return true;
}

}  // namespace internal

// Public interface to algorithm to normalize a count SFST. The input
// should be (smoothed) count SFST e.g., as returned by
// sfst::Count. It should be a 'backoff-complete' SFST (see backoff.h). It
// should not be 'phi-summed' before input (see below). It is
// transformed into a normalized SFST. Returns true on success.
//
// If norm_type == NORM_SUMMED, the algorithm 'phi-sums' the input (by
// adding higher-order counts to lower counts), locally normalizes and then
// determines the failure weights. In this way, the output at a state
// is marginalized over the higher orders from that state. This default
// choice results in simple reliable count normalization.
//
// If norm_type == NORM_KL_MIN, the Kullback–Leibler divergence minimum
// distribution w.r.t. the counts is computed as described in
// Suresh, Roark, Riley, Schogol, "Approximating probabilistic models
// as weighted automata" (experimental/fst/papers/approx/approx.pdf)
//
// If norm_type == NORM_KL_MIN_APPROXIMATED, is similar to the
// NORM_KL_MIN, but may be more numerically stable and can empirically
// give better results. It does so by fixing the failure weight to its
// initial estimate in the above optimization.
//
// The 'delta' (convergence threshold), (-log) 'effective_zero' (underflow)
// and 'maxiters' (iteration count threshold) parameters control
// the iterative computation used in the last two cases.
template <class Arc>
bool CountNormalize(
    fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
    CountNormType norm_type = NORM_SUMMED, bool trim = false,
    float delta = internal::CountNormalizer<Arc>::kNormDelta,
    double effective_zero = internal::CountNormalizer<Arc>::kEffectiveZero,
    size_t maxiters = internal::CountNormalizer<Arc>::kMaxNormIters) {
  internal::CountNormalizer<Arc> normalizer(phi_label, trim, delta,
                                            effective_zero, maxiters);
  return normalizer.Normalize(norm_type, fst);
}

// Modifies input FST to move it toward a globally normalizable
// stochastic FST. Degree of modification controlled by non-negative
// "delta" with "delta = 0.0" meaning no modification. Returns true
// if the operation is successful.
template <class Arc>
bool Condition(fst::MutableFst<Arc>* fst,
               typename Arc::Label phi_label = fst::kNoLabel,
               float delta = fst::kDelta) {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  if (delta < 0.0) {
    LOG(ERROR) << "Condition: conditioning delta is negative";
    return false;
  }
  fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  Weight weight = from_log64(fst::Log64Weight(delta));
  // Multiplies the delta on to every non-special arc weight.
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      // Skips special labels.
      if (arc.ilabel == phi_label || arc.ilabel == 0) continue;
      arc.weight = Times(arc.weight, weight);
      aiter.SetValue(arc);
    }
  }
  return true;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_NORMALIZE_H_
