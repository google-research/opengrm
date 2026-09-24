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

#include "opengrm/sfst/arpa.h"

#include <algorithm>  // NOLINT(misc-include-cleaner)
#include <cmath>
#include <cstddef>
#include <iostream>
#include <istream>
#include <map>  // NOLINT(misc-include-cleaner)
#include <ostream>
#include <queue>    // NOLINT(misc-include-cleaner)
#include <string>
#include <utility>
#include <vector>  // NOLINT(misc-include-cleaner)

#include "absl/container/flat_hash_map.h"  // NOLINT(misc-include-cleaner)
#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"   // NOLINT(misc-include-cleaner)
#include "absl/strings/str_split.h"  // NOLINT(misc-include-cleaner)
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"         // NOLINT(misc-include-cleaner)
#include "openfst/lib/arcsort.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/sfst/canonical.h"

namespace sfst {
namespace internal {

template <typename Label>
inline std::vector<Label> GetSourceHistory(const std::vector<Label>& ngram,
                                           int max_hist_len) {
  return std::vector<Label>(
      ngram.begin(),
      ngram.begin() +
          std::min(static_cast<int>(ngram.size()) - 1, max_hist_len));
}

template <typename Label>
inline std::vector<Label> GetDestinationHistory(const std::vector<Label>& ngram,
                                                int max_hist_len) {
  return std::vector<Label>(
      ngram.end() - std::min(static_cast<int>(ngram.size()), max_hist_len),
      ngram.end());
}

template <typename Label>
inline void EnsureSuffixHistoriesExist(
    absl::flat_hash_set<std::vector<Label>>& unique_histories,
    const std::vector<Label>& hist) {
  for (size_t len = 1; len < hist.size(); ++len) {
    unique_histories.emplace(hist.end() - len, hist.end());
  }
}

template <class Arc>
inline bool GetBackoffWeight(const fst::Fst<Arc>& fst, typename Arc::StateId s,
                             typename Arc::Label phi_label, double* bo_weight) {
  *bo_weight = 0.0;
  for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == phi_label) {
      *bo_weight = -arc.weight.Value();
      return true;
    }
  }
  return false;
}

inline bool IsBosSymbol(absl::string_view token) {
  return token == "<s>" || token == "<S>" || token == "<bos>" ||
         token == "<BOS>" || token == "[BOS]";
}

inline bool IsEosSymbol(absl::string_view token) {
  return token == "</s>" || token == "</S>" || token == "<eos>" ||
         token == "<EOS>" || token == "[EOS]";
}

struct NgramData {
  double log_prob = 0.0;
  double backoff_weight = 0.0;
  bool has_log_prob = false;
  bool has_backoff = false;
};

template <typename Label>
double ResolveLogProb(
    const std::vector<Label>& ngram, const NgramData& data,
    bool collapse_leaf_histories,
    const absl::flat_hash_map<std::vector<Label>, NgramData>& all_ngrams) {
  if (data.has_log_prob || !collapse_leaf_histories) return data.log_prob;
  double accum = 0.0;
  std::vector<Label> cur_hist(ngram.begin(), ngram.end() - 1);
  const Label word = ngram.back();
  while (!cur_hist.empty()) {
    auto bo_it = all_ngrams.find(cur_hist);
    if (bo_it != all_ngrams.end() && bo_it->second.has_backoff) {
      accum += bo_it->second.backoff_weight;
    }
    cur_hist.erase(cur_hist.begin());
    std::vector<Label> candidate = cur_hist;
    candidate.push_back(word);
    auto it = all_ngrams.find(candidate);
    if (it != all_ngrams.end() && it->second.has_log_prob) {
      accum += it->second.log_prob;
      break;
    }
  }
  return accum;
}

}  // namespace internal

