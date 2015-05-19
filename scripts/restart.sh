#!/bin/bash

while [ 1 ]; do
	
	wanrouter stop all
	wanrouter start

	cnt=0;
	
	while [ 1 ]; do
		cnt=$((cnt+1))	
		if [ $cnt -gt 30 ]; then
			echo "All ports connected timeout ... exiting (sec=$cnt)"
			exit 1;
		fi
		sleep 1
		rc=`wanrouter status | grep "Disconnected"`
		if [ "$rc" = "" ]; then
			echo "All ports connected (sec=$cnt)"
			sleep 1
			break;
		fi 
	done


done
