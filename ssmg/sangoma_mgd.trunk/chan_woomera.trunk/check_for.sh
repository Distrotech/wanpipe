#!/bin/bash -p


search_arg=$1
file=$2
output=$3

echo "$search_arg $file $output " >> .log
if [ -f $file ]; then
	echo "grep -c $search_arg $file  $output" >> .log
	grep -c $search_arg $file > $output
else
	echo "echo 0 $output" >> .log
	echo 0 > $output
fi


