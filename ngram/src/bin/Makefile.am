AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl

bin_PROGRAMS = ngramapply ngramcount ngrammake ngrammerge ngramprint \
ngramread ngramshrink ngramperplexity ngramrandgen ngraminfo \
ngramsymbols ngramsort ngramcontext ngrammarginalize ngramtransfer \
ngramsplit

dist_noinst_SCRIPTS = ngram.sh

ngramapply_SOURCES = ngramapply.cc
ngramapply_LDADD = ../lib/libngram.la

ngramcount_SOURCES = ngramcount.cc
ngramcount_LDADD = ../lib/libngram.la

ngrammake_SOURCES = ngrammake.cc
ngrammake_LDADD = ../lib/libngram.la

ngrammarginalize_SOURCES = ngrammarginalize.cc
ngrammarginalize_LDADD = ../lib/libngram.la

ngrammerge_SOURCES = ngrammerge.cc
ngrammerge_LDADD = ../lib/libngram.la

ngramprint_SOURCES = ngramprint.cc
ngramprint_LDADD = ../lib/libngram.la

ngramread_SOURCES = ngramread.cc
ngramread_LDADD = ../lib/libngram.la

ngramsymbols_SOURCES = ngramsymbols.cc
ngramsymbols_LDADD = ../lib/libngram.la

ngramshrink_SOURCES = ngramshrink.cc
ngramshrink_LDADD = ../lib/libngram.la

ngramperplexity_SOURCES = ngramperplexity.cc
ngramperplexity_LDADD = ../lib/libngram.la

ngramrandgen_SOURCES = ngramrandgen.cc
ngramrandgen_LDADD = ../lib/libngram.la

ngraminfo_SOURCES = ngraminfo.cc
ngraminfo_LDADD = ../lib/libngram.la

ngramsort_SOURCES = ngramsort.cc
ngramsort_LDADD = ../lib/libngram.la

ngramcontext_SOURCES = ngramcontext.cc
ngramcontext_LDADD = ../lib/libngram.la

ngramtransfer_SOURCES = ngramtransfer.cc
ngramtransfer_LDADD = ../lib/libngram.la

ngramsplit_SOURCES = ngramsplit.cc
ngramsplit_LDADD = ../lib/libngram.la
