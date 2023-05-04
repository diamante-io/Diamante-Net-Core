# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([xdrpp],[0])
AM_INIT_AUTOMAKE([-Wall subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_CPP
# gcc does this weird thing where it interprets everything (including
# .x files) as a linker script without the -xc option.
AC_MSG_CHECKING([if C preprocessor can be invoked as $CPP -xc])
if $CPP -xc /dev/null > /dev/null 2>&1; then
   CPP="$CPP -xc"
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi
AC_DEFINE_UNQUOTED([CPP_COMMAND], "$CPP", [Shell command for C preprocessor])

AM_PROG_LEX
AC_CHECK_PROGS(YACC, 'bison -y', bison -y)
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX([clang++ eg++ g++ c++ gpp aCC CC cxx cc++ cl.exe FCC KCC RCC xlC_r xlC])
AX_CXX_COMPILE_STDCXX(14,,mandatory)
test -z "$WFLAGS" || CXXFLAGS="$CXXFLAGS $WFLAGS"
AC_LANG(C++)
# -pthread Seems to be required by g++ -stc=c++1[14]
AX_APPEND_COMPILE_FLAGS([-pthread])

AC_C_BIGENDIAN
AC_C_BIGENDIAN(IS_BIG_ENDIAN=1, IS_BIG_ENDIAN=0)
AC_SUBST(IS_BIG_ENDIAN)

AC_MSG_CHECKING(for cereal)
cereal_CPPFLAGS=
if test -n "$srcdir" -a -d "$srcdir/cereal/include/cereal"; then
   cereal_CPPFLAGS='-I$(top_srcdir)/cereal/include'
   USE_CEREAL=yes
elif test -d cereal/include/cereal; then
   cereal_CPPFLAGS='-I$(top_builddir)/cereal/include'
   USE_CEREAL=yes
fi
AM_CONDITIONAL([USE_CEREAL], [test yes = "$USE_CEREAL"])
AC_MSG_RESULT(${USE_CEREAL:-no})
AC_SUBST(cereal_CPPFLAGS)

AC_MSG_CHECKING(for autocheck)
autocheck_CPPFLAGS=
if test -n "$srcdir" -a -d "$srcdir/autocheck/include/autocheck"; then
   autocheck_CPPFLAGS='-I$(top_srcdir)/autocheck/include'
   USE_AUTOCHECK=yes
elif test -d autocheck/include/autocheck; then
   autocheck_CPPFLAGS='-I$(top_builddir)/autocheck/include'
   USE_AUTOCHECK=yes
fi
AM_CONDITIONAL([USE_AUTOCHECK], [test yes = "$USE_AUTOCHECK"])
AC_MSG_RESULT(${USE_AUTOCHECK:-no})
AC_SUBST(autocheck_CPPFLAGS)

rm -f getopt.h
if test -z "${NEED_GETOPT_LONG+set}"; then
   AC_CHECK_FUNCS(getopt_long_only, NEED_GETOPT_LONG=no, NEED_GETOPT_LONG=yes)
fi
if test yes = "$NEED_GETOPT_LONG"; then
   AC_CONFIG_LINKS(getopt.h:compat/getopt.h)
fi
AM_CONDITIONAL([NEED_GETOPT_LONG], [test yes = "$NEED_GETOPT_LONG"])

test -z "$XDRC" -a "x$cross_compiling" != xyes \
     && XDRC='$(top_builddir)/xdrc/xdrc$(EXEEXT)'
if test -z "$XDRC"; then
   AC_PATH_PROG(XDRC, [xdrc])
   if test -z "$XDRC"; then
      AC_MSG_ERROR(Need native xdrc when cross compiling)
   fi
fi

if test -n "${MSVC+set}"; then
   MSVC=1
else
   MSVC=
fi
AC_SUBST(MSVC)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile xdrpp/build_endian.h xdrpp.pc xdrpp-uninstalled.pc])
AC_OUTPUT

