#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest*.pru" 0 2 13 15

set -e

# Count prune
$bin/ngramshrink --method=count_prune --check_consistency \
                 --count_pattern="3+:2" $testdata/earnest.mod \
                 >$tmpdata/earnest-count_prune.pru

fstequal $testdata/earnest-count_prune.pru \
         $tmpdata/earnest-count_prune.pru

# Relative entropy
$bin/ngramshrink --method=relative_entropy --check_consistency \
                 --theta=.00015 $testdata/earnest.mod \
                 >$tmpdata/earnest-relative_entropy.pru

fstequal $testdata/earnest-relative_entropy.pru \
         $tmpdata/earnest-relative_entropy.pru

# Seymore
$bin/ngramshrink --method=seymore --check_consistency \
                 --theta=4 $testdata/earnest.mod \
                 >$tmpdata/earnest-seymore.pru

fstequal $testdata/earnest-seymore.pru \
         $tmpdata/earnest-seymore.pru
