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

#include "opengrm/sfst/ngram-context.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"

namespace sfst {
namespace {

using Label = fst::StdArc::Label;

// Helper class for reverse-padded vector iteration and comparison.
class ReversedPaddedVector {
 public:
  class Iterator {
   public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using pointer = const Label*;
    using reference = const Label&;
    using value_type = Label;

    explicit Iterator(const std::vector<Label>* v, size_t pos)
        : v_(v), pos_(pos) {}

    Iterator(const Iterator&) = default;
    Iterator& operator=(const Iterator&) = default;

    Label operator*() const {
      return (pos_ < v_->size()) ? (*v_)[v_->size() - 1 - pos_] : 0;
    }

    Iterator& operator++() {
      ++pos_;
      return *this;
    }

    Iterator operator++(int) {
      Iterator ret = *this;
      ++pos_;
      return ret;
    }

    bool operator==(const Iterator& other) const {
      return pos_ == other.pos_ && v_ == other.v_;
    }

    bool operator!=(const Iterator& other) const { return !(*this == other); }

   private:
    const std::vector<Label>* v_ = nullptr;
    size_t pos_ = 0;
  };

  ReversedPaddedVector(const std::vector<Label>& v, size_t len)
      : v_(v), len_(std::max(v.size(), len)) {}

  Iterator begin() const { return Iterator(&v_, 0); }

  Iterator end() const { return Iterator(&v_, len_); }

