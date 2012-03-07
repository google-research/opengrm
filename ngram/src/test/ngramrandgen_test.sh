#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.randgen*.far" 0 2 13 15

set -e

echo "a" >$tmpdata/earnest.randgen.far
echo "b" >$tmpdata/earnest.randgen2.far
$bin/ngramrandgen --max_sents=1000 --seed=12 $testdata/earnest.mod $tmpdata/earnest.randgen.far
$bin/ngramrandgen --max_sents=1000 --seed=12 $testdata/earnest.mod $tmpdata/earnest.randgen2.far

farequal $tmpdata/earnest.randgen.far $tmpdata/earnest.randgen2.far

