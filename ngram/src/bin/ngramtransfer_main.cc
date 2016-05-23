
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
// Transfers n-grams from a source model(s) to a destination model.

#include <vector>

#include <ngram/hist-arc.h>
#include <ngram/ngram-complete.h>
#include <ngram/ngram-transfer.h>

DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_string(context_pattern1, "", "Context pattern for first model");
DEFINE_string(context_pattern2, "", "Context pattern for second model");
DEFINE_string(contexts, "", "Context patterns files (all FSTs)");
DEFINE_string(ofile, "", "Output file (prefix)");
DEFINE_string(method, "count_transfer",
              "One of \"count_transfer\", "
              "\"histogram_transfer\"");
DEFINE_int32(index, -1, "Specifies one FST as the destination (source)");
DEFINE_bool(transfer_from, false,
            "Transfer from (to) other FSTS to indexed FST");
DEFINE_bool(normalize, false, "Recompute backoff weights after transfer");
DEFINE_bool(complete, false, "Complete partial models");

template <class Arc>
bool ReadFst(const char *file, std::unique_ptr<fst::VectorFst<Arc>> *fst) {
  string in_name = (strcmp(file, "-") != 0) ? file : "";
  fst->reset(fst::VectorFst<Arc>::Read(file));
  if (!(*fst) || (FLAGS_complete && !ngram::NGramComplete(fst->get())))
    return false;
  return true;
}

bool GetContexts(int in_count, std::vector<string> *contexts) {
  contexts->clear();
  if (!FLAGS_contexts.empty()) {
    ngram::NGramReadContexts(FLAGS_contexts, contexts);
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

template <class Arc>
bool Transfer(string out_name_prefix, int in_count, char **argv) {
  std::unique_ptr<fst::VectorFst<Arc>> index_fst;
  if (!ReadFst<Arc>(argv[FLAGS_index + 1], &index_fst)) return false;

  std::vector<string> contexts;
  if (!GetContexts(in_count, &contexts)) return false;

  if (FLAGS_transfer_from) {
    ngram::NGramTransfer<Arc> transfer(index_fst.get(), contexts[FLAGS_index],
                                       FLAGS_backoff_label);
    for (int src = 0; src < in_count; ++src) {
      if (src == FLAGS_index) continue;

      std::unique_ptr<fst::VectorFst<Arc>> fst_src;
      if (!ReadFst<Arc>(argv[src + 1], &fst_src) ||
          !transfer.TransferNGramsFrom(*fst_src, contexts[src])) {
        return false;
      }
    }

    // Normalization occurs after all transfer has occurred.
    if (FLAGS_normalize && !transfer.TransferNormalize()) {
      NGRAMERROR() << "Unable to normalize after transfer";
      return false;
    }
    index_fst->Write(out_name_prefix);  // no suffix in this case
  } else {
    ngram::NGramTransfer<Arc> transfer(*index_fst, contexts[FLAGS_index],
                                       FLAGS_backoff_label);
    for (int dest = 0; dest < in_count; ++dest) {
      if (dest == FLAGS_index) continue;

      std::unique_ptr<fst::VectorFst<Arc>> fst_dest;
      if (!ReadFst<Arc>(argv[dest + 1], &fst_dest) ||
          !transfer.TransferNGramsTo(fst_dest.get(), contexts[dest])) {
        return false;
      }
      std::ostringstream suffix;
      suffix.width(5);
      suffix.fill('0');
      suffix << dest;
      string out_name = out_name_prefix + suffix.str();
      fst_dest->Write(out_name);
    }
  }
  return true;
}

int main(int argc, char **argv) {
  string usage = "ngramtransfer.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] --ofile=out.fst in1.fst in2.fst [in3.fst ...]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 3) {
    ShowUsage();
    return 1;
  }

  string out_name_prefix =
      FLAGS_ofile.empty() ? (argc > 3 ? argv[3] : "") : FLAGS_ofile;

  int in_count = FLAGS_ofile.empty() ? 2 : argc - 1;

  if (FLAGS_index < 0 || FLAGS_index >= in_count) {
    LOG(ERROR) << "Bad FST index: " << FLAGS_index;
    return 1;
  }

  if (FLAGS_method == "histogram_transfer") {
    if (!Transfer<ngram::HistogramArc>(out_name_prefix, in_count, argv))
      return 1;
  } else if (FLAGS_method == "count_transfer") {
    if (!Transfer<fst::StdArc>(out_name_prefix, in_count, argv))
      return 1;
  } else {
    LOG(ERROR) << argv[0] << ": bad transfer method: " << FLAGS_method;
    return 1;
  }
  return 0;
}
