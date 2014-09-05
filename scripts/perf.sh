#!/bin/bash

dev=$1

if [ -z $dev ]; then
  	echo 
	echo "Usage: $0 <ifname>"
	echo 
	echo "   eg: $0 w1g1"
 	echo
	exit 1
fi

wanpipemon -i $dev -c dperf_on
wanpipemon -i $dev -c fperf

while [ 1 ]
do
	sleep 1
	wanpipemon -i $dev -c dperf
	wanpipemon -i $dev -c fperf

done

