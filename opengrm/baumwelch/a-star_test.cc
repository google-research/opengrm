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

// Enable --logtostderr --v=1 to see the actual number of states explored.

#include "opengrm/baumwelch/a-star.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

class AStarTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string path =
        fst::JoinPath(std::string("."),
                       "opengrm/baumwelch/testdata/a-star");
    const std::string nfa_name = fst::JoinPath(path, "nfa.fst");
    const std::string shortest_name = fst::JoinPath(path, "shortest.fst");

    nfa_.reset(VectorFst<LogArc>::Read(nfa_name));
    CHECK(nfa_ != nullptr);
    shortest_.reset(VectorFst<LogArc>::Read(shortest_name));
    CHECK(shortest_ != nullptr);
  }

  std::unique_ptr<VectorFst<LogArc>> nfa_;
  std::unique_ptr<VectorFst<LogArc>> shortest_;
};

TEST_F(AStarTest, AStarTest) {
  VectorFst<LogArc> shortest;
  AStarSingleShortestString(*nfa_, &shortest);

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

}  // namespace
}  // namespace fst
