#!/bin/bash
# Description:
# Compiles FARs required for testing command line binary ngramapply.

testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

FARNAME="${1}".apply
TAR="${FARNAME}".FSTtxt.tgz
SYM="${FARNAME}".sym
FSTOPTS="-keep_isymbols -keep_osymbols -keep_state_numbering"

farcompilestrings \
  -key_prefix="FST" -generate_keys=4 -symbols="${testdata}/${1}".sym \
  -keep_symbols=1 "${testdata}/${1}".txt >"${tmpdata}/${1}".far

rm -rf "${tmpdata}"/FST*
tar -xzf "${testdata}/${TAR}" -C "${tmpdata}"
fstcompile \
  -isymbols="${testdata}/${SYM}" -osymbols="${testdata}/${SYM}" \
  -keep_isymbols -keep_osymbols -keep_state_numbering \
  "${tmpdata}"/FST0001.txt > "${tmpdata}"/FST0001
ls "${tmpdata}"/FST????.txt | grep -v FST0001 | sed 's/.txt$//g' | \
  while read i; do
    fstcompile "${i}".txt > "${i}";
  done
cd "${tmpdata}"
farcreate \
  FST???? "${FARNAME}".far.ref
rm FST*
