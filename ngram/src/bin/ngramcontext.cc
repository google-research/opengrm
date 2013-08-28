// ngramcontext.cc
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
// Generates a context set of a given size from an input LM

#include <fst/mutable-fst.h>
#include <ngram/ngram-context.h>
#include <ngram/ngram-model.h>

using namespace fst;
using namespace ngram;

DEFINE_int64(contexts, 1, "Number of desired contexts");
DEFINE_double(bigram_threshold, 1.1,
              "Bin overfill to force a bigram context split");

int main(int argc, char **argv) {
  string usage = "Generates a context set from an input LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst] [out.fst]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 2 || argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = argv[1];
  string out_name = argc > 2 ? argv[2] : "";

  StdFst *in_fst = StdFst::Read(in_name);
  if (!in_fst)
    return 1;

  NGramModel ngram(*in_fst, 0, kNormEps, true);
  vector<string> contexts;
  NGramContext::FindContexts(ngram, FLAGS_contexts, &contexts,
                             FLAGS_bigram_threshold);
  bool ret = NGramWriteContexts(out_name, contexts);

  return !ret;
}
