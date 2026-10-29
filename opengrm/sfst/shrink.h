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

// Algorithms for shrinking or pruning SFST models.

#ifndef OPENGRM_SFST_SHRINK_H_
#define OPENGRM_SFST_SHRINK_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {
namespace internal {

template <class Arc>
inline typename Arc::StateId GetBackoffState(const fst::Fst<Arc>& fst,
                                             typename Arc::StateId state,
                                             typename Arc::Label phi_label,
                                             size_t* bo_pos = nullptr) {
  if (state == fst::kNoStateId) return fst::kNoStateId;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, state); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == phi_label) {
      if (bo_pos != nullptr) *bo_pos = aiter.Position();
      return aiter.Value().nextstate;
    }
  }
  return fst::kNoStateId;
}

template <class Arc, class Matcher>
inline bool ComputeStateAndBackoffSums(
    const fst::Fst<Arc>& fst, typename Arc::StateId state,
    typename Arc::Label phi_label, Matcher& matcher, typename Arc::StateId* bo,
    double* hi_neglog_sum, double* low_neglog_sum) {
  *bo = GetBackoffState(fst, state, phi_label);
  if (*bo == fst::kNoStateId) return false;
  *hi_neglog_sum = fst.Final(state).Value();
  *low_neglog_sum = (fst.Final(state) != Arc::Weight::Zero())
                        ? fst.Final(*bo).Value()
                        : Arc::Weight::Zero().Value();
  matcher.SetState(*bo);
  double kahan_val1 = 0.0;
  double kahan_val2 = 0.0;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, state); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label) continue;
    *hi_neglog_sum = NegLogSum(*hi_neglog_sum, arc.weight.Value(), &kahan_val1);
    if (matcher.Find(arc.ilabel)) {
      const auto& barc = matcher.Value();
      *low_neglog_sum =
          NegLogSum(*low_neglog_sum, barc.weight.Value(), &kahan_val2);
    }
  }
  return true;
}

inline void ComputeBackoffNormalizers(double hi_neglog_sum,
                                      double low_neglog_sum, double* num,
                                      double* denom) {
  constexpr double kEffectiveZero = kNormEps * kFloatEps;
  constexpr double kEffectiveNlogZero = 99.0;
  const double tmp_hi = std::max(hi_neglog_sum, kEffectiveZero);
  const double tmp_low = std::max(low_neglog_sum, kEffectiveZero);
  *num = 0.0;
  *denom = 0.0;
  if (tmp_low > 0.0 && tmp_hi > 0.0) {
    *num = (tmp_hi > kEffectiveNlogZero) ? 0.0 : NegLogDiff(0.0, tmp_hi);
    *denom = (tmp_low > kEffectiveNlogZero) ? 0.0 : NegLogDiff(0.0, tmp_low);
  }
}

inline double ComputeUpdatedLogBackoff(double nlog_backoff_num,
                                       double nlog_backoff_denom,
                                       double arc_neglog_weight,
                                       double barc_neglog_weight) {
  return NegLogSum(nlog_backoff_denom, barc_neglog_weight) -
         NegLogSum(nlog_backoff_num, arc_neglog_weight);
}

inline double ComputeStolckeScore(double log_prob_s, double nlog_backoff_num,
                                  double nlog_backoff_denom,
                                  double arc_weight_val,
                                  double barc_weight_val) {
  const double log_prob = -arc_weight_val;
  const double log_backoff_prob = -barc_weight_val;
  const double new_log_backoff = ComputeUpdatedLogBackoff(
      nlog_backoff_num, nlog_backoff_denom, arc_weight_val, barc_weight_val);
  double score = log_backoff_prob + new_log_backoff - log_prob;
  double secondterm = new_log_backoff + (nlog_backoff_num - nlog_backoff_denom);
  secondterm *= std::exp(-nlog_backoff_num);
  score *= std::exp(log_prob);
  score += secondterm;
  score *= -std::exp(log_prob_s);
  return score;
}

template <class Arc>
inline void DeletePrunedArcs(
    fst::MutableFst<Arc>* fst,
    const std::vector<std::pair<typename Arc::StateId, typename Arc::Label>>&
        to_prune) {
  using StateId = typename Arc::StateId;
  std::vector<Arc> surviving_arcs;
  size_t i = 0;
  while (i < to_prune.size()) {
    const StateId state = to_prune[i].first;
    size_t j = i + 1;
    bool sorted = true;
    while (j < to_prune.size() && to_prune[j].first == state) {
      if (to_prune[j].second < to_prune[j - 1].second) sorted = false;
      ++j;
    }
    surviving_arcs.clear();
    size_t k = i;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, state); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      bool pruned = false;
      if (sorted) {
        while (k < j && to_prune[k].second < arc.ilabel) {
          ++k;
        }
        pruned = (k < j && to_prune[k].second == arc.ilabel);
      } else {
        for (size_t idx = i; idx < j; ++idx) {
          if (to_prune[idx].second == arc.ilabel) {
            pruned = true;
            break;
          }
        }
      }
      if (!pruned) surviving_arcs.push_back(arc);
    }
    fst->DeleteArcs(state);
    fst->ReserveArcs(state, surviving_arcs.size());
    for (const auto& arc : surviving_arcs) fst->AddArc(state, arc);
    i = j;
  }
}

