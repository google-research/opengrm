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

// Cascade objects used during the E-step.

#ifndef OPENGRM_BAUMWELCH_CASCADE_H_
#define OPENGRM_BAUMWELCH_CASCADE_H_

#include "openfst/lib/cache.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/state-table.h"

namespace fst {

// Struct holding two cache options structs.
struct CascadeOptions {
  CascadeOptions(const CascadeOptions&) = default;

  explicit CascadeOptions(
      const CacheOptions& co_cache_options = CacheOptions(),
      const CacheOptions& ico_cache_options = CacheOptions())
      : co_cache_options(co_cache_options),
        ico_cache_options(ico_cache_options) {}

  const CacheOptions co_cache_options;
  const CacheOptions ico_cache_options;
};

// Cascade objects represent the composition of a input WFSA (usually a
// string or an LM), the model, and a output string FSA. They minimally have the
// following interface:
//
// template <class Arc>
// class CascadeInterface {
//  public:
//   using StateId = typename Arc::StateId;
//
//   // Required constructor, which builds the cascade.
//   CascadeInterface(const Fst<Arc> &input, const Fst<Arc> &output,
//                    const Fst<Arc> &model);
//
//   // Returns reference to the cascade.
//   const ComposeFst<Arc> &GetFst() const;
// };

// Simple cascade object.
template <class Arc>
class SimpleCascade {
 public:
  using StateId = typename Arc::StateId;

  SimpleCascade(const Fst<Arc>& input, const Fst<Arc>& output,
                const Fst<Arc>& model,
                const CascadeOptions& opts = CascadeOptions())
      : co_options_(opts.co_cache_options),
        co_(model, output, co_options_),
        ico_options_(opts.ico_cache_options),
        ico_(input, co_, ico_options_) {}

  const ComposeFst<Arc>& GetFst() const { return ico_; }

 private:
  SimpleCascade(const SimpleCascade&) = delete;
  SimpleCascade& operator=(const SimpleCascade&) = delete;

  const ComposeFstOptions<Arc> co_options_;
  const ComposeFst<Arc> co_;
  const ComposeFstOptions<Arc> ico_options_;
  const ComposeFst<Arc> ico_;
};

// Cascade object that also keeps track of state IDs in the original model.
template <class Arc, class M = Matcher<Fst<Arc>>,
          class Filter = SequenceComposeFilter<M>,
          class StateTable =
              GenericComposeStateTable<Arc, typename Filter::FilterState>>
class ChannelStateCascade {
 public:
  using StateId = typename Arc::StateId;

  ChannelStateCascade(const Fst<Arc>& input, const Fst<Arc>& output,
                      const Fst<Arc>& model,
                      const CascadeOptions& opts = CascadeOptions())
      : co_options_(opts.co_cache_options, nullptr, nullptr, nullptr,
                    new StateTable(model, output)),
        co_(model, output, co_options_),
        ico_options_(opts.ico_cache_options, nullptr, nullptr, nullptr,
                     new StateTable(input, co_)),
        ico_(input, co_, ico_options_) {}

  const ComposeFst<Arc>& GetFst() const { return ico_; }

  StateId ChannelState(StateId ico_state) const {
    const auto ic_state = InputChannelState(ico_state);
    return co_options_.state_table->Tuple(ic_state).StateId1();
  }

 private:
  ChannelStateCascade(const ChannelStateCascade&) = delete;
  ChannelStateCascade& operator=(const ChannelStateCascade&) = delete;

  StateId InputChannelState(StateId ico_state) const {
    return ico_options_.state_table->Tuple(ico_state).StateId2();
  }

  const ComposeFstOptions<Arc> co_options_;
  const ComposeFst<Arc> co_;
  const ComposeFstOptions<Arc> ico_options_;
  const ComposeFst<Arc> ico_;
};

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_CASCADE_H_
