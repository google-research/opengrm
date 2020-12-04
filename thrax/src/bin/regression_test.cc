// Copyright 2005-2020 Google LLC
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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "fst/arc.h"
#include "fst/compat.h"
#include "fst/compose.h"
#include "fst/fst.h"
#include "fst/script/getters.h"
#include "fst/string.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "gtest/gtest.h"
#include "thrax/compat/compat.h"
#include "thrax/compat/utils.h"
#include "thrax/grm-manager.h"
#include "thrax/symbols.h"
#include "utildefs.h"

// Testdata is presented in test_file, one test case per line.
//
// A test case is either in the 3-column form:
//
//   RULE FLAGS_delimiter input_string FLAGS_delimiter output_string
//
// or in the 2-column form:
//
//   RULE FLAGS_delimiter input_string
//
// The 3-column form asserts that a rewrite of input_string with RULE is
// successful and produces the specified output_string.
//
// The 2-column form asserts that a rewrite of input_string with RULE is
// unsuccessful.
//
// "#" is the comment character. Lines starting with the comment character, as
// well as empty lines, are ignored.
//
// If FLAGS_allow_spaced_quoted_format is true, will also accept test lines of
// the format without giving whitespace errors:
//
//   RULE  FLAGS_delimiter "input_str"  FLAGS_delimiter "output_str"
//   RULE  FLAGS_delimiter "input_str"

ABSL_FLAG(std::string, far_file, "",
          "Location of compiled far for grammar relative to google3.");
ABSL_FLAG(std::string, test_file, "",
          "Location of test data relative to google3.");
ABSL_FLAG(std::string, delimiter, "\t", "The delimiter for the test line.");
ABSL_FLAG(bool, allow_spaced_quoted_format, false,
          "If true, expects the file to contain: "
          "RULE_NAME delim \"input\" delim \"output\"; "
          "the quotes will be removed before testing; "
          "if no quotes found, will remove extra whitespace in "
          "both input and output");
ABSL_FLAG(bool, covering_grammar, false,
          "If true, the grammar is a covering grammar which means that "
          "we only require that the expected string be in the output. "
          "If this is set, require_exactly_one_top_rewrite is otiose.");
ABSL_FLAG(bool, require_exactly_one_top_rewrite, true,
          "If true, requires that the grammar outputs exactly one rewrite "
          "costing min(cost_of_all_possible_rewrites). For example, would "
          "fail if given <'foo', 1> and <'bar', 1>, since these have the "
          "same cost, but would not fail for <'foo', 1> and <'bar', 2>."
          "Setting this to false is usually a bad idea, since this testing "
          "framework does not deal with n-best rewrites; use a custom test "
          "instead.");
ABSL_FLAG(std::string, token_type, "byte",
          "String Token Type, one of \"byte\", \"utf8\", or \"symbol\"");

namespace thrax {
namespace {

using ::fst::Compose;
using ::fst::StdArc;
using ::fst::StdVectorFst;
using ::fst::StringCompiler;
using ::fst::SymbolTable;
using ::fst::TokenType;
using ::fst::script::GetTokenType;

std::unique_ptr<GrmManagerSpec<StdArc>> InitGrmManager() {
  auto result = absl::make_unique<GrmManagerSpec<StdArc>>();
  CHECK(result->LoadArchive(absl::GetFlag(FLAGS_far_file)));
  return result;
}

TokenType InitTokenType() {
  const auto &token_type_string = absl::GetFlag(FLAGS_token_type);
  TokenType token_type;
  if (!GetTokenType(token_type_string, &token_type)) {
    LOG(FATAL) << token_type_string << "is not a valid TokenType.";
  }
  return token_type;
}

struct RegressionTestEnv {
  RegressionTestEnv()
      : grm(InitGrmManager()),
        generated_symtab(GetGeneratedSymbolTable(grm.get())),
        compiler(absl::make_unique<StringCompiler<StdArc>>(InitTokenType())) {}
  std::unique_ptr<GrmManagerSpec<StdArc>> grm;
  std::unique_ptr<const SymbolTable> generated_symtab;
  std::unique_ptr<StringCompiler<StdArc>> compiler;
};

class RegressionTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { env_ = new RegressionTestEnv(); }
  static void TearDownTestSuite() { delete env_; }
  static const RegressionTestEnv *env_;
};

const RegressionTestEnv *RegressionTest::env_ = nullptr;

class RegressionTestImpl : public RegressionTest {
 public:
  RegressionTestImpl(absl::string_view test_line, std::size_t line_number)
      : RegressionTest(), test_line_(test_line), line_number_(line_number) {}