template <class Arc>
inline void PruneFinalAndDeadStates(
    fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
    const std::vector<int>& orders,
    const std::vector<typename Arc::StateId>& finals_to_prune) {
  using StateId = typename Arc::StateId;
  const StateId num_states = fst->NumStates();
  std::vector<StateId> backoff(num_states, fst::kNoStateId);
  for (StateId s = 0; s < num_states; ++s) {
    backoff[s] = GetBackoffState(*fst, s, phi_label);
  }
  std::vector<bool> final_candidates(num_states, false);
  for (const StateId s : finals_to_prune) {
    final_candidates[s] = true;
  }
  std::vector<bool> kept_states(num_states, false);
  for (StateId s = 0; s < num_states; ++s) {
    const bool has_non_phi_arc =
        fst->NumArcs(s) > (backoff[s] != fst::kNoStateId ? 1 : 0);
    const bool has_kept_final =
        fst->Final(s) != Arc::Weight::Zero() && !final_candidates[s];
    if (s == fst->Start() || orders[s] <= 1 || has_non_phi_arc ||
        has_kept_final) {
      StateId curr = s;
      while (curr != fst::kNoStateId && !kept_states[curr]) {
        kept_states[curr] = true;
        curr = backoff[curr];
      }
    }
  }
  std::vector<bool> is_backed_off_to(num_states, false);
  for (StateId other = 0; other < num_states; ++other) {
    if (kept_states[other] && backoff[other] != fst::kNoStateId &&
        backoff[other] != other) {
      is_backed_off_to[backoff[other]] = true;
    }
  }
  for (const StateId s : finals_to_prune) {
    if (!is_backed_off_to[s]) {
      fst->SetFinal(s, Arc::Weight::Zero());
    }
  }
  for (StateId s = 0; s < num_states; ++s) {
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      while (arc.nextstate != fst::kNoStateId && !kept_states[arc.nextstate] &&
             backoff[arc.nextstate] != fst::kNoStateId) {
        arc.nextstate = backoff[arc.nextstate];
      }
      aiter.SetValue(arc);
    }
  }
  fst::Connect(fst);
}

}  // namespace internal

// Computes forward state marginal probabilities for an SFST model.
//
// This algorithm is formulated under standard n-gram assumptions and currently
// only actively propagates probability mass along strictly order-increasing
// transitions. General SFST models containing loops or back-edges between
// the same or lower orders are not actively supported for forward probability
// mass.
template <class Arc>
void ComputeStateProbs(const fst::ExpandedFst<Arc>& fst,
                       typename Arc::Label phi_label,
                       const std::vector<int>& orders,
                       std::vector<double>* probs) {
  using StateId = typename Arc::StateId;
  probs->clear();
  probs->resize(fst.NumStates(), 0.0);
  if (fst.Start() == fst::kNoStateId) return;
  auto unigram_state = fst.Start();
  const auto bo = internal::GetBackoffState(fst, unigram_state, phi_label);
  if (bo != fst::kNoStateId) unigram_state = bo;
  (*probs)[unigram_state] = 1.0;
  if (unigram_state != fst.Start()) {
    (*probs)[fst.Start()] = std::exp(-fst.Final(unigram_state).Value());
  }
  int max_order = 0;
  for (const int o : orders) max_order = std::max(max_order, o);
  std::vector<std::vector<StateId>> buckets(max_order + 1);
  for (StateId s = 0; s < fst.NumStates(); ++s) {
    buckets[orders[s]].push_back(s);
  }
  for (int o = 1; o <= max_order; ++o) {
    for (const StateId s : buckets[o]) {
      const double p_s = (*probs)[s];
      if (p_s == 0.0) continue;
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel == phi_label) continue;
        if (orders[arc.nextstate] > orders[s]) {
          const double weight = std::exp(-arc.weight.Value());
          (*probs)[arc.nextstate] += p_s * weight;
        }
      }
    }
  }
}

