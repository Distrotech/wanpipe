#!/bin/sh

home=`pwd`;

ss7boost="ss7boost"
ss7box="ss7boxd"

NICE=
RENICE=

if [ ! -e /usr/local/ss7box/$ss7boost ]; then
	echo "Error: ss7boost not found in /usr/local/ss7box dir";
	exit 1
fi
if [ ! -e /usr/local/ss7box/$ss7boxd ]; then
	echo "Error: ss7boxd not found in /usr/local/ss7box dir";
	exit 1
fi

eval "ulimit -n 65000"
eval "modprobe sctp > /dev/null 2> /dev/null"

./smg_ctrl stop
./smg_ctrl start

for((i=0;i<20;i++))
do
	if [ -f /var/run/sangoma_mgd.pid ]; then
		break;
	fi
	echo "Waiting for smg to come up!"
	sleep 2
done

cd $home

exit 0;