  void TestBody() override {
    std::vector<std::string> pieces = absl::StrSplit(
        test_line_, absl::ByAnyChar(absl::GetFlag(FLAGS_delimiter)),
        absl::SkipEmpty());
    ASSERT_GE(pieces.size(), 2) << "Line " << line_number_
                                << " has fewer than two fields: " << test_line_;
    ASSERT_LE(pieces.size(), 3)
        << "Line " << line_number_
        << " has more than three fields: " << test_line_;
    auto &rule_list = pieces[0];
    auto &input = pieces[1];
    std::string expected = pieces.size() > 2 ? pieces[2] : "";
    if (absl::GetFlag(FLAGS_allow_spaced_quoted_format)) {
      ProcessForSpacedQuotedFormat(&rule_list, &input, &expected);
    }
    const auto rules = absl::StrSplit(rule_list, ',', absl::SkipEmpty());
    StdVectorFst input_fst;
    StdVectorFst output_fst;
    bool succeeded = true;
    if (!(*env_->compiler)(input, &input_fst)) {
      succeeded = false;
      EXPECT_TRUE(succeeded) << "[line " << line_number_ << "] "
                             << "Unable to parse input string" << std::endl;
      return;
    }
    for (const auto &rule : rules) {
      const std::vector<std::string> rule_bits = absl::StrSplit(rule, '$');
      std::string pdt_parens_rule;
      std::string mpdt_assignments_rule;
      if (rule_bits.size() >= 2) pdt_parens_rule = rule_bits[1];
      if (rule_bits.size() == 3) mpdt_assignments_rule = rule_bits[2];
      // Ensures the rule actually exists within the archive.
      ASSERT_TRUE(env_->grm->GetFst(rule_bits[0]) != nullptr)
          << "[line " << line_number_ << "] Rule '" << rule_bits[0]
          << "' not found in .far archive";
      if (env_->grm->Rewrite(rule_bits[0], input_fst, &output_fst,
                             pdt_parens_rule, mpdt_assignments_rule)) {
        input_fst = output_fst;
      } else {
        succeeded = false;
        break;
      }
    }
    if (absl::GetFlag(FLAGS_covering_grammar)) {
      if (expected.empty()) {
        // If expected is empty the expectation is that there will be no output
        // at all for the given input. (I cannot think of a more reasonable
        // interpretation of that situation.) So if there is output, that's an
        // error. We just copy the behavior from below.
        if (output_fst.NumStates() != 0) {
          EXPECT_FALSE(succeeded)
              << "[line " << line_number_ << "] "
              << "Rewriting succeeded unexpectedly for" << std::endl
              << "    RULE: \"" << rule_list << "\"" << std::endl
              << "   INPUT: \"" << input << "\"" << std::endl;
        }
        return;  // No need to do any of the stuff below.
      }
      StdVectorFst expected_fst;
      if (!(*env_->compiler)(expected, &expected_fst)) {
        succeeded = false;
        EXPECT_TRUE(succeeded)
            << "[line " << line_number_ << "] "
            << "--covering_grammar is set but "
            << "unable to parse expected string " << std::endl;
        return;
      }
      StdVectorFst compose_fst;
      Compose(output_fst, expected_fst, &compose_fst);
      output_fst = compose_fst;
      if (output_fst.NumStates() == 0) {
        succeeded = false;
        EXPECT_TRUE(succeeded) << "[line " << line_number_ << "] "
                               << "--covering_grammar is set  "
                               << "but expected is not contained "
                               << "in the output." << std::endl;
      }
      return;  // No need to do any of the stuff below.
    }
    // Asks for top-2 rewrites if we want to check for exactly one top rewrite.
    const int n = absl::GetFlag(FLAGS_require_exactly_one_top_rewrite) ? 2 : 1;
    std::vector<std::pair<std::string, float>> strings;
    strings.reserve(n);
    std::string output;
    if (FstToStrings(output_fst, &strings, env_->generated_symtab.get(),
                     InitTokenType(), nullptr, n)) {
      const auto &top_rewrite_string = strings[0].first;
      const auto top_rewrite_cost = strings[0].second;
      if (absl::GetFlag(FLAGS_require_exactly_one_top_rewrite) &&
          strings.size() > 1) {
        const auto &tied_rewrite_string = strings[1].first;
        EXPECT_GT(strings[1].second - top_rewrite_cost, 1e-4)
            << "[line " << line_number_ << "] "
            << "Multiple top rewrites found for" << std::endl
            << std::endl
            << "    RULE: \"" << rule_list << "\"" << std::endl
            << "   INPUT: \"" << input << "\"" << std::endl
            << std::endl
            << "Top rewrite is '" << top_rewrite_string << "' with a cost of "
            << top_rewrite_cost << " but another rewrite exists with the same "
            << "cost: '" << tied_rewrite_string << "', which is disallowed "
            << "since --require_exactly_one_top_rewrite is set to true. This "
            << "is undesirable anytime one expects the grammar to produce "
            << "exactly one top rewrite, because minor implementation details "
            << "may cause output to shift unexpectedly.";
      }
      output = top_rewrite_string;
    } else {
      succeeded = false;
    }
    if (!expected.empty()) {
      EXPECT_TRUE(succeeded) << "[line " << line_number_ << "] "
                             << "Rewriting failed for" << std::endl
                             << "    RULE: \"" << rule_list << "\"" << std::endl
                             << "   INPUT: \"" << input << "\"" << std::endl;
      if (succeeded) {
        // EXPECT_EQ spits out non-UTF encoded output into the terminal, so we
        // write our own message.
        EXPECT_TRUE(expected == output)
            << "[line " << line_number_ << "] "
            << "Unexpected rewrite produced for " << std::endl
            << "    RULE: \"" << rule_list << "\"" << std::endl
            << "   INPUT: \"" << input << "\"" << std::endl
            << "EXPECTED: \"" << expected << "\"" << std::endl
            << "  ACTUAL: \"" << output << "\"" << std::endl
            << "Replace this test with" << std::endl
            << rule_list << "\t" << input << "\t" << output << std::endl;
      }
    } else {
      EXPECT_FALSE(succeeded)
          << "[line " << line_number_ << "] "
          << "Rewriting succeeded unexpectedly for" << std::endl
          << "    RULE: \"" << rule_list << "\"" << std::endl
          << "   INPUT: \"" << input << "\"" << std::endl;
    }
  }

