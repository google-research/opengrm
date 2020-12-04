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
// Projects the FST onto the input or output dimension.

#ifndef THRAX_PROJECT_H_
#define THRAX_PROJECT_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/project.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class Project : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  Project() {}
  ~Project() final {}

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 2) {
      std::cout << "Project: Expected 2 arguments but received " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[1]->is<std::string>()) {
      std::cout << "Project: Expected string for argument 2" << std::endl;
      return nullptr;
    }
    const auto& project = *args[1]->get<std::string>();
    if (project == "input") {
      return std::make_unique<::fst::ProjectFst<Arc>>(
          fst, ::fst::ProjectType::INPUT);
    } else if (project == "output") {
      return std::make_unique<::fst::ProjectFst<Arc>>(
          fst, ::fst::ProjectType::OUTPUT);
    } else {
      std::cout << "Project: Invalid projection parameter: " << project
                << " (should be 'input' or 'output')" << std::endl;
      return nullptr;
    }
  }

 private:
  Project<Arc>(const Project<Arc>&) = delete;
  Project<Arc>& operator=(const Project<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_PROJECT_H_
