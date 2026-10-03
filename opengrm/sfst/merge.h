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

#include <cmath>
#include <cstddef>
#include <map>
#include <queue>  // NOLINT(misc-include-cleaner)
#include <set>    // NOLINT(misc-include-cleaner)
#include <vector>

#include "absl/log/log.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {
namespace internal {

// Sentinel label representing BOS (beginning-of-sentence) history in FSTs
// where start_state backs off to a separate unigram_state.
constexpr int kBosSentinel = -2;

// Sentinel label representing EOS (end-of-sentence) final weight transitions.
constexpr int kEosSentinel = -3;

// Returns the unigram state (empty history state) of the FST if start_state
// has a backoff transition to a different state; otherwise returns kNoStateId.
template <class Arc>
typename Arc::StateId FindUnigramState(
    const fst::Fst<Arc>& fst, typename Arc::Label phi_label = fst::kNoLabel) {
  using StateId = typename Arc::StateId;
  StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId) return fst::kNoStateId;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, start_state); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label || arc.ilabel == fst::kNoLabel) {
      if (arc.nextstate != start_state) {
        return arc.nextstate;
      }
    }
  }
  return fst::kNoStateId;
}

// Computes the conditional cost (-log P(w_k | w_1 ... w_{k-1})) of the target
// transition w_k given history w_1 ... w_{k-1} in the given n-gram FST,
// backing off along phi_label (or kNoLabel) transitions as necessary.
template <class Arc>
typename Arc::Weight ScoreNGram(
    const fst::Fst<Arc>& fst,  // NOLINT(misc-include-cleaner)
    const std::vector<typename Arc::Label>& ngram,
    typename Arc::Label phi_label = fst::kNoLabel) {
  using Label = typename Arc::Label;
  using Matcher = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId || ngram.empty()) return Weight::Zero();
  StateId unigram_state = FindUnigramState(fst, phi_label);
  StateId empty_hist_state =
      (unigram_state != fst::kNoStateId) ? unigram_state : start_state;

  Matcher matcher(fst, fst::MATCH_INPUT);
  auto find_backoff = [&](StateId s) {
    matcher.SetState(s);
    if (matcher.Find(phi_label)) return true;
    if (phi_label != fst::kNoLabel && matcher.Find(fst::kNoLabel)) return true;
    return false;
  };
  StateId curr = empty_hist_state;
  for (size_t i = 0; i < ngram.size() - 1; ++i) {
    Label w = ngram[i];
    if (w == kBosSentinel) {
      if (unigram_state != fst::kNoStateId) {
        curr = start_state;
      } else if (fst.InputSymbols()) {
        Label bos = fst.InputSymbols()->Find("<s>");
        matcher.SetState(curr);
        if (bos != fst::kNoLabel && matcher.Find(bos)) {
          curr = matcher.Value().nextstate;
        }
      }
      continue;
    }
    matcher.SetState(curr);
    while (!matcher.Find(w)) {
      if (find_backoff(curr)) {
        curr = matcher.Value().nextstate;
        matcher.SetState(curr);
      } else {
        break;
      }
    }
    if (matcher.Find(w)) {
      curr = matcher.Value().nextstate;
    } else {
      curr = empty_hist_state;
    }
  }

  Label w_k = ngram.back();
  Weight total_weight = Weight::One();
  if (w_k == kEosSentinel) {
    while (fst.Final(curr) == Weight::Zero()) {
      if (find_backoff(curr)) {
        const auto& bo_arc = matcher.Value();
        total_weight = fst::Times(total_weight, bo_arc.weight);
        curr = bo_arc.nextstate;
      } else {
        return Weight::Zero();
      }
    }
    return fst::Times(total_weight, fst.Final(curr));
  }
  matcher.SetState(curr);
  while (!matcher.Find(w_k)) {
    if (find_backoff(curr)) {
      const auto& bo_arc = matcher.Value();
      total_weight = fst::Times(total_weight, bo_arc.weight);
      curr = bo_arc.nextstate;
      matcher.SetState(curr);
    } else {
      return Weight::Zero();
    }
  }
  const auto& arc = matcher.Value();
  return fst::Times(total_weight, arc.weight);
}

