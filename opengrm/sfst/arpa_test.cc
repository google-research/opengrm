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

#include "opengrm/sfst/arpa.h"

#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "absl/strings/match.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/canonical.h"

namespace sfst {
namespace {

TEST(ArpaTest, ReadBasic) {
  std::string arpa_data =
      "\\data\\\n"
      "ngram 1=3\n"
      "ngram 2=2\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a\n"
      "-0.6 b\n"
      "-0.7 c\n"
      "\n"
      "\\2-grams:\n"
      "-0.1 a b\n"
      "-0.2 b c\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_TRUE(ReadArpa(istrm, &fst));
  // Start state + 3 unigram histories = 4 states in canonical 2-gram topology.
  EXPECT_EQ(fst.NumStates(), 4);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)
}

TEST(ArpaTest, WriteBasic) {
  fst::VectorFst<fst::StdArc> fst;
  {
    fst::SymbolTable syms("ARPASymbols");
    fst.SetInputSymbols(&syms);
  }
  auto* isyms = fst.MutableInputSymbols();
  isyms->AddSymbol("<epsilon>");
  isyms->AddSymbol("a");
  isyms->AddSymbol("b");
  fst.SetOutputSymbols(isyms);
  auto s = fst.AddState();
  fst.SetStart(s);
  fst.AddArc(s, fst::StdArc(1, 1, 0.5, fst.AddState()));
  fst.AddArc(s, fst::StdArc(2, 2, 1.0, fst.AddState()));

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "\\data\\"));
  EXPECT_TRUE(absl::StrContains(output, "ngram 1=2"));
  EXPECT_TRUE(absl::StrContains(output, "\\1-grams:"));
  EXPECT_TRUE(absl::StrContains(output, "a"));
  EXPECT_TRUE(absl::StrContains(output, "b"));
}

TEST(ArpaTest, ReadWriteSingleTrigramRoundTrip) {
  std::string arpa_data =
      "\\data\\\n"
      "ngram 3=1\n"
      "\n"
      "\\3-grams:\n"
      "-0.2 a b c\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_TRUE(ReadArpa(istrm, &fst));
  EXPECT_EQ(fst.NumStates(), 6);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)
  EXPECT_NE(fst.Start(), fst::kNoStateId);       // NOLINT(misc-include-cleaner)

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "\\data\\"));
  EXPECT_TRUE(absl::StrContains(output, "ngram 1=3"));
  EXPECT_TRUE(absl::StrContains(output, "ngram 2=2"));
  EXPECT_TRUE(absl::StrContains(output, "ngram 3=1"));
  EXPECT_TRUE(absl::StrContains(output, "\\1-grams:"));
  EXPECT_TRUE(absl::StrContains(output, "a"));
  EXPECT_TRUE(absl::StrContains(output, "b"));
  EXPECT_TRUE(absl::StrContains(output, "c"));
  EXPECT_TRUE(absl::StrContains(output, "\\2-grams:"));
  EXPECT_TRUE(absl::StrContains(output, "a b"));
  EXPECT_TRUE(absl::StrContains(output, "b c"));
  EXPECT_TRUE(absl::StrContains(output, "\\3-grams:"));
  EXPECT_TRUE(absl::StrContains(output, "a b c"));
}

TEST(ArpaTest, WriteSentenceBoundaries) {
  fst::VectorFst<fst::StdArc> fst;
  {
    fst::SymbolTable syms("ARPASymbols");
    fst.SetInputSymbols(&syms);
  }
  auto* isyms = fst.MutableInputSymbols();
  isyms->AddSymbol("<epsilon>");
  isyms->AddSymbol("<s>");
  isyms->AddSymbol("</s>");
  isyms->AddSymbol("a");
  fst.SetOutputSymbols(isyms);
  auto s0 = fst.AddState();
  fst.SetStart(s0);
  auto s1 = fst.AddState();
  fst.AddArc(s0, fst::StdArc(3, 3, 0.5, s1));  // arc labeled 'a'
  fst.SetFinal(s1, fst::StdArc::Weight(1.0));

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "<s>"));
  EXPECT_TRUE(absl::StrContains(output, "</s>"));
  EXPECT_TRUE(absl::StrContains(output, "a </s>"));
}

TEST(ArpaTest, WriteSentenceBoundariesFallback) {
  fst::VectorFst<fst::StdArc> fst;
  {
    fst::SymbolTable syms("ARPASymbolsFallback");
    fst.SetInputSymbols(&syms);
  }
  auto* isyms = fst.MutableInputSymbols();
  isyms->AddSymbol("<epsilon>");
  isyms->AddSymbol("<S>");
  isyms->AddSymbol("</S>");
  isyms->AddSymbol("word");
  fst.SetOutputSymbols(isyms);
  auto s0 = fst.AddState();
  fst.SetStart(s0);
  auto s1 = fst.AddState();
  fst.AddArc(s0, fst::StdArc(3, 3, 2.0, s1));  // arc labeled 'word'
  fst.SetFinal(s1, fst::StdArc::Weight(0.5));

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "<S>"));
  EXPECT_TRUE(absl::StrContains(output, "</S>"));
  EXPECT_TRUE(absl::StrContains(output, "word </S>"));
}

