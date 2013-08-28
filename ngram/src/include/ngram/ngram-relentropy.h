// ngram-relentropy.h
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
// Stolcke relative entropy style model shrinking derived class

#ifndef NGRAM_NGRAM_RELENTROPY_H__
#define NGRAM_NGRAM_RELENTROPY_H__

#include <ngram/ngram-shrink.h>

namespace ngram {

class NGramRelEntropy : public NGramShrink {
 public:
  // Constructs an NGramRelEntropy object that prunes an LM using relative
  // entropy criterion.
  NGramRelEntropy(StdMutableFst *infst, double theta,
		  int shrink_opt = 0, double tot_uni = -1.0,
		  Label backoff_label = 0, double norm_eps = kNormEps,
		  bool check_consistency = false)
    : NGramShrink(infst, shrink_opt, tot_uni, backoff_label, norm_eps,
		  check_consistency), theta_(theta) {}

  // Shrink n-gram model, based on initialized parameters
  void ShrinkNGramModel() {
    // Threshold provided in real domain, convert to log
    theta_ = log(theta_ + 1);  // e^D - 1 <= theta_ -> D <= log(theta_ + 1)
    NGramShrink::ShrinkNGramModel(true);
  }

 protected:
  // provide the pruning threshold
  double GetTheta(StateId state) const {
    return theta_;
  }

  // Compute shrink score for transition based on Stolcke (KL) formula
  // D(p||p') = -p(h) { p(w|h) [ log p(w|h') + log \alpha'(h) - log p(w|h) ] +
  //            \alpha_numerator(h) [ log \alpha'(h) - log \alpha (h) ] }
  // return exp(D(p||p')) - 1
  double ShrinkScore(const ShrinkStateStats &state,
		     const ShrinkArcStats &arc) const {
    double new_log_backoff = CalcNewLogBackoff(arc);
    double score = arc.log_backoff_prob + new_log_backoff - arc.log_prob;
    double secondterm = new_log_backoff + (GetNLogBackoffNum() -
					   GetNLogBackoffDenom());
    secondterm *= exp(-GetNLogBackoffNum());
    score *= exp(arc.log_prob);
    score += secondterm;
    score *= -exp(state.log_prob);
    return score;
  }

 private:
  double theta_;  // Shrinking parameter
  DISALLOW_COPY_AND_ASSIGN(NGramRelEntropy);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_RELENTROPY_H__
