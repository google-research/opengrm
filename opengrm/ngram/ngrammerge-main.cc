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

// Merges two input n-gram models into a single model.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/hist-arc.h"
#include "opengrm/ngram/ngram-bayes-model-merge.h"
#include "opengrm/ngram/ngram-complete.h"
#include "opengrm/ngram/ngram-context-merge.h"
#include "opengrm/ngram/ngram-context.h"
#include "opengrm/ngram/ngram-count-merge.h"
#include "opengrm/ngram/ngram-hist-merge.h"
#include "opengrm/ngram/ngram-model-merge.h"
#include "opengrm/ngram/ngram-replace-merge.h"

ABSL_DECLARE_FLAG(double, alpha);
ABSL_DECLARE_FLAG(double, beta);
ABSL_DECLARE_FLAG(std::string, context_pattern);
ABSL_DECLARE_FLAG(std::string, contexts);
ABSL_DECLARE_FLAG(bool, normalize);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(int32_t, max_replace_order);
ABSL_DECLARE_FLAG(std::string, ofile);
ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(double, norm_eps);
ABSL_DECLARE_FLAG(bool, check_consistency);
ABSL_DECLARE_FLAG(bool, complete);
ABSL_DECLARE_FLAG(bool, round_to_int);

namespace {

bool ValidMergeMethod() {
  if (absl::GetFlag(FLAGS_method) == "count_merge" ||
      absl::GetFlag(FLAGS_method) == "context_merge" ||
      absl::GetFlag(FLAGS_method) == "model_merge" ||
      absl::GetFlag(FLAGS_method) == "bayes_model_merge" ||
      absl::GetFlag(FLAGS_method) == "histogram_merge" ||
      absl::GetFlag(FLAGS_method) == "replace_merge") {
    return true;
  }
  return false;
}

template <class Arc>
bool ReadFst(const char* file, std::unique_ptr<fst::VectorFst<Arc>>* fst) {
  std::string in_name = (strcmp(file, "-") != 0) ? file : "";
  fst->reset(fst::VectorFst<Arc>::Read(file));
  if (!*fst ||
      (absl::GetFlag(FLAGS_complete) && !ngram::NGramComplete(fst->get())))
    return false;
  return true;
}

bool GetContexts(int in_count, std::vector<std::string>* contexts) {
  contexts->clear();
  if (!absl::GetFlag(FLAGS_contexts).empty()) {
    ngram::NGramReadContexts(absl::GetFlag(FLAGS_contexts), contexts);
  } else if (!absl::GetFlag(FLAGS_context_pattern).empty()) {
    contexts->push_back("");
    contexts->push_back(absl::GetFlag(FLAGS_context_pattern));
  } else {
    LOG(ERROR) << "Context patterns not specified";
    return false;
  }
  if (contexts->size() != in_count) {
    LOG(ERROR) << "Wrong number of context patterns specified";
    return false;
  }
  return true;
}

// Rounds -log count to values corresponding to the rounded integer count
// Reduces small floating point precision issues when dealing with int counts
// Primarily for testing that methods for deriving the same model are identical
void RoundCountsToInt(fst::StdMutableFst* fst) {
  for (size_t s = 0; s < fst->NumStates(); ++s) {
    for (fst::MutableArcIterator<fst::StdMutableFst> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      fst::StdArc arc = aiter.Value();
      auto weight = std::round(std::exp(-arc.weight.Value()));
      arc.weight = fst::StdArc::Weight(-std::log(weight));
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != fst::StdArc::Weight::Zero()) {
      auto weight = std::round(std::exp(-fst->Final(s).Value()));
      fst->SetFinal(s, fst::StdArc::Weight(-std::log(weight)));
    }
  }
}

}  // namespace

