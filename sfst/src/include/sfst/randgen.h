
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
// randgen.h
//
// Classes to generate random paths through a stochastic FST.
// The FST must be fully normalized.

#ifndef SFST_RANDGEN_H_
#define SFST_RANDGEN_H_

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include <fst/randgen.h>

namespace sfst {

// Same as FastLogProbArcSelector but handles failure transitions
// labeled with 'phi'. Assumes (but does not check) the input is a
// normalized (see normalize.h) stochastic FST.  Note that
// (non-failure) epsilons are treated as regular symbols where each
// instance behaves as if it is uniquely labeled (i.e., they are not
// constrained by failure transitions).
template <class A>
class SFstArcSelector {
 public:
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  explicit SFstArcSelector(int seed = time(nullptr),
                           Label phi_label = fst::kNoLabel)
      : seed_(seed), phi_label_(phi_label) { srand(seed); }

  // Samples one transition.
  size_t operator()(const fst::Fst<A> &fst, StateId s,
                    double pre_failure_prob, double failure_prob,
                    double numer_prob, ssize_t failure_pos,
                    fst::CacheLogAccumulator<A> *accumulator) const {
    double r = rand()/(RAND_MAX + 1.0);  // NOLINT
    accumulator->SetState(s);
    fst::ArcIterator<fst::Fst<A>> aiter(fst, s);

    if (failure_pos == -1 || r <= pre_failure_prob) {
      // Selects before the failure transition.
      return accumulator->LowerBound(-log(r), &aiter);
    }

    if (r <= pre_failure_prob + numer_prob) {
      // Selects the failure transition.
      return failure_pos;
    }

    // Selects after the failure transition.
    aiter.Seek(failure_pos + 1);
    // Adjusts for the incorrect mass in the cumulative distribution
    // at the failure transition
    r += failure_prob - numer_prob;
    if (r < 0.0) return failure_pos;
    return accumulator->LowerBound(-log(r), &aiter);
  }

  int Seed() const { return seed_; }

  Label PhiLabel() const { return phi_label_; }

 private:
  int seed_;
  Label phi_label_;
  fst::WeightConvert<Weight, fst::LogWeight> to_log_weight_;
};

}  // namespace sfst

namespace fst {

// Specialization for SFstArcSelector.
template <class A>
class ArcSampler<A, sfst::SFstArcSelector<A> > {
 public:
  typedef sfst::SFstArcSelector<A> S;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef typename A::Label Label;
  typedef CacheLogAccumulator<A> C;

  ArcSampler(const Fst<A> &fst, const S &arc_selector,
             int max_length = INT_MAX)
      : fst_(fst),
        arc_selector_(arc_selector),
        max_length_(max_length),
        phi_label_(arc_selector.PhiLabel()),
        accumulator_(new C()),
        matcher_(fst_, MATCH_INPUT) {
    accumulator_->Init(fst);
    rng_.seed(arc_selector.Seed());
  }

  ArcSampler(const ArcSampler<A, S> &sampler, const Fst<A> *fst = nullptr)
      : fst_(fst ? *fst : sampler.fst_),
        arc_selector_(sampler.arc_selector_),
        max_length_(sampler.max_length_),
        phi_label_(sampler.phi_label_),
        matcher_(fst_, MATCH_INPUT) {
    if (fst) {
      accumulator_.reset(new C());
      accumulator_->Init(*fst);
    } else {  // shallow copy
      accumulator_.reset(new C(*sampler.accumulator_));
    }
  }

  bool Sample(const RandState<A> &rstate);

  bool Done() const { return sample_iter_ == sample_map_.end(); }
  void Next() { ++sample_iter_; }
  std::pair<size_t, size_t> Value() const { return *sample_iter_; }
  void Reset() { sample_iter_ = sample_map_.begin(); }

  bool Error() const { return false; }

 private:
  typedef std::mt19937 RNG;

  double TotalProb(StateId s) {
    // Gets cumulative weight at the state.
    ArcIterator< Fst<A> > aiter(fst_, s);
    accumulator_->SetState(s);
    Weight total_weight = accumulator_->Sum(fst_.Final(s), &aiter, 0,
                                            fst_.NumArcs(s));
    return exp(-to_log_weight_(total_weight).Value());
  }

