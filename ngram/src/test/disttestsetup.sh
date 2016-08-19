#!/bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngramdist-$distname-$tmpsuffix-$RANDOM-$$"

trap "rm -f ${tmpprefix}*" 0 2 13 15

PATH="${bin}":"$PATH"
export PATH

if [ -z "${FRAC_DIST}" ]; then
  DIST_BIN="${srcdir}/../bin/ngram.sh"
  NODIST_BIN="${srcdir}/../bin/ngram.sh"
  NODIST_DISCOUNT_BINS="-1"
else
  # with fractional count dist, slightly different scripts and settings.
  DIST_BIN="${srcdir}/../bin/ngramfrac.sh"
  NODIST_BIN="${srcdir}/../bin/ngram.sh"
  NODIST_DISCOUNT_BINS="4"
fi

set -e

# sentence to split data on
SPLIT_DATA=850

# word ID to split context on
SPLIT_CNTX1=250
SPLIT_CNTX2=500
SPLIT_CNTX3=1000

# Sets up distributed data
awk "NR<$SPLIT_DATA" "${testdata}"/earnest.txt >"${tmpprefix}"-earnest.txt1
awk "NR>=$SPLIT_DATA" "${testdata}"/earnest.txt >"${tmpprefix}"-earnest.txt2

# Sets up distributed contexts
cat <<EOF >"${tmpprefix}"-earnest.cntxs
0 : $SPLIT_CNTX1
$SPLIT_CNTX1 : $SPLIT_CNTX2
$SPLIT_CNTX2 : $SPLIT_CNTX3
$SPLIT_CNTX3 : 2306
EOF

# Tests FST equality after assuring same ordering
ngramequal() {
  "${bin}"/ngramsort "$1" >"${tmpprefix}"-earnest.eq1
  "${bin}"/ngramsort "$2" >"${tmpprefix}"-earnest.eq2
  fstequal -v=1 \
    "${tmpprefix}"-earnest.eq1 "${tmpprefix}"-earnest.eq2
}

distributed_test() {
  # Non-distributed version
  "${NODIST_BIN}" --itype=text_sents --symbols="${testdata}"/earnest.syms "$@" \
    --bins="${NODIST_DISCOUNT_BINS}" --ifile="${testdata}/earnest.txt" \
    --ofile="${tmpprefix}"-earnest.nodist

  # Distributed version
  "${DIST_BIN}" --itype=text_sents --symbols="${testdata}"/earnest.syms "$@" \
    --contexts="${tmpprefix}"-earnest.cntxs --merge_contexts \
    --ifile="${tmpprefix}-earnest.txt[12]" --ofile="${tmpprefix}"-earnest.dist

  # Verifies non-distributed and distributed versions give the same result
  ngramequal "${tmpprefix}"-earnest.nodist "${tmpprefix}"-earnest.dist
}
