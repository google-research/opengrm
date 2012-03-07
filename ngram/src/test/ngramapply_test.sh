#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.randgen.apply.far" 0 2 13 15

set -e

$bin/ngramapply $testdata/earnest.mod $testdata/earnest.randgen.far $tmpdata/earnest.randgen.apply.far

farequal $testdata/earnest.randgen.apply.far $tmpdata/earnest.randgen.apply.far 

