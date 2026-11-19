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

#include "opengrm/operators/optimize.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"
#include "opengrm/operators/optimizescript.h"

namespace fst {
namespace {

using Arc = StdArc;

class OptimizeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path = fst::JoinPath(
        std::string("."), "opengrm/operators/testdata");
    // Weighted acyclic acceptor.
    const std::string fst1_name = fst::JoinPath(path, "ofst1.fst");
    const std::string fst1_opt_name = fst::JoinPath(path, "ofst1_opt.fst");
    const std::string fst1_log_opt_name =
        fst::JoinPath(path, "ofst1_log_opt.fst");
    // Cyclic unweighted acceptor.
    const std::string fst2_name = fst::JoinPath(path, "ofst2.fst");
    const std::string fst2_opt_name = fst::JoinPath(path, "ofst2_opt.fst");
    const std::string fst2_log_opt_name =
        fst::JoinPath(path, "ofst2_log_opt.fst");
    // Cyclic weighted acceptor.
    const std::string fst3_name = fst::JoinPath(path, "ofst3.fst");
    const std::string fst3_opt_name = fst::JoinPath(path, "ofst3_opt.fst");
    const std::string fst3_log_opt_name =
        fst::JoinPath(path, "ofst3_log_opt.fst");
    // Functional cyclic weighted transducer
    const std::string fst4_name = fst::JoinPath(path, "ofst4.fst");
    const std::string fst4_opt_name = fst::JoinPath(path, "ofst4_opt.fst");
    const std::string fst4_log_opt_name =
        fst::JoinPath(path, "ofst4_log_opt.fst");
    // Non-functional cyclic weighted transducer
    const std::string fst5_name = fst::JoinPath(path, "ofst5.fst");
    const std::string fst5_opt_name = fst::JoinPath(path, "ofst5_opt.fst");
    const std::string fst5_log_opt_name =
        fst::JoinPath(path, "ofst5_log_opt.fst");

    fst1_.reset(VectorFst<StdArc>::Read(fst1_name));
    fst1_opt_.reset(VectorFst<StdArc>::Read(fst1_opt_name));
    fst1_log_opt_.reset(VectorFst<LogArc>::Read(fst1_log_opt_name));
    fst2_.reset(VectorFst<StdArc>::Read(fst2_name));
    fst2_opt_.reset(VectorFst<StdArc>::Read(fst2_opt_name));
    fst2_log_opt_.reset(VectorFst<LogArc>::Read(fst2_log_opt_name));
    fst3_.reset(VectorFst<StdArc>::Read(fst3_name));
    fst3_opt_.reset(VectorFst<StdArc>::Read(fst3_opt_name));
    fst3_log_opt_.reset(VectorFst<LogArc>::Read(fst3_log_opt_name));
    fst4_.reset(VectorFst<StdArc>::Read(fst4_name));
    fst4_opt_.reset(VectorFst<StdArc>::Read(fst4_opt_name));
    fst4_log_opt_.reset(VectorFst<LogArc>::Read(fst4_log_opt_name));
    fst5_.reset(VectorFst<StdArc>::Read(fst5_name));
    fst5_opt_.reset(VectorFst<StdArc>::Read(fst5_opt_name));
    fst5_log_opt_.reset(VectorFst<LogArc>::Read(fst5_log_opt_name));
  }

  std::unique_ptr<VectorFst<StdArc>> fst1_;
  std::unique_ptr<VectorFst<StdArc>> fst1_opt_;
  std::unique_ptr<VectorFst<LogArc>> fst1_log_opt_;
  std::unique_ptr<VectorFst<StdArc>> fst2_;
  std::unique_ptr<VectorFst<StdArc>> fst2_opt_;
  std::unique_ptr<VectorFst<LogArc>> fst2_log_opt_;
  std::unique_ptr<VectorFst<StdArc>> fst3_;
  std::unique_ptr<VectorFst<StdArc>> fst3_opt_;
  std::unique_ptr<VectorFst<LogArc>> fst3_log_opt_;
  std::unique_ptr<VectorFst<StdArc>> fst4_;
  std::unique_ptr<VectorFst<StdArc>> fst4_opt_;
  std::unique_ptr<VectorFst<LogArc>> fst4_log_opt_;
  std::unique_ptr<VectorFst<StdArc>> fst5_;
  std::unique_ptr<VectorFst<StdArc>> fst5_opt_;
  std::unique_ptr<VectorFst<LogArc>> fst5_log_opt_;
};

