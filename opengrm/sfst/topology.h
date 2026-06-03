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

// Algorithms for constructing specific FST topologies.

#ifndef OPENGRM_SFST_TOPOLOGY_H_
#define OPENGRM_SFST_TOPOLOGY_H_

#include <sys/types.h>

#include <cstddef>
#include <utility>
#include <vector>

#include "absl/container/node_hash_map.h"
#include "absl/container/node_hash_set.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/weight.h"

namespace sfst {

// Finds the ngrams up to a given order in a canonical SFSA.
// N-grams that contain embedded failure or epsilon transitions
// are not considered.
template <class Arc>
class NGramTopology {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  // Constructs an NGramTopology object for a given n-gram 'order'.
  // The 'phi_label' is the failure label. Result is returned
  // in 'ofst'. Output will be a canonical OpenGrm ngram FST model.
  NGramTopology(int order, Label phi_label, fst::MutableFst<Arc>* ofst);

  // Computes ngram topology of 'ifst'. Assumes (but does not fully
  // check) the input is a canonical stochastic FSA (see canonical.h).
  void FindNGrams(const fst::Fst<Arc>& ifst);

 private:
  // Ngram state data.
  struct NGramState {
    StateId backoff_state;  // ID of the backoff state for the current state.
    int order;              // N-gram order of the state (of the outgoing arcs)
    NGramState(StateId s, int o) : backoff_state(s), order(o) {}
  };

  using Pair = std::pair<ssize_t, ssize_t>;

  struct PairHash {
    size_t operator()(const Pair& p) const {
      return (static_cast<size_t>(p.first) * 55697) ^
             (static_cast<size_t>(p.second) * 54631);
    }
  };

  using DestMap = absl::node_hash_map<Pair, ssize_t, PairHash>;

  // Adds n-gram corresponding to the arc labeled 'label'
  // out of state with ID 'state_id' if it doesn't already exist.
  // Returns its destination state.
  StateId UpdateNGram(StateId state_id, Label label, DestMap* dest_map);

  // UpdateNGram for n-grams that end in the super-final label.
  void UpdateFinalNGram(StateId state_id) {
    while (state_id != fst::kNoStateId &&
           ofst_->Final(state_id) == Weight::Zero()) {
      ofst_->SetFinal(state_id, Weight::One());
      state_id = states_[state_id].backoff_state;
    }
  }

  int order_;                       // Max. order of n-gram being counted
  Label phi_label_;                 // Failure label
  fst::MutableFst<Arc>* ofst_;      // Output n-gram FST
  std::vector<NGramState> states_;  // Maps state IDs to NGramStates
  StateId initial_;                 // ID of start state
  StateId backoff_;                 // ID of unigram/backoff state
  fst::WeightConvert<fst::Log64Weight, Weight> from_log_;
  fst::WeightConvert<Weight, fst::Log64Weight> to_log_;

