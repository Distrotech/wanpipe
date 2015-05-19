#!/bin/sh

CMD=$1
OSYSTEM=`uname -s`
RELEASE=`uname -r`
if [ $OSYSTEM = "Linux" ]; then
	MODULE_STAT=lsmod
	WAN_DRIVERS="wan_aften"
	MODULE_LOAD=modprobe
	MODULE_UNLOAD="modprobe -r"
	WAN_AFTEN_DRV="wan_aften"
elif [ $OSYSTEM = "FreeBSD" ]; then

	major_ver=${RELEASE%%.*}
	MODULE_STAT=kldstat
	MODULE_LOAD=kldload
	MODULE_UNLOAD=kldunload
	if [ "$major_ver" = "5" ]; then
		MODULE_DIR=/boot/modules
	else
		MODULE_DIR=/modules
	fi
	WAN_AFTEN_DRV="wan_aften"
elif [ $OSYSTEM = "OpenBSD" ]; then

	MODULE_STAT=modstat
	MODULE_LOAD=modload
	MODULE_UNLOAD=modunload
	MODULE_DIR=/usr/lkm
	WAN_AFTEN_DRV="wan_aften"
elif [ $OSYSTEM = "NetBSD" ]; then

	MODULE_STAT=modstat
	MODULE_LOAD=modload
	MODULE_UNLOAD=modunload
	MODULE_DIR=/usr/lkm
	WAN_AFTEN_DRV="wan_aften"
fi

#ifconfig whdlc0 down
if [ $OSYSTEM = "OpenBSD" -o $OSYSTEM = "NetBSD" ]; then
	\rm -rf $MODULE_DIR/${WAN_AFTEN_DRV}.out
	err=`$MODULE_UNLOAD -n ${WAN_AFTEN_DRV} 2> /dev/null`
else
	echo "$MODULE_UNLOAD ${WAN_AFTEN_DRV}"
	err=`$MODULE_UNLOAD ${WAN_AFTEN_DRV}  2> /dev/null` 
fi
echo "AFT card disabled"
exit $err
