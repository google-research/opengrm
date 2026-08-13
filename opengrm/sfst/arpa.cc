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
#include <vector>  // NOLINT(misc-include-cleaner)

#include "absl/container/flat_hash_map.h"  // NOLINT(misc-include-cleaner)
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"   // NOLINT(misc-include-cleaner)
#include "absl/strings/str_split.h"  // NOLINT(misc-include-cleaner)
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

template <typename Label, typename StateId, class Arc>
inline void EnsureSuffixHistoriesExist(
    std::map<std::vector<Label>, StateId>& history_to_state,
    const std::vector<Label>& hist, fst::MutableFst<Arc>* fst) {
  for (size_t len = 1; len < hist.size(); ++len) {
    std::vector<Label> sub(hist.end() - len, hist.end());
    if (history_to_state.find(sub) == history_to_state.end()) {
      history_to_state[sub] = fst->AddState();
    }
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

}  // namespace internal

template <class Arc>
bool ReadArpa(std::istream& istrm, fst::MutableFst<Arc>* fst) {
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  fst->DeleteStates();
  StateId start_state = fst->AddState();
  fst->SetStart(start_state);
  if (!fst->InputSymbols()) {
    fst::SymbolTable syms("ARPASymbols");
    syms.AddSymbol("<epsilon>");
    fst->SetInputSymbols(&syms);
  }
  fst::SymbolTable* syms = fst->MutableInputSymbols();
  // Loads and buffers all input lines from the stream.
  std::vector<std::string> lines;
  std::string raw_line;
  while (std::getline(istrm, raw_line)) {
    lines.push_back(raw_line);
  }
  struct NgramData {
    double log_prob = 0.0;
    double backoff_weight = 0.0;
    bool has_log_prob = false;
    bool has_backoff = false;
  };
  absl::flat_hash_map<std::vector<Label>, NgramData> all_ngrams;
  int current_order = 0;
  bool in_ngrams = false;
  bool error = false;
  // Collects all n-grams and explicitly populates implied lower-order gaps.
  for (const std::string& line : lines) {
    if (line.empty()) continue;
    if (line[0] == '\\') {
      size_t grams_pos = line.find("-grams:");
      if (grams_pos != std::string::npos && grams_pos > 1) {
        in_ngrams = true;
        std::string order_str = line.substr(1, grams_pos - 1);
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
    if (line == "\\end\\") break;
    const std::vector<std::string> parts =
        absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipEmpty());
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
      ngram.push_back(syms->AddSymbol(parts[i]));
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
    all_ngrams[ngram].log_prob = log_prob * std::log(10.0);
    all_ngrams[ngram].has_log_prob = true;
    if (has_bo) {
      all_ngrams[ngram].backoff_weight = boweight * std::log(10.0);
      all_ngrams[ngram].has_backoff = true;
    }
    for (int len = 1; len < current_order; ++len) {
      for (int start = 0; start <= current_order - len; ++start) {
        std::vector<Label> sub(ngram.begin() + start,
                               ngram.begin() + start + len);
        all_ngrams.try_emplace(sub, NgramData{0.0, 0.0, false, false});
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
  const int max_hist_len = actual_max_order - 1;
  // Allocates FST states for all collected and implied histories.
  std::map<std::vector<Label>, StateId> history_to_state;
  history_to_state[std::vector<Label>()] = start_state;
  for (const auto& pair : all_ngrams) {
    const auto& ngram = pair.first;
    const auto src_hist = internal::GetSourceHistory(ngram, max_hist_len);
    if (history_to_state.find(src_hist) == history_to_state.end()) {
      history_to_state[src_hist] = fst->AddState();
    }
    const auto dst_hist = internal::GetDestinationHistory(ngram, max_hist_len);
    if (history_to_state.find(dst_hist) == history_to_state.end()) {
      history_to_state[dst_hist] = fst->AddState();
    }
    internal::EnsureSuffixHistoriesExist(history_to_state, src_hist, fst);
    internal::EnsureSuffixHistoriesExist(history_to_state, dst_hist, fst);
  }
  // Instantiates all word transitions in the FST.
  for (const auto& pair : all_ngrams) {
    const auto& ngram = pair.first;
    const auto& data = pair.second;
    const auto src_hist = internal::GetSourceHistory(ngram, max_hist_len);
    const auto dst_hist = internal::GetDestinationHistory(ngram, max_hist_len);
    const Label word = ngram.back();
    StateId src = history_to_state[src_hist];
    StateId dst = history_to_state[dst_hist];
    fst->AddArc(src, Arc(word, word, Weight(-data.log_prob), dst));
  }
  // Instantiates all backoff transitions using kNoLabel.
  for (const auto& pair : history_to_state) {
    const auto& hist = pair.first;
    const StateId src = pair.second;
    if (hist.empty()) continue;
    std::vector<Label> bo_hist(hist.begin() + 1, hist.end());
    StateId dst = history_to_state[bo_hist];
    double bo_weight = 0.0;
    auto it = all_ngrams.find(hist);
    if (it != all_ngrams.end() && it->second.has_backoff) {
      bo_weight = it->second.backoff_weight;
    }
    fst->AddArc(src,
                Arc(fst::kNoLabel, fst::kNoLabel, Weight(-bo_weight), dst));
  }
  fst::ArcSort(fst, fst::ILabelCompare<Arc>());
  fst->SetOutputSymbols(fst->InputSymbols());
  return !error;
}

namespace internal {
template <class Arc>
bool WriteNGrams(const fst::Fst<Arc>& fst, std::ostream& ostrm,
                 typename Arc::Label phi_label, bool arpa_format) {
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  if (!fst.InputSymbols()) {
    std::cerr << "WriteArpa: FST has no input symbols" << std::endl;
    return false;
  }
  ostrm.precision(7);
  const StateId start_state = fst.Start();
  if (start_state == fst::kNoStateId) return true;
  const fst::SymbolTable* syms = fst.InputSymbols();
  Label bos_label = syms->Find("<s>");
  if (bos_label == fst::kNoLabel) bos_label = syms->Find("<S>");
  Label eos_label = syms->Find("</s>");
  if (eos_label == fst::kNoLabel) eos_label = syms->Find("</S>");

  const std::string bos_str =
      (bos_label != fst::kNoLabel) ? syms->Find(bos_label) : "<s>";
  const std::string eos_str =
      (eos_label != fst::kNoLabel) ? syms->Find(eos_label) : "</s>";

  // Find unigram state from start_state's phi arc.
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

  struct ArpaNgramPrintData {
    std::string text;
    double log_prob;
    double backoff_weight = 0.0;
    bool has_backoff = false;
  };
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
    int order = H.size() + 1;

    if (fst.Final(s) != Weight::Zero()) {
      std::string text =
          absl::StrJoin(H, " ", [&label_to_str](std::string* out, Label l) {
            out->append(label_to_str(l));
          });
      if (!text.empty()) absl::StrAppend(&text, " ");
      absl::StrAppend(&text, eos_str);
      ArpaNgramPrintData data;
      data.text = text;
      data.log_prob = -fst.Final(s).Value() / std::log(10.0);
      order_to_ngrams[order].push_back(data);
    }
    for (fst::ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == phi_label) continue;
      std::string text =
          absl::StrJoin(H, " ", [&label_to_str](std::string* out, Label l) {
            out->append(label_to_str(l));
          });
      if (!text.empty()) absl::StrAppend(&text, " ");
      absl::StrAppend(&text, label_to_str(arc.ilabel));
      ArpaNgramPrintData data;
      data.text = text;
      data.log_prob = -arc.weight.Value() / std::log(10.0);
      auto bo_it = state_backoff_weights.find(arc.nextstate);
      if (bo_it != state_backoff_weights.end()) {
        data.backoff_weight = bo_it->second / std::log(10.0);
        data.has_backoff = true;
      }
      order_to_ngrams[order].push_back(data);
    }
  }

  int max_order = std::max(model_max_order, 1);
  for (const auto& [order, ngrams] : order_to_ngrams) {
    if (order > max_order) max_order = order;
  }
  // Clear backoff from highest order n-grams.
  if (max_order > 0) {
    for (auto& data : order_to_ngrams[max_order]) {
      data.has_backoff = false;
    }
  }

  if (arpa_format) {
    // Prints and formats the final ARPA LM to the output stream.
    ostrm << "\\data\\" << std::endl;
    for (int o = 1; o <= max_order; ++o) {
      ostrm << "ngram " << o << "=" << order_to_ngrams[o].size() << std::endl;
    }
    for (int o = 1; o <= max_order; ++o) {
      ostrm << "\\" << o << "-grams:" << std::endl;
      for (const auto& data : order_to_ngrams[o]) {
        ostrm << data.log_prob << '\t' << data.text;
        if (data.has_backoff) {
          ostrm << '\t' << data.backoff_weight;
        }
        ostrm << std::endl;
      }
    }
    ostrm << "\\end\\" << std::endl;
  } else {
    // Prints plain TSV format.
    for (int o = 1; o <= max_order; ++o) {
      for (const auto& data : order_to_ngrams[o]) {
        ostrm << data.text << '\t' << data.log_prob;
        if (data.has_backoff) {
          ostrm << '\t' << data.backoff_weight;
        }
        ostrm << std::endl;
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
                                    fst::MutableFst<fst::StdArc>* fst);

template bool WriteArpa<fst::StdArc>(const fst::Fst<fst::StdArc>& fst,
                                     std::ostream& ostrm,
                                     fst::StdArc::Label phi_label);

template bool WriteText<fst::StdArc>(const fst::Fst<fst::StdArc>& fst,
                                     std::ostream& ostrm,
                                     fst::StdArc::Label phi_label);

}  // namespace sfst
