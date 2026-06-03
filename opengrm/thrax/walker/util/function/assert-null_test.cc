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

#include "opengrm/thrax/walker/util/function/assert-null.h"

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
class AssertNullTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  AssertNullTest() = default;

  void SetUp() override {
    const auto start = non_null_fst_.AddState();
    non_null_fst_.SetStart(start);
    non_null_fst_.SetFinal(start);
  }

  MutableTransducer null_fst_;
  MutableTransducer non_null_fst_;
  AssertNull<Arc> func_;
};

TYPED_TEST_SUITE(AssertNullTest, ::testing::Types<::fst::StdArc>);

TYPED_TEST(AssertNullTest, TestNull) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  auto fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->null_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  auto result = this->func_.Run(std::move(args));
  auto* result_fst = *result->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(result_fst);
}

TYPED_TEST(AssertNullTest, TestNotNull) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  auto fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->non_null_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  auto result = this->func_.Run(std::move(args));
  EXPECT_FALSE(result);
}

}  // namespace function
}  // namespace thrax
