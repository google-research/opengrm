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
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"

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

}  // namespace sfst

#endif  // OPENGRM_SFST_NGRAM_CONTEXT_H_
