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

// Classes to identify and remove useless states and transitions in
// a stochastic FST.

#ifndef OPENGRM_SFST_TRIM_H_
#define OPENGRM_SFST_TRIM_H_

#include <sys/types.h>

#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/cc-visitors.h"
#include "openfst/lib/dfs-visit.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {

// Identifies inaccessible transitions due to failure labels.
// Time complexity: O(V + E * max_phi_order).
// (Actually V -> V log(V) due to queue 'optimization' for the
// expected cases). Assumes (but does not fully check) that the input
// has the canonical topology (see canonical.h).
template <class Arc>
class PhiAccess {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;

  // Status of states in visitation.
  using Status = enum {
    UNDISCOVERED,      // not yet seen; not enqueued
    PHI_DISCOVERED,    // seen by a phi-transition; enqueued
    DISCOVERED,        // seen by a non-phi transition; enqueued
    PHI_VISITED,       // expanded with phi discovery; dequeued
    PHI_REDISCOVERED,  // expanded with phi discovery; re-enqueued
    VISITED            // expanded with discovery; dequeued
  };

  PhiAccess(const fst::Fst<Arc>& fst, typename Arc::Label phi_label)
      : fst_(fst),
        phi_label_(phi_label),
        nstates_(0),
        error_(false),
        s_(fst::kNoStateId) {
    if (fst_.Start() == fst::kNoStateId) return;

    ForbiddenLabels forbidden_labels;
    FindForbiddenLabels(&forbidden_labels);
    FindForbiddenPositions(forbidden_labels);
  }

  StateId NumStates() const { return nstates_; }

  // Returns true if there are 'forbidden transitions' at some states.
  bool HasForbidden() const { return !forbidden_positions_.empty(); }

  // Returns true if state is accessible on an allowed path.
  bool Accessible(StateId s) const { return status_[s] != UNDISCOVERED; }

  // These are used to iterate over the 'forbidden positions' at a state.

  // Sets current state for identifying 'forbidden positions'.
  void SetState(StateId s) {
    s_ = s;
    std::tie(forbidden_positions_iter_, forbidden_positions_end_) =
        forbidden_positions_.equal_range(s);
  }

  // Returns an arc position that is not accessible from the
  // current state: a 'forbidden position'. Arc position 0
  // is the first arc from the state, etc. Arc position -1
  // denotes the superfinal transition.
  ssize_t ForbiddenPosition() const {
    return forbidden_positions_iter_->second;
  }

  // Next forbidden position at the current state.
  void Next() { ++forbidden_positions_iter_; }

  // Any more forbidden positions?
  bool Done() const {
    return forbidden_positions_iter_ == forbidden_positions_end_;
  }

  // Error seen with this input.
  bool Error() const { return error_; }

 private:
  // Comparison function for phi-access queue.
  class Compare {
   public:
    Compare(const std::vector<Status>& status,
            const std::vector<StateId>& top_order)
        : status_(status), top_order_(top_order.size(), fst::kNoStateId) {
      // top_order_ gives the topological position of state id s.
      for (StateId s = 0; s < top_order.size(); ++s) {
        top_order_[top_order[s]] = s;
      }
    }

    // Discovered is first, then phi-discovered/phi-visited by top order.
    bool operator()(const StateId s1, const StateId s2) const {
      if (status_[s1] == DISCOVERED && status_[s2] == DISCOVERED) {
        return s1 < s2;
      } else if (status_[s1] == DISCOVERED) {
        return s1;
      } else if (status_[s2] == DISCOVERED) {
        return s2;
      } else {
        return top_order_[s1] < top_order_[s2];
      }
    }

   private:
    const std::vector<Status>& status_;
    std::vector<StateId> top_order_;
  };

  // Queue for visitation
  using PhiAccessQueue = fst::ShortestFirstQueue<StateId, Compare>;

  // Maps from StateId to set of labels that are phi-inaccessible.
  using ForbiddenLabels = std::vector<absl::flat_hash_set<Label>>;

  // Maps from state id to list of newly-allowed labels.
  using AllowedLabels = std::unordered_multimap<StateId, Label>;

  // Maps from state id to list of forbidden positions;
  using ForbiddenPositions = std::multimap<StateId, ssize_t>;

