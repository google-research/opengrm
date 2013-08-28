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

echo "a" >$tmpdata/earnest.randgen.far
echo "b" >$tmpdata/earnest.randgen2.far
compile_test_fst earnest.mod
$bin/ngramrandgen --max_sents=1000 --seed=12 $tmpdata/earnest.mod.ref \
  $tmpdata/earnest.randgen.far
$bin/ngramrandgen --max_sents=1000 --seed=12 $tmpdata/earnest.mod.ref \
  $tmpdata/earnest.randgen2.far

farequal $tmpdata/earnest.randgen.far $tmpdata/earnest.randgen2.far
