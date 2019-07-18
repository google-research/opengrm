AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../script
LDADD = ../script/libbaumwelchscript.la -L/usr/local/lib/fst \
        -lfstfarscript -lfstfar -lfstscript -lfst -lm -ldl

if HAVE_BIN
bin_PROGRAMS = baumwelchtrain baumwelchdecode

baumwelchtrain_SOURCES = baumwelchtrain.cc baumwelchtrain-main.cc
baumwelchdecode_SOURCES = baumwelchdecode.cc baumwelchdecode-main.cc

endif
