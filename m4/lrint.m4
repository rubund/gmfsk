dnl @synopsis AC_C99_FUNC_LRINT
dnl
dnl Check whether C99's lrint function is available.
dnl @version 1.3	Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LRINT],
[AC_CACHE_CHECK(for lrint,
  ac_cv_c99_lrint,
[
lrint_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
], if (!lrint(3.14159)) lrint(2.7183);, ac_cv_c99_lrint=yes, ac_cv_c99_lrint=no)

CFLAGS=$lrint_save_CFLAGS

])

if test "$ac_cv_c99_lrint" = yes; then
  AC_DEFINE(HAVE_LRINT, 1,
            [Define if you have C99's lrint function.])
fi
])# AC_C99_FUNC_LRINT
dnl @synopsis AC_C99_FUNC_LRINTF
dnl
dnl Check whether C99's lrintf function is available.
dnl @version 1.3	Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LRINTF],
[AC_CACHE_CHECK(for lrintf,
  ac_cv_c99_lrintf,
[
lrintf_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
], if (!lrintf(3.14159)) lrintf(2.7183);, ac_cv_c99_lrintf=yes, ac_cv_c99_lrintf=no)

CFLAGS=$lrintf_save_CFLAGS

])

if test "$ac_cv_c99_lrintf" = yes; then
  AC_DEFINE(HAVE_LRINTF, 1,
            [Define if you have C99's lrintf function.])
fi
])# AC_C99_FUNC_LRINTF
dnl @synopsis AC_C99_FUNC_LLRINT
dnl
dnl Check whether C99's llrint function is available.
dnl @version 1.1	Sep 30 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LLRINT],
[AC_CACHE_CHECK(for llrint,
  ac_cv_c99_llrint,
[
llrint_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
#include <stdint.h>
], int64_t	x ; x = llrint(3.14159) ;, ac_cv_c99_llrint=yes, ac_cv_c99_llrint=no)

CFLAGS=$llrint_save_CFLAGS

])

if test "$ac_cv_c99_llrint" = yes; then
  AC_DEFINE(HAVE_LLRINT, 1,
            [Define if you have C99's llrint function.])
fi
])# AC_C99_FUNC_LLRINT
