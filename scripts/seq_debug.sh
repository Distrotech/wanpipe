#!/bin/bash

CMD=${1:-none}

DEVS=$(cat /proc/net/dev | egrep "w.*g" | cut -d':' -f1 | xargs) 

echo "$DEVS"

for dev in $DEVS
do
	if [ "$CMD" = "on" ]; then
		wanpipemon -i $dev -c de_span_seq
	else
		wanpipemon -i $dev -c dd_span_seq
	fi	
done

