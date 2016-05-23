
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
// Generates a context set of a given size from an input LM.

#include <vector>

#include <fst/mutable-fst.h>
#include <ngram/ngram-context.h>
#include <ngram/ngram-model.h>

DEFINE_int64(contexts, 1, "Number of desired contexts");
DEFINE_double(bigram_threshold, 1.1,
              "Bin overfill to force a bigram context split");

int main(int argc, char **argv) {
  string usage = "Generates a context set from an input LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst] [out.fst]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 2 || argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = argv[1];
  string out_name = argc > 2 ? argv[2] : "";

  fst::StdFst *in_fst = fst::StdFst::Read(in_name);
  if (!in_fst) return 1;

  ngram::NGramModel<fst::StdArc> ngram(*in_fst, 0, ngram::kNormEps, true);
  std::vector<string> contexts;
  ngram::NGramContext::FindContexts(ngram, FLAGS_contexts, &contexts,
                                    FLAGS_bigram_threshold);
  bool ret = ngram::NGramWriteContexts(out_name, contexts);

  return !ret;
}
