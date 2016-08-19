#!/bin/bash
# Description:
# Tests the command line binary ngrammake.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngrammake-earnest-$tmpsuffix-$RANDOM-$$"

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

# Default method
compile_test_fst earnest.cnts
compile_test_fst earnest.mod
"${bin}/ngrammake" --check_consistency "${tmpprefix}"-earnest.cnts.ref \
  >"${tmpprefix}"-earnest.mod
fstequal \
  "${tmpprefix}"-earnest.mod.ref "${tmpprefix}"-earnest.mod

# Specified methods
for method in absolute katz witten_bell kneser_ney unsmoothed
do
  compile_test_fst earnest-$method.mod
  "${bin}/ngrammake" --method=$method --check_consistency \
                 "${tmpprefix}"-earnest.cnts.ref \
                 "${tmpprefix}"-earnest-$method.mod

  fstequal \
  "${tmpprefix}"-earnest-$method.mod.ref \
  "${tmpprefix}"-earnest-$method.mod
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