// Stolcke relative entropy style model shrinking.
//
// After:
//   Stolcke, A. 1998. Entropy-based pruning of backoff language models. In
//   Proceedings of the DARPA Broadcast News Transcription and Understanding
//   Workshop, pages 270-274.
//
// Computes shrink score for transition based on Stolcke (KL) formula:
// D(p||p') = -p(h) { p(w|h) [ log p(w|h') + log \alpha'(h) - log p(w|h) ] +
//            \alpha_numerator(h) [ log \alpha'(h) - log \alpha (h) ] }
//
// This algorithm is formulated under standard n-gram assumptions and currently
// only actively propagates probability mass along strictly order-increasing
// transitions. General SFST models containing loops or back-edges between
// the same or lower orders are not actively supported for forward probability
// mass.
// An optional 4th argument `filter` can be either:
// - an integral `min_order` (e.g. `int min_order`), which skips pruning any
//   state with `order < min_order`, or
// - a callable `bool(StateId s, Label l)` returning true to skip pruning
//   specific arcs/finals.
// By default, `filter` is `nullptr_t` and no extra filtering is applied.
template <class Arc, class Filter = std::nullptr_t>
bool StolckeShrink(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
                   double theta, Filter filter = nullptr) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "StolckeShrink: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  const double log_theta = std::log(theta + 1);
  std::vector<std::pair<StateId, Label>> to_prune;
  std::vector<StateId> finals_to_prune;
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if constexpr (std::is_integral_v<Filter>) {
      if (orders[s] < filter) continue;
    }
    if (probs[s] == 0.0) continue;
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        const double score = internal::ComputeStolckeScore(
            log_prob_s, nlog_backoff_num, nlog_backoff_denom,
            arc.weight.Value(), barc.weight.Value());
        if (score <= log_theta) {
          if constexpr (!std::is_same_v<Filter, std::nullptr_t> &&
                        !std::is_integral_v<Filter>) {
            if (filter(s, arc.ilabel)) continue;
          }
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
    if (fst->Final(s) != Arc::Weight::Zero() &&
        fst->Final(bo) != Arc::Weight::Zero()) {
      const double score = internal::ComputeStolckeScore(
          log_prob_s, nlog_backoff_num, nlog_backoff_denom,
          fst->Final(s).Value(), fst->Final(bo).Value());
      if (score <= log_theta) {
        bool skip = false;
        if constexpr (!std::is_same_v<Filter, std::nullptr_t> &&
                      !std::is_integral_v<Filter>) {
          skip = filter(s, fst::kNoLabel);
        }
        if (!skip) finals_to_prune.push_back(s);
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);
  if (!finals_to_prune.empty() || !to_prune.empty()) {
    internal::PruneFinalAndDeadStates(fst, phi_label, orders, finals_to_prune);
  }
  PhiNormalize(fst, phi_label);
  return true;
}

// Returns the theta value that will yield at most target_number_of_ngrams
// when using Stolcke relative entropy pruning.
//
// After:
//   Stolcke, A. 1998. Entropy-based pruning of backoff language models. In
//   Proceedings of the DARPA Broadcast News Transcription and Understanding
//   Workshop, pages 270-274.
template <class Arc>
double StolckeThetaForMaxNGrams(const fst::ExpandedFst<Arc>& fst,
                                typename Arc::Label phi_label,
                                int64_t target_number_of_ngrams,
                                int min_order = 2) {
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(fst, phi_label, orders, &probs);
  std::vector<double> scores;
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      &fst, fst::MATCH_INPUT);
  int64_t unigram_count = 0;
  for (StateId s = 0; s < fst.NumStates(); ++s) {
    if (orders[s] == 1) {
      unigram_count +=
          fst.NumArcs(s) + (fst.Final(s) != Arc::Weight::Zero() ? 1 : 0);
    }
  }
  for (StateId s = 0; s < fst.NumStates(); ++s) {
    if (orders[s] <= 1 || probs[s] == 0.0) continue;
    if (orders[s] < min_order) {
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        if (aiter.Value().ilabel != phi_label) {
          scores.push_back(std::numeric_limits<double>::max());
        }
      }
      if (fst.Final(s) != Arc::Weight::Zero()) {
        scores.push_back(std::numeric_limits<double>::max());
      }
      continue;
    }
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(
            fst, s, phi_label, matcher, &bo, &hi_neglog_sum, &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        scores.push_back(internal::ComputeStolckeScore(
            log_prob_s, nlog_backoff_num, nlog_backoff_denom,
            arc.weight.Value(), barc.weight.Value()));
      }
    }
    if (fst.Final(s) != Arc::Weight::Zero() &&
        fst.Final(bo) != Arc::Weight::Zero()) {
      scores.push_back(internal::ComputeStolckeScore(
          log_prob_s, nlog_backoff_num, nlog_backoff_denom,
          fst.Final(s).Value(), fst.Final(bo).Value()));
    }
  }
  if (scores.empty()) return 0.0;
  std::sort(scores.begin(), scores.end());
  int64_t target = target_number_of_ngrams - unigram_count;
  if (target < 0) target = 0;
  int64_t threshold_index = static_cast<int64_t>(scores.size()) - target - 1;
  if (threshold_index < 0) return std::exp(scores[0]) - 2.0;
  double log_theta = scores[threshold_index];
  while (threshold_index < scores.size() &&
         scores[threshold_index] == log_theta) {
    ++threshold_index;
  }
  if (threshold_index >= scores.size()) {
    log_theta += 1.0;
  } else {
    log_theta = (log_theta + scores[threshold_index]) / 2.0;
  }
  return std::exp(log_theta) - 1.0;
}

