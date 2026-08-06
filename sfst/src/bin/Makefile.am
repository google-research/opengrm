AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl



bin_PROGRAMS = sfstapprox \
               sfstcount \
               sfstinfo \
               sfstintersect \
               sfstngramapprox \
               sfstnormalize \
	       sfstperplexity \
               sfstrandgen \
	       sfstshortestdistance \
	       sfsttopology \
               sfsttrim

sfstapprox_SOURCES = sfstapprox.cc
sfstapprox_LDADD = ../lib/libsfst.la

sfstcount_SOURCES = sfstcount.cc
sfstcount_LDADD = ../lib/libsfst.la

sfstinfo_SOURCES = sfstinfo.cc
sfstinfo_LDADD = ../lib/libsfst.la

sfstintersect_SOURCES = sfstintersect.cc
sfstintersect_LDADD = ../lib/libsfst.la

sfstngramapprox_SOURCES = sfstngramapprox.cc
sfstngramapprox_LDADD = ../lib/libsfst.la

sfstnormalize_SOURCES = sfstnormalize.cc
sfstnormalize_LDADD = ../lib/libsfst.la

sfstperplexity_SOURCES = sfstperplexity.cc
sfstperplexity_LDADD = ../lib/libsfst.la

sfstrandgen_SOURCES = sfstrandgen.cc
sfstrandgen_LDADD = ../lib/libsfst.la

sfstshortestdistance_SOURCES = sfstshortestdistance.cc
sfstshortestdistance_LDADD = ../lib/libsfst.la

sfsttopology_SOURCES = sfsttopology.cc
sfsttopology_LDADD = ../lib/libsfst.la

sfsttrim_SOURCES = sfsttrim.cc
sfsttrim_LDADD = ../lib/libsfst.la

