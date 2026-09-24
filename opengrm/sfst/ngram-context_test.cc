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

#include "opengrm/sfst/ngram-context.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {
namespace {

using Label = fst::StdArc::Label;
using ::testing::ElementsAre;

TEST(NGramContextTest, NullContext) {
  NGramContext context;
  EXPECT_TRUE(context.NullContext());
  EXPECT_TRUE(context.HasContext({1, 2, 3}));
  EXPECT_TRUE(context.HasContext({}));
}

TEST(NGramContextTest, ParseContextIntervalAndGetContextString) {
  std::vector<Label> begin;
  std::vector<Label> end;
  NGramContext::ParseContextInterval("1 2 3 : 1 2 5", &begin, &end);
  EXPECT_THAT(begin, ElementsAre(1, 2, 3));
  EXPECT_THAT(end, ElementsAre(1, 2, 5));

  EXPECT_EQ(NGramContext::GetContextString(begin, end), "1 2 3 : 1 2 5");
}

TEST(NGramContextTest, HasContextMatching) {
  // Interval [1 1 1 1, 1 1 1 5) with hi_order = 5
  NGramContext context("1 1 1 1 : 1 1 1 5", /*hi_order=*/5);
  EXPECT_FALSE(context.NullContext());
  EXPECT_EQ(context.GetHiOrder(), 5);
  EXPECT_THAT(context.GetContextBegin(), ElementsAre(1, 1, 1, 1));
  EXPECT_THAT(context.GetContextEnd(), ElementsAre(1, 1, 1, 5));

  // In interval:
  EXPECT_TRUE(context.HasContext({1, 1, 1, 2}, /*include_all_suffixes=*/false));
  EXPECT_TRUE(context.HasContext({1, 1, 1, 1}, /*include_all_suffixes=*/false));
  EXPECT_TRUE(context.HasContext({1, 1, 1, 4}, /*include_all_suffixes=*/false));

  // Upper bound (half-open interval):
  EXPECT_FALSE(
      context.HasContext({1, 1, 1, 5}, /*include_all_suffixes=*/false));

  // Outside interval:
  EXPECT_FALSE(
      context.HasContext({1, 1, 1, 6}, /*include_all_suffixes=*/false));
  EXPECT_FALSE(
      context.HasContext({1, 1, 1, 0}, /*include_all_suffixes=*/false));
}

TEST(NGramContextTest, SuffixInclusion) {
  NGramContext context("1 1 : 1 5", /*hi_order=*/3);

  // Exact match
  EXPECT_TRUE(context.HasContext({1, 2}, /*include_all_suffixes=*/true));

  // Suffix match when include_all_suffixes is true
  EXPECT_TRUE(context.HasContext({2}, /*include_all_suffixes=*/true));
}

TEST(NGramExtendedContextTest, EmptyPattern) {
  NGramExtendedContext ext_context("", /*hi_order=*/3);
  EXPECT_TRUE(ext_context.NullContext());
  EXPECT_TRUE(ext_context.HasContext({1, 2}));
  EXPECT_EQ(ext_context.GetContext({1, 2}), nullptr);
}

TEST(NGramExtendedContextTest, MergingAdjacentContexts) {
  // Two adjacent intervals "1 1 : 1 3" and "1 3 : 1 5"
  NGramExtendedContext ext_context("1 1 : 1 3, 1 3 : 1 5", /*hi_order=*/3,
                                   /*merge_contexts=*/true);
  EXPECT_FALSE(ext_context.NullContext());
  ASSERT_EQ(ext_context.GetContexts().size(), 1);
  EXPECT_THAT(ext_context.GetContexts()[0].GetContextBegin(),
              ElementsAre(1, 1));
  EXPECT_THAT(ext_context.GetContexts()[0].GetContextEnd(), ElementsAre(1, 5));

  EXPECT_TRUE(ext_context.HasContext({1, 2}));
  EXPECT_TRUE(ext_context.HasContext({1, 4}));
  EXPECT_FALSE(ext_context.HasContext({1, 5}, /*include_all_suffixes=*/false));
}

TEST(NGramExtendedContextTest, DisjointContexts) {
  NGramExtendedContext ext_context("1 1 : 1 2, 1 4 : 1 5", /*hi_order=*/3,
                                   /*merge_contexts=*/true);
  ASSERT_EQ(ext_context.GetContexts().size(), 2);

  EXPECT_TRUE(ext_context.HasContext({1, 1}));
  EXPECT_FALSE(ext_context.HasContext({1, 3}, /*include_all_suffixes=*/false));
  EXPECT_TRUE(ext_context.HasContext({1, 4}));

  EXPECT_NE(ext_context.GetContext({1, 1}), nullptr);
  EXPECT_NE(ext_context.GetContext({1, 4}), nullptr);
  EXPECT_EQ(ext_context.GetContext({1, 3}, /*include_all_suffixes=*/false),
            nullptr);
}

TEST(NGramSplitTest, SplitsBigramByContext) {
  fst::StdVectorFst fst;
  fst.AddStates(4);
  fst.SetStart(1);
  fst.SetFinal(0, fst::StdArc::Weight(1.0f));
  fst.SetFinal(2, fst::StdArc::Weight(1.0f));
  fst.SetFinal(3, fst::StdArc::Weight(1.0f));
  fst.AddArc(1, fst::StdArc(0, 0, fst::StdArc::Weight(0.5f), 0));
  fst.AddArc(1, fst::StdArc(1, 1, fst::StdArc::Weight(1.0f), 2));
  fst.AddArc(1, fst::StdArc(2, 2, fst::StdArc::Weight(1.0f), 3));
  fst.AddArc(0, fst::StdArc(1, 1, fst::StdArc::Weight(1.0f), 2));
  fst.AddArc(0, fst::StdArc(2, 2, fst::StdArc::Weight(1.0f), 3));
  fst.AddArc(2, fst::StdArc(0, 0, fst::StdArc::Weight(0.5f), 0));
  fst.AddArc(2, fst::StdArc(1, 1, fst::StdArc::Weight(1.0f), 2));
  fst.AddArc(3, fst::StdArc(0, 0, fst::StdArc::Weight(0.5f), 0));
  fst.AddArc(3, fst::StdArc(2, 2, fst::StdArc::Weight(1.0f), 3));

  std::vector<std::string> patterns = {" : 2", "2 : "};
  std::vector<fst::StdVectorFst> split_fsts;
  EXPECT_TRUE(NGramSplit(fst, patterns, &split_fsts, /*phi_label=*/0,
                         /*include_all_suffixes=*/true));
  ASSERT_EQ(split_fsts.size(), 2);
  EXPECT_GT(split_fsts[0].NumStates(), 0);
  EXPECT_GT(split_fsts[1].NumStates(), 0);

  // Also tests splitting with include_all_suffixes=false so out-of-split
  // transitions from start state back off via backoff_states.
  std::vector<fst::StdVectorFst> strict_splits;
  EXPECT_TRUE(NGramSplit(fst, patterns, &strict_splits, /*phi_label=*/0,
                         /*include_all_suffixes=*/false));
  ASSERT_EQ(strict_splits.size(), 2);

  // Tests boundary-vector overload of NGramSplit.
  std::vector<std::vector<Label>> boundaries = {{}, {2}, {}};
  std::vector<fst::StdVectorFst> boundary_splits;
  EXPECT_TRUE(NGramSplit(fst, boundaries, &boundary_splits, /*phi_label=*/0,
                         /*include_all_suffixes=*/true));
  ASSERT_EQ(boundary_splits.size(), 2);
}

TEST(NGramSplitTest, SplitsUnigramAndHandlesEmptyFst) {
  fst::StdVectorFst empty_fst;
  std::vector<std::string> patterns = {" : 2", "2 : "};
  std::vector<fst::StdVectorFst> split_fsts;
  EXPECT_FALSE(NGramSplit(empty_fst, patterns, &split_fsts));

  std::vector<std::vector<Label>> boundaries = {{}, {2}, {}};
  EXPECT_FALSE(NGramSplit(empty_fst, boundaries, &split_fsts));

  // Unigram-only model (start state has no backoff arc).
  fst::StdVectorFst unigram_fst;
  unigram_fst.AddState();
  unigram_fst.SetStart(0);
  unigram_fst.SetFinal(0, fst::StdArc::Weight(1.0f));
  unigram_fst.AddArc(0, fst::StdArc(1, 1, fst::StdArc::Weight(0.5f), 0));
  unigram_fst.AddArc(0, fst::StdArc(2, 2, fst::StdArc::Weight(0.5f), 0));

  EXPECT_TRUE(NGramSplit(unigram_fst, patterns, &split_fsts, /*phi_label=*/0,
                         /*include_all_suffixes=*/true));
  ASSERT_EQ(split_fsts.size(), 2);
  EXPECT_EQ(split_fsts[0].NumStates(), 1);
  EXPECT_EQ(split_fsts[1].NumStates(), 1);
}

TEST(NGramExtendedContextTest, ConstructorsAndExtendedContextString) {
  NGramExtendedContext vec_ctor({1, 1}, {1, 3}, /*hi_order=*/2);
  EXPECT_FALSE(vec_ctor.NullContext());
  EXPECT_TRUE(vec_ctor.HasContext({1, 2}));

  // Single NullContext in vector constructor should normalize to empty.
  NGramExtendedContext null_vec(std::vector<NGramContext>{NGramContext()});
  EXPECT_TRUE(null_vec.NullContext());

  // Merges first two intervals and shifts third interval (i == j && i != k).
  std::vector<NGramContext> raw = {
      NGramContext("1 1 : 1 3", /*hi_order=*/2),
      NGramContext("1 3 : 1 5", /*hi_order=*/3),
      NGramContext("1 7 : 1 9", /*hi_order=*/4),
  };
  NGramExtendedContext merged(raw, /*merge_contexts=*/true);
  ASSERT_EQ(merged.GetContexts().size(), 2);
  EXPECT_EQ(
      NGramExtendedContext::GetExtendedContextString(merged.GetContexts()),
      "1 1 : 1 5,1 7 : 1 9");

  // Matches suffixes in GetContext (it->HasContext when
  // include_all_suffixes=true).
  EXPECT_NE(merged.GetContext({8}, /*include_all_suffixes=*/true), nullptr);
  EXPECT_NE(merged.GetContext({}, /*include_all_suffixes=*/true), nullptr);
}

}  // namespace
}  // namespace sfst
