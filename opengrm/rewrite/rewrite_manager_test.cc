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

#include "opengrm/rewrite/rewrite_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "openfst/compat/status_macros.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"

namespace rewrite {
namespace {

using ::fst::STTableFarReader;
using ::fst::StdArc;
using ::fst::StdFst;

class RewriteManagerTest : public ::testing::Test {
 public:
  RewriteManagerTest()
      : far_path_(
            fst::JoinPath(std::string("."),
                           "opengrm/thrax/test/testdata",
                           "cap.sttable.far")) {}
  // Helpers.
  void TopRewrite(absl::string_view rule, absl::string_view input,
                  absl::string_view expected) {
    std::string actual;
    ASSERT_TRUE(manager_.TopRewrite(rule, input, &actual));
    EXPECT_EQ(expected, actual);
  }

  void Matches(absl::string_view rule, absl::string_view input,
               absl::string_view output) {
    EXPECT_TRUE(manager_.Matches(rule, input, output));
  }

  void FailedTopRewrite(absl::string_view rule, absl::string_view input) {
    std::string temp;
    EXPECT_FALSE(manager_.TopRewrite(rule, input, &temp));
  }

  void SetUpFromFar() { ABSL_ASSERT_OK(manager_.LoadWithStatus(far_path_)); }

  absl::flat_hash_map<std::string, std::unique_ptr<const StdFst>> ReadFstMap(
      absl_nonnull std::unique_ptr<STTableFarReader<StdArc>> far_reader) {
    absl::flat_hash_map<std::string, std::unique_ptr<const StdFst>> fst_map;
    for (; !far_reader->Done(); far_reader->Next()) {
      const auto& key = far_reader->GetKey();
      const auto* fst = far_reader->GetFst();
      fst_map[key] = absl::WrapUnique(fst->Copy());
    }
    return fst_map;
  }

  absl::Status SetUpFromFstMap() {
    ASSIGN_OR_RETURN(
        absl_nonnull std::unique_ptr<STTableFarReader<StdArc>> far_reader,
        STTableFarReader<StdArc>::OpenWithStatus(far_path_));
    manager_.Load(ReadFstMap(std::move(far_reader)));
    return absl::OkStatus();
  }

 protected:
  StdRewriteManager manager_;
  const std::string far_path_;
};

TEST_F(RewriteManagerTest, TestMatches) {
  SetUpFromFar();
  Matches("cap", "a", "A");
  Matches("cap", "b", "B");
}

TEST_F(RewriteManagerTest, TestSimpleRewrites) {
  SetUpFromFar();
  TopRewrite("cap", "a", "A");
  TopRewrite("cap", "b", "B");
}

TEST_F(RewriteManagerTest, TestWithSorting) {
  SetUpFromFar();
  TopRewrite("cap_sorted", "a", "A");
  TopRewrite("cap_sorted", "b", "B");
}

TEST_F(RewriteManagerTest, TestFailedRewrites) {
  SetUpFromFar();
  FailedTopRewrite("cap", "z");
}

TEST_F(RewriteManagerTest, TestBadGetFst) {
  SetUpFromFar();
  EXPECT_EQ(nullptr, manager_.GetFst("nonexistent"));
  EXPECT_EQ(nullptr, manager_.GetFstSafe("nonexistent"));
}

TEST_F(RewriteManagerTest, TestBadSetFst) {
  SetUpFromFar();
  ASSERT_FALSE(manager_.SetFst("nonexistent", *manager_.GetFst("cap")));
}

// Same as above tests, only init from FstMap instead of FAR file.
TEST_F(RewriteManagerTest, TestLoadFstMapSimpleRewrites) {
  ABSL_ASSERT_OK(SetUpFromFstMap());
  TopRewrite("cap", "a", "A");
  TopRewrite("cap", "b", "B");
}

TEST_F(RewriteManagerTest, TestLoadFstMapWithSorting) {
  ABSL_ASSERT_OK(SetUpFromFstMap());
  TopRewrite("cap_sorted", "a", "A");
  TopRewrite("cap_sorted", "b", "B");
}

TEST_F(RewriteManagerTest, TestLoadFstMapFailedRewrites) {
  ABSL_ASSERT_OK(SetUpFromFstMap());
  FailedTopRewrite("cap", "z");
}

TEST_F(RewriteManagerTest, TestLoadFstMapBadGetFst) {
  ABSL_ASSERT_OK(SetUpFromFstMap());
  EXPECT_EQ(nullptr, manager_.GetFst("nonexistent"));
  EXPECT_EQ(nullptr, manager_.GetFstSafe("nonexistent"));
}

TEST_F(RewriteManagerTest, TestLoadFstMapBadSetFst) {
  ABSL_ASSERT_OK(SetUpFromFstMap());
  ASSERT_FALSE(manager_.SetFst("nonexistent", *manager_.GetFst("cap")));
}

}  // namespace
}  // namespace rewrite
