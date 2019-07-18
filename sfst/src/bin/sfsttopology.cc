
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
// sfstnormalize.cc

// Algorithms for constructing specific FST topologies.

#include <string.h>

#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/vector-fst.h>
#include <sfst/topology.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_string(method, "ngram",
              "Specifies topology method, one of: "
              "\"ngram\"");
DEFINE_int64(order, 3, "Set maximal order of ngram model");

int main(int argc, char **argv) {
  namespace f = fst;
  std::string usage =
      "Algorithms for constructing specific FST topologies.\n\n";
  usage += "  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  f::StdFst *ifst = f::StdFst::Read(in_name);
  if (!ifst) return 1;

  f::StdVectorFst ofst;

  if (FLAGS_method == "ngram") {
    sfst::NGramTopology<f::StdArc> ngram(FLAGS_order, FLAGS_phi_label, &ofst);
    ngram.FindNGrams(*ifst);
  } else {
    LOG(ERROR) << argv[0] << ": unknown topology method: "
               << FLAGS_method;
    return 1;
  }

  if (!ofst.Write(out_name))
    return 1;

  return 0;
}
