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

// This defines a helper which runs decipherment experiments with two
// training settings and four decoding settings.

#ifndef OPENGRM_BAUMWELCH_DECIPHERMENT_TESTER_H_
#define OPENGRM_BAUMWELCH_DECIPHERMENT_TESTER_H_

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/fst-far-reader.h"
#include "openfst/extensions/far/fst-far-writer.h"
#include "openfst/extensions/far/sttable-far-reader.h"
#include "openfst/extensions/far/sttable-far-writer.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/baumwelch/decode.h"
#include "opengrm/baumwelch/randomize.h"
#include "opengrm/baumwelch/score.h"
#include "opengrm/baumwelch/train.h"

namespace fst {
namespace internal {

class TempFile {
 public:
  explicit TempFile(absl::string_view p) : path_(std::move(p)) {}
  ~TempFile() { std::remove(path_.c_str()); }

 private:
  const std::string path_;
};

}  // namespace internal

template <class Arc>
void DeciphermentTests(const Fst<Arc>& lm, FarReader<Arc>& output,
                       const Fst<Arc>& model, const TrainOptions& opts,
                       FarReader<Arc>& goldtext) {
  // Mocks a FarReader for the LM.
  const std::string input_path =
      fst::JoinPath(::testing::TempDir(), "input.far");
  internal::TempFile input_far(input_path);
  {
    std::unique_ptr<FstFarWriter<Arc>> writer(
        FstFarWriter<Arc>::Create(input_path));
    ASSERT_NE(writer, nullptr) << "Create failed: " << input_path;
    writer->Add("LM", lm);
  }
  std::unique_ptr<FstFarReader<Arc>> input(FstFarReader<Arc>::Open(input_path));
  ASSERT_NE(input, nullptr) << "Create failed: " << input_path;

  // Trains the model.
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "DeciphermentTests"));
  VectorFst<Arc> trained_model(model);
  Randomize(bit_gen, &trained_model);
  Train(*input, output, &trained_model, /*normalize_ilabel=*/true, opts);
  input->Reset();
  output.Reset();

  // Mocks a FAR for the hypothesized decipherment.
  const std::string hypotext_path =
      fst::JoinPath(::testing::TempDir(), "hypotext.far");
  internal::TempFile hypotext_far(hypotext_path);
  std::unique_ptr<STTableFarWriter<Arc>> hypotext_writer;
  std::unique_ptr<STTableFarReader<Arc>> hypotext_reader;

  // Decodes with a normal model and LM.
  hypotext_writer.reset(STTableFarWriter<Arc>::Create(hypotext_path));
  ASSERT_NE(hypotext_writer, nullptr);
  Decode(*input, output, trained_model, *hypotext_writer);
  hypotext_writer.reset();  // Flushes.
  hypotext_reader.reset(STTableFarReader<Arc>::Open(hypotext_path));
  ASSERT_NE(hypotext_reader, nullptr);
  LOG(INFO) << "Normal: " << HammingDistance(goldtext, *hypotext_reader)
            << " error(s)";
  input->Reset();
  output.Reset();

  // Decodes with a penalized model and a normal LM.
  hypotext_writer.reset(STTableFarWriter<Arc>::Create(hypotext_path));
  ASSERT_NE(hypotext_writer, nullptr);
  // Penalizes the model.
  static const PowerMapper<Arc> pmapper(3.0);
  ArcMap(&trained_model, pmapper);
  Decode(*input, output, trained_model, *hypotext_writer);
  hypotext_writer.reset();  // Flushes.
  hypotext_reader.reset(STTableFarReader<Arc>::Open(hypotext_path));
  ASSERT_NE(hypotext_reader, nullptr);
  LOG(INFO) << "Penalized model: "
            << HammingDistance(goldtext, *hypotext_reader) << " error(s)";
}

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_DECIPHERMENT_TESTER_H_
