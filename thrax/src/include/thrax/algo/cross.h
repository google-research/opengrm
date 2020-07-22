#ifndef FST_UTIL_OPERATORS_CROSS_H_
#define FST_UTIL_OPERATORS_CROSS_H_

#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <fst/rmepsilon.h>

namespace fst {

// This function combines two acceptors into a cross-product transducer; that
// if U accepts V_U and L accepts V_L, then their cross-product U x L accepts
// \forall v_u \in V_U, v_l \in V_L: v_u \rightarrow v_r. If called with a
// transducer for the first argument (the upper language), it will act as if it
// had already been projected onto its input, and if called with a transducer
// for the second argument (the lower language), it will act as if it had
// already been projected onto its output.
template <class Arc>
void Cross(const Fst<Arc> &ifst1, const Fst<Arc> &ifst2,
           MutableFst<Arc> *ofst) {
  static const ComposeOptions opts(/*connect=*/true,
                                   /*filter_type=*/MATCH_FILTER);
  static const OutputEpsilonMapper<Arc> oeps;
  static const InputEpsilonMapper<Arc> ieps;
  Compose(RmEpsilonFst<Arc>(MakeArcMapFst(ifst1, oeps)),
          RmEpsilonFst<Arc>(MakeArcMapFst(ifst2, ieps)), ofst, opts);
  // Copies symbol tables (if present).
  ofst->SetInputSymbols(ifst1.InputSymbols());
  ofst->SetOutputSymbols(ifst2.OutputSymbols());
}

}  // namespace fst

#endif  // FST_UTIL_OPERATORS_CROSS_H_
