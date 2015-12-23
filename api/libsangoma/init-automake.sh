#!/bin/sh

PWD=$(pwd)

eval "autoheader"
if [ $? -ne 0 ]; then
	exit $?
fi
eval "libtoolize --force"
if [ $? -ne 0 ]; then
	exit $?
fi
eval "aclocal"
if [ $? -ne 0 ]; then
	exit $?
fi
eval "automake --add-missing"
if [ $? -ne 0 ]; then
	exit $?
fi
eval "autoconf"
if [ $? -ne 0 ]; then
	exit $?
fi

cd sample_c
if [ -f Makefile ]; then
	rm -f Makefile
fi
ln -s Makefile.Linux Makefile

cd ..

cd sample_cpp
if [ -f Makefile ]; then
	rm -f Makefile
fi
ln -s Makefile.Linux Makefile

echo "Automake Init Done"
echo 

echo "Please run " 
echo " -> ./configure --prefix=/usr"
echo " -> make "
echo " -> make install"           

