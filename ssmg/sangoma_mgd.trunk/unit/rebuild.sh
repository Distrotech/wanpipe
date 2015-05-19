#!/bin/sh

if [ "$1" = "all" ]; then
	make clean
fi
make
make install
