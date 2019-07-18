#ifndef NLP_GRM_LANGUAGE_UTIL_REWRITE_TESTER_UTILS_H_
#define NLP_GRM_LANGUAGE_UTIL_REWRITE_TESTER_UTILS_H_

#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <thrax/grm-manager.h>
#include <../bin/utildefs.h>

class RewriteTesterUtils {
  typedef fst::StringCompiler<fst::StdArc> Compiler;

 public:
  RewriteTesterUtils();

  ~RewriteTesterUtils();

  void Initialize();

  void Run();

  // Runs the input through the FSTs. Prepends "Output string:" to each line if
  // prepend_output is true
  const std::string ProcessInput(const std::string& input,
                                 bool prepend_output = true);

 private:
  // Reader for the input in interactive version.
  bool ReadInput(std::string* s);

  thrax::GrmManagerSpec<fst::StdArc> grm_;
  std::vector<std::string> rules_;
  Compiler* compiler_;
  fst::SymbolTable* byte_symtab_;
  fst::SymbolTable* utf8_symtab_;
  const fst::SymbolTable* generated_symtab_;
  fst::SymbolTable* input_symtab_;
  TokenType type_;
  fst::SymbolTable* output_symtab_;

  RewriteTesterUtils(const RewriteTesterUtils&) = delete;
  RewriteTesterUtils& operator=(const RewriteTesterUtils&) = delete;
};

#endif  // NLP_GRM_LANGUAGE_UTIL_REWRITE_TESTER_UTILS_H_
