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

#include "opengrm/thrax/walker/util/function/mpdtcompose.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/extensions/mpdt/compose.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class MPdtComposeTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Label = typename Arc::Label;

  void SetUp() override {
    input_ = std::make_unique<MutableTransducer>();
    auto s = input_->AddState();
    auto a = input_->AddState();
    auto b1 = input_->AddState();
    auto b2 = input_->AddState();
    input_->SetStart(s);
    // abb
    input_->EmplaceArc(s, 'a', 'a', a);
    input_->EmplaceArc(a, 'b', 'b', b1);
    input_->EmplaceArc(b1, 'b', 'b', b2);
    input_->SetFinal(b2);

    // Create an MPDT with the topology:
    //
    // 0 1 a  a
    // 1 0 (  (
    // 0 2 b  b
    // 2 0 <  <
    // 0 3 eps eps
    // 3 4 ) )
    // 4 3 { {
    // 3 5 > >
    // 5 3 [ [
    // 3 6 eps eps
    // 6 7 eps a
    // 7 6 } }
    // 6 8 eps b
    // 8 6 ] ]
    // 6
    //
    // where parens (), <> are in the first stack and {}, [] are in the second
    // stack.
    mpdt_ = std::make_unique<MutableTransducer>();
    int s0 = mpdt_->AddState();
    int a0 = mpdt_->AddState();
    int b0 = mpdt_->AddState();
    int s1 = mpdt_->AddState();
    int a1 = mpdt_->AddState();
    b1 = mpdt_->AddState();
    int s2 = mpdt_->AddState();
    int a2 = mpdt_->AddState();
    b2 = mpdt_->AddState();
    mpdt_->SetStart(s0);
    mpdt_->EmplaceArc(s0, 'a', 'a', a0);
    mpdt_->EmplaceArc(a0, '(', '(', s0);
    mpdt_->EmplaceArc(s0, 'b', 'b', b0);
    mpdt_->EmplaceArc(b0, '<', '<', s0);
    mpdt_->EmplaceArc(s0, 0, 0, s1);
    mpdt_->EmplaceArc(s1, ')', ')', a1);
    mpdt_->EmplaceArc(a1, '{', '{', s1);
    mpdt_->EmplaceArc(s1, '>', '>', b1);
    mpdt_->EmplaceArc(b1, '[', '[', s1);
    mpdt_->EmplaceArc(s1, 0, 0, s2);
    mpdt_->EmplaceArc(s2, 0, 'a', a2);
    mpdt_->EmplaceArc(a2, '}', '}', s2);
    mpdt_->EmplaceArc(s2, 0, 'b', b2);
    mpdt_->EmplaceArc(b2, ']', ']', s2);
    mpdt_->SetFinal(s2);
    std::vector<std::pair<Label, Label>> parens;
    parens.emplace_back('(', ')');
    parens.emplace_back('<', '>');
    parens.emplace_back('{', '}');
    parens.emplace_back('[', ']');
    parens_ = std::make_unique<MutableTransducer>();
    int p = parens_->AddState();
    int q = parens_->AddState();
    parens_->SetStart(p);
    parens_->EmplaceArc(p, '(', ')', q);
    parens_->EmplaceArc(p, '<', '>', q);
    parens_->EmplaceArc(p, '{', '}', q);
    parens_->EmplaceArc(p, '[', ']', q);
    parens_->SetFinal(q);

    std::vector<Label> assignments;
    assignments.push_back(1);
    assignments.push_back(1);
    assignments.push_back(2);
    assignments.push_back(2);

    assignments_ = std::make_unique<MutableTransducer>();
    p = assignments_->AddState();
    q = assignments_->AddState();
    assignments_->SetStart(p);
    assignments_->EmplaceArc(p, '(', 1, q);
    assignments_->EmplaceArc(p, '<', 1, q);
    assignments_->EmplaceArc(p, '{', 2, q);
    assignments_->EmplaceArc(p, '[', 2, q);
    assignments_->SetFinal(q);

    args_ = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    golden_ = std::make_unique<MutableTransducer>();
    ::fst::Compose(*input_, *mpdt_, parens, assignments, golden_.get());
  }

  virtual void RunTest() {
    auto result_data = func_.Run(std::move(args_));
    MutableTransducer result(**result_data->template get<Transducer*>());
    // With MPdtCompose, sorting the MPDT can result in different orderings of
    // the arcs involving epsilons. This causes Equal to fail.
    ::fst::RmEpsilon(&result);
    ::fst::RmEpsilon(golden_.get());
    EXPECT_TRUE(::fst::Equal(result, *golden_));
  }

  MPdtCompose<Arc> func_;

  std::unique_ptr<MutableTransducer> input_;
  std::unique_ptr<MutableTransducer> mpdt_;
  std::unique_ptr<MutableTransducer> parens_;
  std::unique_ptr<MutableTransducer> assignments_;
  std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args_;

  std::unique_ptr<MutableTransducer> golden_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(MPdtComposeTest, ArcTypes);

TYPED_TEST(MPdtComposeTest, TestMPdtCompose) {
  // Default for grammar compiler, MPDT on right.
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->mpdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(
      std::make_unique<DataType>(std::move(this->assignments_)));
  this->RunTest();
}

// Skipping left test since the above is configured as a right-compose
// transducer ... and this is the same in any case as with pdtcompose

TYPED_TEST(MPdtComposeTest, TestMPdtComposeWithLeftSort) {
  // MPDT on right, left sort.
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->mpdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(
      std::make_unique<DataType>(std::move(this->assignments_)));
  this->args_->push_back(std::make_unique<DataType>("right_mpdt"));
  this->args_->push_back(std::make_unique<DataType>("left"));
  this->RunTest();
}

TYPED_TEST(MPdtComposeTest, TestMPdtComposeWithRightSort) {
  // MPDT on right, right sort.
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->mpdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(
      std::make_unique<DataType>(std::move(this->assignments_)));
  this->args_->push_back(std::make_unique<DataType>("right_mpdt"));
  this->args_->push_back(std::make_unique<DataType>("right"));
  this->RunTest();
}

TYPED_TEST(MPdtComposeTest, TestPdtComposeWithBothSort) {
  // MPDT on right, both sort.
  this->args_->push_back(std::make_unique<DataType>(std::move(this->input_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->mpdt_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->parens_)));
  this->args_->push_back(
      std::make_unique<DataType>(std::move(this->assignments_)));
  this->args_->push_back(std::make_unique<DataType>("right_mpdt"));
  this->args_->push_back(std::make_unique<DataType>("both"));
  this->RunTest();
}

}  // namespace function
}  // namespace thrax
