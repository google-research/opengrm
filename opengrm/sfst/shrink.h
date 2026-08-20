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

#include <algorithm>  // NOLINT(misc-include-cleaner)
#include <cmath>
#include <cstddef>
#include <fstream>
#include <memory>
#include <set>  // NOLINT(misc-include-cleaner)
#include <sstream>
#include <string>
#include <utility>  // NOLINT(misc-include-cleaner)
#include <vector>   // NOLINT(misc-include-cleaner)

#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
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

template <class Arc, class Matcher>
inline bool ComputeStateAndBackoffSums(
    const fst::Fst<Arc>& fst, typename Arc::StateId s,
    typename Arc::Label phi_label, Matcher& matcher, typename Arc::StateId* bo,
    double* hi_neglog_sum, double* low_neglog_sum) {
  *bo = -1;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label) {
      *bo = arc.nextstate;
      break;
    }
  }
  if (*bo == -1) return false;
  *hi_neglog_sum = fst.Final(s).Value();
  *low_neglog_sum = fst.Final(*bo).Value();
  matcher.SetState(*bo);
  double KahanVal1 = 0.0;
  double KahanVal2 = 0.0;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label) continue;
    *hi_neglog_sum = NegLogSum(*hi_neglog_sum, arc.weight.Value(), &KahanVal1);
    if (matcher.Find(arc.ilabel)) {
      const auto& barc = matcher.Value();
      *low_neglog_sum =
          NegLogSum(*low_neglog_sum, barc.weight.Value(), &KahanVal2);
    }
  }
  return true;
}

}  // namespace internal

// Helper to compute forward state probabilities. Used internally by shrink
// algorithms.
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
  (*probs)[fst.Start()] = 1.0;
  int max_order = 0;
  for (int o : orders) max_order = std::max(max_order, o);
  std::vector<std::vector<StateId>> buckets(max_order + 1);
  for (StateId s = 0; s < fst.NumStates(); ++s) {
    buckets[orders[s]].push_back(s);
  }
  for (int o = 1; o <= max_order; ++o) {
    for (StateId s : buckets[o]) {
      double p_s = (*probs)[s];
      if (p_s == 0.0) continue;
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel == phi_label) continue;
        if (orders[arc.nextstate] > orders[s]) {
          double weight = std::exp(-arc.weight.Value());
          (*probs)[arc.nextstate] += p_s * weight;
        }
      }
    }
  }
}

// Stolcke relative entropy style model shrinking.
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
// An optional `filter` callable `bool(StateId s, Label l)` can be passed to
// skip pruning specific arcs (returning true prevents pruning). By default,
// filter is nullptr_t and no filtering is applied.
template <class Arc, class Filter = std::nullptr_t>
bool StolckeShrink(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
                   double theta, Filter filter = nullptr) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "StolckeShrink: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  double log_theta = std::log(theta + 1);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    double log_prob_s = std::log(probs[s]);
    StateId bo = -1;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        bo = arc.nextstate;
        break;
      }
    }
    if (bo == -1) continue;
    double hi_neglog_sum = fst->Final(s).Value();
    double low_neglog_sum = fst->Final(bo).Value();
    matcher.SetState(bo);
    double KahanVal1 = 0;
    double KahanVal2 = 0;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      hi_neglog_sum = NegLogSum(hi_neglog_sum, arc.weight.Value(), &KahanVal1);
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        low_neglog_sum =
            NegLogSum(low_neglog_sum, barc.weight.Value(), &KahanVal2);
      }
    }
    double nlog_backoff_num = 0.0;
    double nlog_backoff_denom = 0.0;
    double effective_zero = kNormEps * kFloatEps;
    double effective_nlog_zero = 99.0;
    double tmp_hi = hi_neglog_sum;
    double tmp_low = low_neglog_sum;
    if (tmp_hi < effective_zero) tmp_hi = effective_zero;
    if (tmp_low < effective_zero) tmp_low = effective_zero;
    if (tmp_low > 0 && tmp_hi > 0) {
      if (tmp_hi > effective_nlog_zero) {
        nlog_backoff_num = 0.0;
      } else {
        nlog_backoff_num = NegLogDiff(0.0, tmp_hi);
      }
      if (tmp_low > effective_nlog_zero) {
        nlog_backoff_denom = 0.0;
      } else {
        nlog_backoff_denom = NegLogDiff(0.0, tmp_low);
      }
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        double log_prob = -arc.weight.Value();
        double log_backoff_prob = -barc.weight.Value();
        double new_log_backoff =
            NegLogSum(nlog_backoff_denom, barc.weight.Value()) -
            NegLogSum(nlog_backoff_num, arc.weight.Value());
        double score = log_backoff_prob + new_log_backoff - log_prob;
        double secondterm =
            new_log_backoff + (nlog_backoff_num - nlog_backoff_denom);
        secondterm *= std::exp(-nlog_backoff_num);
        score *= std::exp(log_prob);
        score += secondterm;
        score *= -std::exp(log_prob_s);
        if (score <= log_theta) {
          if constexpr (!std::is_same_v<Filter, std::nullptr_t>) {
            if (filter(s, arc.ilabel)) continue;
          }
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) {
        arcs.push_back(arc);
      }
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) {
      fst->AddArc(s, arc);
    }
  }
  PhiNormalize(fst, phi_label);
  return true;
}

