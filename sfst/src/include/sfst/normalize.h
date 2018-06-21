
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
  if (!PhiShortestDistance(*fst, &distance, phi_label, true, delta))
    return false;

  Weight total_weight =
      f::ComputeTotalWeight(*fst, distance, true);
  f::Reweight(fst, distance, f::REWEIGHT_TO_INITIAL);
  f::RemoveWeight(fst, total_weight, false);

  return true;
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
  typedef typename Arc::Weight Weight;

  if (!IsCanonical(*fst, f::kNoLabel)) {
    LOG(ERROR) << "LocalNormalize: input is not a canonical stochastic FST";
    return false;
  }

  f::WeightConvert<Weight, f::Log64Weight> to_log64;
  f::WeightConvert<f::Log64Weight, Weight> from_log64;

  for (StateId s = 0; s < fst->NumStates(); ++s) {
    Weight final = fst->Final(s);

    f::Adder<f::Log64Weight> adder(to_log64(final));
    for (f::ArcIterator<f::MutableFst<Arc>> aiter(*fst, s);
         !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      f::Log64Weight weight = to_log64(arc.weight);
      adder.Add(weight);
    }
    if (ApproxZero(adder.Sum())) return false;

    Weight sum = from_log64(adder.Sum());
    if (final != Weight::Zero())
      fst->SetFinal(s, Divide(final, sum));
    for (f::MutableArcIterator<f::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      arc.weight = Divide(arc.weight, sum);
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
  typedef f::MutableArcIterator<f::MutableFst<Arc>> MArcItr;
  constexpr float kNormDelta = 1.0e-15;

  std::vector<StateId> top_order;
  if (phi_label == f::kNoLabel) return true;
  if (!IsCanonical(*fst, phi_label, &top_order)) {
    LOG(ERROR) << "PhiNormalize: input is not a canonical stochastic FST";
    return false;
  }

  f::WeightConvert<f::Log64Weight, Weight> from_log64;
  f::WeightConvert<Weight, f::Log64Weight> to_log64;
  for (StateId i = top_order.size() - 1; i >= 0; --i) {
    StateId s = top_order[i];   // ith state in reverse phi-top order
    MArcItr aiter(fst, s);
    f::Log64Weight high_sum, low_sum, phi_weight;
    ssize_t phi_position;
    StateSums(*fst, s, phi_label, &high_sum, &low_sum,
              &phi_weight, &phi_position);

    // Only case where high_sum can be zero is if
    // there is a state with only a phi transition.
    if (ApproxZero(high_sum) && (phi_position == -1 || fst->NumArcs(s) != 1)) {
      return false;
    }

    bool low_sum_ge_one = Less(f::Log64Weight::One(), low_sum) ||
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

// See CountNormalize() below for an explantion of these values.
enum CountNormType {
  NORM_SUMMED = 0,
  NORM_MARGINALLY_CONSTRAINED = 1,
  NORM_MARGINALLY_APPROXIMATED = 2,
};

namespace internal {

constexpr size_t kMaxCNIters = 100;

// Internal class to normalize a count SFST. See the public interface
// below for algorithm and argument description and for a reference.
//
// The algorithm for the marginally-constrained/approximated
// case follows the reference with these differences: (1) the input
// is from the (smoothed) non-phi-summed counts rather a normalized,
// phi-summed model, which more easily generalizes to an SFST. (2)
// the state probabilities are computed directly from state count sums
// rather than explicitly computing the stationary distribution.
template <class Arc>
class CountNormalizer {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using SLWeight = fst::SignedLog64Weight;
  using ArcItr = fst::ArcIterator<fst::Fst<Arc>>;
  using MArcItr = fst::MutableArcIterator<fst::MutableFst<Arc>>;

  CountNormalizer(Label phi_label, float delta,
                  size_t maxiters = kMaxCNIters)
      : phi_label_(phi_label),
        delta_(delta),
        maxiters_(maxiters) { }

  // Performs the normalization.
  bool Normalize(CountNormType norm_type, fst::MutableFst<Arc> *fst);

 private:
  // Internal state associated with the constrained marginalization of an
  // SFST state.
  struct NormState {
    NormState()
        : weight(SLWeight::Zero()),
          weight_ho(SLWeight::Zero()),
          numer(SLWeight::Zero()),
          denom(SLWeight::Zero()) { }

    SLWeight weight;     // state weight excluding higher orders
    SLWeight weight_ho;  // state weight including higher orders w/ backoff
    SLWeight numer;      // numerator of failure arc weight from this state
    SLWeight denom;      // denominator of failure arc weight from this state
    std::vector<StateId> hi_states;  // states with a failure arc to this state
  };

  // Initializes states in the constrainted marginalization.
  void InitStates(const fst::ExpandedFst<Arc> &fst);

  // Iteratively calculates the arc weights at a state.
  bool CalcArcWeights(StateId s, CountNormType norm_type,
                      fst::MutableFst<Arc> *fst);

  // Calculates the state weights w/ backoff.
  void CalcStateWeights(const fst::ExpandedFst<Arc> &fst, StateId s);

  // Calculates the arc weights given backoff weights.
  void ArcFromBackoffWeights(const fst::ExpandedFst<Arc> &fst, StateId s,
                                    std::vector<SLWeight> *arc_weights);

  // Calculates the numerator of the backoff weights given the arc weights.
  void NumerFromArcWeights(StateId s, const std::vector<SLWeight> &arc_weights);

  // Calculates the denominator of the backoff weights given the arc weights.
  void DenomFromArcWeights(const fst::ExpandedFst<Arc> &fst, StateId s,
                           const std::vector<SLWeight> &arc_weights);

  // Ensures arc weights from a prob distribution.
  void NormArcs(StateId s, std::vector<SLWeight> *arc_weights) const;

  Label phi_label_;
  float delta_;
  size_t maxiters_;
  std::unique_ptr<Backoff<Arc>> backoff_;
  std::vector<NormState> norm_states_;
  fst::WeightConvert<SLWeight, Weight> from_log_;
  fst::WeightConvert<Weight, SLWeight> to_log_;

  static const SLWeight kEffectiveZero;

  CountNormalizer(const CountNormalizer &) = delete;
  CountNormalizer &operator=(const CountNormalizer &) = delete;
};

template <class Arc>
const fst::SignedLog64Weight
CountNormalizer<Arc>::kEffectiveZero(1.0, 50.0);

template <class Arc>
bool CountNormalizer<Arc>::Normalize(CountNormType norm_type,
                                     fst::MutableFst<Arc> *fst) {
  namespace f = fst;

  if (norm_type == NORM_SUMMED) {
    // Sums to lower orders.
    if (!SumBackoff(fst, phi_label_)) {
      LOG(ERROR) << "Approx: backoff summation failed";
      return false;
    }

    // Trims the backoff model w.r.t. non-phi weights.  Safe since it
    // is a summed model and thus will keep the backoff topology.
    Trimmer<Arc> trim(fst, phi_label_, TRIM_NEEDED_TRIM);
    trim.WeightTrim(false);
    trim.Finalize();
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
      if (!CalcArcWeights(s, norm_type, fst)) return false;
      CalcStateWeights(*fst, s);
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
  // The state probabilty that excludes higher orders is the sum of
  // the arc counts at the state minus the incoming fail counts.
  for (StateId i = fst.NumStates() - 1; i >= 0; --i) {
    // ith state in reverse phi-top order
    StateId s = backoff_->GetPhiTopOrder(i);
    NormState &state = norm_states_[s];
    state.weight = to_log_(fst.Final(s));
    SLWeight fail_weight = SLWeight::Zero();
    for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc &arc = aiter.Value();
      state.weight = Plus(state.weight, to_log_(arc.weight));
      if (arc.ilabel == phi_label_) fail_weight = to_log_(arc.weight);
    }
    // Initial numerator estimate based on the count FST.
    state.numer = Divide(fail_weight, state.weight);
    StateId bos = backoff_->GetBackoffState(s);
    if (bos != f::kNoStateId) {
      NormState &bo_state = norm_states_[bos];
      // Records the immediately higher-order state sets.
      bo_state.hi_states.push_back(s);
      ArcItr aiter(fst, s);
      aiter.Seek(backoff_->GetBackoffPosition(s));
      const Arc &arc = aiter.Value();
      bo_state.weight = Minus(bo_state.weight, to_log_(arc.weight));
    }
  }

  // Bounds weights.
  for (StateId s = 0; s < fst.NumStates(); ++s) {
    if (Less(norm_states_[s].weight, kEffectiveZero))
      norm_states_[s].weight = kEffectiveZero;
  }
}

// The computations are done in the signed log rather than the log
// semiring. When norm_type == NORM_MARGINALLY_CONSTRAINED the arc
// probabilities are not forced onto the simplex at each
// iteration. Thus, the solution will be exact if it converges
// (within maxiters_). If not, or if a solution is not on the simplex,
// false is returned. If norm_type == NORM_MARGINALLY_APPROXIMATED, the
// arc probabilities are forced onto the simplex through
// re-normalization at each iteration.
template <class Arc>
bool CountNormalizer<Arc>::CalcArcWeights(
    StateId s, CountNormType norm_type, fst::MutableFst<Arc> *fst) {
  namespace f = fst;
  // Finds new arc weights; last position is the super-final weight.
  std::vector<SLWeight> arc_weights(fst->NumArcs(s) + 1, SLWeight::Zero());
  std::vector<SLWeight> prev_arc_weights;
  size_t iters = 0;
  do {
    prev_arc_weights = arc_weights;
    ArcFromBackoffWeights(*fst, s, &arc_weights);
    if (norm_type == NORM_MARGINALLY_APPROXIMATED)
      NormArcs(s, &arc_weights);
    DenomFromArcWeights(*fst, s, arc_weights);
    if (++iters > maxiters_) {
      if (norm_type == NORM_MARGINALLY_APPROXIMATED) {
        break;
      } else {
        LOG(WARNING) << "CountNormalizer: max iterations exceeded";
        return false;
      }
    }
  } while (!ApproxEqualWeights(arc_weights, prev_arc_weights, delta_));
  NumerFromArcWeights(s, arc_weights);
  // Copies arc weights to FST and validates their values.
  ssize_t pos = 0;
  f::Adder<SLWeight> adder;
  bool has_phi = false;
  for (MArcItr aiter(fst, s); !aiter.Done(); aiter.Next(), ++pos) {
    Arc arc = aiter.Value();
    if (arc.ilabel != phi_label_) {
      if (Less(arc_weights[pos], SLWeight::Zero())) {
        LOG(ERROR) << "CountNormalizer: bad arc weight: "
                   << arc_weights[pos];
        return false;
      }
      arc.weight = from_log_(arc_weights[pos]);
      aiter.SetValue(arc);
      adder.Add(arc_weights[pos]);
    } else {
      has_phi = true;
    }
  }
  // ...including the super-final arc.
  if (Less(arc_weights[pos], SLWeight::Zero())) {
    LOG(ERROR) << "CountNormalizer: bad final weight: "
               << arc_weights[pos];
    return false;
  }
  fst->SetFinal(s, from_log_(arc_weights[pos]));
  adder.Add(arc_weights[pos]);

  // Validates total mass.
  if (Less(SLWeight::One(), adder.Sum()) &&
      !ApproxEqual(SLWeight::One(), adder.Sum())) {
    LOG(ERROR) << "CountNormalizer: bad state sum: "
               << adder.Sum();
    return false;
  }
  return true;
}

template <class Arc>
void CountNormalizer<Arc>::CalcStateWeights(
    const fst::ExpandedFst<Arc> &fst, StateId s) {
  namespace f = fst;
  NormState &state = norm_states_[s];
  // The state probability that includes higher orders w/ backoff
  // is computed recursively.
  state.weight_ho = state.weight;
  for (auto his : state.hi_states) {
    NormState &hi_state = norm_states_[his];
    SLWeight fail_weight = Divide(hi_state.numer, hi_state.denom);
    state.weight_ho = Plus(state.weight_ho,
                           Times(hi_state.weight_ho, fail_weight));
  }
}

template <class Arc>
void CountNormalizer<Arc>::ArcFromBackoffWeights(
    const fst::ExpandedFst<Arc> &fst, StateId s,
    std::vector<SLWeight> *arc_weights) {
  // We assume any immediately higher NormStates are completed except
  // for the denom members which should have at least tentative values.
  namespace f = fst;

  CalcStateWeights(fst, s);
  NormState &state = norm_states_[s];

  // Per-arc normalization divisor initialized with state weight that
  // includes higher orders w/ backoff.
  std::vector<SLWeight> norm_arc(arc_weights->size(), state.weight_ho);

  // Subtracts excess high-order backed-off arc weights from norm divisor.
  for (auto his : state.hi_states) {
    NormState &hi_state = norm_states_[his];
    SLWeight fail_weight = Divide(hi_state.numer, hi_state.denom);
    SLWeight excess = Times(hi_state.weight_ho, fail_weight);
    for (size_t hipos = 0; hipos < fst.NumArcs(his); ++hipos) {
      ssize_t pos = backoff_->GetBackedOffArc(his, hipos);
      if (pos != -1) norm_arc[pos] = Minus(norm_arc[pos], excess);
    }
    if (fst.Final(his) != Weight::Zero()) {
      ssize_t pos = fst.NumArcs(s);
      norm_arc[pos] = Minus(norm_arc[pos], excess);
    }
  }

  // Normalizes using above factors.
  size_t pos = 0;
  for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next(), ++pos) {
    const Arc &arc = aiter.Value();
    (*arc_weights)[pos] =  arc.ilabel == phi_label_ ?
        SLWeight::Zero() : Divide(to_log_(arc.weight), norm_arc[pos]);
    if (ApproxZero((*arc_weights)[pos]))
        (*arc_weights)[pos] = kEffectiveZero;
  }
  // ...including the super-final arc.
  if (fst.Final(s) != Weight::Zero()) {
    (*arc_weights)[pos] =  Divide(to_log_(fst.Final(s)), norm_arc[pos]);
    if (ApproxZero((*arc_weights)[pos]))
      (*arc_weights)[pos] = kEffectiveZero;
  }
}

template <class Arc>
void CountNormalizer<Arc>::NumerFromArcWeights(
    StateId s, const std::vector<SLWeight> &arc_weights) {
  namespace f = fst;
  NormState &state = norm_states_[s];
  f::Adder<SLWeight> adder;
  for (const auto &w : arc_weights) adder.Add(w);
  state.numer = Minus(SLWeight::One(), adder.Sum());
  if (ApproxZero(state.numer))
    state.numer = kEffectiveZero;
  // Initializes denominator to the numerator.  This makes the failure
  // weight One() in the initial iteration unlike the opengrm ngram
  // implementation which takes it from the normalized input.
  state.denom = state.numer;
}

template <class Arc>
void CountNormalizer<Arc>::DenomFromArcWeights(
    const fst::ExpandedFst<Arc> &fst, StateId s,
    const std::vector<SLWeight> &arc_weights) {
  namespace f = fst;
  NormState &state = norm_states_[s];
  for (auto his : state.hi_states) {
    f::Adder<SLWeight> adder;
    for (size_t hipos = 0; hipos < fst.NumArcs(his); ++hipos) {
      ssize_t pos = backoff_->GetBackedOffArc(his, hipos);
      if (pos != -1) {
        adder.Add(arc_weights[pos]);
      }
    }
    if (fst.Final(his) != Weight::Zero()) {
      ssize_t pos = fst.NumArcs(s);
      adder.Add(arc_weights[pos]);
    }
    NormState &hi_state = norm_states_[his];
    hi_state.denom = Minus(SLWeight::One(), adder.Sum());
    if (ApproxZero(hi_state.denom))
      hi_state.denom = kEffectiveZero;
  }
}

template <class Arc>
void CountNormalizer<Arc>::NormArcs(StateId s,
                                    std::vector<SLWeight> *arc_weights)
    const {
  namespace f = fst;
  ssize_t failpos = backoff_->GetBackoffPosition(s);

  // Finds current normalization (less backoff weight).
  // and ensures sane arc weights.
  f::Adder<SLWeight> arc_sum;
  for (size_t pos = 0; pos < arc_weights->size(); ++pos) {
    if (pos != failpos) {
      if (!Less(kEffectiveZero, (*arc_weights)[pos])) {
        if (pos != arc_weights->size() - 1 ||
            (*arc_weights)[pos] != SLWeight::Zero()) {
          (*arc_weights)[pos] = kEffectiveZero;
        }
      }
      arc_sum.Add((*arc_weights)[pos]);
    }
  }

  // Renormalizes using 1.0 - numerator estimate.
  SLWeight norm = Minus(SLWeight::One(), norm_states_[s].numer);
  for (size_t pos = 0; pos < arc_weights->size(); ++pos) {
    if (pos != failpos) {
      (*arc_weights)[pos] = Times(Divide((*arc_weights)[pos], arc_sum.Sum()),
                                  norm);
    }
  }
}


}  // namespace internal

// Public interface to algorithm to normalize a count SFST. The input
// should be (smoothed) count SFST e.g., as returned by
// sfst::Count. It should be a 'backoff' SFST (see backoff.h). It
// should not be 'phi-summed' before input (see below).  It is
// transformed into a normalized SFST. Returns true on success.
//
// If norm_type == NORM_SUMMED, the algorithm 'phi-sums' the input (by
// adding higher-order counts to lower counts), locally normalizes and then
// determines the failure weights. In this way, the output at a state
// is marginalized over the higher orders from that state. This default
// choice results in simple reliable count normalization.
//
// If norm_type == NORM_MARGINALLY_CONSTRAINED, the
// marginally-constrained output is computed: i.e., were
// order-marginals taken of the output, they should match the
// ordered_sum=true output. The algorithm in this case is more complex
// being a generization of B. Roark, C. Allauzen and M. Riley,
// "Smoothed marginal distribution constraints for language modeling",
// Proc. ACL 2013. This choice results in a closer approximation to
// the counted distribution (e.g., the input to sfst::Count) but is
// more numerically-sensitive computation that will return false if no
// solution is found.
//
// If norm_type == NORM_MARGINALLY_APPROXIMATED, is similar
// to the NORM_MARGINALLY_CONSTRAINED, but will ensure termination
// by renormalizations at each iteration if necessary. The result
// approximates the NORM_MARGINALLY_CONSTRAINED, but is more reliable.
//
// The 'delta' (convergence threshold) and 'maxiters' (iteration count
// threshold) parameters control the iterative computation used in the
// last two cases.
template <class Arc>
bool CountNormalize(fst::MutableFst<Arc> *fst,
                    typename Arc::Label phi_label,
                    float delta = fst::kDelta,
                    CountNormType norm_type = NORM_SUMMED,
                    size_t maxiters = internal::kMaxCNIters) {
  internal::CountNormalizer<Arc> normalizer(phi_label, delta, maxiters);
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
