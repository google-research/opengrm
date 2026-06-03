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

#include "opengrm/thrax/walker/util/function/closure.h"

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/closure.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/ast/fst-node.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class ClosureTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void SetUp() override {
    auto fst = std::make_unique<MutableTransducer>();
    const auto p = fst->AddState();
    const auto q = fst->AddState();
    fst->SetStart(p);
    fst->EmplaceArc(p, 'a', 'a', q);
    fst->SetFinal(q);
    fst_ = *fst;
    args_ = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    args_->push_back(std::make_unique<DataType>(std::move(fst)));
  }

  Closure<Arc> func_;  // The function runner.

  // Each test should delete the following pointers (and contained
  // sub-pointers) as well.
  MutableTransducer fst_;  // The input FST.
  std::unique_ptr<std::vector<std::unique_ptr<DataType>>>
      args_;  // The argument list to the Closure function.
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(ClosureTest, ArcTypes);

TYPED_TEST(ClosureTest, TestStar) {
  auto golden = this->fst_;
  ::fst::Closure(&golden, ::fst::CLOSURE_STAR);
  this->args_->push_back(
      std::make_unique<DataType>(static_cast<int>(RepetitionFstNode::STAR)));
  auto result_data = this->func_.Run(std::move(this->args_));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, golden));
}

TYPED_TEST(ClosureTest, TestPlus) {
  auto golden = this->fst_;
  ::fst::Closure(&golden, ::fst::CLOSURE_PLUS);
  this->args_->push_back(
      std::make_unique<DataType>(static_cast<int>(RepetitionFstNode::PLUS)));
  auto result_data = this->func_.Run(std::move(this->args_));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, golden));
}

TYPED_TEST(ClosureTest, TestRange) {
  typename TestFixture::MutableTransducer golden;
  golden.AddStates(5);
  golden.SetStart(3);
  golden.EmplaceArc(3, 'a', 'a', 4);
  golden.EmplaceArc(0, 'a', 'a', 1);
  golden.EmplaceArc(0, 0, 0, 2);
  golden.SetFinal(1);
  golden.SetFinal(2);
  golden.EmplaceArc(4, 0, 0, 0);
  this->args_->push_back(
      std::make_unique<DataType>(static_cast<int>(RepetitionFstNode::RANGE)));
  this->args_->push_back(std::make_unique<DataType>(1));
  this->args_->push_back(std::make_unique<DataType>(2));
  auto result_data = this->func_.Run(std::move(this->args_));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, golden));
}

}  // namespace function
}  // namespace thrax
