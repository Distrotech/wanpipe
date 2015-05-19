#!/bin/bash

PROD="ss7boxd_monitor"
SIG="ss7boxd"
LOGFILE=/var/log/messages
LOGROTATE=/etc/logrotate.conf

lcnt=0;

resource_current_cnt=0;
resource_overall_err=0;
resource_threshold=3;

hb_current_cnt=0;
hb_overall_err=0;
hb_threshold=10;
hb_reset=0;


logit()
{
    local data="$1"

    echo "$PROD: $data"
    logger "$PROD: $data"
}


logrotate_files()
{
	if [ -f $LOGROTATE ]; then
	   eval "logrotate -f $LOGROTATE"
	else 
		cp -f $LOGROTATE $LOGROTATE.$((lcnt+1))
	fi

	echo " " > $LOGFILE
}

restart_smg_ctrl()
{

	logrotate_files 

   	logit "$1"
    eval "smg_ctrl stop"
    eval "smg_ctrl start"
	logit "restart_smg_ctrl done"

}

check_for_isupd_errors()
{
	err=`grep "sangoma_isupd.*Resource temporarily unavailable" $LOGFILE -c`
	if [ $? -eq 0 ]; then
		if  [ $err != 0 ] && [ $err != $resource_current_cnt ]; then
				
				resource_overall_err=$((resource_overall_err+1))
				logit "Resource error occoured cnt=$resource_overall_err/$resource_threshold!";
				
				if [ $resource_overall_err -gt $resource_threshold ]; then
					resource_overall_err=0;
					restart_smg_ctrl "Restarting smg_ctrl due to: sangoma_isupd Resource errors"
				fi
		fi
		resource_current_cnt=$err;
	fi
		
	err=`grep  "heartbeat" $LOGFILE -c`
	if [ $? -eq 0 ]; then
		if  [ $err != 0 ] && [ $err != $hb_current_cnt ]; then
				
				hb_overall_err=$((hb_overall_err+1))
				logit "Heartbeat error occoured cnt=$hb_overall_err/$hb_threshold!";
				
				if [ $hb_overall_err -gt $hb_threshold ]; then
					hb_overall_err=0;
					restart_smg_ctrl "Restarting smg_ctrl due to: HeartBeat errors"
				fi
				hb_reset=0
		else 
			hb_reset=$((hb_reset+1))
			if [ $hb_overall_err -gt 0 ] && [ $hb_reset -gt $((hb_threshold*2)) ]; then
				echo "$PROD: Resetting hb overall cnt"
				hb_overall_err=0;
				hb_reset=0;
			fi
		fi
		hb_current_cnt=$err
	fi
}


eval 'pidof $SIG' 2> /dev/null > /dev/null
if  [ $? -ne 0 ]; then
	logit "Error: $SIG not started"
	exit 1
fi
	
logit "Service Started"


while [ 1 ];
do

	eval 'pidof $SIG' 2> /dev/null > /dev/null
	if  [ $? -ne 0 ]; then
		logit "$SIG not running..."	
		exit 1
	fi

	check_for_isupd_errors
	
	#logit "$SIG running..."	
	sleep 1

done
