#!/bin/bash
# Description:
# Tests the command line binary ngramcount.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngramcnt-earnest"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpdata}/ngramcnt-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpdata}/ngramcnt-${1}.ref"
  fi
}

compile_test_far() {
  if [ ! -e $1.far ]
  then
    farcreate \
      $tmpdata/$1.ref $tmpdata/$1.far
  fi
}

farcompilestrings \
  --symbols="${testdata}"/earnest.syms --keep_symbols=1 \
  "${testdata}"/earnest.txt >"${tmpprefix}".far

compile_test_fst earnest.cnts
# Counting from set of string Fsts
"${bin}/ngramcount" --order=5 "${tmpprefix}".far >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}".cnts.ref "${tmpprefix}".cnts

compile_test_fst earnest.fst
compile_test_far ngramcnt-earnest.fst
compile_test_fst earnest-fst.cnts
# Counting from an Fst representing a union of paths
"${bin}/ngramcount" --order=5 "${tmpprefix}".fst.far >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-fst.cnts.ref "${tmpprefix}".cnts

compile_test_fst earnest.det
compile_test_far ngramcnt-earnest.det
compile_test_fst earnest-det.cnts
# Counting from the deterministic "tree" Fst representing the corpus
"${bin}/ngramcount" --order=5 "${tmpprefix}".det.far >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-det.cnts.ref "${tmpprefix}".cnts

compile_test_fst earnest.min
compile_test_far ngramcnt-earnest.min
compile_test_fst earnest-min.cnts
# Counting from the minimal deterministic Fst representing the corpus
"${bin}/ngramcount" --order=5 "${tmpprefix}".min.far >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-min.cnts.ref "${tmpprefix}".cnts

compile_test_fst earnest.cnt_of_cnts
# Counting from counts
"${bin}/ngramcount" --method=count_of_counts \
  "${tmpprefix}".cnts.ref >"${tmpprefix}".cnt_of_cnts
fstequal \
  "${tmpprefix}".cnt_of_cnts.ref "${tmpprefix}".cnt_of_cnts

echo PASS
