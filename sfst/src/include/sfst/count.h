
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
// count.h

// Algorithm to count from a stochastic FST w.r.t. a specified FST topology.

#ifndef SFST_COUNT_H_
#define SFST_COUNT_H_

#include <sys/types.h>

#include <vector>

#include <fst/log.h>
#include <fst/arcsort.h>
#include <fst/compose.h>
#include <fst/expanded-fst.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <sfst/canonical.h>
#include <sfst/normalize.h>
#include <sfst/phi2matcher.h>
#include <sfst/rmphi.h>
#include <sfst/sfst.h>
#include <sfst/shortest-distance.h>

namespace sfst {

// Counter class for counting from a stochastic FSA w.r.t a specified FSA
// topology. Computes the counts C(x,q) with x \in L[q] as described in
// Suresh, Roark, Riley, Schogol, "Approximating probabilistic models
// as weighted automata" (experimental/fst/papers/approx/approx.pdf)
template <class Arc>
class Counter {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using SLArc = fst::SignedLog64Arc;
  using SLStateId = fst::SignedLog64Arc::StateId;
  using SLWeight = fst::SignedLog64Weight;

  // The 'phi_label' is the failure label (fst::kNoLabel ->
  // none). The 'delta' parameter controls the degree
  // of algorithm convergence. The topology FST is passed in and the
  // result is returned in 'ofst'. This FST will have counts placed on
  // it. It should be an epsilon-free (if phi_label != 0),
  // deterministic, canonical SFSA (see canonical.h); these
  // requirements are not fully checked.  Any incoming arc weights on
  // the topology FST are removed.
  Counter(Label phi_label, float delta, fst::MutableFst<Arc> *ofst);

  // Provides an FST to be counted; may be called repeatedly.  Assumes
  // (but does not fully check) the input is a canonical stochastic
  // FSA (see canonical.h).  If the input is cyclic, the SFSA should
  // be a normalized (see normalize.h). Also assumes input has no
  // (non-phi) epsilons (or treats such epsilons w.r.t. the failure
  // semantics as if they were regular, uniquely-labeled symbols).
  void Count(const fst::Fst<Arc> &ifst);

  // Terminates counting and finalizes the FST result.
  void Finalize() {
    PhiCounts();
    FinalizeFst();
  }

 private:
  // Initializes topology FST with markup to associated arc counts.
  void InitFst();

  // Adds super-final state and arcs to an FST.
  StateId AddSuperFinal(fst::MutableFst<Arc> *fst, bool *have_phi);

  // Composes input with topology FST.
  void BuildComp(const fst::Fst<Arc> &ifst,
                 fst::MutableFst<SLArc> *ofst) const;


  // Checks if signed-log input is normalized (no failure transitions).
  bool CheckNorm(const fst::Fst<SLArc> &fst) const {
    namespace f = fst;
    using StateItr = f::StateIterator<f::Fst<SLArc>>;
    using ArcItr = f::ArcIterator<f::Fst<SLArc>>;
    for (StateItr siter(fst); !siter.Done(); siter.Next()) {
      StateId s = siter.Value();
      f::Adder<SLWeight> adder(fst.Final(s));
      for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
        const SLArc &arc = aiter.Value();
        adder.Add(arc.weight);
      }
      if (!ApproxEqual(adder.Sum(), SLWeight::One()))
        return false;
    }
    return true;
  }

  // Computes the distances from the initial and to the final
  // state in cfst. If cfst is normalized, 'norm' is true.
  bool ComputeDistances(bool norm, fst::MutableFst<SLArc> *cfst,
                        std::vector<SLWeight> *distance,
                        std::vector<SLWeight> *rdistance) const;

  // Counts arcs in provided (signed log) FST
  void CountArcs(const fst::ExpandedFst<SLArc> &fst,
                 const std::vector<SLWeight> &distance,
                 const std::vector<SLWeight> &rdistance);

  // Fills in failure counts.
  void PhiCounts();

  // Removes markup from result.
  void FinalizeFst();

