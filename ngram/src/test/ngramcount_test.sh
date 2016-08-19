#!/bin/bash
# Description:
# Tests the command line binary ngramcount.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngramcnt-earnest-$tmpsuffix-$RANDOM-$$"

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

compile_test_far() {
  if [ ! -e $1.far ]
  then
    compile_test_fst "${1}"
    farcreate \
      "${tmpprefix}-${1}.ref" "${tmpprefix}-${1}.far"
  fi
}

farcompilestrings \
  --symbols="${testdata}"/earnest.syms --keep_symbols=1 \
  "${testdata}"/earnest.txt >"${tmpprefix}".far

compile_test_fst earnest.cnts
# Counting from set of string Fsts
"${bin}/ngramcount" --order=5 "${tmpprefix}".far >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-earnest.cnts.ref "${tmpprefix}".cnts

compile_test_far earnest.fst
compile_test_fst earnest-fst.cnts
# Counting from an Fst representing a union of paths
"${bin}/ngramcount" --order=5 "${tmpprefix}"-earnest.fst.far \
  >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-earnest-fst.cnts.ref "${tmpprefix}".cnts

compile_test_far earnest.det
compile_test_fst earnest-det.cnts
# Counting from the deterministic "tree" Fst representing the corpus
"${bin}/ngramcount" --order=5 "${tmpprefix}"-earnest.det.far \
  >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-earnest-det.cnts.ref "${tmpprefix}".cnts

compile_test_far earnest.min
compile_test_fst earnest-min.cnts
# Counting from the minimal deterministic Fst representing the corpus
"${bin}/ngramcount" --order=5 "${tmpprefix}"-earnest.min.far \
  >"${tmpprefix}".cnts
fstequal \
  "${tmpprefix}"-earnest-min.cnts.ref "${tmpprefix}".cnts

compile_test_fst earnest.cnt_of_cnts
# Counting from counts
"${bin}/ngramcount" --method=count_of_counts \
  "${tmpprefix}"-earnest.cnts.ref >"${tmpprefix}".cnt_of_cnts
fstequal \
  "${tmpprefix}"-earnest.cnt_of_cnts.ref "${tmpprefix}".cnt_of_cnts

echo PASS
