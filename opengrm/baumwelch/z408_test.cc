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

// Attempts to decipher Zodiac cipher 408 with Baum-Welch training and a
// language model.

#include <cstdint>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "opengrm/baumwelch/decipherment-tester.h"
#include "opengrm/baumwelch/train.h"

#ifdef TEST_LOG
#include "openfst/extensions/special/phi-fst.h"
#endif  // TEST_LOG

ABSL_FLAG(int, max_iters, ::fst::kMaxIters, "Maximum number of iterations");
ABSL_FLAG(double, alpha, ::fst::kAlpha,
          "Step size reduction power parameter; full standard batch EM is "
          "run (not stepwise) if alpha is set to 0");
ABSL_FLAG(int, batch_size, 0, "Batch size");
ABSL_FLAG(double, delta, .01, "Convergence delta");

namespace fst {
namespace {

class DeciphermentTest : public ::testing::Test {
 protected:
  void SetUp() final {
    absl::SetFlag(&FLAGS_fst_default_cache_gc, false);

    const std::string path =
        fst::JoinPath(std::string("."),
                       "opengrm/baumwelch/testdata/z408");

    // 6-gram character (i.e., byte) LM over the 26 capital ASCII characters
    // (space is deleted as a postprocessing step) with Witten-Bell smoothing
    // and heavy pruning.
    const std::string lm_name = fst::JoinPath(path, "lm.fst");
    const std::string channel_name = fst::JoinPath(path, "channel.fst");
#ifdef TEST_LOG
    const std::string ciphertext_name =
        fst::JoinPath(path, "ciphertext-log.fst");
    const std::string plaintext_name =
        fst::JoinPath(path, "plaintext-log.fst");
    {
      std::unique_ptr<VectorFst<StdArc>> lm_std(
          VectorFst<StdArc>::Read(lm_name));
      CHECK(lm_std != nullptr);
      VectorFst<LogArc> lm_log;
      Cast(*lm_std, &lm_log);
      lm_ = std::make_unique<PhiFst<LogArc>>(lm_log);
    }
    ciphertext_.reset(FarReader<LogArc>::Open(ciphertext_name));
    CHECK(ciphertext_ != nullptr);
    {
      std::unique_ptr<VectorFst<StdArc>> channel_std(
          VectorFst<StdArc>::Read(channel_name));
      CHECK(channel_std != nullptr);
      Cast(*channel_std, &channel_);
    }
    plaintext_.reset(FarReader<LogArc>::Open(plaintext_name));
    CHECK(plaintext_ != nullptr);
  }

  std::unique_ptr<PhiFst<LogArc>> lm_;
  std::unique_ptr<FarReader<LogArc>> ciphertext_;
  std::unique_ptr<VectorFst<StdArc>> channel_std_;
  VectorFst<LogArc> channel_;
  std::unique_ptr<FarReader<LogArc>> plaintext_;
#elif TEST_STD
    const std::string ciphertext_name = fst::JoinPath(path, "ciphertext.fst");
    const std::string plaintext_name = fst::JoinPath(path, "plaintext.fst");
    lm_.reset(VectorFst<StdArc>::Read(lm_name));
    CHECK(lm_ != nullptr);
    ciphertext_.reset(FarReader<StdArc>::Open(ciphertext_name));
    CHECK(ciphertext_ != nullptr);
    channel_.reset(VectorFst<StdArc>::Read(channel_name));
    CHECK(channel_ != nullptr);
    plaintext_.reset(FarReader<StdArc>::Open(plaintext_name));
    CHECK(plaintext_ != nullptr);
  }

  std::unique_ptr<VectorFst<StdArc>> lm_;
  std::unique_ptr<FarReader<StdArc>> ciphertext_;
  std::unique_ptr<VectorFst<StdArc>> channel_;
  std::unique_ptr<FarReader<StdArc>> plaintext_;
#endif
};

TEST_F(DeciphermentTest, Z408Test) {
  const TrainOptions opts(
      /*max_iters=*/absl::GetFlag(FLAGS_max_iters),
      /*alpha=*/absl::GetFlag(FLAGS_alpha),
      /*batch_size=*/absl::GetFlag(FLAGS_batch_size),
      /*delta=*/absl::GetFlag(FLAGS_delta));
#ifdef TEST_LOG
  DeciphermentTests(*lm_, *ciphertext_, channel_, opts, *plaintext_);
#elif TEST_STD
  DeciphermentTests(*lm_, *ciphertext_, *channel_, opts, *plaintext_);
#endif  // TEST_STD
}

}  // namespace
}  // namespace fst
