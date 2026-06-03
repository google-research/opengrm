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

#include "opengrm/operators/concatrange.h"

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/closure.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/fst-class.h"
#include "opengrm/operators/concatrangescript.h"
#include "opengrm/rewrite/rewrite.h"

namespace fst {
namespace {

using ::rewrite::LatticeToStrings;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::IsTrue;
using ::testing::Ne;
using ::testing::UnorderedElementsAre;

using Arc = StdArc;

class ConcatRangeTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string filename =
        fst::JoinPath(std::string("."),
                       "opengrm/operators/testdata/m.fst");

    m_.reset(VectorFst<Arc>::Read(filename));
  }

  // Helper that counts final states.
  int FinalStates(const Fst<Arc>& fst) const {
    int i = 0;
    for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
      i += fst.Final(siter.Value()) != Arc::Weight::Zero();
    }
    return i;
  }

  // Helper that counts non-epsilon arcs, assuming an acceptor.
  int NonEpsilonArcs(const Fst<Arc>& fst) const {
    int i = 0;
    for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
      const auto state = siter.Value();
      i += fst.NumArcs(state) - fst.NumInputEpsilons(state);
    }
    return i;
  }

  std::vector<std::string> AcceptedStrings(const Fst<Arc>& fst) const {
    std::vector<std::string> strings;
    CHECK(LatticeToStrings(fst, &strings));
    return strings;
  }

  // Single character string FST.
  std::unique_ptr<VectorFst<Arc>> m_;
};

TEST_F(ConcatRangeTest, FiniteUpperBoundTest) {
  // Finite upper bound:
  //   # of final states: 1 + upper - lower
  //   # of non-epsilon arcs: upper
  {
    VectorFst<Arc> m(*m_);
    ConcatRange(&m, 0, 1);
    ASSERT_TRUE(Verify(m)) << "0 to 1";
    EXPECT_NE(kCyclic, m.Properties(kCyclic, true));
    EXPECT_EQ(2, FinalStates(m)) << "?";
    EXPECT_EQ(1, NonEpsilonArcs(m)) << "?";
  }
  {
    VectorFst<Arc> m(*m_);
    ConcatRange(&m, 0, 4);
    ASSERT_TRUE(Verify(m)) << "0 to 4";
    EXPECT_NE(kCyclic, m.Properties(kCyclic, true)) << "0 to 4";
    EXPECT_EQ(5, FinalStates(m)) << "0 to 4";
    EXPECT_EQ(4, NonEpsilonArcs(m)) << "0 to 4";
  }
  {
    VectorFst<Arc> m(*m_);
    ConcatRange(&m, 2, 4);
    ASSERT_TRUE(Verify(m)) << "2 to 4";
    EXPECT_NE(kCyclic, m.Properties(kCyclic, true)) << "2 to 4";
    EXPECT_EQ(3, FinalStates(m)) << "2 to 4";
    EXPECT_EQ(4, NonEpsilonArcs(m)) << "2 to 4";
  }
}

TEST_F(ConcatRangeTest, InfiniteUpperBoundTest) {
  // Infinite upper bound:
  //   # of final states: 2
  //   # of non-epsilon arcs: 1 + lower
  {
    VectorFst<Arc> m(*m_);
    ConcatRange(&m);
    ASSERT_TRUE(Verify(m)) << "0 or more";
    EXPECT_EQ(kCyclic, m.Properties(kCyclic, true)) << "0 or more";
    EXPECT_EQ(2, FinalStates(m)) << "0 or more";
    EXPECT_EQ(1, NonEpsilonArcs(m)) << "0 or more";
  }
  {
    VectorFst<Arc> m(*m_);
    ConcatRange(&m, 1);
    ASSERT_TRUE(Verify(m)) << "1 or more";
    EXPECT_EQ(kCyclic, m.Properties(kCyclic, true)) << "1 or more";
    EXPECT_EQ(1, FinalStates(m)) << "1 or more";
    EXPECT_EQ(1, NonEpsilonArcs(m)) << "1 or more";
  }
  {
    VectorFst<Arc> m(*m_);
    ConcatRange(&m, 2);
    ASSERT_TRUE(Verify(m)) << "2 or more";
    EXPECT_EQ(kCyclic, m.Properties(kCyclic, true)) << "2 or more";
    EXPECT_EQ(1, FinalStates(m)) << "2 or more";
    EXPECT_EQ(2, NonEpsilonArcs(m)) << "2 or more";
  }
}

