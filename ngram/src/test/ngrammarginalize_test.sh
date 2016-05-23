#!/bin/bash
# Description:
# Tests the command line binary ngrammarginalize.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngrammarg-earnest-katz"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpdata}/ngrammarg-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpdata}/ngrammarg-${1}.ref"
  fi
}

compile_test_fst earnest-katz.mod
compile_test_fst earnest-katz.marg.mod
compile_test_fst earnest-katz.marg.iter2.mod
"${bin}/ngrammarginalize" "${tmpprefix}".mod.ref "${tmpprefix}".marg.mod

fstequal \
  "${tmpprefix}".marg.mod.ref "${tmpprefix}".marg.mod

"${bin}/ngrammarginalize" --steady_state_file="${tmpprefix}".marg.mod.ref \
  "${tmpprefix}".mod.ref "${tmpprefix}".marg.iter2.mod

fstequal \
  "${tmpprefix}".marg.iter2.mod.ref "${tmpprefix}".marg.iter2.mod

"${bin}/ngrammarginalize" --iterations=2 \
  "${tmpprefix}".mod.ref "${tmpprefix}".marg.iter2I.mod

fstequal \
  "${tmpprefix}".marg.iter2.mod.ref "${tmpprefix}".marg.iter2I.mod

echo PASS
