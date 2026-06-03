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

// Projects the FST onto the input or output dimension.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_PROJECT_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_PROJECT_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "openfst/lib/fst.h"
#include "openfst/lib/project.h"
#include "openfst/script/getters.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

namespace thrax {
namespace function {

template <typename Arc>
class Project : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  Project() = default;
  ~Project() final = default;

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 2) {
      std::cout << "Project: Expected 2 arguments but received " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[1]->is<std::string>()) {
      std::cout << "Project: Expected string for argument 2" << std::endl;
      return nullptr;
    }
    const auto& project_str = *args[1]->get<std::string>();
    ::fst::ProjectType project_type;
    if (!::fst::script::GetProjectType(project_str, &project_type)) {
      std::cout << "Project: Invalid projection parameter: " << project_str
                << " (should be 'input' or 'output')" << std::endl;
      return nullptr;
    }
    return std::make_unique<::fst::ProjectFst<Arc>>(fst, project_type);
  }

 private:
  Project(const Project<Arc>&) = delete;
  Project<Arc>& operator=(const Project<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_PROJECT_H_
