
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
// Reads textual model representations and produces n-gram model FST.

#include <string>

#include <ngram/ngram-input.h>

DEFINE_bool(ARPA, false, "Read model from ARPA format");
DEFINE_bool(renormalize_arpa, false,
            "If true, attempts to renormalize an unnormalized ARPA format "
            "model by normalizing the unigram state and recomputing the "
            "backoff weights.  Only used if --ARPA=true.");
DEFINE_string(symbols, "", "Label symbol table");
DEFINE_string(epsilon_symbol, "<epsilon>", "Label for epsilon transitions");
DEFINE_string(OOV_symbol, "<unk>", "Class label for OOV symbols");
DECLARE_string(start_symbol);  // defined in ngram-output.cc
DECLARE_string(end_symbol);    // defined in ngram-output.cc

int main(int argc, char **argv) {
  string usage = "Transform text formats to fst.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.txt [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  ngram::NGramInput ingram(
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : string(),
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : string(),
      FLAGS_symbols, FLAGS_epsilon_symbol, FLAGS_OOV_symbol, FLAGS_start_symbol,
      FLAGS_end_symbol);
  return !ingram.ReadInput(FLAGS_ARPA, /* symbols = */ false,
                           /* output = */ true, FLAGS_renormalize_arpa);
}
