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

compile_test_fst earnest.mod
$bin/ngraminfo $tmpdata/earnest.mod.ref $tmpdata/earnest.info

cmp $testdata/earnest.info $tmpdata/earnest.info

