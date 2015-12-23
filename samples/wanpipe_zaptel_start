#!/bin/bash

#Make sure that udev/devfs zaptel device
#has come up.
cnt=0
max_delay=30
for ((i=0;i<$max_delay;i++))
do
	if [ -e /dev/zap ]; then
        	break;
	fi

	echo "Waiting for zaptel device /dev/zap ($i/$max_delay)..."
  	sleep 2
done

if [ ! -e /dev/zap ]; then
	echo
	echo "Error: Zaptel device failed to come up";
        echo "Possible Cause: UDEV not installed!";
	echo
	exit 1
fi 

sleep 1

ztcfg -vvvv   

exit 0