// Restricted Stolcke relative entropy style model shrinking.
//
// After:
//   Stolcke, A. 1998. Entropy-based pruning of backoff language models. In
//   Proceedings of the DARPA Broadcast News Transcription and Understanding
//   Workshop, pages 270-274.
//
// Prunes an n-gram transition only if the backoff probability does not exceed
// the higher-order probability (log p(w|h') + log \alpha(h) <= log p(w|h)),
// restricting pruning to cases where the backoff model does not overestimate.
template <class Arc, class Filter = std::nullptr_t>
bool RestrictedRelEntropyShrink(fst::MutableFst<Arc>* fst,
                                typename Arc::Label phi_label, double theta,
                                Filter filter = nullptr) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "RestrictedRelEntropyShrink: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  const double log_theta = std::log(theta + 1);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if constexpr (std::is_integral_v<Filter>) {
      if (orders[s] < filter) continue;
    }
    if (probs[s] == 0.0) continue;
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    const double old_log_backoff = -(nlog_backoff_num - nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        const double log_prob = -arc.weight.Value();
        const double log_backoff_prob = -barc.weight.Value();
        if (log_backoff_prob + old_log_backoff > log_prob) {
          continue;
        }
        const double new_log_backoff = internal::ComputeUpdatedLogBackoff(
            nlog_backoff_num, nlog_backoff_denom, arc.weight.Value(),
            barc.weight.Value());
        double score = log_backoff_prob + new_log_backoff - log_prob;
        double secondterm =
            new_log_backoff + (nlog_backoff_num - nlog_backoff_denom);
        secondterm *= std::exp(-nlog_backoff_num);
        score *= std::exp(log_prob);
        score += secondterm;
        score *= -std::exp(log_prob_s);
        if (score <= log_theta) {
          if constexpr (!std::is_same_v<Filter, std::nullptr_t> &&
                        !std::is_integral_v<Filter>) {
            if (filter(s, arc.ilabel)) continue;
          }
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);
  PhiNormalize(fst, phi_label);
  return true;
}

// Symmetrized relative entropy model shrinking.
//
// After:
//   Stolcke, A. 1998. Entropy-based pruning of backoff language models. In
//   Proceedings of the DARPA Broadcast News Transcription and Understanding
//   Workshop, pages 270-274.
//
// Computes shrink score based on the symmetrized Kullback-Leibler divergence
// D(p||p') + D(p'||p) between the original and pruned language models.
template <class Arc, class Filter = std::nullptr_t>
bool SymmetrizedRelEntropyShrink(fst::MutableFst<Arc>* fst,
                                 typename Arc::Label phi_label, double theta,
                                 Filter filter = nullptr) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "SymmetrizedRelEntropyShrink: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  const double log_theta = std::log(theta + 1);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if constexpr (std::is_integral_v<Filter>) {
      if (orders[s] < filter) continue;
    }
    if (probs[s] == 0.0) continue;
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    const double old_log_backoff = -(nlog_backoff_num - nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        const double log_prob = -arc.weight.Value();
        const double log_backoff_prob = -barc.weight.Value();
        const double new_log_backoff = internal::ComputeUpdatedLogBackoff(
            nlog_backoff_num, nlog_backoff_denom, arc.weight.Value(),
            barc.weight.Value());
        double score = log_backoff_prob + old_log_backoff - log_prob;
        score *=
            std::exp(log_prob) - std::exp(log_backoff_prob + new_log_backoff);
        score *= -std::exp(log_prob_s);
        score /= 2.0;
        if (score <= log_theta) {
          if constexpr (!std::is_same_v<Filter, std::nullptr_t> &&
                        !std::is_integral_v<Filter>) {
            if (filter(s, arc.ilabel)) continue;
          }
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);
  PhiNormalize(fst, phi_label);
  return true;
}

