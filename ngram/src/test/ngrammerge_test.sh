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

compile_test_fst earnest-absolute.mod
compile_test_fst earnest-seymore.pru
compile_test_fst earnest.mrg
$bin/ngrammerge --check_consistency \
                --method=count_merge \
                 $tmpdata/earnest-absolute.mod.ref \
                 $tmpdata/earnest-seymore.pru.ref >$tmpdata/earnest.mrg

fstequal $tmpdata/earnest.mrg.ref $tmpdata/earnest.mrg

compile_test_fst earnest.mrg.norm
$bin/ngrammerge --check_consistency \
                --method=count_merge \
                --normalize \
                $tmpdata/earnest-absolute.mod.ref \
                $tmpdata/earnest-seymore.pru.ref >$tmpdata/earnest.mrg.norm

fstequal $tmpdata/earnest.mrg.norm.ref $tmpdata/earnest.mrg.norm

compile_test_fst earnest.mrg.smooth
$bin/ngrammerge --check_consistency \
                --method=model_merge \
                $tmpdata/earnest-absolute.mod.ref \
                $tmpdata/earnest-seymore.pru.ref >$tmpdata/earnest.mrg.smooth

fstequal $tmpdata/earnest.mrg.smooth.ref $tmpdata/earnest.mrg.smooth

compile_test_fst earnest.mrg.smooth.norm
$bin/ngrammerge --check_consistency \
                --method=model_merge \
                --normalize \
                $tmpdata/earnest-absolute.mod.ref \
                $tmpdata/earnest-seymore.pru.ref \
                >$tmpdata/earnest.mrg.smooth.norm

fstequal $tmpdata/earnest.mrg.smooth.norm.ref $tmpdata/earnest.mrg.smooth.norm
