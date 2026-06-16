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

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "openfst/compat/init.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/arpa.h"

int sfstngramprint_main(int argc, char** argv) {
  std::string usage = "Prints an SFST in ARPA format.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.txt]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    std::cerr << usage << "\n";
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  std::unique_ptr<fst::Fst<fst::StdArc>> fst(
      fst::Fst<fst::StdArc>::Read(in_name));
  if (!fst) {
    LOG(ERROR) << argv[0] << ": Read failed";
    return 1;
  }

  std::ofstream ofstrm;
  if (argc > 2 && (strcmp(argv[2], "-") != 0)) {
    ofstrm.open(argv[2]);
    if (!ofstrm) {
      LOG(ERROR) << argv[0] << ": Open failed: " << argv[2];
      return 1;
    }
  }
  std::ostream& ostrm = ofstrm.is_open() ? ofstrm : std::cout;

  if (!sfst::WriteArpa(*fst, ostrm)) {
    LOG(ERROR) << argv[0] << ": WriteArpa failed";
    return 1;
  }

  return 0;
}