// Tests make copies before optimizing, so the individual units are not
// order-dependent.

// Optimize FSTs using the library API.
TEST_F(OptimizeTest, OptimizeLib) {
  VectorFst<StdArc> fst1_res(*fst1_);
  Optimize(&fst1_res, true);
  ASSERT_TRUE(Verify(fst1_res));
  ASSERT_TRUE(Equal(*fst1_opt_, fst1_res));

  VectorFst<StdArc> fst2_res(*fst2_);
  Optimize(&fst2_res, true);
  ASSERT_TRUE(Verify(fst2_res));
  ASSERT_TRUE(Equal(*fst2_opt_, fst2_res));

  VectorFst<StdArc> fst3_res(*fst3_);
  Optimize(&fst3_res, true);
  ASSERT_TRUE(Verify(fst3_res));
  ASSERT_TRUE(Equal(*fst3_opt_, fst3_res));

  VectorFst<StdArc> fst4_res(*fst4_);
  Optimize(&fst4_res, true);
  ASSERT_TRUE(Verify(fst4_res));
  ASSERT_TRUE(Equal(*fst4_opt_, fst4_res));

  VectorFst<StdArc> fst5_res(*fst5_);
  Optimize(&fst5_res, true);
  ASSERT_TRUE(Verify(fst5_res));
  ASSERT_TRUE(Equal(*fst5_opt_, fst5_res));
}

// The same as OptimizeLib, but using the scripting (template-free) API.
TEST_F(OptimizeTest, OptimizeScript) {
  namespace s = fst::script;

  s::VectorFstClass fstc1(*fst1_);
  s::VectorFstClass fstc1_res(fstc1);
  Optimize(&fstc1_res, true);
  ASSERT_TRUE(s::Verify(fstc1_res));
  s::VectorFstClass fstc1_opt(*fst1_opt_);
  ASSERT_TRUE(s::Equal(fstc1_res, fstc1_opt));

  s::VectorFstClass fstc2(*fst2_);
  s::VectorFstClass fstc2_res(fstc2);
  Optimize(&fstc2_res, true);
  ASSERT_TRUE(s::Verify(fstc2_res));
  s::VectorFstClass fstc2_opt(*fst2_opt_);
  ASSERT_TRUE(s::Equal(fstc2_res, fstc2_opt));

  s::VectorFstClass fstc3(*fst3_);
  s::VectorFstClass fstc3_res(fstc3);
  Optimize(&fstc3_res, true);
  ASSERT_TRUE(s::Verify(fstc3_res));
  s::VectorFstClass fstc3_opt(*fst3_opt_);
  ASSERT_TRUE(s::Equal(fstc3_res, fstc3_opt));

  s::VectorFstClass fstc4(*fst4_);
  s::VectorFstClass fstc4_res(fstc4);
  Optimize(&fstc4_res, true);
  ASSERT_TRUE(s::Verify(fstc4_res));
  s::VectorFstClass fstc4_opt(*fst4_opt_);
  ASSERT_TRUE(s::Equal(fstc4_res, fstc4_opt));

  s::VectorFstClass fstc5(*fst5_);
  s::VectorFstClass fstc5_res(fstc5);
  Optimize(&fstc5_res, true);
  ASSERT_TRUE(s::Verify(fstc5_res));
  s::VectorFstClass fstc5_opt(*fst5_opt_);
  ASSERT_TRUE(s::Equal(fstc5_res, fstc5_opt));
}

