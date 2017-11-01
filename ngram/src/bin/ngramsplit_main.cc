
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
// Splits an n-gram model based on given context patterns.

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <ngram/ngram-complete.h>
#include <ngram/ngram-split.h>

DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_string(contexts, "", "Context patterns file");
DEFINE_string(method, "count_split",
              "One of: \"count_split\", "
              "\"histogram_split\"");
DEFINE_double(norm_eps, ngram::kNormEps, "Normalization check epsilon");
DEFINE_bool(complete, false, "Complete partial models");

template <class Arc>
bool Split(fst::VectorFst<Arc> *fst, std::vector<string> context_patterns,
           string out_name_prefix) {
  ngram::NGramSplit<Arc> split(*fst, context_patterns, FLAGS_backoff_label,
                               FLAGS_norm_eps);

  for (int i = 0; !split.Done(); ++i) {
    fst::VectorFst<Arc> ofst;
    if (!split.NextNGramModel(&ofst)) return true;
    std::ostringstream suffix;
    suffix.width(5);
    suffix.fill('0');
    suffix << i;
    string out_name = out_name_prefix + suffix.str();
    if (!ofst.Write(out_name)) return true;
  }
  return false;
}

int main(int argc, char **argv) {
  string usage = "ngramsplit.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in_fst [out_fsts_prefix]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3 || argc < 2) {
    ShowUsage();
    return 1;
  }

  string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  string out_name_prefix = argc > 2 ? argv[2] : in_name;

  std::vector<string> context_patterns;

  if (FLAGS_contexts.empty()) {
    LOG(ERROR) << "Context patterns file need to be specified using "
               << "--contexts flag.";
    return 1;
  } else {
    ngram::NGramReadContexts(FLAGS_contexts, &context_patterns);
  }

  if (FLAGS_method == "count_split") {
    std::unique_ptr<fst::StdVectorFst> fst(
        fst::StdVectorFst::Read(in_name));
    if (!fst || (FLAGS_complete && !ngram::NGramComplete(fst.get()))) {
      return 1;
    }
    return Split(fst.get(), context_patterns, out_name_prefix);
  } else if (FLAGS_method == "histogram_split") {
    std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> fst(
        fst::VectorFst<ngram::HistogramArc>::Read(in_name));
    if (!fst || (FLAGS_complete && !ngram::NGramComplete(fst.get()))) {
      return 1;
    }
    return Split(fst.get(), context_patterns, out_name_prefix);
  } else {
    LOG(ERROR) << argv[0] << ": bad split method: " << FLAGS_method;
    return 1;
  }
  return 0;
}
