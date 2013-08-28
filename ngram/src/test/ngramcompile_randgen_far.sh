#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

FARNAME=earnest.randgen.apply
TAR=$FARNAME.FSTtxt.tgz
SYM=$FARNAME.sym
FSTOPTS="-keep_isymbols -keep_osymbols -keep_state_numbering"

farcompilestrings -key_prefix="FST" -generate_keys=4 \
  -symbols=$testdata/earnest.randgen.sym -keep_symbols=1 \
  $testdata/earnest.randgen.txt >$tmpdata/earnest.randgen.far

rm -rf $tmpdata/FST*
tar -xzf $testdata/$TAR -C $tmpdata
fstcompile -isymbols=$testdata/$SYM -osymbols=$testdata/$SYM $FSTOPTS \
   $tmpdata/FST0001.txt > $tmpdata/FST0001
ls $tmpdata/FST????.txt | grep -v FST0001 | sed 's/.txt$//g' | \
  while read i; do fstcompile $i.txt > $i; done
cd $tmpdata
farcreate FST???? $FARNAME.far.ref
rm FST*
