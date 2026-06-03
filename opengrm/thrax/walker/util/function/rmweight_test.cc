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

#include "opengrm/thrax/walker/util/function/rmweight.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename A>
class RmWeightTest : public ::testing::Test {
 protected:
  using Arc = A;

  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  RmWeight<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(RmWeightTest, ArcTypes);

TYPED_TEST(RmWeightTest, TestRmWeight) {
  auto in = std::make_unique<typename TestFixture::MutableTransducer>();
  const auto p = in->AddState();
  const auto q = in->AddState();
  in->SetStart(p);
  in->EmplaceArc(p, 'a', 'a', 1, q);
  in->SetFinal(q, typename TestFixture::Arc::Weight(1));
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  auto golden = *in;
  (*args)[0] = std::make_unique<DataType>(std::move(in));
  ::fst::ArcMap(&golden, ::fst::RmWeightMapper<typename TestFixture::Arc>());
  std::unique_ptr<DataType> result_data(this->func_.Run(std::move(args)));
  auto* result = *result_data->get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, golden));
}

}  // namespace function
}  // namespace thrax