// Restricted Stolcke relative entropy style model shrinking.
template <class Arc, class Filter = std::nullptr_t>
bool RestrictedRelEntropyShrink(fst::MutableFst<Arc>* fst,
                                typename Arc::Label phi_label, double theta,
                                Filter filter = nullptr) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "RestrictedRelEntropyShrink: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  double log_theta = std::log(theta + 1);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    double log_prob_s = std::log(probs[s]);
    StateId bo = -1;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        bo = arc.nextstate;
        break;
      }
    }
    if (bo == -1) continue;
    double hi_neglog_sum = fst->Final(s).Value();
    double low_neglog_sum = fst->Final(bo).Value();
    matcher.SetState(bo);
    double KahanVal1 = 0;
    double KahanVal2 = 0;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      hi_neglog_sum = NegLogSum(hi_neglog_sum, arc.weight.Value(), &KahanVal1);
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        low_neglog_sum =
            NegLogSum(low_neglog_sum, barc.weight.Value(), &KahanVal2);
      }
    }
    double nlog_backoff_num = 0.0;
    double nlog_backoff_denom = 0.0;
    double effective_zero = kNormEps * kFloatEps;
    double effective_nlog_zero = 99.0;
    double tmp_hi = hi_neglog_sum;
    double tmp_low = low_neglog_sum;
    if (tmp_hi < effective_zero) tmp_hi = effective_zero;
    if (tmp_low < effective_zero) tmp_low = effective_zero;
    if (tmp_low > 0 && tmp_hi > 0) {
      if (tmp_hi > effective_nlog_zero) {
        nlog_backoff_num = 0.0;
      } else {
        nlog_backoff_num = NegLogDiff(0.0, tmp_hi);
      }
      if (tmp_low > effective_nlog_zero) {
        nlog_backoff_denom = 0.0;
      } else {
        nlog_backoff_denom = NegLogDiff(0.0, tmp_low);
      }
    }
    double old_log_backoff = -(nlog_backoff_num - nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        double log_prob = -arc.weight.Value();
        double log_backoff_prob = -barc.weight.Value();
        if (log_backoff_prob + old_log_backoff > log_prob) {
          continue;
        }
        double new_log_backoff =
            NegLogSum(nlog_backoff_denom, barc.weight.Value()) -
            NegLogSum(nlog_backoff_num, arc.weight.Value());
        double score = log_backoff_prob + new_log_backoff - log_prob;
        double secondterm =
            new_log_backoff + (nlog_backoff_num - nlog_backoff_denom);
        secondterm *= std::exp(-nlog_backoff_num);
        score *= std::exp(log_prob);
        score += secondterm;
        score *= -std::exp(log_prob_s);
        if (score <= log_theta) {
          if constexpr (!std::is_same_v<Filter, std::nullptr_t>) {
            if (filter(s, arc.ilabel)) continue;
          }
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) {
        arcs.push_back(arc);
      }
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) {
      fst->AddArc(s, arc);
    }
  }
  PhiNormalize(fst, phi_label);
  return true;
}

