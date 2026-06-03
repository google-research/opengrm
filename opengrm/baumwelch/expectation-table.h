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

#ifndef OPENGRM_BAUMWELCH_EXPECTATION_TABLE_H_
#define OPENGRM_BAUMWELCH_EXPECTATION_TABLE_H_

// Tables of expectations for use in Baum-Welch training.

#include <cstddef>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "openfst/lib/fst.h"
#include "opengrm/baumwelch/log-adder.h"

namespace fst {
namespace internal {

// Struct representing an arc without a weight, which acts as the key to most
// expectation tables. It is templated on a (weighted) arc.
template <class Arc>
struct UnweightedArc {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  explicit UnweightedArc(const Arc& arc)
      : ilabel(arc.ilabel), olabel(arc.olabel), nextstate(arc.nextstate) {}

  UnweightedArc(Label ilabel, Label olabel, StateId nextstate)
      : ilabel(ilabel), olabel(olabel), nextstate(nextstate) {}

  // Used to represent a final weight.
  UnweightedArc() : ilabel(kNoLabel), olabel(kNoLabel), nextstate(kNoStateId) {}

  const Label ilabel;
  const Label olabel;
  const StateId nextstate;
};

// Equality operators for the above.

template <class Arc>
bool operator==(const UnweightedArc<Arc>& left,
                const UnweightedArc<Arc>& right) {
  return left.ilabel == right.ilabel && left.olabel == right.olabel &&
         left.nextstate == right.nextstate;
}

// Portable hash function for the above.
template <class UArc>
struct UnweightedArcHash {
  size_t operator()(const UArc& uarc) const {
    static constexpr auto prime0 = 7853;
    static constexpr auto prime1 = 7867;
    return uarc.nextstate + uarc.ilabel * prime0 + uarc.olabel * prime1;
  }
};

}  // namespace internal

// Expectation tables hold expectations and likelihoods during the E-step
// and are used to renormalize the weights during the M-step. They are also
// "tagged" with the appropriate cascade type used during the E-step. They have
// the following interface:
//
// template <class A>
// class ExpectationTable {
//  public:
//   using Arc = A;
//   using Label = typename Arc::Label;
//   using StateId = typename Arc::StateId;
//   using Weight = typename Arc::Weight;
//
//   // Required constructor; the FST argument is usually just used to
//   // size the table.
//   explicit ExpectationTable(const Fst<Arc> &channel);
//
//   // Collects an arc expectation.
//   void Forward(StateId state, Label ilabel, Label olabel,
//                Weight weight, StateId nextstate);
//
//   // Collects a state expectation. This may be no-op, or it may
//   // call the above overload using special label/state symbols.
//   void Forward(StateId state, Weight &weight);
//
//   // Returns the M-step weight for a state/arc in the channel model.
//   Weight Backward(StateId state, const Arc &arc) const;
//
//   // Returns the M-step final weight for an state in the channel model.
//   // This may call the above overload using a special label/state symbols.
//   Weight Backward(StateId state) const;
// };

// Normalizes expectations locally, using state ID as the conditioning factor.
//
// This is to be used when one has expanded a single-state channel model so
// that each non-initial state is reached by consuming an input label, and
// the initial state is reached by emitting an olabel (departing from one of the
// non-initial states). Because this results in a smaller binary search space
// during composition matching, this speeds up inference (compared to the
// ILabelExpectationTable) when there are a large number of input/output
// pairs permitted by the channel model.
template <class A>
class StateExpectationTable {
 public:
  using Arc = A;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using UnweightedArc = internal::UnweightedArc<Arc>;
  using ArcHash = internal::UnweightedArcHash<UnweightedArc>;
  using Sum = LogAdder<Weight>;

  explicit StateExpectationTable(const Fst<Arc>& channel)
      : table_(CountStates(channel)) {}

  // Arc built on the fly.
  void Forward(StateId state, Label ilabel, Label olabel, Weight weight,
               StateId nextstate) {
    Forward(state, UnweightedArc(ilabel, olabel, nextstate), std::move(weight));
  }

