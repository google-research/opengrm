
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
// sfst.h
//
// See sfst.opengrm.org for documentation of this library for
// stochastic finite-state transducers.
//
// Common definitions for stochastic FSTs

#include <stddef.h>
#include <sys/types.h>
#include <vector>

#include <fst/float-weight.h>
#include <fst/matcher.h>
#include <fst/signed-log-weight.h>

#ifndef SFST_SFST_H_
#define SFST_SFST_H_

namespace sfst {

// Matches ngram library choice
const fst::Log64Weight kApproxZeroWeight = 99.0;

// Order w.r.t. probability: exp(-weight)
inline bool Less(fst::Log64Weight weight1, fst::Log64Weight weight2) {
  return weight1.Value() > weight2.Value();
}

// Order w.r.t. probability: exp(-weight) for the Tropical weight.
inline bool Less(fst::TropicalWeight weight1,
                 fst::TropicalWeight weight2) {
  return weight1.Value() > weight2.Value();
}

inline bool Less(fst::SignedLog64Weight weight1,
                 fst::SignedLog64Weight weight2) {
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

inline bool ApproxZero(fst::Log64Weight weight) {
  constexpr double kTestApproxZero = 99.0;
  return weight.Value() >= kTestApproxZero;
}

inline bool ApproxZero(fst::SignedLog64Weight weight) {
  constexpr double kTestPosApproxZero = 99.0;
  constexpr double kTestNegApproxZero = 10.0;
  if (weight.Value1().Value() > 0.0) {
    return weight.Value2().Value() >= kTestPosApproxZero;
  } else {
    return weight.Value2().Value() >= kTestNegApproxZero;
  }
}

// Assumes Less(weight2, weight1)
inline fst::Log64Weight Minus(fst::Log64Weight weight1,
                                  fst::Log64Weight weight2) {
  // ... or equivalently f1 < f2
  double f1 = weight1.Value();
  double f2 = weight2.Value();
  if (f2 == fst::FloatLimits<double>::PosInfinity()) return f1;
  double d = weight2.Value() - weight1.Value();
  if (d == fst::FloatLimits<double>::PosInfinity()) return f1;
  return f1 - fst::internal::LogNegExp(d);
}

// Class to get information about the failure path leaving a state.
// Assumes (but does not check) input FST has a canonical topology
// (see canonical.h) for a stochastic FST (when match_input = true,
// o.w. fst^-1 is assumed canonical).
template <class Arc>
class FailurePath {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  typedef fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>>
      Matr;

  FailurePath(const fst::Fst<Arc> &fst, Label phi_label, bool match_input)
      : fst_(fst),
        phi_label_(phi_label),
        matcher_(fst,
                 match_input ? fst::MATCH_INPUT : fst::MATCH_OUTPUT),
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
  ssize_t GetFailureArc(StateId s, Arc *arc);

  const fst::Fst<Arc> &fst_;
  Label phi_label_;
  Matr matcher_;

  StateId s_;

  std::vector<StateId> faildest_;   // phi destination states
  std::vector<Weight> failweight_;  // phi weights
  std::vector<size_t> failpos_;     // phi positions

  FailurePath(const FailurePath &) = delete;
  FailurePath &operator=(const FailurePath &) = delete;
};

template <class Arc>
void FailurePath<Arc>::SetState(StateId s) {
  namespace f = fst;
  if (s == s_)  // Already setup
    return;

  s_ = s;
  faildest_.clear();
  failweight_.clear();
  failpos_.clear();
  Arc failarc;
  for (StateId r = s;; r = failarc.nextstate) {
    ssize_t pos = GetFailureArc(r, &failarc);
    if (failarc.nextstate == f::kNoStateId) break;
    faildest_.push_back(failarc.nextstate);
    failweight_.push_back(failarc.weight);
    failpos_.push_back(pos);
  }
}

template <class Arc>
ssize_t FailurePath<Arc>::GetFailureArc(StateId s, Arc *failarc) {
  namespace f = fst;
  *failarc = Arc(f::kNoLabel, f::kNoLabel, Weight::Zero(), f::kNoStateId);
  ssize_t pos = -1;
  if (phi_label_ == f::kNoLabel) return pos;

  matcher_.SetState(s);
  if (matcher_.Find(phi_label_)) {
    *failarc = matcher_.Value();
    pos = matcher_.GetMatcher()->Position();
  }
  return pos;
}

}  // namespace sfst

#endif  // SFST_SFST_H_
