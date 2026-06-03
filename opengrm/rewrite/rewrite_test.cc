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

#include "opengrm/rewrite/rewrite.h"

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/string/stringcompile.h"

namespace rewrite {
namespace {

using ::fst::Fst;
using ::fst::kNoOEpsilons;
using ::fst::RmWeightMapper;
using ::fst::VectorFst;
using ::testing::Eq;
using ::testing::UnorderedElementsAre;

using Arc = ::fst::StdArc;

class RewriteTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string rule1_name = fst::JoinPath(
        std::string("."),
        "opengrm/rewrite/testdata/rule1.fst");
    const std::string rule2_name = fst::JoinPath(
        std::string("."),
        "opengrm/rewrite/testdata/rule2.fst");
    const std::string rule3_name = fst::JoinPath(
        std::string("."),
        "opengrm/rewrite/testdata/rule3.fst");

    CHECK(StringCompile("bbabbaa", &input_));
    rule1_.reset(VectorFst<Arc>::Read(rule1_name));
    rule2_.reset(VectorFst<Arc>::Read(rule2_name));
    rule3_.reset(VectorFst<Arc>::Read(rule3_name));
  }

  // Helper to test for epsilons in the lattice.
  bool NoEpsilons(const Fst<Arc>& lattice) {
    return lattice.Properties(kNoOEpsilons, true) == kNoOEpsilons;
  }

  VectorFst<Arc> input_;
  // Functional, acyclic when applied, and unweighted.
  std::unique_ptr<VectorFst<Arc>> rule1_;
  // Non-functional, possibly cyclic when applied, and weighted.
  std::unique_ptr<VectorFst<Arc>> rule2_;
  // Non-functional, acyclic when applied, and unweighted.
  std::unique_ptr<VectorFst<Arc>> rule3_;
};

// a -> 0 / b __.
// Rule naturally has output epsilons; these should be removed in lattice
// construction.
TEST_F(RewriteTest, NoEpsilonsInLattice) {
  VectorFst<Arc> lattice;
  ASSERT_TRUE(RewriteLattice(input_, *rule1_, &lattice));
  EXPECT_TRUE(NoEpsilons(lattice));
}

// a -> 0 / b __.
TEST_F(RewriteTest, TestRewrite) {
  std::vector<std::string> outputs;

  ASSERT_TRUE(Rewrites(input_, *rule1_, &outputs));
  EXPECT_THAT(outputs, UnorderedElementsAre("bbbb"));

  ASSERT_TRUE(TopRewrites(input_, *rule1_, &outputs));
  EXPECT_THAT(outputs, UnorderedElementsAre("bbbb"));

  std::string output;

  ASSERT_TRUE(TopRewrite(input_, *rule1_, &output));
  EXPECT_THAT(output, Eq("bbbb"));
}

// 0 -> a / a __ <1> [RTL].
// Will work only when pruning, giving us an identity mapping.
TEST_F(RewriteTest, TestPrunedWeightedCycles) {
  std::vector<std::string> outputs;

  ASSERT_TRUE(TopRewrites(input_, *rule2_, &outputs));
  EXPECT_THAT(outputs, UnorderedElementsAre("bbabbaa"));

  std::string output;

  EXPECT_TRUE(OneTopRewrite(input_, *rule2_, &output));
  EXPECT_THAT(output, Eq("bbabbaa"));
}

// 0 -> a / a __ [RTL].
// Will fail to produce a acyclic lattice with a pruned DFA, but will do so
// with shortest-path.
TEST_F(RewriteTest, TestUnweightedCycles) {
  std::vector<std::string> outputs;
  VectorFst<Arc> rule2_unweighted(*rule2_);
  ArcMap(*rule2_, &rule2_unweighted, RmWeightMapper<Arc>());
  EXPECT_FALSE(TopRewrites(input_, rule2_unweighted, &outputs));
  EXPECT_TRUE(TopRewrites(input_, rule2_unweighted, 128, &outputs));
}

// a -> 0 / __ [optional].
// The rule is inefficiently encoded but this shouldn't impact the results
// we get because of the internal determinization. Note that deleting just the
// first of two 'a's gives us the same output string as deleting just the
// second, so there are only six possible output strings.
TEST_F(RewriteTest, TestOptionalRewrite) {
  std::vector<std::string> outputs;
  EXPECT_TRUE(TopRewrites(input_, *rule3_, &outputs));
  EXPECT_THAT(outputs, UnorderedElementsAre("bbbb", "bbabb", "bbbba", "bbabba",
                                            "bbbbaa", "bbabbaa"));
}

// a -> 0 / __ [optional].
// The rule has multiple optimal outputs so this will fail, but the output
// string will still be populated.
TEST_F(RewriteTest, OneTopRewriteNonfunctionalRule) {
  std::string output;
  EXPECT_FALSE(OneTopRewrite(input_, *rule3_, &output));
  EXPECT_FALSE(output.empty());
}

TEST_F(RewriteTest, MatchesTest) {
  VectorFst<Arc> output;
  ASSERT_TRUE(StringCompile("bbbb", &output));
  EXPECT_TRUE(Matches(input_, output, *rule3_));
  ASSERT_TRUE(StringCompile("aaaa", &output));
  EXPECT_FALSE(Matches(input_, output, *rule3_));
}

}  // namespace
}  // namespace rewrite
