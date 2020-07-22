// Projects the FST onto the input or output dimension.

#ifndef THRAX_PROJECT_H_
#define THRAX_PROJECT_H_

#include <iostream>
#include <string>
#include <vector>

#include <fst/fstlib.h>
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
  Transducer* UnaryFstExecute(const Transducer& fst,
                              const std::vector<DataType*>& args) final {
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
      return new ::fst::ProjectFst<Arc>(fst, ::fst::ProjectType::INPUT);
    } else if (project == "output") {
      return new ::fst::ProjectFst<Arc>(fst,
                                            ::fst::ProjectType::OUTPUT);
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
