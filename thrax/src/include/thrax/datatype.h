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
#ifndef THRAX_DATATYPE_H_
#define THRAX_DATATYPE_H_

#include <memory>
#include <string>
#include <utility>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fst.h>
#include <thrax/compat/oneof.h>

namespace thrax {

class DataType {
 public:
  explicit DataType(std::unique_ptr<::fst::Fst<::fst::StdArc>> thing)
      : thing_(thing.release()) {}
  explicit DataType(std::unique_ptr<::fst::Fst<::fst::LogArc>> thing)
      : thing_(thing.release()) {}
  explicit DataType(std::unique_ptr<::fst::Fst<::fst::Log64Arc>> thing)
      : thing_(thing.release()) {}
  explicit DataType(const ::fst::SymbolTable &thing) : thing_(thing) {}
  explicit DataType(const std::string &thing) : thing_(thing) {}
  explicit DataType(int thing) : thing_(thing) {}

  std::unique_ptr<DataType> Copy() const {
    if (is<::fst::Fst<::fst::StdArc> *>()) {
      return std::make_unique<DataType>(fst::WrapUnique(
          (*get<::fst::Fst<::fst::StdArc> *>())->Copy()));
    } else if (is<::fst::Fst<::fst::LogArc> *>()) {
      return std::make_unique<DataType>(fst::WrapUnique(
          (*get<::fst::Fst<::fst::LogArc> *>())->Copy()));
    } else {
      // NB: We can't directly create a private constructor from `ThingType` for
      // this purpose since `std::variant` makes its constituent types
      // implicitly convertible to it, resulting in ambiguous constructors.
      auto out = fst::WrapUnique(new DataType());
      out->thing_ = thing_;
      return out;
    }
  }

  ~DataType() {
    if (is<::fst::Fst<::fst::StdArc> *>()) {
      delete *get<::fst::Fst<::fst::StdArc> *>();
    } else if (is<::fst::Fst<::fst::LogArc> *>()) {
      delete *get<::fst::Fst<::fst::LogArc> *>();
    }
  }

  template <typename T>
  bool is() const {
    return thing_.is<T>();
  }

  template <typename T>
  const T* get() const {
    return thing_.get<T>();
  }

  template <typename T>
  T* get_mutable() {
    return thing_.get_mutable<T>();
  }

 private:
  using ThingType = thrax::Oneof<::fst::Fst<::fst::StdArc> *,
                                  ::fst::Fst<::fst::LogArc> *,
                                  ::fst::Fst<::fst::Log64Arc> *,
                                  ::fst::SymbolTable, std::string, int>;

  ThingType thing_;
  // TODO(wolfsonkin): Default-initialize `thing_` once we drop `thrax::OneOf`.
  DataType() : thing_(-1) {}
  DataType(const DataType &) = delete;
  DataType &operator=(const DataType &) = delete;
};

}  // namespace thrax

#endif  // THRAX_DATATYPE_H_
