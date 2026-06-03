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

// Tests shrinking functionality of NGram models.

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/ngram-input.h"
#include "opengrm/ngram/ngram-list-prune.h"
#include "opengrm/ngram/ngram-mutable-model.h"
#include "opengrm/ngram/ngram-relentropy.h"
#include "opengrm/ngram/ngram-seymore-shrink.h"

namespace ngram {

constexpr uint32_t kKeepRelEntropy = 1;
constexpr uint32_t kKeepRelEntropyBigram = 2;
constexpr uint32_t kKeepSeymore = 4;
constexpr uint32_t kKeepSeymoreBigram = 8;

using ::fst::StdArc;
using ::fst::StdFst;
using ::fst::StdMutableFst;
using ::fst::StdVectorFst;

class NGramShrinkTest : public ::testing::Test {
 protected:
  void PushNGram(absl::string_view ngram, double prob, uint32_t keep) {
    ngrams_.push_back(std::string(ngram));
    probs_.push_back(prob);
    keep_prune_.push_back((keep & kKeepRelEntropy) == kKeepRelEntropy);
    keep_bigrams_prune_.push_back((keep & kKeepRelEntropyBigram) ==
                                  kKeepRelEntropyBigram);
    keep_seymore_.push_back((keep & kKeepSeymore) == kKeepSeymore);
    keep_bigrams_seymore_.push_back((keep & kKeepSeymoreBigram) ==
                                    kKeepSeymoreBigram);
  }

  void PushUnigram(absl::string_view unigram, double prob) {
    PushNGram(unigram, prob,
              kKeepRelEntropy | kKeepRelEntropyBigram | kKeepSeymore |
                  kKeepSeymoreBigram);
  }

  void PushBigram(absl::string_view bigram, double prob, uint32_t keep) {
    PushNGram(bigram, prob, keep | kKeepRelEntropyBigram | kKeepSeymoreBigram);
  }

  StdVectorFst UpdateFiles(absl::string_view input_string,
                           absl::string_view ngram_set) {
    // Writes ngrams to file and derives FST model from n-grams.
    const std::string file_path =
        fst::JoinPath(::testing::TempDir(), ngram_set);
    std::ofstream ofs(file_path);
    ofs << input_string;
    ofs.close();
    CHECK(!ofs.fail()) << "Failed to write string to " << file_path;

    std::ifstream ifstrm(file_path);
    ngram::NGramInput lm_ngram(ifstrm, std::cout, syms_file_, epsilon_sym_,
                               oov_sym_, start_symbol_, end_symbol_);
    lm_ngram.ReadInput(/*ARPA=*/false, /*symbols=*/false, /*output=*/false);
    std::unique_ptr<StdMutableFst> fst(lm_ngram.GetFst()->Copy());

    // Recalculates backoff and shrinks with a threshold that removes no
    // n-grams, which removes states corresponding to histories with no
    // continuations, only backoff, to make them Isomorphic with test results.
    ngram::NGramMutableModel<StdArc> ngram_mod(fst.get());
    ngram_mod.RecalcBackoff();
    NGramRelEntropy ngramsh(fst.get(),
                            /* theta = */ std::numeric_limits<double>::min());
    ngramsh.ShrinkNGramModel();
    return StdVectorFst(*ngramsh.GetMutableFst());
  }

  void SetUp() override {
    const std::string symbols =
        epsilon_sym_ + "\t0\na\t1\nb\t2\n" + oov_sym_ + "\t3\n";
    std::ofstream ofs(syms_file_);
    ofs << symbols;
    ofs.close();
    ASSERT_FALSE(ofs.fail()) << "Failed to write symbols to " << syms_file_;

    // Builds models from n-grams derived from string "b a b b b b b b b".
    // For each n-gram (in lexicographic order), stores string, probability
    // and whether it will be kept under various shrinking conditions.
    uint32_t keep_all_relentropy = kKeepRelEntropy | kKeepRelEntropyBigram;
    uint32_t keep_all_seymore = kKeepSeymore | kKeepSeymoreBigram;
    PushUnigram("a", 0.1);
    PushBigram("a b", 0.999, kKeepRelEntropy | kKeepSeymore);
    PushNGram("a b b", 0.5, kKeepRelEntropy);
    PushUnigram("b", 0.8);
    PushBigram("b a", 0.124875, kKeepRelEntropy | kKeepSeymore);
    PushNGram("b a b", 0.5, keep_all_relentropy);
    PushBigram("b b", 0.74925, kKeepRelEntropy | kKeepSeymore);
    PushNGram("b b b", 0.8325, kKeepSeymore);
    PushNGram("b b </S>", 0.08333333, uint32_t());
    PushBigram("b </S>", 0.125875, kKeepSeymore);
    PushUnigram("</S>", 0.1);
    PushUnigram("<S>", 0.1);
    PushBigram("<S> b", 0.999, kKeepRelEntropy | kKeepSeymore);
    PushNGram("<S> b a", 0.5, kKeepRelEntropy | keep_all_seymore);
    std::string all_ngrams;
    std::string kept_ngrams;
    std::string kept_seymore;
    std::string kept_bigram_ngrams;
    std::string kept_bigram_seymore;
    std::string kept_prune_a_list;
    for (int i = 0; i < ngrams_.size(); ++i) {
      absl::StrAppend(&all_ngrams, ngrams_[i], "\t", probs_[i], "\n");
      if (ngrams_[i] == "a" || !absl::StrContains(ngrams_[i], 'a')) {
        // No 'b' found in ngram, add to unpruned list.
        absl::StrAppend(&kept_prune_a_list, ngrams_[i], "\t", probs_[i], "\n");
      }
      if (keep_prune_[i]) {
        absl::StrAppend(&kept_ngrams, ngrams_[i], "\t", probs_[i], "\n");
      }
      if (keep_seymore_[i]) {
        absl::StrAppend(&kept_seymore, ngrams_[i], "\t", probs_[i], "\n");
      }
      if (keep_bigrams_prune_[i]) {
        absl::StrAppend(&kept_bigram_ngrams, ngrams_[i], "\t", probs_[i], "\n");
      }
      if (keep_bigrams_seymore_[i]) {
        absl::StrAppend(&kept_bigram_seymore, ngrams_[i], "\t", probs_[i],
                        "\n");
      }
    }
    ngram_fst_.reset(UpdateFiles(all_ngrams, "ngrams.txt").Copy());
    pruned_fst_.reset(UpdateFiles(kept_ngrams, "pruned.txt").Copy());
    seymore_fst_.reset(UpdateFiles(kept_seymore, "seymore.txt").Copy());
    keep_bigrams_pruned_fst_.reset(
        UpdateFiles(kept_bigram_ngrams, "keep_bigrams_pruned.txt").Copy());
    keep_bigrams_seymore_fst_.reset(
        UpdateFiles(kept_bigram_seymore, "keep_bigrams_seymore.txt").Copy());
    keep_prune_a_list_fst_.reset(
        UpdateFiles(kept_prune_a_list, "keep_prune_a_list.txt").Copy());
  }

