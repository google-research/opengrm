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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// TODO: file description & doc.

#include <ngram/ngram-merge.h>

using namespace fst;
using namespace ngram;

DEFINE_double(alpha, 1, "Weight for in1.fst in real semiring");
DEFINE_double(beta, 1, "Weight for in2.fst in real semiring");
DEFINE_bool(normalize, false, "Normalize resulting model");
DEFINE_bool(use_smoothing, false, "Use model smoothing when merging");
DEFINE_bool(fixedorder, false, "Merge in given argument order");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");

int main(int argc, char **argv) {
  string usage = "Merge ngram models.  (alpha)in1 + (beta)in2.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in1.fst in2.fst [out.fst]\n";
  InitFst(usage.c_str(), &argc, &argv, true);

  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  string in1_name = (strcmp(argv[1], "-") != 0) ? argv[1] : "";
  string in2_name = (strcmp(argv[2], "-") != 0) ? argv[2] : "";
  string out_name = argc > 3 ? argv[3] : "";


  StdMutableFst *fst1 = StdMutableFst::Read(in1_name, true);
  if (!fst1) return 1;

  StdMutableFst *fst2 = StdMutableFst::Read(in2_name, true);
  if (!fst2) return 1;

  double alpha = -log(FLAGS_alpha);
  double beta = -log(FLAGS_beta);

  NGramMerge ngramrg(fst1, fst2, alpha, beta, FLAGS_normalize,
		     FLAGS_use_smoothing, FLAGS_fixedorder,
		     FLAGS_backoff_label, FLAGS_norm_eps,
                     FLAGS_check_consistency);

  ngramrg.MergeNGramModels();
  ngramrg.GetFst().Write(out_name);
  return 0;
}
