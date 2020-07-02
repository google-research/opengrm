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
//
// Copyright 2017 and onwards Google, Inc.

// Trains Baum-Welch model.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>

#include <baumwelch/trainscript.h>

DECLARE_int32(batch_size);
DECLARE_double(delta);
DECLARE_double(lr);
DECLARE_int32(max_iters);
DECLARE_bool(normalize_ilabel);

int baumwelchtrain_main(int argc, char **argv) {
  using fst::TrainBaumWelchOptions;
  using fst::script::FarReaderClass;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;
  using fst::script::TrainBaumWelch;

  std::string usage = "Trains a WFST model\n\n  Usage: ";
  usage += argv[0];
  usage += " input.f(ar|st) output.far model.fst [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 4 || argc > 5) {
    ShowUsage();
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

  const TrainBaumWelchOptions opts(/*max_iters=*/FLAGS_max_iters,
                                   /*lr=*/FLAGS_lr,
                                   /*batch_size=*/FLAGS_batch_size,
                                   /*delta=*/FLAGS_delta);

  TrainBaumWelch(input.get(), output.get(), model.get(), FLAGS_normalize_ilabel,
                 opts);

  return !model->Write(out_name);
}

