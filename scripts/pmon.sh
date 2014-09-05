#!/bin/sh

line=${1:t1}

./stats.sh clear

if [ $line = "t1" ]; then
watch -d -n 1 ./stats.sh pmon
else
watch -d -n 1 ./stats.sh pmon_e1
fi
