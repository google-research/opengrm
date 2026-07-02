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

#include "opengrm/compat/file.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace file {
namespace {

class FileTest : public ::testing::Test {
 protected:
  std::string GetTempFilePath(const std::string& filename) {
    return fst::JoinPath(::testing::TempDir(), filename);
  }
};

TEST_F(FileTest, RecursivelyCreateDirAndReadable) {
  const std::string dir = GetTempFilePath("test_dir/nested");
  EXPECT_TRUE(RecursivelyCreateDir(dir));

  const std::string filepath = fst::JoinPath(dir, "test.txt");
  EXPECT_FALSE(Readable(filepath));

  std::unique_ptr<File> fp(Open(filepath, "w"));
  ASSERT_NE(fp, nullptr);
  ASSERT_NE(fp->Stream(), nullptr);
  *fp->Stream() << "Hello, world!\n";
  fp->Close();

  EXPECT_TRUE(Readable(filepath));
}

TEST_F(FileTest, ReadFileToStringStatus) {
  const std::string filepath = GetTempFilePath("read_status.txt");
  std::unique_ptr<File> fp(Open(filepath, "w"));
  ASSERT_NE(fp, nullptr);
  *fp->Stream() << "Test content";
  fp->Close();

  std::string content;
  EXPECT_TRUE(ReadFileToString(filepath, &content).ok());
  EXPECT_EQ(content, "Test content");

  std::string append_content = "Prefix: ";
  EXPECT_TRUE(ReadFileToString(filepath, &append_content).ok());
  EXPECT_EQ(append_content, "Prefix: Test content");

  std::string fail_content;
  EXPECT_FALSE(
      ReadFileToString(GetTempFilePath("nonexistent.txt"), &fail_content).ok());
}

TEST_F(FileTest, ReadFileToStringStatusOr) {
  const std::string filepath = GetTempFilePath("read_statusor.txt");
  std::unique_ptr<File> fp(Open(filepath, "w"));
  ASSERT_NE(fp, nullptr);
  *fp->Stream() << "StatusOr content";
  fp->Close();

  absl::StatusOr<std::string> content = ReadFileToString(filepath);
  ASSERT_TRUE(content.ok());
  EXPECT_EQ(*content, "StatusOr content");

  EXPECT_FALSE(ReadFileToString(GetTempFilePath("nonexistent.txt")).ok());
}

TEST_F(FileTest, InputBufferTest) {
  const std::string filepath = GetTempFilePath("lines.txt");
  std::unique_ptr<File> fp(Open(filepath, "w"));
  ASSERT_NE(fp, nullptr);
  *fp->Stream() << "Line 1\nLine 2\nLine 3";
  fp->Close();

  File* read_fp = Open(filepath, "r");
  ASSERT_NE(read_fp, nullptr);
  file::InputBuffer buffer(read_fp);

  std::string line;
  EXPECT_TRUE(buffer.ReadLine(&line));
  EXPECT_EQ(line, "Line 1");
  EXPECT_TRUE(buffer.ReadLine(&line));
  EXPECT_EQ(line, "Line 2");
  EXPECT_TRUE(buffer.ReadLine(&line));
  EXPECT_EQ(line, "Line 3");
  EXPECT_FALSE(buffer.ReadLine(&line));

  EXPECT_TRUE(buffer.CloseFile());
}

TEST_F(FileTest, OpenOrDieTest) {
  const std::string filepath = GetTempFilePath("open_or_die.txt");
  std::unique_ptr<File> fp(OpenOrDie(filepath, "w"));
  ASSERT_NE(fp, nullptr);
  fp->Close();

  EXPECT_DEATH(OpenOrDie("", "r"), "No file specified");
  EXPECT_DEATH(OpenOrDie(GetTempFilePath("missing_file.txt"), "r"),
               "Can't open file");
}

}  // namespace
}  // namespace file
