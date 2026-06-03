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

#include "opengrm/thrax/walker/util/function/concat.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/concat.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class ConcatTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Concat<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(ConcatTest, ArcTypes);

TYPED_TEST(ConcatTest, TestConcat) {
  auto left = std::make_unique<typename TestFixture::MutableTransducer>();
  auto p = left->AddState();
  auto q = left->AddState();
  left->SetStart(p);
  left->EmplaceArc(p, 'a', 'a', q);
  left->SetFinal(q);

  auto right = std::make_unique<typename TestFixture::MutableTransducer>();
  p = right->AddState();
  q = right->AddState();
  right->SetStart(p);
  right->EmplaceArc(p, 'b', 'b', q);
  right->SetFinal(q);

  auto golden = *right;
  ::fst::Concat(*left, &golden);
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  (*args)[0] = std::make_unique<DataType>(std::move(left));
  (*args)[1] = std::make_unique<DataType>(std::move(right));
  auto result_data = this->func_.Run(std::move(args));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, golden));
}

}  // namespace function
}  // namespace thrax
