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

// Unit tests for Baum-Welch decoding.

#include "opengrm/baumwelch/decode.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/extensions/far/fst-far-reader.h"
#include "openfst/extensions/far/fst-far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/string.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

// -----------------------------------------------------------------------------
// ReverseStateNumbering Tests
// -----------------------------------------------------------------------------

TEST(ReverseStateNumberingTest, ReversesStateOrder) {
  VectorFst<StdArc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  fst.SetStart(s0);
  fst.AddArc(s0, StdArc(1, 1, TropicalWeight::One(), s1));
  fst.AddArc(s1, StdArc(2, 2, TropicalWeight::One(), s2));
  fst.SetFinal(s2, TropicalWeight::One());

  // Before reversing: start is 0
  EXPECT_EQ(fst.Start(), s0);

  internal::ReverseStateNumbering(&fst);

  // After reversing: state numbering is reversed so start is s2 (id 2)
  EXPECT_EQ(fst.Start(), s2);
  EXPECT_EQ(fst.NumStates(), 3);
}

// -----------------------------------------------------------------------------
// ShortestPathOrString Tests
// -----------------------------------------------------------------------------

TEST(ShortestPathOrStringTest, StdArcTropicalWeightPathSemiring) {
  // Path with 2 parallel branches:
  // s0 --(1:1, w=5.0)--> s1 (w=0.0)
  // s0 --(2:2, w=1.0)--> s1 (w=0.0)
  VectorFst<StdArc> ifst;
  const auto s0 = ifst.AddState();
  const auto s1 = ifst.AddState();
  ifst.SetStart(s0);
  ifst.AddArc(s0, StdArc(1, 1, TropicalWeight(5.0), s1));
  ifst.AddArc(s0, StdArc(2, 2, TropicalWeight(1.0), s1));
  ifst.SetFinal(s1, TropicalWeight(0.0));

  VectorFst<StdArc> ofst;
  internal::ShortestPathOrString(ifst, &ofst);

  EXPECT_NE(ofst.Start(), kNoStateId);
  const StringPrinter<StdArc> printer(TokenType::SYMBOL);
  std::string str;
  ASSERT_TRUE(printer(ofst, &str));
  EXPECT_EQ(str, "2");
}

TEST(ShortestPathOrStringTest, LogArcLogWeightNonPathSemiring) {
  // Path with 2 parallel branches in Log semiring:
  // s0 --(1:1, w=3.0)--> s1 (w=0.0)
  // s0 --(2:2, w=0.5)--> s1 (w=0.0)
  VectorFst<LogArc> ifst;
  const auto s0 = ifst.AddState();
  const auto s1 = ifst.AddState();
  ifst.SetStart(s0);
  ifst.AddArc(s0, LogArc(1, 1, LogWeight(3.0), s1));
  ifst.AddArc(s0, LogArc(2, 2, LogWeight(0.5), s1));
  ifst.SetFinal(s1, LogWeight(0.0));

  VectorFst<LogArc> ofst;
  internal::ShortestPathOrString(ifst, &ofst);

  EXPECT_NE(ofst.Start(), kNoStateId);
  const StringPrinter<LogArc> printer(TokenType::SYMBOL);
  std::string str;
  ASSERT_TRUE(printer(ofst, &str));
  EXPECT_EQ(str, "2");
}

// -----------------------------------------------------------------------------
// Decode Test Fixture
// -----------------------------------------------------------------------------

class DecodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    input_far_path_ =
        fst::JoinPath(::testing::TempDir(), "decode_test_input.far");
    output_far_path_ =
        fst::JoinPath(::testing::TempDir(), "decode_test_output.far");
    hypotext_far_path_ =
        fst::JoinPath(::testing::TempDir(), "decode_test_hypotext.far");
  }

  template <class Arc>
  static void WriteFarData(const std::string& path,
                           const std::vector<std::string>& data) {
    std::unique_ptr<FarWriter<Arc>> writer(FarWriter<Arc>::Create(path));
    ASSERT_NE(writer, nullptr);
    const StringCompiler<Arc> compiler(TokenType::SYMBOL);
    for (size_t i = 0; i < data.size(); ++i) {
      VectorFst<Arc> fst;
      ASSERT_TRUE(compiler(data[i], &fst));
      writer->Add(std::to_string(i), fst);
    }
  }

  std::string input_far_path_;
  std::string output_far_path_;
  std::string hypotext_far_path_;
};