TEST(ArpaTest, WriteSentenceBoundariesModernAliases) {
  fst::VectorFst<fst::StdArc> fst;
  {
    fst::SymbolTable syms("ARPASymbolsModern");
    fst.SetInputSymbols(&syms);
  }
  auto* isyms = fst.MutableInputSymbols();
  isyms->AddSymbol("<epsilon>");
  isyms->AddSymbol("<bos>");
  isyms->AddSymbol("<eos>");
  isyms->AddSymbol("word");
  fst.SetOutputSymbols(isyms);
  auto s0 = fst.AddState();
  fst.SetStart(s0);
  auto s1 = fst.AddState();
  fst.AddArc(s0, fst::StdArc(3, 3, 2.0, s1));
  fst.SetFinal(s1, fst::StdArc::Weight(0.5));

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "<bos>"));
  EXPECT_TRUE(absl::StrContains(output, "<eos>"));
  EXPECT_TRUE(absl::StrContains(output, "word <eos>"));
}

TEST(ArpaTest, ReadWithBackoffWeights) {
  std::string arpa_data =
      "\\data\\\n"
      "ngram 1=2\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a -0.2\n"
      "-0.6 b\n"
      "\n"
      "\\2-grams:\n"
      "-0.1 a b\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_TRUE(ReadArpa(istrm, &fst));
  EXPECT_GT(fst.NumStates(), 1);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));

  // Verify that backoff weight -0.2 on 1-gram 'a' is preserved.
  bool found_bo = false;
  for (fst::StateIterator<fst::VectorFst<fst::StdArc>> siter(fst);
       !siter.Done(); siter.Next()) {
    auto s = siter.Value();
    for (fst::ArcIterator<fst::VectorFst<fst::StdArc>> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      const auto& arc = aiter.Value();
      if (arc.ilabel == fst::kNoLabel &&
          std::abs(arc.weight.Value() - (0.2 * std::log(10.0))) < 1e-4) {
        found_bo = true;
      }
    }
  }
  EXPECT_TRUE(found_bo);
}

TEST(ArpaTest, ReadInvalidOrderHeader) {
  // Tests that malformed order headers are skipped without crashing,
  // that current_order is reset so subsequent lines are ignored,
  // and that negative/zero/non-numeric orders cause ReadArpa to return false.
  std::string arpa_data =
      "\\data\\\n"
      "ngram 1=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a\n"
      "\n"
      "\\invalid-grams:\n"
      "-0.5 foo\n"
      "\n"
      "\\-1-grams:\n"
      "-0.5 bar\n"
      "\n"
      "\\0-grams:\n"
      "-0.5 baz\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_FALSE(ReadArpa(istrm, &fst));
  // Only the valid 1-gram 'a' is loaded; 'foo', 'bar', 'baz' are not loaded.
  EXPECT_EQ(fst.NumStates(), 1);
  EXPECT_EQ(fst.NumArcs(fst.Start()), 1);
  EXPECT_EQ(fst.InputSymbols()->Find("foo"), fst::kNoSymbol);
  EXPECT_EQ(fst.InputSymbols()->Find("bar"), fst::kNoSymbol);
  EXPECT_EQ(fst.InputSymbols()->Find("baz"), fst::kNoSymbol);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));
}

TEST(ArpaTest, ReadInvalidNgramLines) {
  // Tests that lines with non-numeric log probabilities or corrupted tokens
  // cause ReadArpa to return false and are skipped.
  std::string arpa_data =
      "\\data\\\n"
      "ngram 1=2\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "not_a_float a\n"
      "-0.5 valid_word\n"
      "-0.3 bad_bo not_a_number\n"
      "\n"
      "\\2-grams:\n"
      "-0.1 valid_word valid_word\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_FALSE(ReadArpa(istrm, &fst));
  // 'valid_word' and 'bad_bo' are parsed, while 'not_a_float' is skipped.
  EXPECT_GT(fst.NumStates(), 1);
  EXPECT_EQ(fst.InputSymbols()->Find("not_a_float"), fst::kNoSymbol);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));
}

TEST(ArpaTest, ReadZeroOrNegativeOrderHeader) {
  // Tests that zero or negative order headers specifically return false.
  std::string arpa_data =
      "\\data\\\n"
      "ngram 1=1\n"
      "\n"
      "\\0-grams:\n"
      "-0.5 foo\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_FALSE(ReadArpa(istrm, &fst));
  EXPECT_EQ(fst.InputSymbols()->Find("foo"), fst::kNoSymbol);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));
}

TEST(ArpaTest, ReadInsufficientTokensInNgramLine) {
  // Tests that n-gram lines with fewer tokens than current_order return false.
  std::string arpa_data =
      "\\data\\\n"
      "ngram 1=1\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a\n"
      "\n"
      "\\2-grams:\n"
      "-0.1 a\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm(arpa_data);
  fst::VectorFst<fst::StdArc> fst;
  EXPECT_FALSE(ReadArpa(istrm, &fst));
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
