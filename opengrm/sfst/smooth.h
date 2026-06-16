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

// Smoothing algorithms for SFST.

#ifndef OPENGRM_SFST_SMOOTH_H_
#define OPENGRM_SFST_SMOOTH_H_

#include <stdio.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "absl/log/log.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/sfst.h"

namespace sfst {
namespace internal {

template <class Arc>
inline void GetStateCountData(const fst::Fst<Arc>& fst, typename Arc::StateId s,
                              typename Arc::Label phi_label,
                              typename Arc::Weight* c_h_weight,
                              ssize_t* phi_pos, size_t* T_h) {
  using Weight = typename Arc::Weight;
  *c_h_weight = Weight::Zero();
  *phi_pos = -1;
  *T_h = 0;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label) {
      *c_h_weight = arc.weight;
      *phi_pos = aiter.Position();
    } else {
      ++(*T_h);
    }
  }
  if (fst.Final(s) != Weight::Zero()) {
    ++(*T_h);
  }
}

}  // namespace internal

// Unsmoothed: just normalize counts.
// Uses CountNormalize with NORM_SUMMED which does exactly this for backoff
// topologies.
template <class Arc>
bool Unsmoothed(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "Unsmoothed: input is not a canonical SFST";
    return false;
  }
  return CountNormalize(fst, phi_label, NORM_SUMMED);
}

// Witten-Bell smoothing.
// Assumes input FST has counts on arcs, and the sum of counts is stored on the
// phi arc (as done by NGramCounter::StateCounts).
template <class Arc>
bool WittenBell(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
                double k = 1.0) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "WittenBell: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  const fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  const fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    Weight c_h_weight;
    ssize_t phi_pos;
    size_t T_h;
    internal::GetStateCountData(*fst, s, phi_label, &c_h_weight, &phi_pos,
                                &T_h);
    if (phi_pos == -1) {
      if (fst->NumArcs(s) > 0 || fst->Final(s) != Weight::Zero()) {
        if (!LocalNormalizeState(s, fst)) return false;
      }
      continue;
    }
    double c_h = to_log64(c_h_weight).Value();
    // Counts are assumed to be stored as negative log counts.
    double c_h_val = std::exp(-c_h);
    double denominator = c_h_val + k * T_h;
    double log_denominator = std::log(denominator);
    double backoff_weight = (k * T_h > 0) ? -std::log((k * T_h) / denominator)
                                          : fst::Log64Weight::Zero().Value();
    // Updates arcs.
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        arc.weight = from_log64(fst::Log64Weight(backoff_weight));
      } else {
        double w = to_log64(arc.weight).Value();
        arc.weight = from_log64(fst::Log64Weight(w + log_denominator));
      }
      aiter.SetValue(arc);
    }
    // Updates final weight.
    if (fst->Final(s) != Weight::Zero()) {
      double w = to_log64(fst->Final(s)).Value();
      fst->SetFinal(s, from_log64(fst::Log64Weight(w + log_denominator)));
    }
  }
  return true;
}

