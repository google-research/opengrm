// ngrammake.cc
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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// TODO: file description & doc.

#include <ngram/ngram-make.h>
#include <ngram/ngram-kneserney.h>
#include <ngram/ngram-absolute.h>
#include <ngram/ngram-katz.h>
#include <ngram/ngram-wittenbell.h>
#include <ngram/ngram-unsmoothed.h>

using namespace fst;
using namespace ngram;

DEFINE_double(witten_bell_k, 1, "Witten-Bell hyperparameter K");
DEFINE_double(discount_D, -1, "Absolute discount value D to use");
DEFINE_string(method, "witten_bell",
	      "One of: \"absolute\", \"katz\", \"kneser_ney\", \"presmoothed\", "
              "\"unsmoothed\", \"witten_bell\" (default)");
DEFINE_bool(backoff, false, "Use backoff smoothing instead of mixture");
DEFINE_int64(bins, -1, "Number of bins for katz or absolute discounting");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");

int main(int argc, char **argv) {
  string usage = "Make ngram model from input count file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  InitFst(usage.c_str(), &argc, &argv, true);

  if (argc < 1 || argc > 3) {
    ShowUsage();
    return 1;
  }
  bool prefix_norm = 0;
  if (FLAGS_method == "presmoothed") {  // only for use with randgen counts
    prefix_norm = 1;
    FLAGS_method = "unsmoothed";  // normalizes only based on prefix count
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  // NOTE: another option: smooth unigram to uniform?
  StdMutableFst *fst = StdMutableFst::Read(in_name, true);
  if (!fst) return 1;

  if (FLAGS_method == "kneser_ney") {
    NGramKneserNey ngram(fst, FLAGS_backoff, FLAGS_backoff_label,
			 FLAGS_norm_eps, FLAGS_check_consistency,
			 FLAGS_discount_D, FLAGS_bins);
    ngram.MakeNGramModel();
    ngram.GetFst().Write(out_name);
  } else if (FLAGS_method == "absolute") {
    NGramAbsolute ngram(fst, FLAGS_backoff, FLAGS_backoff_label,
			FLAGS_norm_eps, FLAGS_check_consistency,
			FLAGS_discount_D, FLAGS_bins);
    ngram.MakeNGramModel();
    ngram.GetFst().Write(out_name);
  } else if (FLAGS_method == "katz") {
    NGramKatz ngram(fst, FLAGS_backoff, FLAGS_backoff_label,
		    FLAGS_norm_eps, FLAGS_check_consistency,
		    FLAGS_bins);
    ngram.MakeNGramModel();
    ngram.GetFst().Write(out_name);
  } else if (FLAGS_method == "witten_bell") {
    NGramWittenBell ngram(fst, FLAGS_backoff, FLAGS_backoff_label,
			  FLAGS_norm_eps, FLAGS_check_consistency,
			  FLAGS_witten_bell_k);
    ngram.MakeNGramModel();
    ngram.GetFst().Write(out_name);
  } else if (FLAGS_method == "unsmoothed") {
    NGramUnsmoothed ngram(fst, 1, prefix_norm, FLAGS_backoff_label,
			  FLAGS_norm_eps, FLAGS_check_consistency);
    ngram.MakeNGramModel();
    ngram.GetFst().Write(out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad model method: " << FLAGS_method;
    return 1;
  }
  return 0;
}
