#!/bin/bash

CMD=none
token=overrun
rec=0

while [ ! -z $1 ]
do
  	if [ "$1" = "clear" ]; then
      	CMD=clear
	fi
	if [ "$1" = "overrun" ]; then
      	token=overrun
	fi
	if [ "$1" = "ifstat" ]; then
      	token=packet
	fi
	if [ "$1" = "ifstat-rx" ]; then
      	token="RX.*packet"
	fi
	if [ "$1" = "ifstat-tx" ]; then
      	token="TX.*packet"
	fi
	if [ "$1" = "pmon" ]; then
      	token="pmon"
	fi
	if [ "$1" = "pmon_e1" ]; then
      	token="pmon_e1"
	fi
	if [ "$1" = "record" ]; then
      	rec=1
	fi
	shift
done

DEVS=$(cat /proc/net/dev | egrep "w.*g" | cut -d':' -f1 | xargs) 

#echo "$DEVS"

if [ $rec = 1 ]; then
echo $(date) >> stats.out
fi
for dev in $DEVS
do
	if [ "$CMD" = "clear" ]; then
		wanpipemon -i $dev -c fc
		wanpipemon -i $dev -c fpm
	fi	

	if [ "$token" = "pmon" ]; then
		wanpipemon -i $dev -c Ta > t.$$ 
		line=`cat t.$$ | grep -e Line |  cut -d':' -f2 | awk ' { print $1 }'` 
		bit=`cat t.$$ | grep -e Bit |  cut -d':' -f2 | awk ' { print $1 }'` 
		out=`cat t.$$ | grep -e Out |  cut -d':' -f2 | awk ' { print $1 }'` 
		sync=`cat t.$$ | grep -e Sync |  cut -d':' -f2 | awk ' { print $1 }'` 
		r_over=`ifconfig $dev | grep RX.*overruns | awk '{ print $5 }' | cut -d':' -f2`
		t_over=`ifconfig $dev | grep TX.*overruns | awk '{ print $5 }' | cut -d':' -f2`
		con_status=`ifconfig $dev | grep -c RUNNING`
		level=`cat t.$$ | grep "Rx Level" | cut -d':' -f2 | awk '{print $1}'`
		if [ $level = ">" ]; then
			level=`cat t.$$ | grep "Rx Level" | cut -d':' -f2 | awk '{print $2}'`
		fi
		rm t.$$
		echo -e "IF $dev | Pmon: Line=$line Bit=$bit Out=$out Sync=$sync Lvl=$level | Overrun: R=$r_over T=$t_over | Status=$con_status"
		continue
	fi
	
	if [ "$token" = "pmon_e1" ]; then
		wanpipemon -i $dev -c Ta > t.$$ 
		line=`cat t.$$ | grep -e Line |  cut -d':' -f2 | awk ' { print $1 }'` 
		far=`cat t.$$ | grep -e "Far End" |  cut -d':' -f2 | awk ' { print $1 }'` 
		crc4=`cat t.$$ | grep -e CRC4 |  cut -d':' -f2 | awk ' { print $1 }'` 
		fas=`cat t.$$ | grep -e FAS |  cut -d':' -f2 | awk ' { print $1 }'` 
		sync=`cat t.$$ | grep -e Sync |  cut -d':' -f2 | awk ' { print $1 }'` 
		r_over=`ifconfig $dev | grep RX.*overruns | awk '{ print $5 }' | cut -d':' -f2`
		t_over=`ifconfig $dev | grep TX.*overruns | awk '{ print $5 }' | cut -d':' -f2`
		con_status=`ifconfig $dev | grep -c RUNNING`
		level=`cat t.$$ | grep "Rx Level" | cut -d':' -f2 | awk '{print $1}'`
		if [ $level = ">" ]; then
			level=`cat t.$$ | grep "Rx Level" | cut -d':' -f2 | awk '{print $2}'`
		fi
		rm t.$$
		echo -e "IF $dev | Pmon: Line=$line Far=$far CRC4=$crc4 FAS=$fas Sync=$sync Lvl=$level | Overrun: R=$r_over T=$t_over | Status=$con_status"
		continue
	fi

	if [ "$token" = "overrun" ]; then
		line=`wanpipemon -i $dev -c sc | grep "$token"`
	else 
		line=`ifconfig $dev | grep "$token"`
	fi

	if [ $rec = 1 ] ; then
		echo "IF => $dev\n$line" | tee -a stats.out
	else
		echo -e "IF => $dev\n$line" 
	fi
done

