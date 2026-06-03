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

#include "opengrm/thrax/walker/util/function/assert-equal.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class AssertEqualTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  AssertEqualTest() = default;

  void SetUp() override {
    // Comparison FST.
    int start = fst_.AddState();
    fst_.SetStart(start);
    int a = fst_.AddState();
    fst_.EmplaceArc(start, 'a', 'a', a);
    int ab = fst_.AddState();
    fst_.EmplaceArc(a, 'c', 'b', ab);
    fst_.SetFinal(ab);
    // FST that should be equal.
    start = golden_fst_.AddState();
    golden_fst_.SetStart(start);
    a = golden_fst_.AddState();
    golden_fst_.EmplaceArc(start, 'a', 'a', a);
    ab = golden_fst_.AddState();
    golden_fst_.EmplaceArc(a, 'c', 'b', ab);
    golden_fst_.SetFinal(ab);
    // FST that should not be equal.
    start = leaden_fst_.AddState();
    leaden_fst_.SetStart(start);
    a = leaden_fst_.AddState();
    leaden_fst_.EmplaceArc(start, 'a', 'a', a);
    int ac = leaden_fst_.AddState();
    leaden_fst_.EmplaceArc(a, 'c', 'c', ab);
    leaden_fst_.SetFinal(ac);
  }

  MutableTransducer golden_fst_;
  MutableTransducer leaden_fst_;
  MutableTransducer empty_leaden_fst_;  // By design, 0 states.
  MutableTransducer fst_;
  AssertEqual<Arc> func_;
};

TYPED_TEST_SUITE(AssertEqualTest, ::testing::Types<::fst::StdArc>);

TYPED_TEST(AssertEqualTest, TestEqual) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  auto fst =
      std::make_unique<typename TestFixture::MutableTransducer>(this->fst_);
  auto golden_fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->golden_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  (*args)[1] = std::make_unique<DataType>(std::move(golden_fst));
  auto result = this->func_.Run(std::move(args));
  auto* result_fst = *result->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(result_fst);
}

TYPED_TEST(AssertEqualTest, TestNotEqual) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  auto fst =
      std::make_unique<typename TestFixture::MutableTransducer>(this->fst_);
  auto leaden_fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->leaden_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  (*args)[1] = std::make_unique<DataType>(std::move(leaden_fst));
  auto result = this->func_.Run(std::move(args));
  EXPECT_FALSE(result);
}

TYPED_TEST(AssertEqualTest, TestNotEqualEmpty) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  auto fst =
      std::make_unique<typename TestFixture::MutableTransducer>(this->fst_);
  auto empty_leaden_fst =
      std::make_unique<typename TestFixture::MutableTransducer>(
          this->empty_leaden_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  (*args)[1] = std::make_unique<DataType>(std::move(empty_leaden_fst));
  auto result = this->func_.Run(std::move(args));
  EXPECT_FALSE(result);
}

TYPED_TEST(AssertEqualTest, TestNotEqualSymbolTable) {
  // Check that this works with a symbol table. For the sake of simplicity we
  // make up one with the labels with values 'a', 'b', 'c' mapped to something
  // else.
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(3);
  auto fst =
      std::make_unique<typename TestFixture::MutableTransducer>(this->fst_);
  auto leaden_fst = std::make_unique<typename TestFixture::MutableTransducer>(
      this->leaden_fst_);
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  (*args)[1] = std::make_unique<DataType>(std::move(leaden_fst));
  ::fst::SymbolTable syms;
  syms.AddSymbol("aardvark", 'a');
  syms.AddSymbol("bullfrog", 'b');
  syms.AddSymbol("cheetah", 'c');
  (*args)[2] = std::make_unique<DataType>(syms);
  auto result = this->func_.Run(std::move(args));
  EXPECT_FALSE(result);
}

}  // namespace function
}  // namespace thrax
