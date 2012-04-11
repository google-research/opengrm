#! /bin/sh

bin=../bin
testdata=$2
tmpdata=$3

set -e
params=$1

# run random trial with specific parameters
. $params
cat $params
$bin/ngramrandgen --seed=$seed1 --max_sents=$sents1 \
  $testdata/init.randcorpus.$V.mod >$tmpdata/testrand.corp0.far
farprintstrings $tmpdata/testrand.corp0.far >$tmpdata/testrand.corp0.txt
$bin/ngramsymbols $tmpdata/testrand.corp0.txt $tmpdata/testrand.syms
farcompilestrings -symbols=$tmpdata/testrand.syms -keep_symbols=1 \
 $tmpdata/testrand.corp0.txt |\
$bin/ngramcount -order=$O1 - | \
$bin/ngrammake --check_consistency >$tmpdata/testrand.m1
$bin/ngramprint --check_consistency --ARPA $tmpdata/testrand.m1 \
  >$tmpdata/testrand.m1.arpa
$bin/ngramrandgen --seed=$seed2 --max_sents=$sents2 \
  $tmpdata/testrand.m1 >$tmpdata/testrand.corpus.far
$bin/ngramcount -order=$O2 $tmpdata/testrand.corpus.far | \
  $bin/ngrammake --check_consistency |\
  $bin/ngramshrink --check_consistency -theta=$T >$tmpdata/testrand.m2.mod
$bin/ngramprint --check_consistency --ARPA $tmpdata/testrand.m2.mod \
  >$tmpdata/testrand.m2.mod.arpa
$bin/ngramread --symbols=$tmpdata/testrand.syms --ARPA \
    $tmpdata/testrand.m2.mod.arpa | \
  $bin/ngramprint --check_consistency --ARPA | \
  cmp - $tmpdata/testrand.m2.mod.arpa
$bin/ngramread --ARPA $tmpdata/testrand.m1.arpa | \
  $bin/ngrammerge --check_consistency - $tmpdata/testrand.m2.mod | \
  $bin/ngramprint --check_consistency --ARPA - | \
  $bin/ngramread --ARPA | \
  $bin/ngramapply - $tmpdata/testrand.corp0.far >$tmpdata/testrand.apply.far;