TEST_F(ConcatRangeTest, ExactTest) {
  VectorFst<Arc> m(*m_);
  ConcatRange(&m, 4, 4);
  ASSERT_TRUE(Verify(m)) << "exactly 4";
  EXPECT_NE(kCyclic, m.Properties(kCyclic, true)) << "exactly 4";
  EXPECT_EQ(1, FinalStates(m)) << "exactly 4";
  EXPECT_EQ(4, NonEpsilonArcs(m)) << "exactly 4";
}

TEST_F(ConcatRangeTest, ZeroLowerBoundEmptyFstTest) {
  // 0 to 1 of the empty machine --> should accept only the empty string.
  {
    VectorFst<Arc> empty;
    ASSERT_THAT(empty.Start(), Eq(kNoStateId));
    ConcatRange(&empty, 0, 1);
    ASSERT_THAT(Verify(empty), IsTrue()) << "0 to 1";
    EXPECT_THAT(empty.Properties(kCyclic, true), Ne(kCyclic));
    EXPECT_THAT(empty.Start(), Ne(kNoStateId));
    EXPECT_THAT(AcceptedStrings(empty), UnorderedElementsAre(""));
  }
  // 0 to 4 of the empty machine --> should accept only the empty string.
  {
    VectorFst<Arc> empty;
    ASSERT_THAT(empty.Start(), Eq(kNoStateId));
    ConcatRange(&empty, 0, 4);
    ASSERT_THAT(Verify(empty), IsTrue());
    EXPECT_THAT(empty.Properties(kCyclic, true), Ne(kCyclic));
    EXPECT_THAT(empty.Start(), Ne(kNoStateId));
    EXPECT_THAT(AcceptedStrings(empty), UnorderedElementsAre(""));
  }
  // 0 or more of the empty machine --> should accept only the empty string.
  {
    VectorFst<Arc> empty;
    ASSERT_THAT(empty.Start(), Eq(kNoStateId));
    ConcatRange(&empty, 0, 0);
    ASSERT_THAT(Verify(empty), IsTrue());
    EXPECT_THAT(empty.Properties(kCyclic, true), Ne(kCyclic));
    EXPECT_THAT(empty.Start(), Ne(kNoStateId));
    EXPECT_THAT(AcceptedStrings(empty), UnorderedElementsAre(""));
  }
  // 4 or more of the empty machine --> should accept nothing.
  {
    VectorFst<Arc> empty;
    ASSERT_THAT(empty.Start(), Eq(kNoStateId));
    ConcatRange(&empty, 4, 0);
    ASSERT_THAT(Verify(empty), IsTrue());
    EXPECT_THAT(empty.Properties(kCyclic, true), Ne(kCyclic));
    EXPECT_THAT(empty.Start(), Eq(kNoStateId));
    EXPECT_THAT(AcceptedStrings(empty), IsEmpty());
  }
}

TEST_F(ConcatRangeTest, ZeroToInfiniteMatchesClosureStarTest) {
  VectorFst<Arc> actual(*m_);
  ConcatRange(&actual);
  VectorFst<Arc> expected(*m_);
  Closure(&expected, CLOSURE_STAR);
  EXPECT_TRUE(Equal(actual, expected)) << "0 or more";
}

TEST_F(ConcatRangeTest, OneToInfiniteMatchesClosurePlusTest) {
  VectorFst<Arc> actual(*m_);
  ConcatRange(&actual, 1);
  VectorFst<Arc> expected(*m_);
  Closure(&expected, CLOSURE_PLUS);
  EXPECT_TRUE(Equal(actual, expected)) << "1 or more";
}

TEST_F(ConcatRangeTest, ScriptTest) {
  namespace s = fst::script;
  s::VectorFstClass m(*m_);
  ConcatRange(&m, 2, 4);
  EXPECT_EQ(10, m.NumStates());
}

}  // namespace
}  // namespace fst
