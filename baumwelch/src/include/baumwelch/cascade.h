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

#ifndef BAUMWELCH_CASCADE_H_
#define BAUMWELCH_CASCADE_H_

#include <fst/fstlib.h>

// Cascade objects used during the E-step.

namespace fst {
namespace internal {

// Cascade objects represent the composition of a plaintext WFSA (usually a
// string or an LM), the channel model, and a ciphertext string FSA. They
// handle the actual composition as well as the mapping from cascade state IDs
// to channel state IDs. They have the following interface:
//
// template <class Arc>
// class Cascade {
//  public:
//   using StateId = typename Arc::StateId;
//
//   // Required constructor, which builds the cascade.
//   Cascade(const Fst<Arc> &plaintext, const Fst<Arc> &ciphertext,
//           const Fst<Arc> &channel);
//
//   // Returns reference to the cascade.
//   const ComposeFst<Arc> &GetFst() const;
//
//   // Maps from cascade state ID to channel state ID.
//   StateId ChannelState(StateId) const;
// };

// Cascade object that assumes the channel only has one state.
template <class Arc>
class SingleStateCascade {
 public:
  using StateId = typename Arc::StateId;

  SingleStateCascade(const Fst<Arc> &plaintext, const Fst<Arc> &ciphertext,
                     const Fst<Arc> &channel,
                     const CacheOptions &co_cache_options = CacheOptions(),
                     const CacheOptions &ico_cache_options = CacheOptions())
      : co_options_(co_cache_options),
        co_(channel, ciphertext, co_options_),
        ico_options_(ico_cache_options),
        ico_(plaintext, co_, ico_options_) {}

  const ComposeFst<Arc> &GetFst() const { return ico_; }

  constexpr StateId ChannelState(StateId) const { return 0; }

 private:
  const ComposeFstOptions<Arc> co_options_;
  const ComposeFst<Arc> co_;
  const ComposeFstOptions<Arc> ico_options_;
  const ComposeFst<Arc> ico_;
};

// Cascade object for a (possibly) multistate channel.
template <class Arc, class M = Matcher<Fst<Arc>>,
          class Filter = SequenceComposeFilter<M>,
          class StateTable =
              GenericComposeStateTable<Arc, typename Filter::FilterState>>
class MultiStateCascade {
 public:
  using StateId = typename Arc::StateId;

  MultiStateCascade(const Fst<Arc> &plaintext, const Fst<Arc> &ciphertext,
                    const Fst<Arc> &channel,
                    const CacheOptions &co_cache_options = CacheOptions(),
                    const CacheOptions &ico_cache_options = CacheOptions())
      : co_options_(co_cache_options, nullptr, nullptr, nullptr,
                    new StateTable(channel, ciphertext)),
        co_(channel, ciphertext, co_options_),
        ico_options_(ico_cache_options, nullptr, nullptr, nullptr,
                     new StateTable(plaintext, co_)),
        ico_(plaintext, co_, ico_options_) {}

  const ComposeFst<Arc> &GetFst() const { return ico_; }

  StateId ChannelState(StateId ico_state) const {
    const auto ic_state = InputChannelState(ico_state);
    return co_options_.state_table->Tuple(ic_state).StateId1();
  }

 private:
  StateId InputChannelState(StateId ico_state) const {
    return ico_options_.state_table->Tuple(ico_state).StateId2();
  }

  const ComposeFstOptions<Arc> co_options_;
  const ComposeFst<Arc> co_;
  const ComposeFstOptions<Arc> ico_options_;
  const ComposeFst<Arc> ico_;
};

}  // namespace internal
}  // namespace fst

#endif  // BAUMWELCH_CASCADE_H_

