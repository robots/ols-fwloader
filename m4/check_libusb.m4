dnl Copied from Libnfc project, modified for libusb-1.0
dnl
dnl Check for LIBUSB-1.0
dnl On success, HAVE_LIBUSB is set to 1 and PKG_CONFIG_REQUIRES is filled when
dnl libusb is found using pkg-config

AC_DEFUN([OLS_CHECK_LIBUSB],
[
  HAVE_LIBUSB=0
  AC_ARG_WITH([libusb],
      [AS_HELP_STRING([--with-libusb], [use libusb from the following location])],
      [LIBUSB_DIR=$withval],
      [LIBUSB_DIR=""])

  # --with-libusb directory have been set
  if test "x$LIBUSB_DIR" != "x"; then
    AC_MSG_NOTICE(["use libusb from $LIBUSB_DIR"])
    libusb_CFLAGS="-I$LIBUSB_DIR/include"
    libusb_LIBS="-L$LIBUSB_DIR/lib/gcc -lusb"
    HAVE_LIBUSB=1
  fi

  # Search using pkg-config
  if test x"$HAVE_LIBUSB" = "x0"; then  
    if test x"$PKG_CONFIG" != "x"; then
      PKG_CHECK_MODULES([libusb], [libusb-1.0], [HAVE_LIBUSB=1], [HAVE_LIBUSB=0])
      if test x"$HAVE_LIBUSB" = "x1"; then
        if test x"$PKG_CONFIG_REQUIRES" != x""; then
          PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES,"
        fi
        PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES libusb"
      fi
    fi
  fi

  # Search using libusb-config
  if test x"$HAVE_LIBUSB" = "x0"; then
    AC_PATH_PROG(libusb_CONFIG,libusb-config)
    if test x"$libusb_CONFIG" != "x" ; then
      libusb_CFLAGS=`$libusb_CONFIG --cflags`
      libusb_LIBS=`$libusb_CONFIG --libs`

      HAVE_LIBUSB=1
    fi
  fi

  # Search the library and headers directly (last chance)
  if test x"$HAVE_LIBUSB" = "x0"; then
    AC_CHECK_HEADER(libusb.h, [], [AC_MSG_ERROR([The libusb headers are missing])])
    AC_CHECK_LIB(usb-1.0, libusb_init, [], [AC_MSG_ERROR([The libusb library is missing])])

    libusb_LIBS="-lusb-1.0"
    HAVE_LIBUSB=1
  fi

  if test x"$HAVE_LIBUSB" = "x0"; then
    AC_MSG_ERROR([libusb-1.0 is mandatory.])
  fi

  AC_SUBST(libusb_LIBS)
  AC_SUBST(libusb_CFLAGS)
])