// -----------------------------------------------------------------------------
// Pair Decoding Tests
// -----------------------------------------------------------------------------

TEST_F(DecodeTest, PairDecodeStdArcWithoutEncoder) {
  // Input sequence: "1 2"
  // Output sequence: "10 20"
  WriteFarData<StdArc>(input_far_path_, {"1 2"});
  WriteFarData<StdArc>(output_far_path_, {"10 20"});

  std::unique_ptr<FarReader<StdArc>> in_reader(
      FarReader<StdArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<StdArc>> out_reader(
      FarReader<StdArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  // Model: maps 1 -> 10 (weight 0.5) and 2 -> 20 (weight 0.5)
  VectorFst<StdArc> model;
  const auto s0 = model.AddState();
  const auto s1 = model.AddState();
  const auto s2 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, StdArc(1, 10, TropicalWeight(0.5), s1));
  model.AddArc(s1, StdArc(2, 20, TropicalWeight(0.5), s2));
  model.SetFinal(s2, TropicalWeight(0.0));

  {
    std::unique_ptr<FarWriter<StdArc>> hypo_writer(
        FarWriter<StdArc>::Create(hypotext_far_path_));
    ASSERT_NE(hypo_writer, nullptr);
    Decode(*in_reader, *out_reader, model, *hypo_writer);
  }

  std::unique_ptr<FarReader<StdArc>> hypo_reader(
      FarReader<StdArc>::Open(hypotext_far_path_));
  ASSERT_NE(hypo_reader, nullptr);
  EXPECT_FALSE(hypo_reader->Done());
  EXPECT_EQ(hypo_reader->GetKey(), "0_0");
  const auto* fst = hypo_reader->GetFst();
  ASSERT_NE(fst, nullptr);
  const StringPrinter<StdArc> printer(TokenType::SYMBOL);
  std::string input_str, output_str;
  ASSERT_TRUE(
      printer(ProjectFst<StdArc>(*fst, ProjectType::INPUT), &input_str));
  EXPECT_EQ(input_str, "1 2");
  ASSERT_TRUE(printer(*fst, &output_str));
  EXPECT_EQ(output_str, "10 20");

  hypo_reader->Next();
  EXPECT_TRUE(hypo_reader->Done());
}

TEST_F(DecodeTest, PairDecodeStdArcWithEncodeMapper) {
  WriteFarData<StdArc>(input_far_path_, {"1 2"});
  WriteFarData<StdArc>(output_far_path_, {"10 20"});

  std::unique_ptr<FarReader<StdArc>> in_reader(
      FarReader<StdArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<StdArc>> out_reader(
      FarReader<StdArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  // Model: maps 1 -> 10, 2 -> 20
  VectorFst<StdArc> model;
  const auto s0 = model.AddState();
  const auto s1 = model.AddState();
  const auto s2 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, StdArc(1, 10, TropicalWeight(1.0), s1));
  model.AddArc(s1, StdArc(2, 20, TropicalWeight(1.0), s2));
  model.SetFinal(s2, TropicalWeight(0.0));

  EncodeMapper<StdArc> encoder(kEncodeLabels, ENCODE);

  {
    std::unique_ptr<FarWriter<StdArc>> hypo_writer(
        FarWriter<StdArc>::Create(hypotext_far_path_));
    ASSERT_NE(hypo_writer, nullptr);
    Decode(*in_reader, *out_reader, model, *hypo_writer, &encoder);
  }

  std::unique_ptr<FarReader<StdArc>> hypo_reader(
      FarReader<StdArc>::Open(hypotext_far_path_));
  ASSERT_NE(hypo_reader, nullptr);
  EXPECT_FALSE(hypo_reader->Done());
  EXPECT_EQ(hypo_reader->GetKey(), "0_0");
  const auto* fst = hypo_reader->GetFst();
  ASSERT_NE(fst, nullptr);
  // With kEncodeLabels, ilabel == olabel for each encoded arc.
  for (StateIterator<Fst<StdArc>> siter(*fst); !siter.Done(); siter.Next()) {
    for (ArcIterator<Fst<StdArc>> aiter(*fst, siter.Value()); !aiter.Done();
         aiter.Next()) {
      const auto& arc = aiter.Value();
      EXPECT_EQ(arc.ilabel, arc.olabel);
      EXPECT_EQ(arc.weight, TropicalWeight::One());  // Weight removed.
    }
  }

  hypo_reader->Next();
  EXPECT_TRUE(hypo_reader->Done());
}

// -----------------------------------------------------------------------------
// Decipherment Decoding Tests
// -----------------------------------------------------------------------------

TEST_F(DecodeTest, DeciphermentDecodeStdArc) {
  // Plaintext Language Model:
  // Accepts "1 2" with weight 1.0 (preferred) and "1 3" with weight 10.0
  VectorFst<StdArc> lm;
  const auto l0 = lm.AddState();
  const auto l1 = lm.AddState();
  const auto l2 = lm.AddState();
  const auto l3 = lm.AddState();
  lm.SetStart(l0);
  lm.AddArc(l0, StdArc(1, 1, TropicalWeight::One(), l1));
  lm.AddArc(l1, StdArc(2, 2, TropicalWeight(1.0), l2));
  lm.AddArc(l1, StdArc(3, 3, TropicalWeight(10.0), l3));
  lm.SetFinal(l2, TropicalWeight::One());
  lm.SetFinal(l3, TropicalWeight::One());

  // Write LM to FST-format FAR (FarType::FST)
  {
    std::unique_ptr<FstFarWriter<StdArc>> lm_writer(
        FstFarWriter<StdArc>::Create(input_far_path_));
    ASSERT_NE(lm_writer, nullptr);
    lm_writer->Add("LM", lm);
  }

  // Ciphertext FAR: contains "10 20"
  WriteFarData<StdArc>(output_far_path_, {"10 20"});

  // Channel Model:
  // Maps plaintext 1 -> 10, 2 -> 20, 3 -> 20 (substitution cipher)
  VectorFst<StdArc> channel;
  const auto c0 = channel.AddState();
  channel.SetStart(c0);
  channel.AddArc(c0, StdArc(1, 10, TropicalWeight::One(), c0));
  channel.AddArc(c0, StdArc(2, 20, TropicalWeight::One(), c0));
  channel.AddArc(c0, StdArc(3, 20, TropicalWeight::One(), c0));
  channel.SetFinal(c0, TropicalWeight::One());

  std::unique_ptr<FstFarReader<StdArc>> lm_reader(
      FstFarReader<StdArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<StdArc>> cipher_reader(
      FarReader<StdArc>::Open(output_far_path_));
  ASSERT_NE(lm_reader, nullptr);
  ASSERT_NE(cipher_reader, nullptr);

  {
    std::unique_ptr<FarWriter<StdArc>> hypo_writer(
        FarWriter<StdArc>::Create(hypotext_far_path_));
    ASSERT_NE(hypo_writer, nullptr);
    // When input is FarType::FST, top-level Decode delegates to
    // decipherment decoding.
    Decode(*lm_reader, *cipher_reader, channel, *hypo_writer);
  }

  std::unique_ptr<FarReader<StdArc>> hypo_reader(
      FarReader<StdArc>::Open(hypotext_far_path_));
  ASSERT_NE(hypo_reader, nullptr);
  EXPECT_FALSE(hypo_reader->Done());
  EXPECT_EQ(hypo_reader->GetKey(), "0");
  const auto* fst = hypo_reader->GetFst();
  ASSERT_NE(fst, nullptr);
  // Plaintext decoded should be the lowest weight LM path: "1 2"
  const StringPrinter<StdArc> printer(TokenType::SYMBOL);
  std::string input_str;
  ASSERT_TRUE(
      printer(ProjectFst<StdArc>(*fst, ProjectType::INPUT), &input_str));
  EXPECT_EQ(input_str, "1 2");

  hypo_reader->Next();
  EXPECT_TRUE(hypo_reader->Done());
}

}  // namespace
}  // namespace fst
