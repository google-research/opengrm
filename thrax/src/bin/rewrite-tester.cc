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
// Stand-alone binary to load up a FAR and rewrite some strings.

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/fst.h>
#include <fst/project.h>
#include <fst/rmepsilon.h>
#include <fst/shortest-path.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <fst/vector-fst.h>
#include <thrax/grm-manager.h>

using fst::StdArc;
using fst::StringCompiler;
using fst::StringPrinter;
using fst::SymbolTable;
using fst::VectorFst;
using thrax::GrmManager;

typedef StringCompiler<StdArc> Compiler;
typedef StringPrinter<StdArc> Printer;
typedef VectorFst<StdArc> Transducer;

DEFINE_string(far, "", "Path to the FAR.");
DEFINE_string(rule, "", "Name of the rewrite rule.");
DEFINE_string(input_mode, "byte", "Either \"byte\", \"utf8\", or the path to a "
              "symbol table for input parsing.");
DEFINE_string(output_mode, "byte", "Either \"byte\", \"utf8\", or the path to "
              "a symbol table for input parsing.");

bool ReadInput(string* s) {
  cout << "Input string: ";
  return static_cast<bool>(getline(cin, *s));
}

bool RewriteOutput(Printer* printer, Transducer* fst,
                   string* output) {
  GrmManager::StringifyFst(fst);
  return printer->operator()(*fst, output);
}

int main(int argc, char** argv) {
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  GrmManager grm;
  CHECK(grm.LoadArchive(FLAGS_far));
  if (!grm.GetFst(FLAGS_rule)) {
    LOG(FATAL) << "grm.GetFst() must be non NULL";
  }

  Compiler* compiler = NULL;
  Printer* printer = NULL;

  SymbolTable* input_symtab = NULL;
  if (FLAGS_input_mode == "byte") {
    compiler = new Compiler(Compiler::BYTE);
  } else if (FLAGS_input_mode == "utf8") {
    compiler = new Compiler(Compiler::UTF8);
  } else {
    input_symtab = SymbolTable::ReadText(FLAGS_input_mode);
    CHECK(input_symtab)
        ;
    compiler = new Compiler(Compiler::SYMBOL, input_symtab);
  }

  SymbolTable* output_symtab = NULL;
  if (FLAGS_output_mode == "byte") {
    printer = new Printer(Printer::BYTE);
  } else if (FLAGS_output_mode == "utf8") {
    printer = new Printer(Printer::UTF8);
  } else {
    output_symtab = SymbolTable::ReadText(FLAGS_output_mode);
    CHECK(output_symtab)
        ;
    printer = new Printer(Printer::SYMBOL, output_symtab);
  }

  string input;
  while (ReadInput(&input)) {
    Transducer input_fst, output_fst;
    if (!compiler->operator()(input, &input_fst)) {
      cout << "Unable to parse input string." << endl;
      continue;
    }
    string output;
    if (grm.Rewrite(FLAGS_rule, input_fst, &output_fst) &&
        RewriteOutput(printer, &output_fst, &output)) {
      cout << "Output string: " << output << endl;
    } else {
      cout << "Rewrite failed." << endl;
    }
  }

  delete compiler;
  delete printer;
  delete input_symtab;
  delete output_symtab;

  return 0;
}