  // Explicit matcher (no epsilon loops)
  using Matr = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;

  // Finds the forbidden label set.
  void FindForbiddenLabels(ForbiddenLabels* forbidden_labels);

  // Converts forbidden labels to forbidden positions.
  void FindForbiddenPositions(const ForbiddenLabels& forbidden_labels);

  // Processes a phi-discovered state.
  void PhiDiscover(StateId s, StateId source, PhiAccessQueue* queue,
                   ForbiddenLabels* forbidden_labels,
                   AllowedLabels* allowed_labels);

  // Processes a newly discovered state.
  void Discover(StateId s, PhiAccessQueue* queue,
                ForbiddenLabels* forbidden_labels);

  // Sets forbidden labels to (phi) source's labels and forbidden labels.
  void SetForbidden(StateId s, StateId source,
                    ForbiddenLabels* forbidden_labels) const;

  // Deletes forbidden labels at s that are not forbidden at (phi) source
  // or by source. If alabels non-null, adds them to allowed labels.
  // Returns true if newly allowed labels at this state.
  bool UpdateForbidden(StateId s, StateId source,
                       ForbiddenLabels* forbidden_labels,
                       AllowedLabels* allowed_labels) const;

  void SetError() { error_ = true; }

  const fst::Fst<Arc>& fst_;
  Label phi_label_;
  ssize_t nstates_;
  bool error_;

  // Status of visitation at a state.
  std::vector<Status> status_;

  // Indicates arc positions at a state that are forbidden.
  ForbiddenPositions forbidden_positions_;

  // Iteration state
  StateId s_;
  typename ForbiddenPositions::iterator forbidden_positions_iter_;
  typename ForbiddenPositions::iterator forbidden_positions_end_;

  PhiAccess(const PhiAccess&) = delete;
  PhiAccess& operator=(const PhiAccess&) = delete;
};

template <class Arc>
void PhiAccess<Arc>::FindForbiddenLabels(ForbiddenLabels* forbidden_labels) {
  std::vector<StateId> top_order;
  bool acyclic = PhiTopOrder(fst_, phi_label_, &top_order);
  if (!acyclic) {
    FSTERROR() << "PhiAccess: FST is not canonical (phi-cyclic)";
    SetError();
    return;
  }

  nstates_ = top_order.size();
  forbidden_labels->resize(nstates_);

  if (phi_label_ == fst::kNoLabel) return;

  AllowedLabels allowed_labels;
  status_.resize(nstates_, UNDISCOVERED);
  Compare comp(status_, top_order);
  PhiAccessQueue queue(comp);
  StateId initial = fst_.Start();
  status_[initial] = DISCOVERED;
  queue.Enqueue(initial);

  // Visitation that favors (non-phi) discovered states first, then
  // phi-discovered states by phi top-order.
  while (!queue.Empty()) {
    StateId s = queue.Head();
    queue.Dequeue();

    if (status_[s] == DISCOVERED || status_[s] == PHI_DISCOVERED) {
      // Iterates over all arcs if newly (phi-)discovered.
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst_, s); !aiter.Done();
           aiter.Next()) {
        const Arc& arc = aiter.Value();
        if (arc.ilabel == phi_label_) {
          PhiDiscover(arc.nextstate, s, &queue, forbidden_labels,
                      &allowed_labels);
        } else if (status_[s] == DISCOVERED ||
                   (*forbidden_labels)[s].count(arc.ilabel) == 0) {
          Discover(arc.nextstate, &queue, forbidden_labels);
        }
      }
      status_[s] = (status_[s] == DISCOVERED) ? VISITED : PHI_VISITED;
    } else {  // PHI_REDISCOVERED
      // Iterates only over newly allowed arcs if already phi-visited.
      Matr matcher(fst_, fst::MATCH_INPUT);
      matcher.SetState(s);
      const auto [range_begin, range_end] = allowed_labels.equal_range(s);
      for (auto liter = range_begin; liter != range_end;) {
        if (matcher.Find(liter->second)) {
          for (; !matcher.Done(); matcher.Next()) {
            const Arc& arc = matcher.Value();
            Discover(arc.nextstate, &queue, forbidden_labels);
          }
          allowed_labels.erase(liter++);
        } else {
          ++liter;
        }
      }

      // Updates along the failure path.
      if (matcher.Find(phi_label_)) {
        const Arc& arc = matcher.Value();
        PhiDiscover(arc.nextstate, s, &queue, forbidden_labels,
                    &allowed_labels);
      }
      status_[s] = PHI_VISITED;
    }
  }
}

