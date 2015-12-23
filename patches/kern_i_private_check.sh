#!/bin/bash

if [ ! -e patches ]; then
  	echo "Error: This script must be run from wanpipe/ directory!"
	exit 1
fi

RESULT_FILE="patches/i_private_found"
if [ -e $RESULT_FILE ]; then
  	rm -f $RESULT_FILE
fi


SOURCEDIR=${1:-/lib/modules/$(uname -r)/build}

if [ -e $SOURCEDIR/include/linux/fs.h ];then
       	eval "grep i_private $SOURCEDIR/include/linux/fs.h >/dev/null 2>/dev/null"
       	if [ $? -eq 0 ]; then 
		touch $RESULT_FILE
	fi
fi
