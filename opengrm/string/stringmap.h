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

// This file contains functions for compiling FSTs from pairs of strings
// using a prefix tree.

#ifndef OPENGRM_STRING_STRINGMAP_H_
#define OPENGRM_STRING_STRINGMAP_H_

#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/log.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/push.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/string/prefix_tree.h"
#include "opengrm/string/stringcompile.h"
#include "opengrm/string/stringfile.h"

namespace fst {
namespace internal {

// Helper class for constructing string maps.
template <class Arc, class PTree>
class StringMapCompiler {
 public:
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  explicit StringMapCompiler(TokenType input_token_type = TokenType::BYTE,
                             TokenType output_token_type = TokenType::BYTE,
                             const SymbolTable* input_symbols = nullptr,
                             const SymbolTable* output_symbols = nullptr)
      : input_token_type_(input_token_type),
        output_token_type_(output_token_type),
        input_symbols_(input_symbols),
        output_symbols_(output_symbols) {}

  // One-string version.
  bool Add(absl::string_view iostring) {
    return Add(iostring, iostring, Weight::One());
  }

  // Two-string version.
  bool Add(absl::string_view istring, absl::string_view ostring,
           Weight weight = Weight::One()) {
    std::vector<Label> ilabels;
    if (!StringToLabels(istring, &ilabels, input_token_type_, input_symbols_))
      return false;
    std::vector<Label> olabels;
    if (!StringToLabels(ostring, &olabels, output_token_type_, output_symbols_))
      return false;
    ptree_.Add(ilabels, olabels, std::move(weight));
    return true;
  }

  // Three-string version, which also requires us to parse the weight.
  bool Add(absl::string_view istring, absl::string_view ostring,
           absl::string_view wstring) {
    std::istringstream strm{std::string(wstring)};
    Weight weight;
    strm >> weight;
    if (!strm) {
      LOG(ERROR) << "StringMapCompiler::Add: Bad weight: " << wstring;
      return false;
    }
    return Add(istring, ostring, std::move(weight));
  }

  void Compile(MutableFst<Arc>* fst) const { ptree_.ToFst(fst); }

