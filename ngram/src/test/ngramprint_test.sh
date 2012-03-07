#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.arpa*" 0 2 13 15

set -e

$bin/ngramprint --ARPA --check_consistency \
                $testdata/earnest.mod >$tmpdata/earnest.arpa

cmp $testdata/earnest.arpa $tmpdata/earnest.arpa

$bin/ngramread --ARPA \
		$testdata/earnest.arpa >$tmpdata/earnest.arpa.mod

$bin/ngramprint --ARPA --check_consistency \
		$tmpdata/earnest.arpa.mod | \
		$bin/ngramread --ARPA >$tmpdata/earnest.arpa.mod2

fstequal $tmpdata/earnest.arpa.mod $tmpdata/earnest.arpa.mod2

$bin/ngramprint --check_consistency \
                $testdata/earnest.cnts >$tmpdata/earnest.cnt.print

cmp $testdata/earnest.cnt.print $tmpdata/earnest.cnt.print

$bin/ngramread -symbols=$testdata/earnest.syms \
		$testdata/earnest.cnt.print >$tmpdata/earnest.cnts

$bin/ngramprint --check_consistency $tmpdata/earnest.cnts | \
                $bin/ngramread -symbols=$testdata/earnest.syms \
		>$tmpdata/earnest.cnts2

fstequal $tmpdata/earnest.cnts $tmpdata/earnest.cnts2