// Absolute Discounting smoothing.
// Assumes input FST has counts on arcs, and the sum of counts is stored on the
// phi arc (as done by NGramCounter::StateCounts).
template <class Arc>
bool AbsoluteDiscounting(fst::MutableFst<Arc>* fst,
                         typename Arc::Label phi_label, double D = 0.75) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "AbsoluteDiscounting: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  const fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  const fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    Weight c_h_weight;
    ssize_t phi_pos;
    size_t T_h;
    internal::GetStateCountData(*fst, s, phi_label, &c_h_weight, &phi_pos,
                                &T_h);
    if (phi_pos == -1) {
      if (fst->NumArcs(s) > 0 || fst->Final(s) != Weight::Zero()) {
        if (!LocalNormalizeState(s, fst)) return false;
      }
      continue;
    }
    double c_h = to_log64(c_h_weight).Value();
    double c_h_val = std::exp(-c_h);
    double discounted_sum = 0;
    // Removes discount D from each seen transition.
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel != phi_label) {
        double c_hw = std::exp(-to_log64(arc.weight).Value());
        double discounted_c = std::max(c_hw - D, 0.0);
        discounted_sum += discounted_c;
        arc.weight = from_log64(fst::Log64Weight(-std::log(discounted_c)));
        aiter.SetValue(arc);
      }
    }
    double final_discounted_c = 0;
    if (fst->Final(s) != Weight::Zero()) {
      double c_h_final = std::exp(-to_log64(fst->Final(s)).Value());
      final_discounted_c = std::max(c_h_final - D, 0.0);
      discounted_sum += final_discounted_c;
    }
    double backoff_mass = c_h_val - discounted_sum;
    if (backoff_mass < 0) backoff_mass = 0;  // Handles float imprecision.
    double backoff_weight = (backoff_mass > 0)
                                ? -std::log(backoff_mass) - c_h
                                : fst::Log64Weight::Zero().Value();
    // Normalizes probabilities.
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        arc.weight = from_log64(fst::Log64Weight(backoff_weight));
      } else {
        double w = to_log64(arc.weight).Value();
        arc.weight = from_log64(fst::Log64Weight(w - c_h));
      }
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != Weight::Zero()) {
      double final_w = (final_discounted_c > 0)
                           ? -std::log(final_discounted_c) - c_h
                           : fst::Log64Weight::Zero().Value();
      fst->SetFinal(s, from_log64(fst::Log64Weight(final_w)));
    }
  }
  return true;
}

// Pre-smoothed: normalize by state count, leaving remainder in backoff.
template <class Arc>
bool PreSmoothed(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "PreSmoothed: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  const fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  const fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    Weight c_h_weight = Weight::Zero();
    ssize_t phi_pos = -1;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        c_h_weight = arc.weight;
        phi_pos = aiter.Position();
        break;
      }
    }
    if (phi_pos == -1) {
      if (fst->NumArcs(s) > 0 || fst->Final(s) != Weight::Zero()) {
        if (!LocalNormalizeState(s, fst)) return false;
      }
      continue;
    }
    double c_h = to_log64(c_h_weight).Value();
    double c_h_val = std::exp(-c_h);
    double outgoing_sum = 0;
    // Computes sum of counts of outgoing arcs.
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel != phi_label) {
        outgoing_sum += std::exp(-to_log64(arc.weight).Value());
      }
    }
    if (fst->Final(s) != Weight::Zero()) {
      outgoing_sum += std::exp(-to_log64(fst->Final(s)).Value());
    }
    double backoff_mass = c_h_val - outgoing_sum;
    if (backoff_mass < 0) backoff_mass = 0;  // Handles float imprecision.
    double backoff_weight = (backoff_mass > 0)
                                ? -std::log(backoff_mass) - c_h
                                : fst::Log64Weight::Zero().Value();
    // Normalizes probabilities by state count.
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        arc.weight = from_log64(fst::Log64Weight(backoff_weight));
      } else {
        double w = to_log64(arc.weight).Value();
        arc.weight = from_log64(fst::Log64Weight(w - c_h));
      }
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != Weight::Zero()) {
      double w = to_log64(fst->Final(s)).Value();
      fst->SetFinal(s, from_log64(fst::Log64Weight(w - c_h)));
    }
  }
  return true;
}

