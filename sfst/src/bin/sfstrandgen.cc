
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
// sfstrandgen.cc
//
// \file
// Generates random paths through a stochastic FST.
// The FST must be a normalized.

#include <limits.h>
#include <string.h>
#include <time.h>
#include <string>

#include <fst/log.h>
#include <fst/randgen.h>
#include <sfst/normalize.h>
#include <sfst/randgen.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_int32(max_length, INT_MAX, "Maximum path length");
DEFINE_int64(npath, 1, "Number of paths to generate");
DEFINE_int32(seed, time(0), "Random seed");
DEFINE_bool(weighted, false,
            "Output tree weighted by path count vs. unweighted paths");
DEFINE_bool(remove_total_weight, false,
            "Remove total weight when output weighted");

int main(int argc, char **argv) {
  namespace f = fst;
  string usage = "Generates random paths through an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  VLOG(1) << argv[0] << ": Seed = " << FLAGS_seed;

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  f::StdFst *ifst = f::StdFst::Read(in_name);
  if (!ifst) return 1;

  if (!sfst::IsNormalized(*ifst, FLAGS_phi_label)) {
    LOG(ERROR) << argv[0] << ": Input is not a normalized stochastic FST";
    return 1;
  }

  f::StdVectorFst ofst;

  sfst::SFstArcSelector<f::StdArc> selector(FLAGS_seed, FLAGS_phi_label);
  f::RandGenOptions<sfst::SFstArcSelector<f::StdArc>>
      opts(selector, FLAGS_max_length, FLAGS_npath, FLAGS_weighted,
           FLAGS_remove_total_weight);
  f::RandGen(*ifst, &ofst, opts);
  if (!ofst.Write(out_name))
    return 1;

  return 0;
}
