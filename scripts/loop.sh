#!/bin/bash

CMD=${1:-none}

DEVS=$(cat /proc/net/dev | egrep "w.*g" | cut -d':' -f1 | xargs) 

echo "$DEVS"

#echo $(date) >> stats.out
for dev in $DEVS
do
	if [ "$CMD" = "clear" ]; then
		wanpipemon -i $dev -c Tddlb
	else	
		wanpipemon -i $dev -c Tadlb
	fi
	#ifconfig $dev | grep dropped
	#line=`wanpipemon -i $dev -c sc | grep "overrun"`
	#echo "IF => $dev $line" | tee -a stats.out
done

