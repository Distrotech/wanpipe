#!/bin/sh

echo "ARGS $1 $2"

ARG1=$1;
ARG2=$2;


if [ "$ARG1" != "" ]; then
WANPIPES="$ARG1"
else
WANPIPES="1"
fi


IFACES="$ARG2"
IFACE_START=1
IFACE_STOP=31


CMD="wanpipe1"

for wanpipe_num in $WANPIPES
do	
	num=$((wanpipe_num-1))

    if [ "$IFACES" = "" ]; then
		for ((i=$IFACE_START;i<=$IFACE_STOP;i+=1)); do
			#CMD=$CMD" w"$wanpipe_num"g"$i   
			if [ $i -eq 16 ]; then
              	continue
			fi
			chan=$((num*31+$i))
			CMD=$CMD" /dev/dahdi/"$chan
		done
	else
		for if_num in $IFACES
		do
			chan=$((num*31+if_num))
			CMD=$CMD" /dev/dahdi/"$chan
		done
	fi
done

echo "./aft_tdm_hdlc_test  $CMD" 

nice -n 20 ./aft_tdm_hdlc_test_zap  $CMD 

