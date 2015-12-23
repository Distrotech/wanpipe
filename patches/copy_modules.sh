#!/bin/bash

from=$1
to=$2
echo "Coping $from to $to"

eval "cp $(find $from -name '*.ko' | xargs) $to"
exit $?
