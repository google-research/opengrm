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

// Unit tests for Baum-Welch scripting APIs.

#include <cmath>
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/no_destructor.h"
#include "absl/flags/flag.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/fst-far-writer.h"
#include "openfst/extensions/far/sttable-far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/string.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "openfst/script/encodemapper-class.h"
#include "openfst/script/fst-class.h"
#include "opengrm/baumwelch/decodescript.h"
#include "opengrm/baumwelch/randomizescript.h"
#include "opengrm/baumwelch/train.h"
#include "opengrm/baumwelch/trainscript.h"

namespace fst {
namespace script {
namespace {

struct UnregisteredArc : public StdArc {
  using StdArc::StdArc;

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("test_unregistered");
    return *type;
  }
};

template <class Arc>
VectorFst<Arc> MakeChannelFst() {
  using Weight = typename Arc::Weight;
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  fst.AddArc(s0, Arc(1, 10, Weight::One(), s0));
  fst.AddArc(s0, Arc(2, 20, Weight::One(), s0));
  fst.AddArc(s0, Arc(3, 20, Weight::One(), s0));
  fst.SetFinal(s0, Weight::One());
  return fst;
}

template <class Arc>
void WriteStTableFar(const std::string& path,
                     const std::vector<std::string>& entries) {
  std::unique_ptr<STTableFarWriter<Arc>> writer(
      STTableFarWriter<Arc>::Create(path));
  ASSERT_NE(writer, nullptr);
  const StringCompiler<Arc> compiler(TokenType::SYMBOL);
  for (size_t i = 0; i < entries.size(); ++i) {
    VectorFst<Arc> fst;
    ASSERT_TRUE(compiler(entries[i], &fst));
    writer->Add(std::to_string(i), fst);
  }
  ASSERT_FALSE(writer->Error());
}

// -----------------------------------------------------------------------------
// Randomize script tests
// -----------------------------------------------------------------------------

TEST(RandomizeScriptTest, RandomizesStdAndLogArcsDeterministically) {
  VectorFstClass std_fst1(MakeChannelFst<StdArc>());
  VectorFstClass std_fst2(MakeChannelFst<StdArc>());
  std::mt19937_64 gen1(42);
  std::mt19937_64 gen2(42);
  Randomize(gen1, &std_fst1);
  Randomize(gen2, &std_fst2);

  EXPECT_EQ(std_fst1.Properties(kError, false), 0);
  EXPECT_TRUE(Equal(*std_fst1.GetFst<StdArc>(), *std_fst2.GetFst<StdArc>()));
  EXPECT_FALSE(Equal(*std_fst1.GetFst<StdArc>(), MakeChannelFst<StdArc>()));

  VectorFstClass log_fst(MakeChannelFst<LogArc>());
  std::mt19937_64 log_gen(42);
  Randomize(log_gen, &log_fst);
  EXPECT_EQ(log_fst.Properties(kError, false), 0);
  EXPECT_FALSE(Equal(*log_fst.GetFst<LogArc>(), MakeChannelFst<LogArc>()));
  const auto* typed_log = log_fst.GetFst<LogArc>();
  for (ArcIterator<Fst<LogArc>> aiter(*typed_log, typed_log->Start());
       !aiter.Done(); aiter.Next()) {
    const float value = aiter.Value().weight.Value();
    EXPECT_GE(value, 0.0f);
    EXPECT_LE(value, -std::log(kDelta) + 1e-5);
  }
}

TEST(RandomizeScriptTest, PreservesExistingErrorProperty) {
  VectorFst<StdArc> original = MakeChannelFst<StdArc>();
  VectorFstClass fst(original);
  fst.SetProperties(kError, kError);

  std::mt19937_64 gen(42);
  Randomize(gen, &fst);

  EXPECT_EQ(fst.Properties(kError, false), kError);
  EXPECT_EQ(fst.GetFst<StdArc>()->Final(0), TropicalWeight::One());
}

TEST(RandomizeScriptTest, UnregisteredArcTypeSetsError) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  VectorFst<UnregisteredArc> typed;
  const auto s0 = typed.AddState();
  typed.SetStart(s0);
  typed.SetFinal(s0, TropicalWeight::One());
  VectorFstClass fst(typed);

