#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.mrg" 0 2 13 15

set -e

$bin/ngrammerge --check_consistency \
                $testdata/earnest-seymore.pru $testdata/earnest-absolute.mod \
                >$tmpdata/earnest.mrg

fstequal $testdata/earnest.mrg $tmpdata/earnest.mrg
