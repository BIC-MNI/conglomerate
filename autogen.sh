#! /bin/sh

set -e

if [ ! -d m4 ]; then
    cat <<EOF
This package needs the MNI autoconf macros installed in directory named "m4".
You can check out that package from CVS using

    cvs -d ... checkout -d m4 mni-acmacros

(replace the dots by /software/source/libraries, or appropriate)

EOF
    exit 1
fi

aclocal -I m4
autoheader
libtoolize --automake
automake --add-missing
autoconf