// Estimates total unigram count from unigram state weights.
template <class Arc>
double EstimateTotalUnigramCount(const fst::Fst<Arc>& fst,
                                 typename Arc::Label phi_label) {
  using StateId = typename Arc::StateId;
  StateId unigram_state = fst.Start();
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, fst.Start()); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == phi_label) {
      unigram_state = aiter.Value().nextstate;
      break;
    }
  }
  double max_val = -1.0;
  double nextmax_val = -1.0;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, unigram_state); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label) continue;
    const double w = arc.weight.Value();
    if (max_val < 0.0 || w > max_val) {
      nextmax_val = max_val;
      max_val = w;
    } else if (w < max_val && (nextmax_val < 0.0 || w > nextmax_val)) {
      nextmax_val = w;
    }
  }
  if (max_val < 0.0) return 0.0;
  if (nextmax_val < 0.0) return std::exp(max_val);
  return std::exp(NegLogDiff(nextmax_val, max_val));
}

// Seymore and Rosenfeld model shrinking.
//
// After:
//   Seymore, K. and Rosenfeld, R. 1996. Scalable backoff language models. In
//   Proceedings of the 4th International Conference on Spoken Language
//   Processing (ICSLP), Vol. 1, pages 232-235.
//
// Computes shrink score for transition based on Seymore/Rosenfeld formula:
// N(w,h) [ log p(w|h) - log p'(w|h) ] where N(w,h) is discounted frequency.
//
// This algorithm is formulated under standard n-gram assumptions and currently
// only actively propagates probability mass along strictly order-increasing
// transitions. General SFST models containing loops or back-edges between
// the same or lower orders are not actively supported for forward probability
// mass.
template <class Arc>
bool SeymoreShrink(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
                   double theta, double total_unigram_count = -1.0) {
  if (total_unigram_count <= 0.0) {
    total_unigram_count = EstimateTotalUnigramCount(*fst, phi_label);
  }
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "SeymoreShrink: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        const double log_prob = -arc.weight.Value();
        const double log_backoff_prob = -barc.weight.Value();
        const double new_log_backoff = internal::ComputeUpdatedLogBackoff(
            nlog_backoff_num, nlog_backoff_denom, arc.weight.Value(),
            barc.weight.Value());
        double score = log_prob - new_log_backoff - log_backoff_prob;
        score *= total_unigram_count;
        score *= std::exp(log_prob_s + log_prob);
        if (score < theta) to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);
  return PhiNormalize(fst, phi_label);
}

// Absolute Seymore and Rosenfeld model shrinking (|score| < theta).
//
// After:
//   Seymore, K. and Rosenfeld, R. 1996. Scalable backoff language models. In
//   Proceedings of the 4th International Conference on Spoken Language
//   Processing (ICSLP), Vol. 1, pages 232-235.
//
// Computes shrink score based on the absolute difference between model
// distributions (|score| < theta), pruning arcs whose change in probability in
// either direction falls within the threshold.
template <class Arc>
bool AbsoluteSeymoreShrink(fst::MutableFst<Arc>* fst,
                           typename Arc::Label phi_label, double theta,
                           double total_unigram_count = -1.0) {
  if (total_unigram_count <= 0.0) {
    total_unigram_count = EstimateTotalUnigramCount(*fst, phi_label);
  }
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "AbsoluteSeymoreShrink: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        const double log_prob = -arc.weight.Value();
        const double log_backoff_prob = -barc.weight.Value();
        const double new_log_backoff = internal::ComputeUpdatedLogBackoff(
            nlog_backoff_num, nlog_backoff_denom, arc.weight.Value(),
            barc.weight.Value());
        double score = log_prob - new_log_backoff - log_backoff_prob;
        score *= total_unigram_count;
        score *= std::exp(log_prob_s + log_prob);
        if (std::abs(score) < theta) {
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);
  return PhiNormalize(fst, phi_label);
}