template <class Arc>
void PhiAccess<Arc>::FindForbiddenPositions(
    const ForbiddenLabels& forbidden_labels) {
  fst::SortedMatcher<fst::Fst<Arc>> matcher(fst_, fst::MATCH_INPUT);
  for (StateId s = 0; s < nstates_; ++s) {
    if (forbidden_labels[s].empty()) continue;
    matcher.SetState(s);
    for (auto liter = forbidden_labels[s].begin();
         liter != forbidden_labels[s].end(); ++liter) {
      if (*liter != fst::kNoLabel && matcher.Find(*liter)) {
        // forbidden label
        for (; !matcher.Done(); matcher.Next()) {
          ssize_t pos = matcher.Position();
          forbidden_positions_.insert(std::make_pair(s, pos));
        }
      } else if (*liter == fst::kNoLabel && fst_.Final(s) != Weight::Zero()) {
        // forbidden superfinal label
        forbidden_positions_.insert(std::make_pair(s, -1));
      }
    }
  }
}

template <class Arc>
void PhiAccess<Arc>::PhiDiscover(StateId s, StateId source,
                                 PhiAccessQueue* queue,
                                 ForbiddenLabels* forbidden_labels,
                                 AllowedLabels* allowed_labels) {
  switch (status_[s]) {
    case UNDISCOVERED:
      SetForbidden(s, source, forbidden_labels);
      status_[s] = PHI_DISCOVERED;
      queue->Enqueue(s);
      break;
    case PHI_DISCOVERED:
      UpdateForbidden(s, source, forbidden_labels, nullptr);
      break;
    case PHI_VISITED:
      if (UpdateForbidden(s, source, forbidden_labels, allowed_labels)) {
        status_[s] = PHI_REDISCOVERED;
        queue->Enqueue(s);
      }
      break;
    case PHI_REDISCOVERED:
      UpdateForbidden(s, source, forbidden_labels, allowed_labels);
      break;
    default:
      break;
  }
}

template <class Arc>
void PhiAccess<Arc>::Discover(StateId s, PhiAccessQueue* queue,
                              ForbiddenLabels* forbidden_labels) {
  switch (status_[s]) {
    case UNDISCOVERED:
      status_[s] = DISCOVERED;
      queue->Enqueue(s);
      break;
    case PHI_DISCOVERED:
    case PHI_VISITED:
    case PHI_REDISCOVERED:
      status_[s] = DISCOVERED;
      queue->Update(s);
      (*forbidden_labels)[s].clear();
      break;
    default:
      break;
  }
}

template <class Arc>
void PhiAccess<Arc>::SetForbidden(StateId s, StateId source,
                                  ForbiddenLabels* forbidden_labels) const {
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst_, source); !aiter.Done();
       aiter.Next()) {
    const Arc& arc = aiter.Value();
    if (arc.ilabel != phi_label_ && arc.ilabel != 0)
      (*forbidden_labels)[s].insert(arc.ilabel);
  }
  if (fst_.Final(source) != Weight::Zero())
    (*forbidden_labels)[s].insert(fst::kNoLabel);

  (*forbidden_labels)[s].insert((*forbidden_labels)[source].begin(),
                                (*forbidden_labels)[source].end());
}

template <class Arc>
bool PhiAccess<Arc>::UpdateForbidden(StateId s, StateId source,
                                     ForbiddenLabels* forbidden_labels,
                                     AllowedLabels* allowed_labels) const {
  Matr matcher(fst_, fst::MATCH_INPUT);
  bool newly_allowed = false;
  matcher.SetState(source);
  auto liter = (*forbidden_labels)[s].begin();
  while (liter != (*forbidden_labels)[s].end()) {
    if (*liter != fst::kNoLabel && matcher.Find(*liter)) {
      ++liter;
    } else if (*liter == fst::kNoLabel &&
               fst_.Final(source) != Weight::Zero()) {
      ++liter;
    } else if ((*forbidden_labels)[source].find(*liter) !=
               (*forbidden_labels)[source].end()) {
      ++liter;
    } else {
      // no longer forbidden
      if (*liter != fst::kNoLabel) {
        if (allowed_labels != nullptr)
          allowed_labels->insert(std::make_pair(s, *liter));
      }
      newly_allowed = true;
      (*forbidden_labels)[s].erase(liter++);
    }
  }
  return newly_allowed;
}

