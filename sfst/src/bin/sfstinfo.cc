
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
// sfstinfo.cc

// Prints out various information about a stochastic FST.

#include <stddef.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/fst-decl.h>
#include <fst/fst.h>
#include <sfst/canonical.h>
#include <sfst/normalize.h>
#include <sfst/trim.h>

DEFINE_double(delta, fst::kDelta, "Comparison delta");
DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");

namespace sfst {

template <class Arc>
void SfstInfo(const fst::Fst<Arc> &fst) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  typedef f::ArcIterator<f::Fst<Arc>> ArcItr;
  typedef f::StateIterator<f::Fst<Arc>> StateItr;

  StateId start = fst.Start();
  StateId nstates = 0;
  size_t narcs = 0;
  size_t nphis = 0;
  size_t nfinal = 0;

  Label phi_label = FLAGS_phi_label;
  bool canonical = IsCanonical(fst, phi_label);
  bool norm = IsNormalized(fst, phi_label, FLAGS_delta);
  bool trim = IsTrim(fst, phi_label);
  std::vector<int> state_order;
  int max_order = 1;
  if (canonical)
    max_order = PhiStateOrder(fst, phi_label, &state_order);
  std::vector<size_t> order_counts(max_order, 0);

  for (StateItr siter(fst); !siter.Done(); siter.Next()) {
    ++nstates;
    StateId s = siter.Value();
    if (fst.Final(s) != Weight::Zero())
      ++nfinal;
    if (canonical)
      ++order_counts[state_order[s] - 1];
    for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
      ++narcs;
      const Arc &arc = aiter.Value();
      if (arc.ilabel == phi_label)
        ++nphis;
    }
  }

  const auto old = std::cout.setf(std::ios::left);
  std::cout.width(50);
  std::cout << "# of states" << nstates << std::endl;
  std::cout.width(50);
  std::cout << "# of arcs" << narcs << std::endl;
  std::cout.width(50);
  std::cout << "# of failure transitions" << nphis << std::endl;
  std::cout.width(50);
  std::cout << "initial state" << start << std::endl;
  std::cout.width(50);
  std::cout << "# of final states" << nfinal << std::endl;
  if (canonical) {
    std::cout.width(50);
    std::cout << "max state order" << max_order << std::endl;
    for (int order = 1; order <= max_order; ++order) {
      std::stringstream label;
      label << "# of order-" << order << " states";
      std::cout.width(50);
      std::cout << label.str() << order_counts[order - 1] << std::endl;
    }
  }
  std::cout.width(50);
  std::cout << "canonical" << (canonical ? 'y' : 'n')
            << std::endl;
  std::cout.width(50);
  std::cout << "trim" << (trim ? 'y' : 'n')
            << std::endl;
  std::cout.width(50);
  std::cout << "normalized" << (norm ? 'y' : 'n')
            << std::endl;
  std::cout.width(50);
  std::cout << "symbols" << (fst.InputSymbols() ? 'y' : 'n') << std::endl;
  std::cout.setf(old);
}

}  // namespace sfst


int main(int argc, char **argv) {
  using fst::StdFst;

  string usage =
      "Prints out information about a stochastic FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  StdFst *ifst = StdFst::Read(in_name);
  if (!ifst) return 1;

  sfst::SfstInfo(*ifst);

  return 0;
}
