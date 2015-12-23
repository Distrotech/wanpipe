#!/bin/sh

home=$(pwd)
arch=`uname -m`

echo "Installing Sangoma BRI Daemon for $arch"
case $arch in 
	i686)
		install -D sangoma_brid.i686 /usr/sbin/sangoma_brid
	;;
	x86_64)
		install -D sangoma_brid.x86_64 /usr/sbin/sangoma_brid
	;;
	*)
	echo "Unsupported architecture $arch"
	exit 1
	;;
esac

echo "Sangoma BRI Install Done"
echo 
echo "Run: wancfg_smg to configure wanpipe BRI"
echo 