template <class Arc>
bool ReadArpa(std::istream& istrm, fst::MutableFst<Arc>* fst,
              typename Arc::Label phi_label) {
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  using internal::NgramData;
  constexpr Label kBosLabel = -2;
  constexpr Label kEosLabel = -3;

  fst->DeleteStates();
  if (!fst->InputSymbols()) {
    fst::SymbolTable syms("ARPASymbols");
    syms.AddSymbol("<epsilon>");
    fst->SetInputSymbols(&syms);
  }
  fst::SymbolTable* syms = fst->MutableInputSymbols();
  absl::flat_hash_map<std::vector<Label>, NgramData> all_ngrams;
  absl::flat_hash_map<std::vector<Label>, double> final_weights;
  bool has_bos = false;
  bool has_eos = false;
  int current_order = 0;
  bool in_ngrams = false;
  bool error = false;
  std::string line;
  std::vector<absl::string_view> parts;
  // Collects all n-grams and explicitly populates implied lower-order gaps.
  while (std::getline(istrm, line)) {
    if (line.empty()) continue;
    if (line == "\\end\\") break;
    if (line[0] == '\\') {
      size_t grams_pos = line.find("-grams:");
      if (grams_pos != std::string::npos && grams_pos > 1) {
        in_ngrams = true;
        absl::string_view order_str =
            absl::string_view(line).substr(1, grams_pos - 1);
        if (!absl::SimpleAtoi(order_str, &current_order) ||
            current_order <= 0) {
          LOG(ERROR) << "ReadArpa: Invalid order in header: " << line;
          error = true;
          current_order = 0;
        }
      }
      continue;
    }
    if (!in_ngrams || current_order <= 0) continue;
    parts = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipEmpty());
    if (parts.size() < current_order + 1) {
      LOG(ERROR) << "ReadArpa: Insufficient tokens for order " << current_order
                 << ": " << line;
      error = true;
      continue;
    }
    double log_prob = 0.0;
    if (!absl::SimpleAtod(parts[0], &log_prob)) {
      LOG(ERROR) << "ReadArpa: Invalid log probability: " << parts[0];
      error = true;
      continue;
    }
    std::vector<Label> ngram;
    ngram.reserve(current_order);
    for (int i = 1; i <= current_order && i < parts.size(); ++i) {
      if (internal::IsBosSymbol(parts[i])) {
        ngram.push_back(kBosLabel);
        has_bos = true;
      } else if (internal::IsEosSymbol(parts[i])) {
        ngram.push_back(kEosLabel);
        has_eos = true;
      } else {
        ngram.push_back(syms->AddSymbol(parts[i]));
      }
    }
    double boweight = 0.0;
    bool has_bo = false;
    if (parts.size() > current_order + 1) {
      if (absl::SimpleAtod(parts[current_order + 1], &boweight)) {
        has_bo = true;
      } else {
        LOG(ERROR) << "ReadArpa: Invalid backoff weight: "
                   << parts[current_order + 1];
        error = true;
      }
    }
    if (ngram.back() == kEosLabel) {
      std::vector<Label> hist(ngram.begin(), ngram.end() - 1);
      final_weights[hist] = log_prob * std::log(10.0);
      for (size_t len = 1; len <= hist.size(); ++len) {
        for (size_t start = 0; start <= hist.size() - len; ++start) {
          std::vector<Label> sub(hist.begin() + start,
                                 hist.begin() + start + len);
          all_ngrams.try_emplace(std::move(sub),
                                 NgramData{0.0, 0.0, false, false});
        }
      }
      continue;
    }
    if (ngram.size() == 1 && ngram[0] == kBosLabel) {
      auto& ngram_data = all_ngrams[ngram];
      if (has_bo) {
        ngram_data.backoff_weight = boweight * std::log(10.0);
        ngram_data.has_backoff = true;
      }
      continue;
    }
    auto& ngram_data = all_ngrams[ngram];
    ngram_data.log_prob = log_prob * std::log(10.0);
    ngram_data.has_log_prob = true;
    if (has_bo) {
      ngram_data.backoff_weight = boweight * std::log(10.0);
      ngram_data.has_backoff = true;
    }
    for (int len = 1; len < current_order; ++len) {
      for (int start = 0; start <= current_order - len; ++start) {
        std::vector<Label> sub(ngram.begin() + start,
                               ngram.begin() + start + len);
        all_ngrams.try_emplace(std::move(sub),
                               NgramData{0.0, 0.0, false, false});
      }
    }
  }
  // Determines the actual maximum model order dynamically from the collected
  // data.
  int actual_max_order = 1;
  for (const auto& pair : all_ngrams) {
    if (pair.first.size() > actual_max_order) {
      actual_max_order = pair.first.size();
    }
  }
  for (const auto& pair : final_weights) {
    if (pair.first.size() + 1 > actual_max_order) {
      actual_max_order = pair.first.size() + 1;
    }
  }
  const int max_hist_len = actual_max_order - 1;
  const bool collapse_leaf_histories = has_bos || has_eos;
  // Collects all unique histories that require states in the canonical FST.
  absl::flat_hash_set<std::vector<Label>> unique_histories;
  unique_histories.insert(std::vector<Label>());
  if (has_bos && max_hist_len >= 1) {
    unique_histories.insert(std::vector<Label>{kBosLabel});
  }
  for (const auto& pair : all_ngrams) {
    const auto& ngram = pair.first;
    if (ngram.size() == 1 && ngram[0] == kBosLabel) continue;
    const auto src_hist = internal::GetSourceHistory(ngram, max_hist_len);
    unique_histories.insert(src_hist);
    internal::EnsureSuffixHistoriesExist(unique_histories, src_hist);
    if (!collapse_leaf_histories) {
      const auto dst_hist =
          internal::GetDestinationHistory(ngram, max_hist_len);
      unique_histories.insert(dst_hist);
      internal::EnsureSuffixHistoriesExist(unique_histories, dst_hist);
    }
  }
  for (const auto& pair : final_weights) {
    const auto src_hist =
        internal::GetDestinationHistory(pair.first, max_hist_len);
    unique_histories.insert(src_hist);
    internal::EnsureSuffixHistoriesExist(unique_histories, src_hist);
  }

  // Sorts histories deterministically: first by length (order), then
  // lexicographically by label sequence. This guarantees identical state
  // numbering and serialization across runs.
  std::vector<std::vector<Label>> sorted_histories(unique_histories.begin(),
                                                   unique_histories.end());
  std::sort(sorted_histories.begin(), sorted_histories.end(),
            [](const std::vector<Label>& a, const std::vector<Label>& b) {
              if (a.size() != b.size()) return a.size() < b.size();
              return a < b;
            });

  // Allocates FST states deterministically.
  fst->DeleteStates();
  fst->AddStates(sorted_histories.size());
  absl::flat_hash_map<std::vector<Label>, StateId> history_to_state;
  history_to_state.reserve(sorted_histories.size());
  for (size_t i = 0; i < sorted_histories.size(); ++i) {
    history_to_state[sorted_histories[i]] = static_cast<StateId>(i);
  }
  if (has_bos && max_hist_len >= 1) {
    fst->SetStart(history_to_state[std::vector<Label>{kBosLabel}]);
  } else {
    fst->SetStart(history_to_state[std::vector<Label>()]);
  }

  // Sets final weights from EOS n-grams.
  for (const auto& [hist, fw] : final_weights) {
    const auto src_hist = internal::GetDestinationHistory(hist, max_hist_len);
    fst->SetFinal(history_to_state[src_hist], Weight(-fw));
  }

  // Instantiates all word transitions in the FST.
  for (const auto& pair : all_ngrams) {
    const auto& ngram = pair.first;
    if (ngram.size() == 1 && ngram[0] == kBosLabel) continue;
    const auto& data = pair.second;
    const auto src_hist = internal::GetSourceHistory(ngram, max_hist_len);
    auto dst_hist = internal::GetDestinationHistory(ngram, max_hist_len);
    while (!history_to_state.contains(dst_hist)) {
      dst_hist.erase(dst_hist.begin());
    }
    const Label word = ngram.back();
    StateId src = history_to_state[src_hist];
    StateId dst = history_to_state[dst_hist];
    const double resolved_log_prob = internal::ResolveLogProb(
        ngram, data, collapse_leaf_histories, all_ngrams);
    fst->AddArc(src, Arc(word, word, Weight(-resolved_log_prob), dst));
  }

  // Instantiates all backoff transitions in deterministic state order using
  // phi_label.
  for (const auto& hist : sorted_histories) {
    if (hist.empty()) continue;
    const StateId src = history_to_state[hist];
    std::vector<Label> bo_hist(hist.begin() + 1, hist.end());
    StateId dst = history_to_state[bo_hist];
    double bo_weight = 0.0;
    auto it = all_ngrams.find(hist);
    if (it != all_ngrams.end() && it->second.has_backoff) {
      bo_weight = it->second.backoff_weight;
    }
    fst->AddArc(src, Arc(phi_label, phi_label, Weight(-bo_weight), dst));
  }
  fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  fst->SetOutputSymbols(fst->InputSymbols());
  return !error;
}

