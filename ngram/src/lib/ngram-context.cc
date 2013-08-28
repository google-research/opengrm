// ngram-context.cc
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
// Class to parse and maintain context specifications.

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <fst/fst.h>
#include <ngram/ngram-context.h>

namespace ngram {

using fst::StdArc;
using std::map;
using std::ifstream;
using std::istream;
using std::ofstream;
using std::ostream;

// If 'include_all_suffixes' is true, then all suffixes of the
// begin and end contexts are considered in context. When false,
// true (reverse) lexicographic order is used.
bool NGramContext::HasContext(vector<Label> ngram, bool include_all_suffixes)
    const {
  if (NullContext())  // accept all
    return true;

  reverse(ngram.begin(), ngram.end());

  vector<Label>::const_iterator context_begin_end;
  if (include_all_suffixes) {
    // Truncate context_begin to ensure all state n-gram suffixes accepted
    context_begin_end =  context_begin_.begin() + ngram.size();
  } else {
    // True (reverse) lexicographic order
    context_begin_end =  context_begin_.end();
  }
  ngram.resize(hi_order_ - 1, 0);

  bool less_begin =
      lexicographical_compare(ngram.begin(), ngram.end(),
			      context_begin_.begin(),
			      context_begin_end);
  bool less_end =
      lexicographical_compare(ngram.begin(), ngram.end(),
			      context_end_.begin(),
                              context_end_.end());

  return !less_begin && less_end;
}

// Derives begin and end context vectors from input context pattern
// string.
void NGramContext::ParseContextInterval(const string &context_pattern,
                                        vector<Label> *context_begin,
                                        vector<Label> *context_end) {
  context_begin->clear();
  context_end->clear();

  if (context_pattern.empty())
    return;

  char line[1024];
  vector<char *> contexts;
  vector<char *> labels1, labels2;
  strncpy(line, context_pattern.c_str(), 1024);
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

// Given a n-gram model, returns 'ncontext' contexts balanced for
// size.  Arg 'bigram_thresh' determines how overfull a context
// bin has to be to force a split at a bigram context.  The model
// must have state n-grams enabled.
void NGramContext::FindContexts(
    const NGramModel &model,
    int ncontexts,
    vector< vector<Label> > *begin_contexts,
    vector< vector<Label> > *end_contexts,
    float bigram_thresh) {

  // state n-gram counts with given unigram suffix
  map<Label, size_t> suffix1_counts;
  // state n-gram counts with given (reversed) bigram suffix
  map<pair<Label, Label>, size_t> suffix2_counts;
  // state n-gram counts at a bigram state
  map<Label, size_t> bigram_counts;
  size_t total_count = 0;
  Label max_label = kNoLabel;

  for (StateId s = 0; s < model.NumStates(); ++s) {
    for (ArcIterator<StdFst> aiter(model.GetFst(), s);
         !aiter.Done();
         aiter.Next()) {
      const StdArc &arc = aiter.Value();
      if (arc.ilabel == kNoLabel || arc.ilabel > max_label)
        max_label = arc.ilabel;
    }
    const vector<Label> &ngram = model.StateNGram(s);
    Label l1 = ngram.size() > 0 ? ngram[ngram.size() - 1] : kNoLabel;
    Label l2 = ngram.size() > 1 ? ngram[ngram.size() - 2] : kNoLabel;
    // if (l1 == 0) continue;
    if (l1 == kNoLabel) continue;
    suffix1_counts[l1] += model.GetFst().NumArcs(s);
    total_count += model.GetFst().NumArcs(s);

    if (l2 != kNoLabel) {
      suffix2_counts[make_pair(l1, l2)] += model.GetFst().NumArcs(s);
    } else {
      bigram_counts[l1] += model.GetFst().NumArcs(s);
    }
  }
  vector<Label> context;
  begin_contexts->clear();
  end_contexts->clear();
  begin_contexts->push_back(context);
  begin_contexts->back().push_back(0);
  ssize_t bin_count = 0;
  map<Label, size_t>::const_iterator it1 = suffix1_counts.begin();

  while (it1 != suffix1_counts.end()) {
    Label suffix1 = it1->first;
    ssize_t delta1 = it1->second;
    ssize_t deltab = bigram_counts[suffix1];
    ++it1;
    if (it1 != suffix1_counts.end() &&
        (bin_count + delta1) * ncontexts < total_count) {
      // Continues to fill bin
      bin_count += delta1;
    } else if ((bin_count + delta1 - deltab) * ncontexts
               < bigram_thresh * total_count) {
      // Splits at a unigram state suffix when bin not overfull
      if (it1 == suffix1_counts.end()) {
        // Finalizes at end of contexts
        end_contexts->push_back(context);
        end_contexts->back().push_back(max_label + 1);
      } else {
        // Splits and continues with next unigram state suffix
        Label next_suffix1 = it1->first;
        end_contexts->push_back(context);
        begin_contexts->push_back(context);
        end_contexts->back().push_back(next_suffix1);
        begin_contexts->back().push_back(next_suffix1);
        total_count -= bin_count + delta1;
        --ncontexts;
        bin_count = 0;
      }
    } else {
      // Splits at a bigram state context
      total_count -= deltab;
      map<pair<Label, Label>, size_t>::const_iterator it2 =
          suffix2_counts.find(pair<Label, Label>(suffix1, 0));
      while (it2 != suffix2_counts.end() && it2->first.first <= suffix1) {
        ssize_t delta2 = it2->second;
        ++it2;
        if (it1 != suffix1_counts.end() &&
            (bin_count + delta2) * ncontexts < total_count) {
          // Continues to fill bin
          bin_count += delta2;
        } else if (it1 == suffix1_counts.end()) {
          // Finalizes at end of contexts
          end_contexts->push_back(context);
          end_contexts->back().push_back(max_label + 1);
        } else if (it2 == suffix2_counts.end() || it2->first.first > suffix1) {
         // Splits and continues with next unigram state suffix
          Label next_suffix1 = it1->first;
          end_contexts->push_back(context);
          begin_contexts->push_back(context);
          end_contexts->back().push_back(next_suffix1);
          begin_contexts->back().push_back(next_suffix1);
          total_count -= bin_count + delta2;
          --ncontexts;
          bin_count = 0;
        } else {
          // Splits and continues with next bigram state suffix
          pair<Label, Label> next_suffix2 = it2->first;
          end_contexts->push_back(context);
          begin_contexts->push_back(context);
          end_contexts->back().push_back(next_suffix2.second);
          end_contexts->back().push_back(next_suffix2.first);
          begin_contexts->back().push_back(next_suffix2.second);
          begin_contexts->back().push_back(next_suffix2.first);
          total_count -= bin_count + delta2;
          --ncontexts;
          bin_count = 0;
        }
      }
    }
  }
}

void NGramContext::Init() {
  if (NullContext())
    return;

  reverse(context_begin_.begin(), context_begin_.end());
  reverse(context_end_.begin(), context_end_.end());
  if (context_begin_.size() >= hi_order_)
    hi_order_ = context_begin_.size() + 1;
  if (context_end_.size() >= hi_order_)
    hi_order_ = context_end_.size() + 1;
  context_begin_.resize(hi_order_ - 1, 0);
  context_end_.resize(hi_order_ - 1, 0);
  if (!lexicographical_compare(context_begin_.begin(), context_begin_.end(),
                               context_end_.begin(), context_end_.end()))
    LOG(FATAL) << "NGramContext: bad context interval";
}

// Reads context specifications form a file into a vector.
bool NGramReadContexts(const string &file, vector<string> *contexts) {
  contexts->clear();
  istream *strm = &cin;
  if (!file.empty()) {
    strm = new ifstream(file.c_str());
    if (!*strm) {
      LOG(ERROR) << "NGramReadContexts: Can't open file: " << file;
      return false;
    }
  }
  string line;
  while (getline(*strm, line))
    contexts->push_back(line);

  if (strm != &cin)
    delete strm;
  return true;
}

// Writes context specifications from a vector to a file.
bool NGramWriteContexts(const string &file, const vector<string> &contexts) {
  ostream *strm = &cout;
  if (!file.empty()) {
    strm = new ofstream(file.c_str());
    if (!*strm) {
      LOG(ERROR) << "NGramWriteContexts: Can't create file: " << file;
      return false;
    }
  }
  for (int i = 0; i < contexts.size(); ++i)
    *strm << contexts[i] << endl;

  bool ret = strm;
  if (strm != &cout)
    delete strm;
  return ret;
}

}  // namespace ngram
