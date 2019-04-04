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
//
// Copyright 2017 and onwards Google, Inc.

#ifndef BAUMWELCH_EXPECTATION_TABLE_H_
#define BAUMWELCH_EXPECTATION_TABLE_H_

// Tables of expectations for use in Baum-Welch training, and some helper
// objects and functions.

#include <cmath>
#include <cstddef>

#include <utility>
#include <vector>

#include <fst/fstlib.h>
#include <baumwelch/cascade.h>
#include <baumwelch/summation.h>
#include <unordered_map>

namespace fst {
namespace internal {

// Struct representing an arc without a weight, which acts as the key to most
// expectation tables. It is templated on a (weighted) arc.
template <class Arc>
struct UnweightedArc {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  explicit UnweightedArc(const Arc &arc)
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
bool operator==(const UnweightedArc<Arc> &left,
                const UnweightedArc<Arc> &right) {
  return left.ilabel == right.ilabel && left.olabel == right.olabel &&
         left.nextstate == right.nextstate;
}

template <class Arc>
bool operator!=(const UnweightedArc<Arc> &left,
                const UnweightedArc<Arc> &right) {
  return !(left == right);
}

// Portable hash function for the above.
template <class UArc>
struct UnweightedArcHash {
  size_t operator()(const UArc &uarc) const {
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
//   using Cascade = ...;
//
//   // Required constructor; the FST argument is usually just used to
//   // size the table.
//   explicit ExpectationTable(const Fst<Arc> &channel);
//
//   // Collects an arc expectation.
//   void CollectExpectation(StateId state, Label ilabel, Label olabel,
//                           const Weight &weight, StateId nextstate);
//
//   // Collects a state expectation. This may be no-op, or it may
//   // call the above overload using special label/state symbols.
//   void CollectExpectation(StateId state, const Weight &weight);
//
//   // Collects a likelihood for an observation using the reverse
//   // shortest path weight on the start state.
//   void CollectLikelihood(const Weight &weight);
//
//   // Returns the likelihood for the current iteration.
//   Weight Likelihood() const;
//
//   // Returns the M-step weight for a state/arc in the channel model.
//   Weight Maximize(StateId state, const Arc &arc) const;
//
//   // Returns the M-step final weight for an state in the channel model.
//   // This may call the above overload using a special label/state symbols.
//   Weight Maximize(StateId state) const;
//
//   // Resets the expectations and the likelihood.
//   void Reset();
// };

// Normalizes expectations globally.
//
// This is to be used with acyclic channel model.
template <class A>
class GlobalExpectationTable {
 public:
  using Arc = A;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using Cascade = internal::MultiStateCascade<Arc>;
  using UnweightedArc = internal::UnweightedArc<Arc>;
  using ArcHash = internal::UnweightedArcHash<UnweightedArc>;
  using Summation = internal::Summation<Weight>;

  explicit GlobalExpectationTable(const Fst<Arc> &channel)
      : num_states_(CountStates(channel)),
        table_(num_states_) {}

  // NB: This copies the table sizing and likelihood but not the expectations.
  GlobalExpectationTable &operator=(const GlobalExpectationTable &other) {
    num_states_ = other.num_states_;
    Reset();
    likelihood_ = other.likelihood_;
    return *this;
  }

  // Arc.
  void CollectExpectation(StateId state, const Arc &arc) {
    CollectExpectation(state, UnweightedArc(arc), arc.weight);
  }

  // Arc built on the fly.
  void CollectExpectation(StateId state, Label ilabel, Label olabel,
                          const Weight &weight, StateId nextstate) {
    CollectExpectation(state, UnweightedArc(ilabel, olabel, nextstate), weight);
  }

  // Final weight.
  void CollectExpectation(StateId state, const Weight &weight) {
    CollectExpectation(state, UnweightedArc(), weight);
  }

  void CollectLikelihood(const Weight &weight) {
    likelihood_.Add(weight);
  }

  Weight Likelihood() const { return likelihood_.Get(); }

  // Arc.
  Weight Maximize(StateId state, const Arc &arc) const {
    return Maximize(state, UnweightedArc(arc));
  }

  // Final weight.
  Weight Maximize(StateId state) const {
    return Maximize(state, UnweightedArc());
  }

  void Reset() {
    likelihood_.Reset();
    table_.clear();
    table_.resize(num_states_);
  }

 private:
  void CollectExpectation(StateId state, UnweightedArc &&uarc,
                          const Weight &weight) {
    auto &s_table = table_[state];
    auto it_and_success = s_table.emplace(uarc, weight);
    if (!it_and_success.second) {
      auto &iweight = it_and_success.first->second;
      iweight.Add(weight);
    }
  }

  Weight Maximize(StateId state, UnweightedArc &&uarc) const {
    const auto &s_table = table_[state];
    const auto &it = s_table.find(uarc);
    return (it == s_table.end() ?
            Weight::Zero() :
            Divide(it->second.Get(), likelihood_.Get()));
  }

  Summation likelihood_;
  size_t num_states_;
  std::vector<std::unordered_map<UnweightedArc, Summation, ArcHash>> table_;
};

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

  using Cascade = internal::MultiStateCascade<Arc>;
  using UnweightedArc = internal::UnweightedArc<Arc>;
  using ArcHash = internal::UnweightedArcHash<UnweightedArc>;
  using Summation = internal::Summation<Weight>;

  explicit StateExpectationTable(const Fst<Arc> &channel)
      : num_states_(CountStates(channel)),
        table_(num_states_) {}

  // NB: This copies the table sizing and likelihood but not the expectations.
  StateExpectationTable &operator=(const StateExpectationTable &other) {
    num_states_ = other.num_states_;
    Reset();
    likelihood_ = other.likelihood_;
    return *this;
  }

  // Arc.
  void CollectExpectation(StateId state, const Arc &arc) {
    CollectExpectation(state,
                       UnweightedArc(arc.ilabel, arc.olabel, arc.nextstate),
                       arc.weight);
  }

  // Arc built on the fly.
  void CollectExpectation(StateId state, Label ilabel, Label olabel,
                          const Weight &weight, StateId nextstate) {
    CollectExpectation(state, UnweightedArc(ilabel, olabel, nextstate), weight);
  }

  // Final weight.
  void CollectExpectation(StateId state, const Weight &weight) {
    CollectExpectation(state, UnweightedArc(), weight);
  }

  void CollectLikelihood(const Weight &weight) {
    likelihood_.Add(weight);
  }

  Weight Likelihood() const { return likelihood_.Get(); }

  // Arc.
  Weight Maximize(StateId state, const Arc &arc) const {
    return Maximize(state, UnweightedArc(arc));
  }

  // Final weight.
  Weight Maximize(StateId state) const {
    return Maximize(state, UnweightedArc());
  }

  void Reset() {
    likelihood_.Reset();
    table_.clear();
    table_.resize(num_states_);
  }

 private:
  void CollectExpectation(StateId state, UnweightedArc &&uarc,
                          const Weight &weight) {
    auto &spair = table_[state];
    spair.likelihood.Add(weight);
    auto it_and_success = spair.expectations.emplace(uarc, weight);
    if (!it_and_success.second) {
      auto &iweight = it_and_success.first->second;
      iweight.Add(weight);
    }
  }

  Weight Maximize(StateId state, UnweightedArc &&uarc) const {
    const auto &spair = table_[state];
    const auto it = spair.expectations.find(uarc);
    return (it == spair.expectations.end() ?
            Weight::Zero() :
            Divide(it->second.Get(), spair.likelihood.Get()));
  }

  struct Pair {
    std::unordered_map<UnweightedArc, Summation, ArcHash> expectations;
    Summation likelihood;
  };

  Summation likelihood_;
  size_t num_states_;
  std::vector<Pair> table_;
};

// Normalizes expectations locally, using input label as the conditioning
// factor.
//
// This is to be used with single-state channel models, as it assumes
// a single-state channel and ignores state IDs otherwise.
template <class A>
class ILabelExpectationTable {
 public:
  using Arc = A;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using Cascade = internal::SingleStateCascade<Arc>;
  using UnweightedArc = internal::UnweightedArc<Arc>;
  using Summation = internal::Summation<Weight>;

