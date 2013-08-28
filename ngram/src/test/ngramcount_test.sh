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

function compile_test_far {
  if [ ! -e $1.far ]
  then
    farcreate $tmpdata/$1.ref $tmpdata/$1.far
  fi
}

farcompilestrings --symbols=$testdata/earnest.syms --keep_symbols=1 \
	$testdata/earnest.txt >$tmpdata/earnest.far

compile_test_fst earnest.cnts
# Counting from set of string Fsts
$bin/ngramcount --order=5 $tmpdata/earnest.far >$tmpdata/earnest.cnts
fstequal $tmpdata/earnest.cnts.ref $tmpdata/earnest.cnts

compile_test_fst earnest.fst
compile_test_far earnest.fst
compile_test_fst earnest-fst.cnts
# Counting from an Fst representing a union of paths
$bin/ngramcount --order=5 $tmpdata/earnest.fst.far >$tmpdata/earnest.cnts
fstequal $tmpdata/earnest-fst.cnts.ref $tmpdata/earnest.cnts

compile_test_fst earnest.det
compile_test_far earnest.det
compile_test_fst earnest-det.cnts
# Counting from the deterministic "tree" Fst representing the corpus
$bin/ngramcount --order=5 $tmpdata/earnest.det.far >$tmpdata/earnest.cnts
fstequal $tmpdata/earnest-det.cnts.ref $tmpdata/earnest.cnts

compile_test_fst earnest.min
compile_test_far earnest.min
compile_test_fst earnest-min.cnts
# Counting from the minimal deterministic Fst representing the corpus
$bin/ngramcount --order=5 $tmpdata/earnest.min.far >$tmpdata/earnest.cnts
fstequal $tmpdata/earnest-min.cnts.ref $tmpdata/earnest.cnts

compile_test_fst earnest.cnt_of_cnts
# Counting from counts
$bin/ngramcount --method=count_of_counts \
  $tmpdata/earnest.cnts.ref >$tmpdata/earnest.cnt_of_cnts
fstequal $tmpdata/earnest.cnt_of_cnts.ref $tmpdata/earnest.cnt_of_cnts
