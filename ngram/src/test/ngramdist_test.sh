#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

PATH=$bin:$PATH
export PATH

trap "rm -f $tmpdata/earnest*" 0 2 13 15

set -e

# sentence to split data on
SPLIT_DATA=850

# word ID to split context on
SPLIT_CNTX1=250
SPLIT_CNTX2=500
SPLIT_CNTX3=1000

# Sets up distributed data
awk "NR<$SPLIT_DATA" $testdata/earnest.txt >$tmpdata/earnest.txt1
awk "NR>=$SPLIT_DATA" $testdata/earnest.txt >$tmpdata/earnest.txt2

# Sets up distributed contexts
cat <<EOF >$tmpdata/earnest.cntxs
0 : $SPLIT_CNTX1
$SPLIT_CNTX1 : $SPLIT_CNTX2
$SPLIT_CNTX2 : $SPLIT_CNTX3
$SPLIT_CNTX3: 2306
EOF

# Tests FST equality after assuring same ordering
function ngramequal  {
  $bin/ngramsort $1 >$tmpdata/earnest.eq1
  $bin/ngramsort $2 >$tmpdata/earnest.eq2
  fstequal -v=1 $tmpdata/earnest.eq1 $tmpdata/earnest.eq2
}

function distributed_test {
  # Non-distributed version
  $bin/ngram.sh \
    --itype=text_sents \
    --ifile="$testdata/earnest.txt" \
    --ofile=$tmpdata/earnest.nodist  \
    --symbols=$testdata/earnest.syms $*

  # Distributed version
  $bin/ngram.sh \
    --contexts=$tmpdata/earnest.cntxs --merge_contexts \
    --itype=text_sents \
    --ifile="$tmpdata/earnest.txt[12]" \
    --ofile=$tmpdata/earnest.dist \
    --symbols=$testdata/earnest.syms $*

  # Verifies non-distributed and distributed versions give the same result
  ngramequal $tmpdata/earnest.nodist $tmpdata/earnest.dist
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