// Computes the joint history cost (-log P(w_1, ..., w_m)) of a history sequence
// in the given n-gram FST for Bayesian model interpolation.
template <class Arc>
typename Arc::Weight ScoreHistory(
    const fst::Fst<Arc>& fst, const std::vector<typename Arc::Label>& hist,
    typename Arc::Label phi_label = fst::kNoLabel) {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  if (hist.empty()) return Weight::One();
  Weight total = Weight::One();
  StateId unigram_state = FindUnigramState(fst, phi_label);
  size_t start_idx = 0;
  if (hist[0] == kBosSentinel) {
    if (unigram_state != fst::kNoStateId &&
        fst.Final(unigram_state) != Weight::Zero()) {
      total = fst.Final(unigram_state);
    }
    start_idx = 1;
  }
  for (size_t i = start_idx; i < hist.size(); ++i) {
    std::vector<typename Arc::Label> prefix(hist.begin(), hist.begin() + i + 1);
    Weight w = ScoreNGram(fst, prefix, phi_label);
    if (w == Weight::Zero()) return Weight::Zero();
    total = fst::Times(total, w);
  }
  return total;
}

// Extracts all explicitly represented n-grams (including EOS final weight
// transitions) from the given FST via breadth-first search.
template <class Arc>
void ExtractExplicitNGrams(const fst::Fst<Arc>& fst,
                           std::set<std::vector<typename Arc::Label>>& ngrams,
                           typename Arc::Label phi_label = fst::kNoLabel,
                           bool* has_bos_state = nullptr,
                           int* max_hist_len = nullptr) {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId) return;
  StateId unigram_state = FindUnigramState(fst, phi_label);
  std::map<StateId, std::vector<Label>> history;
  std::map<StateId, int> dist;
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
    const auto& H = history[u];
    if (max_hist_len && static_cast<int>(H.size()) > *max_hist_len) {
      *max_hist_len = H.size();
    }
    if (fst.Final(u) != Weight::Zero()) {
      std::vector<Label> eos_ngram = H;
      eos_ngram.push_back(kEosSentinel);
      ngrams.insert(eos_ngram);
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, u); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label || arc.ilabel == fst::kNoLabel) continue;
      std::vector<Label> next_ngram = H;
      next_ngram.push_back(arc.ilabel);
      ngrams.insert(next_ngram);
      StateId v = arc.nextstate;
      if (dist.find(v) == dist.end() || d + 1 < dist[v]) {
        dist[v] = d + 1;
        history[v] = next_ngram;
        q.push(v);
      }
    }
  }
}

