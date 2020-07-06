
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
// normalize.h
//
// Algorithms to normalize stochastic FSTs and to check if one is
// normalized.

#ifndef SFST_NORMALIZE_H_
#define SFST_NORMALIZE_H_

#include <stddef.h>
#include <sys/types.h>

#include <algorithm>
#include <vector>

#include <fst/log.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/push.h>
#include <sfst/backoff.h>
#include <sfst/canonical.h>
#include <sfst/sfst.h>
#include <sfst/shortest-distance.h>
#include <sfst/state-weights.h>
#include <sfst/trim.h>

namespace sfst {

// Computes high-order and (if present) low-order sums at a state.
// Note that (non-failure) epsilons are treated as regular symbols
// where each instance behaves as if it is uniquely labeled (i.e.,
// they are not constrained by failure transitions).  Assumes (but does
// not check) that the input has the canonical topology (see canonical.h).
template <class Arc>
void StateSums(const fst::Fst<Arc> &fst,
               typename Arc::StateId s,
               typename Arc::Label phi_label,
               fst::Log64Weight *high_sum,
               fst::Log64Weight *low_sum,
               fst::Log64Weight *phi_weight,
               ssize_t *phi_position) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  typedef f::ArcIterator<f::Fst<Arc>> ArcItr;
  typedef f::ExplicitMatcher<f::Matcher<f::Fst<Arc>>> Matr;

  f::WeightConvert<Weight, f::Log64Weight> to_log64;
  f::Adder<f::Log64Weight> high_adder(to_log64(fst.Final(s)));
  f::Adder<f::Log64Weight> low_adder;
  *phi_weight = f::Log64Weight::Zero();
  *phi_position = -1;

  FailurePath<Arc> failpath(fst, phi_label, true);
  failpath.SetState(s);

  Weight fail_weight = Weight::One();
  for (size_t i = 0; i < failpath.Length(); ++i) {
    if (i == 0) {
      *phi_weight = to_log64(failpath.GetWeight(i));
      *phi_position = failpath.GetPosition(i);
      if (high_adder.Sum() == f::Log64Weight::Zero()) break;
    } else {
      fail_weight = f::Times(fail_weight, failpath.GetWeight(i));
    }
    Weight final = fst.Final(failpath.GetNextState(i));
    if (final != Weight::Zero()) {
      low_adder.Reset(to_log64(f::Times(fail_weight, final)));
      break;
    }
  }

