#!/bin/bash
# Description:
# Tests the command line binary ngraminfo.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngraminfo-earnest-$tmpsuffix-$RANDOM-$$"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpprefix}-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpprefix}-${1}.ref"
  fi
}

compile_test_fst earnest.mod
"${bin}/ngraminfo" "${tmpprefix}"-earnest.mod.ref "${tmpprefix}".info

cmp "${testdata}"/earnest.info "${tmpprefix}".info

echo PASS
