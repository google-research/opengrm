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

#ifndef OPENGRM_SFST_MERGE_H_
#define OPENGRM_SFST_MERGE_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {
namespace internal {

// Sentinel label representing BOS (beginning-of-sentence) history in FSTs
// where start_state backs off to a separate unigram_state.
constexpr int kBosSentinel = -2;

// Sentinel label representing EOS (end-of-sentence) final weight transitions.
constexpr int kEosSentinel = -3;

// Internal sentinel label used to normalize FSTs whose caller-visible backoff
// label is `fst::kNoLabel`.
constexpr int kInternalPhiSentinel = -4;

// Transparent hash and equality functors for looking up `std::vector<Label>`
// keys via `absl::Span<const Label>` without allocating temporary vectors.
template <typename Label>
struct LabelVecHash {
  using is_transparent = void;

  size_t operator()(absl::Span<const Label> v) const { return absl::HashOf(v); }
};

template <typename Label>
struct LabelVecEq {
  using is_transparent = void;

  bool operator()(absl::Span<const Label> a, absl::Span<const Label> b) const {
    return a == b;
  }
};

template <typename Label>
using NGramSet = absl::flat_hash_set<std::vector<Label>, LabelVecHash<Label>,
                                     LabelVecEq<Label>>;

template <typename Label, typename V>
using NGramMap = absl::flat_hash_map<std::vector<Label>, V, LabelVecHash<Label>,
                                     LabelVecEq<Label>>;

// Stateful n-gram scorer that reuses a single `ExplicitMatcher` and caches the
// most recently resolved conditioning history state across consecutive queries.
template <class Arc>
class NGramScorer {
 public:
  using Label = typename Arc::Label;
  using Matcher = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  explicit NGramScorer(const fst::Fst<Arc>& fst,
                       Label phi_label = fst::kNoLabel)
      : fst_(fst),
        phi_label_(phi_label),
        start_state_(fst.Start()),
        unigram_state_(start_state_ != fst::kNoStateId
                           ? FindUnigramState(fst, phi_label)
                           : fst::kNoStateId),
        empty_hist_state_((unigram_state_ != fst::kNoStateId) ? unigram_state_
                                                              : start_state_),
        matcher_(&fst, fst::MATCH_INPUT),
        is_ilabel_sorted_(fst.Properties(fst::kILabelSorted, true)) {}

  // Locates the conditioning state in `fst_` corresponding to the longest
  // suffix of the history prefix `ngram[0 .. ngram.size() - 2]`.
  StateId FindHistoryState(absl::Span<const Label> ngram) {
    if (start_state_ == fst::kNoStateId || ngram.empty())
      return fst::kNoStateId;
    absl::Span<const Label> hist = ngram.first(ngram.size() - 1);
    if (has_cached_hist_ && absl::MakeConstSpan(cached_hist_) == hist) {
      return cached_state_;
    }
    StateId curr = empty_hist_state_;
    for (Label w : hist) {
      if (w == kBosSentinel) {
        curr = start_state_;
        continue;
      }
      matcher_.SetState(curr);
      bool found = matcher_.Find(w);
      Arc bo_arc;
      while (!found && FindBackoff(curr, &bo_arc)) {
        curr = bo_arc.nextstate;
        matcher_.SetState(curr);
        found = matcher_.Find(w);
      }
      curr = found ? matcher_.Value().nextstate : empty_hist_state_;
    }
    cached_hist_.assign(hist.begin(), hist.end());
    cached_state_ = curr;
    has_cached_hist_ = true;
    return curr;
  }

  // Computes the conditional cost (-log P(w_k | w_1 ... w_{k-1})) of the target
  // transition w_k given history w_1 ... w_{k-1} in `fst_`, backing off along
  // `phi_label_` transitions from the conditioning history state as necessary.
  Weight ScoreNGram(absl::Span<const Label> ngram) {
    StateId curr = FindHistoryState(ngram);
    if (curr == fst::kNoStateId) return Weight::Zero();
    Label w_k = ngram.back();
    Weight backoff_weight = Weight::One();
    Arc bo_arc;
    if (w_k == kEosSentinel) {
      while (fst_.Final(curr) == Weight::Zero()) {
        if (!FindBackoff(curr, &bo_arc)) return Weight::Zero();
        backoff_weight = fst::Times(backoff_weight, bo_arc.weight);
        curr = bo_arc.nextstate;
      }
      return fst::Times(backoff_weight, fst_.Final(curr));
    }
    matcher_.SetState(curr);
    while (!matcher_.Find(w_k)) {
      if (!FindBackoff(curr, &bo_arc)) return Weight::Zero();
      backoff_weight = fst::Times(backoff_weight, bo_arc.weight);
      curr = bo_arc.nextstate;
      matcher_.SetState(curr);
    }
    return fst::Times(backoff_weight, matcher_.Value().weight);
  }

