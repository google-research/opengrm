// This function inverts an FST using a delayed FST.

#ifndef THRAX_INVERT_H_
#define THRAX_INVERT_H_

#include <iostream>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fst.h>
#include <fst/invert.h>
#include <fst/vector-fst.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class Invert : public UnaryFstFunction<Arc> {
 public:
  typedef fst::Fst<Arc> Transducer;

  Invert() {}
  ~Invert() final {}

 protected:
  Transducer* UnaryFstExecute(const Transducer& fst,
                              const std::vector<DataType*>& args) final {
    if (args.size() != 1) {
      std::cout << "Invert: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }

    return new fst::InvertFst<Arc>(fst);
  }

 private:
  Invert<Arc>(const Invert<Arc>&) = delete;
  Invert<Arc>& operator=(const Invert<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_INVERT_H_
