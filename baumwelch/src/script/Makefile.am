AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl

if HAVE_SCRIPT
lib_LTLIBRARIES  = libbaumwelchscript.la
libbaumwelchscript_la_SOURCES = decodescript.cc getters.cc trainscript.cc
libbaumwelchscript_la_LDFLAGS  = -version-info 28:0:0
endif
