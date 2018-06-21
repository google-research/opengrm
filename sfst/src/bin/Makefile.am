AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl

bin_PROGRAMS = sfstapprox \
               sfstinfo \
               sfstngramapprox \
               sfstnormalize \
	       sfstperplexity \
               sfstrandgen \
               sfsttrim



sfstapprox_SOURCES = sfstapprox.cc
sfstapprox_LDADD = ../lib/libsfst.la

sfstinfo_SOURCES = sfstinfo.cc
sfstinfo_LDADD = ../lib/libsfst.la

sfstngramapprox_SOURCES = sfstngramapprox.cc
sfstngramapprox_LDADD = ../lib/libsfst.la

sfstnormalize_SOURCES = sfstnormalize.cc
sfstnormalize_LDADD = ../lib/libsfst.la

sfstperplexity_SOURCES = sfstperplexity.cc
sfstperplexity_LDADD = ../lib/libsfst.la

sfstrandgen_SOURCES = sfstrandgen.cc
sfstrandgen_LDADD = ../lib/libsfst.la

sfsttrim_SOURCES = sfsttrim.cc
sfsttrim_LDADD = ../lib/libsfst.la

