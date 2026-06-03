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
#include "openfst/lib/arc.h"
#include "opengrm/thrax/grm-compiler.h"
#include "opengrm/thrax/grm-manager.h"

namespace thrax {
namespace {

using ::fst::StdArc;

class LabelsTest : public ::testing::Test {
 protected:
  LabelsTest() {
    far_dir_ = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/labels");
  }

  virtual void TearDown() { grm_ = nullptr; }

  void LoadGrammar(const std::string& filename) {
    std::string path = fst::JoinPath(far_dir_, filename);
    grm_ = std::make_unique<GrmManagerSpec<StdArc>>();
    ASSERT_TRUE(grm_->LoadArchive(path));
  }

  std::unique_ptr<GrmManagerSpec<StdArc>> grm_;
  std::string far_dir_;
};

TEST_F(LabelsTest, TestSpaces) {
  LoadGrammar("test1.far");
  std::string output;
  EXPECT_TRUE(grm_->RewriteBytes("three_spaces", "   ", &output));
  EXPECT_EQ("   ", output);
}

TEST_F(LabelsTest, TestMatch) {
  LoadGrammar("test2.far");
  std::string output;
  EXPECT_TRUE(grm_->RewriteBytes("C_to_B", "C", &output));
  EXPECT_EQ("B", output);
}

// The purpose of this test is to make sure that symbols don't conflict when
// they're loaded separately in two different grammars (since the symbol map is
// static and thus shared). Note however that if grammars are compiled
// separately (i.e., some via LoadArchive() and some parse on the fly via
// Parse*()/EvaluateAst()) that there's no guarantee that the labels match up.
TEST_F(LabelsTest, TestDualLoad) {
  GrmCompilerSpec<StdArc> one, three;
  ASSERT_TRUE(one.ParseFile(fst::JoinPath(far_dir_, "test1.grm")));
  ASSERT_TRUE(one.EvaluateAst());
  ASSERT_TRUE(three.ParseFile(fst::JoinPath(far_dir_, "test3.grm")));
  ASSERT_TRUE(three.EvaluateAst());
}

}  // namespace
}  // namespace thrax