  // Computes the joint history cost (-log P(w_1, ..., w_m)) of a history
  // sequence in `fst_` for Bayesian model interpolation.
  Weight ScoreHistory(absl::Span<const Label> hist) {
    if (hist.empty()) return Weight::One();
    Weight total = Weight::One();
    size_t start_idx = 0;
    if (hist[0] == kBosSentinel) {
      if (unigram_state_ != fst::kNoStateId &&
          fst_.Final(unigram_state_) != Weight::Zero()) {
        total = fst_.Final(unigram_state_);
      }
      start_idx = 1;
    }
    for (size_t i = start_idx; i < hist.size(); ++i) {
      Weight w = ScoreNGram(hist.first(i + 1));
      if (w == Weight::Zero()) return Weight::Zero();
      total = fst::Times(total, w);
    }
    return total;
  }

 private:
  bool FindBackoff(StateId s, Arc* bo_arc) {
    if (phi_label_ != fst::kNoLabel) {
      matcher_.SetState(s);
      if (matcher_.Find(phi_label_)) {
        *bo_arc = matcher_.Value();
        return true;
      }
      return false;
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst_, s); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      if (arc.ilabel == fst::kNoLabel) {
        *bo_arc = arc;
        return true;
      }
      if (is_ilabel_sorted_ && arc.ilabel > fst::kNoLabel) break;
    }
    return false;
  }

  const fst::Fst<Arc>& fst_;
  const Label phi_label_;
  const StateId start_state_;
  const StateId unigram_state_;
  const StateId empty_hist_state_;
  Matcher matcher_;
  const bool is_ilabel_sorted_;
  std::vector<Label> cached_hist_;
  StateId cached_state_ = fst::kNoStateId;
  bool has_cached_hist_ = false;
};

// Convenience wrapper that scores a single n-gram in `fst`.
template <class Arc>
typename Arc::Weight ScoreNGram(const fst::Fst<Arc>& fst,
                                absl::Span<const typename Arc::Label> ngram,
                                typename Arc::Label phi_label = fst::kNoLabel) {
  return NGramScorer<Arc>(fst, phi_label).ScoreNGram(ngram);
}

// Extracts all explicitly represented n-grams (and their suffixes, including
// EOS final weight transitions) from the given FST via breadth-first search.
template <class Arc>
void ExtractExplicitNGrams(const fst::Fst<Arc>& fst,
                           NGramSet<typename Arc::Label>& ngrams,
                           typename Arc::Label phi_label = fst::kNoLabel,
                           bool* has_bos_state = nullptr,
                           int* max_hist_len = nullptr) {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId) return;
  auto insert_with_suffixes = [&](std::vector<Label> ngram) {
    absl::Span<const Label> span(ngram);
    for (size_t i = 1; i < span.size(); ++i) {
      absl::Span<const Label> suffix = span.subspan(i);
      if (ngrams.contains(suffix)) break;
      ngrams.emplace(suffix.begin(), suffix.end());
    }
    ngrams.insert(std::move(ngram));
  };
  StateId unigram_state = FindUnigramState(fst, phi_label);
  absl::flat_hash_map<StateId, std::vector<Label>> history;
  absl::flat_hash_map<StateId, int> dist;
  std::queue<StateId> q;
  if (unigram_state != fst::kNoStateId) {
    if (has_bos_state) *has_bos_state = true;
    history[unigram_state] = std::vector<Label>();
    dist[unigram_state] = 0;
    q.push(unigram_state);
    history[start_state] = {kBosSentinel};
    dist[start_state] = 1;
    q.push(start_state);
  } else {
    history[start_state] = std::vector<Label>();
    dist[start_state] = 0;
    q.push(start_state);
  }
  while (!q.empty()) {
    StateId u = q.front();
    q.pop();
    int d = dist[u];
    const std::vector<Label> H = history[u];
    if (max_hist_len && static_cast<int>(H.size()) > *max_hist_len) {
      *max_hist_len = H.size();
    }
    if (fst.Final(u) != Weight::Zero()) {
      std::vector<Label> eos_ngram = H;
      eos_ngram.push_back(kEosSentinel);
      insert_with_suffixes(std::move(eos_ngram));
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, u); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label || arc.ilabel < 0) continue;
      std::vector<Label> next_ngram = H;
      next_ngram.push_back(arc.ilabel);
      insert_with_suffixes(next_ngram);
      StateId v = arc.nextstate;
      auto [dist_it, inserted] = dist.try_emplace(v, d + 1);
      if (inserted || d + 1 < dist_it->second) {
        dist_it->second = d + 1;
        history[v] = std::move(next_ngram);
        q.push(v);
      }
    }
  }
}

