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
//         rws@google.com (Richard Sproat)
//
// Stand-alone binary to load up a FAR and rewrite some strings.

#include <iostream>
#include <string>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <fst/vector-fst.h>
#include <thrax/grm-manager.h>
#include <thrax/symbols.h>

using fst::StdArc;
using fst::StringCompiler;
using fst::SymbolTable;
using fst::VectorFst;
using thrax::GrmManagerSpec;
using thrax::SplitStringUsing;

typedef StringCompiler<StdArc> Compiler;
typedef VectorFst<StdArc> Transducer;

DEFINE_string(far, "", "Path to the FAR.");
DEFINE_string(rules, "", "Names of the rewrite rules.");
DEFINE_string(input_mode, "byte", "Either \"byte\", \"utf8\", or the path to a "
              "symbol table for input parsing.");
DEFINE_string(output_mode, "byte", "Either \"byte\", \"utf8\", or the path to "
              "a symbol table for input parsing.");
DEFINE_int64(noutput, 1, "Maximum number of output strings for each input.");

enum TokenType { SYMBOL = 1, BYTE = 2, UTF8 = 3 };

bool ReadInput(string* s) {
  cout << "Input string: ";
  return static_cast<bool>(getline(cin, *s));
}

bool VisitState(const Transducer& fst,
                TokenType type,
                SymbolTable* symtab,
                StdArc::StateId state,
                const string& path,
                float cost,
                vector<pair<string, float> >* paths) {
  if (fst.Final(state) != StdArc::Weight::Zero() &&
      !path.empty()) {
    paths->push_back(make_pair(path, cost + fst.Final(state).Value()));
  }
  fst::ArcIterator<Transducer> aiter(fst, state);
  for (; !aiter.Done(); aiter.Next()) {
    const StdArc &arc = aiter.Value();
    string newpath = path;
    if (arc.olabel != 0) {
      if (type == SYMBOL) {
        string sym = symtab->Find(arc.olabel);
        if (sym == "") {
          LOG(ERROR) << "Missing symbol in symbol table for id: " << arc.olabel;
          return false;
        }
        newpath += sym;
      } else if (type == BYTE) {
        newpath.push_back(arc.olabel);
      } else if (type == UTF8) {
        string utf8_string;
        vector<int> labels;
        labels.push_back(arc.olabel);
        if (!fst::LabelsToUTF8String(labels, &utf8_string)) {
          LOG(ERROR) << "LabelsToUTF8String: Bad code point: "
                     << arc.olabel;
          return false;
        }
        newpath += utf8_string;
      }
    }
    if (!VisitState(fst, type, symtab, arc.nextstate, newpath,
                    cost + arc.weight.Value(), paths)) {
      return false;
    }
  }
  return true;
}

bool FstToStrings(const Transducer& fst,
                  vector<pair<string, float> >* strings,
                  TokenType type = BYTE,
                  SymbolTable* symtab = NULL,
                  size_t n = 1) {
  Transducer temp;
  fst::ShortestPath(fst, &temp, n);
  fst::Project(&temp, fst::PROJECT_OUTPUT);
  fst::RmEpsilon(&temp);
  if (temp.Start() == fst::kNoStateId) {
    return false;
  }
  return VisitState(temp, type, symtab, temp.Start(), string(), 0, strings);
}

int main(int argc, char** argv) {
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  GrmManagerSpec<StdArc> grm;
  CHECK(grm.LoadArchive(FLAGS_far));

  vector<string> rules;
  SplitStringUsing(FLAGS_rules, ", \t", &rules);
  const fst::SymbolTable* byte_symtab = NULL;
  const fst::SymbolTable* utf8_symtab = NULL;
  for (size_t i = 0; i < rules.size(); ++i) {
    fst::Fst<StdArc>* fst = grm.GetFst(rules[i]);
    if (!fst) {
      LOG(FATAL) << "grm.GetFst() must be non NULL for rule: " << rules[i];
    }
    Transducer vfst(*fst);
    // If the input transducers in the FAR have symbol tables then we need to
    // add the appropriate symbol table(s) to the input strings, according to
    // the parse mode.
    if (vfst.InputSymbols()) {
      if (!byte_symtab &&
          vfst.InputSymbols()->Name() ==
          thrax::function::kByteSymbolTableName) {
        byte_symtab = vfst.InputSymbols()->Copy();
      } else if (!utf8_symtab &&
                 vfst.InputSymbols()->Name() ==
                 thrax::function::kUtf8SymbolTableName) {
        utf8_symtab = vfst.InputSymbols()->Copy();
      }
    }
  }

  Compiler* compiler = NULL;
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

  TokenType type;
  SymbolTable* output_symtab = NULL;
  if (FLAGS_output_mode == "byte") {
    type = BYTE;
  } else if (FLAGS_output_mode == "utf8") {
    type = UTF8;
  } else {
    type = SYMBOL;
    output_symtab = SymbolTable::ReadText(FLAGS_output_mode);
    CHECK(output_symtab)
        ;
  }

  string input;
  while (ReadInput(&input)) {
    Transducer input_fst, output_fst;
    if (!compiler->operator()(input, &input_fst)) {
      cout << "Unable to parse input string." << endl;
      continue;
    }
    // Set symbols for the input, if appropriate
    if (byte_symtab && type == BYTE) {
      input_fst.SetInputSymbols(byte_symtab);
      input_fst.SetOutputSymbols(byte_symtab);
    } else if (utf8_symtab && type == UTF8) {
      input_fst.SetInputSymbols(utf8_symtab);
      input_fst.SetOutputSymbols(utf8_symtab);
    } else if (input_symtab && type == SYMBOL) {
      input_fst.SetInputSymbols(input_symtab);
      input_fst.SetOutputSymbols(input_symtab);
    }

    bool succeeded = true;
    for (size_t i = 0; i < rules.size(); ++i) {
      if (grm.Rewrite(rules[i], input_fst, &output_fst)) {
        input_fst = output_fst;
      } else {
        succeeded = false;
      }
    }

    vector<pair<string, float> > strings;
    if (succeeded && FstToStrings(output_fst, &strings, type,
                                  output_symtab, FLAGS_noutput)) {
      vector<pair<string, float> >::iterator itr = strings.begin();
      for (; itr != strings.end(); ++itr) {
        cout << "Output string: " << itr->first;
        if (FLAGS_noutput != 1 && itr->second != 0) {
          cout << " <cost=" << itr->second << ">";
        }
        cout << endl;
      }
    } else {
      cout << "Rewrite failed." << endl;
    }
  }

  delete compiler;
  delete input_symtab;
  delete output_symtab;
  delete byte_symtab;
  delete utf8_symtab;

  return 0;
}
