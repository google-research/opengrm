// This function does a cleaning up of an FST by determinizing and minimizing
// it. If it's a transducer, we encode the arcs beforehand.

#ifndef THRAX_OPTIMIZE_H_
#define THRAX_OPTIMIZE_H_

#include <iostream>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fstlib.h>
#include <thrax/algo/optimize.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class Optimize : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Optimize() {}
  ~Optimize() final {}

  // This function is public since other functions sometimes like to call on
  // this without having to go through the whole function registration
  // rigmarole.
  static Transducer* ActuallyOptimize(const Transducer& fst,
                                      bool compute_props = false) {
    auto* output = new MutableTransducer(fst);
    ::fst::Optimize(output, compute_props);
    return output;
  }

  static Transducer* ActuallyOptimizeDifferenceRhs(const Transducer& fst,
      bool compute_props = false) {
    auto* output = new MutableTransducer(fst);
    ::fst::OptimizeDifferenceRhs(output, compute_props);
    return output;
  }

 protected:
  Transducer* UnaryFstExecute(const Transducer& fst,
                              const std::vector<DataType*>& args) final {
    if (args.size() != 1) {
      std::cout << "Optimize: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }

    return ActuallyOptimize(fst);
  }

 private:
  Optimize<Arc>(const Optimize<Arc>&) = delete;
  Optimize<Arc>& operator=(const Optimize<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_OPTIMIZE_H_