// Tests if input is trim - i.e., no inaccessible or coaccessible
// states or transitions due to failure transitions or otherwise
// (irrespective of weights). Assumes (but does not fully check) that
// the input has the canonical topology (see canonical.h).
template <class Arc>
bool IsTrim(const fst::Fst<Arc>& fst, typename Arc::Label phi_label) {
  uint64_t props = 0;
  fst::SccVisitor<Arc> scc_visitor(nullptr, nullptr, nullptr, &props);
  fst::DfsVisit(fst, &scc_visitor);
  if (props & (fst::kNotAccessible | fst::kNotCoAccessible)) return false;
  PhiAccess<Arc> phi_access(fst, phi_label);
  return !phi_access.HasForbidden();
}

// In trimming, some otherwise useless transitions if removed
// will change the failure semantics. In those cases, the
// trimming algorithm will:
//
// TRIM_NEEDED_TRIM: go ahead and remove. IsTrim() will be true
// but the result may not be equivalent to the input.
//
// Otherwise such transitions are directed to a unique new state n and:
//
// TRIM_NEEDED_FINAL: n is set final and the weights of transitions to
// it are set to kApproxZeroWeight. IsTrim() on the result will be true,
// but additional successful paths of with near zero weight may be added.
//
// TRIM_NEEDED_NONFINAL: n is set non-final. The successful paths are
// unchanged, but the result may have one non-coaccessible state,
// IsTrim() may be false.
enum TrimType {
  TRIM_NEEDED_TRIM = 0,
  TRIM_NEEDED_FINAL = 1,
  TRIM_NEEDED_NONFINAL = 2,
};

// Class to remove useless states and transitions in stochastic
// automata. Assumes (but does not fully check) that the input has the
// canonical topology (see canonical.h).
template <class Arc>
class Trimmer {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  // Explicit matcher (no epsilon loops)
  using Matr = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;
  using PhiMatr = fst::PhiMatcher<fst::Matcher<fst::Fst<Arc>>>;

  Trimmer(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
          TrimType trim_type = TRIM_NEEDED_FINAL)
      : fst_(fst),
        phi_label_(phi_label),
        dead_state_(fst::kNoStateId),
        dead_if_not_needed_state_(fst::kNoStateId),
        needed_state_(fst::kNoStateId),
        trim_type_(trim_type),
        chk_access_(false),
        chk_coaccess_(false) {}

  // Removes inaccessible transitions due to failure labels
  // If both PhiTrim() and (Sum)WeightTrim() are called, call PhiTrim() first.
  void PhiTrim();

  // Removes ApproxZero() weight transitions where possible
  // (optionally including phi_labeled ones) and connects.
  void WeightTrim(bool include_phi, Weight approx_zero = ApproxZeroWeight());

  // Equivalent to SumBackoff() + WeightTrim() followed by restoring original
  // weights on untrimmed transitions. Useful since it preserves
  // backoff-completeness.
  void SumWeightTrim(bool include_phi, Weight approx_zero = ApproxZeroWeight());

  // Removes inaccessible and non-coassessible states treating
  // failure labels as regular labels.
  void Connect() {
    chk_access_ = true;
    chk_coaccess_ = true;
  }

  // Finalizes result.
  void Finalize();

 private:
  // Returns the asborbing state that will be deleted, adding one if needed.
  StateId DeadState() {
    if (dead_state_ == fst::kNoStateId) {
      dead_state_ = fst_->AddState();
      del_states_.push_back(dead_state_);
    }
    return dead_state_;
  }

