dnl Process this file with autoconf to produce a configure script.

AC_INIT
AC_CONFIG_SRCDIR([surface_mask2.c])
AM_INIT_AUTOMAKE(conglomerate, 1.0)
AM_CONFIG_HEADER(config.h)

smr_WITH_BUILD_PATH

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_PROG_CC
AC_PROG_CXX

AM_PROG_LIBTOOL

mni_REQUIRE_BICPL

# check for special header files needed
AC_CHECK_HEADER(numerical.h,,AC_MSG_ERROR([The file numerical.h must be includable - it should be part of the internal bicpl include files - so make sure that those are included in the CPPFLAGS]))

AC_CHECK_HEADER(internal_volume_io.h,,AC_MSG_ERROR([The file internal_volume_io.h must be includable - it should be part of the internal volume_io files - so make sure that those are included in the CPPFLAGS]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

