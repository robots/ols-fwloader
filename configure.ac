AC_INIT([ols-fwloader],[0.1],[michal.demin@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE


AC_LANG_C
AC_PROG_CC
AM_PROG_CC_C_O
# Libtool
LT_INIT

is_win32=no
is_mingw=no
is_darwin=no

case "$host" in
  *-mingw32*)
	 is_win32=yes
	 is_mingw=yes
	 AC_DEFINE(IS_MINGW, 1, [1 if building for MinGW.])
	 AC_DEFINE(IS_DARWIN, 0, [0 if not building for Darwin.])
	 AC_DEFINE(IS_WIN32, 1, [1 if building for Win32.])
   ;;
	*darwin*)
	 is_darwin=yes
	 AC_DEFINE(IS_MINGW, 0, [0 if not building for MinGW.])
	 AC_DEFINE(IS_DARWIN, 1, [1 if building for Darwin.])
	 AC_DEFINE(IS_WIN32, 0, [0 if not building for Win32.])
   ;;
  *)
	 AC_DEFINE(IS_MINGW, 0, [0 if not building for MinGW.])
	 AC_DEFINE(IS_DARWIN, 0, [0 if not building for Darwin.])
	 AC_DEFINE(IS_WIN32, 0, [0 if not building for Win32.])
  ;;
esac

# options
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[Enable debugging output]),[enable_debug=$enableval],[enable_debug="no"])

if test x"$enable_debug" = "xyes"
then
  CFLAGS="$CFLAGS -Wall DDEBUG -g"
fi

win32_LIBS=""
# for nonwin we need libusb
if test $is_win32 = no; then
  # check dependencies
  AC_PATH_PROG(PKG_CONFIG, pkg-config)
  if test x"$PKG_CONFIG" = x""; then
    AC_MSG_ERROR([pkg-config not found.])
  fi

  PKG_CONFIG_REQUIRES=""
	PKG_CHECK_MODULES([libusb], [libusb-1.0 >= 1.0.5],
		[CFLAGS="$CFLAGS $libusb_CFLAGS";
		LIBS="$LIBS $libusb_LIBS"])
	AC_CHECK_LIB(usb-1.0, libusb_init)
  AC_SUBST(PKG_CONFIG_REQUIRES)
else
	win32_LIBS="-lhid -lsetupapi"
fi
AC_SUBST(win32_LIBS)

AM_CONDITIONAL(IS_WIN32, test $is_win32 = yes)
AM_CONDITIONAL(IS_MINGW, test $is_mingw = yes)
AM_CONDITIONAL(IS_DARWIN, test $is_mingw = yes)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

