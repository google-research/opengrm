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

// FST matcher and composition filter that handle failure transitions
// on both components of a composition.

#ifndef OPENGRM_SFST_PHI2MATCHER_H_
#define OPENGRM_SFST_PHI2MATCHER_H_

#include <sys/types.h>

#include <cstdint>

#include "openfst/lib/compose-filter.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"

namespace sfst {

// FST matcher that handles failure transitions on both components of
// a composition. Phi2Matcher is templated itself on a matcher, which
// is used to perform the underlying matching. By default, the
// underlying matcher is constructed by Phi2Matcher. The user can
// instead pass in this object; in that case, Phi2Matcher takes its
// ownership. No non-consuming symbols other than epsilon supported
// with the underlying template argument matcher.
template <class M>
class Phi2Matcher : public fst::MatcherBase<typename M::Arc> {
 public:
  using Arc = typename M::Arc;
  using FST = typename M::FST;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  Phi2Matcher(const FST& fst, fst::MatchType match_type,
              Label phi_label = fst::kNoLabel, bool phi_loop = true,
              fst::MatcherRewriteMode rewrite_mode = fst::MATCHER_REWRITE_AUTO,
              M* matcher = nullptr)
      : matcher_(fst, match_type, phi_label, phi_loop, rewrite_mode, matcher) {
    if (match_type == fst::MATCH_INPUT) {
      phi_arc_ = Arc(fst::kNoLabel, PhiLabel(), Weight::One(), fst::kNoStateId);
    } else {
      phi_arc_ = Arc(PhiLabel(), fst::kNoLabel, Weight::One(), fst::kNoStateId);
    }
  }

  Phi2Matcher(const Phi2Matcher<M>& matcher, bool safe = false)
      : matcher_(matcher.matcher_, safe), phi_arc_(matcher.phi_arc_) {}

  Phi2Matcher<M>* Copy(bool safe = false) const override {
    return new Phi2Matcher<M>(*this, safe);
  }

  fst::MatchType Type(bool test) const override { return matcher_.Type(test); }

  void SetState(StateId s) final {
    phi_arc_.nextstate = s;
    matcher_.SetState(s);
  }

  bool Find(Label match_label) final {
    if (match_label == PhiLabel() && PhiLabel() != fst::kNoLabel &&
        PhiLabel() != 0) {
      // virtual phi loop needs to be returned
      phi_match_ = true;
      phi_match_done_ = false;
      return true;
    } else {
      phi_match_ = false;
      return matcher_.Find(match_label);
    }
  }

  bool Done() const final {
    if (!phi_match_) {
      return matcher_.Done();
    } else {
      return phi_match_done_;
    }
  }

  const Arc& Value() const final {
    if (!phi_match_) {
      return matcher_.Value();
    } else {  // Virtual phi loop
      return phi_arc_;
    }
  }

  void Next() final {
    if (!phi_match_) {
      matcher_.Next();
    } else {
      phi_match_done_ = true;
    }
  }

  Weight Final(StateId s) const final { return matcher_.Final(s); }

  ssize_t Priority(StateId s) final {
    // We use out-degree to determine the priority.
    const FST& fst = GetFst();
    return fst.NumArcs(s);
  }

  const FST& GetFst() const override { return matcher_.GetFst(); }

  uint64_t Properties(uint64_t props) const override {
    auto outprops = matcher_.Properties(props);
    // matcher_.Priority() ensures all phi_labels are removed
    // but we circumvent that above so we adjust the Properties()
    // accordingly.
    if (matcher_.PhiLabel() == 0) {
      if (matcher_.Type(false) == fst::MATCH_INPUT) {
        outprops &= ~(fst::kNoEpsilons | fst::kNoIEpsilons);
      } else if (matcher_.Type(false) == fst::MATCH_OUTPUT) {
        outprops &= ~(fst::kNoEpsilons | fst::kNoOEpsilons);
      }
    }
    return outprops;
  }

  uint32_t Flags() const override {
    // We keep kRequireMatch from the base phi matcher to ensure
    // we are matching on both sides.
    return matcher_.Flags();
  }

  Label PhiLabel() const { return matcher_.PhiLabel(); }

