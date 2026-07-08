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

// Unit tests for rewrite parentheses utilities.

#include "opengrm/rewrite/parentheses.h"

#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/vector-fst.h"

namespace rewrite {
namespace {

using ::fst::StdArc;
using ::fst::VectorFst;
using ::testing::ElementsAre;
using ::testing::Pair;

TEST(ParenthesesTest, MakeParenthesesVectorValid) {
  VectorFst<StdArc> fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  fst.AddArc(s0, StdArc(1, 2, StdArc::Weight::One(), s1));
  fst.AddArc(s1, StdArc(3, 4, StdArc::Weight::One(), s2));
  fst.SetFinal(s2, StdArc::Weight::One());

  std::vector<std::pair<StdArc::Label, StdArc::Label>> parens;
  MakeParenthesesVector(fst, &parens);

  EXPECT_THAT(parens, ElementsAre(Pair(1, 2), Pair(3, 4)));
}

TEST(ParenthesesTest, MakeParenthesesVectorIgnoresEpsilons) {
  VectorFst<StdArc> fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  const auto s3 = fst.AddState();
  fst.AddArc(s0, StdArc(1, 2, StdArc::Weight::One(), s1));
  fst.AddArc(s1, StdArc(3, 4, StdArc::Weight::One(), s2));
  fst.AddArc(s2, StdArc(0, 0, StdArc::Weight::One(), s3));
  fst.SetFinal(s3, StdArc::Weight::One());

  std::vector<std::pair<StdArc::Label, StdArc::Label>> parens;
  MakeParenthesesVector(fst, &parens);

  EXPECT_THAT(parens, ElementsAre(Pair(1, 2), Pair(3, 4)));
}

TEST(ParenthesesTest, MakeAssignmentsVectorValid) {
  const std::vector<std::pair<StdArc::Label, StdArc::Label>> parens = {{1, 2},
                                                                       {3, 4}};

  VectorFst<StdArc> fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  fst.AddArc(s0, StdArc(1, 10, StdArc::Weight::One(), s1));
  fst.AddArc(s1, StdArc(3, 20, StdArc::Weight::One(), s2));
  fst.SetFinal(s2, StdArc::Weight::One());

  std::vector<StdArc::Label> assignments;
  MakeAssignmentsVector(fst, parens, &assignments);

  EXPECT_THAT(assignments, ElementsAre(10, 20));
}

}  // namespace
}  // namespace rewrite
