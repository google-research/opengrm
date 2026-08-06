
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
// rmphi.h
//
// Replaces failure with epsilon transitions, correcting for
// over-acceptance by adding negatively-weighted transitions or by
// subtracting weights from existing transitions and final weights

#ifndef SFST_RMPHI_H_
#define SFST_RMPHI_H_

#include <stddef.h>
#include <sys/types.h>

#include <memory>
#include <vector>

#include <fst/arc-map.h>
#include <fst/arcsort.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/state-map.h>
#include <sfst/sfst.h>

namespace sfst {

namespace internal {

// Replaces failure with epsilon transitions, correcting for
// over-acceptance by adding negatively-weighted transtions or by
// subtracting weights from existing transitions and final weights.
// Weights must be a ring (i.e., have a Minus() defined), e.g.
// SignedLogWeight. The phi label can be an 0 here, but then all
// epsilons will be interpreted as failure of course. (Non-failure)
// epsilons in the input are treated as regular symbols where each
// instance behaves as if it is uniquely labeled (i.e, they are not
// constrained by failure transitions). Assumes (but does not check)
// it has a canonical topology (see canonical.h) for a stochastic FST
// (when match_input = true, o.w. fst^-1 is assumed canonical). The
// rewrite_mode determines if both input and output failure labels are
// replaced. If this mapper is used to directly modify its input, it
// should be applied only to a phi-topsorted input.
template <class Arc>
class RmPhiMapper {
 public:
  typedef Arc FromArc;
  typedef Arc ToArc;

  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  RmPhiMapper(const fst::Fst<Arc> &fst, Label phi, bool match_input = true,
              fst::MatcherRewriteMode rewrite_mode =
              fst::MATCHER_REWRITE_AUTO)
      : fst_(fst.Copy()),
        phi_(phi),
        match_input_(match_input),
        s_(fst::kNoStateId),
        i_(0),
        final_(Weight::Zero()),
        matcher_(*fst_, match_input_ ?
                 fst::MATCH_INPUT : fst::MATCH_OUTPUT),
        failpath_(*fst_, phi, match_input) {
    namespace f = fst;
    if (rewrite_mode == f::MATCHER_REWRITE_AUTO) {
      rewrite_both_ = fst.Properties(f::kAcceptor, true);
    } else if (rewrite_mode == f::MATCHER_REWRITE_ALWAYS) {
      rewrite_both_ = true;
    } else {
      rewrite_both_ = false;
    }
  }

  // Allows updating Fst argument; pass only if changed.
  RmPhiMapper(const RmPhiMapper<Arc> &mapper,
              const fst::Fst<Arc> *fst = 0)
      : fst_(fst ? fst->Copy() : mapper.fst_->Copy()),
        phi_(mapper.phi_),
        match_input_(mapper.match_input_),
        rewrite_both_(mapper.rewrite_both_),
        s_(fst::kNoStateId),
        i_(0),
        final_(Weight::Zero()),
        matcher_(*fst_, match_input_ ?
                 fst::MATCH_INPUT : fst::MATCH_OUTPUT),
      failpath_(*fst_, phi_, match_input_) {}

  StateId Start() { return fst_->Start(); }
  Weight Final(StateId s) const;

  void SetState(StateId s);

  bool Done() const { return i_ >= arcs_.size(); }
  const Arc &Value() const { return arcs_[i_]; }
  void Next() { ++i_; }

  fst::MapSymbolsAction
  InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  fst::MapSymbolsAction
  OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }

  uint64 Properties(uint64 iprops) const {
    namespace f = fst;
    uint64 oprops = iprops & f::kAddArcProperties & f::kSetFinalProperties &
        f::kWeightInvariantProperties;
    if (phi_ != 0 || !rewrite_both_) oprops &= f::kSetArcProperties;
    return oprops;
  }

 private:
  // Constructor arguments
  std::unique_ptr<fst::Fst<Arc>> fst_;
  Label phi_;
  bool match_input_;
  bool rewrite_both_;

  mutable StateId s_;                       // current state

  // Arc and final weight data
  std::vector<Arc> arcs_;                        // current arcs
  ssize_t i_;                               // current arc position
  Weight final_;                            // current final weight

