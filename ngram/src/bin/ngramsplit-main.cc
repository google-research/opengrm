
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

#include <fst/extensions/far/far.h>
#include <fst/extensions/far/getters.h>
#include <ngram/ngram-complete.h>
#include <ngram/ngram-split.h>

DECLARE_int64(backoff_label);
DECLARE_string(contexts);
DECLARE_string(method);
DECLARE_double(norm_eps);
DECLARE_bool(complete);
DECLARE_string(far_type);

namespace {

template <class Arc>
bool Split(fst::VectorFst<Arc> *fst,
           const std::vector<std::string> &context_patterns,
           const std::string &out_name_prefix) {
  ngram::NGramSplit<Arc> split(*fst, context_patterns, FLAGS_backoff_label,
                               FLAGS_norm_eps);
  for (int i = 0; !split.Done(); ++i) {
    fst::VectorFst<Arc> ofst;
    if (!split.NextNGramModel(&ofst)) return true;
    std::ostringstream suffix;
    suffix.width(5);
    suffix.fill('0');
    suffix << i;
    const auto out_name = out_name_prefix + suffix.str();
    if (!ofst.Write(out_name)) return true;
  }
  return false;
}

void GetSortedPatterns(const std::vector<std::string> &context_patterns,
                       std::vector<std::string> *sorted_full_patterns,
                       std::vector<std::string> *sorted_context_patterns) {
  for (size_t i = 0; i < context_patterns.size(); ++i) {
    sorted_full_patterns->push_back(context_patterns[i] + "/" +
                                    std::to_string(i));
  }
  std::sort(sorted_full_patterns->begin(), sorted_full_patterns->end());
  for (auto full_pattern : *sorted_full_patterns) {
    std::vector<std::string> split_pattern =
        ::fst::StringSplit(full_pattern, '/');
    CHECK_EQ(split_pattern.size(), 2);
    sorted_context_patterns->push_back(split_pattern[0]);
  }
}

template <class Arc>
bool Split(fst::VectorFst<Arc> *fst,
           const std::vector<std::string> &context_patterns,
           const std::string &out_name_prefix,
           const std::string &input_far_type) {
  if (input_far_type.empty()) {
    return Split(fst, context_patterns, out_name_prefix);
  }
  std::vector<std::string> sorted_full_patterns;
  std::vector<std::string> sorted_context_patterns;
  GetSortedPatterns(context_patterns, &sorted_full_patterns,
                    &sorted_context_patterns);
  ngram::NGramSplit<Arc> split(*fst, sorted_context_patterns,
                               FLAGS_backoff_label, FLAGS_norm_eps);
  std::unique_ptr<fst::FarWriter<Arc>> far_writer(
      fst::FarWriter<Arc>::Create(
          out_name_prefix + "split_fsts.far",
          fst::script::GetFarType(input_far_type)));
  if (!far_writer) {
    NGRAMERROR() << "Can't open " << out_name_prefix
                 << "split_fsts.far for writing";
    return true;
  }
  for (size_t i = 0; !split.Done(); ++i) {
    fst::VectorFst<Arc> ofst;
    if (!split.NextNGramModel(&ofst)) return true;
    CHECK_LT(i, context_patterns.size());
    far_writer->Add(sorted_full_patterns[i], ofst);
  }
  return false;
}

}  // namespace

int ngramsplit_main(int argc, char **argv) {
  std::string usage =
      "Split an n-gram model using context patterns.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in_fst [out_fsts_prefix]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3 || argc < 2) {
    ShowUsage();
    return 1;
  }

  std::string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  std::string out_name_prefix = argc > 2 ? argv[2] : in_name;

  std::vector<std::string> context_patterns;

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
    return Split(fst.get(), context_patterns, out_name_prefix,
                 FLAGS_far_type);
  } else if (FLAGS_method == "histogram_split") {
    std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> fst(
        fst::VectorFst<ngram::HistogramArc>::Read(in_name));
    if (!fst || (FLAGS_complete && !ngram::NGramComplete(fst.get()))) {
      return 1;
    }
    return Split(fst.get(), context_patterns, out_name_prefix,
                 FLAGS_far_type);
  } else {
    LOG(ERROR) << argv[0] << ": bad split method: " << FLAGS_method;
    return 1;
  }
  return 0;
}
