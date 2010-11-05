#!/bin/sh
#
aclocal
automake --gnu -af
autoconf
cd src-gui; qmake; cd ..

echo "Now you are ready to run ./configure"
echo "remember to run it as ./configure --bindir=/usr/local/nspike"
