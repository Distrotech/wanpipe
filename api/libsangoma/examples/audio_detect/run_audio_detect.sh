#!/bin/bash

#echo "ARGS $1 $2"

ARG1=$1;
ARG2=$2;

timeout=${3:-5}


if [ "$ARG1" != "" ]; then
WANPIPES="$ARG1"
else
WANPIPES="1"
fi


IFACES="$ARG2"

if [ "$IFACES" = "all" ]; then
  	IFACES=
fi

IFACE_START=1

eval "grep \"FE_MEDIA.*=.*E1\" /etc/wanpipe/wanpipe1.conf > /dev/null"
if [ $? -ne 0 ]; then
	IFACE_STOP=24
	SKIP_DCHAN=24
else
	IFACE_STOP=31
	SKIP_DCHAN=16
fi

#Comment this out to include dchan
SKIP_DCHAN=100

CMD=

for wanpipe_num in $WANPIPES
do	

    if [ "$IFACES" = "" ]; then
		for ((i=$IFACE_START;i<=$IFACE_STOP;i+=1)); do
			if [ $i -eq $SKIP_DCHAN ]; then
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


echo "./audio_detect  $CMD" 

eval "nice -n 20 ./audio_detect  $CMD" 
rc=$?

exit $rc


