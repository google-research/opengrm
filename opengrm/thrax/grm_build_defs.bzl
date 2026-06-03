# Copyright 2026 The OpenGrm Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Build rules for compiling Thrax grammars into FST archves (FARs).

This file contains a BUILD rule that generates compiled grammar FSTs from text
grammar rule files.

Sample usage:

compile_grm(name = "test-grammar",
            src = "test-main.grm",
            # Optional arguments start here...
            deps = [
                ":helper-grm-1",
                ":helper-grm-2",
            ],
            args = "--flag_for_grammar_compiler " +
                   "--another_flag_for_compiler=foo",
)

"""

load("@com_google_openfst//openfst/extensions/far:build_defs.bzl", "convert_far_types")

def _rule_to_library(rule):
    """Returns a GeneralLibrary name for the loadable library (i.e. all
    dependencies and both the source and output files) given the original
    rule name.

    Args:
      rule: The original rule name in string form.
    """
    return rule + "_grm_lib"

def compile_grm(
        name,
        src = None,
        out = None,
        deps = None,
        args = None,
        data = None,
        fst_type = None,
        farconvert_args = None,
        **kwds):
    """Generates compiled FAR archived grammars from the src.

    Args:
      name: The BUILD rule name and the file prefix for the generated output.
      src: The main grammar file to be compiled.  If this is not present, then
           we'll search for <name>.grm as the source.
      out: Far file to be generated. If not present, then we'll use
           the source name replacing the .grm extension by .far
      deps: A list of other compile_grm rules that we'll need for the main
            grammar.
      args: An optional string of command line options to pass to the
            rewrite-grammar-compiler.
      data: An optional list of additional files used as resources for the grammar
            compilation.
      fst_type: An optional string specifying the type of the FSTs in the FAR
                  archive.
      farconvert_args: An optional string of command line options to pass to the
                       farconvert command when converting to fst_type.
      **kwds: Attributes common to all BUILD rules, e.g., testonly, visibility.
    """

    grammar_compiler = "//opengrm/thrax:compiler"

    src = src or name + ".grm"
    if not src.endswith(".grm"):
        fail("Rule %s's sources must end with .grm." % name)
    deps = deps or []
    out = out or src.replace(".grm", ".far")
    data = data or []

    # If fst_type is specified, then we need a temporary file for the compiler
    # output. The compiler outputs vector FSTs by default.
    convert_fst_type = fst_type and fst_type != "vector"
    if convert_fst_type:
        compiler_rule_name = name + "_compile_grm"
        far_extension = ".far"
        if out.endswith(far_extension):
            compiler_out = out[:-len(far_extension)] + ".tmp.far"
        else:
            compiler_out = out + ".tmp.far"
    else:
        compiler_rule_name = name
        compiler_out = out

    src_path = "$(location %s)" % src
    out_path = "$(location %s)" % compiler_out

    # Construct the compiler call.
    cmd = "$(location %s)" % grammar_compiler
    cmd += " --input_grammar=%s" % src_path
    cmd += " --output_far=%s" % out_path
    cmd += " --noprint_rules"

    cmd += " --indir=."
    cmd += " %s" % args

    # Following loop copies all the transitive dependencies.
    prep_cmd = ""
    for d in deps:
        prep_cmd += "\nfor f in $(locations " + d + "_lib); do"
        prep_cmd += "\n  if [[ \"$$f\" == *.far ]]; then"
        prep_cmd += "\n    rel_path=$${f#$(GENDIR)/}"
        prep_cmd += "\n    mkdir -p $$(dirname $$rel_path)"
        prep_cmd += "\n    cp -f $$f $$rel_path"
        prep_cmd += "\n  fi"
        prep_cmd += "\ndone\n"

    native.genrule(
        name = compiler_rule_name,
        srcs = [src] + deps + [d + "_lib" for d in deps] + data,
        outs = [compiler_out],
        cmd = prep_cmd + cmd,
        tools = [grammar_compiler],
        message = "Compiling Thrax grammar %s ==> %s" % (src, compiler_out),
        **kwds
    )

    if convert_fst_type:
        convert_far_types(
            name = name,
            far_binary_rule = "//opengrm/thrax:far",
            far_in = compiler_out,
            far_out = out,
            fst_type = fst_type,
            extra_args = farconvert_args,
            **kwds
        )

    # Helper target to collect all files transitively.
    native.filegroup(
        name = name + "_lib",
        srcs = [name, src] + [d + "_lib" for d in deps],
        **kwds,
    )
