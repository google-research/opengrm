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

#include "opengrm/operators/cdrewritescript.h"

#include <cstdint>

#include "openfst/lib/properties.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/script-impl.h"
#include "opengrm/operators/cdrewrite.h"

namespace fst {
namespace script {

void CDRewriteCompile(const FstClass& tau, const FstClass& lambda,
                      const FstClass& rho, const FstClass& sigma,
                      MutableFstClass* ofst, CDRewriteDirection dir,
                      CDRewriteMode mode, int64_t initial_boundary_marker,
                      int64_t final_boundary_marker) {
  if (!internal::ArcTypesMatch(tau, lambda, "CDRewriteCompile") ||
      !internal::ArcTypesMatch(lambda, rho, "CDRewriteCompile") ||
      !internal::ArcTypesMatch(rho, sigma, "CDRewriteCompile") ||
      !internal::ArcTypesMatch(sigma, *ofst, "CDRewriteCompile")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  FstCDRewriteCompileArgs args{tau,
                               lambda,
                               rho,
                               sigma,
                               ofst,
                               dir,
                               mode,
                               initial_boundary_marker,
                               final_boundary_marker};
  Apply<Operation<FstCDRewriteCompileArgs>>("CDRewriteCompile", tau.ArcType(),
                                            &args);
}

REGISTER_FST_OPERATION_3ARCS(CDRewriteCompile, FstCDRewriteCompileArgs);

}  // namespace script
}  // namespace fst
