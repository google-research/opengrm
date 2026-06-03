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

}  // namespace
}  // namespace fst
