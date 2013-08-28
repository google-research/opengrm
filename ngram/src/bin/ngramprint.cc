// ngramprint.cc
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
// Prints a given n-gram model to various kinds of textual formats

#include <ngram/ngram-output.h>

using namespace fst;
using namespace ngram;

DEFINE_bool(ARPA, false, "Print in ARPA format");
DEFINE_bool(backoff, false, "Show epsilon backoff transitions when printing");
DEFINE_bool(negativelogs, false,
	    "Show negative log probs/counts when printing");
DEFINE_bool(integers, false, "Show just integer counts when printing");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_bool(check_consistency, false, "Check model consistency");
DEFINE_string(context_pattern, "", "Pattern of contexts to print");
DEFINE_bool(include_all_suffixes, false, "Include suffixes of contexts");

int main(int argc, char **argv) {
  string usage = "Print ngram counts and models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.txt]]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "stdout";

  StdMutableFst *fst = StdMutableFst::Read(in_name, true);
  if (!fst) return 1;

  ostream *ostrm = (argc > 2 && (strcmp(argv[2], "-") != 0)) ?
    new ofstream(argv[2]) : &std::cout;
  if (!(*ostrm)) {
    LOG(ERROR) << "Can't open for writing: " << out_name;
    return 1;
  }

  NGramOutput ngram(fst, *ostrm, FLAGS_backoff_label, FLAGS_check_consistency,
		    FLAGS_context_pattern, FLAGS_include_all_suffixes);
  ngram.ShowNGramModel(FLAGS_backoff, FLAGS_negativelogs,
		       FLAGS_integers, FLAGS_ARPA);
  if (ostrm != &std::cout) {
    delete ostrm;
  }
  return 0;
}
