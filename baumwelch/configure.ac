AC_INIT([BaumWelch], [0.2.8], [kbg@google.com])
AM_INIT_AUTOMAKE([foreign nostdinc -Wall -Werror])
AM_PROG_AR

# This library, like OpenFst, does not throw exceptions, so we do not generate
# exception handling code. However, users are free to re-enable exception
# handling.
# This library, like OpenFst assumes char is unsigned; -fsigned-char is likely
# unsafe.
CPPFLAGS="$CPPFLAGS -fno-exceptions -funsigned-char"
CXXFLAGS="$CXXFLAGS -std=c++11"

AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/bin/Makefile
  src/include/Makefile
  src/script/Makefile
])

AC_CONFIG_MACRO_DIR([m4])
AC_LANG([C++])

AC_CHECK_HEADER([fst/fstlib.h], [],
  [AC_MSG_ERROR([fst/fstlib.h header not found])]
)

AC_CHECK_HEADER([fst/script/fstscript.h], [],
  [AC_MSG_ERROR([fst/script/fstscript.h header not found])]
)

AC_CHECK_HEADER([fst/extensions/far/far.h], [],
  [AC_MSG_ERROR([fst/extensions/far/far.h header not found])]
)

AC_CHECK_LIB([dl], dlopen, [DL_LIBS=-ldl])
AC_SUBST([DL_LIBS])

# --enable-bin enables script and bin "extensions".
AC_ARG_ENABLE([bin],
              [AS_HELP_STRING([--enable-bin],
              [enable fst::script and command-line binaries])],
              [],
              [enable_bin=yes])
AM_CONDITIONAL([HAVE_BIN], [test "x$enable_bin" != xno])
AM_CONDITIONAL([HAVE_SCRIPT], [test "x$enable_bin" != xno])

AC_OUTPUT
