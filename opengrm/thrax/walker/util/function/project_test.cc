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

#include "opengrm/thrax/walker/util/function/project.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/string.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class ProjectTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Project<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(ProjectTest, ArcTypes);

TYPED_TEST(ProjectTest, TestProjectInput) {
  const ::fst::StringCompiler<TypeParam> compiler(::fst::TokenType::BYTE);
  auto golden = std::make_unique<typename TestFixture::MutableTransducer>();
  ASSERT_TRUE(compiler("ab", golden.get()));
  auto input = std::make_unique<typename TestFixture::MutableTransducer>();
  auto p = input->AddState();
  auto q = input->AddState();
  auto r = input->AddState();
  input->SetStart(p);
  input->EmplaceArc(p, 'a', 'c', q);
  input->EmplaceArc(q, 'b', 'd', r);
  input->SetFinal(r);
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  (*args)[0] = std::make_unique<DataType>(std::move(input));
  (*args)[1] = std::make_unique<DataType>("input");
  auto result_data = this->func_.Run(std::move(args));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  auto actual_result =
      std::make_unique<typename TestFixture::MutableTransducer>(*result);
  EXPECT_TRUE(::fst::Equal(*actual_result, *golden));
}

TYPED_TEST(ProjectTest, TestProjectOutput) {
  const ::fst::StringCompiler<TypeParam> compiler(::fst::TokenType::BYTE);
  auto golden = std::make_unique<typename TestFixture::MutableTransducer>();
  ASSERT_TRUE(compiler("cd", golden.get()));
  auto input = std::make_unique<typename TestFixture::MutableTransducer>();
  auto p = input->AddState();
  auto q = input->AddState();
  auto r = input->AddState();
  input->SetStart(p);
  input->EmplaceArc(p, 'a', 'c', q);
  input->EmplaceArc(q, 'b', 'd', r);
  input->SetFinal(r);
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  (*args)[0] = std::make_unique<DataType>(std::move(input));
  (*args)[1] = std::make_unique<DataType>("output");
  auto result_data = this->func_.Run(std::move(args));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  auto actual_result =
      std::make_unique<typename TestFixture::MutableTransducer>(*result);
  EXPECT_TRUE(::fst::Equal(*actual_result, *golden));
}

}  // namespace function
}  // namespace thrax