 private:
  fst::PhiMatcher<M> matcher_;
  bool phi_match_;       // Is Find(phi_label) true (w/ phi_label != 0)?
  bool phi_match_done_;
  Arc phi_arc_;          // Loop arc to return when phi_match_ is true.

  Phi2Matcher& operator=(const Phi2Matcher&) = delete;
};

// Companion filter for the phi2 matcher. The first FST must
// be an acceptor and the second must be input-epsilon free (when
// phi_label != 0).
template <class M1, class M2 = M1>
class Phi2Filter {
 public:
  using Arc = typename fst::SequenceComposeFilter<M1, M2>::Arc;
  using Filter = fst::SequenceComposeFilter<M1, M2>;
  using FilterState = typename fst::SequenceComposeFilter<M1, M2>::FilterState;
  using FST1 = typename fst::SequenceComposeFilter<M1, M2>::FST1;
  using FST2 = typename fst::SequenceComposeFilter<M1, M2>::FST2;
  using Label = typename Arc::Label;
  using Matcher1 = typename fst::SequenceComposeFilter<M1, M2>::Matcher1;
  using Matcher2 = typename fst::SequenceComposeFilter<M1, M2>::Matcher2;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  Phi2Filter(const FST1& fst1, const FST2& fst2, Matcher1* matcher1 = nullptr,
             Matcher2* matcher2 = nullptr)
      : filter_(fst1, fst2, matcher1, matcher2), error_(false) {
    Label phi1_label = filter_.GetMatcher1()->PhiLabel();
    Label phi2_label = filter_.GetMatcher2()->PhiLabel();
    fst::MatchType match_type1 = filter_.GetMatcher1()->Type(false);
    fst::MatchType match_type2 = filter_.GetMatcher2()->Type(false);
    if ((phi1_label != fst::kNoLabel || match_type1 != fst::MATCH_NONE) &&
        (phi2_label != fst::kNoLabel || match_type2 != fst::MATCH_NONE) &&
        phi1_label != phi2_label) {
      FSTERROR() << "Phi2Filter : non-trivial matcher phi labels do not match";
      error_ = true;
    }
    if (!fst1.Properties(fst::kAcceptor, true)) {
      FSTERROR() << "Phi2Matcher: input FST1 must be an acceptor";
      error_ = true;
    }
    // Care must be taken when failure transitions and epsilons are
    // mixed. The requirement that the second FST not contain any
    // epsilons together with the use of the underlying sequence
    // filter is one way to ensure correct semantics.
    if (phi2_label != fst::kNoLabel && phi2_label != 0 &&
        fst2.Properties(fst::kIEpsilons, true)) {
      FSTERROR() << "Phi2Matcher: input FST2 must be epsilon-free";
      error_ = true;
    }
  }

  Phi2Filter(const Phi2Filter& filter, bool safe = false)
      : filter_(filter.filter_, safe), error_(filter.error_) {}

  FilterState Start() const { return filter_.Start(); }

  void SetState(StateId s1, StateId s2, const FilterState& f) {
    return filter_.SetState(s1, s2, f);
  }

  FilterState FilterArc(Arc* arc1, Arc* arc2) const {
    Label phi1_label = filter_.GetMatcher1()->PhiLabel();
    Label phi2_label = filter_.GetMatcher2()->PhiLabel();
    if (phi1_label == 0 || phi2_label == 0) {
      // Overrides any epsilon filtering.
      return filter_.Start();
    } else {
      return filter_.FilterArc(arc1, arc2);
    }
  }

  void FilterFinal(Weight* w1, Weight* w2) const {
    return filter_.FilterFinal(w1, w2);
  }

  // Return resp matchers. Ownership stays with filter.

  Matcher1* GetMatcher1() { return filter_.GetMatcher1(); }

  Matcher2* GetMatcher2() { return filter_.GetMatcher2(); }

  uint64_t Properties(uint64_t iprops) const {
    uint64_t oprops = filter_.Properties(iprops);
    if (error_) oprops |= fst::kError;
    return oprops;
  }

 private:
  mutable Filter filter_;  // Mutable for the Getmatcher() call in FilterArc().
  bool error_;

  Phi2Filter& operator=(const Phi2Filter&) = delete;
};

}  // namespace sfst

#endif  // OPENGRM_SFST_PHI2MATCHER_H_