  std::unique_ptr<StdMutableFst> ngram_fst_;
  std::unique_ptr<StdMutableFst> pruned_fst_;
  std::unique_ptr<StdMutableFst> seymore_fst_;
  std::unique_ptr<StdMutableFst> keep_bigrams_pruned_fst_;
  std::unique_ptr<StdMutableFst> keep_bigrams_seymore_fst_;
  std::unique_ptr<StdMutableFst> keep_prune_a_list_fst_;
  std::vector<std::string> ngrams_;  // n-gram string.
  std::vector<double> probs_;        // n-gram probability.
  std::vector<bool> keep_prune_;     // whether n-gram pruned standardly.
  std::vector<bool> keep_seymore_;   // whether n-gram pruned standardly using
                                     // Seymore-Rosenfeld.
  std::vector<bool> keep_bigrams_prune_;  // whether n-gram pruned when bigrams
                                          // are not pruned.
  std::vector<bool>
      keep_bigrams_seymore_;  // whether n-gram pruned when bigrams
                              // are not pruned, using Seymore-Rosenfeld.
  const std::string syms_file_ =
      fst::JoinPath(::testing::TempDir(), "ngram.syms");
  const std::string epsilon_sym_ = "<epsilon>";
  const std::string oov_sym_ = "<UNK>";
  const std::string start_symbol_ = "<S>";
  const std::string end_symbol_ = "</S>";
};

// Checks that relative entropy shrinking produces correct set of n-grams.
TEST_F(NGramShrinkTest, StandardRelEntropyShrinkTest) {
  std::unique_ptr<StdMutableFst> input_fst(ngram_fst_->Copy());
  NGramRelEntropy ngramsh(input_fst.get(), /* theta = */ 0.0);
  ngramsh.CalculateTheta(10);
  ngramsh.ShrinkNGramModel();
  ASSERT_TRUE(Isomorphic(*pruned_fst_, *input_fst));
}

// Checks that relative entropy shrinking produces correct set of n-grams,
// when bigrams are not pruned.
TEST_F(NGramShrinkTest, StandardRelEntropyKeepBigramShrinkTest) {
  std::unique_ptr<StdMutableFst> input_fst(ngram_fst_->Copy());
  NGramRelEntropy ngramsh(input_fst.get(), /* theta = */ 0.0);
  ngramsh.CalculateTheta(9, /* min_order = */ 3);
  ngramsh.ShrinkNGramModel(/* min_order = */ 3);
  ASSERT_TRUE(Isomorphic(*keep_bigrams_pruned_fst_, *input_fst));
}

// Checks that Seymore-Rosenfeld shrinking produces correct set of n-grams.
TEST_F(NGramShrinkTest, StandardSeymoreShrinkTest) {
  std::unique_ptr<StdMutableFst> input_fst(ngram_fst_->Copy());
  NGramSeymoreShrink ngramsh(input_fst.get(), /* theta = */ 0.0);
  ngramsh.CalculateTheta(10);
  ngramsh.ShrinkNGramModel();
  ASSERT_TRUE(Isomorphic(*seymore_fst_, *input_fst));
}

// Checks that Seymore-Rosenfeld shrinking produces correct set of n-grams,
// when bigrams are not pruned.
TEST_F(NGramShrinkTest, StandardSeymoreKeepBigramShrinkTest) {
  std::unique_ptr<StdMutableFst> input_fst(ngram_fst_->Copy());
  NGramSeymoreShrink ngramsh(input_fst.get(), /* theta = */ 0.0);
  ngramsh.CalculateTheta(9, /* min_order = */ 3);
  ngramsh.ShrinkNGramModel(/* min_order = */ 3);
  ASSERT_TRUE(Isomorphic(*keep_bigrams_seymore_fst_, *input_fst));
}

TEST_F(NGramShrinkTest, NGramListShrinkTest) {
  std::unique_ptr<StdMutableFst> input_fst(ngram_fst_->Copy());
  std::vector<std::string> ngram_prune_string{"a"};
  std::set<std::vector<fst::StdArc::Label>> ngram_list;
  ngram::GetNGramListToPrune(ngram_prune_string, input_fst->InputSymbols(),
                             &ngram_list);
  NGramListPrune ngramsh(input_fst.get(), ngram_list);
  ngramsh.ShrinkNGramModel();
  ASSERT_TRUE(Isomorphic(*keep_prune_a_list_fst_, *input_fst));
}

}  // namespace ngram
