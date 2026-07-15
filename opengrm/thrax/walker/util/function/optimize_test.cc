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

#include "opengrm/thrax/walker/util/function/optimize.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/operators/optimize.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class OptimizeTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Optimize<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(OptimizeTest, ArcTypes, );

// Tests that Optimize removes epsilons and minimizes the FST.
TYPED_TEST(OptimizeTest, TestOptimize) {
  auto input = std::make_unique<typename TestFixture::MutableTransducer>();
  auto p = input->AddState();
  input->SetStart(p);
  auto q = input->AddState();
  auto r1 = input->AddState();
  auto r2 = input->AddState();

  input->EmplaceArc(p, 0, 0, q);
  input->EmplaceArc(q, 'a', 'a', r1);
  input->EmplaceArc(q, 'b', 'b', r2);
  input->SetFinal(r1);
  input->SetFinal(r2);

  ASSERT_EQ(input->Properties(::fst::kAcyclic | ::fst::kAcceptor,
                              /*test=*/true),
            ::fst::kAcyclic | ::fst::kAcceptor);

  auto expected = *input;
  ::fst::Optimize(&expected);

  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  (*args)[0] = std::make_unique<DataType>(std::move(input));
  auto result_data = this->func_.Run(std::move(args));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, expected));
}

}  // namespace function
}  // namespace thrax
