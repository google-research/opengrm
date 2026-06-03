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

#include "opengrm/thrax/walker/util/function/rmepsilon.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class RmEpsilonTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  RmEpsilon<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(RmEpsilonTest, ArcTypes);

TYPED_TEST(RmEpsilonTest, TestRmEpsilon) {
  auto fst = std::make_unique<typename TestFixture::MutableTransducer>();
  const auto p = fst->AddState();
  const auto q = fst->AddState();
  const auto r = fst->AddState();
  fst->SetStart(p);
  fst->EmplaceArc(p, 'a', 'a', q);
  fst->EmplaceArc(q, 0, 0, r);
  fst->SetFinal(r);

  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  auto golden = *fst;
  (*args)[0] = std::make_unique<DataType>(std::move(fst));
  ::fst::RmEpsilon(&golden);
  auto result_data = this->func_.Run(std::move(args));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*result, golden));
}

}  // namespace function
}  // namespace thrax
