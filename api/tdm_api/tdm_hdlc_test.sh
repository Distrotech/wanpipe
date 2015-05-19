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

    if [ "$IFACES" = "" ]; then
		for ((i=$IFACE_START;i<=$IFACE_STOP;i+=1)); do
			if [ $i -eq 16 ]; then
              	continue
			fi
			#CMD=$CMD" w"$wanpipe_num"g"$i   
			CMD=$CMD" s"$wanpipe_num"c"$i   
		done
	else
		for if_num in $IFACES
		do
			CMD=$CMD" s"$wanpipe_num"c"$if_num
		done
	fi
done

echo "./aft_tdm_hdlc_test  $CMD" 

nice -n 20 ./aft_tdm_hdlc_test  $CMD 

