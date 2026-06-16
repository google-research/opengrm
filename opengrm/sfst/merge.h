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

template <class Arc>
typename Arc::Weight ScoreNGram(
    const fst::Fst<Arc>& fst,  // NOLINT(misc-include-cleaner)
    const std::vector<typename Arc::Label>& ngram) {
  using Label = typename Arc::Label;
  using Matcher = fst::ExplicitMatcher<fst::Matcher<fst::Fst<Arc>>>;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  if (ngram.empty()) return Weight::Zero();
  Matcher matcher(fst, fst::MATCH_INPUT);
  StateId curr = fst.Start();
  Weight total_weight = Weight::One();
  for (size_t i = 0; i < ngram.size() - 1; ++i) {
    Label w = ngram[i];
    matcher.SetState(curr);
    while (!matcher.Find(w)) {
      matcher.SetState(curr);
      if (matcher.Find(fst::kNoLabel)) {
        const auto& bo_arc = matcher.Value();
        total_weight = fst::Times(total_weight, bo_arc.weight);
        curr = bo_arc.nextstate;
        matcher.SetState(curr);
      } else {
        return Weight::Zero();
      }
    }
    const auto& arc = matcher.Value();
    curr = arc.nextstate;
  }
  Label w_k = ngram.back();
  matcher.SetState(curr);
  while (!matcher.Find(w_k)) {
    matcher.SetState(curr);
    if (matcher.Find(fst::kNoLabel)) {
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

template <class Arc>
void ExtractExplicitNGrams(const fst::Fst<Arc>& fst,
                           std::set<std::vector<typename Arc::Label>>& ngrams) {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId) return;
  std::map<StateId, std::vector<Label>> history;
  std::map<StateId, int> dist;
  std::queue<StateId> q;
  history[start_state] = std::vector<Label>();
  dist[start_state] = 0;
  q.push(start_state);
  while (!q.empty()) {
    StateId u = q.front();
    q.pop();
    int d = dist[u];
    const auto& H = history[u];
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, u); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == fst::kNoLabel) continue;
      std::vector<Label> next_ngram = H;
      next_ngram.push_back(arc.ilabel);
      ngrams.insert(next_ngram);
      StateId v = arc.nextstate;
      if (dist.find(v) == dist.end() || d + 1 < dist[v]) {
        dist[v] = d + 1;
        std::vector<Label> next_H = H;
        next_H.push_back(arc.ilabel);
        history[v] = next_H;
        q.push(v);
      }
    }
  }
}

template <class Arc>
void BuildCanonicalFst(
    const std::map<std::vector<typename Arc::Label>, double>& mixed_ngrams,
    int max_hist_len, const fst::SymbolTable* syms,
    fst::MutableFst<Arc>* fst) {  // NOLINT(misc-include-cleaner)
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
  history_to_state[std::vector<Label>()] = start_state;
  for (const auto& pair : mixed_ngrams) {
    const auto& ngram = pair.first;
    for (size_t len = 1; len < ngram.size(); ++len) {
      std::vector<Label> sub(ngram.end() - len, ngram.end());
      if (history_to_state.find(sub) == history_to_state.end()) {
        history_to_state[sub] = fst->AddState();
      }
    }
    std::vector<Label> src_hist(ngram.begin(), ngram.end() - 1);
    if (history_to_state.find(src_hist) == history_to_state.end()) {
      history_to_state[src_hist] = fst->AddState();
    }
    std::vector<Label> dst_hist =
        (ngram.size() > max_hist_len)
            ? std::vector<Label>(ngram.begin() + 1, ngram.end())
            : ngram;
    if (history_to_state.find(dst_hist) == history_to_state.end()) {
      history_to_state[dst_hist] = fst->AddState();
    }
  }
  for (const auto& pair : mixed_ngrams) {
    const auto& ngram = pair.first;
    double cost = pair.second;
    std::vector<Label> src_hist(ngram.begin(), ngram.end() - 1);
    std::vector<Label> dst_hist =
        (ngram.size() > max_hist_len)
            ? std::vector<Label>(ngram.begin() + 1, ngram.end())
            : ngram;
    Label word = ngram.back();
    StateId src = history_to_state[src_hist];
    StateId dst = history_to_state[dst_hist];
    fst->AddArc(src, Arc(word, word, Weight(-cost), dst));
  }
  for (const auto& pair : history_to_state) {
    const auto& H = pair.first;
    StateId src = pair.second;
    if (H.empty()) continue;
    std::vector<Label> bo_H(H.begin() + 1, H.end());
    StateId dst = history_to_state[bo_H];
    fst->AddArc(src, Arc(fst::kNoLabel, fst::kNoLabel, Weight::One(), dst));
  }
  fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  PhiNormalize(fst, fst::kNoLabel);
}