// Optimize FSTs as log-weight (i.e., non-idempotent) FSTs.
TEST_F(OptimizeTest, OptimizeLog) {
  const StdToLogMapper mapper;

  VectorFst<LogArc> fst1_res;
  ArcMap(*fst1_, &fst1_res, mapper);
  Optimize(&fst1_res, true);
  ASSERT_TRUE(Verify(fst1_res));
  ASSERT_TRUE(Equal(*fst1_log_opt_, fst1_res));

  VectorFst<LogArc> fst2_res;
  ArcMap(*fst2_, &fst2_res, mapper);
  Optimize(&fst2_res, true);
  ASSERT_TRUE(Verify(fst2_res));
  ASSERT_TRUE(Equal(*fst2_log_opt_, fst2_res));

  VectorFst<LogArc> fst3_res;
  ArcMap(*fst3_, &fst3_res, mapper);
  Optimize(&fst3_res, true);
  ASSERT_TRUE(Verify(fst3_res));
  ASSERT_TRUE(Equal(*fst3_log_opt_, fst3_res));

  VectorFst<LogArc> fst4_res;
  ArcMap(*fst4_, &fst4_res, mapper);
  Optimize(&fst4_res, true);
  ASSERT_TRUE(Verify(fst4_res));
  ASSERT_TRUE(Equal(*fst4_log_opt_, fst4_res));

  VectorFst<LogArc> fst5_res;
  ArcMap(*fst5_, &fst5_res, mapper);
  Optimize(&fst5_res, true);
  ASSERT_TRUE(Verify(fst5_res));
  ASSERT_TRUE(Equal(*fst5_log_opt_, fst5_res));
}

TEST(OptimizeTransducerArcSortTest, CyclicUnweightedTransducerSortsArcs) {
  // Nondeterministic cyclic unweighted transducer where ilabel 3 precedes
  // ilabel 1 in EncodeTable encounter order.
  VectorFst<StdArc> input;
  input.AddStates(3);
  input.SetStart(0);
  input.SetFinal(0, StdArc::Weight::One());
  input.AddArc(0, StdArc(3, 4, StdArc::Weight::One(), 0));
  input.AddArc(0, StdArc(0, 0, StdArc::Weight::One(), 1));
  input.AddArc(1, StdArc(1, 2, StdArc::Weight::One(), 2));
  input.AddArc(1, StdArc(1, 2, StdArc::Weight::One(), 0));
  input.AddArc(2, StdArc(2, 3, StdArc::Weight::One(), 0));

  VectorFst<StdArc> fst_no_props(input);
  Optimize(&fst_no_props, /*compute_props=*/false);
  ASSERT_TRUE(Verify(fst_no_props));

  VectorFst<StdArc> fst_with_props(input);
  Optimize(&fst_with_props, /*compute_props=*/true);
  ASSERT_TRUE(Verify(fst_with_props));
  EXPECT_EQ(fst_with_props.Properties(kILabelSorted, false), kILabelSorted);
  EXPECT_EQ(fst_with_props.Properties(kILabelSorted, true), kILabelSorted);
  EXPECT_TRUE(Equal(fst_no_props, fst_with_props));
}

TEST(OptimizeTransducerArcSortTest, AcyclicLogTransducerSortsArcs) {
  // Nondeterministic acyclic LogArc transducer where ilabel 3 precedes
  // ilabel 1 in EncodeTable encounter order.
  VectorFst<LogArc> fst;
  fst.AddStates(3);
  fst.SetStart(0);
  fst.SetFinal(2, LogArc::Weight::One());
  fst.AddArc(0, LogArc(3, 4, LogArc::Weight(0.5), 2));
  fst.AddArc(0, LogArc(1, 2, LogArc::Weight(1.0), 1));
  fst.AddArc(0, LogArc(1, 2, LogArc::Weight(2.0), 2));
  fst.AddArc(1, LogArc(2, 3, LogArc::Weight(0.25), 2));

  Optimize(&fst, /*compute_props=*/true);
  ASSERT_TRUE(Verify(fst));
  EXPECT_EQ(fst.Properties(kILabelSorted, false), kILabelSorted);
  EXPECT_EQ(fst.Properties(kILabelSorted, true), kILabelSorted);
}

template <class Arc>
void ExpectOptimizeDeterministicMatches(const VectorFst<Arc>& input,
                                        const VectorFst<Arc>& expected) {
  // Run compute_props=true first (detaching on mutation) before
  // Properties(..., true) caches properties on input's shared Impl.
  VectorFst<Arc> fst_with_props(input);
  Optimize(&fst_with_props, /*compute_props=*/true);

  // Pre-compute stored properties (as CompactFst / ConstFst headers do in FAR
  // archives) so compute_props=false enters the kIDeterministic branch.
  VectorFst<Arc> fst_stored_props(input);
  fst_stored_props.Properties(kFstProperties, /*test=*/true);
  Optimize(&fst_stored_props, /*compute_props=*/false);

  ASSERT_TRUE(Verify(fst_stored_props));
  EXPECT_EQ(fst_stored_props.NumStates(), expected.NumStates());
  EXPECT_TRUE(Isomorphic(fst_stored_props, expected));
  if (input.Properties(kAcceptor, true) != kAcceptor) {
    EXPECT_EQ(fst_stored_props.Properties(kILabelSorted, false), kILabelSorted);
    EXPECT_EQ(fst_stored_props.Properties(kILabelSorted, true), kILabelSorted);
  }
  ASSERT_TRUE(Verify(fst_with_props));
  EXPECT_TRUE(Equal(fst_stored_props, fst_with_props));
}

