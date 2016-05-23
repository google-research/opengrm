#!/bin/bash
# Description:
# Tests the command line binary ngrammake.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngrammake-earnest"

trap "rm -f ${tmpprefix}*" 0 2 13 15

set -e
compile_test_fst() {
  if [ ! -e "${tmpdata}/ngrammake-${1}.ref" ]
  then
    fstcompile \
      -isymbols="${testdata}/${1}.sym" -osymbols="${testdata}/${1}.sym" \
      -keep_isymbols -keep_osymbols -keep_state_numbering \
      "${testdata}/${1}.txt" "${tmpdata}/ngrammake-${1}.ref"
  fi
}

# Default method
compile_test_fst earnest.cnts
compile_test_fst earnest.mod
"${bin}/ngrammake" --check_consistency "${tmpprefix}".cnts.ref \
  >"${tmpprefix}".mod
fstequal \
  "${tmpprefix}".mod.ref "${tmpprefix}".mod

# Specified methods
for method in absolute katz witten_bell kneser_ney unsmoothed
do
  compile_test_fst earnest-$method.mod
  "${bin}/ngrammake" --method=$method --check_consistency \
                 "${tmpprefix}".cnts.ref \
                 "${tmpprefix}"-$method.mod

  fstequal \
  "${tmpprefix}"-$method.mod.ref \
  "${tmpprefix}"-$method.mod
done

# Fractional counting
farcompilestrings \
  --symbols="${testdata}"/earnest.syms --keep_symbols=1 \
  "${testdata}"/earnest.txt >"${tmpprefix}".far

"${bin}/ngramcount" --method=histograms "${tmpprefix}".far "${tmpprefix}".hsts

"${bin}/ngramcount" --method=counts "${tmpprefix}".far "${tmpprefix}".cnts

"${bin}/ngrammake" --method=katz_frac --check_consistency \
               "${tmpprefix}".hsts "${tmpprefix}"-katz_frac.mod

"${bin}/ngrammake" --method=katz --bins=4 --check_consistency \
               "${tmpprefix}".cnts "${tmpprefix}"-katz_frac.mod.ref

fstequal \
  "${tmpprefix}"-katz_frac.mod.ref "${tmpprefix}"-katz_frac.mod

echo PASS