// Kneser-Ney smoothing.
// Assumes input FST has counts on arcs, and the sum of counts is stored on the
// phi arc.
template <class Arc>
bool KneserNey(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
               double D = 0.75) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "KneserNey: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  const fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  const fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  const double log_kNormEps = -std::log(kNormEps);
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  int max_order = 0;
  for (int o : orders) max_order = std::max(max_order, o);
  fst::ExplicitMatcher<fst::SortedMatcher<fst::MutableFst<Arc>>> matcher(
      fst, fst::MATCH_INPUT);
  // Step 1: Order reduction.
  for (int order = 2; order <= max_order; ++order) {
    for (StateId s = 0; s < fst->NumStates(); ++s) {
      if (orders[s] != order) continue;
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
      // Order reduction for arcs.
      matcher.SetState(bo);
      fst::MutableArcIterator<fst::MutableFst<Arc>> biter(fst, bo);
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel == phi_label) continue;
        if (matcher.Find(arc.ilabel)) {
          biter.Seek(matcher.GetMatcher()->Position());
          auto barc = biter.Value();
          double lo_val = to_log64(barc.weight).Value();
          double hi_val = to_log64(arc.weight).Value();
          double new_val = NegLogDiff(lo_val, hi_val);
          if (new_val > log_kNormEps) {
            new_val = fst::Log64Weight::Zero().Value();
          }
          barc.weight = from_log64(fst::Log64Weight(new_val));
          biter.SetValue(barc);
        }
      }
      // Order reduction for final weights.
      if (fst->Final(s) != Weight::Zero() && fst->Final(bo) != Weight::Zero()) {
        double lo_val = to_log64(fst->Final(bo)).Value();
        double hi_val = to_log64(fst->Final(s)).Value();
        double new_val = NegLogDiff(lo_val, hi_val);
        if (new_val > log_kNormEps) {
          new_val = fst::Log64Weight::Zero().Value();
        }
        fst->SetFinal(bo, from_log64(fst::Log64Weight(new_val)));
      }
    }
  }
  // Step 2: Context accumulation.
  for (int order = max_order; order > 1; --order) {
    for (StateId s = 0; s < fst->NumStates(); ++s) {
      if (orders[s] != order) continue;
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
      // Context accumulation for arcs.
      matcher.SetState(bo);
      fst::MutableArcIterator<fst::MutableFst<Arc>> biter(fst, bo);
      for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel == phi_label) continue;
        if (matcher.Find(arc.ilabel)) {
          biter.Seek(matcher.GetMatcher()->Position());
          auto barc = biter.Value();
          double lo_val = to_log64(barc.weight).Value();
          double new_val = NegLogSum(lo_val, 0.0);
          barc.weight = from_log64(fst::Log64Weight(new_val));
          biter.SetValue(barc);
        }
      }
      // Context accumulation for final weights.
      if (fst->Final(s) != Weight::Zero()) {
        double lo_val = to_log64(fst->Final(bo)).Value();
        double new_val = NegLogSum(lo_val, 0.0);
        fst->SetFinal(bo, from_log64(fst::Log64Weight(new_val)));
      }
    }
  }
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    double sum = to_log64(fst->Final(s)).Value();
    ssize_t phi_pos = -1;
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel != phi_label) {
        sum = NegLogSum(sum, to_log64(arc.weight).Value());
      } else {
        phi_pos = aiter.Position();
      }
    }
    if (phi_pos != -1) {
      fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
      aiter.Seek(phi_pos);
      auto arc = aiter.Value();
      arc.weight = from_log64(fst::Log64Weight(sum));
      aiter.SetValue(arc);
    }
  }
  return AbsoluteDiscounting(fst, phi_label, D);
}

