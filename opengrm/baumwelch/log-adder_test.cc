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

#include "opengrm/baumwelch/log-adder.h"

#include <cmath>

#include "gtest/gtest.h"
#include "openfst/lib/float-weight.h"

namespace fst {
namespace internal {
namespace {

using MyTypes = ::testing::Types<TropicalWeight, LogWeight, Log64Weight>;

template <typename T>
class LogAdderTest : public ::testing::Test {};

TYPED_TEST_SUITE(LogAdderTest, MyTypes);

TYPED_TEST(LogAdderTest, ZeroPlusOnePlusOneEqualsTwo) {
  const auto& zero = TypeParam::Zero();
  const auto& one = TypeParam::One();
  const auto two = TypeParam(-log(2.));
  LogAdder<TypeParam> sum;
  EXPECT_EQ(zero, sum.Sum());
  sum.Add(zero);
  EXPECT_EQ(zero, sum.Sum());
  sum.Add(one);
  EXPECT_EQ(one, sum.Sum());
  sum.Add(one);
  EXPECT_EQ(two, sum.Sum());
}

}  // namespace
}  // namespace internal
}  // namespace fst