// Shrinks the model by pruning transitions below specified count thresholds.
//
// The `count_pattern` parameter specifies pruning thresholds in the format:
//   order[+]:count[;order[+]:count...]
//
// Where:
//   - `order`: Positive integer specifying the n-gram order (1 for unigrams,
//     2 for bigrams, etc.).
//   - `+`: Optional suffix indicating that the threshold applies to `order` and
//     all higher orders up to `max_order`.
//   - `count`: Positive numeric threshold below which transitions are pruned.
//   - `;`: Semicolon delimiter separating multiple order rules.
//
// Examples:
//   - "2:5": Prune bigrams with count < 5.
//   - "3+:2": Prune trigrams and higher-order n-grams with count < 2.
//   - "2:5;3:10;4+:20": Prune bigrams < 5, trigrams < 10, and 4-grams+ < 20.
template <class Arc>
bool CountPrune(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
                const std::string& count_pattern) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "CountPrune: input is not a canonical SFST";
    return false;
  }
  if (count_pattern.empty()) return false;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  int max_order = 0;
  for (const int o : orders) max_order = std::max(max_order, o);
  std::vector<double> count_minimums(max_order, -Weight::Zero().Value());
  for (absl::string_view spec :
       absl::StrSplit(count_pattern, ';', absl::SkipWhitespace())) {
    const size_t colon_pos = spec.find(':');
    if (colon_pos == absl::string_view::npos) {
      LOG(ERROR) << "CountPrune: invalid spec (missing ':'): " << spec;
      return false;
    }
    absl::string_view order_str = spec.substr(0, colon_pos);
    const absl::string_view count_str = spec.substr(colon_pos + 1);
    const bool plus = absl::ConsumeSuffix(&order_str, "+");
    int order = 0;
    if (!absl::SimpleAtoi(order_str, &order) || order <= 0) {
      LOG(ERROR) << "CountPrune: invalid order: " << spec.substr(0, colon_pos);
      return false;
    }
    double count = 0.0;
    if (!absl::SimpleAtod(count_str, &count) || std::isnan(count)) {
      LOG(ERROR) << "CountPrune: invalid count: " << count_str;
      return false;
    }
    const double theta =
        (count <= 0.0) ? Weight::Zero().Value() : std::log(count);
    if (order > 0 && order <= max_order) {
      if (count_minimums[order - 1] < theta) count_minimums[order - 1] = theta;
      if (plus) {
        for (int i = order; i < max_order; ++i) {
          if (count_minimums[i] < theta) count_minimums[i] = theta;
        }
      }
    }
  }
  std::vector<Arc> surviving_arcs;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    const int order = orders[s];
    if (order == 0) continue;
    const double theta = count_minimums[order - 1];

    // Fast path: checks if any arc qualifies for pruning before mutating the
    // state. Unpruned states are skipped without arc deletions or copies.
    bool has_pruned = false;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != phi_label && -arc.weight.Value() < theta) {
        has_pruned = true;
        break;
      }
    }
    if (!has_pruned) continue;
    // Prunes arcs in-place in a single pass: collects all surviving arcs
    // (including the failure/phi arc), deletes all arcs from state `s` once,
    // and re-adds the survivors in their original order.
    surviving_arcs.clear();
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label || -arc.weight.Value() >= theta) {
        surviving_arcs.push_back(arc);
      }
    }
    fst->DeleteArcs(s);
    fst->ReserveArcs(s, surviving_arcs.size());
    for (const auto& arc : surviving_arcs) {
      fst->AddArc(s, arc);
    }
  }
  return PhiNormalize(fst, phi_label);
}

// Reads a list of n-grams from a file to be used for list pruning.
inline void ReadNGramList(
    absl::string_view file_name, const fst::SymbolTable* syms,
    std::set<std::vector<fst::StdArc::Label>>* ngram_list) {
  if (ngram_list == nullptr) return;
  std::ifstream ifstrm{std::string(file_name)};
  if (!ifstrm) {
    LOG(ERROR) << "Can't open " << file_name;
    return;
  }
  std::string line;
  while (std::getline(ifstrm, line)) {
    std::vector<fst::StdArc::Label> ngram;
    for (absl::string_view token :
         absl::StrSplit(line, absl::ByAnyChar(" \t\r"), absl::SkipEmpty())) {
      fst::StdArc::Label label;
      if (syms) {
        label = syms->Find(token);
        if (label == fst::kNoSymbol) {
          LOG(ERROR) << "Symbol " << token << " not found in symbol table";
          continue;
        }
      } else {
        if (!absl::SimpleAtoi(token, &label)) {
          LOG(ERROR) << "Invalid label ID: " << token;
          continue;
        }
      }
      ngram.push_back(label);
    }
    if (!ngram.empty()) ngram_list->insert(std::move(ngram));
  }
}

// Reads a word set from a file or comma-separated list of symbols.
template <class Label>
inline void ReadWordSet(absl::string_view word_set_spec,
                        const fst::SymbolTable* syms,
                        absl::flat_hash_set<Label>* word_set) {
  if (word_set_spec.empty()) return;
  const std::string file_path(word_set_spec);
  std::ifstream strm(file_path);
  if (strm) {
    std::string line;
    while (std::getline(strm, line)) {
      if (syms) {
        const auto l = syms->Find(line);
        if (l != fst::kNoSymbol) word_set->insert(l);
      } else {
        int64_t l;
        if (absl::SimpleAtoi(line, &l)) word_set->insert(l);
      }
    }
  } else {
    for (absl::string_view w : absl::StrSplit(word_set_spec, ',')) {
      if (syms) {
        const auto l = syms->Find(std::string(w));
        if (l != fst::kNoSymbol) word_set->insert(l);
      } else {
        int64_t l;
        if (absl::SimpleAtoi(w, &l)) word_set->insert(l);
      }
    }
  }
}