// Constructs a canonical backoff n-gram FST from a map of mixed n-grams and
// their negative log probabilities, then normalizes all states and backoff
// weights using PhiNormalize.
template <class Arc>
void BuildCanonicalFst(
    const std::map<std::vector<typename Arc::Label>, double>& mixed_ngrams,
    int max_hist_len, const fst::SymbolTable* syms, fst::MutableFst<Arc>* fst,
    typename Arc::Label phi_label = fst::kNoLabel,
    bool has_bos_state = false) {  // NOLINT(misc-include-cleaner)
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  fst->DeleteStates();
  StateId start_state = fst->AddState();
  fst->SetStart(start_state);
  if (syms) {
    fst->SetInputSymbols(syms);
    fst->SetOutputSymbols(syms);
  }
  std::map<std::vector<Label>, StateId> history_to_state;
  if (has_bos_state) {
    StateId unigram_state = fst->AddState();
    history_to_state[std::vector<Label>()] = unigram_state;
    history_to_state[{kBosSentinel}] = start_state;
  } else {
    history_to_state[std::vector<Label>()] = start_state;
  }
  for (const auto& pair : mixed_ngrams) {
    const auto& ngram = pair.first;
    std::vector<Label> src_hist(ngram.begin(), ngram.end() - 1);
    if (history_to_state.find(src_hist) == history_to_state.end()) {
      history_to_state[src_hist] = fst->AddState();
    }
    for (size_t len = 1; len < src_hist.size(); ++len) {
      std::vector<Label> sub(src_hist.end() - len, src_hist.end());
      if (history_to_state.find(sub) == history_to_state.end()) {
        history_to_state[sub] = fst->AddState();
      }
    }
    if (ngram.back() != kEosSentinel) {
      std::vector<Label> dst_hist =
          (static_cast<int>(ngram.size()) > max_hist_len)
              ? std::vector<Label>(ngram.end() - max_hist_len, ngram.end())
              : ngram;
      if (history_to_state.find(dst_hist) == history_to_state.end()) {
        history_to_state[dst_hist] = fst->AddState();
      }
      for (size_t len = 1; len < dst_hist.size(); ++len) {
        std::vector<Label> sub(dst_hist.end() - len, dst_hist.end());
        if (history_to_state.find(sub) == history_to_state.end()) {
          history_to_state[sub] = fst->AddState();
        }
      }
    }
  }
  for (const auto& pair : mixed_ngrams) {
    const auto& ngram = pair.first;
    double cost = pair.second;
    std::vector<Label> src_hist(ngram.begin(), ngram.end() - 1);
    StateId src = history_to_state[src_hist];
    Label word = ngram.back();
    if (word == kEosSentinel) {
      fst->SetFinal(src, Weight(cost));
    } else {
      std::vector<Label> dst_hist =
          (static_cast<int>(ngram.size()) > max_hist_len)
              ? std::vector<Label>(ngram.end() - max_hist_len, ngram.end())
              : ngram;
      StateId dst = history_to_state[dst_hist];
      fst->AddArc(src, Arc(word, word, Weight(cost), dst));
    }
  }
  for (const auto& pair : history_to_state) {
    const auto& H = pair.first;
    StateId src = pair.second;
    if (H.empty()) continue;
    std::vector<Label> bo_H(H.begin() + 1, H.end());
    StateId dst = history_to_state[bo_H];
    fst->AddArc(src, Arc(phi_label, phi_label, Weight::One(), dst));
  }
  fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  PhiNormalize(fst, phi_label);
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
  std::set<std::vector<Label>> raw_ngrams;
  bool has_bos_state = false;
  int max_hist_len = 0;
  internal::ExtractExplicitNGrams(fst1, raw_ngrams, phi_label, &has_bos_state,
                                  &max_hist_len);
  internal::ExtractExplicitNGrams(fst2, raw_ngrams, phi_label, &has_bos_state,
                                  &max_hist_len);
  if (raw_ngrams.empty()) return true;
  std::set<std::vector<Label>> all_ngrams = raw_ngrams;
  for (const auto& ngram : raw_ngrams) {
    for (size_t i = 1; i < ngram.size(); ++i) {
      all_ngrams.insert(std::vector<Label>(ngram.begin() + i, ngram.end()));
    }
  }
  for (const auto& ngram : all_ngrams) {
    int hist_len = static_cast<int>(ngram.size()) - 1;
    if (hist_len > max_hist_len) max_hist_len = hist_len;
  }
  const fst::SymbolTable* syms =
      fst1.InputSymbols() ? fst1.InputSymbols() : fst2.InputSymbols();
  std::map<std::vector<Label>, double> mixed_ngrams;
  for (const auto& ngram : all_ngrams) {
    Weight w1 = internal::ScoreNGram(fst1, ngram, phi_label);
    Weight w2 = internal::ScoreNGram(fst2, ngram, phi_label);
    double val1 =
        (w1 != Weight::Zero()) ? w1.Value() : fst::Log64Weight::Zero().Value();
    double val2 =
        (w2 != Weight::Zero()) ? w2.Value() : fst::Log64Weight::Zero().Value();
    double mixed_val = weight_mixer(ngram, val1, val2);
    if (mixed_val < fst::Log64Weight::Zero().Value()) {
      mixed_ngrams[ngram] = mixed_val;
    }
  }
  BuildCanonicalFst(mixed_ngrams, max_hist_len, syms, out_fst, phi_label,
                    has_bos_state);
  return true;
}

}  // namespace internal

// Weight mixer functor for linear interpolation of two n-gram models with
// constant prior weights alpha and beta.
template <class Arc>
struct LinearMixer {
  LinearMixer(double alpha, double beta)
      : neglog_a_(-std::log(alpha)), neglog_b_(-std::log(beta)) {}

  double operator()(const std::vector<typename Arc::Label>& /*ngram*/,
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
  BayesMixer(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2, double alpha,
             double beta, typename Arc::Label phi_label = fst::kNoLabel)
      : fst1_(fst1),
        fst2_(fst2),
        neglog_a_(-std::log(alpha)),
        neglog_b_(-std::log(beta)),
        phi_label_(phi_label) {}

  double operator()(const std::vector<typename Arc::Label>& ngram, double val1,
                    double val2) const {
    using Label = typename Arc::Label;
    using Weight = typename Arc::Weight;
    std::vector<Label> hist(ngram.begin(), ngram.end() - 1);
    double alpha_h = neglog_a_;
    double beta_h = neglog_b_;
    if (!hist.empty()) {
      Weight h_w1 = internal::ScoreHistory(fst1_, hist, phi_label_);
      Weight h_w2 = internal::ScoreHistory(fst2_, hist, phi_label_);
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
    }
    return NegLogSum(val1 + alpha_h, val2 + beta_h);
  }

  const fst::Fst<Arc>& fst1_;
  const fst::Fst<Arc>& fst2_;
  const double neglog_a_;
  const double neglog_b_;
  const typename Arc::Label phi_label_;
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
