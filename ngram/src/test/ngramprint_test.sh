#!/bin/bash
# Description:
# Tests the command line binary ngramprint, along with ngramread.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngramprint-earnest-$tmpsuffix-$RANDOM-$$"

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

compile_test_fst earnest-witten_bell.mod
"${bin}/ngramprint" --ARPA --check_consistency \
  "${tmpprefix}"-earnest-witten_bell.mod.ref "${tmpprefix}".arpa

cmp "${testdata}"/earnest.arpa "${tmpprefix}".arpa

"${bin}/ngramread" --ARPA "${testdata}"/earnest.arpa "${tmpprefix}".arpa.mod

"${bin}/ngramprint" --ARPA --check_consistency \
  "${tmpprefix}".arpa.mod | "${bin}/ngramread" --ARPA - "${tmpprefix}".arpa.mod2

fstequal \
  "${tmpprefix}".arpa.mod "${tmpprefix}".arpa.mod2

compile_test_fst earnest.cnts
"${bin}/ngramprint" --check_consistency \
  "${tmpprefix}"-earnest.cnts.ref "${tmpprefix}".cnt.print

cmp "${testdata}"/earnest.cnt.print "${tmpprefix}".cnt.print

"${bin}/ngramread" -symbols="${testdata}"/earnest.syms \
  "${testdata}"/earnest.cnt.print "${tmpprefix}".cnts

"${bin}/ngramprint" --check_consistency "${tmpprefix}".cnts | \
  "${bin}/ngramread" -symbols="${testdata}"/earnest.syms - "${tmpprefix}".cnts2

fstequal \
  "${tmpprefix}".cnts "${tmpprefix}".cnts2

echo PASS