 private:
  const std::vector<Label>& v_;
  const size_t len_;
};

}  // namespace

NGramContext::NGramContext(std::vector<Label> context_begin,
                           std::vector<Label> context_end, int hi_order)
    : hi_order_(hi_order),
      context_begin_(std::move(context_begin)),
      context_end_(std::move(context_end)) {
  Init();
}

NGramContext::NGramContext(absl::string_view context_pattern, int hi_order)
    : hi_order_(hi_order) {
  ParseContextInterval(context_pattern, &context_begin_, &context_end_);
  Init();
}

bool NGramContext::HasContext(const std::vector<Label>& ngram,
                              bool include_all_suffixes) const {
  if (NullContext()) return true;
  ReversedPaddedVector ngram_for_cmp(ngram, hi_order_ - 1);
  auto context_begin_end = include_all_suffixes
                               ? context_begin_.begin() + ngram.size()
                               : context_begin_.end();
  bool less_begin =
      std::lexicographical_compare(ngram_for_cmp.begin(), ngram_for_cmp.end(),
                                   context_begin_.begin(), context_begin_end);
  bool less_end =
      std::lexicographical_compare(ngram_for_cmp.begin(), ngram_for_cmp.end(),
                                   context_end_.begin(), context_end_.end());
  return !less_begin && less_end;
}

void NGramContext::ParseContextInterval(absl::string_view context_pattern,
                                        std::vector<Label>* context_begin,
                                        std::vector<Label>* context_end) {
  context_begin->clear();
  context_end->clear();
  if (context_pattern.empty()) return;
  std::vector<absl::string_view> contexts =
      absl::StrSplit(context_pattern, ':', absl::SkipEmpty());
  if (contexts.size() != 2) return;
  std::vector<absl::string_view> labels1 =
      absl::StrSplit(contexts[0], ' ', absl::SkipEmpty());
  std::vector<absl::string_view> labels2 =
      absl::StrSplit(contexts[1], ' ', absl::SkipEmpty());
  for (const auto& l : labels1) {
    Label label;
    if (absl::SimpleAtoi(l, &label)) context_begin->push_back(label);
  }
  for (const auto& l : labels2) {
    Label label;
    if (absl::SimpleAtoi(l, &label)) context_end->push_back(label);
  }
}

std::string NGramContext::GetContextString(
    absl::Span<const Label> context_begin,
    absl::Span<const Label> context_end) {
  std::string str;
  for (size_t i = 0; i < context_begin.size(); ++i) {
    absl::StrAppend(&str, context_begin[i], " ");
  }
  absl::StrAppend(&str, ":");
  for (size_t i = 0; i < context_end.size(); ++i) {
    absl::StrAppend(&str, " ", context_end[i]);
  }
  return str;
}

std::vector<NGramContext::Label> NGramContext::GetContextBegin() const {
  std::vector<Label> ngram(context_begin_);
  while (ngram.size() > 1 && ngram.back() == 0) ngram.pop_back();
  std::reverse(ngram.begin(), ngram.end());
  return ngram;
}

std::vector<NGramContext::Label> NGramContext::GetContextEnd() const {
  std::vector<Label> ngram(context_end_);
  while (ngram.size() > 1 && ngram.back() == 0) ngram.pop_back();
  std::reverse(ngram.begin(), ngram.end());
  return ngram;
}

void NGramContext::SetHiOrder(int hi_order) {
  if (hi_order > hi_order_) {
    if (!NullContext()) {
      context_begin_.resize(hi_order - 1, 0);
      context_end_.resize(hi_order - 1, 0);
    }
    hi_order_ = hi_order;
  }
}

void NGramContext::Init() {
  if (NullContext()) return;
  std::reverse(context_begin_.begin(), context_begin_.end());
  std::reverse(context_end_.begin(), context_end_.end());
  if (context_begin_.size() >= hi_order_) {
    hi_order_ = context_begin_.size() + 1;
  }
  if (context_end_.size() >= hi_order_) {
    hi_order_ = context_end_.size() + 1;
  }
  context_begin_.resize(hi_order_ - 1, 0);
  context_end_.resize(hi_order_ - 1, 0);
}

NGramExtendedContext::NGramExtendedContext(
    const std::vector<Label>& context_begin,
    const std::vector<Label>& context_end, int hi_order) {
  contexts_.push_back(NGramContext(context_begin, context_end, hi_order));
  Init(/*merge_contexts=*/false);
}

NGramExtendedContext::NGramExtendedContext(
    absl::string_view extended_context_pattern, int hi_order,
    bool merge_contexts) {
  ParseContextIntervals(extended_context_pattern, hi_order, &contexts_);
  Init(merge_contexts);
}

NGramExtendedContext::NGramExtendedContext(
    const std::vector<NGramContext>& contexts, bool merge_contexts)
    : contexts_(contexts) {
  Init(merge_contexts);
}

bool NGramExtendedContext::HasContext(const std::vector<Label>& ngram,
                                      bool include_all_suffixes) const {
  if (contexts_.empty()) return true;
  return GetContext(ngram, include_all_suffixes) != nullptr;
}

const NGramContext* NGramExtendedContext::GetContext(
    const std::vector<Label>& ngram, bool include_all_suffixes) const {
  if (contexts_.empty()) return nullptr;
  std::vector<Label> ngram_end(ngram);
  if (ngram_end.empty()) ngram_end.push_back(0);
  std::vector<Label> ngram_beg(ngram_end);
  ++ngram_end[0];  // ensures non-empty interval below
  int hi_order = contexts_[0].GetHiOrder();
  NGramContext ngram_context(std::move(ngram_beg), std::move(ngram_end),
                             hi_order);
  ContextCompare context_cmp;
  auto it = std::upper_bound(contexts_.begin(), contexts_.end(), ngram_context,
                             context_cmp);
  if (it != contexts_.begin() &&
      (it - 1)->HasContext(ngram, include_all_suffixes)) {
    return &*(it - 1);  // strict match
  } else if (include_all_suffixes && it != contexts_.end() &&
             it->HasContext(ngram, include_all_suffixes)) {
    return &*it;  // suffix match
  } else {
    return nullptr;  // no match (or null context)
  }
}

void NGramExtendedContext::ParseContextIntervals(
    absl::string_view extended_context_pattern, int hi_order,
    std::vector<NGramContext>* contexts) {
  contexts->clear();
  std::vector<absl::string_view> context_patterns =
      absl::StrSplit(extended_context_pattern, ',', absl::SkipEmpty());
  for (const auto& context_pattern : context_patterns) {
    contexts->push_back(NGramContext(context_pattern, hi_order));
  }
}

std::string NGramExtendedContext::GetExtendedContextString(
    absl::Span<const NGramContext> contexts) {
  std::string str;
  for (size_t i = 0; i < contexts.size(); ++i) {
    if (i > 0) absl::StrAppend(&str, ",");
    absl::StrAppend(
        &str, NGramContext::GetContextString(contexts[i].GetContextBegin(),
                                             contexts[i].GetContextEnd()));
  }
  return str;
}

void NGramExtendedContext::Init(bool merge_contexts) {
  ContextCompare context_cmp;
  std::sort(contexts_.begin(), contexts_.end(), context_cmp);
  if (contexts_.empty()) return;
  if (contexts_.size() == 1 && contexts_[0].NullContext()) {
    contexts_.pop_back();
    return;
  }
  int hi_order = 0;
  for (const auto& context : contexts_) {
    if (context.GetHiOrder() > hi_order) {
      hi_order = context.GetHiOrder();
    }
  }
  for (auto& context : contexts_) {
    context.SetHiOrder(hi_order);
  }
  if (!CheckContexts()) return;
  if (merge_contexts) {
    size_t i = 0;
    size_t j = 1;
    size_t k = 0;
    for (; j < contexts_.size(); ++j) {
      const auto& e1 = contexts_[j - 1].GetReverseContextEnd();
      const auto& b2 = contexts_[j].GetReverseContextBegin();
      if (e1 != b2) {
        MergeContexts(i, j - 1, k++);
        i = j;
      }
    }
    MergeContexts(i, j - 1, k++);
    contexts_.resize(k);
  }
}

bool NGramExtendedContext::CheckContexts() const {
  for (size_t i = 0; i < contexts_.size(); ++i) {
    if (contexts_[i].NullContext()) return false;
  }
  for (size_t i = 1; i < contexts_.size(); ++i) {
    const auto& e1 = contexts_[i - 1].GetReverseContextEnd();
    const auto& b2 = contexts_[i].GetReverseContextBegin();
    if (std::lexicographical_compare(b2.begin(), b2.end(), e1.begin(),
                                     e1.end())) {
      return false;
    }
  }
  return true;
}

void NGramExtendedContext::MergeContexts(size_t i, size_t j, size_t k) {
  if (i != j) {
    contexts_[k] =
        NGramContext(contexts_[i].GetContextBegin(),
                     contexts_[j].GetContextEnd(), contexts_[0].GetHiOrder());
  } else if (i != k) {
    contexts_[k] = contexts_[i];
  }
}

}  // namespace sfst