template <class Arc>
void AscendAndMark(const fst::Fst<Arc>& fst, typename Arc::StateId s,
                   typename Arc::Label phi_label,
                   const std::vector<int>& orders,
                   std::vector<bool>* state_on_prune_path) {
  if ((*state_on_prune_path)[s]) {
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != phi_label && !(*state_on_prune_path)[arc.nextstate] &&
          orders[s] < orders[arc.nextstate]) {
        (*state_on_prune_path)[arc.nextstate] = true;
        AscendAndMark(fst, arc.nextstate, phi_label, orders,
                      state_on_prune_path);
      }
    }
  }
}

// Model shrinking for a list of n-grams to be pruned.
template <class Arc>
bool ListPrune(
    fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
    const std::set<std::vector<typename Arc::Label>>& ngrams_to_prune) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "ListPrune: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  int max_order = 0;
  for (const int o : orders) max_order = std::max(max_order, o);
  std::vector<bool> state_on_prune_path(fst->NumStates(), false);
  std::set<std::pair<StateId, Label>> highest_order_prune_arcs;
  std::set<StateId> highest_order_prune_origin_state;
  StateId unigram_state = fst->Start();
  std::set<StateId> backoff_states;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        backoff_states.insert(arc.nextstate);
        break;
      }
    }
  }
  for (const StateId s : backoff_states) {
    if (orders[s] == 1) {
      unigram_state = s;
      break;
    }
  }
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (const auto& ngram : ngrams_to_prune) {
    // Prevents origin_state == kNoStateId crash.
    if (ngram.empty()) continue;
    StateId origin_state = fst::kNoStateId;
    StateId curr_state = unigram_state;
    bool ngram_found = true;
    for (const auto label : ngram) {
      if (label < 0) {
        ngram_found = false;
      } else {
        const bool still_ascending = origin_state == fst::kNoStateId ||
                                     orders[origin_state] < orders[curr_state];
        matcher.SetState(curr_state);
        if (still_ascending && matcher.Find(label)) {
          const Arc& arc = matcher.Value();
          origin_state = curr_state;
          curr_state = arc.nextstate;
        } else {
          ngram_found = false;
        }
      }
      if (!ngram_found) break;
    }
    if (ngram_found) {
      if (orders[origin_state] < orders[curr_state]) {
        state_on_prune_path[curr_state] = true;
      } else {
        highest_order_prune_origin_state.insert(origin_state);
        highest_order_prune_arcs.insert({origin_state, ngram.back()});
      }
    }
  }
  std::vector<std::vector<StateId>> order_states(max_order + 1);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    const int state_order = orders[s];
    if (state_order >= 0 && !state_on_prune_path[s])
      order_states[state_order].push_back(s);
  }
  for (int order = 1; order <= max_order; ++order) {
    for (const auto s : order_states[order]) {
      StateId phi_state = -1;
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel == phi_label) {
          phi_state = arc.nextstate;
          break;
        }
      }
      if (phi_state != -1) {
        if (state_on_prune_path[phi_state]) {
          state_on_prune_path[s] = true;
        } else if (highest_order_prune_origin_state.find(phi_state) !=
                   highest_order_prune_origin_state.end()) {
          for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
               aiter.Next()) {
            const auto& arc = aiter.Value();
            if (arc.ilabel == phi_label) continue;
            if (highest_order_prune_arcs.find({phi_state, arc.ilabel}) !=
                highest_order_prune_arcs.end()) {
              highest_order_prune_arcs.insert({s, arc.ilabel});
            }
          }
        }
      }
    }
  }
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    AscendAndMark(*fst, s, phi_label, orders, &state_on_prune_path);
  }
  std::vector<std::pair<StateId, Label>> to_prune;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (state_on_prune_path[s] ||
          (arc.nextstate != fst::kNoStateId &&
           state_on_prune_path[arc.nextstate]) ||
          highest_order_prune_arcs.find({s, arc.ilabel}) !=
              highest_order_prune_arcs.end()) {
        to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);
  return PhiNormalize(fst, phi_label);
}