  // Final weight.
  void Forward(StateId state, Weight weight) {
    Forward(state, UnweightedArc(), std::move(weight));
  }

  // Arc.
  Weight Backward(StateId state, const Arc& arc) const {
    return Backward(state, UnweightedArc(arc));
  }

  // Final weight.
  Weight Backward(StateId state) const {
    return Backward(state, UnweightedArc());
  }

 private:
  StateExpectationTable(const StateExpectationTable&) = delete;
  StateExpectationTable& operator=(const StateExpectationTable&) = delete;

  void Forward(StateId state, UnweightedArc&& uarc, Weight weight) {
    auto& spair = table_[state];
    spair.likelihood.Add(weight);
    auto it_and_success = spair.expectations.emplace(uarc, weight);
    if (!it_and_success.second) {
      auto& iweight = it_and_success.first->second;
      iweight.Add(weight);
    }
  }

  Weight Backward(StateId state, UnweightedArc&& uarc) const {
    const auto& spair = table_[state];
    const auto it = spair.expectations.find(uarc);
    if (it == spair.expectations.end()) return Weight::Zero();
    const auto quotient = Divide(it->second.Sum(), spair.likelihood.Sum());
    return quotient.Member() ? quotient : Weight::Zero();
  }

  struct Pair {
    absl::flat_hash_map<UnweightedArc, Sum, ArcHash> expectations;
    Sum likelihood;
  };

  std::vector<Pair> table_;
};

// Normalizes expectations locally, using state ID and input label as
// conditioning factors.
//
// This is the most generic case, and can be used for a wide variety of channel
// topologies.
template <class A>
class StateILabelExpectationTable {
 public:
  using Arc = A;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using UnweightedArc = internal::UnweightedArc<Arc>;
  using ArcHash = internal::UnweightedArcHash<UnweightedArc>;
  using Sum = LogAdder<Weight>;

  explicit StateILabelExpectationTable(const Fst<Arc>& channel)
      : table_(CountStates(channel)) {}

  // Arc built on the fly.
  void Forward(StateId state, Label ilabel, Label olabel, Weight weight,
               StateId nextstate) {
    Forward(state, UnweightedArc(ilabel, olabel, nextstate), std::move(weight));
  }

  // Final weight.
  void Forward(StateId state, Weight weight) {
    Forward(state, UnweightedArc(), std::move(weight));
  }

  // Arc.
  Weight Backward(StateId state, const Arc& arc) const {
    return Backward(state, UnweightedArc(arc));
  }

  // Final weight.
  Weight Backward(StateId state) const {
    return Backward(state, UnweightedArc());
  }

 private:
  StateILabelExpectationTable(const StateILabelExpectationTable&) = delete;
  StateILabelExpectationTable& operator=(const StateILabelExpectationTable&) =
      delete;

  void Forward(StateId state, UnweightedArc&& uarc, Weight weight) {
    auto& spair = table_[state];
    // Handles the expectations.
    {
      auto it_and_success = spair.expectations.emplace(uarc, weight);
      if (!it_and_success.second) {
        auto& iweight = it_and_success.first->second;
        iweight.Add(weight);
      }
    }
    // Handles the likelihoods.
    {
      auto it_and_success = spair.likelihoods.emplace(uarc.ilabel, weight);
      if (!it_and_success.second) {
        auto& iweight = it_and_success.first->second;
        iweight.Add(weight);
      }
    }
  }

  Weight Backward(StateId state, UnweightedArc&& uarc) const {
    const auto& spair = table_[state];
    const auto& it = spair.expectations.find(uarc);
    if (it == spair.expectations.end()) return Weight::Zero();
    const auto likelihood = spair.likelihoods.find(uarc.ilabel)->second;
    const auto quotient = Divide(it->second.Sum(), likelihood.Sum());
    return quotient.Member() ? quotient : Weight::Zero();
  }

  struct Pair {
    absl::flat_hash_map<UnweightedArc, Sum, ArcHash> expectations;
    absl::flat_hash_map<Label, Sum> likelihoods;
  };

  std::vector<Pair> table_;
};

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_EXPECTATION_TABLE_H_
