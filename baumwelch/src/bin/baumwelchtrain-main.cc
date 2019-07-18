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

// Trains Baum-Welch channel model.

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>

#include <fst/flags.h>

#include <baumwelch/trainscript.h>

DECLARE_bool(decipherment);
DECLARE_string(expectation_table);
DECLARE_int32(max_iters);
DECLARE_bool(flat_start);
DECLARE_int32(random_starts);
DECLARE_bool(remove_zero_arcs);
DECLARE_double(delta);
DECLARE_int32(seed);

int baumwelchtrain_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::BaumWelchTrainOptions;
  using fst::ExpectationTableType;
  using fst::script::FarReaderClass;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  std::string usage = "Trains a WFST channel model\n\n  Usage: ";
  usage += argv[0];
  usage += " input.f(ar|st) output.far channel.fst [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 4 || argc > 5) {
    ShowUsage();
    return 1;
  }

  const std::string input_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string output_name = strcmp(argv[2], "-") != 0 ? argv[2] : "";
  const std::string channel_name = strcmp(argv[3], "-") != 0 ? argv[3] : "";
  const std::string out_name = argc > 4 ? argv[4] : "";

  if (input_name.empty() && (output_name.empty() || channel_name.empty())) {
    LOG(ERROR) << argv[0] << ": Can't take more than one input from standard "
               << "input";
    return 1;
  }
  if (output_name.empty() && channel_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take more than one input from standard "
               << "input";
    return 1;
  }

  const std::unique_ptr<FarReaderClass> output(
      FarReaderClass::Open(output_name));
  if (!output) return 1;

  const std::unique_ptr<MutableFstClass> channel(
      MutableFstClass::Read(channel_name));
  if (!channel) return 1;

  ExpectationTableType etype;
  if (!s::GetExpectationTableType(FLAGS_expectation_table, &etype)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported expectation table type: "
               << FLAGS_expectation_table;
    return 1;
  }

  const BaumWelchTrainOptions opts(FLAGS_max_iters, FLAGS_flat_start,
                                   FLAGS_random_starts, FLAGS_remove_zero_arcs,
                                   FLAGS_delta);

  srand(FLAGS_seed);

  if (FLAGS_decipherment) {
    const std::unique_ptr<const FstClass> input(FstClass::Read(input_name));
    if (!input) return 1;

    if (!TrainBaumWelch(*input, output.get(), channel.get(), etype, opts)) {
      LOG(WARNING) << argv[0] << ": Training did not converge";
    }
  } else {
    const std::unique_ptr<FarReaderClass> input(
        FarReaderClass::Open(input_name));
    if (!input) return 1;

    if (!TrainBaumWelch(input.get(), output.get(), channel.get(), etype,
                        opts)) {
      LOG(WARNING) << argv[0] << ": Training did not converge";
    }
  }

  return !channel->Write(out_name);
}