  mutable fst::Matcher< fst::Fst<Arc> > matcher_;
  // Failure path for current state
  mutable FailurePath<Arc> failpath_;
};

template <class Arc>
typename Arc::Weight RmPhiMapper<Arc>::Final(StateId s) const {
  namespace f = fst;

  Weight sfinal = fst_->Final(s);
  if (sfinal == Weight::Zero())
    return sfinal;

  failpath_.SetState(s);
  Weight fail_weight = Weight::One();
  for (size_t i = 0; i < failpath_.Length(); ++i) {
    fail_weight = Times(fail_weight, failpath_.GetWeight(i));
    Weight dfinal = fst_->Final(failpath_.GetNextState(i));
    if (dfinal != Weight::Zero()) {
      // Subtracts corrective weight from final weight
      sfinal = Minus(sfinal, Times(fail_weight, dfinal));
      break;
    }
  }

  return sfinal;
}

template <class Arc>
void RmPhiMapper<Arc>::SetState(StateId s) {
  namespace f = fst;

  failpath_.SetState(s);

  i_ = 0;
  arcs_.clear();
  arcs_.reserve(fst_->NumArcs(s));

  Label prev_label = f::kNoLabel;
  for (f::ArcIterator<f::Fst<Arc>> aiter(*fst_, s);
       !aiter.Done(); aiter.Next()) {
    Arc arc = aiter.Value();
    Label label = match_input_ ? arc.ilabel : arc.olabel;
    if (label == phi_) {  // rewrites failure as epsilon transition
      if (arc.ilabel == phi_ && (rewrite_both_ || match_input_))
        arc.ilabel = 0;
      if (arc.olabel == phi_ && (rewrite_both_ || !match_input_))
        arc.olabel = 0;
    } else if (label != 0) {
      Weight fail_weight = Weight::One();
      // skips any correction if label already processed
      bool matched = label == prev_label;
      for (size_t i = 0; i < failpath_.Length() && !matched; ++i) {
        fail_weight = f::Times(fail_weight, failpath_.GetWeight(i));
        matcher_.SetState(failpath_.GetNextState(i));
        for (matcher_.Find(label); !matcher_.Done(); matcher_.Next()) {
          Arc failarc = matcher_.Value();
          Label faillabel = match_input_ ? failarc.ilabel : failarc.olabel;
          Weight corr_weight = f::Times(fail_weight, failarc.weight);
          Weight comb_weight = Minus(arc.weight, corr_weight);
          if (faillabel == f::kNoLabel) {
            // implicit self-loop
            continue;
          } else if (failarc.nextstate == arc.nextstate &&
                     failarc.ilabel == arc.ilabel &&
                     failarc.olabel == arc.olabel &&
                     !IsNegative(comb_weight) &&
                     comb_weight != Weight::Zero()) {
            // Subtracts corrective weight from arc.
            // Not applied if result is a negative weight
            // since these arcs need to be treated specially
            // by the shortest distance algorithm
            arc.weight = comb_weight;
            matched = true;
            } else {
            // Adds negatively-weighted correcting arc
            failarc.weight = Minus(Weight::Zero(), corr_weight);
            arcs_.push_back(failarc);
            matched = true;
          }
        }
      }
    }
    arcs_.push_back(arc);  // adds input FST transitions
    prev_label = label;
  }
}

// Converts a canonical SFST possibly with phi labels to an equivalent FST
// where the failure transitions are now epsilon transitions. Assumes input has
// no (non-phi) epsilons (or treats such epsilons as if they were regular
// symbols that are uniquely labeled wrt equivalence). The rewrite_mode
// determines if both input and output failure labels are replaced. The
// 'OArc' weight (e.g. for SignedLog64Arc) must have a Minus() operation
// (forming a ring) and a WeightConvert method from 'IArc'.
template <class IArc, class OArc,
          class WC = fst::WeightConvert<typename IArc::Weight,
                                           typename OArc::Weight>>
void RmPhi(const fst::Fst<IArc>& ifst,
           fst::MutableFst<OArc> *ofst,
           typename IArc::Label phi_label = fst::kNoLabel,
           fst::MatcherRewriteMode rewrite_mode =
           fst::MATCHER_REWRITE_AUTO,
           const WC &weight_convert = WC()) {
  namespace f = fst;
  using WCM = f::WeightConvertMapper<IArc, OArc, WC>;

  WCM to_signed_mapper(weight_convert);
  if (phi_label != f::kNoLabel) {
    auto tfst = MakeArcMapFst(ifst, to_signed_mapper);
    RmPhiMapper<OArc> rm_phi_mapper(
        tfst, phi_label, true, rewrite_mode);
    f::StateMap(tfst, ofst, rm_phi_mapper);
    f::ArcSort(ofst, f::ILabelCompare<OArc>());
  } else {
    f::ArcMap(ifst, ofst, to_signed_mapper);
  }
}

}  // namespace internal

}  // namespace sfst

#endif  // SFST_RMPHI_H_