  Matr matcher(fst, f::MATCH_INPUT);
  Label prev_label = f::kNoLabel;
  for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
    const Arc &high_arc = aiter.Value();
    Label label = high_arc.ilabel;
    if (label != phi_label) {
      f::Log64Weight high_weight = to_log64(high_arc.weight);
      high_adder.Add(high_weight);
      if (label != 0) {
        fail_weight = Weight::One();
        bool matched = label == prev_label;
        for (size_t i = 0; i < failpath.Length() && !matched; ++i) {
          matcher.SetState(failpath.GetNextState(i));
          if (i > 0)
            fail_weight = Times(fail_weight, failpath.GetWeight(i));
          for (matcher.Find(label); !matcher.Done(); matcher.Next()) {
            const Arc &low_arc = matcher.Value();
            f::Log64Weight low_weight =
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
bool IsNormalizedState(const fst::Fst<Arc> &fst,
                       typename Arc::StateId s,
                       typename Arc::Label phi_label,
                       float delta) {
  namespace f = fst;
  f::Log64Weight high_sum, low_sum, phi_weight;
  ssize_t phi_position;
  StateSums(fst, s, phi_label, &high_sum, &low_sum,
            &phi_weight, &phi_position);
  // Checks if high_sum is a proper probability (<=1)
  bool high_sum_le_one = Less(high_sum, f::Log64Weight::One()) ||
      ApproxEqual(high_sum, f::Log64Weight::One(), delta);
  // Checks if high_sum, low_sum, and phi_weight are consistent:
  //  phi_weight = (1 - high_sum)/(1 - low_sum)
  bool phi_norm = f::ApproxEqual(f::Plus(high_sum, phi_weight),
                                 f::Plus(f::Times(phi_weight, low_sum),
                                         f::Log64Weight::One()),
                                 delta);
  bool ret = high_sum_le_one && phi_norm;
  if (!ret) {
    VLOG(1) << "IsNormalized: State not normalized: " << s
            << " high_sum: " << high_sum
            << " low_sum: " << low_sum
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
bool IsNormalized(const fst::Fst<Arc> &fst,
                  typename Arc::Label phi_label = fst::kNoLabel,
                  float delta = fst::kDelta) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef f::StateIterator<f::Fst<Arc>> StateItr;

  if (!IsCanonical(fst, phi_label)) return false;
  for (StateItr siter(fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    if (!IsNormalizedState(fst, s, phi_label, delta))
      return false;
  }
  return true;
}

// Globally normalizes a weighted FST, when possible, as a stochastic
// FST.  This preserves successful path weights up to a global
// constant.  Normalization is possible when the sum of the weight of
// all successful paths from the initial state is finite. Always
// possible in the acyclic case.  Returns true if the operation is
// successful. The 'delta' parameter controls the degree of
// convergence.
template <class Arc>
bool GlobalNormalize(fst::MutableFst<Arc> *fst,
                     typename Arc::Label phi_label = fst::kNoLabel,
                     float delta = fst::kDelta) {
  namespace f = fst;
  typedef typename Arc::Weight Weight;

  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "GlobalNormalize: input is not a canonical stochastic FST";
    return false;
  }
  // Reweights with the weights found above.
  std::vector<Weight> distance;
  const auto total_weight =
      ShortestDistance(*fst, &distance, phi_label, true, delta);
  if (!total_weight.Member())
    return false;
  f::Reweight(fst, distance, f::REWEIGHT_TO_INITIAL);
  f::RemoveWeight(fst, total_weight, false);

  return true;
}

// Locally normalizes a state of a weighted FST, when possible, as a stochastic
// FST.  This rescales out-going arc weights (including super-final
// weight) from each state by a state-dependent constant. Any phi or
// epsilon labels are considered as regular symbols.  Normalization is
// always possible when the sum of the weight of the out-going arcs of
// each state is non-Zero(). Returns true if the operation is
// successful.
template <class Arc>
bool LocalNormalizeState(typename Arc::StateId s,
                         fst::MutableFst<Arc> *fst) {
  namespace f = fst;
  typedef typename Arc::Weight Weight;

  f::WeightConvert<Weight, f::Log64Weight> to_log64;
  f::WeightConvert<f::Log64Weight, Weight> from_log64;
  if (s < 0 || s >= fst->NumStates()) {
    return false;
  } else {
    Weight final = fst->Final(s);

    f::Adder<f::Log64Weight> adder(to_log64(final));
    for (f::ArcIterator<f::MutableFst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      f::Log64Weight weight = to_log64(arc.weight);
      adder.Add(weight);
    }
    if (ApproxZero(adder.Sum())) return false;

    Weight sum = from_log64(adder.Sum());
    if (final != Weight::Zero()) fst->SetFinal(s, Divide(final, sum));
    for (f::MutableArcIterator<f::MutableFst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      arc.weight = Divide(arc.weight, sum);
      aiter.SetValue(arc);
    }
    return true;
  }
}

// Locally normalizes a weighted FST, when possible, as a stochastic
// FST.  This rescales out-going arc weights (including super-final
// weight) from each state by a state-dependent constant. Any phi or
// epsilon labels are considered as regular symbols.  Normalization is
// always possible when the sum of the weight of the out-going arcs of
// each state is non-Zero(). Returns true if the operation is
// successful.
template <class Arc>
bool LocalNormalize(fst::MutableFst<Arc> *fst) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;

  if (!IsCanonical(*fst, f::kNoLabel)) {
    LOG(ERROR) << "LocalNormalize: input is not a canonical stochastic FST";
    return false;
  }

  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (!LocalNormalizeState(s, fst)) return false;
  }
  return true;
}

// Normalizes a state of a weighted FST, when possible, as a stochastic FST by
// computing the appropriate failure transition weights.  The
// non-failure transition weights are assumed correct where possible,
// otherwise they are locally normalized. Returns true if the
// operation is successful.
template <class Arc>
bool PhiNormalizeState(typename Arc::StateId s, fst::MutableFst<Arc> *fst,
                       typename Arc::Label phi_label = fst::kNoLabel) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef f::MutableArcIterator<f::MutableFst<Arc>> MArcItr;
  constexpr float kNormDelta = 1.0e-15;
  f::WeightConvert<f::Log64Weight, Weight> from_log64;
  f::WeightConvert<Weight, f::Log64Weight> to_log64;
  if (s < 0 || s >= fst->NumStates()) {
    return false;
  } else {
    MArcItr aiter(fst, s);
    f::Log64Weight high_sum, low_sum, phi_weight;
    ssize_t phi_position;
    StateSums(*fst, s, phi_label, &high_sum, &low_sum, &phi_weight,
              &phi_position);

    // Only case where high_sum can be zero is if
    // there is a state with only a phi transition.
    if (ApproxZero(high_sum) && (phi_position == -1 || fst->NumArcs(s) != 1)) {
      return false;
    }

    bool low_sum_ge_one =
        Less(f::Log64Weight::One(), low_sum) ||
        ApproxEqual(low_sum, f::Log64Weight::One(), kNormDelta);
    bool high_sum_eq_one =
        ApproxEqual(high_sum, f::Log64Weight::One(), kNormDelta);
    bool high_sum_gt_one = Less(f::Log64Weight::One(), high_sum);

    // Locally normalizes if necessary
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
      high_sum = f::Log64Weight::One();
    }

    if (phi_position != -1) {
      if (high_sum == f::Log64Weight::One()) {
        phi_weight = kApproxZeroWeight;
      } else {
        f::Log64Weight numer = Minus(f::Log64Weight::One(), high_sum);
        f::Log64Weight denom = Minus(f::Log64Weight::One(), low_sum);
        phi_weight = f::Divide(numer, denom);
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
// computing the appropriate failure transition weights.  The
// non-failure transition weights are assumed correct where possible,
// otherwise they are locally normalized. Returns true if the
// operation is successful.
template <class Arc>
bool PhiNormalize(fst::MutableFst<Arc> *fst,
                  typename Arc::Label phi_label = fst::kNoLabel) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  std::vector<StateId> top_order;
  if (phi_label == f::kNoLabel) return true;
  if (!IsCanonical(*fst, phi_label, &top_order)) {
    LOG(ERROR) << "PhiNormalize: input is not a canonical stochastic FST";
    return false;
  }

  for (StateId i = top_order.size() - 1; i >= 0; --i) {
    StateId s = top_order[i];   // ith state in reverse phi-top order
    if (!PhiNormalizeState(s, fst, phi_label)) {
      return false;
    }
  }
  return true;
}

// See CountNormalize() below for an explantion of these values.
enum CountNormType {
  NORM_SUMMED = 0,
  NORM_KL_MIN = 1,
  NORM_MARGINALLY_CONSTRAINED = 1,   // deprecated
  NORM_KL_MIN_APPROXIMATED = 2,
  NORM_MARGINALLY_APPROXIMATED = 2,  // deprecated
};


namespace internal {

// Internal class to normalize a count SFST. See the public interface
// below for algorithm and argument description and for a reference.
// The formula used in comments are w.r.t the reference.
template <class Arc>
class CountNormalizer {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using SLWeight = fst::SignedLog64Weight;
  using ArcItr = fst::ArcIterator<fst::Fst<Arc>>;
  using MArcItr = fst::MutableArcIterator<fst::MutableFst<Arc>>;
  using Matr = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;

  explicit CountNormalizer(Label phi_label, bool trim = false,
                           float delta = kNormDelta,
                           double effective_zero = kEffectiveZero,
                           size_t maxiters = kMaxNormIters)
      : phi_label_(phi_label),
        trim_(trim),
        delta_(delta),
        effective_zero_(1.0, effective_zero),
        maxiters_(maxiters) { }

  // Performs the normalization.
  bool Normalize(CountNormType norm_type, fst::MutableFst<Arc> *fst);

  // Comparison delta for state normalization.
  static const float kNormDelta;
  // Within epsilon of zero.
  static const double kEffectiveZero;
  // Maximum iterations for convergence of normalization.
  static const size_t kMaxNormIters;

 private:
  // Internal state associated with the constrained marginalization of an
  // SFST state.
  struct NormState {
    NormState()
        : count(SLWeight::Zero()),
          fail_count(SLWeight::Zero()),
          denom(SLWeight::Zero()) { }
    SLWeight count;       // sum of outgoing transitions weights from counts:
                          //   C(q)
    SLWeight fail_count;  // failure transition weight from counts:
                          //   C(\varphi, q)
    SLWeight denom;       // denominator of failure arc weight from this state:
                          //   d(q, q') where q \in B_1(q')
    std::vector<StateId> hi_states;  // states with a failure arc to this state
  };

  // Initializes states in the KL minimization.
  void InitStates(const fst::ExpandedFst<Arc> &fst);

  // Iteratively calculates the arc weights at a state in the KL minimization.
  // This uses a DC (difference of convex functions) optimization.
  bool KLMinimizeState(StateId s, CountNormType norm_type,
                       fst::MutableFst<Arc> *fst);

  // Calculates the per arc normalization factor f(x,s,y^n)
  // used to minimize the KL divergence (see NormArcWeights).
  // Returns true on success.
  bool ComputeNormFactor(const fst::ExpandedFst<Arc> &fst,
                         StateId s, std::vector<SLWeight> *norm_factor) const;

  // Normalizes the arc count using the arc normalization factor
  // and the lambda Lagrange multiplier to compute
  // y^{n+1} = C(x, s)/(lambda - f(x, s, y^n))
  // in the minimization of the KL divergence (see LambdaSearch).
  // Returns the sum of these new weights at the state.
  SLWeight NormArcWeights(const fst::ExpandedFst<Arc> &fst,
                          StateId s, CountNormType norm_type,
                          const std::vector<SLWeight> &norm_factor,
                          SLWeight lambda,
                          std::vector<SLWeight> *arc_weights) const;

  // Search for the normalization that makes the arc weights a
  // prob distribution. Does so by a binary search on the lambda argument
  // to NormArcWeights(). Returns true on success.
  bool LambdaSearch(const fst::ExpandedFst<Arc> &fst,
                    StateId s, CountNormType norm_type,
                    const std::vector<SLWeight> &norm_factor,
                    std::vector<SLWeight> *arc_weights) const;

  // Initializes arc weights from counts. Returns true if computed arc weights
  // are solely an initialization; returns false if they are the final answer.
  bool InitArcWeights(const fst::ExpandedFst<Arc> &fst,
                      StateId s, std::vector<SLWeight> *arc_weights) const;

  // Calculates the denominator of the backoff weights given the arc weights.
  // Returns true on success.
  bool ComputeDenom(const fst::ExpandedFst<Arc> &fst, StateId s,
                    const std::vector<SLWeight> &arc_weights);

  // Returns number of arcs at a state including any super-final but
  // excluding any failure arcs.
  size_t NumNonPhiArcs(const fst::Fst<Arc> &fst, StateId s) const {
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

  CountNormalizer(const CountNormalizer &) = delete;
  CountNormalizer &operator=(const CountNormalizer &) = delete;
};

template <class Arc>
const float CountNormalizer<Arc>::kNormDelta = 1.0e-5;

template <class Arc>
const double CountNormalizer<Arc>::kEffectiveZero = 35.0;

template <class Arc>
const size_t CountNormalizer<Arc>::kMaxNormIters = 1000;


template <class Arc>
bool CountNormalizer<Arc>::Normalize(CountNormType norm_type,
                                     fst::MutableFst<Arc> *fst) {
  namespace f = fst;

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
    if (fst->Properties(f::kError, false)) return false;
  }


  if (phi_label_ == f::kNoLabel || norm_type == NORM_SUMMED) {
    if (!LocalNormalize(fst)) {
      LOG(ERROR) << "CountNormalizer: local normalization failed";
      return false;
    }
  } else {  // marginally-constrained case
    InitStates(*fst);

    for (StateId i = 0; i < fst->NumStates(); ++i) {
      StateId s = backoff_->GetPhiTopOrder(i);  // ith state in phi-top order
      if (!KLMinimizeState(s, norm_type, fst)) return false;
    }
  }

  if (phi_label_ != f::kNoLabel && !PhiNormalize(fst, phi_label_)) {
    LOG(ERROR) << "CountNormalizer: phi normalization failed";
    return false;
  }
  return true;
}

template <class Arc>
void CountNormalizer<Arc>::InitStates(
    const fst::ExpandedFst<Arc> &fst) {
  namespace f = fst;
  backoff_.reset(new Backoff<Arc>(fst, phi_label_));

  norm_states_.clear();
  norm_states_.resize(fst.NumStates());
  for (StateId i = fst.NumStates() - 1; i >= 0; --i) {
    const StateId s = backoff_->GetPhiTopOrder(i);
    NormState &state = norm_states_[s];
    f::Adder<SLWeight> adder(to_log_(fst.Final(s)));
    for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc &arc = aiter.Value();
      if (arc.ilabel == phi_label_)
        state.fail_count = to_log_(arc.weight);
      adder.Add(to_log_(arc.weight));
    }
    state.count = adder.Sum();
    const StateId bos = backoff_->GetBackoffState(s);
    if (bos != f::kNoStateId) {
      NormState &bo_state = norm_states_[bos];
      // Records the immediately higher-order state sets.
      bo_state.hi_states.push_back(s);
    }
  }
}

// The computations are done in the signed log rather than the log semiring
// for numerical stability.
template <class Arc>
bool CountNormalizer<Arc>::KLMinimizeState(
    StateId s, CountNormType norm_type, fst::MutableFst<Arc> *fst) {
  namespace f = fst;
  // Stores new arc weights; last position is the super-final weight.
  std::vector<SLWeight> arc_weights(fst->NumArcs(s) + 1, SLWeight::Zero());
  std::vector<SLWeight> prev_arc_weights;
  std::vector<SLWeight> norm_factor(fst->NumArcs(s) + 1, SLWeight::Zero());
  size_t iters = 0;
  if (InitArcWeights(*fst, s, &arc_weights)) {
    if (!ComputeDenom(*fst, s, arc_weights))
      return false;

    do {
      prev_arc_weights = arc_weights;
      if (!ComputeNormFactor(*fst, s, &norm_factor))
        return false;

      if (!LambdaSearch(*fst, s, norm_type, norm_factor, &arc_weights)) {
        LOG(ERROR) << "CountNormalizer: lambda iterations failed";
        return false;
      }
      if (!ComputeDenom(*fst, s, arc_weights))
        return false;
      if (++iters > maxiters_) {
        LOG(WARNING) << "CountNormalizer: max DC iterations exceeded:"
                     << " state: " << s;
        break;  // Allows sub-optimal solution (with warning).
      }
    } while (!ApproxEqualWeights(arc_weights, prev_arc_weights));
  }

  NormWeights(&arc_weights);  // explicitly renormalizes sum to One()

  // Copies arc weights to FST and validates their values.
  ssize_t pos = 0;
  f::Adder<SLWeight> adder;
  for (MArcItr aiter(fst, s); !aiter.Done(); aiter.Next(), ++pos) {
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
    const fst::ExpandedFst<Arc> &fst, StateId s,
    std::vector<SLWeight> *norm_factor) const {
  // We assume any immediately higher NormStates are completed except
  // for the denom members which should have at least tentative values.
  namespace f = fst;

  const NormState &state = norm_states_[s];
  // Per-arc normalization factor.
  fill(norm_factor->begin(), norm_factor->end(), SLWeight::Zero());
  // The higher-order state probabilities w/ backoff.
  for (auto his : state.hi_states) {
    // If all arcs are backed-off, skip.
    if (NumNonPhiArcs(fst, s) == NumNonPhiArcs(fst, his))
      continue;
    const NormState &hi_state = norm_states_[his];
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
    const fst::ExpandedFst<Arc> &fst, StateId s,
    CountNormType norm_type, const std::vector<SLWeight> &norm_factor,
    SLWeight lambda, std::vector<SLWeight> *arc_weights) const {
  namespace f = fst;

  // Normalizes using arc normalization factors.
  size_t pos = 0;
  f::Adder<SLWeight> adder;
  for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next(), ++pos) {
    const Arc &arc = aiter.Value();
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
  // ...including the super-final arc
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
      (*arc_weights)[pos] =  Divide(final_weight, norm);
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
    const fst::ExpandedFst<Arc> &fst,  StateId s,
    CountNormType norm_type, const std::vector<SLWeight> &norm_factor,
    std::vector<SLWeight> *arc_weights) const {
  namespace f = fst;
  // Chooses lambda lower bound as max_x (f(x, s, y^n) + C(x,s))
  // Chooses lambda higher bound as C(s) + max_x f(x, s, y^n)
  ssize_t pos = 0;
  SLWeight maxfx = SLWeight::Zero();
  SLWeight lambda_low = SLWeight::Zero();
  SLWeight lambda_hi = SLWeight::Zero();
  for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next(), ++pos) {
    SLWeight cx = to_log_(aiter.Value().weight);
    SLWeight fx = norm_factor[pos];
    SLWeight cfx = Plus(cx, fx);
    if (Less(maxfx, fx)) maxfx = fx;
    if (Less(lambda_low, cfx)) lambda_low = cfx;
    lambda_hi  = Plus(lambda_hi, cx);
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
                 << " state: " << s
                 << " lambda_low: " << lambda_low
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
                 << " state: " << s
                 << " lambda_low: " << lambda_low
                 << " lambda_hi: " << lambda_hi
                 << " arc_sum: " << arc_sum;
      return false;
    }
  }
}

template <class Arc>
bool CountNormalizer<Arc>::InitArcWeights(
    const fst::ExpandedFst<Arc> &fst,
    StateId s, std::vector<SLWeight> *arc_weights) const {
  namespace f = fst;
  const NormState &state = norm_states_[s];

  // Initializes the initialization.
  fill(arc_weights->begin(), arc_weights->end(), SLWeight::Zero());

  // Finds the number of arcs (including super-final arc)
  ssize_t num_arcs = fst.NumArcs(s);
  if (fst.Final(s) != Weight::Zero())
    ++num_arcs;
  const SLWeight num_arcs_weight =
      to_log_(-std::log(static_cast<double>(num_arcs)));

  if (IsEffectiveZero(state.count)) {
    // Trivial case: (no count mass at the state)
    // y^0 = 1/|L(s)| and return false
    for (size_t pos = 0; pos < num_arcs; ++pos)
      (*arc_weights)[pos] = Divide(SLWeight::One(), num_arcs_weight);
    return false;
  } else {
    // General case:
    //  y^0 = C(x,s)/C(s) (1 - |L(s)| eps) + eps and return true
    const SLWeight arc_mult = Minus(SLWeight::One(),
                                    Times(num_arcs_weight, effective_zero_));
    size_t pos = 0;
    for (ArcItr aiter(fst, s);
       !aiter.Done();
       aiter.Next(), ++pos) {
      const Arc &arc = aiter.Value();
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
    const fst::ExpandedFst<Arc> &fst, StateId s,
    const std::vector<SLWeight> &arc_weights) {
  namespace f = fst;
  NormState &state = norm_states_[s];
  for (auto his : state.hi_states) {
    NormState &hi_state = norm_states_[his];
    f::Adder<SLWeight> adder;
    // Computes d(his,s) = 1 - \sum{x \in L[his]\cap\Sigma} y_x.
    for (size_t hipos = 0; hipos < fst.NumArcs(his); ++hipos) {
      const ssize_t pos = backoff_->GetBackedOffArc(his, hipos);
      if (pos != -1)
        adder.Add(arc_weights[pos]);
    }
    if (fst.Final(his) != Weight::Zero()) {
      const ssize_t pos = fst.NumArcs(s);
      adder.Add(arc_weights[pos]);
    }
    hi_state.denom = Minus(SLWeight::One(), adder.Sum());

    if (Less(hi_state.denom, effective_zero_)) {
      // Computes d(his,s) = \sum{x \notin L[his]\cap\Sigma} y_x.  Equal
      // mathematically to above but possibly different numerically. This
      // ensures the low probability is not due to that on the few states that
      // get here.
      adder.Reset();
      Matr matcher(fst, f::MATCH_INPUT);
      matcher.SetState(his);
      ssize_t pos = 0;
      for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next(), ++pos) {
        const Arc &arc = aiter.Value();
        if (arc.ilabel == phi_label_ || !matcher.Find(arc.ilabel))
          adder.Add(arc_weights[pos]);
      }
      if (fst.Final(his) == Weight::Zero())
        adder.Add(arc_weights[pos]);
      hi_state.denom = adder.Sum();
    }

    if (IsEffectiveZero(hi_state.denom)) {
      // Probability mass at the lower order for arcs at this state is nearly
      // one, so that the denominator in backoff calculation is effectively
      // zero.  This can happen for states with a large number of arcs and
      // nearly the same arcs at each state.
      hi_state.denom = effective_zero_;
    } else if (Less(hi_state.denom, SLWeight::Zero())) {
      LOG(ERROR) << "CountNormalizer: bad backoff denominator: "
                 << hi_state.denom
                 << " state: " << s << " high state: " << his;
      return false;
    }
  }
  return true;
}

}  // namespace internal

// Public interface to algorithm to normalize a count SFST. The input
// should be (smoothed) count SFST e.g., as returned by
// sfst::Count. It should be a 'backoff-complete' SFST (see backoff.h). It
// should not be 'phi-summed' before input (see below).  It is
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
    fst::MutableFst<Arc> *fst,
    typename Arc::Label phi_label,
    CountNormType norm_type = NORM_SUMMED,
    bool trim = false,
    float delta = internal::CountNormalizer<Arc>::kNormDelta,
    double effective_zero = internal::CountNormalizer<Arc>::kEffectiveZero,
    size_t maxiters = internal::CountNormalizer<Arc>::kMaxNormIters) {
  internal::CountNormalizer<Arc> normalizer(phi_label, trim, delta,
                                            effective_zero, maxiters);
  return normalizer.Normalize(norm_type, fst);
}


// Modifies input FST to move it toward a globally normalizable
// stochastic FST. Degree of modification controlled by non-negative
// "delta" with "delta = 0.0" meaning no modification.  Returns true
// if the operation is successful.
template <class Arc>
bool Condition(fst::MutableFst<Arc> *fst,
               typename Arc::Label phi_label = fst::kNoLabel,
               float delta = fst::kDelta) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  if (delta < 0.0) {
    LOG(ERROR) << "Condition: conditioning delta is negative";
    return false;
  }

  f::WeightConvert<f::Log64Weight, Weight> from_log64;
  Weight weight = from_log64(delta);

  // Multiplies the delta on to every non-special arc weight
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    for (f::MutableArcIterator<f::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      // Skips special labels
      if (arc.ilabel == phi_label || arc.ilabel == 0)
        continue;
      arc.weight = Times(arc.weight, weight);
      aiter.SetValue(arc);
    }
  }
  return true;
}

}  // namespace sfst

#endif  // SFST_NORMALIZE_H_