  // Returns the asborbing state that will be deleted if not needed,
  // adding one if needed.
  StateId DeadIfNotNeededState() {
    if (dead_if_not_needed_state_ == fst::kNoStateId) {
      dead_if_not_needed_state_ = fst_->AddState();
      del_states_.push_back(dead_if_not_needed_state_);
    }
    return dead_if_not_needed_state_;
  }

  // Returns the asborbing state that will be kept, adding one if needed.
  StateId NeededState() {
    if (needed_state_ == fst::kNoStateId) {
      needed_state_ = fst_->AddState();
      if (trim_type_ == TRIM_NEEDED_FINAL)
        fst_->SetFinal(needed_state_, Weight::One());
    }
    return needed_state_;
  }

  // Returns state ID of (first) failure state from s or kNoStateId
  // in none.
  StateId PhiNextState(StateId s) const {
    if (phi_label_ == fst::kNoLabel) return fst::kNoStateId;

    Matr matcher(fst_, fst::MATCH_INPUT);
    matcher.SetState(s);
    return matcher.Find(phi_label_) ? matcher.Value().nextstate
                                    : fst::kNoStateId;
  }

  // Returns weight near Zero()
  static Weight ApproxZeroWeight() {
    fst::WeightConvert<fst::Log64Weight, Weight> from_log;
    return from_log(kApproxZeroWeight);
  }

  fst::MutableFst<Arc>* fst_;
  Label phi_label_;
  StateId dead_state_;                // dead state
  StateId dead_if_not_needed_state_;  // dead-if-not-needed state
  StateId needed_state_;              // needed state
  TrimType trim_type_;
  std::vector<StateId> del_states_;  // states to delete
  bool chk_access_;                  // checks accessibility
  bool chk_coaccess_;                // checks coaccesssibility

  fst::WeightConvert<Weight, fst::Log64Weight> to_log_;

  Trimmer(const Trimmer&) = delete;
  Trimmer& operator=(const Trimmer&) = delete;
};

template <class Arc>
void Trimmer<Arc>::PhiTrim() {
  PhiAccess<Arc> phi_access(*fst_, phi_label_);
  if (phi_access.Error()) {
    fst_->SetProperties(fst::kError, fst::kError);
    return;
  }

  // Points forbidden arcs (including superfinal) to the dead state.
  for (StateId s = 0; s < phi_access.NumStates(); ++s) {
    // Marks an inaccessible state to be deleted.
    if (!phi_access.Accessible(s)) {
      del_states_.push_back(s);
      continue;
    }

    // Points forbidden arcs (including superfinal) to the dead state.
    fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst_, s);
    phi_access.SetState(s);
    for (; !phi_access.Done(); phi_access.Next()) {
      ssize_t pos = phi_access.ForbiddenPosition();
      if (pos != -1) {
        aiter.Seek(pos);
        Arc arc = aiter.Value();
        arc.nextstate = DeadState();
        arc.ilabel = fst::kNoLabel;
        arc.olabel = fst::kNoLabel;
        aiter.SetValue(arc);
      } else {
        fst_->SetFinal(s, Weight::Zero());
      }
    }
  }
  chk_coaccess_ = true;
  if (!del_states_.empty()) {
    if (dead_if_not_needed_state_ != fst::kNoStateId) {
      FSTERROR() << "Trimmer::PhiTrim() called after (Sum)WeightTrim()";
      fst_->SetProperties(fst::kError, fst::kError);
    }
    fst_->DeleteStates(del_states_);
    fst::ArcSort(fst_, fst::ILabelCompare<Arc>());
    del_states_.clear();
    dead_state_ = fst::kNoLabel;
  }
}

template <class Arc>
void Trimmer<Arc>::WeightTrim(bool include_phi, Weight approx_zero) {
  const fst::Log64Weight log_approx_zero = to_log_(approx_zero);
  const StateId nstates = fst_->NumStates();  // excludes any added states
  for (StateId s = 0; s < nstates; ++s) {
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst_, s);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      if ((include_phi || arc.ilabel != phi_label_) &&
          arc.nextstate != dead_state_) {
        fst::Log64Weight arc_weight = to_log_(arc.weight);
        if (ApproxZero(arc_weight, log_approx_zero)) {
          // redirects to dead-if-not-needed state
          arc.nextstate = DeadIfNotNeededState();
          aiter.SetValue(arc);
        }
      }
    }
  }
  chk_access_ = true;
  chk_coaccess_ = true;
}

