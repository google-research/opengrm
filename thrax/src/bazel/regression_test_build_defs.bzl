# Copyright 2005-2020 Google LLC
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
#
# -*- mode: python; -*-

"""Grammar regression test BUILD rule.

This file contains a BUILD rule that generates regression tests for compiled
FSTs using regression_test.cc. It is a simple wrapper around a cc_test based on
regression_test-lib, to reduce error-prone boilerplate in BUILD files local to
the grammars being tested.

Example Usage:
grm_regression_test(
    name = "example_grammar_test",
    grammar = ":example_grammar",
    testdata = "testdata/example_grammar.tsv"
)
"""

# pylint: disable=undefined-variable
def _GetGrammarParams(basename, grammar, far_file_target):
    """Determine full grammar target/path."""

    if far_file_target != None:
        if far_file_target.startswith(":"):
            # Make a relative `far_file_target` absolute.
            far_file_target = "//" + native.package_name() + far_file_target
        elif not far_file_target.startswith("//"):
            fail("far_file_target must be either relative, absolute, or None.")
    else:
        if not grammar:
            grammar_target = ":" + basename + "_sttable"
        else:
            grammar_target = grammar

        if grammar_target.endswith("_sttable"):
            grammar_base = grammar_target[:-8]
        else:
            grammar_base = grammar_target

        if grammar_base.startswith("//" + native.package_name()):
            grammar_package_absolute, far_name_relative = grammar_base.split(":", 1)
            far_name_relative += ".far"
        elif grammar_base.startswith(":"):
            grammar_package_absolute = "//" + native.package_name()
            far_name_relative = grammar_base[1:] + ".far"
        else:
            fail("Unhandled grammar: %s" % grammar)
        far_file_target = grammar_package_absolute + ":" + far_name_relative
    far_file_path = far_file_target[2:].replace(":", "/")
    return far_file_path, far_file_target

# pylint: disable=undefined-variable
def _GetTestFileParams(test_file_ext, testdata_packaged, basename, test_file):
    """Determine full test file target & path."""
    if not test_file:
        if testdata_packaged:
            test_file_base = "testdata:" + basename + test_file_ext
            test_file_target = "//" + native.package_name() + "/" + test_file_base
        else:
            test_file_base = "testdata/" + basename + test_file_ext
            test_file_target = test_file_base
    else:
        test_file_base = test_file
        if testdata_packaged or ":" in test_file_base:
            test_file_target = "//" + native.package_name() + "/" + test_file
        else:
            test_file_target = test_file_base
    test_file_path = native.package_name() + "/" + test_file_base.replace(":", "/")
    return test_file_target, test_file_path

# pylint: disable=g-bad-name,g-doc-args,undefined-variable
def grm_regression_test(
        name,
        grammar = None,
        test_file = None,
        test_file_ext = ".tsv",
        testdata_packaged = False,
        delimiter = None,
        size = "small",
        covering_grammar = False,
        require_exactly_one_top_rewrite = True,
        test_file_target = None,
        test_file_path = None,
        far_file_target = None,
        token_type = "byte",
        extra_deps = [],
        **kwds):
    """Generates a regression test for the specified grammar.

    Args:
      name: The BUILD rule name, this should generally include a _test suffix.
      grammar: The rule defining the compiled grammar. If absent,
               <current package>:<name - '_test'>_sttable is used.
      test_file: A file containing test data in the format expected by
                 regression_test.cc. If absent, testdata/<name - '_test'>.tsv
                 is used.
      testdata_packaged: Whether the testdata files are in a package (i.e. a
                         directory containing a BUILD rule). This affects the
                         specification of the data target to the underlying rule.
                         If the test_file argument contains a colon, this is
                         inferred to be True.
      covering_grammar: If true, the grammar is a covering grammar which means
                        that we only require that the expected string be in the
                        output. If this is set, require_exactly_one_top_rewrite
                        is otiose.
      require_exactly_one_top_rewrite: If true, requires that the grammar outputs
                                       exactly one rewrite costing
                                       min(cost_of_all_possible_rewrites). For
                                       example, would fail if given <'foo', 1> and
                                       <'bar', 1>, since these have the same cost,
                                       but would not fail for <'foo', 1> and
                                       <'bar', 2>. Setting this to false is
                                       usually a bad idea, since this testing
                                       framework does not deal with n-best
                                       rewrites. Use a custom test in this case."
      test_file_target: Path to the target which contains testdata files.
                        This should be set together with test_file_path parameter.
      test_file_path: Path to the grammar testdata file.
                      This should be set together with test_file_target parameter.
      far_file_target: The target of the FAR file to test. If absent, <grammar>.far
                      is used.
      token_type: TokenType to use when parsing the text examples that are to
                be composed with rule FSTs in the FAR. One of 'byte' or 'utf8'.
      extra_deps: Extra dependences to link into the test, usually FST registrations.
      kwds: Attributes passed to the underlying cc_test rule.
    """
    if name.endswith("_test"):
        basename = name[:-5]
    else:
        basename = name
    far_file_path, far_file_target = _GetGrammarParams(basename, grammar, far_file_target)

    if ((test_file_target == None and test_file_path != None) or
        (test_file_target != None and test_file_path == None)):
        fail("Both test_file_target and test_file_path need " +
             "to be set or unset together.")

    if (test_file_target == None and test_file_path == None):
        test_file_target, test_file_path = _GetTestFileParams(
            test_file_ext,
            testdata_packaged,
            basename,
            test_file,
        )

    args = [
        "--far_file=" + far_file_path,
        "--test_file=" + test_file_path,
    ]
    if delimiter:
        args.append("--delimiter='" + delimiter + "'")

    if covering_grammar:
        args.append("--covering_grammar")
    else:
        args.append("--nocovering_grammar")

    if require_exactly_one_top_rewrite:
        args.append("--require_exactly_one_top_rewrite")
    else:
        args.append("--norequire_exactly_one_top_rewrite")

    if token_type != "byte" and token_type != "utf8":
        fail("token_type must be either \"byte\" or \"utf8\"")
    args.append("--token_type=" + token_type)

    native.cc_test(
        name = name,
        size = size,
        args = args,
        data = [
            # We want the test to trigger if the underlying grammar changes.
            far_file_target,
            test_file_target,
        ],
        deps = extra_deps + [
            "@org_opengrm_thrax//:regression_test-lib",
        ],
        **kwds
    )
