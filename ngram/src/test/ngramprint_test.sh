#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest*" 0 2 13 15

set -e

function compile_test_fst {
  if [ ! -e $tmpdata/$1.ref ]
  then
    fstcompile -isymbols=$testdata/$1.sym -osymbols=$testdata/$1.sym \
      -keep_isymbols -keep_osymbols \
      -keep_state_numbering $testdata/$1.txt >$tmpdata/$1.ref
  fi
}

compile_test_fst earnest-witten_bell.mod
$bin/ngramprint --ARPA --check_consistency \
                $tmpdata/earnest-witten_bell.mod.ref >$tmpdata/earnest.arpa

cmp $testdata/earnest.arpa $tmpdata/earnest.arpa

$bin/ngramread --ARPA \
		$testdata/earnest.arpa >$tmpdata/earnest.arpa.mod

$bin/ngramprint --ARPA --check_consistency \
		$tmpdata/earnest.arpa.mod | \
		$bin/ngramread --ARPA >$tmpdata/earnest.arpa.mod2

fstequal $tmpdata/earnest.arpa.mod $tmpdata/earnest.arpa.mod2

compile_test_fst earnest.cnts
$bin/ngramprint --check_consistency \
                $tmpdata/earnest.cnts.ref >$tmpdata/earnest.cnt.print

cmp $testdata/earnest.cnt.print $tmpdata/earnest.cnt.print

$bin/ngramread -symbols=$testdata/earnest.syms \
		$testdata/earnest.cnt.print >$tmpdata/earnest.cnts

$bin/ngramprint --check_consistency $tmpdata/earnest.cnts | \
                $bin/ngramread -symbols=$testdata/earnest.syms \
		>$tmpdata/earnest.cnts2

fstequal $tmpdata/earnest.cnts $tmpdata/earnest.cnts2
