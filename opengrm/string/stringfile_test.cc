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

#include "opengrm/string/stringfile.h"

#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"

namespace fst {
namespace internal {
namespace {

class StringFileTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string testdir = fst::JoinPath(
        std::string("."), "opengrm/string/testdata");

    map_name_ = fst::JoinPath(testdir, "str.map");
    empty_map_name_ = fst::JoinPath(testdir, "str_empty.map");
    no_newline_map_name_ = fst::JoinPath(testdir, "str_no_newline.map");
  }

  std::string map_name_;
  std::string empty_map_name_;
  std::string no_newline_map_name_;
};

TEST_F(StringFileTest, TestNonexistentFile) {
  StringFile sf("*nonexistent*");
  ASSERT_TRUE(sf.Error());
  EXPECT_TRUE(sf.Done());
}

TEST_F(StringFileTest, StringFileTestEmptyFile) {
  StringFile sf(empty_map_name_);
  ASSERT_FALSE(sf.Error());
  ASSERT_TRUE(sf.Done());
}

TEST_F(StringFileTest, StringFileTest) {
  StringFile sf(map_name_);
  ASSERT_FALSE(sf.Error());
  ASSERT_FALSE(sf.Done());
  // First line is a comment.
  EXPECT_EQ(2, sf.LineNumber());
  EXPECT_EQ("[Bel Paese]\tSorry", sf.GetString());
  sf.Next();
  ASSERT_FALSE(sf.Done());
  // Third line is empty.
  EXPECT_EQ(4, sf.LineNumber());
  EXPECT_EQ("Cheddar", sf.GetString());
  sf.Next();
  ASSERT_FALSE(sf.Done());
  EXPECT_EQ(5, sf.LineNumber());
  EXPECT_EQ("Caithness\tPont-l'Évêque\t.666", sf.GetString());
  sf.Next();
  ASSERT_FALSE(sf.Done());
  EXPECT_EQ(6, sf.LineNumber());
  EXPECT_EQ("Pont-l'Évêque\tCamembert", sf.GetString());
  sf.Next();
  ASSERT_TRUE(sf.Done());
}

TEST_F(StringFileTest, StringFileTestNoNewline) {
  StringFile sf(no_newline_map_name_);
  ASSERT_FALSE(sf.Error());
  ASSERT_FALSE(sf.Done());
  EXPECT_EQ(1, sf.LineNumber());
  EXPECT_EQ("first", sf.GetString());
  sf.Next();
  ASSERT_FALSE(sf.Done());
  EXPECT_EQ(2, sf.LineNumber());
  EXPECT_EQ("second\tsegundo", sf.GetString());
  sf.Next();
  ASSERT_FALSE(sf.Done());
  EXPECT_EQ(3, sf.LineNumber());
  EXPECT_EQ("third\ttercero\t-3", sf.GetString());
  sf.Next();
  ASSERT_TRUE(sf.Done());
}

TEST_F(StringFileTest, ColumnStringFileTest) {
  ColumnStringFile csf(map_name_);
  ASSERT_FALSE(csf.Error());
  EXPECT_FALSE(csf.Done());
  {
    const auto& row = csf.Row();
    ASSERT_EQ(2, row.size());
    EXPECT_EQ("[Bel Paese]", row[0]);
    EXPECT_EQ("Sorry", row[1]);
  }
  csf.Next();
  ASSERT_FALSE(csf.Done());
  {
    const auto& row = csf.Row();
    EXPECT_EQ(1, row.size());
    EXPECT_EQ("Cheddar", row[0]);
  }
  csf.Next();
  ASSERT_FALSE(csf.Done());
  {
    const auto& row = csf.Row();
    EXPECT_EQ(3, row.size());
    EXPECT_EQ("Caithness", row[0]);
    EXPECT_EQ("Pont-l'Évêque", row[1]);
    EXPECT_EQ(".666", row[2]);
  }
  csf.Next();
  ASSERT_FALSE(csf.Done());
  csf.Next();
  ASSERT_TRUE(csf.Done());
}

TEST_F(StringFileTest, ColumnStringFileTestNoNewline) {
  ColumnStringFile csf(no_newline_map_name_);
  ASSERT_FALSE(csf.Error());
  EXPECT_FALSE(csf.Done());
  {
    const auto& row = csf.Row();
    ASSERT_EQ(1, row.size());
    EXPECT_EQ("first", row[0]);
  }
  csf.Next();
  ASSERT_FALSE(csf.Done());
  {
    const auto& row = csf.Row();
    EXPECT_EQ(2, row.size());
    EXPECT_EQ("second", row[0]);
    EXPECT_EQ("segundo", row[1]);
  }
  csf.Next();
  ASSERT_FALSE(csf.Done());
  {
    const auto& row = csf.Row();
    EXPECT_EQ(3, row.size());
    EXPECT_EQ("third", row[0]);
    EXPECT_EQ("tercero", row[1]);
    EXPECT_EQ("-3", row[2]);
  }
  csf.Next();
  ASSERT_TRUE(csf.Done());
}

TEST_F(StringFileTest, ColumnStringFileTestEmptyFile) {
  ColumnStringFile csf(empty_map_name_);
  ASSERT_FALSE(csf.Error());
  ASSERT_TRUE(csf.Done());
}

}  // namespace
}  // namespace internal
}  // namespace fst
