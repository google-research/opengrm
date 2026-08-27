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

// Unit tests for normalized stochastic FST intersection algorithm.

#include "opengrm/sfst/intersect.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "absl/log/globals.h"
#include "absl/memory/memory.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/trim.h"

namespace sfst {
namespace {

using fst::StdArc;
using fst::VectorFst;
using Label = StdArc::Label;
using StateId = StdArc::StateId;
using Weight = StdArc::Weight;

class IntersectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    constexpr char kTestDataDir[] = "opengrm/sfst/testdata";
    auto read_fst = [kTestDataDir](const std::string& name) {
      const std::string path =
          fst::JoinPath(std::string("."), kTestDataDir, name);
      return absl::WrapUnique(fst::Fst<StdArc>::Read(path));
    };
    sfst1_ = read_fst("phi1.fst");
    sfst2_ = read_fst("phi2.fst");
    sfst3_ = read_fst("phi3.fst");
    sfst4_ = read_fst("phi4.fst");
    sfst5_ = read_fst("phi5.fst");
    sfst6_ = read_fst("phi6.fst");
  }

  std::unique_ptr<fst::Fst<StdArc>> sfst1_;
  std::unique_ptr<fst::Fst<StdArc>> sfst2_;
  std::unique_ptr<fst::Fst<StdArc>> sfst3_;
  std::unique_ptr<fst::Fst<StdArc>> sfst4_;
  std::unique_ptr<fst::Fst<StdArc>> sfst5_;
  std::unique_ptr<fst::Fst<StdArc>> sfst6_;
};

TEST_F(IntersectTest, IntersectBiMatcherNoTrim) {
  ASSERT_NE(sfst1_, nullptr);
  ASSERT_NE(sfst2_, nullptr);
  ASSERT_NE(sfst3_, nullptr);
  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(*sfst1_, *sfst2_, &ofst, 0, false));
  EXPECT_TRUE(fst::Equal(ofst, *sfst3_));
}

TEST_F(IntersectTest, IntersectTriMatcherNoTrim) {
  ASSERT_NE(sfst4_, nullptr);
  ASSERT_NE(sfst5_, nullptr);
  ASSERT_NE(sfst6_, nullptr);
  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(*sfst4_, *sfst5_, &ofst, 0, false));
  EXPECT_TRUE(fst::Equal(ofst, *sfst6_));
}

TEST_F(IntersectTest, IntersectBiMatcherWithTrim) {
  ASSERT_NE(sfst1_, nullptr);
  ASSERT_NE(sfst2_, nullptr);
  VectorFst<StdArc> ofst;
  EXPECT_TRUE(
      Intersect(*sfst1_, *sfst2_, &ofst, 0, true, sfst::TRIM_NEEDED_FINAL));
  EXPECT_NE(ofst.Start(), fst::kNoStateId);
}

TEST_F(IntersectTest, IntersectTriMatcherWithTrim) {
  ASSERT_NE(sfst4_, nullptr);
  ASSERT_NE(sfst5_, nullptr);
  VectorFst<StdArc> ofst;
  EXPECT_TRUE(
      Intersect(*sfst4_, *sfst5_, &ofst, 0, true, sfst::TRIM_NEEDED_FINAL));
  EXPECT_NE(ofst.Start(), fst::kNoStateId);
}

TEST_F(IntersectTest, IntersectNonZeroPhiLabel) {
  ASSERT_NE(sfst4_, nullptr);
  ASSERT_NE(sfst5_, nullptr);
  ASSERT_NE(sfst6_, nullptr);
  constexpr Label kPhiLabel = -2;
  VectorFst<StdArc> sfst4(*sfst4_);
  VectorFst<StdArc> sfst5(*sfst5_);
  std::vector<std::pair<Label, Label>> pairs = {{0, kPhiLabel}};
  std::vector<std::pair<Label, Label>> invpairs = {{kPhiLabel, 0}};
  fst::Relabel(&sfst4, pairs, pairs);
  fst::Relabel(&sfst5, pairs, pairs);

  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(sfst4, sfst5, &ofst, kPhiLabel, false));
  fst::Relabel(&ofst, invpairs, invpairs);
  EXPECT_TRUE(fst::Equal(ofst, *sfst6_));
}

