
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
// Context pruning style model shrinking derived class.

#ifndef NGRAM_NGRAM_CONTEXT_PRUNE_H_
#define NGRAM_NGRAM_CONTEXT_PRUNE_H_

#include <string>
#include <vector>

#include <ngram/ngram-context.h>
#include <ngram/ngram-count-prune.h>
#include <ngram/ngram-relentropy.h>
#include <ngram/ngram-seymore-shrink.h>
#include <ngram/ngram-shrink.h>

namespace ngram {

// Context-restricting pruning
class NGramContextPrune : public NGramShrink<StdArc> {
 public:
  // Constructs an NGramShrink object, including an NGramModel and
  // parameters.  This version is passed a context pattern string;  see
  // 'ngram-context.h' for meaning of the context specification.  The
  // specified contexts will NOT be pruned from the model; all others
  // will be (where possible to maintain a well-formed LM).
  explicit NGramContextPrune(StdMutableFst *infst,
                             const string &context_pattern = "",
                             int shrink_opt = 0, double tot_uni = -1.0,
                             Label backoff_label = 0,
                             double norm_eps = kNormEps,
                             bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramShrink<StdArc>(infst, shrink_opt < 2 ? shrink_opt : 0, tot_uni,
                            backoff_label, norm_eps, true),
        context_(context_pattern, HiOrder()) {}

  // Constructs an NGramShrink object, including an NGramModel and
  // parameters.  This version is given begin and end context
  // vectors; see 'ngram-context.h' for meaning of the context
  // specification.  The specified contexts will NOT be pruned from
  // the model; all others will be (where possible to maintaine a
  // well-formed LM).
  NGramContextPrune(StdMutableFst *infst,
                    const std::vector<Label> &context_begin,
                    const std::vector<Label> &context_end, int shrink_opt = 0,
                    double tot_uni = -1.0, Label backoff_label = 0,
                    double norm_eps = kNormEps, bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramShrink(infst, shrink_opt < 2 ? shrink_opt : 0, tot_uni,
                    backoff_label, norm_eps, true),
        context_(context_begin, context_end, HiOrder()) {}
  ~NGramContextPrune() override {}

  // Shrinks n-gram model, based on initialized parameters
  bool ShrinkNGramModel() {
    return NGramShrink<StdArc>::ShrinkNGramModel(false);
  }

 protected:
  // Gives the pruning threshold
  double GetTheta(StateId state) const override { return 0.0; }

  double ShrinkScore(const ShrinkStateStats &state,
                     const ShrinkArcStats &arc) const override {
    const std::vector<Label> &ngram = StateNGram(state.state);
    return context_.HasContext(ngram) ? 1.0 : -1.0;
  }

 private:
  NGramContext context_;  // context specification
};

// Joint context-restricting and count pruning
class NGramContextCountPrune : public NGramCountPrune {
 public:
  NGramContextCountPrune(StdMutableFst *infst,
                         const string &count_pattern,
                         string context_pattern, int shrink_opt = 0,
                         double tot_uni = -1.0, Label backoff_label = 0,
                         double norm_eps = kNormEps,
                         bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramCountPrune(infst, count_pattern, shrink_opt < 2 ? shrink_opt : 0,
                        tot_uni, backoff_label, norm_eps, true),
        context_(context_pattern, HiOrder()) {}

  NGramContextCountPrune(StdMutableFst *infst,
                         const std::vector<double> &count_minimums,
                         const std::vector<Label> &context_begin,
                         const std::vector<Label> &context_end,
                         int shrink_opt = 0, double tot_uni = -1.0,
                         Label backoff_label = 0, double norm_eps = kNormEps,
                         bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramCountPrune(infst, count_minimums, shrink_opt < 2 ? shrink_opt : 0,
                        tot_uni, backoff_label, norm_eps, true),
        context_(context_begin, context_end, HiOrder()) {}

  ~NGramContextCountPrune() override {}

  // Shrinks n-gram model, based on initialized parameters
  bool ShrinkNGramModel() { return NGramCountPrune::ShrinkNGramModel(); }

 protected:
  double ShrinkScore(const ShrinkStateStats &state,
                     const ShrinkArcStats &arc) const override {
    const std::vector<Label> &ngram = StateNGram(state.state);
    if (context_.HasContext(ngram)) {
      return NGramCountPrune::ShrinkScore(state, arc);
    } else {
      return GetTheta(state.state) - 1.0;
    }
  }

 private:
  NGramContext context_;  // context specification

  NGramContextCountPrune(const NGramContextCountPrune &) = delete;
};

// Joint context-restricting and relative entropy pruning
class NGramContextRelEntropy : public NGramRelEntropy {
 public:
  NGramContextRelEntropy(StdMutableFst *infst, double theta,
                         const string &context_pattern, int shrink_opt = 0,
                         double tot_uni = -1.0, Label backoff_label = 0,
                         double norm_eps = kNormEps,
                         bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramRelEntropy(infst, theta, shrink_opt < 2 ? shrink_opt : 0, tot_uni,
                        backoff_label, norm_eps, true),
        context_(context_pattern, HiOrder()) {}

  NGramContextRelEntropy(StdMutableFst *infst, double theta,
                         const std::vector<Label> &context_begin,
                         const std::vector<Label> &context_end,
                         int shrink_opt = 0, double tot_uni = -1.0,
                         Label backoff_label = 0, double norm_eps = kNormEps,
                         bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramRelEntropy(infst, theta, shrink_opt < 2 ? shrink_opt : 0, tot_uni,
                        backoff_label, norm_eps, true),
        context_(context_begin, context_end, HiOrder()) {}

  ~NGramContextRelEntropy() override {}

  // Shrinks n-gram model, based on initialized parameters
  bool ShrinkNGramModel() { return NGramRelEntropy::ShrinkNGramModel(); }

 protected:
  double ShrinkScore(const ShrinkStateStats &state,
                     const ShrinkArcStats &arc) const override {
    const std::vector<Label> &ngram = StateNGram(state.state);
    if (context_.HasContext(ngram)) {
      return NGramRelEntropy::ShrinkScore(state, arc);
    } else {
      return GetTheta(state.state) - 1.0;
    }
  }

 private:
  NGramContext context_;  // context specification

  NGramContextRelEntropy(const NGramContextRelEntropy &) = delete;
  NGramContextRelEntropy &operator=(const NGramContextRelEntropy &) = delete;
};

// Joint context-restricting and SeymoreShrink-Rosenfeld pruning
class NGramContextSeymoreShrink : public NGramSeymoreShrink {
 public:
  NGramContextSeymoreShrink(StdMutableFst *infst, double theta,
                            const string &context_pattern, int shrink_opt = 0,
                            double tot_uni = -1.0, Label backoff_label = 0,
                            double norm_eps = kNormEps,
                            bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramSeymoreShrink(infst, theta, shrink_opt < 2 ? shrink_opt : 0,
                           tot_uni, backoff_label, norm_eps, true),
        context_(context_pattern, HiOrder()) {}

  NGramContextSeymoreShrink(StdMutableFst *infst, double theta,
                            const std::vector<Label> &context_begin,
                            const std::vector<Label> &context_end,
                            int shrink_opt = 0, double tot_uni = -1.0,
                            Label backoff_label = 0, double norm_eps = kNormEps,
                            bool check_consistency = false)
      // shrink_opt must be less than 2 for context pruning
      : NGramSeymoreShrink(infst, theta, shrink_opt < 2 ? shrink_opt : 0,
                           tot_uni, backoff_label, norm_eps, true),
        context_(context_begin, context_end, HiOrder()) {}

  ~NGramContextSeymoreShrink() override {}

  // Shrinks n-gram model, based on initialized parameters
  bool ShrinkNGramModel() { return NGramSeymoreShrink::ShrinkNGramModel(); }

 protected:
  double ShrinkScore(const ShrinkStateStats &state,
                     const ShrinkArcStats &arc) const override {
    const std::vector<Label> &ngram = StateNGram(state.state);
    if (context_.HasContext(ngram)) {
      return NGramSeymoreShrink::ShrinkScore(state, arc);
    } else {
      return GetTheta(state.state) - 1.0;
    }
  }

 private:
  NGramContext context_;  // context specification

  NGramContextSeymoreShrink(const NGramContextSeymoreShrink &) = delete;
  NGramContextSeymoreShrink
      &operator=(const NGramContextSeymoreShrink &) = delete;
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_CONTEXT_PRUNE_H_