template <class Arc, typename WeightMixer>
bool MergeModels(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                 double alpha, double beta, fst::MutableFst<Arc>* out_fst,
                 WeightMixer weight_mixer) {
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  std::set<std::vector<Label>> all_ngrams;
  internal::ExtractExplicitNGrams(fst1, all_ngrams);
  internal::ExtractExplicitNGrams(fst2, all_ngrams);
  if (all_ngrams.empty()) return true;
  int max_order = 1;
  for (const auto& ngram : all_ngrams) {
    if (ngram.size() > max_order) max_order = ngram.size();
  }
  const fst::SymbolTable* syms =
      fst1.InputSymbols() ? fst1.InputSymbols() : fst2.InputSymbols();
  if (!syms) return false;
  const double neglog_alpha_global = -std::log(alpha);
  const double neglog_beta_global = -std::log(beta);
  std::map<std::vector<Label>, double> mixed_ngrams;
  for (const auto& ngram : all_ngrams) {
    Weight w1 = internal::ScoreNGram(fst1, ngram);
    Weight w2 = internal::ScoreNGram(fst2, ngram);
    double val1 =
        (w1 != Weight::Zero()) ? w1.Value() : fst::Log64Weight::Zero().Value();
    double val2 =
        (w2 != Weight::Zero()) ? w2.Value() : fst::Log64Weight::Zero().Value();
    double mixed_val = weight_mixer(ngram, val1, val2, neglog_alpha_global,
                                    neglog_beta_global, fst1, fst2);
    mixed_ngrams[ngram] = mixed_val;
  }
  BuildCanonicalFst(mixed_ngrams, max_order - 1, syms, out_fst);
  return true;
}

}  // namespace internal

template <class Arc>
struct LinearMixer {
  double operator()(const std::vector<typename Arc::Label>& ngram, double val1,
                    double val2, double neglog_a, double neglog_b,
                    const fst::Fst<Arc>& fst1,
                    const fst::Fst<Arc>& fst2) const {
    return NegLogSum(val1 + neglog_a, val2 + neglog_b);
  }
};

template <class Arc>
struct BayesMixer {
  double operator()(const std::vector<typename Arc::Label>& ngram, double val1,
                    double val2, double neglog_a_global, double neglog_b_global,
                    const fst::Fst<Arc>& fst1,
                    const fst::Fst<Arc>& fst2) const {
    using Label = typename Arc::Label;
    using Weight = typename Arc::Weight;
    std::vector<Label> hist(ngram.begin(), ngram.end() - 1);
    double alpha_h = neglog_a_global;
    double beta_h = neglog_b_global;
    if (!hist.empty()) {
      Weight h_w1 = internal::ScoreNGram(fst1, hist);
      Weight h_w2 = internal::ScoreNGram(fst2, hist);
      double h_val1 = (h_w1 != Weight::Zero())
                          ? h_w1.Value()
                          : fst::Log64Weight::Zero().Value();
      double h_val2 = (h_w2 != Weight::Zero())
                          ? h_w2.Value()
                          : fst::Log64Weight::Zero().Value();
      double numer1 = h_val1 + neglog_a_global;
      double numer2 = h_val2 + neglog_b_global;
      double denom = NegLogSum(numer1, numer2);
      if (denom < fst::Log64Weight::Zero().Value()) {
        alpha_h = numer1 - denom;
        beta_h = NegLogDiff(0.0, alpha_h);
      }
    }
    return NegLogSum(val1 + alpha_h, val2 + beta_h);
  }
};

template <class Arc>
bool LinearMerge(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                 double alpha, double beta, fst::MutableFst<Arc>* out_fst) {
  return internal::MergeModels(fst1, fst2, alpha, beta, out_fst,
                               LinearMixer<Arc>());
}

template <class Arc>
bool BayesMerge(const fst::Fst<Arc>& fst1, const fst::Fst<Arc>& fst2,
                double alpha, double beta, fst::MutableFst<Arc>* out_fst) {
  return internal::MergeModels(fst1, fst2, alpha, beta, out_fst,
                               BayesMixer<Arc>());
}

}  // namespace sfst

#endif  // OPENGRM_SFST_MERGE_H_
