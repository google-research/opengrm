
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Merges two input n-gram models into a single model.

#include <memory>
#include <string>
#include <vector>

#include <ngram/ngram-complete.h>
#include <ngram/ngram-bayes-model-merge.h>
#include <ngram/ngram-context-merge.h>
#include <ngram/ngram-count-merge.h>
#include <ngram/ngram-hist-merge.h>
#include <ngram/ngram-model-merge.h>

DEFINE_double(alpha, 1.0, "Weight for first FST");
DEFINE_double(beta, 1.0, "Weight for second (and subsequent) FST(s)");
DEFINE_string(context_pattern, "", "Context pattern for second FST");
DEFINE_string(contexts, "", "Context patterns file (all FSTs)");
DEFINE_bool(normalize, false, "Normalize resulting model");
DEFINE_string(method, "count_merge",
              "One of: \"context_merge\", \"count_merge\", \"model_merge\" "
              "\"bayes_model_merge\", \"histogram_merge\"");
DEFINE_string(ofile, "", "Output file");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, ngram::kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");
DEFINE_bool(complete, false, "Complete partial models");
DEFINE_bool(round_to_int, false, "Round all merged counts to integers");

bool ValidMergeMethod() {
  if (FLAGS_method == "count_merge" || FLAGS_method == "context_merge" ||
      FLAGS_method == "model_merge" || FLAGS_method == "bayes_model_merge" ||
      FLAGS_method == "histogram_merge") {
    return true;
  }
  return false;
}

template <class Arc>
bool ReadFst(const char *file, std::unique_ptr<fst::VectorFst<Arc>> *fst) {
  string in_name = (strcmp(file, "-") != 0) ? file : "";
  fst->reset(fst::VectorFst<Arc>::Read(file));
  if (!(*fst) || (FLAGS_complete && !ngram::NGramComplete(fst->get())))
    return false;
  return true;
}

bool GetContexts(int in_count, std::vector<string> *contexts) {
  contexts->clear();
  if (!FLAGS_contexts.empty()) {
    ngram::NGramReadContexts(FLAGS_contexts, contexts);
  } else if (!FLAGS_context_pattern.empty()) {
    contexts->push_back("");
    contexts->push_back(FLAGS_context_pattern);
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
void RoundCountsToInt(fst::StdMutableFst *fst) {
  for (size_t s = 0; s < fst->NumStates(); ++s) {
    for (fst::MutableArcIterator<fst::StdMutableFst> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      fst::StdArc arc = aiter.Value();
      int weight = round(exp(-arc.weight.Value()));
      arc.weight = -log(weight);
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != fst::StdArc::Weight::Zero()) {
      int weight = round(exp(-fst->Final(s).Value()));
      fst->SetFinal(s, -log(weight));
    }
  }
}

int main(int argc, char **argv) {
  string usage = "Merge ngram models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] -ofile=out.fst in1.fst in2.fst [in3.fst ...]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 3) {
    ShowUsage();
    return 1;
  }

  string out_name =
      FLAGS_ofile.empty() ? (argc > 3 ? argv[3] : "") : FLAGS_ofile;

  int in_count = FLAGS_ofile.empty() ? 2 : argc - 1;
  if (in_count < 2) {
    LOG(ERROR) << "Only one model given, no merging to do";
    ShowUsage();
    return 1;
  }

  if (!ValidMergeMethod()) {
    LOG(ERROR) << argv[0] << ": bad merge method: " << FLAGS_method;
    return 1;
  }

  if (FLAGS_method != "histogram_merge") {
    std::unique_ptr<fst::StdVectorFst> fst1;
    if (!ReadFst<fst::StdArc>(argv[1], &fst1)) return 1;
    std::unique_ptr<fst::StdVectorFst> fst2;
    if (FLAGS_method == "count_merge") {
      ngram::NGramCountMerge ngramrg(fst1.get(), FLAGS_backoff_label,
                                     FLAGS_norm_eps, FLAGS_check_consistency);
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        bool norm = FLAGS_normalize && i == in_count;
        ngramrg.MergeNGramModels(*fst2, FLAGS_alpha, FLAGS_beta, norm);
        if (ngramrg.Error()) return 1;
        if (FLAGS_round_to_int) RoundCountsToInt(ngramrg.GetMutableFst());
      }
      ngramrg.GetFst().Write(out_name);
    } else if (FLAGS_method == "model_merge") {
      ngram::NGramModelMerge ngramrg(fst1.get(), FLAGS_backoff_label,
                                     FLAGS_norm_eps, FLAGS_check_consistency);
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        ngramrg.MergeNGramModels(*fst2, FLAGS_alpha, FLAGS_beta,
                                 FLAGS_normalize);
        if (ngramrg.Error()) return 1;
      }
      ngramrg.GetFst().Write(out_name);
    } else if (FLAGS_method == "bayes_model_merge") {
      ngram::NGramBayesModelMerge ngramrg(fst1.get(), FLAGS_backoff_label,
                                          FLAGS_norm_eps);
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        ngramrg.MergeNGramModels(*fst2, FLAGS_alpha, FLAGS_beta);
        if (ngramrg.Error()) return 1;
      }
      ngramrg.GetFst().Write(out_name);
    } else if (FLAGS_method == "context_merge") {
      ngram::NGramContextMerge ngramrg(fst1.get(), FLAGS_backoff_label,
                                       FLAGS_norm_eps, FLAGS_check_consistency);
      std::vector<string> contexts;
      if (!GetContexts(in_count, &contexts)) return 1;
      for (int i = 2; i <= in_count; ++i) {
        if (!ReadFst<fst::StdArc>(argv[i], &fst2)) return 1;
        bool norm = FLAGS_normalize && i == in_count;
        ngramrg.MergeNGramModels(*fst2, contexts[i - 1], norm);
        if (ngramrg.Error()) return 1;
      }
      ngramrg.GetFst().Write(out_name);
    }
  } else {
    std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> hist_fst1;
    if (!ReadFst<ngram::HistogramArc>(argv[1], &hist_fst1)) return 1;
    ngram::NGramHistMerge ngramrg(hist_fst1.get(), FLAGS_backoff_label,
                                  FLAGS_norm_eps, FLAGS_check_consistency);
    for (int i = 2; i <= in_count; ++i) {
      std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> hist_fst2;
      if (!ReadFst<ngram::HistogramArc>(argv[i], &hist_fst2)) return 1;
      ngramrg.MergeNGramModels(*hist_fst2, FLAGS_alpha, FLAGS_beta,
                               FLAGS_normalize);
      if (ngramrg.Error()) return 1;
    }
    ngramrg.GetFst().Write(out_name);
  }
  return 0;
}
