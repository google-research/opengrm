#!/bin/bash
# Description:
# Tests the command line binary ngramapply.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngramapp-earnest"
tmpprefix2="${tmpdata}/earnest.randgen"

trap "rm -f ${tmpprefix}* ${tmpprefix2}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpdata}/ngramapp-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpdata}/ngramapp-${1}.ref"
  fi
}

$srcdir/ngramcompile_randgen_far.sh earnest.randgen
compile_test_fst earnest-witten_bell.mod
"${bin}/ngramapply" "${tmpprefix}"-witten_bell.mod.ref \
  "${tmpprefix2}".far "${tmpprefix2}".apply.far

farequal \
  "${tmpprefix2}".apply.far.ref "${tmpprefix2}".apply.far

echo PASS
