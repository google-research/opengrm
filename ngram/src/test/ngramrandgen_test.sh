#!/bin/bash
# Description:
# Tests the command line binary ngramrandgen.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngramrandgen-earnest"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpdata}/ngramrandgen-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpdata}/ngramrandgen-${1}.ref"
  fi
}

echo "a" >"${tmpprefix}".randgen.far
echo "b" >"${tmpprefix}".randgen2.far
compile_test_fst earnest.mod
"${bin}/ngramrandgen" --max_sents=1000 --seed=12 \
  "${tmpprefix}".mod.ref "${tmpprefix}".randgen.far
"${bin}/ngramrandgen" --max_sents=1000 --seed=12 \
  "${tmpprefix}".mod.ref "${tmpprefix}".randgen2.far

farequal \
  "${tmpprefix}".randgen.far "${tmpprefix}".randgen2.far

echo PASS
