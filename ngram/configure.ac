AC_INIT([OpenGrm NGram], [1.2.0], [ngram@www.opengrm.org])
AM_INIT_AUTOMAKE([foreign nostdinc -Wall -Werror])

CXXFLAGS="$CXXFLAGS -std=c++0x"

AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/include/ngram/ngram-model.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/include/Makefile
  src/lib/Makefile
  src/bin/Makefile
  src/test/Makefile
])
AC_LANG([C++])

AC_CHECK_HEADER([fst/fst.h], [],
 [AC_MSG_ERROR([fst/fst.h header not found])]
)

AC_CHECK_HEADER([fst/extensions/far/far.h], [],
 [AC_MSG_ERROR([fst/extensions/far/far.h header not found])]
)

AC_OUTPUT
