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

// Classes for registering derived FUNCTION for generic reading.

#ifndef OPENGRM_THRAX_WALKER_UTIL_REGISTER_H_
#define OPENGRM_THRAX_WALKER_UTIL_REGISTER_H_

#include <string>

#include "absl/strings/string_view.h"
#include "openfst/lib/generic-register.h"
#include "openfst/lib/util.h"

namespace thrax {
namespace function {

template <class Arc>
class Function;

}  // namespace function

// This class represents a single entry in a FunctionRegister.
template <class Arc>
using FunctionRegisterEntry = const function::Function<Arc>*;

template <class FUNCTION>
const FUNCTION* GetStaticFunctionPointer() {
  static const auto* ptr = new FUNCTION();
  return ptr;
}

// This class maintains the correspondence between a string describing a
// FUNCTION type, and its reader and converter.
template <class Arc>
class FunctionRegister
    : public ::fst::GenericRegister<std::string, FunctionRegisterEntry<Arc>,
                                    FunctionRegister<Arc>> {
 protected:
  std::string ConvertKeyToSoFilename(absl::string_view key) const override {
    std::string legal_type(key);
    ::fst::ConvertToLegalCSymbol(&legal_type);
    legal_type.append("-function.so");
    return legal_type;
  }
};

// This class registers a Thrax FUNCTION type for generic invocation.
// The type must have a default constructor and a copy constructor from
// Fst<Arc>.
template <class Arc>
using FunctionRegisterer = ::fst::GenericRegisterer<FunctionRegister<Arc>>;

// Convenience macro to generate a static FunctionRegisterer instance.
// `FUNCTION` and `Arc` must be identifiers (that is, not a qualified type).
// Users SHOULD NOT register within the `thrax` namespace. To register a
// FUNCTION for StdArc, for example, use:
//
// namespace example {
// using fst::StdArc;
// REGISTER_FUNCTION(MyFunction, StdArc);
// }  // namespace example
#define REGISTER_FUNCTION(FUNCTION, Arc)                               \
  static thrax::FunctionRegisterer<Arc> FUNCTION##_##Arc##_registerer( \
      #FUNCTION, GetStaticFunctionPointer<FUNCTION<Arc>>())

}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_REGISTER_H_
