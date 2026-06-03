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

#include "opengrm/thrax/walker/util/function/assert-empty.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class AssertEmptyTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  AssertEmptyTest() = default;

  void SetUp() override {
    auto start = empty_fst_.AddState();
    empty_fst_.SetStart(start);
    empty_fst_.SetFinal(start);
    start = non_empty_fst_.AddState();
    non_empty_fst_.SetStart(start);
    const auto a = non_empty_fst_.AddState();
    non_empty_fst_.EmplaceArc(start, 'a', 'a', a);
    const auto ab = non_empty_fst_.AddState();
    non_empty_fst_.EmplaceArc(a, 'c', 'b', ab);
    non_empty_fst_.SetFinal(ab);
  }

  MutableTransducer empty_fst_;
  MutableTransducer non_empty_fst_;
  AssertEmpty<Arc> func_;
};

TYPED_TEST_SUITE(AssertEmptyTest, ::testing::Types<::fst::StdArc>);

TYPED_TEST(AssertEmptyTest, TestEmpty) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  auto fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->empty_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  auto result = this->func_.Run(std::move(args));
  auto* result_fst = *result->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(result_fst);
}

TYPED_TEST(AssertEmptyTest, TestNotEmpty) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  auto fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->non_empty_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  auto result = this->func_.Run(std::move(args));
  EXPECT_FALSE(result);
}

}  // namespace function
}  // namespace thrax
