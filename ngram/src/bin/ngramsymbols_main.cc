
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
// Derives a symbol table from an input text corpus.

#include <string>

#include <ngram/ngram-input.h>

DEFINE_string(epsilon_symbol, "<epsilon>", "Label for epsilon");
DEFINE_string(OOV_symbol, "<unk>", "Class label for OOV symbols");

int main(int argc, char **argv) {
  string usage = "Derives a symbol table from a corpus.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.txt [out.txt]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  ngram::NGramInput ingram(
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : string(),
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : string(), "",
      FLAGS_epsilon_symbol, FLAGS_OOV_symbol, "", "");
  return !ingram.ReadInput(0, 1);
}
