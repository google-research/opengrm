AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(srcdir)/../include $(ICU_CPPFLAGS)

lib_LTLIBRARIES = libthrax.la
libthrax_la_SOURCES = ast/collection-node.cc ast/grammar-node.cc \
                      ast/fst-node.cc ast/function-node.cc \
                      ast/identifier-node.cc ast/import-node.cc ast/node.cc \
                      ast/return-node.cc ast/rule-node.cc \
                      ast/statement-node.cc ast/string-node.cc flags/flags.cc \
                      main/grm-compiler.cc main/lexer.cc main/parser.yy \
                      main/compiler-stdarc.cc main/compiler-log.cc \
                      main/compiler-log64.cc util/getters.cc \
                      util/stringcompile.cc util/stringfile.cc \
                      util/stringutil.cc util/utils.cc \
                      walker/assert-equal.cc walker/assert-null.cc \
                      walker/assert-empty.cc walker/cdrewrite.cc \
                      walker/closure.cc walker/compose.cc walker/concat.cc \
                      walker/determinize.cc walker/difference.cc \
                      walker/evaluator.cc \
                      walker/expand.cc walker/features.cc \
                      walker/identifier-counter.cc walker/invert.cc \
                      walker/lenientlycompose.cc walker/loadfstfromfar.cc \
                      walker/loadfst.cc walker/minimize.cc \
                      walker/mpdtcompose.cc walker/namespace.cc \
                      walker/optimize.cc walker/paradigm.cc \
                      walker/pdtcompose.cc walker/printer.cc walker/project.cc \
                      walker/replace.cc walker/reverse.cc walker/rewrite.cc \
                      walker/rmepsilon.cc walker/rmweight.cc \
                      walker/stringfile.cc walker/stringfst.cc \
                      walker/symbols.cc walker/symboltable.cc walker/union.cc \
                      walker/walker.cc
libthrax_la_LDFLAGS = -version-info 138:0:0
