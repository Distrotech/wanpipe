#!/bin/sh

INSTALL_PATH=${1:-/usr}

if [ -d $INSTALL_PAT ]; then

eval "./init-automake.sh"
if [ $? -ne 0 ]; then
	exit $?
fi

eval "./configure --prefix=/usr"
if [ $? -ne 0 ]; then
	exit $?
fi
eval "make clean"
eval "make"
if [ $? -ne 0 ]; then
	exit $?
fi

eval "make install"
if [ $? -ne 0 ]; then
	exit $?
fi

else 

echo "Error: Path $INSTALL_PATH does not exist!\n"
exit 1

fi

exit 0
