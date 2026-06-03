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

#include "opengrm/thrax/walker/util/function/compose.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class ComposeTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void SetUp() override {
    auto left = std::make_unique<MutableTransducer>();
    auto p = left->AddState();
    auto q = left->AddState();
    left->SetStart(p);
    left->EmplaceArc(p, 'a', 'b', q);
    left->SetFinal(q);
    auto right = std::make_unique<MutableTransducer>();
    p = right->AddState();
    q = right->AddState();
    right->SetStart(p);
    right->EmplaceArc(p, 'b', 'c', q);
    right->SetFinal(q);
    golden_ = std::make_unique<MutableTransducer>();
    ::fst::Compose(*left, *right, golden_.get());
    args_ = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
    (*args_)[0] = std::make_unique<DataType>(std::move(left));
    (*args_)[1] = std::make_unique<DataType>(std::move(right));
  }

  virtual void RunTest() {
    auto result_data = func_.Run(std::move(args_));
    auto* result = *result_data->template get<Transducer*>();
    EXPECT_TRUE(::fst::Equal(*result, *golden_));
  }

  Compose<Arc> func_;

  std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args_;

  std::unique_ptr<MutableTransducer> golden_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(ComposeTest, ArcTypes);

TYPED_TEST(ComposeTest, TestCompose) { this->RunTest(); }

TYPED_TEST(ComposeTest, TestComposeWithLeftSort) {
  this->args_->push_back(std::make_unique<DataType>("left"));
  this->RunTest();
}

TYPED_TEST(ComposeTest, TestComposeWithRightSort) {
  this->args_->push_back(std::make_unique<DataType>("right"));
  this->RunTest();
}

TYPED_TEST(ComposeTest, TestComposeWithBothSort) {
  this->args_->push_back(std::make_unique<DataType>("both"));
  this->RunTest();
}

}  // namespace function
}  // namespace thrax
