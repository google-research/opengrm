// Copyright 2005-2020 Google LLC
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
//
#ifndef NLP_GRM_LANGUAGE_COMPILER_H_
#define NLP_GRM_LANGUAGE_COMPILER_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/grm-compiler.h>
#include <thrax/grm-manager.h>

DECLARE_string(indir);

namespace thrax {

template <typename Arc>
bool CompileGrammar(const std::string& input_grammar,
                    const std::string& output_far, bool emit_ast_only,
                    bool line_numbers_in_ast) {
  GrmCompilerSpec<Arc> grammar;
  if (!grammar.ParseFile(JoinPath(FLAGS_indir, input_grammar))) {
    return false;
  }
  if (emit_ast_only) {
    return grammar.PrintAst(line_numbers_in_ast);
  } else if (grammar.EvaluateAst()) {
    const GrmManagerSpec<Arc>* manager = grammar.GetGrmManager();
    manager->ExportFar(output_far);
    return true;
  }
  return false;
}

extern template bool CompileGrammar<::fst::StdArc>(const std::string&,
                                                       const std::string&, bool,
                                                       bool);

extern template bool CompileGrammar<::fst::LogArc>(const std::string&,
                                                       const std::string&, bool,
                                                       bool);

extern template bool CompileGrammar<::fst::Log64Arc>(const std::string&,
                                                         const std::string&,
                                                         bool, bool);

}  // namespace thrax

#endif  // NLP_GRM_LANGUAGE_COMPILER_H_
