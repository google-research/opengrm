#!/bin/bash
# Description:
# Compiles FARs required for testing command line binary ngramapply.

testdata=$srcdir/testdata
randgenname="${1}"
outpath="${2}"

FARNAME="${outpath}".apply
TAR="${randgenname}".apply.FSTtxt.tgz
SYM="${randgenname}".apply.sym
FSTOPTS="-keep_isymbols -keep_osymbols -keep_state_numbering"

set -e

farcompilestrings \
  -key_prefix="FST" -generate_keys=4 -symbols="${testdata}/${randgenname}".sym \
  -keep_symbols=1 "${testdata}/${randgenname}".txt >"${outpath}".far

rm -rf "${outpath}"
mkdir -p "${outpath}"
tar -xzf "${testdata}/${TAR}" -C "${outpath}"
fstcompile \
  -isymbols="${testdata}/${SYM}" -osymbols="${testdata}/${SYM}" \
  -keep_isymbols -keep_osymbols -keep_state_numbering \
  "${outpath}"/FST0001.txt > "${outpath}"/FST0001
ls "${outpath}"/FST????.txt | grep -v FST0001 | sed 's/.txt$//g' | \
  while read i; do
    fstcompile \
      "${i}".txt > "${i}";
  done
cd "${outpath}"
farcreate \
  FST???? "${FARNAME}".far.ref
