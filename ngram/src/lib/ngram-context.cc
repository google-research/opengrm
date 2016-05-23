
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
// Class to parse and maintain context specifications.

#include <algorithm>
#include <cstring>
#include <fstream>
#include <istream>
#include <ostream>
#include <map>
#include <string>
#include <vector>

#include <fst/fst.h>
#include <ngram/ngram-context.h>

namespace ngram {

using fst::StdArc;
using std::map;

bool NGramContext::HasContext(vector<Label> ngram,
                              bool include_all_suffixes) const {
  if (NullContext())  // accept all
    return true;

  reverse(ngram.begin(), ngram.end());

  vector<Label>::const_iterator context_begin_end;
  if (include_all_suffixes) {
    // Truncate context_begin to ensure all state n-gram suffixes accepted
    context_begin_end = context_begin_.begin() + ngram.size();
  } else {
    // True (reverse) lexicographic order
    context_begin_end = context_begin_.end();
  }
  ngram.resize(hi_order_ - 1, 0);

  bool less_begin = lexicographical_compare(
      ngram.begin(), ngram.end(), context_begin_.begin(), context_begin_end);
  bool less_end = lexicographical_compare(
      ngram.begin(), ngram.end(), context_end_.begin(), context_end_.end());

  return !less_begin && less_end;
}

void NGramContext::ParseContextInterval(const string &context_pattern,
                                        vector<Label> *context_begin,
                                        vector<Label> *context_end) {
  context_begin->clear();
  context_end->clear();

  if (context_pattern.empty()) return;

  const int linelen = 1024;
  if (context_pattern.size() >= linelen)
    LOG(FATAL) << "NGramContext::ParseContextInterval: "
               << "context pattern too long";
  char line[linelen];
  vector<char *> contexts;
  vector<char *> labels1, labels2;
  strncpy(line, context_pattern.c_str(), linelen);
  fst::SplitToVector(line, ":", &contexts, true);
  if (contexts.size() != 2)
    LOG(FATAL) << "NGramContext: bad context pattern: " << context_pattern;
  fst::SplitToVector(contexts[0], " ", &labels1, true);
  fst::SplitToVector(contexts[1], " ", &labels2, true);
  for (int i = 0; i < labels1.size(); ++i) {
    Label label = fst::StrToInt64(labels1[i], "context begin", 1, false);
    context_begin->push_back(label);
  }
  for (int i = 0; i < labels2.size(); ++i) {
    Label label = fst::StrToInt64(labels2[i], "context end", 1, false);
    context_end->push_back(label);
  }
}

void NGramContext::Init() {
  if (NullContext()) return;

  reverse(context_begin_.begin(), context_begin_.end());
  reverse(context_end_.begin(), context_end_.end());
  if (context_begin_.size() >= hi_order_) hi_order_ = context_begin_.size() + 1;
  if (context_end_.size() >= hi_order_) hi_order_ = context_end_.size() + 1;
  context_begin_.resize(hi_order_ - 1, 0);
  context_end_.resize(hi_order_ - 1, 0);
  if (!lexicographical_compare(context_begin_.begin(), context_begin_.end(),
                               context_end_.begin(), context_end_.end()))
    LOG(FATAL) << "NGramContext: bad context interval";
}

void NGramExtendedContext::Init(bool merge_contexts) {
  ContextCompare context_cmp;
  std::sort(contexts_.begin(), contexts_.end(), context_cmp);
  if (contexts_.size() == 0) return;

  if (contexts_.size() == 1 && contexts_[0].NullContext()) {
    contexts_.pop_back();
    return;
  }

  if (!CheckContexts()) {
    LOG(FATAL) << "NGramContexts: bad contexts";
  }

  if (merge_contexts) {
    size_t i = 0;
    size_t j = 1;
    size_t k = 0;
    for (; j < contexts_.size(); ++j) {
      const vector<Label> &e1 = contexts_[j - 1].GetReverseContextEnd();
      const vector<Label> &b2 = contexts_[j].GetReverseContextBegin();
      if (e1 != b2) {
        MergeContexts(i, j - 1, k++);
        i = j;
      }
    }
    MergeContexts(i, j - 1, k++);
    contexts_.resize(k);
  }
}

bool NGramExtendedContext::CheckContexts() {
  int hi_order = 0;
  for (size_t i = 0; i < contexts_.size(); ++i) {
    if (contexts_[i].NullContext()) {
      LOG(WARNING) << "CheckContexts: null context";
      return false;
    }
    if (hi_order < contexts_[i].GetHiOrder())
      hi_order = contexts_[i].GetHiOrder();
  }

  for (size_t i = 0; i < contexts_.size(); ++i)
    contexts_[i].SetHiOrder(hi_order);

  for (size_t i = 1; i < contexts_.size(); ++i) {
    const vector<Label> &e1 = contexts_[i - 1].GetReverseContextEnd();
    const vector<Label> &b2 = contexts_[i].GetReverseContextBegin();
    if (lexicographical_compare(b2.begin(), b2.end(), e1.begin(), e1.end())) {
      LOG(WARNING) << "CheckContexts: over-lapping context intevals";
      return false;
    };
  }

  return true;
}

void NGramExtendedContext::MergeContexts(size_t i, size_t j, size_t k) {
  if (i != j) {  // non-trivial merge?
    const vector<Label> &b1 = contexts_[i].GetContextBegin();
    const vector<Label> &e2 = contexts_[j].GetContextEnd();
    int hi_order = contexts_[0].GetHiOrder();
    NGramContext context(b1, e2, hi_order);
    contexts_[k] = context;
  } else if (i != k) {  // non-trivial move?
    contexts_[k] = contexts_[i];
  }
}

void NGramExtendedContext::ParseContextIntervals(
    const string &extended_context_pattern, int hi_order,
    vector<NGramContext> *contexts) {
  contexts->clear();
  int linelen = extended_context_pattern.size() + 1;
  std::unique_ptr<char[]> line(new char[linelen]);
  strncpy(line.get(), extended_context_pattern.c_str(), linelen);
  vector<char *> context_patterns;
  fst::SplitToVector(line.get(), ",", &context_patterns, true);

  for (size_t i = 0; i < context_patterns.size(); ++i)
    contexts->push_back(NGramContext(context_patterns[i], hi_order));
}

bool NGramExtendedContext::HasContext(const vector<Label> &ngram,
                                      bool include_all_suffixes) const {
  if (contexts_.empty())
    return true;
  else
    return GetContext(ngram, include_all_suffixes) != 0;
}

const NGramContext *NGramExtendedContext::GetContext(
    const vector<Label> &ngram, bool include_all_suffixes) const {
  vector<Label> ngram_end(ngram);
  if (ngram_end.empty()) ngram_end.push_back(0);
  vector<Label> ngram_beg(ngram_end);
  ++ngram_end[0];  // ensures non-empty interval below
  int hi_order = contexts_[0].GetHiOrder();
  NGramContext ngram_context(ngram_beg, ngram_end, hi_order);
  ContextCompare context_cmp;
  auto it = upper_bound(contexts_.begin(), contexts_.end(), ngram_context,
                        context_cmp);
  if (it != contexts_.begin() &&
      (it - 1)->HasContext(ngram, include_all_suffixes)) {
    return &*(it - 1);  // strict match
  } else if (include_all_suffixes && it != contexts_.end() &&
             it->HasContext(ngram, include_all_suffixes)) {
    return &*it;  // suffix match
  } else {
    return 0;  // no match (or null context)
  }
}

bool NGramReadContexts(const string &file, vector<string> *contexts) {
  contexts->clear();
  std::ifstream fstrm;
  fstrm.open(file);
  if (!fstrm) {
    LOG(ERROR) << "NGramReadContexts: Can't open file: " << file;
    return false;
  }
  std::istream &strm = fstrm.is_open() ? fstrm : std::cin;
  string line;
  while (getline(strm, line)) contexts->push_back(line);
  return true;
}

bool NGramWriteContexts(const string &file, const vector<string> &contexts) {
  std::ofstream ofstrm;
  if (!file.empty()) {
    ofstrm.open(file);
    if (!ofstrm) {
      LOG(ERROR) << "NGramWriteContexts: Can't create file: " << file;
      return false;
    }
  }
  std::ostream &strm = ofstrm.is_open() ? ofstrm : std::cout;
  for (int i = 0; i < contexts.size(); ++i) strm << contexts[i] << std::endl;
  return true;
}

}  // namespace ngram