  Label phi_label_;                   // failure label
  float delta_;                       // delta value used by shortest-distance
  fst::MutableFst<Arc> *ofst_;    // output topology FST with counts
  Label sf_label_;                    // super-final label
  std::vector<SLWeight> arc_counts_;  // maps from arc IDs to arc counts
  std::vector<SLWeight> phi_counts_;  // maps from state IDs to phi counts
  StateId superfinal_;                // ID of super-final state
  bool have_iphi_;                    // phis on input FST
  bool have_ophi_;                    // phis on topology FST
  fst::WeightConvert<SLWeight, Weight> from_sl_;  // convert from SLWeight

  Counter(const Counter &) = delete;
  Counter &operator=(const Counter &) = delete;
};

template <class Arc>
Counter<Arc>::Counter(Label phi_label, float delta,
                      fst::MutableFst<Arc> *ofst)
    : phi_label_(phi_label), delta_(delta), ofst_(ofst), sf_label_(-2) {
  namespace f = fst;

  if (ofst_->Start() == f::kNoStateId) {
    FSTERROR() << "Counter: topology FST has no states";
    ofst_->SetProperties(f::kError, f::kError);
    return;
  }
  uint64 props = f::kAcceptor | f::kIDeterministic | f::kILabelSorted;
  if (phi_label != 0) props |= f::kNoEpsilons;
  if (ofst_->Properties(props, true) != props) {
    FSTERROR() << "Counter: topology FST not an epsilon-free"
               << " deterministic canonical SFSA";
    ofst_->SetProperties(f::kError, f::kError);
    return;
  }

  InitFst();
}

