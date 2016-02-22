// ngrammerge.cc
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
//
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// Merges two input n-gram models into a single model

#include <ngram/ngram-complete.h>
#include <ngram/ngram-context-merge.h>
#include <ngram/ngram-count-merge.h>
#include <ngram/ngram-model-merge.h>

using namespace fst;
using namespace ngram;

DEFINE_double(alpha, 1.0, "Weight for first FST");
DEFINE_double(beta, 1.0, "Weight for second (and subsequent) FST(s)");
DEFINE_string(context_pattern, "", "Context pattern for second FST");
DEFINE_string(contexts, "", "Context patterns file (all FSTs)");
DEFINE_bool(normalize, false, "Normalize resulting model");
DEFINE_string(method, "count_merge",
	      "One of: \"context_merge\", \"count_merge\", \"model_merge\"");
DEFINE_string(ofile, "", "Output file");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");
DEFINE_bool(complete, false, "Complete partial models");
DEFINE_bool(round_to_int, false, "Round all merged counts to integers");

StdMutableFst *ReadFst(const char *file) {
  string in_name = (strcmp(file, "-") != 0) ? file : "";
  StdMutableFst *fst = StdMutableFst::Read(file, true);
  if (!fst) return 0;
  if (FLAGS_complete)
    NGramComplete(fst);
  return fst;
}

bool GetContexts(int in_count, std::vector<string> *contexts) {
  contexts->clear();
  if (!FLAGS_contexts.empty()) {
    NGramReadContexts(FLAGS_contexts, contexts);
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
void RoundCountsToInt(StdMutableFst *fst) {
  for (size_t s = 0; s < fst->NumStates(); ++s) {
    for (MutableArcIterator<StdMutableFst> aiter(fst, s);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      int weight = round(exp(-arc.weight.Value()));
      arc.weight = -log(weight);
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != StdArc::Weight::Zero()) {
      int weight = round(exp(-fst->Final(s).Value()));
      fst->SetFinal(s, -log(weight));
    }
  }
}

int main(int argc, char **argv) {
  string usage = "Merge ngram models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in1.fst in2.fst [out.fst]\n";
  usage += "        ";
  usage += argv[0];
  usage += " [--options] -ofile=out.fst in1.fst in2.fst [in3.fst ...]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 3) {
    ShowUsage();
    return 1;
  }

  string out_name = FLAGS_ofile.empty() ? (argc > 3 ? argv[3] : "")
      : FLAGS_ofile;

  int in_count = FLAGS_ofile.empty() ? 2 : argc - 1;

  StdMutableFst *fst1 = ReadFst(argv[1]);
  if (!fst1) return 1;

  if (FLAGS_method == "count_merge") {
    NGramCountMerge ngramrg(fst1, FLAGS_backoff_label, FLAGS_norm_eps,
                            FLAGS_check_consistency);
    for (int i = 2; i <= in_count; ++i) {
      StdMutableFst *fst2 = ReadFst(argv[i]);
      if (!fst2) return 1;
      bool norm = FLAGS_normalize && i == in_count;
      ngramrg.MergeNGramModels(*fst2, FLAGS_alpha, FLAGS_beta, norm);
      if (FLAGS_round_to_int)
	RoundCountsToInt(ngramrg.GetMutableFst());
      delete fst2;
    }
    ngramrg.GetFst().Write(out_name);
  } else if (FLAGS_method == "model_merge") {
    NGramModelMerge ngramrg(fst1, FLAGS_backoff_label, FLAGS_norm_eps,
                            FLAGS_check_consistency);
    for (int i = 2; i <= in_count; ++i) {
      StdMutableFst *fst2 = ReadFst(argv[i]);
      if (!fst2) return 1;
      bool norm = FLAGS_normalize && i == in_count;
      ngramrg.MergeNGramModels(*fst2, FLAGS_alpha, FLAGS_beta, norm);
      delete fst2;
    }
    ngramrg.GetFst().Write(out_name);
  } else if (FLAGS_method == "context_merge") {
    NGramContextMerge ngramrg(fst1, FLAGS_backoff_label, FLAGS_norm_eps,
                              FLAGS_check_consistency);
    std::vector<string> contexts;
    if (!GetContexts(in_count,&contexts))
      return 1;
    for (int i = 2; i <= in_count; ++i) {
      StdMutableFst *fst2 = ReadFst(argv[i]);
      if (!fst2) return 1;
      bool norm = FLAGS_normalize && i == in_count;
      ngramrg.MergeNGramModels(*fst2, contexts[i - 1], norm);
      delete fst2;
    }
    ngramrg.GetFst().Write(out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad merge method: " << FLAGS_method;
    return 1;
  }

  return 0;
}
