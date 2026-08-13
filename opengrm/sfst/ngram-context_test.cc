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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"

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

}  // namespace
}  // namespace sfst
