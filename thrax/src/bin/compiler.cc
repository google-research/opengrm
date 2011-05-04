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
// Author: wojciech@google.com (Wojciech Skut)
//         ttai@google.com (Terry Tai)
//
// Main compiler entry point.  Compiles a grm source file to the FST archive.
// Returns zero on success and non-zero on failure.

#include <string>

#include <thrax/compat/utils.h>
#include <thrax/grm-compiler.h>
#include <thrax/grm-manager.h>
#include <thrax/function.h>

DEFINE_string(input_grammar, "", "Name of the grammar file.");
DEFINE_string(output_far, "", "Location to write the FST archive.");

using thrax::GrmCompiler;
using thrax::GrmManager;

int main(int argc, char **argv) {
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  thrax::function::RegisterFunctions();

  GrmCompiler grammar;
  if (grammar.ParseFile(FLAGS_input_grammar) &&
      grammar.EvaluateAst()) {
    const GrmManager* manager = grammar.GetGrmManager();
    manager->ExportFar(FLAGS_output_far);
    return 0;
  } else {
    return 1;
  }
}