TEST(OptimizeDeterministicTest, NegativeWeightCycleTransducer) {
  // Deterministic transducer with a -1.0 cycle at state 0 (added before
  // ilabels 1 and 2 so EncodeTable order is unsorted) and two equivalent
  // states 1 and 2.
  VectorFst<StdArc> input;
  input.AddStates(3);
  input.SetStart(0);
  input.SetFinal(0, StdArc::Weight::One());
  input.SetFinal(1, StdArc::Weight::One());
  input.SetFinal(2, StdArc::Weight::One());
  input.AddArc(0, StdArc(3, 4, StdArc::Weight(-1.0f), 0));
  input.AddArc(0, StdArc(1, 2, StdArc::Weight::One(), 1));
  input.AddArc(0, StdArc(2, 3, StdArc::Weight::One(), 2));
  input.AddArc(1, StdArc(4, 5, StdArc::Weight(2.0f), 0));
  input.AddArc(2, StdArc(4, 5, StdArc::Weight(2.0f), 0));

  VectorFst<StdArc> expected;
  expected.AddStates(2);
  expected.SetStart(0);
  expected.SetFinal(0, StdArc::Weight::One());
  expected.SetFinal(1, StdArc::Weight::One());
  expected.AddArc(0, StdArc(1, 2, StdArc::Weight::One(), 1));
  expected.AddArc(0, StdArc(2, 3, StdArc::Weight::One(), 1));
  expected.AddArc(0, StdArc(3, 4, StdArc::Weight(-1.0f), 0));
  expected.AddArc(1, StdArc(4, 5, StdArc::Weight(2.0f), 0));

  ExpectOptimizeDeterministicMatches(input, expected);
}

TEST(OptimizeDeterministicTest, NegativeWeightCycleAcceptor) {
  // Deterministic acceptor with a -100.0 cycle at state 0 and two equivalent
  // states 1 and 2; unencoded Minimize would saturate float32 in Push and wipe
  // the -100.0 cycle weight to 0.0.
  VectorFst<StdArc> input;
  input.AddStates(3);
  input.SetStart(0);
  input.SetFinal(0, StdArc::Weight::One());
  input.SetFinal(1, StdArc::Weight::One());
  input.SetFinal(2, StdArc::Weight::One());
  input.AddArc(0, StdArc(3, 3, StdArc::Weight(-100.0f), 0));
  input.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  input.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 2));
  input.AddArc(1, StdArc(4, 4, StdArc::Weight(2.0f), 0));
  input.AddArc(2, StdArc(4, 4, StdArc::Weight(2.0f), 0));

  VectorFst<StdArc> expected;
  expected.AddStates(2);
  expected.SetStart(0);
  expected.SetFinal(0, StdArc::Weight::One());
  expected.SetFinal(1, StdArc::Weight::One());
  expected.AddArc(0, StdArc(1, 1, StdArc::Weight::One(), 1));
  expected.AddArc(0, StdArc(2, 2, StdArc::Weight::One(), 1));
  expected.AddArc(0, StdArc(3, 3, StdArc::Weight(-100.0f), 0));
  expected.AddArc(1, StdArc(4, 4, StdArc::Weight(2.0f), 0));

  ExpectOptimizeDeterministicMatches(input, expected);
}

