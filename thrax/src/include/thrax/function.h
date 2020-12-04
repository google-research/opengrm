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
// Basic function class that implements some operation in C++. Inheritors
// should perform some sort of manipulation based on the input arguments and
// then return a DataType. Implementors also need to free the provided argument
// vector (both the contained pointers and the vector pointer itself). Finally,
// they should also be templated on the Arc type and call
// REGISTER_GRM_FUNCTION(ClassName) for StdArc and LogArc support (usually in
// loader.cc).

#ifndef THRAX_FUNCTION_H_
#define THRAX_FUNCTION_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fst.h>
#include <thrax/datatype.h>
#include <thrax/compat/stlfunctions.h>
#include <thrax/compat/registry.h>

namespace thrax {
namespace function {

template <typename Arc>
class Function {
 public:
  Function() {}
  virtual ~Function() {}

  // Runs the desired function by wrapping Execute() and then freeing the
  // arguments.
  std::unique_ptr<DataType> Run(
      std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args) {
    auto return_value = Execute(*args);
    return return_value;
  }

 protected:
  // Actually performs the function's work, without deleting the provided
  // arguments.
  virtual std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) = 0;

 private:
  Function<Arc>(const Function<Arc>&) = delete;
  Function<Arc>& operator=(const Function<Arc>&) = delete;
};


extern Registry<Function<fst::StdArc>* > kStdArcRegistry;
extern Registry<Function<fst::LogArc>* > kLogArcRegistry;
extern Registry<Function<fst::Log64Arc>* > kLog64ArcRegistry;
extern void RegisterFunctions();

#define REGISTER_STDARC_FUNCTION(function) \
  kStdArcRegistry.Register(#function, new function)

#define REGISTER_LOGARC_FUNCTION(function) \
  kLogArcRegistry.Register(#function, new function)

#define REGISTER_LOG64ARC_FUNCTION(function) \
  kLogArcRegistry.Register(#function, new function)

#define REGISTER_GRM_FUNCTION(name) \
  typedef name<fst::StdArc> StdArc ## name; \
  REGISTER_STDARC_FUNCTION(StdArc ## name); \
  typedef name<fst::LogArc> LogArc ## name; \
  REGISTER_LOGARC_FUNCTION(LogArc ## name); \
  typedef name<fst::LogArc> Log64Arc ## name; \
  REGISTER_LOGARC_FUNCTION(Log64Arc ## name)

template <typename Arc>
class UnaryFstFunction : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  UnaryFstFunction() {}
  ~UnaryFstFunction() override {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.empty()) {
      std::cout << "UnaryFstFunction: Expected at least 1 argument"
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<Transducer*>()) {
      std::cout << "UnaryFstFunction: Expected FST for argument 1" << std::endl;
      return nullptr;
    }
    const auto* fst = *args[0]->get<Transducer*>();
    auto output = UnaryFstExecute(*fst, args);
    return output ? std::make_unique<DataType>(std::move(output)) : nullptr;
  }

  // This should actually perform the operation, using the FST argument
  // provided along with extra arguments still in the vector. The final result
  // should be allocated by this function. Ownership of provided arguments
  // remains with the caller. To signal an error, return nullptr.
  virtual std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) = 0;

 private:
  UnaryFstFunction(const UnaryFstFunction&) = delete;
  UnaryFstFunction& operator=(const UnaryFstFunction&) = delete;
};


template <typename Arc>
class BinaryFstFunction : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  BinaryFstFunction() {}
  ~BinaryFstFunction() override {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() < 2) {
      std::cout << "BinaryFstFunction: Expected at least 2 arguments"
                << std::endl;
      return nullptr;
    }
    for (int i = 0; i < 2; ++i) {
      if (!args[i]->is<Transducer*>()) {
        std::cout << "BinaryFstFunction: Expected FST for argument " << i + 1
                  << std::endl;
        return nullptr;
      }
    }
    const auto* left = *args[0]->get<Transducer*>();
    const auto* right = *args[1]->get<Transducer*>();
    auto output = BinaryFstExecute(*left, *right, args);
    return output ? std::make_unique<DataType>(std::move(output)) : nullptr;
  }

  // Same as above (with UnaryFstFunction), except now with two arguments.
  virtual std::unique_ptr<Transducer> BinaryFstExecute(
      const Transducer& left, const Transducer& right,
      const std::vector<std::unique_ptr<DataType>>& args) = 0;

 private:
  BinaryFstFunction(const BinaryFstFunction&) = delete;
  BinaryFstFunction& operator=(const BinaryFstFunction&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_FUNCTION_H_
