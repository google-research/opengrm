// ngramread.cc
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
// TODO: file description & doc

#include <string>
#include <ngram/ngram-input.h>

using namespace fst;
using namespace ngram;

DEFINE_bool(ARPA, false, "Read model from ARPA format");
DEFINE_string(symbols, "", "Label symbol table");
DEFINE_string(epsilon_symbol, "<epsilon>", "Label for epsilon transitions");
DEFINE_string(OOV_symbol, "<unk>", "Class label for OOV symbols");
DEFINE_string(start_symbol, "<s>", "Class label for sentence start");
DEFINE_string(end_symbol, "</s>", "Class label for sentence start");

int main(int argc, char **argv) {
  string usage = "Transform text formats to fst.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.txt [out.fst]]\n";
  InitFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  NGramInput ingram = NGramInput(
    (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : string(),
    (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : string(),
    FLAGS_symbols, FLAGS_epsilon_symbol, FLAGS_OOV_symbol, 
    FLAGS_start_symbol, FLAGS_end_symbol);
  ingram.ReadInput(FLAGS_ARPA, 0);
  return 0;
}
