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

#include <sstream>  // NOLINT(misc-include-cleaner)
#include <string>   // NOLINT(misc-include-cleaner)

#include "openfst/compat/init.h"
#include "gtest/gtest.h"
#include "absl/strings/match.h"
#include "openfst/lib/arc.h"         // NOLINT(misc-include-cleaner)
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/info.h"

namespace sfst {
namespace {

TEST(SfstInfoTest, BasicTest) {
  fst::VectorFst<fst::StdArc> fst;
  fst.SetStart(fst.AddState());  // State 0
  fst.AddArc(0, fst::StdArc(1, 1, fst::StdArc::Weight::One(),
                            fst.AddState()));  // State 1
  fst.SetFinal(1, fst::StdArc::Weight::One());

  std::stringstream ss;
  SfstInfo(fst, ss);
  std::string output = ss.str();

  // Verify basic fields.
  EXPECT_TRUE(absl::StrContains(output, "# of states"));
  EXPECT_TRUE(absl::StrContains(output, "# of arcs"));
  EXPECT_TRUE(absl::StrContains(output, "initial state"));
  EXPECT_TRUE(absl::StrContains(output, "# of final states"));
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  fst::InitOpenFst(argv[0], &argc, &argv, true);
  return RUN_ALL_TESTS();
}