// Symmetrized relative entropy model shrinking.
template <class Arc, class Filter = std::nullptr_t>
bool SymmetrizedRelEntropyShrink(fst::MutableFst<Arc>* fst,
                                 typename Arc::Label phi_label, double theta,
                                 Filter filter = nullptr) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "SymmetrizedRelEntropyShrink: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  double log_theta = std::log(theta + 1);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    double log_prob_s = std::log(probs[s]);
    StateId bo = -1;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        bo = arc.nextstate;
        break;
      }
    }
    if (bo == -1) continue;
    double hi_neglog_sum = fst->Final(s).Value();
    double low_neglog_sum = fst->Final(bo).Value();
    matcher.SetState(bo);
    double KahanVal1 = 0;
    double KahanVal2 = 0;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      hi_neglog_sum = NegLogSum(hi_neglog_sum, arc.weight.Value(), &KahanVal1);
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        low_neglog_sum =
            NegLogSum(low_neglog_sum, barc.weight.Value(), &KahanVal2);
      }
    }
    double nlog_backoff_num = 0.0;
    double nlog_backoff_denom = 0.0;
    double effective_zero = kNormEps * kFloatEps;
    double effective_nlog_zero = 99.0;
    double tmp_hi = hi_neglog_sum;
    double tmp_low = low_neglog_sum;
    if (tmp_hi < effective_zero) tmp_hi = effective_zero;
    if (tmp_low < effective_zero) tmp_low = effective_zero;
    if (tmp_low > 0 && tmp_hi > 0) {
      if (tmp_hi > effective_nlog_zero) {
        nlog_backoff_num = 0.0;
      } else {
        nlog_backoff_num = NegLogDiff(0.0, tmp_hi);
      }
      if (tmp_low > effective_nlog_zero) {
        nlog_backoff_denom = 0.0;
      } else {
        nlog_backoff_denom = NegLogDiff(0.0, tmp_low);
      }
    }
    double old_log_backoff = -(nlog_backoff_num - nlog_backoff_denom);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        double log_prob = -arc.weight.Value();
        double log_backoff_prob = -barc.weight.Value();
        double new_log_backoff =
            NegLogSum(nlog_backoff_denom, barc.weight.Value()) -
            NegLogSum(nlog_backoff_num, arc.weight.Value());
        double score = log_backoff_prob + old_log_backoff - log_prob;
        score *=
            std::exp(log_prob) - std::exp(log_backoff_prob + new_log_backoff);
        score *= -std::exp(log_prob_s);
        score /= 2.0;
        if (score <= log_theta) {
          if constexpr (!std::is_same_v<Filter, std::nullptr_t>) {
            if (filter(s, arc.ilabel)) continue;
          }
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) {
        arcs.push_back(arc);
      }
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) {
      fst->AddArc(s, arc);
    }
  }
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
    double w = arc.weight.Value();
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
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num = 0.0;
    double nlog_backoff_denom = 0.0;
    double effective_zero = kNormEps * kFloatEps;
    double effective_nlog_zero = 99.0;
    double tmp_hi = hi_neglog_sum;
    double tmp_low = low_neglog_sum;
    if (tmp_hi < effective_zero) tmp_hi = effective_zero;
    if (tmp_low < effective_zero) tmp_low = effective_zero;
    if (tmp_low > 0 && tmp_hi > 0) {
      nlog_backoff_num =
          (tmp_hi > effective_nlog_zero) ? 0.0 : NegLogDiff(0.0, tmp_hi);
      nlog_backoff_denom =
          (tmp_low > effective_nlog_zero) ? 0.0 : NegLogDiff(0.0, tmp_low);
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        double log_prob = -arc.weight.Value();
        double log_backoff_prob = -barc.weight.Value();
        double new_log_backoff =
            NegLogSum(nlog_backoff_denom, barc.weight.Value()) -
            NegLogSum(nlog_backoff_num, arc.weight.Value());
        double score = log_prob - new_log_backoff - log_backoff_prob;
        score *= total_unigram_count;
        score *= std::exp(log_prob_s + log_prob);
        if (score < theta) to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) arcs.push_back(arc);
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) fst->AddArc(s, arc);
  }
  return PhiNormalize(fst, phi_label);
}

