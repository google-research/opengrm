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

// Unit tests for Baum-Welch primitive string scoring functions.

#include "opengrm/baumwelch/score.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

TEST(ScoreTest, HammingDistanceStringEqual) {
  EXPECT_EQ(HammingDistance("abc", "abc"), 0);
  EXPECT_EQ(HammingDistance("", ""), 0);
}

TEST(ScoreTest, HammingDistanceStringSubstitution) {
  EXPECT_EQ(HammingDistance("abc", "adc"), 1);
  EXPECT_EQ(HammingDistance("abc", "xyz"), 3);
}

TEST(ScoreTest, HammingDistanceStringLengthMismatch) {
  EXPECT_EQ(HammingDistance("abcde", "abc"), 2);
  EXPECT_EQ(HammingDistance("abc", "abcde"), 2);
  EXPECT_EQ(HammingDistance("abc", ""), 3);
  EXPECT_EQ(HammingDistance("", "abc"), 3);
  EXPECT_EQ(HammingDistance("abc", "ad"), 2);
}

TEST(ScoreTest, HammingDistanceFst) {
  const StringCompiler<StdArc> compiler(TokenType::BYTE);
  VectorFst<StdArc> gld, hyp1, hyp2, hyp3;

  ASSERT_TRUE(compiler("cat", &gld));
  ASSERT_TRUE(compiler("cut", &hyp1));
  ASSERT_TRUE(compiler("cat", &hyp2));
  ASSERT_TRUE(compiler("cater", &hyp3));

  EXPECT_EQ(HammingDistance(gld, hyp1), 1);
  EXPECT_EQ(HammingDistance(gld, hyp2), 0);
  EXPECT_EQ(HammingDistance(gld, hyp3), 2);
}

TEST(ScoreTest, HammingDistanceFstUtf8MultiByte) {
  const StringCompiler<StdArc> compiler(TokenType::UTF8);
  VectorFst<StdArc> gld, hyp;

  // "é" (2 UTF-8 bytes) vs "a" (1 byte) should be 1 codepoint substitution
  // without misaligning the trailing "bc".
  ASSERT_TRUE(compiler("ébc", &gld));
  ASSERT_TRUE(compiler("abc", &hyp));

  EXPECT_EQ(HammingDistance(gld, hyp), 1);
}

TEST(ScoreTest, HammingDistanceFstSymbolMultiChar) {
  SymbolTable syms;
  syms.AddSymbol("<eps>", 0);
  syms.AddSymbol("cat", 1);
  syms.AddSymbol("dog", 2);
  syms.AddSymbol("elephant", 3);

  const StringCompiler<StdArc> compiler(TokenType::SYMBOL, &syms);
  VectorFst<StdArc> gld, hyp;
  ASSERT_TRUE(compiler("cat dog", &gld));
  ASSERT_TRUE(compiler("elephant dog", &hyp));
  gld.SetOutputSymbols(&syms);
  hyp.SetOutputSymbols(&syms);

  // Differing symbol string lengths must not misalign subsequent symbols.
  EXPECT_EQ(HammingDistance(gld, hyp), 1);
}

TEST(ScoreTest, HammingDistanceFstOmitsEpsilons) {
  VectorFst<StdArc> gld, hyp;
  const auto g0 = gld.AddState();
  const auto g1 = gld.AddState();
  const auto g2 = gld.AddState();
  gld.SetStart(g0);
  gld.AddArc(g0, StdArc(1, 1, 0.0, g1));
  gld.AddArc(g1, StdArc(2, 2, 0.0, g2));
  gld.SetFinal(g2, StdArc::Weight::One());

  const auto h0 = hyp.AddState();
  const auto h1 = hyp.AddState();
  const auto h2 = hyp.AddState();
  const auto h3 = hyp.AddState();
  hyp.SetStart(h0);
  hyp.AddArc(h0, StdArc(1, 1, 0.0, h1));
  hyp.AddArc(h1, StdArc(0, 0, 0.0, h2));
  hyp.AddArc(h2, StdArc(2, 2, 0.0, h3));
  hyp.SetFinal(h3, StdArc::Weight::One());

  EXPECT_EQ(HammingDistance(gld, hyp), 0);
}

TEST(ScoreTest, HammingDistanceFstNonStringDeathTest) {
  const StringCompiler<StdArc> compiler(TokenType::BYTE);
  VectorFst<StdArc> gld;
  ASSERT_TRUE(compiler("cat", &gld));

  // Construct cyclic (non-string) FST.
  VectorFst<StdArc> cyclic;
  const auto s0 = cyclic.AddState();
  cyclic.SetStart(s0);
  cyclic.AddArc(s0, StdArc(1, 1, 0.0, s0));  // self-loop
  cyclic.SetFinal(s0, StdArc::Weight(0.0));

  EXPECT_DEATH(HammingDistance(cyclic, gld), "Label extraction failed");
  EXPECT_DEATH(HammingDistance(gld, cyclic), "Label extraction failed");
}