  void ProcessSpacedQuotedString(std::string *input) {
    absl::StripTrailingAsciiWhitespace(input);
    absl::StripLeadingAsciiWhitespace(input);
    if (!input->empty() && (*input)[0] == '\"' &&
        (*input)[input->size() - 1] == '\"') {
      input->erase(input->begin());
      if (!input->empty()) input->erase(input->size() - 1);
    }
  }

  void ProcessForSpacedQuotedFormat(std::string *rule, std::string *input,
                                    std::string *expected) {
    absl::RemoveExtraAsciiWhitespace(rule);
    ProcessSpacedQuotedString(input);
    ProcessSpacedQuotedString(expected);
  }

  absl::string_view test_line_;
  std::size_t line_number_;
};

void RegisterTests(const std::string &test_file_path,
                   const std::string &test_file_contents) {
  std::vector<absl::string_view> lines =
      absl::StrSplit(test_file_contents, '\n');
  for (int i = 0; i < lines.size(); ++i) {
    absl::string_view line = lines[i];
    const int line_num = i + 1;
    if (line.empty() || line[0] == '#') continue;
    ::testing::RegisterTest("RegressionTest",
                            absl::StrCat(test_file_path, ":", line_num).c_str(),
                            /*type_param=*/nullptr, /*value_param=*/nullptr,
                            __FILE__, __LINE__, [=]() -> RegressionTest * {
                              return new RegressionTestImpl(line, line_num);
                            });
  }
}

}  // namespace
}  // namespace thrax

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  const std::string test_file_path = absl::GetFlag(FLAGS_test_file);
  std::string test_file_contents;
  thrax::ReadFileToStringOrDie(test_file_path, &test_file_contents);
  thrax::RegisterTests(absl::GetFlag(FLAGS_test_file), test_file_contents);
  return RUN_ALL_TESTS();
}
