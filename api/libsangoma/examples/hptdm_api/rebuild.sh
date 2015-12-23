#!/bin/sh

home=$(pwd)

cd ../libsangoma
make
make install

cd $home
make clean
make


