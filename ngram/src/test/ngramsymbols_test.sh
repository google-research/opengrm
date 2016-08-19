#!/bin/bash
# Description:
# Tests the command line binary ngramsymbols.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngramsymbols-earnest-$tmpsuffix-$RANDOM-$$"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e

"${bin}/ngramsymbols" "${testdata}"/earnest.txt "${tmpprefix}".syms

cmp "${testdata}"/earnest.syms "${tmpprefix}".syms

echo PASS
