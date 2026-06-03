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

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/grm-manager.h"
#include "opengrm/thrax/test/total-sort.h"

namespace thrax {
namespace {

using ::fst::StdArc;
using ::fst::StringCompiler;
using ::fst::StringPrinter;
using ::fst::SymbolTable;
using ::fst::TokenType;

using MutableTransducer = ::fst::VectorFst<StdArc>;

class BasicRegressionTest : public ::testing::Test {
 protected:
  BasicRegressionTest() {
    base_test_path_ = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/regression");

    far_path_ = fst::JoinPath(base_test_path_, "basic.far");

    symtab_ = absl::WrapUnique(
        SymbolTable::ReadText(fst::JoinPath(base_test_path_, "symtab")));
    CHECK(symtab_ != nullptr);

    symtab_compiler_ = std::make_unique<StringCompiler<StdArc>>(
        TokenType::SYMBOL, symtab_.get());
    symtab_printer_ = std::make_unique<StringPrinter<StdArc>>(TokenType::SYMBOL,
                                                              symtab_.get());

    grm_ = std::make_unique<GrmManagerSpec<StdArc>>();
    CHECK(grm_->LoadArchive(far_path_));
  }

  void TestFstsEqual(absl::string_view fn1, absl::string_view fn2) {
    auto generated = absl::WrapUnique(MutableTransducer::Read(fn1));
    auto golden = absl::WrapUnique(MutableTransducer::Read(fn2));

    ASSERT_TRUE(generated != nullptr);
    ASSERT_TRUE(golden != nullptr);
    EXPECT_TRUE(EqualUnordered(*generated, *golden));
  }

