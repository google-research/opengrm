AM_CPPFLAGS = -I$(srcdir)/../include

lib_LTLIBRARIES = libsfst.la

libsfst_la_SOURCES = stationary-distrib.cc

libsfst_la_LDFLAGS = -version-info 3:0:0 -lfst -lm
libsfst_la_LIBADD = $(DL_LIBS)
