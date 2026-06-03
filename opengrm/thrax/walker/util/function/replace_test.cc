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

#include "opengrm/thrax/walker/util/function/replace.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class ReplaceTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Label = typename Arc::Label;

  void SetUp() override {
    Label eps = 0;
    Label Root = 1;
    Label Name = 2;
    Label FirstName = 3;
    Label LastName = 4;
    Label dial = 5;
    Label please = 6;
    Label johan = 7;
    Label schalkwyk = 8;
    Label google = 9;
    Label michael = 10;
    Label riley = 11;
    // label fst to represent the sequence of labels to be interpreted as the
    // replacement arcs
    labels_ = std::make_unique<MutableTransducer>();
    auto s0 = labels_->AddState();
    auto s1 = labels_->AddState();
    auto s2 = labels_->AddState();
    auto s3 = labels_->AddState();
    auto s4 = labels_->AddState();
    labels_->SetStart(s0);
    labels_->SetFinal(s4);
    labels_->EmplaceArc(s0, Root, Root, s1);
    labels_->EmplaceArc(s1, Name, Name, s2);
    labels_->EmplaceArc(s2, FirstName, FirstName, s3);
    labels_->EmplaceArc(s3, LastName, LastName, s4);
    // a1, root FST
    a1_ = std::make_unique<MutableTransducer>();
    s0 = a1_->AddState();
    s1 = a1_->AddState();
    s2 = a1_->AddState();
    s3 = a1_->AddState();
    a1_->SetStart(s0);
    a1_->SetFinal(s3);
    a1_->EmplaceArc(s0, dial, dial, s1);
    a1_->EmplaceArc(s1, google, google, s2);
    a1_->EmplaceArc(s1, Name, Name, s2);
    a1_->EmplaceArc(s2, please, please, s3);
    // a2, FST for non-terminal $Name
    a2_ = std::make_unique<MutableTransducer>();
    s0 = a2_->AddState();
    s1 = a2_->AddState();
    s2 = a2_->AddState();
    a2_->SetStart(s0);
    a2_->SetFinal(s2);
    a2_->EmplaceArc(s0, michael, michael, s1);
    a2_->EmplaceArc(s0, FirstName, FirstName, s1);
    a2_->EmplaceArc(s1, riley, riley, s2);
    a2_->EmplaceArc(s1, LastName, LastName, s2);
    // a3, FST for non-terminal $FirstName
    a3_ = std::make_unique<MutableTransducer>();
    s0 = a3_->AddState();
    s1 = a3_->AddState();
    a3_->SetStart(s0);
    a3_->SetFinal(s1);
    a3_->EmplaceArc(s0, johan, johan, s1);
    // a4, FST for non-terminal $LastName
    a4_ = std::make_unique<MutableTransducer>();
    s0 = a4_->AddState();
    s1 = a4_->AddState();
    a4_->SetStart(s0);
    a4_->SetFinal(s1);
    a4_->EmplaceArc(s0, schalkwyk, schalkwyk, s1);
    // golden FST to match output
    golden_ = std::make_unique<MutableTransducer>();
    s0 = golden_->AddState();
    s1 = golden_->AddState();
    s2 = golden_->AddState();
    s3 = golden_->AddState();
    s4 = golden_->AddState();
    auto s5 = golden_->AddState();
    auto s6 = golden_->AddState();
    auto s7 = golden_->AddState();
    auto s8 = golden_->AddState();
    auto s9 = golden_->AddState();
    auto s10 = golden_->AddState();
    golden_->SetStart(s0);
    golden_->SetFinal(s4);
    golden_->EmplaceArc(s0, dial, dial, s1);
    golden_->EmplaceArc(s1, google, google, s2);
    golden_->EmplaceArc(s2, please, please, s4);
    golden_->EmplaceArc(s1, eps, eps, s3);
    golden_->EmplaceArc(s3, michael, michael, s5);
    golden_->EmplaceArc(s5, riley, riley, s7);
    golden_->EmplaceArc(s7, eps, eps, s2);
    golden_->EmplaceArc(s3, eps, eps, s6);
    golden_->EmplaceArc(s6, johan, johan, s9);
    golden_->EmplaceArc(s9, eps, eps, s5);
    golden_->EmplaceArc(s5, eps, eps, s8);
    golden_->EmplaceArc(s8, schalkwyk, schalkwyk, s10);
    golden_->EmplaceArc(s10, eps, eps, s7);
    args_ = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
  }

  virtual void RunTest() {
    auto result_data = func_.Run(std::move(args_));
    MutableTransducer result(**result_data->template get<Transducer*>());
    EXPECT_TRUE(::fst::Equal(result, *golden_));
  }

  Replace<Arc> func_;

  // Implements the example from
  // http://www.openfst.org/twiki/bin/view/FST/ReplaceDoc
  std::unique_ptr<MutableTransducer> labels_;
  std::unique_ptr<MutableTransducer> a1_;
  std::unique_ptr<MutableTransducer> a2_;
  std::unique_ptr<MutableTransducer> a3_;
  std::unique_ptr<MutableTransducer> a4_;
  std::unique_ptr<MutableTransducer> golden_;

  std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(ReplaceTest, ArcTypes);

TYPED_TEST(ReplaceTest, TestReplace) {
  this->args_->push_back(std::make_unique<DataType>(std::move(this->labels_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->a1_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->a2_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->a3_)));
  this->args_->push_back(std::make_unique<DataType>(std::move(this->a4_)));
  this->RunTest();
}

}  // namespace function
}  // namespace thrax
