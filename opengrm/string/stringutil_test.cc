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

#include "opengrm/string/stringutil.h"

#include <string>

#include "gtest/gtest.h"

namespace fst {
namespace {

// Comment stripping.

TEST(StripComment, NoCommentTest) {
  const std::string input = "This line has\tno comment";
  EXPECT_EQ(input, StripCommentAndRemoveEscape(input));
}

TEST(StripComment, NoCommentTestWithTrailingWhitespace) {
  const std::string input =
      "This line has\tno comment and trailing whitespace    ";
  EXPECT_EQ(input, StripCommentAndRemoveEscape(input));
}

TEST(StripComment, CommentStartsLineTest) {
  const std::string input = "# This line is just a comment.";
  EXPECT_EQ("", StripCommentAndRemoveEscape(input));
}

TEST(StripComment, CommentStartsLineTestWithTrailingWhitespace) {
  const std::string input = "       # This line is just a comment.";
  EXPECT_EQ("", StripCommentAndRemoveEscape(input));
}

TEST(StripComment, CommentAfterLine) {
  const std::string input = "foo\tbar      # This line has a comment.";
  EXPECT_EQ("foo\tbar", StripCommentAndRemoveEscape(input));
}

TEST(StripComment, EscapedCommentChar) {
  const std::string input = "\\#\thash";
  EXPECT_EQ("#\thash", StripCommentAndRemoveEscape(input));
}

TEST(StripComment, EscapedCommentCharAndComment) {
  const std::string input = "\\#\thash # That was a real hash.";
  EXPECT_EQ("#\thash", StripCommentAndRemoveEscape(input));
}

TEST(StripComment, EscapedCommentCharAndAdjacentComment) {
  const std::string input = "Foo\\## That was a real hash.";
  EXPECT_EQ("Foo#", StripCommentAndRemoveEscape(input));
}

// Escaping.

TEST(StripComment, NoEscape) {
  const std::string input = "Clear";
  EXPECT_EQ(input, Escape(input));
}

TEST(StripComment, BracketEscape) {
  const std::string input = R"(This [has brackets])";
  EXPECT_EQ(R"(This \[has brackets\])", Escape(input));
}

TEST(StripComment, BackslashEscape) {
  const std::string input = R"(This \ has \ slashes)";
  EXPECT_EQ(R"(This \\ has \\ slashes)", Escape(input));
}

}  // namespace
}  // namespace fst
