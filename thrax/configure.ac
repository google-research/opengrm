AC_INIT([Thrax], [0.1.0], [help@www.opengrm.org])
AM_INIT_AUTOMAKE([foreign nostdinc -Wall -Werror])

AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_YACC

AC_CONFIG_HEADERS([config.h src/include/thrax/config.h])
AC_CONFIG_SRCDIR([src/lib/main/grm-compiler.cc])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/include/Makefile
  src/lib/Makefile
  src/bin/Makefile
  src/grammars/Makefile
  src/utils/Makefile
])

AC_CONFIG_MACRO_DIR([m4])
AC_LANG([C++])

AC_CHECK_TYPES([std::tr1::hash<long long unsigned>], [], [],
               [[#include <tr1/unordered_map>]])
AC_CHECK_TYPES([__gnu_cxx::slist<int>], [], [],
               [[#include <ext/slist>]])

AC_ARG_ENABLE(
	[bin],
	[AS_HELP_STRING(
		[--enable-bin],
		[enable command-line binaries])],
	[],
	[enable_bin=yes])
AM_CONDITIONAL([HAVE_BIN], [test "x$enable_bin" != xno])

AC_ARG_WITH([icu],
  [AS_HELP_STRING([--with-icu],
    [enable UTF8 support using the ICU Library])],
  [],
  [with_icu=no])
AS_IF([test "x$with_icu" != xno],
  [AC_CHECK_ICU(
    [4.2],
    [AC_DEFINE([HAVE_ICU],
      [1],
      [Define to 1 if you have the ICU library.])],
    [AC_MSG_FAILURE([--with-icu was given, but ICU Library not found])])])

AC_CHECK_HEADER([fst/fst.h], [],
 [AC_MSG_ERROR([fst/fst.h header not found])]
)

AC_CHECK_HEADER([fst/extensions/far/far.h], [],
 [AC_MSG_ERROR([fst/extensions/far/far.h header not found])]
)

AC_OUTPUT
