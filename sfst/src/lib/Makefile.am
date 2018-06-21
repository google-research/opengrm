AM_CPPFLAGS = -I$(srcdir)/../include

lib_LTLIBRARIES = libsfst.la

libsfst_la_SOURCES = shortest-distance.cc \
                     stationary-distrib.cc

libsfst_la_LDFLAGS = -version-info 1:0:0 -lfst -lm
libsfst_la_LIBADD = $(DL_LIBS)
