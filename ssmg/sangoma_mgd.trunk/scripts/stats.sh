#!/bin/sh

CMD=${1:-none}

DEVS=$(cat /proc/net/dev | egrep "w.g" | cut -d':' -f1 | sort | xargs) 
DEVSS=$(cat /proc/net/dev | egrep "w..g" | cut -d':' -f1 | sort | xargs) 

DEVS="$DEVS $DEVSS"

echo "$DEVS"

echo $(date) >> stats.out
for dev in $DEVS
do
	if [ "$CMD" = "clear" ]; then
		wanpipemon -i $dev -c fc
	fi	
	line=`wanpipemon -i $dev -c sc | grep "overrun"`
	echo "IF => $dev $line" | tee -a stats.out
done

