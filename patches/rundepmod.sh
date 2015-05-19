#!/bin/bash

if [ ! -e patches ]; then
  	echo "Error: This script must be run from wanpipe/ directory!"
	exit 1
fi

superuser=NO;
if [ "$UID" = 0 ]; then
	superuser=YES;
fi 

if [ $superuser = "YES" ]; then
	echo -n "Running Depmod ..."
	eval "depmod -a"
	if [ $? -eq 0 ]; then
          	echo "Ok"
	else
                echo "Failed: ($?)"
	fi
fi

