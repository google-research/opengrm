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

#include "opengrm/ngram/ngram-count.h"

#include <cmath>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/ngram-model.h"

namespace ngram {
namespace {

using ::fst::Log64Arc;
using ::fst::LogArc;
using ::fst::StdArc;
using ::fst::SymbolTable;
using ::fst::VectorFst;
using ::testing::DoubleEq;
using ::testing::DoubleNear;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

TEST(NgramCountTest, ReverseContextUnigrams) {
  VectorFst<Log64Arc> example_fst;
  auto start = example_fst.AddState();
  example_fst.SetStart(start);
  auto end = example_fst.AddState();
  example_fst.SetFinal(end, Log64Arc::Weight::One());

  // Add path "1 2" with probability 0.5
  auto state1 = example_fst.AddState();
  example_fst.AddArc(start, Log64Arc(1, 1, -log(0.5), state1));
  example_fst.AddArc(state1, Log64Arc(2, 2, Log64Arc::Weight::One(), end));

  // Add path "3" with probability 0.5
  example_fst.AddArc(start, Log64Arc(3, 3, -log(0.5), end));

  NGramCounter<LogArc::Weight> counter(/*order=*/1);
  counter.Count(example_fst);
  std::vector<std::pair<std::vector<int>, std::pair<int, double>>> rcns;
  counter.GetReverseContextNGrams<StdArc>(&rcns);
  EXPECT_THAT(
      rcns,
      UnorderedElementsAre(
          Pair(std::vector<int>(), Pair(0, DoubleEq(0.0))),
          Pair(std::vector<int>(), Pair(1, DoubleNear(-log(0.5), kFloatEps))),
          Pair(std::vector<int>(), Pair(2, DoubleNear(-log(0.5), kFloatEps))),
          Pair(std::vector<int>(), Pair(3, DoubleNear(-log(0.5), kFloatEps)))));
}

TEST(NgramSymbolCountTest, AddUnigramCountToSymbols) {
  VectorFst<Log64Arc> example_fst;
  auto start = example_fst.AddState();
  example_fst.SetStart(start);
  auto end = example_fst.AddState();
  example_fst.SetFinal(end, Log64Arc::Weight::One());

  // Add path "1 2" with probability 0.5
  auto state1 = example_fst.AddState();
  example_fst.AddArc(start, Log64Arc(1, 1, -log(0.5), state1));
  example_fst.AddArc(state1, Log64Arc(2, 2, Log64Arc::Weight::One(), end));

  // Add path "3" with probability 0.5
  example_fst.AddArc(start, Log64Arc(3, 3, -log(0.5), end));

  SymbolTable syms;
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  syms.AddSymbol("d");
  example_fst.SetInputSymbols(&syms);
  example_fst.SetOutputSymbols(&syms);
  NGramCounter<LogArc::Weight> counter(/*order=*/1);
  counter.Count(example_fst);
  counter.AddCountToSymbolUnigrams(syms,
                                   /*neg_log_count=*/LogArc::Weight::One());
  std::vector<std::pair<std::vector<int>, std::pair<int, double>>> rcns;
  counter.GetReverseContextNGrams<StdArc>(&rcns);
  EXPECT_THAT(
      rcns,
      UnorderedElementsAre(
          Pair(std::vector<int>(), Pair(0, DoubleNear(-log(2.0), kFloatEps))),
          Pair(std::vector<int>(), Pair(1, DoubleNear(-log(1.5), kFloatEps))),
          Pair(std::vector<int>(), Pair(2, DoubleNear(-log(1.5), kFloatEps))),
          Pair(std::vector<int>(), Pair(3, DoubleNear(-log(1.5), kFloatEps))),
          Pair(std::vector<int>(), Pair(4, DoubleNear(-log(1.0), kFloatEps)))));
}

}  // namespace
}  // namespace ngram
