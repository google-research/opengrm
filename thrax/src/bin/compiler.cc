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
// Main compiler entry point, compiling a GRM source file into an FST archive.

#include <thrax/compiler.h>

#include <thrax/compat/utils.h>
#include <thrax/grm-compiler.h>
#include <thrax/grm-manager.h>
#include <thrax/function.h>

DEFINE_string(input_grammar, "", "Path to the grammar file");
DEFINE_string(output_far, "", "Path for write the FST archive");
DEFINE_string(arc_type, "standard", "Arc type for compiled FSTs");
DEFINE_bool(emit_ast_only,
            false,
            "Parse the input, write its AST to stdout, and exit without "
            "writing an FST archive");

using ::thrax::CompileGrammar;

int main(int argc, char **argv) {
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(argv[0], &argc, &argv, true);

  thrax::function::RegisterFunctions();
  if (FST_FLAGS_arc_type == "standard") {
    if (CompileGrammar<::fst::StdArc>(
            FST_FLAGS_input_grammar, FST_FLAGS_output_far,
            FST_FLAGS_emit_ast_only,
            FST_FLAGS_line_numbers_in_ast))
      return 0;
  } else if (FST_FLAGS_arc_type == "log") {
    if (CompileGrammar<::fst::LogArc>(
            FST_FLAGS_input_grammar, FST_FLAGS_output_far,
            FST_FLAGS_emit_ast_only,
            FST_FLAGS_line_numbers_in_ast))
      return 0;
  } else if (FST_FLAGS_arc_type == "log64") {
    if (CompileGrammar<::fst::Log64Arc>(
            FST_FLAGS_input_grammar, FST_FLAGS_output_far,
            FST_FLAGS_emit_ast_only,
            FST_FLAGS_line_numbers_in_ast))
      return 0;
  } else {
    LOG(FATAL) << "Unsupported arc type: " << FST_FLAGS_arc_type;
  }
  return 1;
}