  // Constructor argument is ignored.
  explicit ILabelExpectationTable(const Fst<Arc> &) {}

  // NB: This copies the likelihood but not the expectations.
  ILabelExpectationTable &operator=(const ILabelExpectationTable &other) {
    Reset();
    likelihood_ = other.likelihood_;
    return *this;
  }

  // Arc.
  void CollectExpectation(StateId, const Arc &arc) {
    CollectExpectation(arc.ilabel, arc.olabel, arc.weight);
  }

  // Arc built on the fly.
  void CollectExpectation(StateId, Label ilabel, Label olabel,
                          const Weight &weight, StateId) {
    CollectExpectation(ilabel, olabel, weight);
  }

  // Final weight.
  void CollectExpectation(StateId, const Weight &weight) {
    CollectExpectation(kNoLabel, kNoLabel, weight);
  }

  void CollectLikelihood(const Weight &weight) {
    likelihood_.Add(weight);
  }

  Weight Likelihood() const { return likelihood_.Get(); }

  Weight Maximize(StateId, const Arc &arc) const {
    return Maximize(arc.ilabel, arc.olabel);
  }

  Weight Maximize(StateId) const { return Maximize(kNoLabel, kNoLabel); }

  void Reset() {
    likelihood_.Reset();
    table_.clear();
  }

