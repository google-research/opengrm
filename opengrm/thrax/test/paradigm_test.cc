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

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "opengrm/compat/file.h"
#include "opengrm/compat/file.h"

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/strings/str_split.h"
#include "openfst/lib/arc.h"
#include "opengrm/thrax/grm-manager.h"

namespace thrax {
namespace {

using ::fst::StdArc;

class ParadigmTest : public ::testing::Test {
 protected:
  ParadigmTest() {
    far_path_ = fst::JoinPath(std::string("."),
                               "opengrm/thrax/test/"
                               "testdata/paradigm/paradigm.far");
    grm_ = std::make_unique<GrmManagerSpec<StdArc>>();
    CHECK(grm_->LoadArchive(far_path_));

    testfile_path_ = fst::JoinPath(std::string("."),
                                    "opengrm/thrax/test/"
                                    "testdata/paradigm/paradigm.txt");
  }

  std::string far_path_;
  std::unique_ptr<GrmManagerSpec<StdArc>> grm_;
  std::string testfile_path_;
};

TEST_F(ParadigmTest, FileTest) {
  file::InputBuffer testfile(file::OpenOrDie(testfile_path_, "r"));

  std::string line;
  while (testfile.ReadLine(&line)) {
    // Skip empty and comment lines.
    if (line.empty() || line[0] == '#') continue;

    std::vector<std::string> pieces =
        absl::StrSplit(line, '\t', absl::SkipEmpty());
    ASSERT_EQ(3, pieces.size());

    const auto& rule = pieces[0];
    const auto& input = pieces[1];
    const auto& expected = pieces[2];

    std::string output;
    bool success = grm_->RewriteBytes(rule, input, &output), equal = true;
    EXPECT_TRUE(success);

    if (success) {
      equal = expected == output;
      EXPECT_TRUE(equal);
    }

    if (!success || !equal) {
      std::cout << "    RULE: " << rule << std::endl;
      std::cout << "   INPUT: " << input << std::endl;
      std::cout << "EXPECTED: " << expected << std::endl;
      if (!equal) std::cout << "     GOT: " << output << std::endl;
      std::cout << "-------------------------------------------------------"
                << std::endl;
    }
  }

  testfile.CloseFile();
}

}  // namespace
}  // namespace thrax
