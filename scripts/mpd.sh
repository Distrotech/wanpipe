#!/bin/bash

CMD=${1:-none}

DEVS=$(cat /proc/net/wanrouter/status | grep wanpipe | cut -d' ' -f1) 

echo "$DEVS"

#echo $(date) >> stats.out
for dev in $DEVS
do
	wan_ec_client $dev mpd all
done

