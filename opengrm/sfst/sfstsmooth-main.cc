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

// Binary to smooth SFST models.

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/sfst/smooth.h"

ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(double, witten_bell_k);
ABSL_DECLARE_FLAG(double, discount_D);
ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(int64_t, bins);

int sfstsmooth_main(int argc, char** argv) {
  std::string usage = "Smooth SFST models.\n\n";
  usage += "  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    std::cerr << usage << "\n";
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = (argc > 2) ? argv[2] : "";

  std::unique_ptr<fst::MutableFst<fst::StdArc>> fst(
      fst::MutableFst<fst::StdArc>::Read(in_name, true));
  if (!fst) {
    LOG(ERROR) << argv[0] << ": Read failed: " << in_name;
    return 1;
  }

  std::string method = absl::GetFlag(FLAGS_method);
  int64_t phi_label = absl::GetFlag(FLAGS_phi_label);
  bool success = false;

  if (method == "witten_bell") {
    success = sfst::WittenBell(fst.get(), phi_label,
                               absl::GetFlag(FLAGS_witten_bell_k));
  } else if (method == "absolute") {
    success = sfst::AbsoluteDiscounting(fst.get(), phi_label,
                                        absl::GetFlag(FLAGS_discount_D));
  } else if (method == "unsmoothed") {
    success = sfst::Unsmoothed(fst.get(), phi_label);
  } else if (method == "kneser_ney") {
    success =
        sfst::KneserNey(fst.get(), phi_label, absl::GetFlag(FLAGS_discount_D));
  } else if (method == "modified_kneser_ney") {
    int bins = absl::GetFlag(FLAGS_bins);
    if (bins <= 0) bins = 3;
    success = sfst::ModifiedKneserNey(fst.get(), phi_label, bins);
  } else if (method == "katz") {
    success = sfst::Katz(fst.get(), phi_label, absl::GetFlag(FLAGS_bins));
  } else if (method == "presmoothed") {
    success = sfst::PreSmoothed(fst.get(), phi_label);
  } else {
    LOG(ERROR) << "Unknown smoothing method: " << method;
    return 1;
  }
  if (!success) {
    LOG(ERROR) << "Smoothing failed";
    return 1;
  }

  if (!fst->Write(out_name)) return 1;

  return 0;
}