 private:
  const TokenType input_token_type_;
  const TokenType output_token_type_;
  const SymbolTable* input_symbols_;
  const SymbolTable* output_symbols_;
  PTree ptree_;
};

// StringType could reasonably be `std::string` or `absl::string_view`.
template <class StringType>
bool StringMapLineIsAcceptor(absl::Span<const StringType> line) {
  switch (line.size()) {
    case 1:
      return true;
    case 2:
    case 3: {
      return line[0] == line[1];
    }
    default:
      return false;
  }
}

template <class StringType>
bool StringMapLineIsAcceptor(const std::vector<StringType>& line) {
  return StringMapLineIsAcceptor(absl::Span<const StringType>(line));
}

template <class Weight>
bool StringMapLineIsAcceptor(
    const std::tuple<std::string, std::string, Weight>& line) {
  return std::get<0>(line) == std::get<1>(line);
}

inline bool StringMapSameTokenTypeKernel(TokenType input_token_type,
                                         TokenType output_token_type,
                                         const SymbolTable* input_symbols,
                                         const SymbolTable* output_symbols) {
  if (input_token_type != output_token_type) return false;
  switch (input_token_type) {
    case TokenType::BYTE:
    case TokenType::UTF8: {
      return true;
    }
    case TokenType::SYMBOL: {
      // The pointers should either both be nullptr or both be non-nullptr.
      if ((!input_symbols != !output_symbols)) return false;
      return CompatSymbols(input_symbols, output_symbols);
    }
  }
  return false;  // Unreachable.
}

inline bool StringMapCheckRepresentableAsAcceptor(
    internal::ColumnStringFile* csf, TokenType input_token_type,
    TokenType output_token_type, const SymbolTable* input_symbols,
    const SymbolTable* output_symbols) {
  if (!StringMapSameTokenTypeKernel(input_token_type, output_token_type,
                                    input_symbols, output_symbols)) {
    return false;
  }
  for (; !csf->Done(); csf->Next()) {
    const auto& line = csf->Row();
    if (!StringMapLineIsAcceptor(line)) return false;
  }
  return true;
}

// `LineType` is (currently) any type with a `StringMapLineIsAcceptor` overload,
// which currently includes `std::vector<std::string>`,
// `std::vector<absl::string_view>`, and `std::tuple<std::string, std::string,
// Weight>` (given some `Weight` type). The functions below accept an
// absl::Span of these types.
template <class LineType>
bool StringMapCheckRepresentableAsAcceptor(absl::Span<const LineType> lines,
                                           TokenType input_token_type,
                                           TokenType output_token_type,
                                           const SymbolTable* input_symbols,
                                           const SymbolTable* output_symbols) {
  if (!StringMapSameTokenTypeKernel(input_token_type, output_token_type,
                                    input_symbols, output_symbols)) {
    return false;
  }
  for (const auto& line : lines) {
    if (!StringMapLineIsAcceptor(line)) return false;
  }
  return true;
}

template <class PTree, class Arc>
bool StringMapCompile(internal::ColumnStringFile* csf, MutableFst<Arc>* fst,
                      TokenType input_token_type, TokenType output_token_type,
                      const SymbolTable* input_symbols,
                      const SymbolTable* output_symbols) {
  internal::StringMapCompiler<Arc, PTree> compiler(
      input_token_type, output_token_type, input_symbols, output_symbols);
  for (csf->Reset(); !csf->Done(); csf->Next()) {
    const auto& line = csf->Row();
    const auto log_line_compilation_error = [&csf, &line]() {
      LOG(ERROR) << "StringFileCompile: Ill-formed line " << csf->LineNumber()
                 << " in file " << csf->Filename() << ": `"
                 << absl::StrJoin(line, "\t") << "`";
    };
    switch (line.size()) {
      case 1: {
        if (!compiler.Add(line[0])) {
          log_line_compilation_error();
          return false;
        }
        break;
      }
      case 2: {
        if (!compiler.Add(line[0], line[1])) {
          log_line_compilation_error();
          return false;
        }
        break;
      }
      case 3: {
        if (!compiler.Add(line[0], line[1], line[2])) {
          log_line_compilation_error();
          return false;
        }
        break;
      }
      default: {
        log_line_compilation_error();
        return false;
      }
    }
  }
  compiler.Compile(fst);
  return true;
}

template <class PTree, class Arc>
bool StringMapCompile(absl::Span<const std::vector<std::string>> lines,
                      MutableFst<Arc>* fst, TokenType input_token_type,
                      TokenType output_token_type,
                      const SymbolTable* input_symbols,
                      const SymbolTable* output_symbols) {
  internal::StringMapCompiler<Arc, PTree> compiler(
      input_token_type, output_token_type, input_symbols, output_symbols);
  for (const auto& line : lines) {
    const auto log_line_compilation_error = [&line]() {
      LOG(ERROR) << "StringMapCompile: Ill-formed line: `"
                 << absl::StrJoin(line, "\t") << "`";
    };
    switch (line.size()) {
      case 1: {
        if (!compiler.Add(line[0])) {
          log_line_compilation_error();
          return false;
        }
        break;
      }
      case 2: {
        if (!compiler.Add(line[0], line[1])) {
          log_line_compilation_error();
          return false;
        }
        break;
      }
      case 3: {
        if (!compiler.Add(line[0], line[1], line[2])) {
          log_line_compilation_error();
          return false;
        }
        break;
      }
      default: {
        log_line_compilation_error();
        return false;
      }
    }
  }
  compiler.Compile(fst);
  return true;
}

template <class PTree, class Arc>
bool StringMapCompile(
    absl::Span<const std::tuple<std::string, std::string, typename Arc::Weight>>
        lines,
    MutableFst<Arc>* fst, TokenType input_token_type = TokenType::BYTE,
    TokenType output_token_type = TokenType::BYTE,
    const SymbolTable* input_symbols = nullptr,
    const SymbolTable* output_symbols = nullptr) {
  internal::StringMapCompiler<Arc, PTree> compiler(
      input_token_type, output_token_type, input_symbols, output_symbols);
  for (const auto& [istring, ostring, weight] : lines) {
    if (!compiler.Add(istring, ostring, weight)) {
      LOG(ERROR) << "StringMapCompile: Ill-formed line: `(" << istring << ", "
                 << ostring << ", " << weight << ")`";
      return false;
    }
  }
  compiler.Compile(fst);
  return true;
}

template <class Arc, class Container>
bool StringMapCompileWithAcceptorCheck(
    Container container, MutableFst<Arc>* fst,
    TokenType input_token_type = TokenType::BYTE,
    TokenType output_token_type = TokenType::BYTE,
    const SymbolTable* input_symbols = nullptr,
    const SymbolTable* output_symbols = nullptr) {
  const bool representable_as_acceptor =
      internal::StringMapCheckRepresentableAsAcceptor(
          container, input_token_type, output_token_type, input_symbols,
          output_symbols);
  if (representable_as_acceptor) {
    return internal::StringMapCompile<AcceptorPrefixTree<Arc>>(
        container, fst, input_token_type, output_token_type, input_symbols,
        output_symbols);
  } else {
    if (!internal::StringMapCompile<TransducerPrefixTree<Arc>>(
            container, fst, input_token_type, output_token_type, input_symbols,
            output_symbols)) {
      return false;
    }
    // Applies optimizations specific to transducer string maps.
    Push<Arc, REWEIGHT_TO_INITIAL>(*fst, fst, kPushLabels);
    RmEpsilon(fst);
    static const ILabelCompare<Arc> icomp;
    ArcSort(fst, icomp);
    return true;
  }
}

}  // namespace internal

// Compiles deterministic FST representing the union of the cross-product of
// pairs of weighted string cross-products from a TSV file of string triples.
// It will be an acceptor if all lines represent the same istring and ostring
// and also the (token_type, symbols) is the same for input and output.
template <class Arc>
ABSL_MUST_USE_RESULT bool StringFileCompile(
    absl::string_view source, MutableFst<Arc>* fst,
    TokenType input_token_type = TokenType::BYTE,
    TokenType output_token_type = TokenType::BYTE,
    const SymbolTable* input_symbols = nullptr,
    const SymbolTable* output_symbols = nullptr) {
  internal::ColumnStringFile csf(source);
  if (csf.Error()) return false;  // File opening failed.
  return internal::StringMapCompileWithAcceptorCheck(
      &csf, fst, input_token_type, output_token_type, input_symbols,
      output_symbols);
}

// Compiles deterministic FST representing the union of the cross-product of
// pairs of weighted string cross-products from a vector of vector of strings.
// It will be an acceptor if all lines represent the same istring and ostring
// and also the (token_type, symbols) is the same for input and output.
template <class Arc>
ABSL_MUST_USE_RESULT bool StringMapCompile(
    absl::Span<const std::vector<std::string>> lines, MutableFst<Arc>* fst,
    TokenType input_token_type = TokenType::BYTE,
    TokenType output_token_type = TokenType::BYTE,
    const SymbolTable* input_symbols = nullptr,
    const SymbolTable* output_symbols = nullptr) {
  return internal::StringMapCompileWithAcceptorCheck(
      lines, fst, input_token_type, output_token_type, input_symbols,
      output_symbols);
}

// Compiles deterministic FST representing the union of the cross-product of
// pairs of weighted string cross-products from a vector of tuples of
// (istring, ostring, weight). It will be an acceptor if all lines represent the
// same istring and ostring and also the (token_type, symbols) is the same for
// input and output.
template <class Arc>
ABSL_MUST_USE_RESULT bool StringMapCompile(
    absl::Span<const std::tuple<std::string, std::string, typename Arc::Weight>>
        lines,
    MutableFst<Arc>* fst, TokenType input_token_type = TokenType::BYTE,
    TokenType output_token_type = TokenType::BYTE,
    const SymbolTable* input_symbols = nullptr,
    const SymbolTable* output_symbols = nullptr) {
  return internal::StringMapCompileWithAcceptorCheck(
      lines, fst, input_token_type, output_token_type, input_symbols,
      output_symbols);
}

}  // namespace fst

#endif  // OPENGRM_STRING_STRINGMAP_H_
