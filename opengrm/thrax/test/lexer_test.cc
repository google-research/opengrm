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

#include "opengrm/thrax/lexer.h"

#include <string>

#include "opengrm/compat/file.h"

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace thrax {
namespace {

class LexerTest : public ::testing::Test {
 protected:
  void Run(const std::string& name) {
    std::string grm;
    ABSL_ASSERT_OK(file::ReadFileToString(fst::JoinPath(
            std::string("."),
            "opengrm/thrax/test/testdata/compilation",
            absl::StrCat(name, ".grm")), &grm));

    Lexer lexer;
    lexer.ScanString(grm);

    while (lexer.YYLex() != Lexer::EOS) {
      // No token can be empty in the source code.
      ASSERT_LT(lexer.YYBeginPos(), lexer.YYEndPos());
      std::string str =
          grm.substr(lexer.YYBeginPos(), lexer.YYEndPos() - lexer.YYBeginPos());
      if (str.front() == '"' || str.front() == '\'') {
        ASSERT_GE(str.size(), 2);
        ASSERT_EQ(str.front(), str.back());
        str = Unescape(str);
      } else if (str.front() == '<') {
        ASSERT_EQ('>', str.back());
        str = str.substr(1, str.size() - 2);
      }
      EXPECT_EQ(lexer.YYString(), str);
    }
  }

 private:
  // Mimics the behavior of quote string unescaping in YYLex().
  std::string Unescape(absl::string_view quoted) {
    DLOG(INFO) << "before unescaping: \"" << absl::Utf8SafeCEscape(quoted)
               << "\"";
    char terminator = quoted.front();
    std::string result;
    int at = 1;
    while (at < quoted.size() - 1) {
      if (quoted[at] == '\\') {
        ++at;
        CHECK_LT(at, quoted.size() - 1);
        if (quoted[at] != terminator) result.push_back('\\');
      }
      result.push_back(quoted[at]);
      ++at;
    }
    DLOG(INFO) << "after unescaping: \"" << absl::Utf8SafeCEscape(result)
               << "\"";
    return result;
  }
};

TEST_F(LexerTest, PosBasic) { Run("basic"); }

TEST_F(LexerTest, PosAst) { Run("ast"); }

TEST_F(LexerTest, PosAdvanced) { Run("advanced"); }

TEST_F(LexerTest, PosImport) { Run("import"); }

TEST_F(LexerTest, PosStringBoundaries) { Run("stringboundaries"); }

}  // namespace
}  // namespace thrax
