// This function removes epsilon arcs from an FST. The argument FST is be
// expanded so that epsilon arcs can be removed in place.

#ifndef THRAX_RMEPSILON_H_
#define THRAX_RMEPSILON_H_

#include <iostream>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fstlib.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class RmEpsilon : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  RmEpsilon() {}
  ~RmEpsilon() final {}

 protected:
  Transducer* UnaryFstExecute(const Transducer& fst,
                              const std::vector<DataType*>& args) final {
    if (args.size() != 1) {
      std::cout << "RmEpsilon: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    auto* output = new MutableTransducer(fst);
    ::fst::RmEpsilon(output);
    return output;
  }

 private:
  RmEpsilon<Arc>(const RmEpsilon<Arc>&) = delete;
  RmEpsilon<Arc>& operator=(const RmEpsilon<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_RMEPSILON_H_
