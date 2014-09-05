#!/bin/sh

echo " "  > /var/log/messages
echo " "  > /var/log/asterisk/messages
echo " "  > /var/log/woomerang.log
echo " "  > /var/log/sangoma_mgd.log

ss7boost=ss7boost_i_0.2.26


./clog.sh
while [ 1 ]
do
	./channel-testL.pl
	sleep 20
	rm -rf /var/spool/asterisk/outgoing/*

	eval "pidof $ss7boost"
	if [ $? -ne 0 ]; then
		echo "BOOST DIED!!!"
		date
		#exit 0
	fi

	eval "grep ISUP_1 /var/log/messages > /dev/null 2> /dev/null"
        if [ $? -eq 0 ]; then
                echo "ISUP_1 Detected DIED!!!"
                date
                #exit 0
        fi

done