int ngrammerge_main(int argc, char** argv) {
  std::string usage = "Merge n-gram models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] -ofile=out.fst in1.fst in2.fst [in3.fst ...]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string out_name = absl::GetFlag(FLAGS_ofile).empty()
                             ? (argc > 3 ? argv[3] : "")
                             : absl::GetFlag(FLAGS_ofile);

  int in_count = absl::GetFlag(FLAGS_ofile).empty() ? 2 : argc - 1;
  if (in_count < 2) {
    LOG(ERROR) << "Only one model given, no merging to do";
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  if (!ValidMergeMethod()) {
    LOG(ERROR) << argv[0]
               << ": bad merge method: " << absl::GetFlag(FLAGS_method);
    return 1;
  }

  if (absl::GetFlag(FLAGS_method) != "histogram_merge") {
    std::unique_ptr<fst::StdVectorFst> fst1;
    if (!ReadFst<fst::StdArc>(argv[1], &fst1)) return 1;
    std::unique_ptr<fst::StdVectorFst> fst2;
    if (absl::GetFlag(FLAGS_method) == "count_merge") {
      ngram::NGramCountMerge ngramrg(fst1.get(),
                                     absl::GetFlag(FLAGS_backoff_label),
                                     absl::GetFlag(FLAGS_norm_eps),
                                     absl::GetFlag(FLAGS_check_consistency));
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        bool norm = absl::GetFlag(FLAGS_normalize) && i == in_count;
        ngramrg.MergeNGramModels(*fst2, absl::GetFlag(FLAGS_alpha),
                                 absl::GetFlag(FLAGS_beta), norm);
        if (ngramrg.Error()) return 1;
        if (absl::GetFlag(FLAGS_round_to_int))
          RoundCountsToInt(ngramrg.GetMutableFst());
      }
      ngramrg.GetFst().Write(out_name);
    } else if (absl::GetFlag(FLAGS_method) == "model_merge") {
      ngram::NGramModelMerge ngramrg(fst1.get(),
                                     absl::GetFlag(FLAGS_backoff_label),
                                     absl::GetFlag(FLAGS_norm_eps),
                                     absl::GetFlag(FLAGS_check_consistency));
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        ngramrg.MergeNGramModels(*fst2, absl::GetFlag(FLAGS_alpha),
                                 absl::GetFlag(FLAGS_beta),
                                 absl::GetFlag(FLAGS_normalize));
        if (ngramrg.Error()) return 1;
      }
      ngramrg.GetFst().Write(out_name);
    } else if (absl::GetFlag(FLAGS_method) == "bayes_model_merge") {
      ngram::NGramBayesModelMerge ngramrg(fst1.get(),
                                          absl::GetFlag(FLAGS_backoff_label),
                                          absl::GetFlag(FLAGS_norm_eps));
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        ngramrg.MergeNGramModels(*fst2, absl::GetFlag(FLAGS_alpha),
                                 absl::GetFlag(FLAGS_beta));
        if (ngramrg.Error()) return 1;
      }
      ngramrg.GetFst().Write(out_name);
    } else if (absl::GetFlag(FLAGS_method) == "replace_merge") {
      if (in_count != 2) {
        LOG(ERROR) << argv[0] << "Only 2 models allowed for replace merge";
        return 1;
      }
      ngram::NGramReplaceMerge ngramrg(fst1.get(),
                                       absl::GetFlag(FLAGS_backoff_label),
                                       absl::GetFlag(FLAGS_norm_eps));
      if (!ReadFst<fst::StdArc>(argv[2], &fst2)) return 1;
      ngramrg.MergeNGramModels(*fst2, absl::GetFlag(FLAGS_max_replace_order),
                               absl::GetFlag(FLAGS_normalize));
      if (ngramrg.Error()) return 1;
      ngramrg.GetFst().Write(out_name);
    } else if (absl::GetFlag(FLAGS_method) == "context_merge") {
      ngram::NGramContextMerge ngramrg(fst1.get(),
                                       absl::GetFlag(FLAGS_backoff_label),
                                       absl::GetFlag(FLAGS_norm_eps),
                                       absl::GetFlag(FLAGS_check_consistency));
      std::vector<std::string> contexts;
      if (!GetContexts(in_count, &contexts)) return 1;
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        bool norm = absl::GetFlag(FLAGS_normalize) && i == in_count;
        ngramrg.MergeNGramModels(*fst2, contexts[i - 1], norm);
        if (ngramrg.Error()) return 1;
      }
      ngramrg.GetFst().Write(out_name);
    }
  } else {
    std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> hist_fst1;
    if (!ReadFst<ngram::HistogramArc>(argv[1], &hist_fst1)) return 1;
    ngram::NGramHistMerge ngramrg(
        hist_fst1.get(), absl::GetFlag(FLAGS_backoff_label),
        absl::GetFlag(FLAGS_norm_eps), absl::GetFlag(FLAGS_check_consistency));
    for (int i = 2; i <= in_count; ++i) {
      std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> hist_fst2;
      if (!ReadFst<ngram::HistogramArc>(argv[i], &hist_fst2)) return 1;
      ngramrg.MergeNGramModels(*hist_fst2, absl::GetFlag(FLAGS_alpha),
                               absl::GetFlag(FLAGS_beta),
                               absl::GetFlag(FLAGS_normalize));
      if (ngramrg.Error()) return 1;
    }
    ngramrg.GetFst().Write(out_name);
  }
  return 0;
}
