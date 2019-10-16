// Removes weights from arcs and final states (i.e., by setting them all to
// Weight::One()).

#ifndef THRAX_RMWEIGHT_H_
#define THRAX_RMWEIGHT_H_

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
class RmWeight : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  RmWeight() {}
  ~RmWeight() final {}

 protected:
  Transducer* UnaryFstExecute(const Transducer& fst,
                              const std::vector<DataType*>& args) final {
    if (args.size() != 1) {
      std::cout << "Invert: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    static const ::fst::RmWeightMapper<Arc> mapper;
    return new ::fst::ArcMapFst<Arc, Arc, ::fst::RmWeightMapper<Arc>>(
        fst, mapper);
  }

 private:
  RmWeight<Arc>(const RmWeight<Arc>&) = delete;
  RmWeight<Arc>& operator=(const RmWeight<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_RMWEIGHT_H_