  double PreFailureProb(StateId s, ssize_t failure_pos) {
    // Gets cumulative weight up to (but not including) the
    // failure transition at the state.
    if (failure_pos < 1) return 0.0;
    ArcIterator< Fst<A> > aiter(fst_, s);
    accumulator_->SetState(s);
    Weight cumul_weight = accumulator_->Sum(Weight::Zero(), &aiter, 0,
                                            failure_pos);
    return exp(-to_log_weight_(cumul_weight).Value());
  }

  // Determines if a sample candidate at a given arc position
  // is disallowed because its label was present in a parent state.
  bool ForbiddenPosition(size_t p, const RandState<A> &rstate);

  // Determines if a sample candidate of a given label is disallowed
  // because its label was present in a parent state.
  bool ForbiddenLabel(Label l, const RandState<A> &rstate);

  void MultinomialSample(const RandState<A> &rstate, Weight fail_weight);

  const Fst<A> &fst_;
  const S &arc_selector_;
  int max_length_;
  Label phi_label_;

  // Stores (N, K) as described for Value().
  std::map<size_t, size_t> sample_map_;
  std::map<size_t, size_t>::const_iterator sample_iter_;
  std::unique_ptr<C> accumulator_;

  RNG rng_;
  std::vector<double> pr_;            // multinomial parameters
  std::vector<unsigned int> pos_;     // sample positions
  std::vector<unsigned int> n_;       // sample counts
  WeightConvert<Weight, Log64Weight> to_log_weight_;
  WeightConvert<Log64Weight, Weight> to_weight_;
  std::set<size_t> forbidden_positions_;  // arc pos forbidden fo failure arcs
  std::set<Label> forbidden_labels_;  // labels forbidden for failure arcs
  ExplicitMatcher<SortedMatcher<Fst<A>>> matcher_;
};

template <class A>
bool ArcSampler<A, sfst::SFstArcSelector<A> >::Sample(
  const RandState<A> &rstate) {
  sample_map_.clear();
  forbidden_positions_.clear();
  forbidden_labels_.clear();
  bool is_final = fst_.Final(rstate.state_id) != Weight::Zero();
  size_t narcs_and_final = fst_.NumArcs(rstate.state_id) + is_final;

  if (narcs_and_final == 0 || rstate.length == max_length_) {
    Reset();
    return false;
  }

  double failure_prob = 0.0;   // failure probability
  double total_prob = 1.0;     // total sum of state transitions
  double numer_prob = 0.0;     // numerator in failure probablity
  ssize_t failure_pos = -1;    // arc position of failure transition
  matcher_.SetState(rstate.state_id);

  if (phi_label_ != kNoLabel && matcher_.Find(phi_label_)) {
    const A &arc = matcher_.Value();
    Log64Weight failure_weight = to_log_weight_(arc.weight);
    failure_prob = exp(-failure_weight.Value());
    failure_pos = matcher_.GetMatcher()->Position();
    total_prob = TotalProb(rstate.state_id);
    // total = failure - numer + 1
    numer_prob = 1.0 + failure_prob - total_prob;
  }

  if (fst_.NumArcs(rstate.state_id) + 1 < rstate.nsamples) {
    Weight numer_weight = to_weight_(-log(numer_prob));
    MultinomialSample(rstate, numer_weight);
    Reset();
    return true;
  }

  double pre_failure_prob = PreFailureProb(rstate.state_id, failure_pos);

  for (size_t i = 0; i < rstate.nsamples; ++i) {
    size_t pos = 0;
    do {
      if (forbidden_positions_.size() == narcs_and_final)
        return false;  // non-coaccessible state
      pos = arc_selector_(fst_, rstate.state_id, pre_failure_prob,
                          failure_prob, numer_prob, failure_pos,
                          accumulator_.get());
    } while (ForbiddenPosition(pos, rstate));
    ++sample_map_[pos];
  }
  Reset();
  return true;
}

template <class A>
bool ArcSampler<A, sfst::SFstArcSelector<A> >::ForbiddenPosition(
    size_t pos, const RandState<A> &rstate) {
  if (forbidden_positions_.count(pos) > 0)
    return true;

  Label label = kNoLabel;   // super-final label
  if (pos < fst_.NumArcs(rstate.state_id)) {
    ArcIterator<Fst<A>> aiter(fst_, rstate.state_id);
    aiter.Seek(pos);
    label = aiter.Value().ilabel;
  }
  bool forbidden_label = ForbiddenLabel(label, rstate);
  if (forbidden_label)
    forbidden_positions_.insert(pos);
  return forbidden_label;
}

template <class A>
bool ArcSampler<A, sfst::SFstArcSelector<A> >::ForbiddenLabel(
    Label l, const RandState<A> &rstate) {
  if (phi_label_ == kNoLabel || l == phi_label_ || l == 0)
    return false;

  if (fst_.NumArcs(rstate.state_id) > rstate.nsamples) {
    for (const RandState<A> *rs = &rstate;
         rs->parent != nullptr;
         rs = rs->parent) {
      StateId parent_id = rs->parent->state_id;
      ArcIterator < Fst<A> > aiter(fst_, parent_id);
      aiter.Seek(rs->select);
      if (aiter.Value().ilabel != phi_label_)  // not failure transition
        return false;

      if (l == kNoLabel) {            // super-final label
        if (fst_.Final(parent_id) != Weight::Zero())
          return true;
      } else {
        matcher_.SetState(parent_id);
        if (matcher_.Find(l))
          return true;
      }
    }
    return false;
  } else {
    if (forbidden_labels_.empty()) {
      for (const RandState<A> *rs = &rstate;
           rs->parent != nullptr;
           rs = rs->parent) {
        StateId parent_id = rs->parent->state_id;
        ArcIterator < Fst<A> > aiter(fst_, parent_id);
        aiter.Seek(rs->select);
        if (aiter.Value().ilabel != phi_label_)  // not failure transition
          break;

        for (aiter.Reset(); !aiter.Done(); aiter.Next()) {
          Label l = aiter.Value().ilabel;
          if (l != phi_label_)
            forbidden_labels_.insert(l);
        }

        if (fst_.Final(parent_id) != Weight::Zero())
          forbidden_labels_.insert(kNoLabel);
      }
    }
    return forbidden_labels_.count(l) > 0;
  }
}

template <class A>
void ArcSampler<A, sfst::SFstArcSelector<A> >::MultinomialSample(
    const RandState<A> &rstate, Weight fail_weight) {
  pr_.clear();
  pos_.clear();
  n_.clear();
  size_t pos = 0;
  for (ArcIterator< Fst<A> > aiter(fst_, rstate.state_id);
       !aiter.Done();
       aiter.Next(), ++pos) {
    const A &arc = aiter.Value();
    if (!ForbiddenLabel(arc.ilabel, rstate)) {
      pos_.push_back(pos);
      Weight weight = arc.ilabel == phi_label_ ? fail_weight : arc.weight;
      pr_.push_back(exp(-to_log_weight_(weight).Value()));
    }
  }
  if (fst_.Final(rstate.state_id) != Weight::Zero()
      && !ForbiddenLabel(kNoLabel, rstate)) {
    pos_.push_back(pos);
    pr_.push_back(exp(-to_log_weight_(fst_.Final(rstate.state_id)).Value()));
  }

  if (rstate.nsamples < std::numeric_limits<RNG::result_type>::max()) {
    n_.resize(pr_.size());
    OneMultinomialSample(pr_, rstate.nsamples, &n_, &rng_);
    for (size_t i = 0; i < n_.size(); ++i)
      if (n_[i] != 0) sample_map_[pos_[i]] = n_[i];
  } else {
    for (size_t i = 0; i < pr_.size(); ++i)
      sample_map_[pos_[i]] = ceil(pr_[i] * rstate.nsamples);
  }
}

}  // namespace fst

#endif  // SFST_RANDGEN_H_
