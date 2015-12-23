#!/bin/sh

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

if [ $OSYSTEM = "Linux" ]; then
	echo "$MODULE_LOAD $WAN_AFTEN_DRV  > /dev/null"
	$MODULE_LOAD $WAN_AFTEN_DRV  > /dev/null 
	err=$?
elif [ $OSYSTEM = "FreeBSD" ]; then
	err=`$MODULE_LOAD ${WAN_AFTEN_DRV} >/dev/null`
elif [ $OSYSTEM = "OpenBSD" -o $OSYSTEM = "NetBSD" ]; then
	err=`$MODULE_LOAD -o $MODULE_DIR/${WAN_AFTEN_DRV}1.out -e${WAN_AFTEN_DRV} $MODULE_DIR/${WAN_AFTEN_DRV}.o  1> /dev/null`
fi
echo "AFT card enabled"
#ifconfig whdlc0 up
exit $err
