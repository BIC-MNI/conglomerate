#! /bin/sh

set -e

if [ ! -d m4 ]; then
    cat <<EOF
This package needs the MNI autoconf macros installed in directory named "m4".
You can check out that package from CVS using

    cvs -d /software/source checkout -d m4 libraries/mni-acmacros

(replace /software/source by user@shadow:/software/source, if remote)

EOF
    exit 1
fi

aclocal -I m4
autoheader
libtoolize --automake
automake --add-missing
autoconf

