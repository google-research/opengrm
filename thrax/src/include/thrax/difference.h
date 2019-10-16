// Takes the difference of two FSTs. This function may expand the second of the
// FSTs so that it can be optimized (determinized and epsilon-removed) if
// necessary.

#ifndef THRAX_DIFFERENCE_H_
#define THRAX_DIFFERENCE_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/difference.h>
#include <fst/fst.h>
#include <fst/properties.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/optimize.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class Difference : public BinaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  Difference() {}
  ~Difference() final {}

 protected:
  Transducer* BinaryFstExecute(const Transducer& left, const Transducer& right,
                               const std::vector<DataType*>& args) final {
    if (args.size() != 2) {
      std::cout << "Difference: Expected 2 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (FLAGS_save_symbols) {
      if (!::fst::CompatSymbols(left.InputSymbols(),
                                    right.InputSymbols())) {
        std::cout << "Difference: input symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
      if (!::fst::CompatSymbols(left.OutputSymbols(),
                                    right.OutputSymbols())) {
        std::cout << "Difference: output symbol table of 1st argument "
                  << "does not match output symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
    }
    // The function may only be called with an unweighted acceptor RHS.
    constexpr auto kRightInputProps =
        ::fst::kAcceptor | ::fst::kUnweighted;
    if (right.Properties(kRightInputProps, true) != kRightInputProps) {
      std::cout << "Difference: 2nd argument must be an unweighted acceptor"
                << std::endl;
      return nullptr;
    }
    // The underlying difference operation requires the RHS to have these
    // properties as well, but they may be satisfied by optimization.
    constexpr auto kRightOptimizationProps =
        ::fst::kNoEpsilons | ::fst::kIDeterministic;
    if (right.Properties(kRightOptimizationProps, false) ==
                         kRightOptimizationProps) {
      return new ::fst::DifferenceFst<Arc>(left, right);
    } else {
      std::unique_ptr<Transducer> optimized_right(
          Optimize<Arc>::ActuallyOptimizeDifferenceRhs(right, true));
      return new ::fst::DifferenceFst<Arc>(left, *optimized_right);
    }
  }

 private:
  Difference<Arc>(const Difference<Arc>&) = delete;
  Difference<Arc>& operator=(const Difference<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_DIFFERENCE_H_
