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

#include "opengrm/thrax/walker/util/function/pdtcompose.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/extensions/pdt/compose.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class PdtComposeTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Label = typename Arc::Label;

  void SetUp() override {
    input_ = std::make_unique<MutableTransducer>();
    auto p = input_->AddState();
    auto q = input_->AddState();
    input_->SetStart(p);
    input_->EmplaceArc(p, 'a', 'a', p);
    input_->EmplaceArc(p, 'b', 'b', q);
    input_->EmplaceArc(q, 'b', 'b', q);
    input_->SetFinal(p);
    input_->SetFinal(q);

    // Create a PDT with the topology:
    //
    // 0 1 a
    // 1 0 (
    // 0 2 eps
    // 2 3 b
    // 3 2 )
    // 2
    pdt_ = std::make_unique<MutableTransducer>();
    const auto a0 = pdt_->AddState();
    const auto a1 = pdt_->AddState();
    const auto b0 = pdt_->AddState();
    const auto b1 = pdt_->AddState();
    pdt_->SetStart(a0);
    pdt_->EmplaceArc(a0, 'a', 'a', a1);
    pdt_->EmplaceArc(a1, '(', '(', a0);
    pdt_->EmplaceArc(a0, 0, 0, b0);
    pdt_->EmplaceArc(b0, 'b', 'b', b1);
    pdt_->EmplaceArc(b1, ')', ')', b0);
    pdt_->SetFinal(b0);

    std::vector<std::pair<Label, Label>> parens;
    parens.emplace_back('(', ')');

    parens_ = std::make_unique<MutableTransducer>();
    p = parens_->AddState();
    q = parens_->AddState();
    parens_->SetStart(p);
    parens_->EmplaceArc(p, '(', ')', q);
    parens_->SetFinal(q);

    args_ = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    golden_ = std::make_unique<MutableTransducer>();
    ::fst::Compose(*input_, *pdt_, parens, golden_.get());
  }

  virtual void RunTest() {
    auto result_data = func_.Run(std::move(args_));
    MutableTransducer result(**result_data->template get<Transducer*>());
    // With PdtCompose, sorting the PDT can result in different orderings of the
    // arcs involving epsilons. This causes Equal to fail.
    ::fst::RmEpsilon(&result);
    ::fst::RmEpsilon(golden_.get());
    EXPECT_TRUE(::fst::Equal(result, *golden_));
  }

  PdtCompose<Arc> func_;

  std::unique_ptr<MutableTransducer> input_;
  std::unique_ptr<MutableTransducer> pdt_;
  std::unique_ptr<MutableTransducer> parens_;
  std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args_;

  std::unique_ptr<MutableTransducer> golden_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(PdtComposeTest, ArcTypes);

TYPED_TEST(PdtComposeTest, TestPdtCompose) {
  // Default for grammar compiler: PDT on right
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->pdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->RunTest();
}

TYPED_TEST(PdtComposeTest, TestPdtComposeLeft) {
  // PDT on left
  this->args_->push_back(std::make_unique<DataType>(std::move(this->pdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(std::make_unique<DataType>("left_pdt"));
  this->RunTest();
}

TYPED_TEST(PdtComposeTest, TestPdtComposeWithLeftSort) {
  // PDT on right, left sort
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->pdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(std::make_unique<DataType>("right_pdt"));
  this->args_->push_back(std::make_unique<DataType>("left"));
  this->RunTest();
}

TYPED_TEST(PdtComposeTest, TestPdtComposeWithRightSort) {
  // PDT on right, right sort
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->pdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(std::make_unique<DataType>("right_pdt"));
  this->args_->push_back(std::make_unique<DataType>("right"));
  this->RunTest();
}

TYPED_TEST(PdtComposeTest, TestPdtComposeWithBothSort) {
  // PDT on right, both sort
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->pdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(std::make_unique<DataType>("right_pdt"));
  this->args_->push_back(std::make_unique<DataType>("both"));
  this->RunTest();
}

}  // namespace function
}  // namespace thrax