// Constructs a canonical backoff n-gram FST from a sequence of mixed n-grams
// and their negative log probabilities, then normalizes all states and backoff
// weights using PhiNormalize.
template <class Arc>
bool BuildCanonicalFst(
    const std::vector<std::pair<std::vector<typename Arc::Label>, double>>&
        mixed_ngrams,
    int max_hist_len, const fst::SymbolTable* syms, fst::MutableFst<Arc>* fst,
    typename Arc::Label phi_label = fst::kNoLabel, bool has_bos_state = false) {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  fst->DeleteStates();
  StateId start_state = fst->AddState();
  fst->SetStart(start_state);
  if (fst->InputSymbols() != syms) fst->SetInputSymbols(syms);
  if (fst->OutputSymbols() != syms) fst->SetOutputSymbols(syms);
  NGramMap<Label, StateId> history_to_state;
  std::vector<std::pair<std::vector<Label>, StateId>> state_histories;
  auto get_or_add_state = [&](absl::Span<const Label> hist) -> StateId {
    auto it = history_to_state.find(hist);
    if (it != history_to_state.end()) return it->second;
    StateId s = fst->AddState();
    std::vector<Label> hist_vec(hist.begin(), hist.end());
    history_to_state.emplace(hist_vec, s);
    state_histories.emplace_back(std::move(hist_vec), s);
    return s;
  };
  if (has_bos_state) {
    StateId unigram_state = fst->AddState();
    history_to_state.emplace(std::vector<Label>(), unigram_state);
    state_histories.emplace_back(std::vector<Label>(), unigram_state);
    history_to_state.emplace(std::vector<Label>{kBosSentinel}, start_state);
    state_histories.emplace_back(std::vector<Label>{kBosSentinel}, start_state);
  } else {
    history_to_state.emplace(std::vector<Label>(), start_state);
    state_histories.emplace_back(std::vector<Label>(), start_state);
  }
  for (const auto& [ngram, cost] : mixed_ngrams) {
    (void)cost;
    absl::Span<const Label> src_hist =
        absl::MakeConstSpan(ngram).first(ngram.size() - 1);
    get_or_add_state(src_hist);
    for (size_t len = 1; len < src_hist.size(); ++len) {
      get_or_add_state(src_hist.last(len));
    }
  }
  for (const auto& [ngram, cost] : mixed_ngrams) {
    absl::Span<const Label> ngram_span(ngram);
    absl::Span<const Label> src_hist = ngram_span.first(ngram_span.size() - 1);
    auto src_it = history_to_state.find(src_hist);
    CHECK(src_it != history_to_state.end());
    StateId src = src_it->second;
    Label word = ngram_span.back();
    if (word == kEosSentinel) {
      fst->SetFinal(src, Weight(cost));
    } else {
      absl::Span<const Label> dst_hist =
          (static_cast<int>(ngram_span.size()) > max_hist_len)
              ? ngram_span.last(max_hist_len)
              : ngram_span;
      auto dst_it = history_to_state.find(dst_hist);
      while (dst_it == history_to_state.end()) {
        dst_hist.remove_prefix(1);
        dst_it = history_to_state.find(dst_hist);
      }
      StateId dst = dst_it->second;
      fst->AddArc(src, Arc(word, word, Weight(cost), dst));
    }
  }
  const Label norm_phi_label =
      (phi_label == fst::kNoLabel) ? kInternalPhiSentinel : phi_label;
  for (const auto& [H, src] : state_histories) {
    if (H.empty()) continue;
    absl::Span<const Label> bo_H = absl::MakeConstSpan(H).subspan(1);
    auto bo_it = history_to_state.find(bo_H);
    CHECK(bo_it != history_to_state.end());
    StateId dst = bo_it->second;
    fst->AddArc(src, Arc(norm_phi_label, norm_phi_label, Weight::One(), dst));
  }
  fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  if (!PhiNormalize(fst, norm_phi_label)) return false;
  if (phi_label == fst::kNoLabel) {
    for (fst::StateIterator<fst::MutableFst<Arc>> siter(*fst); !siter.Done();
         siter.Next()) {
      for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst,
                                                               siter.Value());
           !aiter.Done(); aiter.Next()) {
        Arc arc = aiter.Value();
        if (arc.ilabel == norm_phi_label) {
          arc.ilabel = fst::kNoLabel;
          arc.olabel = fst::kNoLabel;
          aiter.SetValue(arc);
        }
      }
    }
  }
  fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  return true;
}