 private:
  void CollectExpectation(Label ilabel, Label olabel, const Weight &weight) {
    auto it_and_success = table_.emplace(ilabel, Pair(olabel, weight));
    // If insertion just succeeded, we're done.
    if (it_and_success.second) return;
    // Otherwise, we need to mutate the inner table.
    auto &inner = it_and_success.first->second;
    // Handles the expectations.
    auto &expectations = inner.expectations;
    auto it = expectations.find(olabel);
    if (it == expectations.end()) {
      expectations[olabel].Set(weight);
    } else {
      auto &iweight = expectations[olabel];
      iweight.Add(weight);
    }
    // Handles the likelihood.
    auto &ilikelihood = inner.likelihood;
    ilikelihood.Add(weight);
  }

  Weight Maximize(Label ilabel, Label olabel) const {
    const auto it = table_.find(ilabel);
    // Fails to find ilabel.
    if (it == table_.end()) return Weight::Zero();
    const auto &inner = it->second;
    const auto iit = inner.expectations.find(olabel);
    return (iit == inner.expectations.end() ?
            Weight::Zero() :
            Divide(iit->second.Get(), inner.likelihood.Get()));
  }

  struct Pair {
    std::unordered_map<Label, Summation> expectations;
    Summation likelihood;

    // This constructor takes an olabel/weight pair and initializes both
    // elements accordingly.
    Pair(Label olabel, const Weight &weight) {
      expectations[olabel].Set(weight);
      likelihood.Set(weight);
    }
  };

  Summation likelihood_;
  std::unordered_map<Label, Pair> table_;
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

  using Cascade = internal::MultiStateCascade<Arc>;
  using UnweightedArc = internal::UnweightedArc<Arc>;
  using ArcHash = internal::UnweightedArcHash<UnweightedArc>;
  using Summation = internal::Summation<Weight>;

  explicit StateILabelExpectationTable(const Fst<Arc> &channel) :
        num_states_(CountStates(channel)),
        table_(num_states_) {}

  // NB: This copies the table sizing but not the actual expectations.
  StateILabelExpectationTable &operator=(
      const StateILabelExpectationTable &other) {
    num_states_ = other.num_states_;
    Reset();
    likelihood_ = other.likelihood_;
    return *this;
  }

  // Arc.
  void CollectExpectation(StateId state, const Arc &arc) {
    CollectExpectation(state, UnweightedArc(arc), arc.weight);
  }

  // Arc built on the fly.
  void CollectExpectation(StateId state, Label ilabel, Label olabel,
                          const Weight &weight, StateId nextstate) {
    CollectExpectation(state, UnweightedArc(ilabel, olabel, nextstate), weight);
  }

  // Final weight.
  void CollectExpectation(StateId state, const Weight &weight) {
    CollectExpectation(state, UnweightedArc(), weight);
  }

  void CollectLikelihood(const Weight &weight) { likelihood_.Add(weight); }

  Weight Likelihood() const { return likelihood_.Get(); }

  // Arc.
  Weight Maximize(StateId state, const Arc &arc) const {
    return Maximize(state, UnweightedArc(arc));
  }

  // Final weight.
  Weight Maximize(StateId state) const {
    return Maximize(state, UnweightedArc());
  }

  void Reset() {
    likelihood_.Reset();
    table_.clear();
    table_.resize(num_states_);
  }

 private:
  void CollectExpectation(StateId state, UnweightedArc &&uarc,
                          const Weight &weight) {
    auto &spair = table_[state];
    // Handles the expectations.
    {
      auto it_and_success = spair.expectations.emplace(uarc, weight);
      if (!it_and_success.second) {
        auto &iweight = it_and_success.first->second;
        iweight.Add(weight);
      }
    }
    // Handles the likelihoods.
    {
      auto it_and_success = spair.likelihoods.emplace(uarc.ilabel, weight);
      if (!it_and_success.second) {
        auto &iweight = it_and_success.first->second;
        iweight.Add(weight);
      }
    }
  }

  Weight Maximize(StateId state, UnweightedArc &&uarc) const {
    const auto &spair = table_[state];
    const auto &it = spair.expectations.find(uarc);
    if (it == spair.expectations.end()) {
      return Weight::Zero();
    } else {
      const auto likelihood = spair.likelihoods.find(uarc.ilabel)->second;
      return Divide(it->second.Get(), likelihood.Get());
    }
  }

  struct Pair {
    std::unordered_map<UnweightedArc, Summation, ArcHash> expectations;
    std::unordered_map<Label, Summation> likelihoods;
  };

  Summation likelihood_;
  size_t num_states_;
  std::vector<Pair> table_;
};

}  // namespace fst

#endif  // BAUMWELCH_EXPECTATION_TABLE_H_

