#!/bin/bash
# Description:
# Tests the command line binary ngrammerge.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngrammerge-earnest-$tmpsuffix-$RANDOM-$$"

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

compile_test_fst earnest-absolute.mod
compile_test_fst earnest-seymore.pru
compile_test_fst earnest.mrg
"${bin}/ngrammerge" --check_consistency --method=count_merge \
  "${tmpprefix}"-earnest-absolute.mod.ref \
  "${tmpprefix}"-earnest-seymore.pru.ref "${tmpprefix}".mrg

fstequal \
  "${tmpprefix}"-earnest.mrg.ref "${tmpprefix}".mrg

compile_test_fst earnest.mrg.norm
"${bin}/ngrammerge" --check_consistency --method=count_merge --normalize \
  "${tmpprefix}"-earnest-absolute.mod.ref \
  "${tmpprefix}"-earnest-seymore.pru.ref "${tmpprefix}".mrg.norm

fstequal \
  "${tmpprefix}"-earnest.mrg.norm.ref "${tmpprefix}".mrg.norm

compile_test_fst earnest.mrg.smooth
"${bin}/ngrammerge" --check_consistency --method=model_merge \
  "${tmpprefix}"-earnest-absolute.mod.ref \
  "${tmpprefix}"-earnest-seymore.pru.ref "${tmpprefix}".mrg.smooth

fstequal \
  "${tmpprefix}"-earnest.mrg.smooth.ref "${tmpprefix}".mrg.smooth

compile_test_fst earnest.mrg.smooth.norm
"${bin}/ngrammerge" --check_consistency --method=model_merge --normalize \
  "${tmpprefix}"-earnest-absolute.mod.ref \
  "${tmpprefix}"-earnest-seymore.pru.ref "${tmpprefix}".mrg.smooth.norm

fstequal \
  "${tmpprefix}"-earnest.mrg.smooth.norm.ref "${tmpprefix}".mrg.smooth.norm

echo PASS
