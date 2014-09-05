#!/bin/bash

cmd=$1;

if [ ! -e wan_aftup ]; then
	eval "make clean > /dev/null"
	eval "make > /dev/null"
fi

if [ -e /proc/net/wanrouter ]; then

	echo
	echo "Warning: Wanpipe modules loaded"
	echo "         Please remove wanpipe modules from the kernel"
	echo
	echo "         eg: wanrouter stop"
	echo
	exit 1
fi


trap '' 2

eval "./scripts/load.sh"

if [ "$cmd" == "auto" ]; then
	eval "./wan_aftup -auto"
else
	eval "./wan_aftup "
fi

eval "./scripts/unload.sh"