template <class Arc>
void Counter<Arc>::InitFst() {
  namespace f = fst;
  superfinal_ = AddSuperFinal(ofst_, &have_ophi_);

  for (StateId s = 0; s < ofst_->NumStates(); ++s) {
    for (f::MutableArcIterator<f::MutableFst<Arc>> aiter(ofst_, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      arc.weight = Weight::One();  // Removes any arc weight
      if (arc.ilabel != phi_label_) {
        ssize_t arc_id = arc_counts_.size();
        // Output label stores the arc ID during the construction.
        arc.olabel = arc_id;

        if (arc.olabel != arc_id) {
          FSTERROR() << "Counter: arc label size needs to be large"
                     << " enough to store arc IDs in construction";
          ofst_->SetProperties(f::kError, f::kError);
          return;
        }
        arc_counts_.push_back(SLWeight::Zero());
      }
      aiter.SetValue(arc);
    }
  }
}

template <class Arc>
typename Arc::StateId Counter<Arc>::AddSuperFinal(
    fst::MutableFst<Arc> *fst,  bool *have_phi) {
  namespace f = fst;
  *have_phi = false;

  // Checks for failure and superfinal labels.
  // Makes all final states point to super-final state
  StateId sf_state = fst->AddState();
  fst->SetFinal(sf_state, Weight::One());
  for (StateId s = 0; s < fst->NumStates() - 1; ++s) {
    for (f::ArcIterator<f::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      if (arc.ilabel == sf_label_) {
        FSTERROR() << "Counter: label " << sf_label_
                   << " reserved for the superfinal label";
        ofst_->SetProperties(f::kError, f::kError);
        return f::kNoStateId;
      }
      if (arc.ilabel == phi_label_) *have_phi = true;
    }

    if (fst->Final(s) != Weight::Zero()) {
      fst->AddArc(s, Arc(sf_label_, sf_label_, fst->Final(s), sf_state));
      fst->SetFinal(s, Weight::Zero());
    }
  }
  f::ArcSort(fst, f::ILabelCompare<Arc>());
  return sf_state;
}

template <class Arc>
void Counter<Arc>::Count(const fst::Fst<Arc> &ifst) {
  namespace f = fst;
  if (ifst.Start() == f::kNoStateId) {
    FSTERROR() << "Counter: input FST has no states";
    ofst_->SetProperties(f::kError, f::kError);
    return;
  }
  if (!ifst.Properties(f::kAcceptor, true)) {
    FSTERROR() << "Counter: input FST not an acceptor";
    ofst_->SetProperties(f::kError, f::kError);
    return;
  }

  f::VectorFst<SLArc> cfst;
  {
    // Adds super-final transitions and state to the input FST.
    f::VectorFst<Arc> sffst(ifst);
    AddSuperFinal(&sffst, &have_iphi_);

    // Builds the composition of ifst and ofst and converts
    // to signed log.
    BuildComp(sffst, &cfst);
  }

  // Checks if composition is normalized.
  bool cnorm = CheckNorm(cfst);

  std::vector<SLWeight> distance, rdistance;
  if (!ComputeDistances(cnorm, &cfst, &distance, &rdistance)) {
    FSTERROR() << "Counter: shortest-distance computation failed.";
    ofst_->SetProperties(f::kError, f::kError);
    return;
  }

  // Extracts counts from the composition using the shortest distance
  CountArcs(cfst, distance, rdistance);
}

template <class Arc>
void Counter<Arc>::BuildComp(const fst::Fst<Arc> &ifst,
                             fst::MutableFst<SLArc> *ofst) const {
  namespace f = fst;
  using PM = Phi2Matcher<f::Matcher<f::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  f::ComposeFstOptions<Arc, PM, PF> copts;
  copts.gc_limit = 0;
  if (have_iphi_ || have_ophi_) {
    Label iphi_label = have_iphi_ ? phi_label_ : f::kNoLabel;
    Label ophi_label = have_ophi_ ? phi_label_ : f::kNoLabel;
    f::MatchType imatch_type = have_iphi_ ? f::MATCH_OUTPUT : f::MATCH_NONE;
    f::MatchType omatch_type = have_ophi_ ? f::MATCH_INPUT : f::MATCH_NONE;
    copts.matcher1 = new PM(ifst, imatch_type, iphi_label);
    copts.matcher2 = new PM(*ofst_, omatch_type, ophi_label);
  }

  f::ComposeFst<Arc> cfst(ifst, *ofst_, copts);
  // Moves to the signed-log converting phi-labels to epsilons.
  const Label rm_phi_label =
      have_iphi_ && have_ophi_ ? phi_label_ : f::kNoLabel;
  SLRmPhi(cfst, ofst, rm_phi_label, fst::MATCHER_REWRITE_NEVER);
}

template <class Arc>
bool Counter<Arc>::ComputeDistances(bool norm, fst::MutableFst<SLArc> *cfst,
                                    std::vector<SLWeight> *distance,
                                    std::vector<SLWeight> *rdistance) const {
  namespace f = fst;
  // Computes shortest distance on signed log semiring
  if (norm) {
    // Normalized: we only need the s.d. from the initial states
    SLShortestDistance sdist(cfst, phi_label_, delta_);
    if (!sdist.ComputeDistance(distance, false)) {
      return false;
    }
    rdistance->resize(distance->size(), SLWeight::One());
    return true;
  } else {
    // Not normalized: we need the s.d. in both directions
    SLShortestDistance sdist(cfst, phi_label_, delta_);
    return sdist.ComputeDistance(distance, false) &&
           sdist.ComputeDistance(rdistance, true);
  }
}

template <class Arc>
void Counter<Arc>::CountArcs(const fst::ExpandedFst<SLArc> &fst,
                             const std::vector<SLWeight> &distance,
                             const std::vector<SLWeight> &rdistance) {
  namespace f = fst;
  for (StateId s = 0; s < fst.NumStates(); ++s) {
    if (s >= distance.size()) continue;  // distance is Zero()
    SLWeight dist = distance[s];
    if (ApproxZero(dist)) continue;
    for (f::ArcIterator<f::Fst<SLArc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const auto &arc = aiter.Value();
      if (arc.ilabel && arc.ilabel != phi_label_) {
        SLWeight count = Times(dist, arc.weight);
        if (arc.nextstate >= rdistance.size()) continue;  // rdistance is Zero()
        SLWeight rdist = rdistance[arc.nextstate];
        if (ApproxZero(rdist)) continue;
        count = Times(count, rdist);
        // olabel stores arc ID
        ssize_t arc_id = arc.olabel;
        arc_counts_[arc_id] = Plus(arc_counts_[arc_id], count);
      }
    }
  }
}

template <class Arc>
void Counter<Arc>::PhiCounts() {
  namespace f = fst;
  using Matr = f::ExplicitMatcher<f::Matcher<f::Fst<Arc>>>;

  StateId initial = ofst_->Start();
  phi_counts_.resize(ofst_->NumStates(), SLWeight::Zero());

  if (phi_label_ == f::kNoLabel) return;

  for (StateId s = 0; s < ofst_->NumStates(); ++s) {
    for (f::ArcIterator<f::Fst<Arc>> aiter(*ofst_, s); !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      if (arc.ilabel == phi_label_) continue;
      ssize_t arc_id = arc.olabel;
      phi_counts_[arc.nextstate] =
          Plus(phi_counts_[arc.nextstate], arc_counts_[arc_id]);
      phi_counts_[s] = Minus(phi_counts_[s], arc_counts_[arc_id]);
    }
  }

  // Incoming count mass to superfinal state => incoming mass
  // to initial state.
  phi_counts_[initial] = Plus(phi_counts_[initial], phi_counts_[superfinal_]);

  std::vector<StateId> top_order;
  bool acyclic = PhiTopOrder(*ofst_, phi_label_, &top_order);
  if (!acyclic) {
    FSTERROR() << "Counter: topology FST is not canonical (phi-cyclic)";
    ofst_->SetProperties(f::kError, f::kError);
    return;
  }

  Matr matcher(*ofst_, f::MATCH_INPUT);
  for (StateId i = 0; i < ofst_->NumStates(); ++i) {
    StateId s = top_order[i];  // ith state in phi-top order
    matcher.SetState(s);
    if (matcher.Find(phi_label_)) {
      const Arc &arc = matcher.Value();
      StateId fails = arc.nextstate;
      phi_counts_[fails] = Plus(phi_counts_[fails], phi_counts_[s]);
    }
  }
}

template <class Arc>
void Counter<Arc>::FinalizeFst() {
  namespace f = fst;

  for (StateId s = 0; s < ofst_->NumStates(); ++s) {
    for (f::MutableArcIterator<f::MutableFst<Arc>> aiter(ofst_, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      SLWeight arc_count;
      if (arc.ilabel != phi_label_) {
        arc_count = arc_counts_[arc.olabel];  // olabel holds arc_id
      } else {
        arc_count = phi_counts_[s];
      }

      if (Less(arc_count, SLWeight::Zero())) {
        arc.weight = Weight::Zero();  // ensures sanity
      } else {
        arc.weight = from_sl_(arc_count);
      }

      if (arc.ilabel != sf_label_) {
        arc.olabel = arc.ilabel;  // projects on to input
        aiter.SetValue(arc);
      } else {
        ofst_->SetFinal(s, arc.weight);
      }
    }
  }

  // Deletes super-final state and transitions entering it.
  std::vector<StateId> dstates(1, superfinal_);
  ofst_->DeleteStates(dstates);
  f::ArcSort(ofst_, f::ILabelCompare<Arc>());
}


// Tests that the total weight entering each state
// equals the total weight leaving that state. This should
// be true, for example, of the result of Counter().
template <class Arc>
bool IsConservative(const fst::Fst<Arc> &fst,
                    float delta = fst::kDelta) {
  namespace f = fst;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using ArcItr = f::ArcIterator<f::Fst<Arc>>;
  using Log64Weight = f::Log64Weight;

  StateId nstates = CountStates(fst);

  f::WeightConvert<Weight, Log64Weight> to_log64;
  std::vector<Log64Weight> in_weight(nstates, Log64Weight::Zero());
  std::vector<Log64Weight> out_weight(nstates, Log64Weight::Zero());

  for (StateId s = 0; s < nstates; ++s) {
    for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc &arc = aiter.Value();
      const Log64Weight weight = to_log64(arc.weight);
      in_weight[arc.nextstate] = Plus(in_weight[arc.nextstate], weight);
      out_weight[s] = Plus(out_weight[s], weight);
    }
    if (fst.Final(s) != Weight::Zero()) {
      const Log64Weight weight = to_log64(fst.Final(s));
      out_weight[s] = Plus(out_weight[s], weight);
      in_weight[fst.Start()] = Plus(in_weight[fst.Start()], weight);
    }
  }
  for (StateId s = 0; s < nstates; ++s) {
    if (!ApproxEqual(in_weight[s], out_weight[s], delta)) {
      VLOG(1) << "state: " << s
              << " in_weight: " << in_weight[s]
              << " out_weight: " << out_weight[s];
      return false;
    }
  }
  return true;
}


}  // namespace sfst

#endif  // SFST_COUNT_H_
