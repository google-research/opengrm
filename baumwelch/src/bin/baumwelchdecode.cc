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

// Decodes Baum-Welch channel model.

#include <cstring>

#include <memory>
#include <string>
using std::string;

#include <fst/flags.h>

#include <baumwelch/decodescript.h>

DEFINE_bool(decipherment, false,
            "Use decipherment construction; i.e., input is a WFSA rather "
            "than a FAR");

int main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FarReaderClass;
  using fst::script::FarWriterClass;
  using fst::script::FstClass;

  string usage = "Decodes a WFST channel model\n\n  Usage: ";
  usage += argv[0];
  usage += " input.f(ar|st) output.far channel.fst [out.far]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 4 || argc > 5) {
    ShowUsage();
    return 1;
  }

  const string input_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const string output_name = strcmp(argv[2], "-") != 0 ? argv[2] : "";
  const string channel_name = strcmp(argv[3], "-") != 0 ? argv[3] : "";
  const string out_name = argc > 4 ? argv[4] : "";

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

  const std::unique_ptr<const FstClass> channel(FstClass::Read(channel_name));
  if (!channel) return 1;

  const std::unique_ptr<FarWriterClass> out(
      FarWriterClass::Create(out_name, output->ArcType()));
  if (!out) return 1;

  if (FLAGS_decipherment) {
    const std::unique_ptr<const FstClass> input(FstClass::Read(input_name));
    if (!input) return 1;
    s::DecodeBaumWelch(*input, output.get(), *channel, out.get());
  } else {
    const std::unique_ptr<FarReaderClass> input(
        FarReaderClass::Open(input_name));
    if (!input) return 1;
    s::DecodeBaumWelch(input.get(), output.get(), *channel, out.get());
  }

  return out->Error();
}