  std::mt19937_64 gen(42);
  Randomize(gen, &fst);

  EXPECT_EQ(fst.Properties(kError, false), kError);
}

// -----------------------------------------------------------------------------
// Train script tests
// -----------------------------------------------------------------------------

TEST(TrainScriptTest, TrainsPairModelStdAndLogArc) {
  const std::string std_in_path =
      fst::JoinPath(::testing::TempDir(), "script_train_std_in.far");
  const std::string std_out_path =
      fst::JoinPath(::testing::TempDir(), "script_train_std_out.far");
  WriteStTableFar<StdArc>(std_in_path, {"1 2"});
  WriteStTableFar<StdArc>(std_out_path, {"10 20"});

  auto std_in = FarReaderClass::Open(std_in_path);
  auto std_out = FarReaderClass::Open(std_out_path);
  ASSERT_NE(std_in, nullptr);
  ASSERT_NE(std_out, nullptr);

  VectorFstClass std_model(MakeChannelFst<StdArc>());
  Train(*std_in, *std_out, &std_model, /*normalize_ilabel=*/true,
        TrainOptions(/*max_iters=*/5));
  EXPECT_EQ(std_model.Properties(kError, false), 0);

  const std::string log_in_path =
      fst::JoinPath(::testing::TempDir(), "script_train_log_in.far");
  const std::string log_out_path =
      fst::JoinPath(::testing::TempDir(), "script_train_log_out.far");
  WriteStTableFar<LogArc>(log_in_path, {"1 2"});
  WriteStTableFar<LogArc>(log_out_path, {"10 20"});

  auto log_in = FarReaderClass::Open(log_in_path);
  auto log_out = FarReaderClass::Open(log_out_path);
  ASSERT_NE(log_in, nullptr);
  ASSERT_NE(log_out, nullptr);

  VectorFstClass log_model(MakeChannelFst<LogArc>());
  Train(*log_in, *log_out, &log_model, /*normalize_ilabel=*/true,
        TrainOptions(/*max_iters=*/5));
  EXPECT_EQ(log_model.Properties(kError, false), 0);
}

TEST(TrainScriptTest, ArcTypeMismatchSetsModelError) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  const std::string std_path =
      fst::JoinPath(::testing::TempDir(), "script_train_mismatch_std.far");
  const std::string log_path =
      fst::JoinPath(::testing::TempDir(), "script_train_mismatch_log.far");
  WriteStTableFar<StdArc>(std_path, {"1 2"});
  WriteStTableFar<LogArc>(log_path, {"10 20"});

  {
    auto log_in = FarReaderClass::Open(log_path);
    auto std_out = FarReaderClass::Open(std_path);
    ASSERT_NE(log_in, nullptr);
    ASSERT_NE(std_out, nullptr);
    VectorFstClass std_model(MakeChannelFst<StdArc>());
    Train(*log_in, *std_out, &std_model);
    EXPECT_EQ(std_model.Properties(kError, false), kError);
  }
  {
    auto std_in = FarReaderClass::Open(std_path);
    auto log_out = FarReaderClass::Open(log_path);
    ASSERT_NE(std_in, nullptr);
    ASSERT_NE(log_out, nullptr);
    VectorFstClass std_model(MakeChannelFst<StdArc>());
    Train(*std_in, *log_out, &std_model);
    EXPECT_EQ(std_model.Properties(kError, false), kError);
  }
}

