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
