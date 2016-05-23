
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Shrinks an input n-gram model using given pruning criteria.

#include <ngram/ngram-context-prune.h>
#include <ngram/ngram-count-prune.h>
#include <ngram/ngram-relentropy.h>
#include <ngram/ngram-seymore-shrink.h>
#include <ngram/ngram-shrink.h>

DEFINE_double(total_unigram_count, -1.0, "Total unigram count");
DEFINE_double(theta, 0.0, "Pruning threshold theta");
DEFINE_int64(target_number_of_ngrams, -1,
             "Maximum number of ngrams to leave in model after pruning."
             "Value less than zero means no target number, just use theta.");
DEFINE_string(method, "seymore",
              "One of: \"context_prune\", \"count_prune\", "
              "\"relative_entropy\", \"seymore\"");
DEFINE_string(count_pattern, "", "Pattern of counts to prune");
DEFINE_string(context_pattern, "", "Pattern of contexts to prune");
DEFINE_int32(shrink_opt, 0,
             "Optimization level: Range 0 (fastest) to 2 (most accurate)");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, ngram::kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");

int main(int argc, char **argv) {
  string usage = "Shrink ngram model from input model file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  fst::StdMutableFst *fst = fst::StdMutableFst::Read(in_name, true);
  if (!fst) return 1;

  bool full_context = FLAGS_context_pattern.empty();

  if (FLAGS_target_number_of_ngrams >= 0) {
    if ((FLAGS_method != "relative_entropy" && FLAGS_method != "seymore") ||
        !full_context) {
      LOG(ERROR) << "--target_number_of_ngrams flag only available for "
                    "relative_entropy or seymore shrinking with a full context";
    } else if (FLAGS_theta != 0.0) {
      LOG(ERROR) << "If specifying target number of ngrams, "
                    "theta must be at the default value of 0.0";
    }
  }

  if (FLAGS_method == "count_prune" && full_context) {
    ngram::NGramCountPrune ngramsh(
        fst, FLAGS_count_pattern, FLAGS_shrink_opt, FLAGS_total_unigram_count,
        FLAGS_backoff_label, FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramCountPrune: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "relative_entropy" && full_context) {
    ngram::NGramRelEntropy ngramsh(
        fst, FLAGS_theta, FLAGS_shrink_opt, FLAGS_total_unigram_count,
        FLAGS_backoff_label, FLAGS_norm_eps, FLAGS_check_consistency);
    if (FLAGS_target_number_of_ngrams >= 0)
      ngramsh.CalculateTheta(FLAGS_target_number_of_ngrams);
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramRelEntropy: failed to calculate theta";
      return 1;
    }
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramRelEntropy: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "seymore" && full_context) {
    ngram::NGramSeymoreShrink ngramsh(
        fst, FLAGS_theta, FLAGS_shrink_opt, FLAGS_total_unigram_count,
        FLAGS_backoff_label, FLAGS_norm_eps, FLAGS_check_consistency);
    if (FLAGS_target_number_of_ngrams >= 0)
      ngramsh.CalculateTheta(FLAGS_target_number_of_ngrams);
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramSeymoreShrink: failed to calculate theta";
      return 1;
    }
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramSeymoreShrink: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "context_prune") {
    ngram::NGramContextPrune ngramsh(
        fst, FLAGS_context_pattern, FLAGS_shrink_opt, FLAGS_total_unigram_count,
        FLAGS_backoff_label, FLAGS_norm_eps, FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramContextPrune: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "count_prune" && !full_context) {
    ngram::NGramContextCountPrune ngramsh(
        fst, FLAGS_count_pattern, FLAGS_context_pattern, FLAGS_shrink_opt,
        FLAGS_total_unigram_count, FLAGS_backoff_label, FLAGS_norm_eps,
        FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramContextCountPrune: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "relative_entropy" && !full_context) {
    ngram::NGramContextRelEntropy ngramsh(
        fst, FLAGS_theta, FLAGS_context_pattern, FLAGS_shrink_opt,
        FLAGS_total_unigram_count, FLAGS_backoff_label, FLAGS_norm_eps,
        FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramContextRelEntropy: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else if (FLAGS_method == "seymore" && !full_context) {
    ngram::NGramContextSeymoreShrink ngramsh(
        fst, FLAGS_theta, FLAGS_context_pattern, FLAGS_shrink_opt,
        FLAGS_total_unigram_count, FLAGS_backoff_label, FLAGS_norm_eps,
        FLAGS_check_consistency);
    ngramsh.ShrinkNGramModel();
    if (ngramsh.Error()) {
      NGRAMERROR() << "NGramContextSeymoreShrink: failed to shrink model";
      return 1;
    }
    ngramsh.GetFst().Write(out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad shrink method: " << FLAGS_method;
    return 1;
  }
  return 0;
}
