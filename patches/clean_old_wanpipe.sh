#!/bin/bash


if [ ! -e patches ]; then
  	echo "Error: This script must be run from wanpipe/ directory!"
	exit 1
fi

superuser=NO;
if [ "$UID" = 0 ]; then
	superuser=YES;
fi   

LWH_DIR=${1:-patches/kdrivers/include}
KWH_DIR=${2:-/lib/modules/$(uname -r)/build/include/linux}
KWH_INC_DIR=/usr/include/linux
WH_INC_DIR=/usr/include/wanpipe

if [ -e $KWH_DIR/zaptel.h ]; then 
	rm -f $KWH_DIR/zaptel.h
fi

W_H_FILES=`find $LWH_DIR -name "*.h" | xargs`
for file_raw in $W_H_FILES
do
    file=${file_raw##*\/} 

	if [ $file = "wanrouter.h" ]; then
      	continue;
	fi

    if [ -e $KWH_DIR/$file ]; then
      	rm -f $KWH_DIR/$file
    fi

    if [ $superuser = "YES" ]; then
	    if [ -e $KWH_INC_DIR/$file ]; then
			rm -f $KWH_INC_DIR/$file
	    fi
	    if [ -e $WH_INC_DIR/$file ]; then
			rm -f $WH_INC_DIR/$file
	    fi
    fi
done
    
if [ $superuser = "YES" ]; then
	if [ -d $WH_INC_DIR ]; then
		\rm -rf $WH_INC_DIR
		\mkdir -p $WH_INC_DIR
	fi
fi

