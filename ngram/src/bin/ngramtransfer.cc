// ngramtransfer.cc
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
// Transfers n-grams from a source model(s) to a destination model

#include <ngram/ngram-complete.h>
#include <ngram/ngram-transfer.h>

using namespace fst;
using namespace ngram;

DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_string(context_pattern1, "", "Context pattern for first model");
DEFINE_string(context_pattern2, "", "Context pattern for second model");
DEFINE_string(contexts, "", "Context patterns files (all FSTs)");
DEFINE_string(ofile, "", "Output file (prefix)");
DEFINE_int32(index, -1, "Specifies one FST as the destination (source)");
DEFINE_bool(transfer_from, false, "Transfer from (to) other FSTS to indexed FST");
DEFINE_bool(normalize, false, "Recompute backoff weights after transfer");
DEFINE_bool(complete, false, "Complete partial models");

StdMutableFst *ReadFst(const char *file) {
  string in_name = (strcmp(file, "-") != 0) ? file : "";
  StdMutableFst *fst = StdMutableFst::Read(file, true);
  if (!fst) return 0;
  if (FLAGS_complete)
    NGramComplete(fst);
  return fst;
}

bool GetContexts(int in_count, std::vector<string> *contexts) {
  contexts->clear();
  if (!FLAGS_contexts.empty()) {
    NGramReadContexts(FLAGS_contexts, contexts);
  } else if (!FLAGS_context_pattern1.empty() &&
             !FLAGS_context_pattern2.empty()) {
    contexts->push_back(FLAGS_context_pattern1);
    contexts->push_back(FLAGS_context_pattern2);
  } else {
    LOG(ERROR) << "Context patterns not specified";
    return false;
  }
  if (contexts->size() != in_count) {
    LOG(ERROR) << "Wrong number of context patterns specified";
    return false;
  }
  return true;
}

int main(int argc, char **argv) {
  string usage = "ngramtransfer.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in1.fst in2.fst [out.fst]\n";
  usage += "        ";
  usage += argv[0];
  usage += " [--options] --ofile=out.fst in1.fst in2.fst [in3.fst ...]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 3) {
    ShowUsage(argv[0]);
    return 1;
  }

  string out_name_prefix = FLAGS_ofile.empty() ? (argc > 3 ? argv[3] : "")
      : FLAGS_ofile;

  int in_count = FLAGS_ofile.empty() ? 2 : argc - 1;

  if (FLAGS_index < 0 || FLAGS_index >= in_count) {
    LOG(ERROR) << "Bad FST index: " << FLAGS_index;
    return 1;
  }

  StdMutableFst *index_fst = ReadFst(argv[FLAGS_index + 1]);
  if (!index_fst) return 1;

  std::vector<string> contexts;
  if (!GetContexts(in_count,&contexts))
    return 1;


  if (FLAGS_transfer_from) {
    NGramTransfer transfer(index_fst, contexts[FLAGS_index], FLAGS_backoff_label);
    for (int src = 0; src < in_count; ++src) {
      if (src == FLAGS_index) continue;

      StdMutableFst *fst_src = ReadFst(argv[src + 1]);
      if (!fst_src) return 1;

      transfer.TransferNGramsFrom(*fst_src, contexts[src], FLAGS_normalize);
      delete fst_src;
    }
    index_fst->Write(out_name_prefix);   // no suffix in this case
  }
  else {
    NGramTransfer transfer(*index_fst, contexts[FLAGS_index], FLAGS_backoff_label);
    for (int dest = 0; dest < in_count; ++dest) {
      if (dest == FLAGS_index) continue;

      StdMutableFst *fst_dest = ReadFst(argv[dest + 1]);
      if (!fst_dest) return 1;

      transfer.TransferNGramsTo(fst_dest, contexts[dest], FLAGS_normalize);
      ostringstream suffix;
      suffix.width(5);
      suffix.fill('0');
      suffix << dest;
      string out_name = out_name_prefix + suffix.str();
      fst_dest->Write(out_name);
      delete fst_dest;
    }
  }
  return 0;
}
