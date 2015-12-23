#!/bin/sh

CMD=${1:-none}


echo "$DEVS"
WAN_DIR=/etc/wanpipe


for ((i=1;i<=32;i++))
do
	voice_flag=0;
	sig_flag=0;

	if [ ! -e $WAN_DIR/wanpipe$i.conf ]; then
		continue
	fi

	status=`cat /proc/net/wanrouter/status | grep "wanpipe$i " | cut -d'|' -f4`
	status=${status// /}

	voice_cfg=`grep TDM_VOICE_API $WAN_DIR/wanpipe$i.conf`
	xmtp2_cfg=`grep XMTP2_API $WAN_DIR/wanpipe$i.conf`
	span_cfg=`grep TDMV_SPAN $WAN_DIR/wanpipe$i.conf | cut -d'=' -f2`
	span_cfg=${span_cfg// /}

	if [ "$voice_cfg" != "" ]; then
		voice_flag=1
	fi
	if [ "$xmtp2_cfg" != "" ]; then
		sig_flag=1
	fi

	if [ $voice_flag -eq 0 ] && [ $sig_flag -eq 0 ] ; then
		echo "========================================================"
		echo "Error: Invalid Interface configuration in  $WAN_DIR/wanpipe$i.conf"
		echo "========================================================"
		continue 
	fi
	if [ "$span_cfg" = " " ]; then
		echo "========================================================"
		echo "Error: Invalid Span configuration in  $WAN_DIR/wanpipe$i.conf"
		echo "========================================================"
		continue 
	fi
 
	if [ $i -lt 10 ]; then
		echo -n "wanpipe$i:  " 
	else
		echo -n "wanpipe$i: "
	fi
	echo -n "Sig=$sig_flag Voice=$voice_flag "

	if [ $span_cfg -lt 10 ]; then
		echo -n "Span=$span_cfg  "
	else
		echo -n "Span=$span_cfg "
	fi

	#echo "Status = $status" 

	if [ "$status" = "Connected" ]; then
		echo -n "Stat=Conn";
	else
		echo -n "Stat=Disc";
	fi
	echo

done

