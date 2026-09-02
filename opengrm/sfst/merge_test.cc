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
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/match.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/arpa.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/normalize.h"

namespace sfst {
namespace {

TEST(MergeTest, LinearInterpolateSingleTrigrams) {
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
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst));
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));

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
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));

  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b c"));
  EXPECT_TRUE(absl::StrContains(output, "b c d"));
}

TEST(MergeTest, LinearMergeDisjointVocabularies) {
  fst::SymbolTable syms("SharedSymbols");
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
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));
  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b"));
  EXPECT_TRUE(absl::StrContains(output, "d e"));
}

TEST(MergeTest, LinearMergeDifferingOrders) {
  fst::SymbolTable syms("SharedSymbols");
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
  EXPECT_TRUE(IsCanonical(out_fst, fst::kNoLabel));
  std::stringstream ostrm;
  EXPECT_TRUE(WriteArpa(out_fst, ostrm));
  std::string output = ostrm.str();
  EXPECT_TRUE(absl::StrContains(output, "a b c"));
  EXPECT_TRUE(absl::StrContains(output, "b c"));
}

TEST(MergeTest, ReadWriteUnigramRoundTrip) {
  fst::SymbolTable syms("SharedSymbols");
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
  EXPECT_TRUE(IsCanonical(fst, fst::kNoLabel));
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

TEST(MergeTest, LinearMergeWithCustomPhiLabel) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>");  // 0
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  syms.AddSymbol("d");

  std::string arpa1 =
      "\\data\\\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a -0.3\n"
      "\n"
      "\\2-grams:\n"
      "-0.2 a b\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);

  std::string arpa2 =
      "\\data\\\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 c -0.3\n"
      "\n"
      "\\2-grams:\n"
      "-0.2 c d\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);

  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(out_fst, /*phi_label=*/0));

  int epsilon_count = 0;
  int knolabel_count = 0;
  for (fst::StateIterator<fst::VectorFst<fst::StdArc>> siter(out_fst);
       !siter.Done(); siter.Next()) {
    for (fst::ArcIterator<fst::VectorFst<fst::StdArc>> aiter(out_fst,
                                                             siter.Value());
         !aiter.Done(); aiter.Next()) {
      if (aiter.Value().ilabel == 0) ++epsilon_count;
      if (aiter.Value().ilabel == fst::kNoLabel) ++knolabel_count;
    }
  }
  EXPECT_GT(epsilon_count, 0);
  EXPECT_EQ(knolabel_count, 0);
}

TEST(MergeTest, BayesMergeWithCustomPhiLabel) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>");  // 0
  syms.AddSymbol("a");
  syms.AddSymbol("b");
  syms.AddSymbol("c");
  syms.AddSymbol("d");

  std::string arpa1 =
      "\\data\\\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 a -0.3\n"
      "\n"
      "\\2-grams:\n"
      "-0.2 a b\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ReadArpa(istrm1, &fst1);

  std::string arpa2 =
      "\\data\\\n"
      "ngram 2=1\n"
      "\n"
      "\\1-grams:\n"
      "-0.5 c -0.3\n"
      "\n"
      "\\2-grams:\n"
      "-0.2 c d\n"
      "\n"
      "\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ReadArpa(istrm2, &fst2);

  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(BayesMerge(fst1, fst2, 0.5, 0.5, &out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(out_fst, /*phi_label=*/0));

  int epsilon_count = 0;
  int knolabel_count = 0;
  for (fst::StateIterator<fst::VectorFst<fst::StdArc>> siter(out_fst);
       !siter.Done(); siter.Next()) {
    for (fst::ArcIterator<fst::VectorFst<fst::StdArc>> aiter(out_fst,
                                                             siter.Value());
         !aiter.Done(); aiter.Next()) {
      if (aiter.Value().ilabel == 0) ++epsilon_count;
      if (aiter.Value().ilabel == fst::kNoLabel) ++knolabel_count;
    }
  }
  EXPECT_GT(epsilon_count, 0);
  EXPECT_EQ(knolabel_count, 0);
}

