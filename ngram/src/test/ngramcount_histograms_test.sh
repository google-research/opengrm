#!/bin/bash
# Description:
# Tests that ngramcount computes correct histogram counts.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/cnthist"

trap "rm -f ${tmpprefix}-*" 0 2 13 15

set -e

compile_test_fst() {
  if [ ! -e "${tmpdata}"/cnthist-"${1}".ref ]
  then
    fstcompile \
      -isymbols="${testdata}/${2}".syms -osymbols="${testdata}/${2}".syms \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}".txt >"${tmpdata}/cnthist-${1}".ref
  fi
}

compile_test_fst single_fst ab
farcreate \
  "${tmpprefix}"-single_fst.ref "${tmpprefix}"-single_fst.far
./ngramhisttest --ifile="${testdata}"/single_fst_ref.txt \
  --syms="${testdata}"/ab.syms --ofile="${tmpprefix}"-single_fst_ref.ref
"${bin}/ngramcount" --order=3 --method=histograms \
  "${tmpprefix}"-single_fst.far "${tmpprefix}"-single_fst.cnts
./ngramhisttest --ifile="${tmpprefix}"-single_fst.cnts \
  --cfile="${tmpprefix}"-single_fst_ref.ref

./ngramhisttest --ifile="${testdata}"/hist.ref.txt \
  --syms="${testdata}"/ab.syms --ofile="${tmpprefix}"-hist.ref.ref
compile_test_fst fst1.hist ab
compile_test_fst fst2.hist ab
farcreate \
  "${tmpprefix}"-fst1.hist.ref "${tmpprefix}"-fst2.hist.ref \
  "${tmpprefix}"-test.far
"${bin}/ngramcount" --order=2 --method=histograms \
  "${tmpprefix}"-test.far "${tmpprefix}"-test.cnts
./ngramhisttest --ifile="${tmpprefix}"-test.cnts \
  --cfile="${tmpprefix}"-hist.ref.ref

echo PASS