TEST(ScoreTest, HammingDistanceFstSymbolTableMismatchDeathTest) {
  SymbolTable syms1;
  syms1.AddSymbol("a", 1);
  SymbolTable syms2;
  syms2.AddSymbol("b", 1);

  const StringCompiler<StdArc> compiler1(TokenType::SYMBOL, &syms1);
  const StringCompiler<StdArc> compiler2(TokenType::SYMBOL, &syms2);
  VectorFst<StdArc> fst1, fst2;
  ASSERT_TRUE(compiler1("a", &fst1));
  ASSERT_TRUE(compiler2("b", &fst2));
  fst1.SetOutputSymbols(&syms1);
  fst2.SetOutputSymbols(&syms2);

  EXPECT_DEATH(HammingDistance(fst1, fst2), "Output symbol table mismatch");
}

TEST(ScoreTest, HammingDistanceFar) {
  const std::string gld_path =
      fst::JoinPath(::testing::TempDir(), "score_gld.far");
  const std::string hyp_path =
      fst::JoinPath(::testing::TempDir(), "score_hyp.far");

  const StringCompiler<StdArc> compiler(TokenType::BYTE);
  VectorFst<StdArc> fst_cat, fst_cut, fst_dog;

  ASSERT_TRUE(compiler("cat", &fst_cat));
  ASSERT_TRUE(compiler("cut", &fst_cut));
  ASSERT_TRUE(compiler("dog", &fst_dog));

  {
    std::unique_ptr<FarWriter<StdArc>> gld_writer(
        FarWriter<StdArc>::Create(gld_path));
    ASSERT_NE(gld_writer, nullptr);
    gld_writer->Add("0", fst_cat);
    gld_writer->Add("1", fst_dog);

    std::unique_ptr<FarWriter<StdArc>> hyp_writer(
        FarWriter<StdArc>::Create(hyp_path));
    ASSERT_NE(hyp_writer, nullptr);
    hyp_writer->Add("0", fst_cut);
    hyp_writer->Add("1", fst_dog);
  }

  std::unique_ptr<FarReader<StdArc>> gld_reader(
      FarReader<StdArc>::Open(gld_path));
  ASSERT_NE(gld_reader, nullptr);
  std::unique_ptr<FarReader<StdArc>> hyp_reader(
      FarReader<StdArc>::Open(hyp_path));
  ASSERT_NE(hyp_reader, nullptr);

  // "cat" vs "cut" (dist 1) + "dog" vs "dog" (dist 0) = 1.
  EXPECT_EQ(HammingDistance(*gld_reader, *hyp_reader), 1);

  // Verify that readers were reset to initial position.
  EXPECT_FALSE(gld_reader->Done());
  EXPECT_FALSE(hyp_reader->Done());
}

TEST(ScoreTest, HammingDistanceFarCountMismatchDeathTest) {
  const std::string gld_path =
      fst::JoinPath(::testing::TempDir(), "mismatch_gld.far");
  const std::string hyp_path =
      fst::JoinPath(::testing::TempDir(), "mismatch_hyp.far");

  const StringCompiler<StdArc> compiler(TokenType::BYTE);
  VectorFst<StdArc> fst_cat, fst_dog;

  ASSERT_TRUE(compiler("cat", &fst_cat));
  ASSERT_TRUE(compiler("dog", &fst_dog));

  {
    std::unique_ptr<FarWriter<StdArc>> gld_writer(
        FarWriter<StdArc>::Create(gld_path));
    ASSERT_NE(gld_writer, nullptr);
    gld_writer->Add("0", fst_cat);
    gld_writer->Add("1", fst_dog);  // 2 FSTs in gld

    std::unique_ptr<FarWriter<StdArc>> hyp_writer(
        FarWriter<StdArc>::Create(hyp_path));
    ASSERT_NE(hyp_writer, nullptr);
    hyp_writer->Add("0", fst_cat);  // Only 1 FST in hyp
  }

  std::unique_ptr<FarReader<StdArc>> gld_reader(
      FarReader<StdArc>::Open(gld_path));
  ASSERT_NE(gld_reader, nullptr);
  std::unique_ptr<FarReader<StdArc>> hyp_reader(
      FarReader<StdArc>::Open(hyp_path));
  ASSERT_NE(hyp_reader, nullptr);

  EXPECT_DEATH(HammingDistance(*gld_reader, *hyp_reader),
               "FAR reader size mismatch");
}

}  // namespace
}  // namespace fst
