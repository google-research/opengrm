// ngramsplit.cc
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
// Splits an n-gram model based on given context patterns

#include <ngram/ngram-complete.h>
#include <ngram/ngram-split.h>
#include <sstream>

using namespace fst;
using namespace ngram;

DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_string(contexts, "", "Context patterns file");
DEFINE_double(norm_eps, kNormEps, "Normalization check epsilon");
DEFINE_bool(complete, false, "Complete partial models");

int main(int argc, char **argv) {
  string usage = "ngramsplit.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in_fst [out_fsts_prefix]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

   if (argc > 3 || argc < 2) {
    ShowUsage(argv[0]);
    return 1;
  }

   string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
   string out_name_prefix = argc > 2 ? argv[2] : in_name;

   vector<string> context_patterns;

   if (FLAGS_contexts.empty()) {
     LOG(ERROR) << "Context patterns file need to be specified using "
                << "--contexts flag.";
     return 1;
   } else {
     NGramReadContexts(FLAGS_contexts, &context_patterns);
   }

   StdMutableFst *fst = StdMutableFst::Read(in_name, true);
   if (!fst)
     return 1;
  if (FLAGS_complete)
    NGramComplete(fst);

   NGramSplit split(*fst, context_patterns,
                    FLAGS_backoff_label, FLAGS_norm_eps);

   StdVectorFst ofst;
   for (int i = 0; !split.Done(); ++i) {
     split.NextNGramModel(&ofst);
     ostringstream suffix;
     suffix.width(5);
     suffix.fill('0');
     suffix << i;
     string out_name = out_name_prefix + suffix.str();
     if (!ofst.Write(out_name))
       return 1;
   }

   return 0;
}
