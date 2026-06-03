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

#include "opengrm/thrax/walker/util/function/lenientlycompose.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/operators/lenientlycompose.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class LenientlyComposeTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void SetUp() override {
    auto sigstar = std::make_unique<MutableTransducer>();
    auto p = sigstar->AddState();
    sigstar->SetStart(p);
    sigstar->SetFinal(p);
    sigstar->EmplaceArc(p, 'a', 'a', p);
    sigstar->EmplaceArc(p, 'b', 'b', p);
    sigstar->EmplaceArc(p, 'c', 'c', p);

    auto left = std::make_unique<MutableTransducer>();
    p = left->AddState();
    int q = left->AddState();
    left->SetStart(p);
    left->EmplaceArc(p, Arc('a', 'b', q));
    left->SetFinal(q);

    auto right = std::make_unique<MutableTransducer>();
    p = right->AddState();
    q = right->AddState();
    right->SetStart(p);
    right->EmplaceArc(p, 'b', 'c', q);
    right->SetFinal(q);

    golden_ = std::make_unique<MutableTransducer>();
    ::fst::LenientlyCompose(*left, *right, *sigstar, golden_.get());

    args_ = std::make_unique<std::vector<std::unique_ptr<DataType>>>(3);
    (*args_)[0] = std::make_unique<DataType>(std::move(left));
    (*args_)[1] = std::make_unique<DataType>(std::move(right));
    (*args_)[2] = std::make_unique<DataType>(std::move(sigstar));
  }

  virtual void RunTest() {
    auto result_data = func_.Run(std::move(args_));
    auto* result = *result_data->template get<Transducer*>();
    // As with ComposeTest, this just verifies that the result of running the
    // function is the same as what is produced by calling LenientlyCompose
    // directly on the arguments.
    EXPECT_TRUE(::fst::Equal(*result, *golden_));
  }

  LenientlyCompose<Arc> func_;

  std::unique_ptr<MutableTransducer> golden_;
  std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(LenientlyComposeTest, ArcTypes);

TYPED_TEST(LenientlyComposeTest, TestLenientlyCompose) { this->RunTest(); }

}  // namespace function
}  // namespace thrax
