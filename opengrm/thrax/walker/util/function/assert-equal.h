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

// Assert the equality of the first and second argument, issuing a warning if
// the two are not the same and returning the value of the first. More
// specifically, the first argument is assumed to be a transducer whose *output*
// value (unweighted shortest path of righthand projection) the user wishes to
// test. The second argument is assumed to be the intended value. Thus for
// example if I have a transducer "trans", I might test if applying "trans" to
// "foo" yields "bar" thus:
//
// equality = AssertEqual[foo @ trans, bar];
//
// This allows the user to check that the behavior is as intended without an
// external test file.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_ASSERT_EQUAL_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_ASSERT_EQUAL_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/intersect.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

ABSL_DECLARE_FLAG(bool, save_symbols);  // From util/flags.cc.

// TODO: Some day we should make this so that it doesn't return a value,
// but merely runs the assertion. That, however, would require changing the
// parser.

namespace thrax {
namespace function {

template <typename Arc>
class AssertEqual : public BinaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Weight = typename Arc::Weight;
  // Prune and ShortestPath require the path property.
  static_assert(::fst::IsPath<Weight>::value,
                "Weight must have path property.");

  AssertEqual() = default;
  ~AssertEqual() final = default;

 protected:
  std::unique_ptr<Transducer> BinaryFstExecute(
      const Transducer& left, const Transducer& right,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() < 2 || args.size() > 3) {
      std::cout << "AssertEqual: Expected 2 or 3 arguments but got "
                << args.size() << std::endl;
      return nullptr;
    }
    auto mode = ::fst::TokenType::BYTE;
    const ::fst::SymbolTable* symbols = nullptr;
    if (args.size() > 2) {
      if (args[2]->is<std::string>()) {
        if (*args[2]->get<std::string>() == "utf8") {
          mode = ::fst::TokenType::UTF8;
        } else {
          mode = ::fst::TokenType::BYTE;
        }
      } else if (args[2]->is<::fst::SymbolTable>()) {
        symbols = args[2]->get<::fst::SymbolTable>();
        mode = ::fst::TokenType::SYMBOL;
      } else {
        std::cout << "AssertEqual: Invalid parse mode or symbol table "
                  << "for symbols" << std::endl;
        return nullptr;
      }
    }
    if (absl::GetFlag(FLAGS_save_symbols)) {
      if (!::fst::CompatSymbols(left.InputSymbols(), right.InputSymbols())) {
        std::cout << "AssertEqual: input symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
      if (!::fst::CompatSymbols(left.OutputSymbols(), right.OutputSymbols())) {
        std::cout << "AssertEqual: output symbol table of 1st argument "
                  << "does not match output symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
    }
    auto mutable_left = std::make_unique<MutableTransducer>(left);
    ::fst::Project(mutable_left.get(), ::fst::ProjectType::OUTPUT);
    ::fst::RmEpsilon(mutable_left.get());
    MutableTransducer determinized_left;
    ::fst::Determinize(*mutable_left, &determinized_left);
    ::fst::Prune(determinized_left, mutable_left.get(), Arc::Weight::One());
    static const ::fst::RmWeightMapper<Arc> mapper;
    ::fst::ArcMap(mutable_left.get(), mapper);
    MutableTransducer mutable_right(right);
    ::fst::Project(&mutable_right, ::fst::ProjectType::OUTPUT);
    ::fst::RmEpsilon(&mutable_right);
    MutableTransducer determinized_right;
    ::fst::Determinize(mutable_right, &determinized_right);
    ::fst::Prune(determinized_right, &mutable_right, Arc::Weight::One());
    ::fst::ArcMap(&mutable_right, mapper);
    MutableTransducer intersection;
    ::fst::Intersect(*mutable_left, mutable_right, &intersection);
    // If both mutable_left and mutable_right have zero states, then they count
    // as equivalent. We only consider the intersection a failure if at least
    // one of them has some states.
    if ((mutable_left->NumStates() != 0 || mutable_right.NumStates() != 0) &&
        intersection.Start() == ::fst::kNoStateId) {
      // Prints strings for debug message.
      // TODO: This is still going to fail to produce useful output for
      // extended labels since those will have labels outside the byte or UTF-8
      // range.
      ::fst::RmEpsilon(mutable_left.get());
      ::fst::RmEpsilon(&mutable_right);
      std::string lstring;
      if (mutable_left->Start() == ::fst::kNoStateId) {
        lstring = "nullptr";
      } else {
        std::string content;
        AssertEqual<Arc>::CoerceToString(*mutable_left, &content, mode,
                                         symbols);
        lstring = absl::StrCat("\"", absl::Utf8SafeCEscape(content), "\"");
      }
      std::string rstring;
      if (mutable_right.Start() == ::fst::kNoStateId) {
        rstring = "nullptr";
      } else {
        std::string content;
        AssertEqual<Arc>::CoerceToString(mutable_right, &content, mode,
                                         symbols);
        rstring = absl::StrCat("\"", absl::Utf8SafeCEscape(content), "\"");
      }
      std::cout << "Arguments to AssertEqual are not equivalent:\n"
                << "  expect: " << rstring << "\n"
                << "     got: " << lstring << "\n"
                << std::endl;
      return nullptr;
    }
    return mutable_left;
  }

 private:
  // Coerces an FST to a string by calling ShortestPath, TopSort, and the string
  // printer. This is necessary so we have exactly one string to show in the
  // debug message.
  static void CoerceToString(const MutableTransducer& fst, std::string* str,
                             ::fst::TokenType token_type,
                             const ::fst::SymbolTable* symbols = nullptr) {
    const ::fst::StringPrinter<Arc> printer(token_type, symbols);
    if (fst.Properties(::fst::kString, true) == ::fst::kString) {
      printer(fst, str);
    } else {
      MutableTransducer string_fst(fst);
      ::fst::ShortestPath(fst, &string_fst);
      ::fst::TopSort(&string_fst);
      printer(string_fst, str);
    }
  }
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_ASSERT_EQUAL_H_
