// ngramshrink.cc
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
// Shrinks an input n-gram model using given pruning criteria

#include <ngram/ngram-shrink.h>
#include <ngram/ngram-relentropy.h>
#include <ngram/ngram-seymore-shrink.h>
#include <ngram/ngram-count-prune.h>
#include <ngram/ngram-context-prune.h>

using namespace fst;
using namespace ngram;

DEFINE_double(total_unigram_count, -1.0, "Total unigram count");
DEFINE_double(theta, 0.0, "Pruning threshold theta");
DEFINE_string(method, "seymore",
	      "One of: \"context_prune\", \"count_prune\", \"relative_entropy\", \"seymore\"");
DEFINE_string(count_pattern, "", "Pattern of counts to prune");
DEFINE_string(context_pattern, "", "Pattern of contexts to prune");
DEFINE_int32(shrink_opt, 0,
	     "Optimization level: Range 0 (fastest) to 2 (most accurate)");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");

int main(int argc, char **argv) {
  string usage = "Shrink ngram model from input model file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  StdMutableFst *fst = StdMutableFst::Read(in_name, true);
  if (!fst)
    return 1;

  bool full_context = FLAGS_context_pattern.empty();

  if (FLAGS_method == "count_prune" && full_context) {
    NGramCountPrune ngramsh(fst, FLAGS_count_pattern,
			    FLAGS_shrink_opt, FLAGS_total_unigram_count,
			    FLAGS_backoff_label, FLAGS_norm_eps,
			    FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "relative_entropy" && full_context) {
    NGramRelEntropy ngramsh(fst, FLAGS_theta, FLAGS_shrink_opt,
			    FLAGS_total_unigram_count, FLAGS_backoff_label,
			    FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "seymore" && full_context) {
    NGramSeymoreShrink ngramsh(fst, FLAGS_theta, FLAGS_shrink_opt,
			       FLAGS_total_unigram_count, FLAGS_backoff_label,
			       FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "context_prune") {
    NGramContextPrune ngramsh(fst, FLAGS_context_pattern, FLAGS_shrink_opt,
                              FLAGS_total_unigram_count, FLAGS_backoff_label,
                              FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "count_prune" && !full_context) {
    NGramContextCountPrune ngramsh(fst, FLAGS_count_pattern,
				   FLAGS_context_pattern,
				   FLAGS_shrink_opt,
				   FLAGS_total_unigram_count,
				   FLAGS_backoff_label, FLAGS_norm_eps,
				   FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "relative_entropy" && !full_context) {
    NGramContextRelEntropy ngramsh(fst, FLAGS_theta, FLAGS_context_pattern,
				   FLAGS_shrink_opt,
				   FLAGS_total_unigram_count,
				   FLAGS_backoff_label,
				   FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "seymore" && !full_context) {
    NGramContextSeymoreShrink ngramsh(fst, FLAGS_theta, FLAGS_context_pattern,
				      FLAGS_shrink_opt,
				      FLAGS_total_unigram_count,
				      FLAGS_backoff_label,
				      FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    ngramsh.GetFst().Write(out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad shrink method: " << FLAGS_method;
    return 1;
  }
  return 0;
}
