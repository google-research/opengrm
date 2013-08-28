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
for method in count_prune relative_entropy seymore 
do
  case $method in
    count_prune) param="--count_pattern=3+:2" ;;
    relative_entropy) param="--theta=.00015" ;;
    seymore) param="--theta=4" ;;
  esac     

  compile_test_fst earnest-$method.pru
  $bin/ngramshrink --method=$method --check_consistency \
                   $param $tmpdata/earnest-witten_bell.mod.ref \
                   >$tmpdata/earnest-$method.pru

  fstequal $tmpdata/earnest-$method.pru.ref \
           $tmpdata/earnest-$method.pru
done