// Absolute Seymore and Rosenfeld model shrinking (|score| < theta).
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
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  std::vector<double> probs;
  ComputeStateProbs(*fst, phi_label, orders, &probs);
  std::vector<std::pair<StateId, Label>> to_prune;
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    double log_prob_s = std::log(probs[s]);
    StateId bo;
    double hi_neglog_sum;
    double low_neglog_sum;
    if (!internal::ComputeStateAndBackoffSums(*fst, s, phi_label, matcher, &bo,
                                              &hi_neglog_sum,
                                              &low_neglog_sum)) {
      continue;
    }
    double nlog_backoff_num = 0.0;
    double nlog_backoff_denom = 0.0;
    double effective_zero = kNormEps * kFloatEps;
    double effective_nlog_zero = 99.0;
    double tmp_hi = hi_neglog_sum;
    double tmp_low = low_neglog_sum;
    if (tmp_hi < effective_zero) tmp_hi = effective_zero;
    if (tmp_low < effective_zero) tmp_low = effective_zero;
    if (tmp_low > 0 && tmp_hi > 0) {
      nlog_backoff_num =
          (tmp_hi > effective_nlog_zero) ? 0.0 : NegLogDiff(0.0, tmp_hi);
      nlog_backoff_denom =
          (tmp_low > effective_nlog_zero) ? 0.0 : NegLogDiff(0.0, tmp_low);
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        double log_prob = -arc.weight.Value();
        double log_backoff_prob = -barc.weight.Value();
        double new_log_backoff =
            NegLogSum(nlog_backoff_denom, barc.weight.Value()) -
            NegLogSum(nlog_backoff_num, arc.weight.Value());
        double score = log_prob - new_log_backoff - log_backoff_prob;
        score *= total_unigram_count;
        score *= std::exp(log_prob_s + log_prob);
        if (std::abs(score) < theta) to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) arcs.push_back(arc);
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) fst->AddArc(s, arc);
  }
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
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  int max_order = 0;
  for (int o : orders) max_order = std::max(max_order, o);
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
  std::vector<std::pair<StateId, Label>> to_prune;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    int order = orders[s];
    if (order == 0) continue;
    double theta = count_minimums[order - 1];
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      double log_prob = -arc.weight.Value();
      if (log_prob < theta) {
        to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) {
        arcs.push_back(arc);
      }
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) {
      fst->AddArc(s, arc);
    }
  }
  return PhiNormalize(fst, phi_label);
}