TEST(MergeTest, MergeOpenGrmTopologyWithBosAndFinalWeights) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  syms.AddSymbol("b", 2);

  auto make_opengrm_fst = [&](int label, float weight) {
    fst::VectorFst<fst::StdArc> fst;
    fst.SetInputSymbols(&syms);
    fst.SetOutputSymbols(&syms);
    const auto start = fst.AddState();
    const auto unigram_state = fst.AddState();
    const auto word_state = fst.AddState();
    fst.SetStart(start);
    fst.AddArc(start, fst::StdArc(0, 0, 0.0, unigram_state));
    fst.SetFinal(unigram_state, fst::StdArc::Weight::One());
    fst.AddArc(
        unigram_state,
        fst::StdArc(label, label, fst::StdArc::Weight(weight), word_state));
    fst.SetFinal(word_state, fst::StdArc::Weight::One());
    fst.AddArc(word_state,
               fst::StdArc(0, 0, fst::StdArc::Weight::One(), unigram_state));
    EXPECT_TRUE(RecalcBackoff(&fst, /*phi_label=*/0));
    return fst;
  };

  auto fst1 = make_opengrm_fst(1, 1.0f);
  auto fst2 = make_opengrm_fst(2, 2.0f);

  fst::VectorFst<fst::StdArc> out_fst;
  EXPECT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(out_fst, /*phi_label=*/0));
  EXPECT_EQ(out_fst.NumStates(), 4);  // start, unigram, state_a, state_b
  EXPECT_NE(FindUnigramState(out_fst, /*phi_label=*/0), fst::kNoStateId);
  EXPECT_NE(internal::ScoreNGram(out_fst, {internal::kBosSentinel, 1, 2},
                                 /*phi_label=*/0),
            fst::StdArc::Weight::Zero());

  fst::VectorFst<fst::StdArc> bayes_out_fst;
  EXPECT_TRUE(
      BayesMerge(fst1, fst2, 0.5, 0.5, &bayes_out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(bayes_out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(bayes_out_fst, /*phi_label=*/0));
  EXPECT_EQ(bayes_out_fst.NumStates(), 4);
}

TEST(MergeTest, CollapsesLeafDestinationHistoriesWithoutOutgoingNGrams) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  syms.AddSymbol("b", 2);
  syms.AddSymbol("c", 3);
  syms.AddSymbol("d", 4);

  std::string arpa1 =
      "\\data\\\n"
      "ngram 1=4\n"
      "ngram 2=1\n\n"
      "\\1-grams:\n"
      "-0.6 a -0.3\n"
      "-0.6 b\n"
      "-0.6 c\n"
      "-0.6 d\n\n"
      "\\2-grams:\n"
      "-0.3 a b\n\n"
      "\\end\\\n";
  std::stringstream istrm1(arpa1);
  fst::VectorFst<fst::StdArc> fst1;
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);
  ASSERT_TRUE(ReadArpa(istrm1, &fst1, /*phi_label=*/0));

  std::string arpa2 =
      "\\data\\\n"
      "ngram 1=4\n"
      "ngram 2=1\n\n"
      "\\1-grams:\n"
      "-0.6 a\n"
      "-0.6 b\n"
      "-0.6 c -0.3\n"
      "-0.6 d\n\n"
      "\\2-grams:\n"
      "-0.3 c d\n\n"
      "\\end\\\n";
  std::stringstream istrm2(arpa2);
  fst::VectorFst<fst::StdArc> fst2;
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);
  ASSERT_TRUE(ReadArpa(istrm2, &fst2, /*phi_label=*/0));

  fst::VectorFst<fst::StdArc> out_fst;
  ASSERT_TRUE(LinearMerge(fst1, fst2, 0.5, 0.5, &out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(out_fst, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(out_fst, /*phi_label=*/0));
  // Only histories with outgoing n-grams ({}, {a}, {c}) should have states;
  // leaf words b and d have no outgoing bigrams and must route back to the
  // unigram state rather than allocating dead-end backoff-only states.
  EXPECT_EQ(out_fst.NumStates(), 3);
}

TEST(MergeTest, EmptyFstsAndScorerEdgeCases) {
  fst::SymbolTable syms("EmptySyms");
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  fst::VectorFst<fst::StdArc> empty1;
  empty1.SetInputSymbols(&syms);
  empty1.SetOutputSymbols(&syms);
  fst::VectorFst<fst::StdArc> empty2;
  fst::VectorFst<fst::StdArc> out_fst;
  out_fst.AddState();
  out_fst.SetStart(0);
  out_fst.SetFinal(0, fst::StdArc::Weight::One());
  EXPECT_TRUE(LinearMerge(empty1, empty2, 0.5, 0.5, &out_fst));
  EXPECT_EQ(out_fst.NumStates(), 0);
  EXPECT_EQ(out_fst.Start(), fst::kNoStateId);
  ASSERT_NE(out_fst.InputSymbols(), nullptr);
  ASSERT_NE(out_fst.OutputSymbols(), nullptr);
  EXPECT_EQ(out_fst.InputSymbols()->Find("a"), 1);
  EXPECT_EQ(out_fst.OutputSymbols()->Find("a"), 1);
  // Merging two symbol-less empty FSTs clears stale states and symbols.
  fst::VectorFst<fst::StdArc> bare_empty;
  EXPECT_TRUE(LinearMerge(bare_empty, empty2, 0.5, 0.5, &out_fst));
  EXPECT_EQ(out_fst.NumStates(), 0);
  EXPECT_EQ(out_fst.InputSymbols(), nullptr);
  EXPECT_EQ(out_fst.OutputSymbols(), nullptr);
  EXPECT_EQ(internal::ScoreNGram(empty1, {1}), fst::StdArc::Weight::Zero());

  fst::VectorFst<fst::StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);
  fst.SetFinal(1, fst::StdArc::Weight::One());
  fst.AddArc(0, fst::StdArc(0, 0, fst::StdArc::Weight(0.5f), 1));
  fst.AddArc(1, fst::StdArc(1, 1, fst::StdArc::Weight(0.5f), 0));
  EXPECT_EQ(internal::ScoreNGram(fst, {}, /*phi_label=*/0),
            fst::StdArc::Weight::Zero());
  // State 0 has Final == Zero and backs off along phi_label=0 to state 1 for
  // EOS.
  EXPECT_NEAR(
      internal::ScoreNGram(fst, {1, internal::kEosSentinel}, /*phi_label=*/0)
          .Value(),
      0.5f, 1e-5f);

  // When a non-zero phi_label is specified, an outgoing epsilon (0) arc must
  // not be treated as a backoff transition.
  EXPECT_EQ(
      internal::ScoreNGram(fst, {1, internal::kEosSentinel}, /*phi_label=*/5),
      fst::StdArc::Weight::Zero());

  // When phi_label == fst::kNoLabel, backoff arcs labeled with kNoLabel are
  // traversed during scoring and normalized in the merged output.
  fst::VectorFst<fst::StdArc> knolabel_fst;
  knolabel_fst.AddState();
  knolabel_fst.AddState();
  knolabel_fst.SetStart(0);
  knolabel_fst.SetFinal(1, fst::StdArc::Weight::One());
  knolabel_fst.AddArc(0, fst::StdArc(fst::kNoLabel, fst::kNoLabel,
                                     fst::StdArc::Weight(0.5f), 1));
  knolabel_fst.AddArc(1, fst::StdArc(1, 1, fst::StdArc::Weight(0.5f), 0));
  EXPECT_NEAR(internal::ScoreNGram(knolabel_fst, {1, internal::kEosSentinel},
                                   fst::kNoLabel)
                  .Value(),
              0.5f, 1e-5f);
  // When phi_label != fst::kNoLabel, kNoLabel arcs are ignored by
  // GetBackoffState, FindUnigramState, NGramScorer, and LinearMerge/BayesMerge
  // (preventing infinite loops on malformed kNoLabel self-loops).
  EXPECT_EQ(FindUnigramState(knolabel_fst, /*phi_label=*/0), fst::kNoStateId);
  EXPECT_EQ(internal::ScoreNGram(knolabel_fst, {1, internal::kEosSentinel},
                                 /*phi_label=*/0),
            fst::StdArc::Weight::Zero());
  fst::VectorFst<fst::StdArc> self_loop_fst;
  self_loop_fst.AddState();
  self_loop_fst.AddState();
  self_loop_fst.SetStart(0);
  self_loop_fst.SetFinal(1, fst::StdArc::Weight(0.693147f));
  self_loop_fst.AddArc(0, fst::StdArc(0, 0, fst::StdArc::Weight::One(), 1));
  self_loop_fst.AddArc(1, fst::StdArc(fst::kNoLabel, fst::kNoLabel,
                                      fst::StdArc::Weight(0.3f), 1));
  self_loop_fst.AddArc(1, fst::StdArc(1, 1, fst::StdArc::Weight(0.693147f), 1));
  fst::ArcSort(&self_loop_fst, fst::ILabelCompare<fst::StdArc>());
  EXPECT_EQ(internal::ScoreNGram(self_loop_fst, {2}, /*phi_label=*/0),
            fst::StdArc::Weight::Zero());
  fst::VectorFst<fst::StdArc> self_loop_merged;
  ASSERT_TRUE(LinearMerge(self_loop_fst, fst, 0.5, 0.5, &self_loop_merged,
                          /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(self_loop_merged, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(self_loop_merged, /*phi_label=*/0));
  fst::VectorFst<fst::StdArc> self_loop_bayes;
  ASSERT_TRUE(BayesMerge(self_loop_fst, fst, 0.5, 0.5, &self_loop_bayes,
                         /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(self_loop_bayes, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(self_loop_bayes, /*phi_label=*/0));
}

// Verifies that merging with the default `fst::kNoLabel` failure transition
// normalizes backoff weights identically to merging with `phi_label = 0`
// (exercising `kInternalPhiSentinel` in `BuildCanonicalFst`) and preserves the
// `fst::kILabelSorted` property bit on both outputs.
TEST(MergeTest, DefaultKNoLabelMatchesExplicitZeroPhiLabel) {
  fst::SymbolTable syms("SharedSymbols");
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  syms.AddSymbol("b", 2);
  syms.AddSymbol("c", 3);
  const std::string arpa1 =
      "\\data\\\n"
      "ngram 1=4\n"
      "ngram 2=2\n\n"
      "\\1-grams:\n"
      "-0.60206\t</s>\n"
      "-0.52288\ta\t-0.17609\n"
      "-0.69897\tb\n"
      "-0.60206\tc\n\n"
      "\\2-grams:\n"
      "-0.30103\ta b\n"
      "-0.60206\t<s> a\n\n"
      "\\end\\\n";
  const std::string arpa2 =
      "\\data\\\n"
      "ngram 1=4\n"
      "ngram 2=2\n\n"
      "\\1-grams:\n"
      "-0.52288\t</s>\n"
      "-0.69897\ta\n"
      "-0.52288\tb\t-0.22185\n"
      "-0.69897\tc\n\n"
      "\\2-grams:\n"
      "-0.39794\tb c\n"
      "-0.52288\t<s> b\n\n"
      "\\end\\\n";
  auto read_model = [&](const std::string& arpa, fst::StdArc::Label phi) {
    std::stringstream istrm(arpa);
    fst::VectorFst<fst::StdArc> fst;
    fst.SetInputSymbols(&syms);
    fst.SetOutputSymbols(&syms);
    EXPECT_TRUE(ReadArpa(istrm, &fst, phi));
    return fst;
  };
  const fst::VectorFst<fst::StdArc> kno1 = read_model(arpa1, fst::kNoLabel);
  const fst::VectorFst<fst::StdArc> kno2 = read_model(arpa2, fst::kNoLabel);
  const fst::VectorFst<fst::StdArc> zero1 = read_model(arpa1, /*phi_label=*/0);
  const fst::VectorFst<fst::StdArc> zero2 = read_model(arpa2, /*phi_label=*/0);
  const std::vector<std::pair<fst::StdArc::Label, fst::StdArc::Label>>
      kno_to_zero = {{fst::kNoLabel, 0}};
  fst::VectorFst<fst::StdArc> lin_kno;
  fst::VectorFst<fst::StdArc> lin_zero;
  ASSERT_TRUE(LinearMerge(kno1, kno2, 0.4, 0.6, &lin_kno));
  ASSERT_TRUE(LinearMerge(zero1, zero2, 0.4, 0.6, &lin_zero, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(lin_kno, fst::kNoLabel));
  EXPECT_EQ(lin_kno.Properties(fst::kILabelSorted, false), fst::kILabelSorted);
  EXPECT_EQ(lin_zero.Properties(fst::kILabelSorted, false), fst::kILabelSorted);
  fst::Relabel(&lin_kno, kno_to_zero, kno_to_zero);
  fst::ArcSort(&lin_kno, fst::ILabelCompare<fst::StdArc>());
  EXPECT_TRUE(IsNormalized(lin_kno, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(lin_zero, /*phi_label=*/0));
  EXPECT_TRUE(fst::Equal(lin_kno, lin_zero));
  fst::VectorFst<fst::StdArc> bayes_kno;
  fst::VectorFst<fst::StdArc> bayes_zero;
  ASSERT_TRUE(BayesMerge(kno1, kno2, 0.4, 0.6, &bayes_kno));
  ASSERT_TRUE(BayesMerge(zero1, zero2, 0.4, 0.6, &bayes_zero, /*phi_label=*/0));
  EXPECT_TRUE(IsCanonical(bayes_kno, fst::kNoLabel));
  EXPECT_EQ(bayes_kno.Properties(fst::kILabelSorted, false),
            fst::kILabelSorted);
  EXPECT_EQ(bayes_zero.Properties(fst::kILabelSorted, false),
            fst::kILabelSorted);
  fst::Relabel(&bayes_kno, kno_to_zero, kno_to_zero);
  fst::ArcSort(&bayes_kno, fst::ILabelCompare<fst::StdArc>());
  EXPECT_TRUE(IsNormalized(bayes_kno, /*phi_label=*/0));
  EXPECT_TRUE(IsNormalized(bayes_zero, /*phi_label=*/0));
  EXPECT_TRUE(fst::Equal(bayes_kno, bayes_zero));
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