// Significance-based model shrinking.
//
// After:
//   Moore, R. C. and Quirk, C. 2009. Less is more: Significance-based
//   n-gram selection for smaller, better language models. In Proceedings of the
//   2009 Conference on Empirical Methods in Natural Language Processing
//   (EMNLP), pages 746-755.
//
// Prunes n-grams whose estimated probability is not statistically significantly
// different from the backoff model's estimate according to empirical binomial
// confidence intervals.
template <class Arc>
bool SignificanceShrink(fst::MutableFst<Arc>* fst,
                        typename Arc::Label phi_label,
                        const fst::ExpandedFst<Arc>* count_fst = nullptr,
                        double total_unigram_count = -1.0) {
  if (!count_fst && total_unigram_count <= 0.0) {
    total_unigram_count = EstimateTotalUnigramCount(*fst, phi_label);
  }
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "SignificanceShrink: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);

  std::vector<double> state_counts;
  std::unique_ptr<fst::Matcher<fst::ExpandedFst<Arc>>> count_matcher;
  if (count_fst) {
    state_counts.resize(fst->NumStates(), -1.0);
    for (StateId s = 0; s < fst->NumStates(); ++s) {
      for (fst::ArcIterator<fst::ExpandedFst<Arc>> aiter(*count_fst, s);
           !aiter.Done(); aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel == phi_label) {
          state_counts[s] = std::exp(-arc.weight.Value());
          break;
        }
      }
    }
    count_matcher = std::make_unique<fst::Matcher<fst::ExpandedFst<Arc>>>(
        count_fst, fst::MATCH_INPUT);
  }

  auto distance = [](double x, double lower, double upper) {
    if (x < lower) return lower - x;
    if (x > upper) return x - upper;
    return 0.0;
  };

  std::vector<absl::flat_hash_set<size_t>> to_prune_pos(fst->NumStates());
  fst::ExplicitMatcher<fst::SortedMatcher<fst::Fst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (orders[s] < 1 || probs[s] == 0.0) continue;
    const double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num;
    double nlog_backoff_denom;
    internal::ComputeBackoffNormalizers(hi_neglog_sum, low_neglog_sum,
                                        &nlog_backoff_num, &nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        const double log_prob = -arc.weight.Value();
        const double log_backoff_prob = -barc.weight.Value();
        const double new_log_backoff = internal::ComputeUpdatedLogBackoff(
            nlog_backoff_num, nlog_backoff_denom, arc.weight.Value(),
            barc.weight.Value());

        double state_count = 0.0;
        double arc_count = 0.0;
        if (count_fst) {
          state_count = (s < state_counts.size()) ? state_counts[s] : 0.0;
          count_matcher->SetState(s);
          if (count_matcher->Find(arc.ilabel)) {
            arc_count = std::exp(-count_matcher->Value().weight.Value());
          } else {
            arc_count = 0.0;
          }
        } else {
          state_count = total_unigram_count * std::exp(log_prob_s);
          arc_count = state_count * std::exp(log_prob);
        }

        const double lower = arc_count / (state_count + 1.0);
        const double upper = (arc_count + 1.0) / (state_count + 1.0);
        const double backoff_prob =
            std::exp(log_backoff_prob + new_log_backoff);
        const double prob = std::exp(log_prob);
        const double dist_bo = distance(backoff_prob, lower, upper);
        const double dist_curr = distance(prob, lower, upper);
        bool needed = false;
        if (arc.nextstate != fst::kNoStateId &&
            static_cast<size_t>(arc.nextstate) < orders.size() &&
            orders[arc.nextstate] > orders[s]) {
          for (fst::ArcIterator<fst::Fst<Arc>> naiter(*fst, arc.nextstate);
               !naiter.Done(); naiter.Next()) {
            if (naiter.Value().ilabel != phi_label) {
              needed = true;
              break;
            }
          }
        }
        if (!needed &&
            ((dist_bo < kFloatEps) || (dist_bo < dist_curr + kFloatEps))) {
          to_prune_pos[s].insert(aiter.Position());
        }
      }
    }
  }
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (to_prune_pos[s].empty()) continue;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      if (!to_prune_pos[s].contains(aiter.Position())) {
        arcs.push_back(aiter.Value());
      }
    }
    fst->DeleteArcs(s);
    fst->ReserveArcs(s, arcs.size());
    for (const auto& arc : arcs) fst->AddArc(s, arc);
  }
  return PhiNormalize(fst, phi_label);
}

// Pruning method subject to keeping all n-grams containing any word in
// word_set.
template <class Arc>
bool WordShrink(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
                const absl::flat_hash_set<typename Arc::Label>& word_set) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "WordShrink: input is not a canonical SFST";
    return false;
  }
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);

  std::vector<StateId> prefix_state(fst->NumStates(), fst::kNoStateId);
  std::vector<Label> incoming_label(fst->NumStates(), fst::kNoLabel);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (arc.nextstate < fst->NumStates() &&
          orders[s] < orders[arc.nextstate]) {
        prefix_state[arc.nextstate] = s;
        incoming_label[arc.nextstate] = arc.ilabel;
      }
    }
  }

  std::vector<std::pair<StateId, Label>> to_prune;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (orders[s] <= 1) continue;
    bool history_has_word = false;
    for (StateId curr = s; curr >= 0 && curr < fst->NumStates() &&
                           prefix_state[curr] != fst::kNoStateId;
         curr = prefix_state[curr]) {
      if (word_set.contains(incoming_label[curr])) {
        history_has_word = true;
        break;
      }
    }
    if (history_has_word) continue;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (!word_set.contains(arc.ilabel)) {
        to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  internal::DeletePrunedArcs(fst, to_prune);

  return true;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_SHRINK_H_
