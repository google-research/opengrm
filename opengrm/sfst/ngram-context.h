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

// Classes to parse and maintain context specifications for SFst models.

#ifndef OPENGRM_SFST_NGRAM_CONTEXT_H_
#define OPENGRM_SFST_NGRAM_CONTEXT_H_

#include <algorithm>
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {

// Represents a context interval.
class NGramContext {
 public:
  using Label = fst::StdArc::Label;
  using StateId = fst::StdArc::StateId;

  // Constructs a context specification from begin and end context
  // vectors. If the context is less than the n-gram order - 1, it is
  // padded with 0 on the left. The begin and end context vectors
  // specify a (half-open) interval of highest-order state contexts in
  // an LM with the interval defined using the reverse lexicographic
  // order (i.e., on the reverse of the context). All suffixes of
  // these contexts are also included for proper backoff (when
  // include_all_suffixes = true).
  //
  // Example 1: context_begin = {1,1,1,1} and context_end = {1,1,1,5} with
  // a 5-gram:
  //   specifies states that have a rightmost context in [1,5).
  //
  // Example 2: context_begin = {1} and context_end = {5,6} with a 5-gram:
  //   same as context_begin = {0,0,0,1} and context_end = {0,0,5,6}.
  NGramContext(std::vector<Label> context_begin, std::vector<Label> context_end,
               int hi_order);

  // Constructs a context specification from context pattern string.
  // Expected format: "w_1 ... w_m : v_1 ... v_n" where
  // the w_i and v_i are numeric word IDs and m, n are typically less than
  // the n-gram order. A word ID 0 signifies the initial word.
  //
  // Example: "1 1 1 1 : 1 1 1 5" signifies a begin context vector of
  //   {1,1,1,1} and an end context vector of {1,1,1,5}.
  NGramContext(absl::string_view context_pattern, int hi_order);

  // Null context.
  NGramContext() : hi_order_(0) {}

  // Is n-gram in context? If 'include_all_suffixes' is true, then all
  // suffixes of the begin and end contexts are considered in
  // context. When false, true (reverse) lexicographic order is used.
  bool HasContext(const std::vector<Label>& ngram,
                  bool include_all_suffixes = true) const;

  // No/empty context requested?
  int NullContext() const { return context_begin_.empty(); }

  // Derives begin and end context vectors from input context pattern string.
  static void ParseContextInterval(absl::string_view context_pattern,
                                   std::vector<Label>* context_begin,
                                   std::vector<Label>* context_end);

  // Generates context string from begin and end context vectors.
  static std::string GetContextString(absl::Span<const Label> context_begin,
                                      absl::Span<const Label> context_end);

  // Begin context as could be passed to class constructor.
  std::vector<Label> GetContextBegin() const;

  // End context as could be passed to class constructor.
  std::vector<Label> GetContextEnd() const;

  // Context is reversed and padded to high-order.
  const std::vector<Label>& GetReverseContextBegin() const {
    return context_begin_;
  }

  // Context is reversed and padded to high-order.
  const std::vector<Label>& GetReverseContextEnd() const {
    return context_end_;
  }

  // Note order is with respect to transitions not states in the model;
  // so state ngram.size() == 1 has order 2.
  int GetHiOrder() const { return hi_order_; }

  // Changes hi order (which affects context padding).
  // Used by NGramExtendedContext to put several NGramContexts on the same
  // hi-order.
  void SetHiOrder(int hi_order);

 private:
  void Init();

  int hi_order_;
  std::vector<Label> context_begin_;
  std::vector<Label> context_end_;
};

// Represents a set of disjoint context intervals.
class NGramExtendedContext {
 public:
  using Label = fst::StdArc::Label;

  // Constructs a context specification from begin and end context vectors.
  // See the corresponding NGramContext constructor.
  NGramExtendedContext(const std::vector<Label>& context_begin,
                       const std::vector<Label>& context_end, int hi_order);

  // Constructs a context specification from an extended context
  // pattern string. An extended context pattern is a comma-separated
  // set of NGramContext context patterns that must be disjoint.
  // If 'merge_contexts' is true, adjacent contexts will be merged.
  NGramExtendedContext(absl::string_view extended_context_pattern, int hi_order,
                       bool merge_contexts = true);

  // Constructs a context specification from a NGramContext vector.
  // If 'merge_contexts' is true, adjacent contexts will be merged.
  explicit NGramExtendedContext(const std::vector<NGramContext>& contexts,
                                bool merge_contexts = true);

  // Null context.
  NGramExtendedContext() = default;

  // No/empty context requested?
  int NullContext() const { return contexts_.empty(); }

  // Is n-gram in context? If 'include_all_suffixes' is true, then all
  // suffixes of the begin and end contexts are considered in
  // context. When false, true (reverse) lexicographic order is used.
  bool HasContext(const std::vector<Label>& ngram,
                  bool include_all_suffixes = true) const;

  // Finds NGramContext that matches context. Returns a null pointer
  // if no match or if the input is the null context. If
  // 'include_all_suffixes' is true, then all suffixes of the begin
  // and end contexts are considered in context. When false, true
  // (reverse) lexicographic order is used.
  const NGramContext* GetContext(const std::vector<Label>& ngram,
                                 bool include_all_suffixes = true) const;

  // Derives NGramContext vector from input extended context pattern string.
  static void ParseContextIntervals(absl::string_view extended_context_pattern,
                                    int hi_order,
                                    std::vector<NGramContext>* contexts);

  // Generates an extended context string from a vector of NGramContexts.
  static std::string GetExtendedContextString(
      absl::Span<const NGramContext> contexts);

  const std::vector<NGramContext>& GetContexts() const { return contexts_; }

 private:
  struct ContextCompare {
    bool operator()(const NGramContext& c1, const NGramContext& c2) const {
      const auto& b1 = c1.GetReverseContextBegin();
      const auto& b2 = c2.GetReverseContextBegin();
      return std::lexicographical_compare(b1.begin(), b1.end(), b2.begin(),
                                          b2.end());
    }
  };

  // Ensures disjoint, same hi-order and canonicalizes.
  void Init(bool merge_contexts);

  bool CheckContexts() const;

  void MergeContexts(size_t i, size_t j, size_t k);

  std::vector<NGramContext> contexts_;
};

namespace internal {

template <class Arc>
struct SplitModelInfo {
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  StateId num_states = 0;
  StateId unigram = fst::kNoStateId;
  int hi_order = 0;
  std::vector<StateId> backoff_states;
  std::vector<Weight> backoff_weights;
  std::vector<int> state_orders;
  std::vector<std::vector<Label>> state_ngrams;
};

template <class Arc>
bool InitSplitModel(const fst::Fst<Arc>& fst, typename Arc::Label phi_label,
                    SplitModelInfo<Arc>* info) {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  if (fst.Start() == fst::kNoStateId) return false;
  info->num_states = fst::CountStates(fst);
  info->backoff_states.assign(info->num_states, fst::kNoStateId);
  info->backoff_weights.assign(info->num_states, Weight::Zero());
  for (StateId s = 0; s < info->num_states; ++s) {
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        info->backoff_states[s] = arc.nextstate;
        info->backoff_weights[s] = arc.weight;
        break;
      }
    }
  }
  info->state_orders.assign(info->num_states, -1);
  info->state_ngrams.resize(info->num_states);
  info->hi_order = 1;
  info->unigram = info->backoff_states[fst.Start()];
  std::deque<StateId> state_queue;
  if (info->unigram != fst::kNoStateId) {
    info->state_orders[info->unigram] = 1;
    state_queue.push_back(info->unigram);
    info->state_orders[fst.Start()] = info->hi_order = 2;
    state_queue.push_back(fst.Start());
    info->state_ngrams[fst.Start()].push_back(0);
  } else {
    info->unigram = fst.Start();
    info->state_orders[fst.Start()] = 1;
    state_queue.push_back(fst.Start());
  }
  while (!state_queue.empty()) {
    StateId state = state_queue.front();
    state_queue.pop_front();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, state); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      if (info->state_orders[arc.nextstate] == -1) {
        info->state_orders[arc.nextstate] = info->state_orders[state] + 1;
        info->state_ngrams[arc.nextstate] = info->state_ngrams[state];
        info->state_ngrams[arc.nextstate].push_back(arc.ilabel);
        if (info->state_orders[state] >= info->hi_order) {
          info->hi_order = info->state_orders[state] + 1;
        }
        state_queue.push_back(arc.nextstate);
      }
    }
  }
  return true;
}