// Reads a list of n-grams from a file to be used for list pruning.
inline void ReadNGramList(
    const std::string& file_name, const fst::SymbolTable* syms,
    std::set<std::vector<fst::StdArc::Label>>* ngram_list) {
  std::ifstream ifstrm(file_name);
  if (!ifstrm) {
    LOG(ERROR) << "Can't open " << file_name;
    return;
  }
  std::string line;
  while (std::getline(ifstrm, line)) {
    std::stringstream ss(line);
    std::string token;
    std::vector<fst::StdArc::Label> ngram;
    while (ss >> token) {
      fst::StdArc::Label label;
      if (syms) {
        label = syms->Find(token);
        if (label == -1) {
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
    if (!ngram.empty()) ngram_list->insert(ngram);
  }
}

// Reads a word set from a file or comma-separated list of symbols.
template <class Label>
inline void ReadWordSet(absl::string_view word_set_spec,
                        const fst::SymbolTable* syms,
                        absl::flat_hash_set<Label>* word_set) {
  if (word_set_spec.empty()) return;
  std::string file_path(word_set_spec);
  std::ifstream strm(file_path);
  if (strm) {
    std::string line;
    while (std::getline(strm, line)) {
      if (syms) {
        auto l = syms->Find(line);
        if (l != fst::kNoSymbol) word_set->insert(l);
      } else {
        int64_t l;
        if (absl::SimpleAtoi(line, &l)) word_set->insert(l);
      }
    }
  } else {
    for (absl::string_view w : absl::StrSplit(word_set_spec, ',')) {
      if (syms) {
        auto l = syms->Find(std::string(w));
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
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  int max_order = 0;
  for (int o : orders) max_order = std::max(max_order, o);
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
  for (StateId s : backoff_states) {
    if (orders[s] == 1) {
      unigram_state = s;
      break;
    }
  }
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
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
        bool still_ascending = origin_state == fst::kNoStateId ||
                               orders[origin_state] < orders[curr_state];
        matcher.SetState(curr_state);
        if (still_ascending && matcher.Find(label)) {
          Arc arc = matcher.Value();
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
    int state_order = orders[s];
    if (state_order >= 0 && !state_on_prune_path[s])
      order_states[state_order].push_back(s);
  }
  for (int order = 1; order <= max_order; ++order) {
    for (auto s : order_states[order]) {
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
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) arcs.push_back(arc);
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) fst->AddArc(s, arc);
  }
  return PhiNormalize(fst, phi_label);
}

// Significance-based pruning method of Moore and Quirk (EMNLP 2009).
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
  using Label = typename Arc::Label;
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
        *count_fst, fst::MATCH_INPUT);
  }

  auto distance = [](double x, double lower, double upper) {
    if (x < lower) return lower - x;
    if (x > upper) return x - upper;
    return 0.0;
  };

  std::vector<std::pair<StateId, Label>> to_prune;
  fst::Matcher<fst::Fst<Arc>> matcher(*fst, fst::MATCH_INPUT);
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (probs[s] == 0.0) continue;
    double log_prob_s = std::log(probs[s]);
    StateId bo = -1;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        bo = arc.nextstate;
        break;
      }
    }
    if (bo == -1) continue;
    double hi_neglog_sum = fst->Final(s).Value();
    double low_neglog_sum = fst->Final(bo).Value();
    matcher.SetState(bo);
    double KahanVal1 = 0;
    double KahanVal2 = 0;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      hi_neglog_sum = NegLogSum(hi_neglog_sum, arc.weight.Value(), &KahanVal1);
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        low_neglog_sum =
            NegLogSum(low_neglog_sum, barc.weight.Value(), &KahanVal2);
      }
    }
    double nlog_backoff_num = 0.0;
    double nlog_backoff_denom = 0.0;
    double effective_zero = kNormEps * kFloatEps;
    double effective_nlog_zero = 99.0;
    double tmp_hi = hi_neglog_sum;
    double tmp_low = low_neglog_sum;
    if (tmp_hi < effective_zero) tmp_hi = effective_zero;
    if (tmp_low < effective_zero) tmp_low = effective_zero;
    if (tmp_low > 0 && tmp_hi > 0) {
      nlog_backoff_num =
          (tmp_hi > effective_nlog_zero) ? 0.0 : NegLogDiff(0.0, tmp_hi);
      nlog_backoff_denom =
          (tmp_low > effective_nlog_zero) ? 0.0 : NegLogDiff(0.0, tmp_low);
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      if (matcher.Find(arc.ilabel)) {
        const auto& barc = matcher.Value();
        double log_prob = -arc.weight.Value();
        double log_backoff_prob = -barc.weight.Value();
        double new_log_backoff =
            NegLogSum(nlog_backoff_denom, barc.weight.Value()) -
            NegLogSum(nlog_backoff_num, arc.weight.Value());

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

        double lower = arc_count / (state_count + 1.0);
        double upper = (arc_count + 1.0) / (state_count + 1.0);
        double backoff_prob = std::exp(log_backoff_prob + new_log_backoff);
        double prob = std::exp(log_prob);
        double dist_bo = distance(backoff_prob, lower, upper);
        double dist_curr = distance(prob, lower, upper);
        if ((dist_bo < kFloatEps) || (dist_bo < dist_curr + kFloatEps)) {
          to_prune.push_back({s, arc.ilabel});
        }
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) arcs.push_back(arc);
    }
    fst->DeleteArcs(s);
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
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
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

  auto get_state_ngram = [&](StateId st, auto& self) -> std::vector<Label> {
    std::vector<Label> ngram;
    if (st >= 0 && st < fst->NumStates() &&
        prefix_state[st] != fst::kNoStateId) {
      ngram = self(prefix_state[st], self);
      ngram.push_back(incoming_label[st]);
    }
    return ngram;
  };

  std::vector<std::pair<StateId, Label>> to_prune;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    if (orders[s] <= 1) continue;
    std::vector<Label> state_ngram = get_state_ngram(s, get_state_ngram);
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      std::vector<Label> ngram = state_ngram;
      ngram.push_back(arc.ilabel);
      bool keep = false;
      for (Label l : ngram) {
        if (word_set.contains(l)) {
          keep = true;
          break;
        }
      }
      if (!keep) {
        to_prune.push_back({s, arc.ilabel});
      }
    }
  }
  for (const auto& p : to_prune) {
    StateId s = p.first;
    Label l = p.second;
    std::vector<Arc> arcs;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != l) arcs.push_back(arc);
    }
    fst->DeleteArcs(s);
    for (const auto& arc : arcs) fst->AddArc(s, arc);
  }
  return true;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_SHRINK_H_