TEST(OptimizeDeterministicTest, CyclicLogAcceptorAndTransducer) {
  // Deterministic LogArc acceptor and transducer with an unweighted cycle at
  // state 0 and weighted transitions into equivalent final sink states 1 and 2,
  // so kUnweightedCycles (and thus kDoNotEncodeWeights) holds while kAcyclic
  // does not. Over LogArc, unencoded Minimize's Push diverges on the One()
  // cycle and corrupts total path weights.
  VectorFst<LogArc> acc;
  acc.AddStates(3);
  acc.SetStart(0);
  acc.SetFinal(0, LogArc::Weight::One());
  acc.SetFinal(1, LogArc::Weight::One());
  acc.SetFinal(2, LogArc::Weight::One());
  acc.AddArc(0, LogArc(3, 3, LogArc::Weight::One(), 0));
  acc.AddArc(0, LogArc(1, 1, LogArc::Weight(2.0f), 1));
  acc.AddArc(0, LogArc(2, 2, LogArc::Weight(2.0f), 2));

  VectorFst<LogArc> expected_acc;
  expected_acc.AddStates(2);
  expected_acc.SetStart(0);
  expected_acc.SetFinal(0, LogArc::Weight::One());
  expected_acc.SetFinal(1, LogArc::Weight::One());
  expected_acc.AddArc(0, LogArc(1, 1, LogArc::Weight(2.0f), 1));
  expected_acc.AddArc(0, LogArc(2, 2, LogArc::Weight(2.0f), 1));
  expected_acc.AddArc(0, LogArc(3, 3, LogArc::Weight::One(), 0));

  VectorFst<LogArc> trans;
  trans.AddStates(3);
  trans.SetStart(0);
  trans.SetFinal(0, LogArc::Weight::One());
  trans.SetFinal(1, LogArc::Weight::One());
  trans.SetFinal(2, LogArc::Weight::One());
  trans.AddArc(0, LogArc(3, 4, LogArc::Weight::One(), 0));
  trans.AddArc(0, LogArc(1, 2, LogArc::Weight(2.0f), 1));
  trans.AddArc(0, LogArc(2, 3, LogArc::Weight(2.0f), 2));

  VectorFst<LogArc> expected_trans;
  expected_trans.AddStates(2);
  expected_trans.SetStart(0);
  expected_trans.SetFinal(0, LogArc::Weight::One());
  expected_trans.SetFinal(1, LogArc::Weight::One());
  expected_trans.AddArc(0, LogArc(1, 2, LogArc::Weight(2.0f), 1));
  expected_trans.AddArc(0, LogArc(2, 3, LogArc::Weight(2.0f), 1));
  expected_trans.AddArc(0, LogArc(3, 4, LogArc::Weight::One(), 0));

  ExpectOptimizeDeterministicMatches(acc, expected_acc);
  ExpectOptimizeDeterministicMatches(trans, expected_trans);

  EXPECT_EQ(
      acc.Properties(internal::kDoNotEncodeWeights | kAcyclic, /*test=*/true),
      kUnweightedCycles);
  EXPECT_EQ(
      trans.Properties(internal::kDoNotEncodeWeights | kAcyclic, /*test=*/true),
      kUnweightedCycles);
}

TEST(OptimizeDeterministicTest, AcyclicTransducerPushesWeightsAndSortsArcs) {
  // States 1 and 2 have different suffix weights (2.0 vs 1.0) that become
  // equivalent only when weighted Minimize pushes weights to state 0.
  VectorFst<StdArc> std_fst;
  std_fst.AddStates(4);
  std_fst.SetStart(0);
  std_fst.SetFinal(3, StdArc::Weight::One());
  std_fst.AddArc(0, StdArc(3, 4, StdArc::Weight(1.0f), 1));
  std_fst.AddArc(0, StdArc(1, 2, StdArc::Weight(2.0f), 2));
  std_fst.AddArc(1, StdArc(5, 6, StdArc::Weight(2.0f), 3));
  std_fst.AddArc(2, StdArc(5, 6, StdArc::Weight(1.0f), 3));

  VectorFst<LogArc> log_fst;
  ArcMap(std_fst, &log_fst, StdToLogMapper());

  Optimize(&std_fst, /*compute_props=*/true);
  Optimize(&log_fst, /*compute_props=*/true);

  ASSERT_TRUE(Verify(std_fst));
  EXPECT_EQ(std_fst.NumStates(), 3);
  EXPECT_EQ(std_fst.Properties(kILabelSorted, false), kILabelSorted);
  EXPECT_EQ(std_fst.Properties(kILabelSorted, true), kILabelSorted);
  ASSERT_TRUE(Verify(log_fst));
  EXPECT_EQ(log_fst.NumStates(), 3);
  EXPECT_EQ(log_fst.Properties(kILabelSorted, false), kILabelSorted);
  EXPECT_EQ(log_fst.Properties(kILabelSorted, true), kILabelSorted);
}

}  // namespace
}  // namespace fst