TEST(TrainScriptTest, PreservesExistingModelError) {
  const std::string in_path =
      fst::JoinPath(::testing::TempDir(), "script_train_err_in.far");
  const std::string out_path =
      fst::JoinPath(::testing::TempDir(), "script_train_err_out.far");
  WriteStTableFar<StdArc>(in_path, {"1 2"});
  WriteStTableFar<StdArc>(out_path, {"10 20"});

  auto in = FarReaderClass::Open(in_path);
  auto out = FarReaderClass::Open(out_path);
  ASSERT_NE(in, nullptr);
  ASSERT_NE(out, nullptr);

  VectorFstClass model(MakeChannelFst<StdArc>());
  model.SetProperties(kError, kError);
  Train(*in, *out, &model);
  EXPECT_EQ(model.Properties(kError, false), kError);
}

// -----------------------------------------------------------------------------
// Decode script tests
// -----------------------------------------------------------------------------

TEST(DecodeScriptTest, DecodesDeciphermentStdArc) {
  const std::string lm_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_lm.fst");
  const std::string cipher_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_cipher.far");
  const std::string hypo_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_hypo.far");

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
  {
    std::unique_ptr<FstFarWriter<StdArc>> lm_writer(
        FstFarWriter<StdArc>::Create(lm_path));
    ASSERT_NE(lm_writer, nullptr);
    lm_writer->Add("lm", lm);
  }
  WriteStTableFar<StdArc>(cipher_path, {"10 20"});

  auto lm_reader = FarReaderClass::Open(lm_path);
  auto cipher_reader = FarReaderClass::Open(cipher_path);
  ASSERT_NE(lm_reader, nullptr);
  ASSERT_NE(cipher_reader, nullptr);

  const FstClass model(MakeChannelFst<StdArc>());
  {
    auto hypo_writer =
        FarWriterClass::Create(hypo_path, "standard", FarType::STTABLE);
    ASSERT_NE(hypo_writer, nullptr);
    EXPECT_TRUE(Decode(*lm_reader, *cipher_reader, model, *hypo_writer));
  }

  auto hypo_reader = FarReaderClass::Open(hypo_path);
  ASSERT_NE(hypo_reader, nullptr);
  ASSERT_FALSE(hypo_reader->Done());
  const auto* decoded = hypo_reader->GetFstClass()->GetFst<StdArc>();
  ASSERT_NE(decoded, nullptr);
  std::string decoded_str;
  ASSERT_TRUE(StringPrinter<StdArc>(TokenType::SYMBOL)(
      ProjectFst<StdArc>(*decoded, ProjectType::INPUT), &decoded_str));
  EXPECT_EQ(decoded_str, "1 2");
}

TEST(DecodeScriptTest, DecodesPairWithEncodeMapper) {
  const std::string in_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_pair_in.far");
  const std::string out_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_pair_out.far");
  const std::string hypo_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_pair_hypo.far");
  WriteStTableFar<StdArc>(in_path, {"1 2"});
  WriteStTableFar<StdArc>(out_path, {"10 20"});

  auto in_reader = FarReaderClass::Open(in_path);
  auto out_reader = FarReaderClass::Open(out_path);
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  const FstClass model(MakeChannelFst<StdArc>());
  EncodeMapperClass encoder("standard", kEncodeLabels);
  {
    auto hypo_writer =
        FarWriterClass::Create(hypo_path, "standard", FarType::STTABLE);
    ASSERT_NE(hypo_writer, nullptr);
    EXPECT_TRUE(Decode(*in_reader, *out_reader, model, *hypo_writer, &encoder));
  }

  auto hypo_reader = FarReaderClass::Open(hypo_path);
  ASSERT_NE(hypo_reader, nullptr);
  ASSERT_FALSE(hypo_reader->Done());
  EXPECT_EQ(hypo_reader->GetKey(), "0_0");
  const auto* encoded = hypo_reader->GetFstClass()->GetFst<StdArc>();
  ASSERT_NE(encoded, nullptr);
  for (StateIterator<Fst<StdArc>> siter(*encoded); !siter.Done();
       siter.Next()) {
    for (ArcIterator<Fst<StdArc>> aiter(*encoded, siter.Value()); !aiter.Done();
         aiter.Next()) {
      EXPECT_EQ(aiter.Value().ilabel, aiter.Value().olabel);
      EXPECT_EQ(aiter.Value().weight, TropicalWeight::One());
    }
  }
}

