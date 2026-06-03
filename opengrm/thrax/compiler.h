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

#ifndef OPENGRM_THRAX_COMPILER_H_
#define OPENGRM_THRAX_COMPILER_H_

#include <string>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/grm-compiler.h"
#include "opengrm/thrax/grm-manager.h"

ABSL_DECLARE_FLAG(std::string, indir);

namespace thrax {

template <typename Arc>
bool CompileGrammar(absl::string_view input_grammar,
                    absl::string_view output_far, bool emit_ast_only,
                    bool line_numbers_in_ast) {
  GrmCompilerSpec<Arc> grammar;
  if (!grammar.ParseFile(
          fst::JoinPath(absl::GetFlag(FLAGS_indir), input_grammar))) {
    return false;
  }
  if (emit_ast_only) {
    return grammar.PrintAst(line_numbers_in_ast);
  } else if (grammar.EvaluateAst()) {
    const GrmManagerSpec<Arc>* manager = grammar.GetGrmManager();
    manager->ExportFar(std::string(output_far));
    return true;
  }
  return false;
}

extern template bool CompileGrammar<::fst::StdArc>(absl::string_view,
                                                   absl::string_view, bool,
                                                   bool);

extern template bool CompileGrammar<::fst::LogArc>(absl::string_view,
                                                   absl::string_view, bool,
                                                   bool);

extern template bool CompileGrammar<::fst::Log64Arc>(absl::string_view,
                                                     absl::string_view, bool,
                                                     bool);

}  // namespace thrax

#endif  // OPENGRM_THRAX_COMPILER_H_