// Merges two n-gram FSTs using the provided WeightMixer functor.
template <class Arc, typename WeightMixer>
bool MergeModels(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                 fst::MutableFst<Arc>* out_fst, WeightMixer weight_mixer,
                 typename Arc::Label phi_label = fst::kNoLabel) {
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  if (!fst::CompatSymbols(fst1.InputSymbols(), fst2.InputSymbols(),
                          /*warning=*/false)) {
    LOG(ERROR) << "MergeModels: Symbol tables of input models do not match";
    return false;
  }
  const fst::SymbolTable* syms =
      fst1.InputSymbols() ? fst1.InputSymbols() : fst2.InputSymbols();
  NGramSet<Label> all_ngrams;
  bool has_bos_state = false;
  int max_hist_len = 0;
  internal::ExtractExplicitNGrams(fst1, all_ngrams, phi_label, &has_bos_state,
                                  &max_hist_len);
  internal::ExtractExplicitNGrams(fst2, all_ngrams, phi_label, &has_bos_state,
                                  &max_hist_len);
  if (all_ngrams.empty()) {
    out_fst->DeleteStates();
    if (out_fst->InputSymbols() != syms) out_fst->SetInputSymbols(syms);
    if (out_fst->OutputSymbols() != syms) out_fst->SetOutputSymbols(syms);
    return true;
  }
  std::vector<std::vector<Label>> sorted_ngrams;
  sorted_ngrams.reserve(all_ngrams.size());
  for (auto it = all_ngrams.begin(); it != all_ngrams.end();) {
    std::vector<Label> ngram = std::move(all_ngrams.extract(it++).value());
    int hist_len = static_cast<int>(ngram.size()) - 1;
    if (hist_len > max_hist_len) max_hist_len = hist_len;
    sorted_ngrams.push_back(std::move(ngram));
  }
  // Sorts n-grams by order (length) and then lexicographically so that all
  // n-grams sharing the same (k - 1)-word history are contiguous and
  // `NGramScorer` resolves each history state once.
  std::sort(sorted_ngrams.begin(), sorted_ngrams.end(),
            [](const std::vector<Label>& a, const std::vector<Label>& b) {
              if (a.size() != b.size()) return a.size() < b.size();
              return a < b;
            });
  internal::NGramScorer<Arc> scorer1(fst1, phi_label);
  internal::NGramScorer<Arc> scorer2(fst2, phi_label);
  std::vector<std::pair<std::vector<Label>, double>> mixed_ngrams;
  mixed_ngrams.reserve(sorted_ngrams.size());
  for (auto& ngram : sorted_ngrams) {
    Weight w1 = scorer1.ScoreNGram(ngram);
    Weight w2 = scorer2.ScoreNGram(ngram);
    double val1 =
        (w1 != Weight::Zero()) ? w1.Value() : fst::Log64Weight::Zero().Value();
    double val2 =
        (w2 != Weight::Zero()) ? w2.Value() : fst::Log64Weight::Zero().Value();
    double mixed_val = weight_mixer(ngram, val1, val2);
    if (mixed_val < fst::Log64Weight::Zero().Value()) {
      mixed_ngrams.emplace_back(std::move(ngram), mixed_val);
    }
  }
  return BuildCanonicalFst(mixed_ngrams, max_hist_len, syms, out_fst, phi_label,
                           has_bos_state);
}

}  // namespace internal