template <class Arc>
void Trimmer<Arc>::SumWeightTrim(bool include_phi, Weight approx_zero) {
  const fst::Log64Weight log_approx_zero = to_log_(approx_zero);
  fst::VectorFst<Arc> sum_fst(*fst_);
  SumBackoff(&sum_fst, phi_label_);
  const StateId nstates = fst_->NumStates();  // excludes any added states
  for (StateId s = 0; s < nstates; ++s) {
    fst::ArcIterator<fst::Fst<Arc>> sum_aiter(sum_fst, s);
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst_, s);
         !aiter.Done(); aiter.Next(), sum_aiter.Next()) {
      Arc arc = aiter.Value();
      if ((include_phi || arc.ilabel != phi_label_) &&
          arc.nextstate != dead_state_) {
        const Arc& sum_arc = sum_aiter.Value();
        fst::Log64Weight sum_arc_weight = to_log_(sum_arc.weight);
        if (ApproxZero(sum_arc_weight, log_approx_zero)) {
          // redirects to dead-if-not-needed state
          arc.nextstate = DeadIfNotNeededState();
          aiter.SetValue(arc);
        }
      }
    }
  }
  chk_access_ = true;
  chk_coaccess_ = true;
}

template <class Arc>
void Trimmer<Arc>::Finalize() {
  std::vector<bool> access;
  std::vector<bool> coaccess;
  uint64_t props = 0;
  if (chk_access_ || chk_coaccess_) {
    fst::SccVisitor<Arc> scc_visitor(nullptr, &access, &coaccess, &props);
    fst::DfsVisit(*fst_, &scc_visitor);
    PhiMatr phi_matcher(fst_, fst::MATCH_INPUT, phi_label_);

    const ssize_t nstates = fst_->NumStates();
    for (StateId s = 0; s < nstates; ++s) {
      // Marks an inaccessible state for deletion.
      if (chk_access_ && !access[s]) del_states_.push_back(s);

      // Marks a non-coaccessible, unneeded state for deletion.
      if (chk_coaccess_ && s != needed_state_ && !coaccess[s])
        del_states_.push_back(s);

      if (trim_type_ == TRIM_NEEDED_TRIM) continue;

      // Points arcs needed to preserve the failure semantics to the
      // needed state. Gives them kApproxZeroWeight.
      StateId phi_nextstate = PhiNextState(s);
      if (chk_coaccess_ && phi_nextstate != fst::kNoStateId) {
        phi_matcher.SetState(phi_nextstate);
        fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst_, s);
        for (; !aiter.Done(); aiter.Next()) {
          Arc arc = aiter.Value();
          if (arc.nextstate == dead_state_ || coaccess[arc.nextstate] ||
              arc.ilabel == phi_label_ || arc.ilabel == 0) {
            continue;
          }
          for (phi_matcher.Find(arc.ilabel); !phi_matcher.Done();
               phi_matcher.Next()) {
            const Arc& failed_arc = phi_matcher.Value();
            if (coaccess[failed_arc.nextstate]) {
              // redirects to the needed state
              arc.nextstate = NeededState();
              arc.weight = ApproxZeroWeight();
              aiter.SetValue(arc);
              break;
            }
          }
        }
      }
    }
  }

  if (!del_states_.empty()) fst_->DeleteStates(del_states_);
  if (!del_states_.empty() || needed_state_ != fst::kNoStateId)
    fst::ArcSort(fst_, fst::ILabelCompare<Arc>());
}

// Simple interface to trimming. Removes useless states and
// transitions in stochastic automata (irrespective of weights).
// Returns true on success.
template <class Arc>
bool Trim(fst::MutableFst<Arc>* fst,
          typename Arc::Label phi_label = fst::kNoLabel,
          TrimType trim_type = TRIM_NEEDED_FINAL) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "Trim: FST is not canonical";
    return false;
  }
  Trimmer<Arc> trim(fst, phi_label, trim_type);
  trim.PhiTrim();
  trim.Finalize();
  return !fst->Properties(fst::kError, false);
}

}  // namespace sfst

#endif  // OPENGRM_SFST_TRIM_H_
