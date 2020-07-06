
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
// Intersects two canonical stochastic FSAs.
// The second FSA must be input-epsilon free (when phi_label != 0).

#include <string.h>

#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/mutable-fst.h>
#include <fst/vector-fst.h>
#include <sfst/intersect.h>
#include <sfst/trim.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_bool(trim, true,
             "Removes useless states and transitions");
DEFINE_string(trim_type, "needed_final", "Trim type, one of: "
              "\"needed_trim\", \"needed_final\", "
              "\"needed_nonfinal");

int main(int argc, char **argv) {
  namespace f = fst;
  std::string usage = "Intersects two canonical stochastic FSAs.\n\n Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 4) {
    ShowUsage();
    return 1;
  }

  const std::string in1_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string in2_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";
  const std::string out_name = argc > 3 ? argv[3] : "";

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input";
    return 1;
  }

  f::StdFst *ifst1 = f::StdFst::Read(in1_name);
  if (!ifst1) return 1;

  f::StdFst *ifst2 = f::StdFst::Read(in2_name);
  if (!ifst2) return 1;

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

  f::StdVectorFst ofst;
  if (!sfst::Intersect(*ifst1, *ifst2, &ofst, FLAGS_phi_label,
                       FLAGS_trim, trim_type))
    return 1;

  if (!ofst.Write(out_name))
    return 1;

  return 0;
}
