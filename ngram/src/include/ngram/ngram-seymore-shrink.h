// ngram-seymore-shrink.h
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
// Seymore and Rosenfeld style model shrinking derived class

#ifndef NGRAM_NGRAM_SEYMORESHRINK_H__
#define NGRAM_NGRAM_SEYMORESHRINK_H__

#include <ngram/ngram-shrink.h>

namespace ngram {

class NGramSeymoreShrink : public NGramShrink {
 public:
  // Constructs an NGramSeymoreShrink object that prunes an LM using the
  // Seymore-Rosenfeld criterion.
  NGramSeymoreShrink(StdMutableFst *infst, double theta,
		     int shrink_opt = 0, double tot_uni = -1.0,
		     Label backoff_label = 0, double norm_eps = kNormEps,
		     bool check_consistency = false)
    : NGramShrink(infst, shrink_opt, tot_uni, backoff_label, norm_eps,
		  check_consistency), theta_(theta) {}

  // Shrink n-gram model, based on initialized parameters (req's normalized)
  void ShrinkNGramModel() {
    NGramShrink::ShrinkNGramModel(true);
  }


  // provide the pruning threshold
  double GetTheta(StateId state) const {
    return theta_;
  }

 protected:
  // Compute shrink score for transition based on Seymore/Rosenfeld formula
  // N(w,h) [ log p(w|h) - log p'(w|h) ] where N(w,h) is discounted frequency
  double ShrinkScore(const ShrinkStateStats &state,
		     const ShrinkArcStats &arc) const {
    double score = arc.log_prob - CalcNewLogBackoff(arc);
    score -= arc.log_backoff_prob;
    score *= GetTotalUnigramCount();
    score *= exp(state.log_prob + arc.log_prob);
    return score;
  }

 private:
  double theta_;  // Shrinking parameter
  DISALLOW_COPY_AND_ASSIGN(NGramSeymoreShrink);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_SEYMORESHRINK_H__
