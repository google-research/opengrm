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

#include "opengrm/string/stringprintscript.h"

#include <string>

#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/script-impl.h"

namespace fst {
namespace script {

bool StringPrint(const FstClass& fst, std::string* str, TokenType token_type,
                 const SymbolTable* symbols) {
  StringPrintInnerArgs iargs(fst, str, token_type, symbols);
  StringPrintArgs args(iargs);
  Apply<Operation<StringPrintArgs>>("StringPrint", fst.ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(StringPrint, StringPrintArgs);

}  // namespace script
}  // namespace fst
