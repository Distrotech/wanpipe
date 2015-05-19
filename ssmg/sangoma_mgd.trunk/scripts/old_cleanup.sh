#!/bin/bash -p

if [ "$UID" != 0 ]; then
	echo "System cleanup only allowed in super user mode!"
	exit 1
fi

eval "find /etc -name 'S*wanrouter' | xargs rm" 2> /dev/null > /dev/null
eval "find /etc -name 'K*wanrouter' | xargs rm" 2> /dev/null > /dev/null

if [ -e /usr/sbin/smgss7_ctrl ]; then 
	rm -f /usr/sbin/smgss7_ctrl 
fi
if [ -e /usr/sbin/smgss7_init_ctrl ]; then 
	rm -f /usr/sbin/smgss7_init_ctrl
fi
if [ -e /etc/init.d/smgss7_ctrl ]; then 
	rm -f /etc/init.d/smgss7_ctrl
fi

exit 0
