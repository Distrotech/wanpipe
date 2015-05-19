#!/bin/bash

TEMP="/tmp/temp."$$
files=$1
src=$2
rep=$3

if [ "$files" = "all" ]; then
	files="1 2 3 4 5 6 7 8"
fi

echo "In file: $files  Change '$src'  to '$rep'"
echo "Are you sure? (y/n): "
read response

if [ "$response" != "y" ]; then
	echo "User aborted"
	exit 1
fi


for fnum in $files
do

file="wanpipe"$fnum".conf"

if [ ! -e $file ]; then
  	echo "Error $file not found"
	continue
fi

eval "cat $file | awk '{ gsub(\"$src\", \"$rep\") ; print }' > $TEMP"
if [ $? -ne 0 ]; then
	echo "Replace Failed"
	exit 1
fi
eval "cp $TEMP $file"

done

echo "Done: $?" 
