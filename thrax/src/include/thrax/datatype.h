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
#include <type_traits>
#include <utility>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/symbol-table.h>
#include <fst/vector-fst.h>
#include <variant>

namespace thrax {

template <typename T>
struct is_fst_raw_ptr : std::false_type {};
template <typename Arc>
struct is_fst_raw_ptr<::fst::Fst<Arc> *> : std::true_type {};
template <typename T>
inline constexpr bool is_fst_raw_ptr_v = is_fst_raw_ptr<T>::value;

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
    // NB: We can't directly create a private constructor from `ThingType` for
    // this purpose since `std::variant` makes its constituent types implicitly
    // convertible to it, resulting in ambiguous constructors.
    auto out = fst::WrapUnique(new DataType());
    out->thing_ = std::visit(
        [](auto &&arg) {
          using T = std::remove_cv_t<std::remove_reference_t<decltype(arg)>>;
          if constexpr (is_fst_raw_ptr_v<T>)
            return ThingType{arg->Copy()};
          else
            return ThingType{arg};
        },
        thing_);
    return out;
  }

  ~DataType() {
    std::visit(
        [](auto &&arg) {
          using T = std::remove_cv_t<std::remove_reference_t<decltype(arg)>>;
          if constexpr (is_fst_raw_ptr_v<T>) delete arg;
        },
        thing_);
  }

  template <typename T>
  bool is() const {
    return std::holds_alternative<T>(thing_);
  }

  template <typename T>
  const T* get() const {
    return std::get_if<T>(&thing_);
  }

  template <typename T>
  T* get_mutable() {
    return std::get_if<T>(&thing_);
  }

 private:
  using ThingType = std::variant<::fst::Fst<::fst::StdArc> *,
                                  ::fst::Fst<::fst::LogArc> *,
                                  ::fst::Fst<::fst::Log64Arc> *,
                                  ::fst::SymbolTable, std::string, int>;

  ThingType thing_;
  DataType() : thing_() {}
  DataType(const DataType &) = delete;
  DataType &operator=(const DataType &) = delete;
  DataType(DataType &&) = delete;
  DataType &operator=(DataType &&) = delete;
};

}  // namespace thrax

#endif  // THRAX_DATATYPE_H_
