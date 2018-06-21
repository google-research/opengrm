
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2018 Google, Inc.
// sfsttrim.cc

// Removes useless states and transitions in stochastic automata.

#include <string.h>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/fst-decl.h>
#include <fst/mutable-fst.h>
#include <sfst/trim.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_bool(phi_trim, true,
            "Removes inaccessible transitions due to failure labels");
DEFINE_bool(weight_trim, false,
            "Removes ApproxZero() weight transitions");
DEFINE_bool(include_phi, false,
            "Include phi transitions when weight trimming");
DEFINE_bool(connect, true,
            "Removes inaccessible/non-accessible states treating"
            " failure labels as regular labels");
DEFINE_string(trim_type, "needed_final", "Trim type, one of: "
              "\"needed_trim\", \"needed_final\", "
              "\"needed_nonfinal");


int main(int argc, char **argv) {
  namespace f = fst;
  string usage = "Removes useless states and transitions in stochastic ";
  usage += " automata.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  f::StdMutableFst *fst = f::StdMutableFst::Read(in_name, true);
  if (!fst) return 1;

  sfst::TrimType trim_type;
  if (FLAGS_trim_type == "needed_trim") {
    trim_type = sfst::TRIM_NEEDED_TRIM;
  } else if (FLAGS_trim_type == "needed_final") {
    trim_type = sfst::TRIM_NEEDED_FINAL;
  } else if (FLAGS_trim_type == "needed_nonfinal") {
    trim_type = sfst::TRIM_NEEDED_NONFINAL;
  } else {
    LOG(ERROR) << argv[0] << ": Bad trim type: " << FLAGS_trim_type;
    return 1;
  }

  sfst::Trimmer<f::StdArc> trim(fst, FLAGS_phi_label, trim_type);

  if (FLAGS_phi_trim)
    trim.PhiTrim();

  if (FLAGS_weight_trim)
    trim.WeightTrim(FLAGS_include_phi);

  if (FLAGS_connect)
    trim.Connect();

  trim.Finalize();

  if (fst->Properties(f::kError, false)) {
    LOG(ERROR) << argv[0] << ": trimming failed";
    return 2;
  }

  if (!fst->Write(out_name))
    return 1;

  return 0;
}
