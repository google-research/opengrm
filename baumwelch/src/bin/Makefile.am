AM_CPPFLAGS = -I$(srcdir)/../include -I$(srcdir)/../script
LDADD = ../script/libbaumwelchscript.la -L/usr/local/lib/fst \
        -lfstfarscript -lfstfar -lfstscript -lfst -lm -ldl

if HAVE_BIN
bin_PROGRAMS = baumwelchdecode baumwelchrandomize baumwelchtrain

baumwelchdecode_SOURCES = baumwelchdecode.cc baumwelchdecode-main.cc
baumwelchrandomize_SOURCES = baumwelchrandomize.cc baumwelchrandomize-main.cc
baumwelchtrain_SOURCES = baumwelchtrain.cc baumwelchtrain-main.cc

endif
