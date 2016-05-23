#!/bin/bash
# Description:
# Tests the command line binary ngraminfo.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngraminfo-earnest"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpdata}/ngraminfo-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpdata}/ngraminfo-${1}.ref"
  fi
}

compile_test_fst earnest.mod
"${bin}/ngraminfo" "${tmpprefix}".mod.ref "${tmpprefix}".info

cmp "${testdata}"/earnest.info "${tmpprefix}".info

echo PASS