// Katz smoothing.
// Assumes input FST has counts on arcs, and the sum of counts is stored on the
// phi arc.
template <class Arc>
bool Katz(fst::MutableFst<Arc>* fst, typename Arc::Label phi_label,
          int bins = 5) {
  if (!IsCanonical(*fst, phi_label)) {
    LOG(ERROR) << "Katz: input is not a canonical SFST";
    return false;
  }
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  const fst::WeightConvert<fst::Log64Weight, Weight> from_log64;
  const fst::WeightConvert<Weight, fst::Log64Weight> to_log64;
  std::vector<int> orders;
  PhiStateOrder(*fst, phi_label, &orders);
  int max_order = 0;
  for (int o : orders) max_order = std::max(max_order, o);
  // Accumulates count-of-counts.
  std::vector<std::vector<double>> count_of_counts(
      max_order + 1, std::vector<double>(bins + 2, 0.0));
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    int order = orders[s];
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      double count = std::exp(-to_log64(arc.weight).Value());
      int r = std::round(count);
      if (r >= 1 && r <= bins + 1) count_of_counts[order][r] += 1.0;
    }
    if (fst->Final(s) != Weight::Zero()) {
      double count = std::exp(-to_log64(fst->Final(s)).Value());
      int r = std::round(count);
      if (r >= 1 && r <= bins + 1) count_of_counts[order][r] += 1.0;
    }
  }
  // Calculates discounts.
  std::vector<std::vector<double>> discounts(
      max_order + 1, std::vector<double>(bins + 1, 1.0));
  for (int order = 1; order <= max_order; ++order) {
    double n1 = count_of_counts[order][1];
    double n_bins_plus1 = count_of_counts[order][bins + 1];
    double rnorm = 0.0;
    if (n1 > 0) rnorm = (bins + 1) * n_bins_plus1 / n1;
    for (int r = 1; r <= bins; ++r) {
      double nr = count_of_counts[order][r];
      double nr_plus1 = count_of_counts[order][r + 1];
      if (nr > 0) {
        double rstar = (r + 1) * nr_plus1 / nr;
        discounts[order][r] = (rstar / r - rnorm) / (1.0 - rnorm);
        if (discounts[order][r] <= 0.0 || discounts[order][r] >= 1.0) {
          discounts[order][r] = 1.0 - 0.001;
        }
      }
    }
  }
  // Applies discounts and normalizes.
  for (StateId s = 0; s < fst->NumStates(); ++s) {
    int order = orders[s];
    Weight c_h_weight = Weight::Zero();
    ssize_t phi_pos = -1;
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(*fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        c_h_weight = arc.weight;
        phi_pos = aiter.Position();
        break;
      }
    }
    if (phi_pos == -1) {
      if (fst->NumArcs(s) > 0 || fst->Final(s) != Weight::Zero()) {
        if (!LocalNormalizeState(s, fst)) return false;
      }
      continue;
    }
    double c_h = to_log64(c_h_weight).Value();
    double c_h_val = std::exp(-c_h);
    double discounted_sum = 0;
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel != phi_label) {
        double count = std::exp(-to_log64(arc.weight).Value());
        int r = std::round(count);
        double d = 1.0;
        if (r >= 1 && r <= bins) d = discounts[order][r];
        double discounted_c = count * d;
        discounted_sum += discounted_c;
        arc.weight = from_log64(fst::Log64Weight(-std::log(discounted_c)));
        aiter.SetValue(arc);
      }
    }
    double final_discounted_c = 0;
    if (fst->Final(s) != Weight::Zero()) {
      double count = std::exp(-to_log64(fst->Final(s)).Value());
      int r = std::round(count);
      double d = 1.0;
      if (r >= 1 && r <= bins) d = discounts[order][r];
      final_discounted_c = count * d;
      discounted_sum += final_discounted_c;
    }
    double backoff_mass = c_h_val - discounted_sum;
    if (backoff_mass < 0) backoff_mass = 0;
    double backoff_weight = (backoff_mass > 0)
                                ? -std::log(backoff_mass) - c_h
                                : fst::Log64Weight::Zero().Value();
    for (fst::MutableArcIterator<fst::MutableFst<Arc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      auto arc = aiter.Value();
      if (arc.ilabel == phi_label) {
        arc.weight = from_log64(fst::Log64Weight(backoff_weight));
      } else {
        double w = to_log64(arc.weight).Value();
        arc.weight = from_log64(fst::Log64Weight(w - c_h));
      }
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != Weight::Zero()) {
      double final_w = (final_discounted_c > 0)
                           ? -std::log(final_discounted_c) - c_h
                           : fst::Log64Weight::Zero().Value();
      fst->SetFinal(s, from_log64(fst::Log64Weight(final_w)));
    }
  }
  return true;
}

}  // namespace sfst

#endif  // OPENGRM_SFST_SMOOTH_H_
