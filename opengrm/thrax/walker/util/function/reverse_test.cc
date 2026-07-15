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

#include "opengrm/thrax/walker/util/function/reverse.h"

#include <memory>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class ReverseTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Reverse<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(ReverseTest, ArcTypes, );

// Tests that Reverse correctly reverses the paths in the FST.
TYPED_TEST(ReverseTest, TestReverse) {
  auto input = std::make_unique<typename TestFixture::MutableTransducer>();
  auto p = input->AddState();
  auto q = input->AddState();
  auto r = input->AddState();
  input->SetStart(p);
  input->EmplaceArc(p, 'a', 'a', q);
  input->EmplaceArc(q, 'b', 'b', r);
  input->SetFinal(r);

  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  (*args)[0] = std::make_unique<DataType>(std::move(input));
  auto result_data = this->func_.Run(std::move(args));
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();

  // Note that by default fst::Reverse creates a superinitial state with an
  // epsilon transition pointing to the reversed final state of the input
  // transducer. As a result, the reversed output has four states instead of
  // three.
  typename TestFixture::MutableTransducer expected;
  auto start = expected.AddState();
  expected.SetStart(start);
  auto s1 = expected.AddState();
  auto s2 = expected.AddState();
  auto final_state = expected.AddState();

  expected.EmplaceArc(start, 0, 0, s1);
  expected.EmplaceArc(s1, 'b', 'b', s2);
  expected.EmplaceArc(s2, 'a', 'a', final_state);
  expected.SetFinal(final_state);

  // Use Isomorphic instead of Equal because Equal requires state IDs to match
  // exactly, whereas Reverse might assign different state IDs.
  EXPECT_TRUE(::fst::Isomorphic(*result, expected));
}

}  // namespace function
}  // namespace thrax
