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

#include "opengrm/sfst/merge.h"

#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "absl/strings/match.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/arpa.h"
#include "opengrm/sfst/canonical.h"

namespace sfst {
namespace {

TEST(MergeTest, LinearInterpolateSingleTrigrams) {
  fst::SymbolTable syms("SharedSymbols");  // NOLINT(misc-include-cleaner)
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  syms.AddSymbol("d");

  std::string arpa1 =
      "\\data\\\n"
      "ngram 3=1\n"
      "\n"
      "\\3-grams:\n"
      "-0.2 a b c\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);

  std::string arpa2 =
      "\\data\\\n"
      "ngram 3=1\n"
      "\n"
      "\\3-grams:\n"
      "-0.5 b c d\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);

  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(
      IsCanonical(out_fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b c"));
  EXPECT_TRUE(absl::StrContains(output, "b c d"));
}

TEST(MergeTest, BayesInterpolateSingleTrigrams) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  syms.AddSymbol("d");

  std::string arpa1 =
      "\\data\\\n"
      "ngram 3=1\n"
      "\n"
      "\\3-grams:\n"
      "-0.2 a b c\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);

  std::string arpa2 =
      "\\data\\\n"
      "ngram 3=1\n"
      "\n"
      "\\3-grams:\n"
      "-0.5 b c d\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);

  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(BayesMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(
      IsCanonical(out_fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b c"));
  EXPECT_TRUE(absl::StrContains(output, "b c d"));
}

TEST(MergeTest, LinearMergeDisjointVocabularies) {
  fst::SymbolTable syms("SharedSymbols");  // NOLINT(misc-include-cleaner)
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  syms.AddSymbol("d");
  syms.AddSymbol("e");
  syms.AddSymbol("f");
  std::string arpa1 =
      "\\data\\\nngram 2=1\n\n\\2-grams:\n-0.2 a b\n\n\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);
  std::string arpa2 =
      "\\data\\\nngram 2=1\n\n\\2-grams:\n-0.5 d e\n\n\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);
  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(
      IsCanonical(out_fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)
  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b"));
  EXPECT_TRUE(absl::StrContains(output, "d e"));
}

TEST(MergeTest, LinearMergeDifferingOrders) {
  fst::SymbolTable syms("SharedSymbols");  // NOLINT(misc-include-cleaner)
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  std::string arpa1 =
      "\\data\\\nngram 3=1\n\n\\3-grams:\n-0.2 a b c\n\n\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);
  std::string arpa2 =
      "\\data\\\nngram 2=1\n\n\\2-grams:\n-0.5 b c\n\n\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);
  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(
      IsCanonical(out_fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)
  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b c"));
  EXPECT_TRUE(absl::StrContains(output, "b c"));
}

TEST(MergeTest, ReadWriteUnigramRoundTrip) {
  fst::SymbolTable syms("SharedSymbols");  // NOLINT(misc-include-cleaner)
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  std::string arpa =
      "\\data\\\nngram 1=2\n\n\\1-grams:\n-0.2 a\n-0.5 b\n\n\\end\\\n";
  std::stringstream istrm(arpa);
  fst::VectorFst<fst::StdArc> fst;
  fst.SetInputSymbols(&syms);
  fst.SetOutputSymbols(&syms);
  ReadArpa(istrm, &fst);
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));  // NOLINT(misc-include-cleaner)
  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "ngram 1=2"));
  EXPECT_TRUE(absl::StrContains(output, "\\1-grams:"));
  EXPECT_TRUE(absl::StrContains(output, "a"));
  EXPECT_TRUE(absl::StrContains(output, "b"));
}

TEST(MergeTest, IncompatibleSymbolTablesRejected) {
  fst::SymbolTable syms1("Symbols1");
  syms1.AddSymbol("<epsilon>");
  syms1.AddSymbol("a");
  syms1.AddSymbol("b");

  fst::SymbolTable syms2("Symbols2");
  syms2.AddSymbol("<epsilon>");
  syms2.AddSymbol("c");
  syms2.AddSymbol("d");

  std::string arpa1 = "\\data\\\nngram 1=1\n\n\\1-grams:\n-0.2 a\n\n\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms1);
  fst1.SetOutputSymbols(&syms1);
  ReadArpa(istrm1, &fst1);

  std::string arpa2 = "\\data\\\nngram 1=1\n\n\\1-grams:\n-0.5 c\n\n\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms2);
  fst2.SetOutputSymbols(&syms2);
  ReadArpa(istrm2, &fst2);

  fst::VectorFst<fst::StdArc> out_fst;
  // Merging models with conflicting symbol tables must fail safely.
  EXPECT_FALSE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_FALSE(BayesMerge(fst1, fst2, 0.5, 0.5, &out_fst));
}

TEST(MergeTest, NullSymbolTablesAllowed) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");

  std::string arpa1 = "\\data\\\nngram 1=1\n\n\\1-grams:\n-0.2 a\n\n\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);
  fst1.SetInputSymbols(nullptr);
  fst1.SetOutputSymbols(nullptr);

  std::string arpa2 = "\\data\\\nngram 1=1\n\n\\1-grams:\n-0.5 b\n\n\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);
  fst2.SetInputSymbols(nullptr);
  fst2.SetOutputSymbols(nullptr);

  fst::VectorFst<fst::StdArc> out_fst;
  // Merging pure integer-labeled FSTs without symbol tables is allowed.
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));
  EXPECT_EQ(out_fst.InputSymbols(), nullptr);

  EXPECT_TRUE(BayesMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));
  EXPECT_EQ(out_fst.InputSymbols(), nullptr);
}

TEST(MergeTest, OneNullSymbolTableAllowed) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>");
  syms.AddSymbol("a");
  syms.AddSymbol("b");

  std::string arpa1 = "\\data\\\nngram 1=1\n\n\\1-grams:\n-0.2 a\n\n\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);

  std::string arpa2 = "\\data\\\nngram 1=1\n\n\\1-grams:\n-0.5 b\n\n\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);
  fst2.SetInputSymbols(nullptr);
  fst2.SetOutputSymbols(nullptr);

  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));
  EXPECT_NE(out_fst.InputSymbols(), nullptr);

  EXPECT_TRUE(BayesMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));
  EXPECT_NE(out_fst.InputSymbols(), nullptr);
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
