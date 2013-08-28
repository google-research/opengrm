#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.perp" 0 2 13 15

set -e

function compile_test_fst {
  if [ ! -e $tmpdata/$1.ref ]
  then
    fstcompile -isymbols=$testdata/$1.sym -osymbols=$testdata/$1.sym \
      -keep_isymbols -keep_osymbols \
      -keep_state_numbering $testdata/$1.txt >$tmpdata/$1.ref
  fi
}

farcompilestrings --symbols=$testdata/earnest.syms --keep_symbols=1 \
  $testdata/earnest.txt >$tmpdata/earnest.far

compile_test_fst earnest-witten_bell.mod
$bin/ngramperplexity --OOV_probability=0.01 \
  $tmpdata/earnest-witten_bell.mod.ref $tmpdata/earnest.far \
  $tmpdata/earnest.perp

cmp $testdata/earnest.perp $tmpdata/earnest.perp
