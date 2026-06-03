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

// Main compiler entry point, compiling a GRM source file into an FST archive.

#include "opengrm/thrax/compiler.h"

#include <string>

#include "openfst/compat/init.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "opengrm/thrax/grm-compiler.h"

ABSL_FLAG(std::string, input_grammar, "", "Path to the grammar file");
ABSL_FLAG(std::string, output_far, "", "Path for write the FST archive");
ABSL_FLAG(std::string, arc_type, "standard", "Arc type for compiled FSTs");
ABSL_FLAG(bool, emit_ast_only, false,
          "Parse the input, write its AST to stdout, and exit without "
          "writing an FST archive");

using ::thrax::CompileGrammar;

int main(int argc, char** argv) {
  fst::InitOpenFst(argv[0], &argc, &argv, true);

  if (absl::GetFlag(FLAGS_arc_type) == "standard") {
    if (CompileGrammar<::fst::StdArc>(absl::GetFlag(FLAGS_input_grammar),
                                      absl::GetFlag(FLAGS_output_far),
                                      absl::GetFlag(FLAGS_emit_ast_only),
                                      absl::GetFlag(FLAGS_line_numbers_in_ast)))
      return 0;
  } else if (absl::GetFlag(FLAGS_arc_type) == "log") {
    if (CompileGrammar<::fst::LogArc>(absl::GetFlag(FLAGS_input_grammar),
                                      absl::GetFlag(FLAGS_output_far),
                                      absl::GetFlag(FLAGS_emit_ast_only),
                                      absl::GetFlag(FLAGS_line_numbers_in_ast)))
      return 0;
  } else if (absl::GetFlag(FLAGS_arc_type) == "log64") {
    if (CompileGrammar<::fst::Log64Arc>(
            absl::GetFlag(FLAGS_input_grammar), absl::GetFlag(FLAGS_output_far),
            absl::GetFlag(FLAGS_emit_ast_only),
            absl::GetFlag(FLAGS_line_numbers_in_ast)))
      return 0;
  } else {
    LOG(FATAL) << "Unsupported arc type: " << absl::GetFlag(FLAGS_arc_type);
  }
  return 1;
}
