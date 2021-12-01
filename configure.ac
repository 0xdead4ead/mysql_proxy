dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([mysql_proxy], [1.0], [cplusplus@0xdead4ead.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT()

AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([tar-ustar])

AC_PROG_CXX
AC_LANG(C++)
AC_PROG_LIBTOOL
AX_BOOST_BASE([1.75.0], [], [
  AC_MSG_ERROR([Boost is not found!])])
AX_CHECK_OPENSSL([],[
  AC_MSG_ERROR([OpenSSL is not found!])])

AS_CASE([${enable_shared}], [yes], AC_DEFINE([BOOST_ALL_DYN_LINK]))

case $host in
  *-*-linux*)
    CXXFLAGS="$CXXFLAGS -pthread"
    LDFLAGS="$LDFLAGS -pthread"
    LIBS="$LIBS -lrt"
    ;;
  *-*-solaris*)
    if test "$GXX" = yes; then
      CXXFLAGS="$CXXFLAGS -D_PTHREADS"
    else
      # We'll assume Sun's CC.
      CXXFLAGS="$CXXFLAGS -mt"
    fi
    LIBS="$LIBS -lsocket -lnsl -lpthread"
    ;;
  *-*-mingw32*|*-*-mingw64*)
    CXXFLAGS="$CXXFLAGS -mthreads"
    LDFLAGS="$LDFLAGS -mthreads"
    LIBS="$LIBS -lws2_32 -lmswsock"
    ;;
  *-pc-cygwin*)
    CXXFLAGS="$CXXFLAGS -D__USE_W32_SOCKETS -D_WIN32_WINNT=0x0601"
    LIBS="$LIBS -lws2_32 -lmswsock"
    ;;
  *-*-freebsd*|*-*-netbsd*)
    CXXFLAGS="$CXXFLAGS -pthread"
    LDFLAGS="$LDFLAGS -pthread"
    ;;
  *-*-haiku*)
    CXXFLAGS="$CXXFLAGS -lnetwork"
    LDFLAGS="$LDFLAGS -lnetwork"

esac

AC_CONFIG_FILES([Makefile system/Makefile tracker/Makefile])
AC_OUTPUT
