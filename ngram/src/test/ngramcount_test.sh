#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/earnest.cnts" 0 2 13 15

set -e

farcompilestrings -symbols=$testdata/earnest.syms -keep_symbols=1 \
	$testdata/earnest.txt >$tmpdata/earnest.far 
farequal $testdata/earnest.far $tmpdata/earnest.far

# Counting from set of string Fsts
$bin/ngramcount -order=5 $testdata/earnest.far >$tmpdata/earnest.cnts
fstequal $testdata/earnest.cnts $tmpdata/earnest.cnts

# Counting from an Fst representing a union of paths
$bin/ngramcount -order=5 $testdata/earnest.fst.far >$tmpdata/earnest.cnts
fstequal $testdata/earnest-fst.cnts $tmpdata/earnest.cnts

# Counting from the determinitic "tree" Fst representing the corpus
$bin/ngramcount -order=5 $testdata/earnest.det.far >$tmpdata/earnest.cnts
fstequal $testdata/earnest-det.cnts $tmpdata/earnest.cnts

# Counting from the minimal deterministic Fst representing the corpus
$bin/ngramcount -order=5 $testdata/earnest.min.far >$tmpdata/earnest.cnts
fstequal $testdata/earnest-min.cnts $tmpdata/earnest.cnts
