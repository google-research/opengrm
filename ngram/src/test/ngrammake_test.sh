#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest*.mod" 0 2 13 15

set -e

# Default method
$bin/ngrammake --check_consistency $testdata/earnest.cnts >$tmpdata/earnest.mod

fstequal $testdata/earnest.mod $tmpdata/earnest.mod

# Specified methods
for method in absolute katz kneser_ney witten_bell
do
  $bin/ngrammake --method=$method --check_consistency \
                 $testdata/earnest.cnts >$tmpdata/earnest-$method.mod

  fstequal $testdata/earnest-$method.mod $tmpdata/earnest-$method.mod
done
