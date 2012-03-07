#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.perp" 0 2 13 15

set -e

$bin/ngramperplexity --OOV_probability=0.01 $testdata/earnest.mod $testdata/earnest.far $tmpdata/earnest.perp

cmp $testdata/earnest.perp $tmpdata/earnest.perp

