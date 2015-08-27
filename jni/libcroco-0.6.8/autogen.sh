#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="libcroco"

REQUIRED_AUTOMAKE_VERSION=1.7

(test -f $srcdir/configure.in \
  && test -f $srcdir/README \
  && test -f $srcdir/src/cr-parser.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}

if test $# -eq 0 ; then
    default_args="--enable-tests=yes --enable-seleng=auto --enable-layeng=auto"
fi

REQUIRED_AUTOMAKE_VERSION=1.7.2
USE_GNOME2_MACROS=1 . gnome-autogen.sh $default_args
