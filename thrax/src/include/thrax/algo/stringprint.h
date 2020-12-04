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
#ifndef FST_UTIL_STRING_STRINGPRINT_H_
#define FST_UTIL_STRING_STRINGPRINT_H_

#include <string>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fst-decl.h>
#include <fst/string.h>

namespace fst {

template <class Arc>
bool StringPrint(const Fst<Arc> &fst, std::string *str,
                 TokenType token_type = TokenType::BYTE,
                 const SymbolTable *symbols = nullptr) {
  const StringPrinter<Arc> printer(token_type, symbols);
  return printer(fst, str);
}

}  // namespace fst

#endif  // FST_UTIL_STRING_STRINGPRINT_H_
