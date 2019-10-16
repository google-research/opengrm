#include <thrax/evaluator.h>

#include <string>

#include <fst/fst-decl.h>
#include <thrax/function.h>
#include <thrax/compat/utils.h>
#include <thrax/compat/registry.h>

DEFINE_bool(optimize_all_fsts, false,
            "If true, we'll run Optimize[] on all FSTs.");
DEFINE_bool(print_rules, true,
            "If true, we'll print out the rules as we evaluate them.");

namespace thrax {


Registry<function::Function<fst::StdArc>* > function::kStdArcRegistry;
Registry<function::Function<fst::LogArc>* > function::kLogArcRegistry;
Registry<function::Function<fst::Log64Arc>* > function::kLog64ArcRegistry;

template <>
function::Function<fst::StdArc>* GetFunction(const std::string& func_name) {
  std::string name = "StdArc" + func_name;
  return function::kStdArcRegistry.Get(name);
}

template <>
function::Function<fst::LogArc>* GetFunction(const std::string& func_name) {
  std::string name = "LogArc" + func_name;
  return function::kLogArcRegistry.Get(name);
}

template <>
function::Function<fst::Log64Arc>* GetFunction(const std::string& func_name) {
  std::string name = "Log64Arc" + func_name;
  return function::kLog64ArcRegistry.Get(name);
}

}  // namespace thrax
