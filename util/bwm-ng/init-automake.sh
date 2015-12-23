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
eval "automake -a"
if [ $? -ne 0 ]; then
	exit $?
fi
eval "autoconf"
if [ $? -ne 0 ]; then
	exit $?
fi

echo "Automake Init Done"
echo 

echo "Please run " 
echo " -> ./configure --prefix=/usr"
echo " -> make "
echo " -> make install"           

