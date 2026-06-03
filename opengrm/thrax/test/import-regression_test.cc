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
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "opengrm/thrax/grm-manager.h"

namespace thrax {
namespace {

using ::fst::StdArc;
using Transducer = ::fst::Fst<StdArc>;

class ImportRegressionTest : public ::testing::Test {
 protected:
  ImportRegressionTest() {
    far_path_ = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/regression/"
        "import.far");

    grm_ = std::make_unique<GrmManagerSpec<StdArc>>();
    CHECK(grm_->LoadArchive(far_path_));
  }

  std::string far_path_;
  std::unique_ptr<GrmManagerSpec<StdArc>> grm_;
};

TEST_F(ImportRegressionTest, TestFstsEqual) {
  const auto* test1 = grm_->GetFst("test1");
  const auto* test2 = grm_->GetFst("test2");
  const auto* test3 = grm_->GetFst("test3");
  const auto* test4 = grm_->GetFst("test4");

  EXPECT_TRUE(::fst::Equal(*test1, *test2));
  EXPECT_TRUE(::fst::Equal(*test2, *test3));
  EXPECT_TRUE(::fst::Equal(*test3, *test4));
}

}  // namespace
}  // namespace thrax
