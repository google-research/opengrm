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

# Default method
compile_test_fst earnest.cnts
compile_test_fst earnest.mod
$bin/ngrammake --check_consistency $tmpdata/earnest.cnts.ref \
  >$tmpdata/earnest.mod
fstequal $tmpdata/earnest.mod.ref $tmpdata/earnest.mod

# Specified methods
for method in absolute katz witten_bell kneser_ney unsmoothed
do
  compile_test_fst earnest-$method.mod
  $bin/ngrammake --method=$method --check_consistency \
                 $tmpdata/earnest.cnts.ref >$tmpdata/earnest-$method.mod

  fstequal $tmpdata/earnest-$method.mod.ref $tmpdata/earnest-$method.mod
done
