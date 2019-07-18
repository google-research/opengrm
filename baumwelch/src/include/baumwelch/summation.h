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
// Copyright 2017 and onwards Google, Inc.

#ifndef BAUMWELCH_SUMMATION_H_
#define BAUMWELCH_SUMMATION_H_

#include <type_traits>

#include <fst/float-weight.h>

namespace fst {
namespace internal {

// Class storing the summation of multiple weights. If the input weight type
// is idempotent, we use a LogWeight(Tpl) of the appropriate size and assume
// that there are valid weight converters between the template weight type and
// the said LogWeight.
template <class Weight>
class Summation {
 public:
  // HelperWeight is Weight for non-idempotent weights, as is the converter;
  // it is a LogWeightTpl of appropriate precision otherwise.
  using HelperWeight = typename std::conditional<
      (Weight::Properties() & kIdempotent) == kIdempotent,
      LogWeightTpl<typename Weight::ValueType>, Weight>::type;

  Summation() : sum_(HelperWeight::Zero()) {}

  Summation &operator=(const Summation &other) {
    sum_ = other.sum_;
    return *this;
  }

  explicit Summation(const Weight &weight) {
    Set(weight);
  }

  // Sets the summation's value using a weight type.
  void Set(const Weight &weight) {
    sum_ = Summation<Weight>::To(weight);
  }

  // Resets the summation to Zero.
  void Reset() {
    sum_ = HelperWeight::Zero();
  }

  // Adds a weight to the sum.
  void Add(const Weight &weight) {
    sum_ = Plus(sum_, Summation<Weight>::To(weight));
  }

  // Gets the sum in the template-argument weight type.
  Weight Get() const {
    return Summation<Weight>::From(sum_);
  }

 private:
  static HelperWeight To(const Weight &weight) {
    static const WeightConvert<Weight, HelperWeight> to;
    return to(weight);
  }

  static Weight From(const HelperWeight &weight) {
    static const WeightConvert<HelperWeight, Weight> from;
    return from(weight);
  }

  HelperWeight sum_;
};

}  // namespace internal
}  // namespace fst

#endif  // BAUMWELCH_SUMMATION_H_

