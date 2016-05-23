
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Compiles and tests equality of HistogramArc models during unit tests.
// Avoid complex shared object issues for equivalent FST library functions.

#include <istream>
#include <fstream>

#include <fst/script/compile.h>
#include <fst/script/equal.h>

DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_string(ifile, "", "input file name");
DEFINE_string(syms, "", "input file symbols for compiling");
DEFINE_string(cfile, "", "file to compare input file to");
DEFINE_string(ofile, "", "file for output");

int main(int argc, char **argv) {
  using fst::script::FstClass;
  string usage =
      "Compiles and tests equality of HistogramArc models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (FLAGS_ifile.empty() || (FLAGS_syms.empty() && FLAGS_cfile.empty())) {
    LOG(ERROR)
        << "The --ifile option and one of --syms and --cfile must be non-empty";
    return 1;
  }
  if (!FLAGS_syms.empty() && !FLAGS_cfile.empty()) {
    LOG(ERROR) << "Both --syms and --cfile cannot be provided.  Give --syms to "
                  "compile the --ifile; give --cfile to compare with --ifile.";
    return 1;
  }
  if (FLAGS_syms.empty()) {
    std::unique_ptr<FstClass> ifst1(FstClass::Read(FLAGS_ifile));
    if (!ifst1) return 1;

    std::unique_ptr<FstClass> ifst2(FstClass::Read(FLAGS_cfile));
    if (!ifst2) return 1;

    bool result = fst::script::Equal(*ifst1, *ifst2, FLAGS_delta);
    if (!result) VLOG(1) << "FSTs are not equal.";

    return result ? 0 : 2;
  } else {
    std::unique_ptr<const fst::SymbolTable> syms(
        fst::SymbolTable::ReadText(FLAGS_syms));
    if (!syms) return 1;
    std::unique_ptr<const fst::SymbolTable> ssyms;
    std::ifstream fstrm;
    fstrm.open(FLAGS_ifile);
    if (!fstrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << FLAGS_ifile;
      return 1;
    }
    std::istream &istrm = fstrm.is_open() ? fstrm : std::cin;
    fst::script::CompileFst(istrm, FLAGS_ifile, FLAGS_ofile, "vector",
                                "hist", syms.get(), syms.get(), ssyms.get(),
                                false, true, true, true, false);
  }
  return 0;
}