  std::string base_test_path_;
  std::string far_path_;
  std::unique_ptr<GrmManagerSpec<StdArc>> grm_;
  std::unique_ptr<SymbolTable> symtab_;
  std::unique_ptr<StringCompiler<StdArc>> symtab_compiler_;
  std::unique_ptr<StringPrinter<StdArc>> symtab_printer_;
};

TEST_F(BasicRegressionTest, BasicTest) {
  std::string output;

  // "b"
  EXPECT_TRUE(grm_->RewriteBytes("b1", "b", &output));
  EXPECT_EQ("b", output);
  EXPECT_FALSE(grm_->RewriteBytes("b1", "q", &output));

  // "b"{2,4}
  EXPECT_TRUE(grm_->RewriteBytes("b24", "bb", &output));
  EXPECT_EQ("bb", output);
  EXPECT_TRUE(grm_->RewriteBytes("b24", "bbb", &output));
  EXPECT_EQ("bbb", output);
  EXPECT_TRUE(grm_->RewriteBytes("b24", "bbbb", &output));
  EXPECT_EQ("bbbb", output);
  EXPECT_FALSE(grm_->RewriteBytes("b24", "b", &output));
  EXPECT_FALSE(grm_->RewriteBytes("b24", "bbbbb", &output));

  // Clean["b"{2,4}]
  EXPECT_TRUE(grm_->RewriteBytes("b24_clean", "bb", &output));
  EXPECT_EQ("bb", output);
  EXPECT_TRUE(grm_->RewriteBytes("b24_clean", "bbb", &output));
  EXPECT_EQ("bbb", output);
  EXPECT_TRUE(grm_->RewriteBytes("b24_clean", "bbbb", &output));
  EXPECT_EQ("bbbb", output);
  EXPECT_FALSE(grm_->RewriteBytes("b24_clean", "b", &output));
  EXPECT_FALSE(grm_->RewriteBytes("b24_clean", "bbbbb", &output));

  // "a" : "b"
  EXPECT_TRUE(grm_->RewriteBytes("a_to_b", "a", &output));
  EXPECT_EQ("b", output);
}

TEST_F(BasicRegressionTest, SymbolTableTest) {
  std::string output;

  // "fairy bears" : "polar bears"
  MutableTransducer fairy_bears;
  MutableTransducer polar_bears;
  (*symtab_compiler_)("fairy bears", &fairy_bears);
  ASSERT_TRUE(grm_->Rewrite("fb_to_pb", fairy_bears, &polar_bears));
  GrmManagerSpec<StdArc>::StringifyFst(&polar_bears);
  (*symtab_printer_)(polar_bears, &output);
  EXPECT_EQ("polar bears", output);
}

TEST_F(BasicRegressionTest, BasicOperationsTest) {
  std::string output;

  // Concatenation
  EXPECT_TRUE(grm_->RewriteBytes("concat", "ab", &output));
  EXPECT_FALSE(grm_->RewriteBytes("concat", "", &output));
  EXPECT_FALSE(grm_->RewriteBytes("concat", "a", &output));
  EXPECT_FALSE(grm_->RewriteBytes("concat", "b", &output));
  EXPECT_FALSE(grm_->RewriteBytes("concat", "abab", &output));

  // Union
  EXPECT_TRUE(grm_->RewriteBytes("union", "a", &output));
  EXPECT_TRUE(grm_->RewriteBytes("union", "b", &output));
  EXPECT_FALSE(grm_->RewriteBytes("union", "", &output));
  EXPECT_FALSE(grm_->RewriteBytes("union", "ab", &output));

  // Difference
  EXPECT_TRUE(grm_->RewriteBytes("diff", "a", &output));
  EXPECT_FALSE(grm_->RewriteBytes("diff", "", &output));
  EXPECT_FALSE(grm_->RewriteBytes("diff", "b", &output));
  EXPECT_FALSE(grm_->RewriteBytes("diff", "ab", &output));

  // Repetition - Plus
  EXPECT_TRUE(grm_->RewriteBytes("rep_plus", "a", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_plus", "aa", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_plus", "aaa", &output));
  EXPECT_FALSE(grm_->RewriteBytes("rep_plus", "", &output));

  // Repetition - Star
  EXPECT_TRUE(grm_->RewriteBytes("rep_star", "", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_star", "a", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_star", "aa", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_star", "aaa", &output));

  // Repetition - Interval
  EXPECT_TRUE(grm_->RewriteBytes("rep_int", "a", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_int", "aa", &output));
  EXPECT_FALSE(grm_->RewriteBytes("rep_int", "", &output));
  EXPECT_FALSE(grm_->RewriteBytes("rep_int", "aaa", &output));

  // Repetition - Question
  EXPECT_TRUE(grm_->RewriteBytes("rep_question", "", &output));
  EXPECT_TRUE(grm_->RewriteBytes("rep_question", "a", &output));
  EXPECT_FALSE(grm_->RewriteBytes("rep_question", "aa", &output));
  EXPECT_FALSE(grm_->RewriteBytes("rep_question", "aaa", &output));

  // Composition
  EXPECT_TRUE(grm_->RewriteBytes("comp", "a", &output));
  EXPECT_EQ("b", output);
  EXPECT_FALSE(grm_->RewriteBytes("comp", "", &output));
  EXPECT_FALSE(grm_->RewriteBytes("comp", "b", &output));
}

TEST_F(BasicRegressionTest, BogusRulesTest) {
  std::string output;

  EXPECT_FALSE(grm_->RewriteBytes("a1", "a", &output));      // Unexported.
  EXPECT_FALSE(grm_->RewriteBytes("cookie", "a", &output));  // Does not exist.
}

TEST_F(BasicRegressionTest, EscapedSymbolsTest) {
  std::string output;

  EXPECT_TRUE(grm_->RewriteBytes("escaped_symbols", R"("a"\b")", &output));
  EXPECT_FALSE(grm_->RewriteBytes("escaped_symbols", "ab", &output));
  EXPECT_FALSE(grm_->RewriteBytes("escaped_symbols", "a\"b", &output));

  EXPECT_TRUE(grm_->RewriteBytes("escaped_symbols_2", "a\\b\\", &output));
  EXPECT_FALSE(grm_->RewriteBytes("escaped_symbols_2", "a\\b\\\"", &output));
  EXPECT_FALSE(grm_->RewriteBytes("escaped_symbols_2", "a\\b\"", &output));
}

TEST_F(BasicRegressionTest, CheckExtractedFst_concat) {
  TestFstsEqual(fst::JoinPath(base_test_path_, "concat.fst"),
                fst::JoinPath(base_test_path_, "golden/concat.fst"));
}

TEST_F(BasicRegressionTest, CheckExtractedFst_union) {
  TestFstsEqual(fst::JoinPath(base_test_path_, "union.fst"),
                fst::JoinPath(base_test_path_, "golden/union.fst"));
}

TEST_F(BasicRegressionTest, MinusSignAndSpace) {
  std::string output;
  EXPECT_TRUE(grm_->RewriteBytes("minus_sign_and_space", "a", &output));
  EXPECT_FALSE(grm_->RewriteBytes("minus_sign_and_space", "b", &output));
}

}  // namespace
}  // namespace thrax
