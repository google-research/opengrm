// Copyright 2026 The OpenGrm Authors.
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

// Splits an n-gram model based on given context patterns.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/extensions/far/far.h"
#include "openfst/extensions/far/getters.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/hist-arc.h"
#include "opengrm/ngram/ngram-complete.h"
#include "opengrm/ngram/ngram-context.h"
#include "opengrm/ngram/ngram-split.h"
#include "opengrm/ngram/util.h"

ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(std::string, contexts);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(double, norm_eps);
ABSL_DECLARE_FLAG(bool, complete);
ABSL_DECLARE_FLAG(std::string, far_type);

namespace {

template <class Arc>
bool SplitToFsts(fst::VectorFst<Arc>* fst,
                 absl::Span<const std::string> context_patterns,
                 const std::string& out_name_prefix) {
  ngram::NGramSplit<Arc> split(*fst, context_patterns,
                               absl::GetFlag(FLAGS_backoff_label),
                               absl::GetFlag(FLAGS_norm_eps));
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

void GetSortedPatterns(absl::Span<const std::string> context_patterns,
                       std::vector<std::string>* sorted_full_patterns,
                       std::vector<std::string>* sorted_context_patterns) {
  for (size_t i = 0; i < context_patterns.size(); ++i) {
    sorted_full_patterns->push_back(context_patterns[i] + "/" +
                                    std::to_string(i));
  }
  std::sort(sorted_full_patterns->begin(), sorted_full_patterns->end());
  for (const auto& full_pattern : *sorted_full_patterns) {
    std::vector<std::string> split_pattern =
        absl::StrSplit(full_pattern, '/', absl::SkipEmpty());
    QCHECK_EQ(split_pattern.size(), 2);
    sorted_context_patterns->push_back(split_pattern[0]);
  }
}

template <class Arc>
bool SplitToFar(fst::VectorFst<Arc>* fst,
                absl::Span<const std::string> context_patterns,
                const std::string& out_name_prefix,
                const fst::FarType input_far_type) {
  std::vector<std::string> sorted_full_patterns;
  std::vector<std::string> sorted_context_patterns;
  GetSortedPatterns(context_patterns, &sorted_full_patterns,
                    &sorted_context_patterns);
  ngram::NGramSplit<Arc> split(*fst, sorted_context_patterns,
                               absl::GetFlag(FLAGS_backoff_label),
                               absl::GetFlag(FLAGS_norm_eps));
  std::unique_ptr<fst::FarWriter<Arc>> far_writer(fst::FarWriter<Arc>::Create(
      out_name_prefix + "split_fsts.far", input_far_type));
  if (!far_writer) {
    NGRAMERROR() << "Can't open " << out_name_prefix
                 << "split_fsts.far for writing";
    return true;
  }
  for (size_t i = 0; !split.Done(); ++i) {
    fst::VectorFst<Arc> ofst;
    if (!split.NextNGramModel(&ofst)) return true;
    QCHECK_LT(i, context_patterns.size());
    far_writer->Add(sorted_full_patterns[i], ofst);
  }
  return false;
}

template <class Arc>
bool CountOrHistogramSplit(absl::string_view in_name,
                           absl::Span<const std::string> context_patterns,
                           const std::string& out_name_prefix,
                           const std::string& input_far_type_str) {
  std::unique_ptr<fst::VectorFst<Arc>> fst(fst::VectorFst<Arc>::Read(in_name));
  if (!fst ||
      (absl::GetFlag(FLAGS_complete) && !ngram::NGramComplete(fst.get()))) {
    return true;
  }
  if (input_far_type_str.empty()) {
    // When an empty string far_type is provided, this is not a FarType at all,
    // and it means there is a number of created output FSTs.
    return SplitToFsts(fst.get(), context_patterns, out_name_prefix);
  } else {
    // Otherwise, we expect the user plans to provide a well-formed FarType
    // string representation.
    fst::FarType far_type;
    if (!fst::script::GetFarType(input_far_type_str, &far_type)) {
      NGRAMERROR() << "Unknown or unsupported FAR type: " << input_far_type_str;
      return true;
    }
    if (!SplitToFar(fst.get(), context_patterns, out_name_prefix, far_type)) {
      return true;
    }
  }
  return false;
}

}  // namespace

int ngramsplit_main(int argc, char** argv) {
  std::string usage =
      "Split an n-gram model using context patterns.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] in_fst [out_fsts_prefix]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3 || argc < 2) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  std::string out_name_prefix = argc > 2 ? argv[2] : in_name;

  std::vector<std::string> context_patterns;

  if (absl::GetFlag(FLAGS_contexts).empty()) {
    LOG(ERROR) << "Context patterns file need to be specified using "
               << "--contexts flag.";
    return 1;
  } else {
    ngram::NGramReadContexts(absl::GetFlag(FLAGS_contexts), &context_patterns);
  }

  if (absl::GetFlag(FLAGS_method) == "count_split") {
    return CountOrHistogramSplit<fst::StdArc>(in_name, context_patterns,
                                              out_name_prefix,
                                              absl::GetFlag(FLAGS_far_type));
  } else if (absl::GetFlag(FLAGS_method) == "histogram_split") {
    return CountOrHistogramSplit<ngram::HistogramArc>(
        in_name, context_patterns, out_name_prefix,
        absl::GetFlag(FLAGS_far_type));
  } else {
    LOG(ERROR) << argv[0]
               << ": bad split method: " << absl::GetFlag(FLAGS_method);
    return 1;
  }
  return 0;
}