TEST(DecodeScriptTest, ReturnsFalseOnArcTypeMismatch) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  const std::string std_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_mismatch_std.far");
  const std::string log_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_mismatch_log.far");
  const std::string hypo_std_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_hypo_std.far");
  const std::string hypo_log_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_hypo_log.far");
  WriteStTableFar<StdArc>(std_path, {"1 2"});
  WriteStTableFar<LogArc>(log_path, {"10 20"});

  const FstClass std_model(MakeChannelFst<StdArc>());

  // Input mismatch.
  {
    auto log_in = FarReaderClass::Open(log_path);
    auto std_out = FarReaderClass::Open(std_path);
    auto hypo =
        FarWriterClass::Create(hypo_std_path, "standard", FarType::STTABLE);
    ASSERT_NE(log_in, nullptr);
    ASSERT_NE(std_out, nullptr);
    ASSERT_NE(hypo, nullptr);
    EXPECT_FALSE(Decode(*log_in, *std_out, std_model, *hypo));
  }
  // Output mismatch.
  {
    auto std_in = FarReaderClass::Open(std_path);
    auto log_out = FarReaderClass::Open(log_path);
    auto hypo =
        FarWriterClass::Create(hypo_std_path, "standard", FarType::STTABLE);
    ASSERT_NE(std_in, nullptr);
    ASSERT_NE(log_out, nullptr);
    ASSERT_NE(hypo, nullptr);
    EXPECT_FALSE(Decode(*std_in, *log_out, std_model, *hypo));
  }
  // Hypotext writer mismatch.
  {
    auto std_in = FarReaderClass::Open(std_path);
    auto std_out = FarReaderClass::Open(std_path);
    auto hypo = FarWriterClass::Create(hypo_log_path, "log", FarType::STTABLE);
    ASSERT_NE(std_in, nullptr);
    ASSERT_NE(std_out, nullptr);
    ASSERT_NE(hypo, nullptr);
    EXPECT_FALSE(Decode(*std_in, *std_out, std_model, *hypo));
  }
  // EncodeMapper mismatch.
  {
    auto std_in = FarReaderClass::Open(std_path);
    auto std_out = FarReaderClass::Open(std_path);
    auto hypo =
        FarWriterClass::Create(hypo_std_path, "standard", FarType::STTABLE);
    EncodeMapperClass log_encoder("log", kEncodeLabels);
    ASSERT_NE(std_in, nullptr);
    ASSERT_NE(std_out, nullptr);
    ASSERT_NE(hypo, nullptr);
    EXPECT_FALSE(Decode(*std_in, *std_out, std_model, *hypo, &log_encoder));
  }
}

TEST(DecodeScriptTest, ReturnsFalseWhenModelHasError) {
  const std::string in_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_err_in.far");
  const std::string out_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_err_out.far");
  const std::string hypo_path =
      fst::JoinPath(::testing::TempDir(), "script_decode_err_hypo.far");
  WriteStTableFar<StdArc>(in_path, {"1 2"});
  WriteStTableFar<StdArc>(out_path, {"10 20"});

  auto in = FarReaderClass::Open(in_path);
  auto out = FarReaderClass::Open(out_path);
  auto hypo = FarWriterClass::Create(hypo_path, "standard", FarType::STTABLE);
  ASSERT_NE(in, nullptr);
  ASSERT_NE(out, nullptr);
  ASSERT_NE(hypo, nullptr);

  VectorFst<StdArc> err_model = MakeChannelFst<StdArc>();
  err_model.SetProperties(kError, kError);
  EXPECT_FALSE(Decode(*in, *out, FstClass(err_model), *hypo));
}

}  // namespace
}  // namespace script
}  // namespace fst
