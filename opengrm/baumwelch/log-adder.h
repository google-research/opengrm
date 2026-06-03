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

#ifndef OPENGRM_BAUMWELCH_LOG_ADDER_H_
#define OPENGRM_BAUMWELCH_LOG_ADDER_H_

#include <type_traits>

#include "openfst/lib/float-weight.h"
#include "openfst/lib/weight.h"

namespace fst {

// This wraps the Adder class, using LogWeight(Tpl) of the appropriate size
// internally and hiding conversion. It assumes the existence of a valid weight
// converters between the template weight type and the said LogWeight.
template <class Weight>
class LogAdder {
 public:
  // HelperWeight is Weight for non-idempotent weights, as is the converter;
  // it is a LogWeightTpl of appropriate precision otherwise.
  using HelperWeight =
      typename std::conditional_t<IsIdempotent<Weight>::value,
                                  LogWeightTpl<typename Weight::ValueType>,
                                  Weight>;

  explicit LogAdder(Weight weight = Weight::Zero())
      : sum_(LogAdder<Weight>::To(weight)) {}

  // Adds a weight to the sum.
  void Add(const Weight& weight) { sum_.Add(LogAdder<Weight>::To(weight)); }

  // Gets the sum in the template-argument weight type.
  Weight Sum() const { return LogAdder<Weight>::From(sum_.Sum()); }

 private:
  static HelperWeight To(const Weight& weight) {
    static const WeightConvert<Weight, HelperWeight> to;
    return to(weight);
  }

  static Weight From(const HelperWeight& weight) {
    static const WeightConvert<HelperWeight, Weight> from;
    return from(weight);
  }

  Adder<HelperWeight> sum_;
};

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_LOG_ADDER_H_
