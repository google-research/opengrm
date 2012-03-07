#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.info" 0 2 13 15

set -e

$bin/ngraminfo $testdata/earnest.mod $tmpdata/earnest.info

cmp $testdata/earnest.info $tmpdata/earnest.info

