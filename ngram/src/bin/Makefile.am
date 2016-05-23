AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl

bin_PROGRAMS = ngramapply \
               ngramcontext \
               ngramcount \
               ngraminfo \
               ngrammake \
               ngrammarginalize \
               ngrammerge \
               ngramperplexity \
               ngramprint \
               ngramrandgen \
               ngramread \
               ngramshrink \
               ngramsort \
               ngramsplit \
               ngramsymbols \
               ngramtransfer

dist_noinst_SCRIPTS = ngram.sh ngramfrac.sh

ngramapply_SOURCES = ngramapply_main.cc
ngramapply_LDADD = ../lib/libngram.la

ngramcontext_SOURCES = ngramcontext_main.cc
ngramcontext_LDADD = ../lib/libngram.la

ngramcount_SOURCES = ngramcount_main.cc
ngramcount_LDADD = ../lib/libngram.la ../lib/libngramhist.la

ngraminfo_SOURCES = ngraminfo_main.cc
ngraminfo_LDADD = ../lib/libngram.la

ngrammake_SOURCES = ngrammake_main.cc
ngrammake_LDADD = ../lib/libngram.la ../lib/libngramhist.la

ngrammarginalize_SOURCES = ngrammarginalize_main.cc
ngrammarginalize_LDADD = ../lib/libngram.la

ngrammerge_SOURCES = ngrammerge_main.cc
ngrammerge_LDADD = ../lib/libngram.la ../lib/libngramhist.la

ngramperplexity_SOURCES = ngramperplexity_main.cc
ngramperplexity_LDADD = ../lib/libngram.la

ngramprint_SOURCES = ngramprint_main.cc
ngramprint_LDADD = ../lib/libngram.la

ngramrandgen_SOURCES = ngramrandgen_main.cc
ngramrandgen_LDADD = ../lib/libngram.la

ngramread_SOURCES = ngramread_main.cc
ngramread_LDADD = ../lib/libngram.la

ngramshrink_SOURCES = ngramshrink_main.cc
ngramshrink_LDADD = ../lib/libngram.la

ngramsort_SOURCES = ngramsort_main.cc
ngramsort_LDADD = ../lib/libngram.la

ngramsplit_SOURCES = ngramsplit_main.cc
ngramsplit_LDADD = ../lib/libngram.la ../lib/libngramhist.la

ngramsymbols_SOURCES = ngramsymbols_main.cc
ngramsymbols_LDADD = ../lib/libngram.la

ngramtransfer_SOURCES = ngramtransfer_main.cc
ngramtransfer_LDADD = ../lib/libngram.la ../lib/libngramhist.la
