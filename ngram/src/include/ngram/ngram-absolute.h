// ngram-absolute.h
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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
//
// Absolute Discounting derived class for smoothing

#ifndef NGRAM_NGRAM_ABSOLUTE_H__
#define NGRAM_NGRAM_ABSOLUTE_H__

#include <vector>

#include<ngram/ngram-make.h>

namespace ngram {

class NGramAbsolute : public NGramMake {
 public:
  // Construct NGramMake object, consisting of the FST and some
  // information about the states under the assumption that the FST is a model.
  // Ownership of the FST is retained by the caller.
  NGramAbsolute(StdMutableFst *infst, 
		bool backoff = false, Label backoff_label = 0,
		double norm_eps = kNormEps, bool check_consistency = false,
		double parameter = 0.0, int bins = -1)
    : NGramMake(infst, backoff, backoff_label, norm_eps, check_consistency),
      parameter_(parameter), bins_(bins) {}

  // Smooth model according to 'method' and parameters.
  void MakeNGramModel();

 protected:
  // Return negative log discounted count for provided negative log count
  double GetDiscount(double neglogcount, int order) const;

  // return the number of bins
  int GetBins() const;

  // increment histogram bin
  void IncrementCountBin(int order, int bin);

 private:
  // Absolute discount for count i
  void CalculateAbsoluteDiscount(int order, int bin) {
    if (parameter_ >= 0) {  // user provided discount value
      discount_[order][bin] = parameter_;
    } else {
      double d = histogram_[order][0];  // rule of thumb: n1 / (n1 + 2 * n2)
      d /= d + 2 * histogram_[order][1];
      // Using Chen and Goodman version from equation (26)
      // For count i, discount: i - (i+1) D n_{i+1} / n_{i}
      //   where D = n_1 / (n_1 + 2 * n_2)
      // Note: discounts stored with indices starting at 0, so bin 0 is count 1
      discount_[order][bin] = bin + 1;
      double n = bin + 2;
      n *= d * histogram_[order][bin + 1];
      n /= histogram_[order][bin];
      discount_[order][bin] -= n;
    }
  }

  // Calculate discounts for each order, according to the requested method
  void CalculateDiscounts();

  vector < vector <double> > histogram_;  // count histogram for orders
  vector < vector <double> > discount_;  // discount for bins
  double parameter_;  // Absolute Discounting D
  int bins_;  // Number of bins for discounting
  DISALLOW_COPY_AND_ASSIGN(NGramAbsolute);
};


}  // namespace ngram

#endif  // NGRAM_NGRAM_ABSOLUTE_H__
