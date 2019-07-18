// Main compiler entry point; Compiles a GRM source file into an FST archive.

#include <thrax/compiler.h>
#include <thrax/compat/utils.h>
#include <thrax/grm-compiler.h>
#include <thrax/grm-manager.h>
#include <thrax/function.h>

DEFINE_string(input_grammar, "", "Path to the grammar file");
DEFINE_string(output_far, "", "Path for write the FST archive");
DEFINE_string(arc_type, "standard", "Arc type for compiled FSTs");
DEFINE_bool(emit_ast_only,
            false,
            "Parse the input, write its AST to stdout, and exit without "
            "writing an FST archive");

using thrax::CompileGrammar;
using thrax::GrmCompilerSpec;
using thrax::GrmManagerSpec;

int main(int argc, char **argv) {
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(argv[0], &argc, &argv, true);

  thrax::function::RegisterFunctions();
  if (FLAGS_arc_type == "standard") {
    if (CompileGrammar<fst::StdArc>(FLAGS_input_grammar, FLAGS_output_far,
                                        FLAGS_emit_ast_only,
                                        FLAGS_line_numbers_in_ast))
      return 0;
  } else if (FLAGS_arc_type == "log") {
    if (CompileGrammar<fst::LogArc>(FLAGS_input_grammar, FLAGS_output_far,
                                        FLAGS_emit_ast_only,
                                        FLAGS_line_numbers_in_ast))
      return 0;
  } else if (FLAGS_arc_type == "log64") {
    if (CompileGrammar<fst::Log64Arc>(FLAGS_input_grammar, FLAGS_output_far,
                                          FLAGS_emit_ast_only,
                                          FLAGS_line_numbers_in_ast))
      return 0;
  } else {
    LOG(FATAL) << "Unsupported arc type: " << FLAGS_arc_type;
  }
  return 1;
}
