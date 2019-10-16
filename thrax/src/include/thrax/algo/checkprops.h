#ifndef FST_UTIL_OPERATORS_CHECKPROPS_H_
#define FST_UTIL_OPERATORS_CHECKPROPS_H_

#include <string>

#include <fst/fst.h>

namespace fst {
namespace internal {

// Checks that an FST is an unweighted acceptor.
template <class Arc>
bool CheckUnweightedAcceptor(const Fst<Arc> &fst, const std::string &op_name,
                             const std::string &fst_name) {
  static constexpr auto props = kAcceptor | kUnweighted;
  if (fst.Properties(props, true) != props) {
    LOG(ERROR) << op_name << ": " << fst_name
               << " must be a unweighted acceptor";
    return false;
  }
  return true;
}

}  // namespace internal
}  // namespace fst

#endif  // FST_UTIL_OPERATORS_CHECKPROPS_H_