namespace internal {

struct ArpaNgramPrintData {
  std::string text;
  double log_prob = 0.0;
  double backoff_weight = 0.0;
  bool has_backoff = false;
};

template <class Arc>
bool WriteNGrams(const fst::Fst<Arc>& fst, std::ostream& ostrm,
                 typename Arc::Label phi_label, bool arpa_format) {
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  if (!fst.InputSymbols()) {
    LOG(ERROR) << "WriteArpa: FST has no input symbols";
    return false;
  }
  ostrm.precision(7);
  const StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId) return true;
  const fst::SymbolTable* syms = fst.InputSymbols();
  Label bos_label = syms->Find("<s>");
  if (bos_label == fst::kNoLabel) bos_label = syms->Find("<S>");
  if (bos_label == fst::kNoLabel) bos_label = syms->Find("<bos>");
  if (bos_label == fst::kNoLabel) bos_label = syms->Find("<BOS>");
  if (bos_label == fst::kNoLabel) bos_label = syms->Find("[BOS]");
  Label eos_label = syms->Find("</s>");
  if (eos_label == fst::kNoLabel) eos_label = syms->Find("</S>");
  if (eos_label == fst::kNoLabel) eos_label = syms->Find("<eos>");
  if (eos_label == fst::kNoLabel) eos_label = syms->Find("<EOS>");
  if (eos_label == fst::kNoLabel) eos_label = syms->Find("[EOS]");
  if (eos_label == fst::kNoLabel) {
    LOG(WARNING)
        << "WriteArpa: No standard EOS symbol (</s>, </S>, <eos>, <EOS>, "
        << "[EOS]) found in input symbol table; defaulting to </s>.";
  }
  const std::string bos_str =
      (bos_label != fst::kNoLabel) ? syms->Find(bos_label) : "<s>";
  const std::string eos_str =
      (eos_label != fst::kNoLabel) ? syms->Find(eos_label) : "</s>";
  // Finds unigram state from start_state's phi arc.
  StateId unigram_state = fst::kNoStateId;
  if (phi_label != fst::kNoLabel) {
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, start_state); !aiter.Done();
         aiter.Next()) {
      if (aiter.Value().ilabel == phi_label) {
        unigram_state = aiter.Value().nextstate;
        break;
      }
    }
  }
  // Computes state orders via canonical PhiStateOrder.
  std::vector<int> state_orders;
  int model_max_order = PhiStateOrder(fst, phi_label, &state_orders);
  // Computes shortest word-path distances to determine canonical history
  // vectors for states.
  std::map<StateId, std::vector<Label>> history;
  std::map<StateId, int> dist;
  std::queue<StateId> q;
  if (unigram_state != fst::kNoStateId && unigram_state != start_state) {
    history[unigram_state] = std::vector<Label>();
    dist[unigram_state] = 0;
    q.push(unigram_state);
    history[start_state] = {bos_label != fst::kNoLabel ? bos_label : -2};
    dist[start_state] = 1;
    q.push(start_state);
  } else {
    history[start_state] = std::vector<Label>();
    dist[start_state] = 0;
    q.push(start_state);
  }
  // Traverses the FST to map each reachable state to its unique history
  // vector.
  while (!q.empty()) {
    StateId u = q.front();
    q.pop();
    int d = dist[u];
    const auto& H = history[u];
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, u); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
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
  // Precomputes backoff weights for all states to avoid O(E * d) inner loops.
  absl::flat_hash_map<StateId, double> state_backoff_weights;
  if (phi_label != fst::kNoLabel) {
    for (const auto& pair : history) {
      const StateId s = pair.first;
      double bw = 0.0;
      if (internal::GetBackoffWeight(fst, s, phi_label, &bw)) {
        state_backoff_weights[s] = bw;
      }
    }
  }
  std::map<int, std::vector<ArpaNgramPrintData>> order_to_ngrams;
  if (bos_label != fst::kNoLabel ||
      (unigram_state != fst::kNoStateId && unigram_state != start_state)) {
    ArpaNgramPrintData bos_data;
    bos_data.text = bos_str;
    bos_data.log_prob = -99.0;
    auto bo_it = state_backoff_weights.find(start_state);
    if (bo_it != state_backoff_weights.end()) {
      bos_data.backoff_weight = bo_it->second / std::log(10.0);
      bos_data.has_backoff = true;
    } else {
      bos_data.text += '\t';
    }
    order_to_ngrams[1].push_back(bos_data);
  }
  auto label_to_str = [&syms, &bos_str](Label l) -> std::string {
    if (l == -2) return bos_str;
    return syms->Find(l);
  };
  // Serializes all word and backoff transitions into the ARPA format data
  // structures.
  for (const auto& pair : history) {
    const StateId s = pair.first;
    const auto& H = pair.second;
    const int order = H.size() + 1;
    std::string history_prefix =
        absl::StrJoin(H, " ", [&label_to_str](std::string* out, Label l) {
          out->append(label_to_str(l));
        });
    if (!history_prefix.empty()) history_prefix.push_back(' ');
    if (fst.Final(s) != Weight::Zero()) {
      ArpaNgramPrintData data;
      data.text = absl::StrCat(history_prefix, eos_str);
      data.log_prob = -fst.Final(s).Value() / std::log(10.0);
      order_to_ngrams[order].push_back(std::move(data));
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      ArpaNgramPrintData data;
      data.text = absl::StrCat(history_prefix, label_to_str(arc.ilabel));
      data.log_prob = -arc.weight.Value() / std::log(10.0);
      auto bo_it = state_backoff_weights.find(arc.nextstate);
      if (bo_it != state_backoff_weights.end()) {
        data.backoff_weight = bo_it->second / std::log(10.0);
        data.has_backoff = true;
      }
      order_to_ngrams[order].push_back(std::move(data));
    }
  }
  int max_order = std::max(model_max_order, 1);
  for (const auto& [order, ngrams] : order_to_ngrams) {
    if (order > max_order) max_order = order;
  }
  // Clears backoff from highest order n-grams.
  if (max_order > 0) {
    for (auto& data : order_to_ngrams[max_order]) {
      data.has_backoff = false;
    }
  }
  if (arpa_format) {
    // Prints and formats the final ARPA LM to the output stream.
    ostrm << "\\data\\\n";
    for (int o = 1; o <= max_order; ++o) {
      ostrm << "ngram " << o << "=" << order_to_ngrams[o].size() << '\n';
    }
    for (int o = 1; o <= max_order; ++o) {
      ostrm << "\\" << o << "-grams:\n";
      for (const auto& data : order_to_ngrams[o]) {
        ostrm << data.log_prob << '\t' << data.text;
        if (data.has_backoff) {
          ostrm << '\t' << data.backoff_weight;
        }
        ostrm << '\n';
      }
    }
    ostrm << "\\end\\\n";
  } else {
    // Prints plain TSV format.
    for (int o = 1; o <= max_order; ++o) {
      for (const auto& data : order_to_ngrams[o]) {
        ostrm << data.text << '\t' << data.log_prob;
        if (data.has_backoff) {
          ostrm << '\t' << data.backoff_weight;
        }
        ostrm << '\n';
      }
    }
  }
  return true;
}

}  // namespace internal

template <class Arc>
bool WriteArpa(const fst::Fst<Arc>& fst, std::ostream& ostrm,
               typename Arc::Label phi_label) {
  return internal::WriteNGrams(fst, ostrm, phi_label, /*arpa_format=*/true);
}

template <class Arc>
bool WriteText(const fst::Fst<Arc>& fst, std::ostream& ostrm,
               typename Arc::Label phi_label) {
  return internal::WriteNGrams(fst, ostrm, phi_label, /*arpa_format=*/false);
}

template bool ReadArpa<fst::StdArc>(std::istream& istrm,
                                    fst::MutableFst<fst::StdArc>* fst,
                                    fst::StdArc::Label phi_label);

template bool WriteArpa<fst::StdArc>(const fst::Fst<fst::StdArc>& fst,
                                     std::ostream& ostrm,
                                     fst::StdArc::Label phi_label);

template bool WriteText<fst::StdArc>(const fst::Fst<fst::StdArc>& fst,
                                     std::ostream& ostrm,
                                     fst::StdArc::Label phi_label);

}  // namespace sfst