template <class Arc>
bool CreateSplitFsts(const fst::Fst<Arc>& fst, const SplitModelInfo<Arc>& info,
                     absl::Span<const NGramContext> contexts,
                     typename Arc::Label phi_label, bool include_all_suffixes,
                     std::vector<fst::VectorFst<Arc>>* out_fsts) {
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  std::vector<absl::flat_hash_set<size_t>> state_splits(info.num_states);
  for (StateId state = 0; state < info.num_states; ++state) {
    const std::vector<Label>& ngram = info.state_ngrams[state];
    for (size_t i = 0; i < contexts.size(); ++i) {
      if (contexts[i].HasContext(ngram, include_all_suffixes)) {
        state_splits[state].insert(i);
      }
    }
  }
  for (size_t i = 0; i < contexts.size(); ++i) {
    state_splits[fst.Start()].insert(i);
  }
  for (int order = info.hi_order; order > 0; --order) {
    for (StateId state = 0; state < info.num_states; ++state) {
      if (info.state_orders[state] != order) continue;
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, state); !aiter.Done();
           aiter.Next()) {
        const Arc& arc = aiter.Value();
        if (info.state_orders[arc.nextstate] != info.state_orders[state] + 1) {
          continue;
        }
        for (size_t idx : state_splits[arc.nextstate]) {
          state_splits[state].insert(idx);
        }
      }
      StateId bo_state = info.backoff_states[state];
      if (bo_state >= 0) {
        for (size_t idx : state_splits[state]) {
          state_splits[bo_state].insert(idx);
        }
      }
    }
  }
  for (size_t i = 0; i < contexts.size(); ++i) {
    if (state_splits[info.unigram].count(i) != 1) {
      LOG(ERROR) << "NGramSplit: Unigram state not added to every split";
      return false;
    }
  }
  std::vector<std::vector<StateId>> context_states(contexts.size());
  for (StateId state = 0; state < info.num_states; ++state) {
    for (size_t idx : state_splits[state]) {
      context_states[idx].push_back(state);
    }
  }
  out_fsts->clear();
  out_fsts->resize(contexts.size());
  for (size_t context_idx = 0; context_idx < contexts.size(); ++context_idx) {
    fst::VectorFst<Arc>* split_fst = &(*out_fsts)[context_idx];
    split_fst->SetInputSymbols(fst.InputSymbols());
    split_fst->SetOutputSymbols(fst.OutputSymbols());
    std::vector<StateId> state_map(info.num_states, fst::kNoStateId);
    split_fst->AddStates(context_states[context_idx].size());
    for (size_t i = 0; i < context_states[context_idx].size(); ++i) {
      StateId state = context_states[context_idx][i];
      state_map[state] = static_cast<StateId>(i);
    }
    for (StateId state : context_states[context_idx]) {
      StateId bo_state = info.backoff_states[state];
      if (bo_state != fst::kNoStateId) {
        split_fst->AddArc(state_map[state],
                          Arc(phi_label, phi_label, info.backoff_weights[state],
                              state_map[bo_state]));
      }
      if (state == fst.Start()) split_fst->SetStart(state_map[state]);
      const std::vector<Label>& ngram = info.state_ngrams[state];
      bool in_context =
          contexts[context_idx].HasContext(ngram, include_all_suffixes);
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, state); !aiter.Done();
           aiter.Next()) {
        const Arc& arc = aiter.Value();
        if (arc.ilabel == phi_label) continue;
        StateId nextstate = arc.nextstate;
        if (!in_context &&
            ((info.state_orders[nextstate] != info.state_orders[state] + 1) ||
             state_map[nextstate] == fst::kNoStateId)) {
          continue;
        }
        if (!in_context && state != fst.Start() &&
            state_map[nextstate] == fst::kNoStateId) {
          LOG(ERROR)
              << "NGramSplit: out of context n-gram with no destination state";
          return false;
        }
        while (state_map[nextstate] == fst::kNoStateId) {
          nextstate = info.backoff_states[nextstate];
          if (nextstate == fst::kNoStateId) {
            LOG(ERROR)
                << "NGramSplit: backoff state not found for destination state";
            return false;
          }
        }
        split_fst->AddArc(
            state_map[state],
            Arc(arc.ilabel, arc.olabel, arc.weight, state_map[nextstate]));
      }
      if (in_context && fst.Final(state) != Weight::Zero()) {
        split_fst->SetFinal(state_map[state], fst.Final(state));
      }
    }
    fst::ArcSort(split_fst, fst::ILabelCompare<Arc>());
  }
  return true;
}

}  // namespace internal

