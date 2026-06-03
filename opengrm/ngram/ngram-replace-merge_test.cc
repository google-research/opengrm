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

// Tests for replacement merging of NGramModels.

#include "opengrm/ngram/ngram-replace-merge.h"

#include <cmath>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "opengrm/ngram/ngram-make.h"

namespace ngram {

using ::fst::Equal;
using ::fst::StdArc;
using ::fst::StdFst;
using ::fst::StdILabelCompare;
using ::fst::StdVectorFst;
using ::fst::SymbolTable;
using ::fst::Verify;

class NGramReplaceMergeMethodsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Two count FSTs, both containing counts corresponding to strings "a b"
    // and "b a", with the second one additionally countaining counts
    // corresponding to string "c". Parts of the automaton structures will be
    // shared.  The third FST will have the unigrams from one and the bigrams
    // from the other.
    lm1_fst_.AddState();                     // Unigram state (index 0).
    lm1_fst_.SetStart(lm1_fst_.AddState());  // Start state (index 1).
    fst::TropicalWeight wz = fst::TropicalWeight::One();
    lm1_fst_.SetFinal(lm1_fst_.AddState(), wz);  // Bigram state "a" (index 2).
    lm1_fst_.SetFinal(lm1_fst_.AddState(), wz);  // Bigram state "b" (index 3).
    SymbolTable lm_syms;
    int bo_sym = lm_syms.AddSymbol("<epsilon>");
    int a_sym = lm_syms.AddSymbol("a");
    int b_sym = lm_syms.AddSymbol("b");
    fst::TropicalWeight w = fst::TropicalWeight(-log(2.0));
    lm1_fst_.AddArc(1, StdArc(a_sym, a_sym, wz, 2));   // bigram "<S> a", c=1.
    lm1_fst_.AddArc(1, StdArc(b_sym, b_sym, wz, 3));   // bigram "<S> b", c=1.
    lm1_fst_.AddArc(0, StdArc(a_sym, a_sym, w, 2));    // 1-gram "a", c=2.
    lm1_fst_.AddArc(0, StdArc(b_sym, b_sym, w, 3));    // 1-gram "b", c=2.
    lm1_fst_.AddArc(2, StdArc(b_sym, b_sym, wz, 3));   // bigram "a b", c=1.
    lm1_fst_.AddArc(3, StdArc(a_sym, a_sym, wz, 2));   // bigram "b a", c=1.
    lm1_fst_.AddArc(2, StdArc(bo_sym, bo_sym, w, 0));  // "a" backoff.
    lm1_fst_.AddArc(3, StdArc(bo_sym, bo_sym, w, 0));  // "b" backoff.
    lm2_fst_ = lm1_fst_;  // All the shared structure between lm1 and lm2.
    lm1_fst_.AddArc(1, StdArc(bo_sym, bo_sym, w, 0));  // <S> backoff.
    lm3_fst_ = lm1_fst_;      // All the shared structure between lm1 and lmM.
    lm1_fst_.SetFinal(0, w);  // unigram </S>.
    lm1_fst_.SetInputSymbols(&lm_syms);
    lm1_fst_.SetOutputSymbols(&lm_syms);
    ArcSort(&lm1_fst_, StdILabelCompare());
    NGramMakeModel(&lm1_fst_, "katz");  // Makes LM from these counts.
    int c_sym = lm_syms.AddSymbol("c");
    lm3_fst_.AddArc(0, StdArc(c_sym, c_sym, 0.0, 0));      // unigram "c".
    lm3_fst_.SetFinal(0, fst::TropicalWeight(-log(3.0)));  // unigram </S>.
    lm3_fst_.SetInputSymbols(&lm_syms);
    lm3_fst_.SetOutputSymbols(&lm_syms);
    ArcSort(&lm3_fst_, StdILabelCompare());
    NGramMakeModel(&lm3_fst_, "katz");  // Makes LM with extra unigrams.
    lm2_fst_.AddArc(1, StdArc(bo_sym, bo_sym, -log(3.0), 0));  // <S> backoff.
    lm2_fst_.SetFinal(lm2_fst_.AddState(), wz);  // Bigram state "c" (index 4).
    lm2_fst_.AddArc(0, StdArc(c_sym, c_sym, wz, 4));       // unigram "c".
    lm2_fst_.SetFinal(0, fst::TropicalWeight(-log(3.0)));  // unigram </S>.
    lm2_fst_.AddArc(1, StdArc(c_sym, c_sym, wz, 4));   // bigram "<S> c", c=1.
    lm2_fst_.AddArc(4, StdArc(bo_sym, bo_sym, w, 0));  // "c" backoff.
    lm2_fst_.SetInputSymbols(&lm_syms);
    lm2_fst_.SetOutputSymbols(&lm_syms);
    ArcSort(&lm2_fst_, StdILabelCompare());
    NGramMakeModel(&lm2_fst_, "katz");  // Makes LM with extra uni/bigrams.
  }

  StdVectorFst lm1_fst_;
  StdVectorFst lm2_fst_;
  StdVectorFst lm3_fst_;
};

// Checks new bayes merging.
TEST_F(NGramReplaceMergeMethodsTest, NGramReplaceMergeTest) {
  StdVectorFst merged_fst(lm1_fst_);
  NGramReplaceMerge ngrammerge(&merged_fst);
  ngrammerge.MergeNGramModels(lm2_fst_, /* max_replace_order = */ 1,
                              /* norm = */ true);
  Verify(merged_fst);
  ASSERT_TRUE(Equal(merged_fst, lm3_fst_));
}

}  // namespace ngram
