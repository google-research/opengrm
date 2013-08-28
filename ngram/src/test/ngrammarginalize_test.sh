#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest-katz*" 0 2 13 15

set -e

function compile_test_fst {
  if [ ! -e $tmpdata/$1.ref ]
  then
    fstcompile -isymbols=$testdata/$1.sym -osymbols=$testdata/$1.sym \
      -keep_isymbols -keep_osymbols \
      -keep_state_numbering $testdata/$1.txt >$tmpdata/$1.ref
  fi
}

compile_test_fst earnest-katz.mod
compile_test_fst earnest-katz.marg.mod
compile_test_fst earnest-katz.marg.iter2.mod
$bin/ngrammarginalize $tmpdata/earnest-katz.mod.ref \
  $tmpdata/earnest-katz.marg.mod

fstequal $tmpdata/earnest-katz.marg.mod.ref $tmpdata/earnest-katz.marg.mod

$bin/ngrammarginalize --steady_state_file=$tmpdata/earnest-katz.marg.mod.ref \
  $tmpdata/earnest-katz.mod.ref $tmpdata/earnest-katz.marg.iter2.mod

fstequal $tmpdata/earnest-katz.marg.iter2.mod.ref \
  $tmpdata/earnest-katz.marg.iter2.mod

$bin/ngrammarginalize --iterations=2 \
  $tmpdata/earnest-katz.mod.ref $tmpdata/earnest-katz.marg.iter2I.mod

fstequal $tmpdata/earnest-katz.marg.iter2.mod.ref \
  $tmpdata/earnest-katz.marg.iter2I.mod