  NGramTopology(const NGramTopology&) = delete;
  NGramTopology& operator=(const NGramTopology&) = delete;
};

template <class Arc>
NGramTopology<Arc>::NGramTopology(int order, Label phi_label,
                                  fst::MutableFst<Arc>* ofst)
    : order_(order), phi_label_(phi_label), ofst_(ofst) {
  ofst->DeleteStates();

  if (order == 0) {
    FSTERROR() << "NGramTopology: order must be greater than 0";
    ofst->SetProperties(fst::kError, fst::kError);
    return;
  }
  if (phi_label_ == fst::kNoLabel) {
    FSTERROR() << "NGramTopology: bad phi label: " << phi_label_;
    ofst->SetProperties(fst::kError, fst::kError);
    return;
  }

  backoff_ = ofst->AddState();
  states_.push_back(NGramState(fst::kNoStateId, 1));
  if (order == 1) {
    initial_ = backoff_;
  } else {
    initial_ = ofst->AddState();
    states_.push_back(NGramState(backoff_, 2));
  }
  ofst->SetStart(initial_);
}

template <class Arc>
void NGramTopology<Arc>::FindNGrams(const fst::Fst<Arc>& ifst) {
  if (ifst.Start() == fst::kNoStateId) {
    FSTERROR() << "NGramTopology: input FST has no states";
    ofst_->SetProperties(fst::kError, fst::kError);
    return;
  }
  if (!ifst.Properties(fst::kAcceptor, true)) {
    FSTERROR() << "NGramTopology: input FST not an acceptor";
    ofst_->SetProperties(fst::kError, fst::kError);
    return;
  }

  // Finds n-grams up to order n in the input.
  std::vector<Pair> queue;
  absl::node_hash_set<Pair, PairHash> pairset;
  DestMap dest_map;
  Pair start_pair(ifst.Start(), initial_);
  pairset.insert(start_pair);
  queue.push_back(start_pair);
  bool non_trivial_label = false;
  while (!queue.empty()) {
    Pair current_pair = queue.back();
    auto fst_state = current_pair.first;
    StateId ngram_state = current_pair.second;
    queue.pop_back();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(ifst, fst_state); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      // Next pair will use backoff_ count state if failure arc
      Pair next_pair(arc.nextstate, backoff_);
      if (arc.ilabel && arc.ilabel != phi_label_) {
        // Next pair uses n-gram destination state of non-special arc
        next_pair.second = UpdateNGram(ngram_state, arc.ilabel, &dest_map);
        non_trivial_label = true;
      } else if (phi_label_ != 0 && arc.ilabel == 0) {
        // Next pair uses n-gram source state for epsilon arc
        next_pair.second = ngram_state;
      }
      auto iter = pairset.find(next_pair);
      if (iter == pairset.end()) {  // If new pair, inserts it.
        pairset.insert(next_pair);
        queue.push_back(next_pair);
      }
    }
    if (ifst.Final(fst_state) != Weight::Zero()) UpdateFinalNGram(ngram_state);
  }

  // Input should have some non-trival labeled arcs
  if (!non_trivial_label) {
    FSTERROR() << "NGramTopology: input FST has no non-trivial path";
    ofst_->SetProperties(fst::kError, fst::kError);
    return;
  }

  // Sets up backoff arcs and symbol tables.
  for (StateId s = 0; s < states_.size(); ++s) {
    if (states_[s].backoff_state != fst::kNoStateId) {
      ofst_->AddArc(s, Arc(phi_label_, phi_label_, Weight::One(),
                           states_[s].backoff_state));
    }
  }

  fst::ArcSort(ofst_, fst::ILabelCompare<Arc>());
  ofst_->SetInputSymbols(ifst.InputSymbols());
  ofst_->SetOutputSymbols(ifst.OutputSymbols());
}

template <class Arc>
typename Arc::StateId NGramTopology<Arc>::UpdateNGram(StateId state_id,
                                                      Label label,
                                                      DestMap* dest_map) {
  // First determines if there already exists a corresponding arc.
  Pair p(state_id, label);
  auto iter = dest_map->find(p);
  if (iter != dest_map->end()) return iter->second;

  // Otherwise, creates the arc
  NGramState ngram_state = states_[state_id];

  Arc arc(label, label, Weight::One(), initial_);

  if (order_ != 1) {
    // Finds the backed-off arc destination.
    StateId backoff_dest =
        ngram_state.backoff_state == fst::kNoStateId
            ? fst::kNoStateId
            : UpdateNGram(ngram_state.backoff_state, label, dest_map);

    // Computes the current arc destination state.
    if (ngram_state.order == order_) {
      // The destination state is the destination of the backed-off arc.
      arc.nextstate = backoff_dest;
    } else {
      // The destination state needs to be created.
      arc.nextstate = ofst_->AddState();
      NGramState next_ngram_state(
          backoff_dest == fst::kNoStateId ? backoff_ : backoff_dest,
          ngram_state.order + 1);
      states_.push_back(next_ngram_state);
    }
  }
  ofst_->AddArc(state_id, arc);
  (*dest_map)[p] = arc.nextstate;
  return arc.nextstate;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_TOPOLOGY_H_
