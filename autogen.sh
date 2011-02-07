#!/bin/sh

OS=`uname`

LIBTOOLIZE=libtoolize
ACLOCAL_DIR=

if [ "x$OS" = "xDarwin" ]; then
	LIBTOOLIZE=glibtoolize
	# fink installs aclocal macros here
	ACLOCAL_DIR="-I /sw/share/aclocal"
elif [ "x$OS" = "xMINGW32_NT-5.1" ]; then
	ACLOCAL_DIR="-I /usr/local/share/aclocal"
fi

echo "Generating build system..."
${LIBTOOLIZE} --install --copy --quiet || exit 1
aclocal ${ACLOCAL_DIR} || exit 1
autoheader || exit 1
automake --add-missing --copy --foreign || exit 1
autoconf || exit 1

