#!/bin/bash

home=$(pwd)

if [ ! -d patches ]; then
	echo "This program must be run from wanpipe/ directory"
	exit 0
fi
	
cd patches/kdrivers/src/net
if [ -e sdladrv_src.c ]; then
	rm sdladrv_src.c
fi
ln -s sdladrv.c sdladrv_src.c

cd $home

cd patches/kdrivers/src/wanrouter
if [  -e af_wanpipe_src.c ]; then
	rm af_wanpipe_src.c
fi
ln -s af_wanpipe.c af_wanpipe_src.c

cd $home

cd patches/kdrivers/src/wan_aften
if [  -e wan_aften_src.c ]; then
	rm wan_aften_src.c
fi
ln -s wan_aften.c wan_aften_src.c


cd $home

cd patches/kdrivers/include
if [  -e linux ]; then
	rm linux
fi
ln -s . linux

cd $home

echo "Done"

exit 0
