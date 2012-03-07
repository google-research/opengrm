#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.syms" 0 2 13 15

set -e

$bin/ngramsymbols $testdata/earnest.txt $tmpdata/earnest.syms

cmp $testdata/earnest.syms $tmpdata/earnest.syms

