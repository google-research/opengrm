AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl

if HAVE_SCRIPT
lib_LTLIBRARIES  = libbaumwelchscript.la
libbaumwelchscript_la_SOURCES = decodescript.cc \
                                randomizescript.cc \
								trainscript.cc
libbaumwelchscript_la_LDFLAGS  = -version-info 33:0:0
endif
