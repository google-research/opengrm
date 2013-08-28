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

./ngramcompile_randgen_far.sh
compile_test_fst earnest-witten_bell.mod
$bin/ngramapply $tmpdata/earnest-witten_bell.mod.ref \
  $tmpdata/earnest.randgen.far $tmpdata/earnest.randgen.apply.far

farequal $tmpdata/earnest.randgen.apply.far.ref \
         $tmpdata/earnest.randgen.apply.far 
