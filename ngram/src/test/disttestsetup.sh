#!/bin/sh

trap "rm -f ${tmpdata}/${distname}-earnest*" 0 2 13 15

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

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
awk "NR<$SPLIT_DATA" "${testdata}"/earnest.txt \
  >"${tmpdata}/${distname}"-earnest.txt1
awk "NR>=$SPLIT_DATA" "${testdata}"/earnest.txt \
  >"${tmpdata}/${distname}"-earnest.txt2

# Sets up distributed contexts
cat <<EOF >"${tmpdata}/${distname}"-earnest.cntxs
0 : $SPLIT_CNTX1
$SPLIT_CNTX1 : $SPLIT_CNTX2
$SPLIT_CNTX2 : $SPLIT_CNTX3
$SPLIT_CNTX3 : 2306
EOF

# Tests FST equality after assuring same ordering
ngramequal() {
  "${bin}"/ngramsort "$1" >"${tmpdata}/${distname}"-earnest.eq1
  "${bin}"/ngramsort "$2" >"${tmpdata}/${distname}"-earnest.eq2
  fstequal -v=1 \
    "${tmpdata}/${distname}"-earnest.eq1 "${tmpdata}/${distname}"-earnest.eq2
}

distributed_test() {
  # Non-distributed version
  "${NODIST_BIN}" --itype=text_sents --symbols="${testdata}"/earnest.syms "$@" \
    --bins="${NODIST_DISCOUNT_BINS}" \
    --ifile="${testdata}/earnest.txt" \
    --ofile="${tmpdata}/${distname}"-earnest.nodist

  # Distributed version
  "${DIST_BIN}" --itype=text_sents --symbols="${testdata}"/earnest.syms "$@" \
    --contexts="${tmpdata}/${distname}"-earnest.cntxs --merge_contexts \
    --ifile="${tmpdata}/${distname}-earnest.txt[12]" \
    --ofile="${tmpdata}/${distname}"-earnest.dist

  # Verifies non-distributed and distributed versions give the same result
  ngramequal "${tmpdata}/${distname}"-earnest.nodist \
              "${tmpdata}/${distname}"-earnest.dist
}
