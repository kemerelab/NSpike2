#!/bin/sh
#
aclocal
automake --gnu -af
autoconf

echo "Now you are ready to run ./configure"
echo "remember to run it as ./configure --bindir=/usr/local/nspike"
