// ngram-make.h
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
// NGram model class for making a model from raw counts

#ifndef NGRAM_NGRAM_MAKE_H__
#define NGRAM_NGRAM_MAKE_H__

#include <vector>

#include<ngram/ngram-mutable-model.h>

namespace ngram {

class NGramMake : public NGramMutableModel {
 public:

  // Construct NGramMake object, consisting of the FST and some
  // information about the states under the assumption that the FST is a model.
  // Ownership of the FST is retained by the caller.
  NGramMake(StdMutableFst *infst, bool backoff, Label backoff_label = 0,
	    double norm_eps = kNormEps, bool check_consistency = false,
	    bool infinite_backoff = false)
    : NGramMutableModel(infst, backoff_label, norm_eps, check_consistency,
			infinite_backoff),
      backoff_(backoff) {}

  virtual ~NGramMake() {}

 protected:
  // Normalize n-gram counts and smooth to create an n-gram model
  void MakeNGramModel();

  // Return negative log discounted count for provided negative log count
  // Need to override if some discounting is done during smoothing
  // Default can be used by non-discounting methods, e.g., Witten-Bell
  virtual double GetDiscount(double nlog_count, int order) const {
    return nlog_count;
  }

  // Additional count mass at state if nothing reserved via smoothing method
  // Override if method requires less or more; usmoothed should be zero
  // Default can be used by most methods
  virtual double EpsilonMassIfNoneReserved() const {
    return 1.0;
  }

  // Return high order count mass (sum of discounted counts)
  // Need to override if high order mass is not defined by discounts
  // Default can be used by discounting methods, e.g., Katz or Absolute Disc.
  virtual double CalculateHiOrderMass(const vector<double> &discounts,
				      double nlog_count) const {
    double discount_norm = discounts[0],  // discounted count of </s>
      KahanVal = 0;  // Value for Kahan summation
    for (int i = 1; i < discounts.size(); ++i)  // Sum discount counts
      discount_norm = NegLogSum(discount_norm, discounts[i], &KahanVal);
    return discount_norm;
  }

  // Return normalization constant given the count and state
  // Need to override if normalization constant is not just the count
  // Default can be used if the normalizing constant is just count
  virtual double CalculateTotalMass(double nlog_count, StateId st) {
    return nlog_count;
  }

 private:
  // Normalize and smooth states, using parameterized smoothing method
  void SmoothState(StateId st);

  // Calculate smoothed value for arc out of a state
  double SmoothVal(double discount_cnt, double norm,
		   double neglog_bo_prob, double backoff_weight) const {
    double value = discount_cnt - norm;
    if (!backoff_) {
      double mixvalue = neglog_bo_prob + backoff_weight;
      value = NegLogSum(value, mixvalue);
    }
    return value;
  }

  // Checks to see if all n-grams already represented at state
  bool HasAllArcsInBackoff(StateId st);

  // Calculate smoothed values for all arcs leaving a state
  void NormalizeStateArcs(StateId st, double Norm, double neglog_bo_prob,
                          const vector<double> &discounts);

  // Collects discounted counts into vector, and returns normalization constant
  double CollectDiscounts(StateId st, vector<double> *discounts) const;

  vector<bool> has_all_ngrams_;
  bool backoff_;  // whether to make the model as backoff or mixture model

  DISALLOW_COPY_AND_ASSIGN(NGramMake);
};


}  // namespace ngram

#endif  // NGRAM_NGRAM_MAKE_H__
