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
// Copyright 2005-2011 Google, Inc.
// Author: ttai@google.com (Terry Tai)
//
// Wrapper for the context dependent rewriter.  Arguments should be, in order,
// 1.) The rewrite transducer (tau).
// 2.) The left context acceptor (lambda).
// 3.) The right context acceptor (rho).
// 4.) The minimized alphabet closure acceptor (sigma*).
// 5.) The string 'ltr', 'rtl', or 'sim' for the direction of rewrite. [opt]
// 6.) The string 'obl' or 'opt' for the rewrite mode. [opt]
//
// If arguments 5 and 6 are omitted, we'll perform a left-to-right and
// obligatory rewrite by default.

#ifndef THRAX_CDREWRITE_H_
#define THRAX_CDREWRITE_H_

#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fst.h>
#include <fst/vector-fst.h>
#include <thrax/algo/context_dependent_rewrite.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/compat/stlfunctions.h>

DECLARE_int64(initial_boundary_marker);
DECLARE_int64(final_boundary_marker);

namespace thrax {
namespace function {

template <typename Arc>
class CDRewrite : public Function<Arc> {
 public:
  typedef fst::Fst<Arc> Transducer;
  typedef fst::VectorFst<Arc> MutableTransducer;

  CDRewrite() : Function<Arc>() {}
  virtual ~CDRewrite() {}

  virtual DataType* Execute(const vector<DataType*>& args) {
    if (args.size() != 4 && args.size() != 6) {
      cout << "CDRewrite: Expected 4 or 6 arguments but received "
           << args.size() << endl;
      return NULL;
    }
    for (int i = 0; i < 4; ++i) {
      if (!args[i]->is<Transducer*>()) {
        cout << "CDRewrite: Expect FST for argument " << i + 1 << endl;
        return NULL;
      }
    }

    const Transducer* tau = *args[0]->get<Transducer*>();
    const Transducer* lambda = *args[1]->get<Transducer*>();
    const Transducer* rho = *args[2]->get<Transducer*>();
    const Transducer* sigma = *args[3]->get<Transducer*>();

    fst::ContextDependentRewriteDirection dir = fst::LEFT_TO_RIGHT;
    fst::ContextDependentRewriteMode mode = fst::OBLIGATORY;
    if (args.size() == 6) {
      for (int i = 4; i < 6; ++i) {
        if (!args[i]->is<string>()) {
          cout << "CDRewrite: Expected string for argument " << i + 1 << endl;
          return NULL;
        }
      }

      const string& direction_str = *args[4]->get<string>();
      if (direction_str == "ltr") {
        dir = fst::LEFT_TO_RIGHT;
      } else if (direction_str == "rtl") {
        dir = fst::RIGHT_TO_LEFT;
      } else if (direction_str == "sim") {
        dir = fst::SIMULTANEOUS;
      } else {
        cout << "CDRewrite: Invalid direction: " << direction_str << endl;
        return NULL;
      }

      const string& mode_str = *args[5]->get<string>();
      if (mode_str == "obl") {
        mode = fst::OBLIGATORY;
      } else if (mode_str == "opt") {
        mode = fst::OPTIONAL;
      } else {
        cout << "CDRewrite: Invalid mode: " << mode_str << endl;
        return NULL;
      }
    }

    MutableTransducer sigma_aug(*sigma);
    MutableTransducer* output = new MutableTransducer();
    AddBoundaryMarkersToSigma(&sigma_aug);
    fst::ContextDependentRewriteCompile(
        *tau, *lambda, *rho, sigma_aug, output, dir, mode);
    MutableTransducer inserter;
    BoundaryInserter(*sigma, &inserter);
    MutableTransducer deleter;
    BoundaryDeleter(*sigma, &deleter);
    MutableTransducer tmp;
    fst::ArcSort(&inserter, fst::OLabelCompare<Arc>());
    fst::Compose(inserter, *output, &tmp);
    fst::ArcSort(&deleter, fst::ILabelCompare<Arc>());
    fst::Compose(tmp, deleter, output);
    return new DataType(output);
  }

 private:
  // Essentially the same functionality as
  // ContextDependentRewriteRule<Arc>::AddMarkersToSigma(), which is a private
  // method of that class. This adds the beginning and ending markers which must
  // be a part of sigma.
  void AddBoundaryMarkersToSigma(MutableTransducer* sigma) {
    for (typename Arc::StateId s = 0; s < sigma->NumStates(); ++s) {
      if (sigma->Final(s) != Arc::Weight::Zero()) {
        sigma->AddArc(s, Arc(FLAGS_initial_boundary_marker,
                             FLAGS_initial_boundary_marker,
                             Arc::Weight::One(), sigma->Start()));
        sigma->AddArc(s, Arc(FLAGS_final_boundary_marker,
                             FLAGS_final_boundary_marker,
                             Arc::Weight::One(), sigma->Start()));
      }
    }
  }

  // Construct transducer that either inserts or deletes boundary markers.
  void HandleBoundaryMarkers(const Transducer& sigma,
                             MutableTransducer* final,
                             bool del) {
    MutableTransducer initial;
    typename Arc::StateId start = initial.AddState();
    typename Arc::StateId end = initial.AddState();
    initial.SetStart(start);
    initial.SetFinal(end, Arc::Weight::One());
    initial.AddArc(start, Arc(del ? FLAGS_initial_boundary_marker : 0,
                              del ? 0 : FLAGS_initial_boundary_marker,
                              Arc::Weight::One(), end));
    start = final->AddState();
    end = final->AddState();
    final->SetStart(start);
    final->SetFinal(end, Arc::Weight::One());
    final->AddArc(start, Arc(del ? FLAGS_final_boundary_marker : 0,
                             del ? 0 : FLAGS_final_boundary_marker,
                             Arc::Weight::One(), end));
    fst::Concat(&initial, sigma);
    fst::Concat(initial, final);
  }

  // Construct epsilon:initial sigma* epsilon:final
  void BoundaryInserter(const Transducer& sigma, MutableTransducer* final) {
    HandleBoundaryMarkers(sigma, final, false);
  }

  // Construct initial:epsilon sigma* final:epsilon
  void BoundaryDeleter(const Transducer& sigma, MutableTransducer* final) {
    HandleBoundaryMarkers(sigma, final, true);
  }

  DISALLOW_COPY_AND_ASSIGN(CDRewrite<Arc>);
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_CDREWRITE_H_
