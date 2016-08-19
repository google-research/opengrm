#!/bin/bash
# Description:
# Tests the command line binary ngramsymbols.

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngramshrink-earnest-$tmpsuffix-$RANDOM-$$"

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
for method in count_prune relative_entropy seymore
do
  case "${method}" in
    count_prune) param="--count_pattern=3+:2" ;;
    relative_entropy) param="--theta=.00015" ;;
    seymore) param="--theta=4" ;;
  esac

  compile_test_fst "earnest-${method}.pru"
  "${bin}/ngramshrink" --method="${method}" --check_consistency "${param}" \
    "${tmpprefix}-earnest-witten_bell.mod.ref" "${tmpprefix}-${method}.pru"

  fstequal \
    "${tmpprefix}-earnest-${method}.pru.ref" "${tmpprefix}-${method}.pru"
done

for method in relative_entropy seymore
do
  case "${method}" in
    relative_entropy) target=5897 ;;
    seymore) target=5276 ;;
  esac

  "${bin}/ngramshrink" --method="${method}" --check_consistency \
    --target_number_of_ngrams="${target}" \
    "${tmpprefix}-earnest-witten_bell.mod.ref" \
    "${tmpprefix}-${method}.target.pru"

  fstequal \
    "${tmpprefix}-earnest-${method}.pru.ref" "${tmpprefix}-${method}.target.pru"
done

echo PASS