TEST_F(IntersectTest, IntersectNoLabel) {
  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst1.SetFinal(1, Weight::One());

  VectorFst<StdArc> fst2;
  fst2.AddState();
  fst2.AddState();
  fst2.SetStart(0);
  fst2.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst2.SetFinal(1, Weight::One());

  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(fst1, fst2, &ofst, fst::kNoLabel, true));
  EXPECT_NE(ofst.Start(), fst::kNoStateId);
  EXPECT_EQ(ofst.NumStates(), 2);
}

TEST_F(IntersectTest, IncompatibleSymbolTablesRejected) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst1.SetFinal(1, Weight::One());
  fst::SymbolTable syms1;
  syms1.AddSymbol("<epsilon>", 0);
  syms1.AddSymbol("a", 1);
  fst1.SetInputSymbols(&syms1);
  fst1.SetOutputSymbols(&syms1);

  VectorFst<StdArc> fst2;
  fst2.AddState();
  fst2.AddState();
  fst2.SetStart(0);
  fst2.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst2.SetFinal(1, Weight::One());
  fst::SymbolTable syms2;
  syms2.AddSymbol("<epsilon>", 0);
  syms2.AddSymbol("b", 1);
  fst2.SetInputSymbols(&syms2);
  fst2.SetOutputSymbols(&syms2);

  VectorFst<StdArc> ofst;
  EXPECT_FALSE(Intersect(fst1, fst2, &ofst, fst::kNoLabel, true));
}

TEST_F(IntersectTest, MatchingSymbolTablesAllowed) {
  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst1.SetFinal(1, Weight::One());
  fst::SymbolTable syms;
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);

  VectorFst<StdArc> fst2;
  fst2.AddState();
  fst2.AddState();
  fst2.SetStart(0);
  fst2.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst2.SetFinal(1, Weight::One());
  fst2.SetInputSymbols(&syms);
  fst2.SetOutputSymbols(&syms);

  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(fst1, fst2, &ofst, fst::kNoLabel, true));
  EXPECT_NE(ofst.Start(), fst::kNoStateId);
}

TEST_F(IntersectTest, NullSymbolTablesAllowed) {
  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst1.SetFinal(1, Weight::One());

  VectorFst<StdArc> fst2;
  fst2.AddState();
  fst2.AddState();
  fst2.SetStart(0);
  fst2.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst2.SetFinal(1, Weight::One());

  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(fst1, fst2, &ofst, fst::kNoLabel, true));
  EXPECT_NE(ofst.Start(), fst::kNoStateId);
}

TEST_F(IntersectTest, OneNullSymbolTableAllowed) {
  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst1.SetFinal(1, Weight::One());
  fst::SymbolTable syms;
  syms.AddSymbol("<epsilon>", 0);
  syms.AddSymbol("a", 1);
  fst1.SetInputSymbols(&syms);
  fst1.SetOutputSymbols(&syms);

  VectorFst<StdArc> fst2;
  fst2.AddState();
  fst2.AddState();
  fst2.SetStart(0);
  fst2.AddArc(0, StdArc(1, 1, Weight(0.5), 1));
  fst2.SetFinal(1, Weight::One());
  VectorFst<StdArc> ofst;
  EXPECT_TRUE(Intersect(fst1, fst2, &ofst, fst::kNoLabel, true));
  EXPECT_NE(ofst.Start(), fst::kNoStateId);
}

TEST_F(IntersectTest, NonAcceptorRejected) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  VectorFst<StdArc> fst1;
  fst1.AddState();
  fst1.AddState();
  fst1.SetStart(0);
  fst1.AddArc(0,
              StdArc(/*ilabel=*/1, /*olabel=*/2, Weight(0.5), /*nextstate=*/1));
  fst1.SetFinal(1, Weight::One());

  VectorFst<StdArc> fst2;
  fst2.AddState();
  fst2.AddState();
  fst2.SetStart(0);
  fst2.AddArc(0,
              StdArc(/*ilabel=*/1, /*olabel=*/1, Weight(0.5), /*nextstate=*/1));
  fst2.SetFinal(1, Weight::One());

  VectorFst<StdArc> ofst;
  EXPECT_FALSE(Intersect(fst1, fst2, &ofst, fst::kNoLabel, true));
  EXPECT_FALSE(Intersect(fst2, fst1, &ofst, fst::kNoLabel, true));
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
