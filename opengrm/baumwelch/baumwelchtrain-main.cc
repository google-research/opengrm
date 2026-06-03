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

// Trains Baum-Welch model.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/lib/util.h"
#include "openfst/script/fst-class.h"
#include "opengrm/baumwelch/train.h"
#include "opengrm/baumwelch/trainscript.h"

ABSL_DECLARE_FLAG(int32_t, batch_size);
ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(double, alpha);
ABSL_DECLARE_FLAG(int32_t, max_iters);
ABSL_DECLARE_FLAG(bool, normalize_ilabel);

int baumwelchtrain_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::TrainOptions;
  using fst::script::FarReaderClass;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  std::string usage = "Trains a WFST model\n\n  Usage: ";
  usage += argv[0];
  usage += " input.f(ar|st) output.far model.fst [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 4 || argc > 5) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string input_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string output_name = strcmp(argv[2], "-") != 0 ? argv[2] : "";
  const std::string model_name = strcmp(argv[3], "-") != 0 ? argv[3] : "";
  const std::string out_name = argc > 4 ? argv[4] : "";

  if (input_name.empty() && (output_name.empty() || model_name.empty())) {
    LOG(ERROR) << argv[0] << ": Can't take more than one input from standard "
               << "input";
    return 1;
  }
  if (output_name.empty() && model_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take more than one input from standard "
               << "input";
    return 1;
  }

  const std::unique_ptr<FarReaderClass> input(FarReaderClass::Open(input_name));
  if (!input) return 1;

  const std::unique_ptr<FarReaderClass> output(
      FarReaderClass::Open(output_name));
  if (!output) return 1;

  const std::unique_ptr<MutableFstClass> model(
      MutableFstClass::Read(model_name));
  if (!model) return 1;

  const TrainOptions opts(
      /*max_iters=*/absl::GetFlag(FLAGS_max_iters),
      /*alpha=*/absl::GetFlag(FLAGS_alpha),
      /*batch_size=*/absl::GetFlag(FLAGS_batch_size),
      /*delta=*/absl::GetFlag(FLAGS_delta));

  s::Train(*input, *output, model.get(), absl::GetFlag(FLAGS_normalize_ilabel),
           opts);

  if (input->Error()) {
    FSTERROR() << "Error reading FAR: " << input_name;
    return 1;
  }
  if (output->Error()) {
    FSTERROR() << "Error reading FAR: " << output_name;
    return 1;
  }

  return !model->Write(out_name);
}
