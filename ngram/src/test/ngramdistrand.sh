#!/bin/bash
# Description:
# Random test of distributed training functions.

bin=../bin
tmpdata=${TMPDIR:-/tmp}

PATH="$bin":"$PATH"
export PATH

trap "rm -f "$tmpdata"/earnest*" 0 2 13 15

set -e

RANDF="$1"  # input file prefix
ORDER="$2"  # order of ngrams
VERBOSE="$3"

# Tests FST equality after assuring same ordering
ngramequal() {
  "$bin"/ngramsort "$1" >"$tmpdata"/"$RANDF".eq1
  "$bin"/ngramsort "$2" >"$tmpdata"/"$RANDF".eq2
  fstequal -v=1 --delta=0.01 \
    "$tmpdata"/"$RANDF".eq1 "$tmpdata"/"$RANDF".eq2
}

distributed_test() {
  # Non-distributed version
  "$srcdir/../bin/ngram.sh" \
    --order="$ORDER" $VERBOSE \
    --round_to_int \
    --itype=fst_sents \
    --ifile="$tmpdata/$RANDF.tocount.far" \
    --ofile="$tmpdata"/"$RANDF".nodist  \
    --symbols="$tmpdata"/"$RANDF".syms "$@"

  # Distributed version
  "$srcdir/../bin/ngram.sh" \
    --contexts="$tmpdata"/"$RANDF".cntxs --merge_contexts \
    --order="$ORDER" $VERBOSE \
    --round_to_int \
    --itype=fst_sents \
    --ifile="$tmpdata/$RANDF.tocount.far.*" \
    --ofile="$tmpdata"/"$RANDF".dist \
    --symbols="$tmpdata"/"$RANDF".syms "$@"

  # Verifies non-distributed and distributed versions give the same result
  ngramequal "$tmpdata"/"$RANDF".nodist "$tmpdata"/"$RANDF".dist
}

# checks distributed counting
distributed_test --otype=counts

# checks distributed estimation
distributed_test --otype=lm --smooth_method=katz
distributed_test --otype=lm --smooth_method=absolute
distributed_test --otype=lm --smooth_method=witten_bell

# checks distributed pruning
distributed_test --otype=pruned_lm --smooth_method=katz \
  --shrink_method=relative_entropy --theta=.00015
distributed_test --otype=pruned_lm --smooth_method=katz \
  --shrink_method=seymore --theta=4
distributed_test --otype=pruned_lm --smooth_method=witten_bell \
  --shrink_method=relative_entropy --theta=.00015
