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

#include "opengrm/string/string-view-fst.h"

#include <string>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/string.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

template <typename T>
class StringViewFstTest : public ::testing::Test {};

using MyTypes = ::testing::Types<ByteViewer<StdArc>, Utf8Viewer<StdArc>>;

TYPED_TEST_SUITE(StringViewFstTest, MyTypes);

TYPED_TEST(StringViewFstTest, SanityAndEquivalenceTest) {
  std::string str = "Unicode 😎 déál with it¡";
  StringCompiler<StdArc> compiler(TypeParam::TokenType());
  StdVectorFst vfst;
  compiler(str, &vfst);
  StringViewFst<StdArc, TypeParam> svfst(str);

  // Tests FST sanity.
  EXPECT_TRUE(Verify(svfst));

  // Tests via copy-into-vector and connect.
  StdVectorFst copy(svfst);
  Connect(&copy);
  EXPECT_TRUE(Equal(vfst, copy));

  // Tests equivalence via roundtrip compile-and-print.
  StringPrinter<StdArc> printer(TypeParam::TokenType());
  std::string reconstruct;
  printer(svfst, &reconstruct);
  EXPECT_EQ(reconstruct, str);
}

TYPED_TEST(StringViewFstTest, ArcIteratorTest) {
  std::string str = "Unicode 😎 déál with it¡";
  using SVFst = StringViewFst<StdArc, TypeParam>;
  SVFst svfst(str);
  const auto expected_num_states = svfst.NumStates();

  typename SVFst::StateId counted_num_states = 0;
  for (StateIterator<SVFst> siter(svfst); !siter.Done(); siter.Next()) {
    ++counted_num_states;
    // The number of arcs in the nth state of this string FST is expected to be
    // 0 if at the end, and 1 otherwise.
    const typename SVFst::StateId expected_num_arcs =
        counted_num_states == expected_num_states ? 0 : 1;

    // Create and use post-construction ArcIterator.
    ArcIterator<SVFst> aiter(svfst, siter.Value());
    {
      typename SVFst::StateId counted_num_arcs = 0;
      for (; !aiter.Done(); aiter.Next()) {
        EXPECT_EQ(aiter.Position(), 0);
        ++counted_num_arcs;
      }
      EXPECT_EQ(aiter.Position(), 1);
      EXPECT_EQ(counted_num_arcs, expected_num_arcs);
    }

    // Resets ArcIterator (equivalent to `Seek`ing to position 0). Should
    // operate the same as before.
    {
      typename SVFst::StateId counted_num_arcs = 0;
      for (aiter.Reset(); !aiter.Done(); aiter.Next()) {
        EXPECT_EQ(aiter.Position(), 0);
        ++counted_num_arcs;
      }
      EXPECT_EQ(aiter.Position(), 1);
      EXPECT_EQ(counted_num_arcs, expected_num_arcs);
    }

    // Seeks ArcIterator to position 1. This is past the end, and now should be
    // in the "done" state.
    aiter.Seek(1);
    EXPECT_TRUE(aiter.Done());
    EXPECT_EQ(aiter.Position(), 1);

    // Seeks ArcIterator to position 1000. This is past the end, and now should
    // be in the "done" state.
    aiter.Seek(1000);
    EXPECT_TRUE(aiter.Done());
    EXPECT_EQ(aiter.Position(), 1);
  }
  EXPECT_EQ(counted_num_states, expected_num_states);
}

}  // namespace
}  // namespace fst
