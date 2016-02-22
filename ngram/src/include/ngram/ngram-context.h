// ngram-context.h
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

#ifndef NGRAM_NGRAM_CONTEXT_H__
#define NGRAM_NGRAM_CONTEXT_H__

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <fst/fst.h>
#include <ngram/ngram-model.h>

namespace ngram {

using fst::StdArc;
using std::ostringstream;
using std::vector;

class NGramContext {
 public:
  typedef StdArc::Label Label;
  typedef StdArc::StateId StateId;

  // Constructs a context specification from begin and end context
  // vectors.  If the context is less than the n-gram order - 1, it is
  // padded with 0 on the left. The begin and end context vectors
  // specify a (half-open) interval of highest-order state contexts in
  // an LM with the interval defined using the reverse lexicographic
  // order (i.e., on the reverse of the context). All suffixes of
  // these contexts are also included for proper backoff (when
  // include_all_suffixes = true).
  //
  // Example1: context_begin = {1,1,1,1} and context_end = {1,1,1,5} with
  // a 5-gram:
  //   specifies states that have a rightmost context in [1,5).
  //
  // Example2: context_begin = {1} and context_end = {5,6} with a 5-gram:
  //   same as context_begin = {1,0,0,0} and context_end = {5,6,0,0}.
  NGramContext(const vector<Label> &context_begin,
               const vector<Label> &context_end, int hi_order)
      : hi_order_(hi_order),
        context_begin_(context_begin),
        context_end_(context_end) {
    Init();
  }

  // Constructs a context specification from context pattern string.
  // Expected format: "w_1 ...w_m : v_1 ... v_n" where
  // the w_i and v_i are numeric word IDs and m,n are typically less than
  // the n-gram order. A word ID 0 signfies the initial word.
  //
  // Example: "1 1 1 1 : 1 1 1 5" signifies a begin context vector of
  //   {1,1,1,1} and and end context vector of {1,1,1,5}. See next constructor
  //   for the behavior with these vectors.
  NGramContext(const string &context_pattern, int hi_order)
      : hi_order_(hi_order) {
    ParseContextInterval(context_pattern, &context_begin_, &context_end_);
    Init();
  }

  // Null context
  NGramContext() : hi_order_(0) {}

  // If 'include_all_suffixes' is true, then all suffixes of the
  // begin and end contexts are considered in context. When false,
  // true (reverse) lexicographic order is used.
  bool HasContext(vector<Label> ngram, bool include_all_suffixes = true) const;

  // No/empty context requested?
  int NullContext() const { return context_begin_.empty(); }

  // Derives begin and end context vectors from input context pattern
  // string.
  static void ParseContextInterval(const string &context_pattern,
                                   vector<Label> *context_begin,
                                   vector<Label> *context_end);


  // Generates context string from begin and end context vectors
  static string GetContextString(const vector<Label> &context_begin,
                                 const vector<Label> &context_end) {
    ostringstream context_pattern_strm;
    for (int i = 0; i < context_begin.size(); ++i)
      context_pattern_strm << context_begin[i] << " ";
    context_pattern_strm << ":";
    for (int i = 0; i < context_end.size(); ++i)
      context_pattern_strm << " " << context_end[i];
    return context_pattern_strm.str();
  }


  // Given a n-gram model, returns 'ncontext' contexts balanced for
  // size.  Arg 'bigram_thresh' determines how overfull a context
  // bin has to be to force a split at a bigram context.  The model
  // must have state n-grams enabled.
  static void FindContexts(const NGramModel &model,
                           int ncontexts,
                           vector<string> *contexts,
                           float bigram_thresh = 1.1) {
    vector< vector<Label> > begin_contexts;
    vector< vector<Label> > end_contexts;
    FindContexts(model, ncontexts, &begin_contexts, &end_contexts, bigram_thresh);

    for (int i = 0; i < begin_contexts.size(); ++i)
      contexts->push_back(GetContextString(begin_contexts[i],
                                           end_contexts[i]));
  }


  // Given a n-gram model, returns 'ncontext' contexts balanced for
  // size.  Arg 'bigram_thresh' determines how overfull a context
  // bin has to be to force a split at a bigram context.  The model
  // must have state n-grams enabled.
  static void FindContexts(const NGramModel &model,
                           int ncontexts,
                           vector< vector<StdArc::Label> > *begin_contexts,
                           vector< vector<StdArc::Label> > *end_contexts,
                           float bigram_thresh = 1.1);
 private:
  void Init();

  int hi_order_;
  vector<Label> context_begin_;
  vector<Label> context_end_;
  DISALLOW_COPY_AND_ASSIGN(NGramContext);
};

// Reads context specifications form a file into a vector.
bool NGramReadContexts(const string &file, vector<string> *contexts);
// Writes context specifications from a vector to a file.
bool NGramWriteContexts(const string &file, const vector<string> &contexts);

}  // namespace ngram

#endif  // NGRAM_NGRAM_CONTEXT_H__
