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

// Decodes Baum-Welch model.

#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/util.h"
#include "openfst/script/encodemapper-class.h"
#include "openfst/script/fst-class.h"
#include "opengrm/baumwelch/decodescript.h"

int baumwelchdecode_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::kEncodeLabels;
  using fst::script::EncodeMapperClass;
  using fst::script::FarReaderClass;
  using fst::script::FarWriterClass;
  using fst::script::FstClass;

  std::string usage = "Decodes a WFST model\n\n  Usage: ";
  usage += argv[0];
  usage += " input.f(ar|st) output.far model.fst out.far [enc.map]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 4 || argc > 6) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string input_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string output_name = strcmp(argv[2], "-") != 0 ? argv[2] : "";
  const std::string model_name = strcmp(argv[3], "-") != 0 ? argv[3] : "";
  const std::string hypotext_name = argc > 4 ? argv[4] : "";
  const std::string encodemapper_name = argc > 5 ? argv[5] : "";

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

  const std::unique_ptr<const FstClass> model(FstClass::Read(model_name));
  if (!model) return 1;

  const std::unique_ptr<FarWriterClass> hypotext(
      FarWriterClass::Create(hypotext_name, output->ArcType()));
  if (!hypotext) return 1;

  if (encodemapper_name.empty()) {
    s::Decode(*input, *output, *model, *hypotext);
  } else {
    s::EncodeMapperClass mapper(model->ArcType(), kEncodeLabels);
    s::Decode(*input, *output, *model, *hypotext, &mapper);
    if (!mapper.Write(encodemapper_name)) return 1;
  }

  if (input->Error()) {
    FSTERROR() << "Error reading FAR: " << input_name;
    return 1;
  }
  if (output->Error()) {
    FSTERROR() << "Error reading FAR: " << output_name;
    return 1;
  }
  if (hypotext->Error()) {
    FSTERROR() << "Error writing FAR: " << hypotext_name;
    return 1;
  }

  return 0;
}