// Splits an SFst model into multiple parts by context patterns.
template <class Arc>
bool NGramSplit(const fst::Fst<Arc>& fst,
                absl::Span<const std::string> context_patterns,
                std::vector<fst::VectorFst<Arc>>* out_fsts,
                typename Arc::Label phi_label = 0,
                bool include_all_suffixes = false) {
  internal::SplitModelInfo<Arc> info;
  if (!internal::InitSplitModel(fst, phi_label, &info)) return false;
  std::vector<NGramContext> contexts;
  contexts.reserve(context_patterns.size());
  for (const auto& pattern : context_patterns) {
    contexts.emplace_back(pattern, info.hi_order);
  }
  return internal::CreateSplitFsts(fst, info, contexts, phi_label,
                                   include_all_suffixes, out_fsts);
}

// Splits an SFst model into multiple parts by context boundary vectors.
template <class Arc>
bool NGramSplit(const fst::Fst<Arc>& fst,
                absl::Span<const std::vector<typename Arc::Label>> boundaries,
                std::vector<fst::VectorFst<Arc>>* out_fsts,
                typename Arc::Label phi_label = 0,
                bool include_all_suffixes = false) {
  internal::SplitModelInfo<Arc> info;
  if (!internal::InitSplitModel(fst, phi_label, &info)) return false;
  std::vector<NGramContext> contexts;
  contexts.reserve(boundaries.size() > 1 ? boundaries.size() - 1 : 0);
  for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
    contexts.emplace_back(boundaries[i], boundaries[i + 1], info.hi_order);
  }
  return internal::CreateSplitFsts(fst, info, contexts, phi_label,
                                   include_all_suffixes, out_fsts);
}

}  // namespace sfst

#endif  // OPENGRM_SFST_NGRAM_CONTEXT_H_
