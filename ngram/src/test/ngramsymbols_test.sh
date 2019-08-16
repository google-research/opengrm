#!/bin/bash
# Tests the command line binary ngramsymbols.

set -eou pipefail

readonly BIN="../bin"
readonly TESTDATA="${srcdir}/testdata"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

"${BIN}/ngramsymbols" "${TESTDATA}/earnest.txt" "${TEST_TMPDIR}/earnest.syms"

cmp "${TESTDATA}/earnest.syms" "${TEST_TMPDIR}/earnest.syms"
