#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([NSpike], [2.0.0], [ckemere@gmail.com])
AM_INIT_AUTOMAKE(NSpike, 2.0.0)
AC_CONFIG_SRCDIR([src-main/spike_main.cpp])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([GL], [glBegin])
# FIXME: Replace `main' with a function in `-lX11':
# AC_CHECK_LIB([X11], [main])

AC_CHECK_LIB([avcodec], [avcodec_open])

AC_CHECK_LIB([m], [sin])
AC_CHECK_LIB([z], [gzwrite])

AC_MSG_CHECKING(QT4 directory)
QTDIR=xxx
AC_ARG_WITH([qt-dir],
             AC_HELP_STRING([--with-qt-dir=/path/to/Qt4],
                            [to specify the path to the Qt4 directory.]),
             [QTPATHS="$withval"],
             [QTPATHS="/usr/share/qt4 /usr/lib/qt4"])

for x in $QTPATHS; do
    if test -d $x/include ; then
       QTDIR="$x"
       AC_MSG_RESULT(found at $QTDIR)
       AC_SUBST([QTDIR])
    fi
done
if test $QTDIR = xxx ; then
   AC_MSG_ERROR(Could not locate QT4)
fi

AC_ARG_ENABLE([nspike-fixpos],
[  --enable-nspike-fixpos     Enable compilation of nspike-fixpos (requires glut and mpeg2 libraries)], 
[], [enable_nspike_fixpos=no])

AM_CONDITIONAL([MAKE_NSPIKE_FIXPOS], [test x$enable_nspike_fixpos = xyes])
# These are only needed for nspike-fixpos
AM_COND_IF([MAKE_NSPIKE_FIXPOS],[
  AC_CHECK_LIB([glut], [glut_init])
  AC_CHECK_LIB([mpeg2], [mpeg2_init])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero floor gethostbyaddr gethostbyname gethostname gettimeofday memset munmap select socket sqrt strchr strerror strrchr])

AC_PROG_INSTALL

AC_CONFIG_FILES([Makefile
           src-modules/Makefile
           src-extract/Makefile
           src-fixpos/Makefile
           src-user/Makefile
           src-main/Makefile])

AC_OUTPUT