// Weight mixer functor for linear interpolation of two n-gram models with
// constant prior weights alpha and beta.
template <class Arc>
struct LinearMixer {
  LinearMixer(double alpha, double beta)
      : neglog_a_(-std::log(alpha)), neglog_b_(-std::log(beta)) {}

  double operator()(absl::Span<const typename Arc::Label> /*ngram*/,
                    double val1, double val2) const {
    return NegLogSum(val1 + neglog_a_, val2 + neglog_b_);
  }

  const double neglog_a_;
  const double neglog_b_;
};

// Weight mixer functor for Bayesian interpolation of two n-gram models, where
// mixing weights are dynamically conditioned on the posterior probability of
// each n-gram's history state.
template <class Arc>
struct BayesMixer {
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  BayesMixer(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, double alpha,
             double beta, Label phi_label = fst::kNoLabel)
      : scorer1_(fst1, phi_label),
        scorer2_(fst2, phi_label),
        neglog_a_(-std::log(alpha)),
        neglog_b_(-std::log(beta)) {}

  double operator()(absl::Span<const Label> ngram, double val1, double val2) {
    absl::Span<const Label> hist = ngram.first(ngram.size() - 1);
    double alpha_h = neglog_a_;
    double beta_h = neglog_b_;
    if (!hist.empty()) {
      if (has_cached_hist_ && absl::MakeConstSpan(cached_hist_) == hist) {
        return NegLogSum(val1 + cached_alpha_h_, val2 + cached_beta_h_);
      }
      Weight h_w1 = scorer1_.ScoreHistory(hist);
      Weight h_w2 = scorer2_.ScoreHistory(hist);
      double h_val1 = (h_w1 != Weight::Zero())
                          ? h_w1.Value()
                          : fst::Log64Weight::Zero().Value();
      double h_val2 = (h_w2 != Weight::Zero())
                          ? h_w2.Value()
                          : fst::Log64Weight::Zero().Value();
      double numer1 = h_val1 + neglog_a_;
      double numer2 = h_val2 + neglog_b_;
      double denom = NegLogSum(numer1, numer2);
      if (denom < fst::Log64Weight::Zero().Value()) {
        alpha_h = numer1 - denom;
        beta_h = NegLogDiff(0.0, alpha_h);
      }
      cached_hist_.assign(hist.begin(), hist.end());
      cached_alpha_h_ = alpha_h;
      cached_beta_h_ = beta_h;
      has_cached_hist_ = true;
    }
    return NegLogSum(val1 + alpha_h, val2 + beta_h);
  }

  internal::NGramScorer<Arc> scorer1_;
  internal::NGramScorer<Arc> scorer2_;
  const double neglog_a_;
  const double neglog_b_;
  std::vector<Label> cached_hist_;
  double cached_alpha_h_ = 0.0;
  double cached_beta_h_ = 0.0;
  bool has_cached_hist_ = false;
};

// Performs linear interpolation of two n-gram FSTs with mixing weights alpha
// and beta, writing the normalized result to out_fst.
template <class Arc>
bool LinearMerge(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                 double alpha, double beta, fst::MutableFst<Arc>* out_fst,
                 typename Arc::Label phi_label = fst::kNoLabel) {
  return internal::MergeModels(fst1, fst2, out_fst,
                               LinearMixer<Arc>(alpha, beta), phi_label);
}

// Performs Bayesian interpolation of two n-gram FSTs with prior weights alpha
// and beta, writing the normalized result to out_fst.
template <class Arc>
bool BayesMerge(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                double alpha, double beta, fst::MutableFst<Arc>* out_fst,
                typename Arc::Label phi_label = fst::kNoLabel) {
  return internal::MergeModels(
      fst1, fst2, out_fst, BayesMixer<Arc>(fst1, fst2, alpha, beta, phi_label),
      phi_label);
}

}  // namespace sfst

#endif  // OPENGRM_SFST_MERGE_H_
