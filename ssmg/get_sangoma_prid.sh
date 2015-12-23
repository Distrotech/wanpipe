#!/bin/bash -p 
MY_ARCH="i386"
UPDATE="NO"
ARCH=`uname -m`
if [ $ARCH = "x86_64" ]; then
	MY_ARCH="x86_64"
fi

if [ $1 ] && [ "$1" == "--update" ]; then
  UPDATE="YES"
fi

#version format: wanpipe-<ROUTER_VERSION_MAIN>.<ROUTER_VERSION_MAJOR>.<ROUTER_VERSION_MINOR>.<ROUTER_VERSION_SUB>

ROUTER_VERSION_MAIN="`cat ../.router_version | cut -d "-" -f2 | cut -d "." -f1`"
ROUTER_VERSION_MAJOR="`cat ../.router_version | cut -d "-" -f2 | cut -d "." -f2`"
ROUTER_VERSION_MINOR="`cat ../.router_version | cut -d "-" -f2 | cut -d "." -f3`"
ROUTER_VERSION_SUB="`cat ../.router_version | cut -d "-" -f2 | cut -d "." -f4`"

ROUTER_VERSION=$ROUTER_VERSION_MAIN.$ROUTER_VERSION_MAJOR.$ROUTER_VERSION_MINOR.$ROUTER_VERSION_SUB
ROUTER_VERSION_MAIN_RELEASE=$ROUTER_VERSION_MAIN.$ROUTER_VERSION_MAJOR.$ROUTER_VERSION_MINOR

if [ "$ROUTER_VERSION" = "" ]; then
	echo "Failed to obtain wanrouter version"
	exit 1
fi


PRI_PACKAGE_FORMAT="sangoma_prid-*-$ROUTER_VERSION$MY_ARCH.tgz"
PRI_PACKAGE_NAME="`find -name \"$PRI_PACKAGE_FORMAT\" | cut -d "/" -f2`"

if [ "$PRI_PACKAGE_NAME" != "" ] && [ "$UPDATE" != "YES" ]; then
	echo "Using $PRI_PACKAGE_NAME"
	echo " "
else
  #if a pri package was already downloaded
  if [ "$PRI_PACKAGE_NAME" != "" ]; then
    echo "Renaming $PRI_PACKAGE_NAME to $PRI_PACKAGE_NAME.old"
    eval "mv -f $PRI_PACKAGE_NAME $PRI_PACKAGE_NAME.old"
    if [ $? -ne 0 ]; then
      echo "Failed to rename existing sangoma_prid package"
      exit 1
    fi
  fi
	echo "Fetching PRI package from Sangoma FTP"
	eval "wget ftp://ftp.sangoma.com/linux/sangoma_prid/wanpipe-$ROUTER_VERSION/sangoma_prid-*-$ROUTER_VERSION.$MY_ARCH.tgz"
	if [ $? -ne 0 ]; then
		echo "Looking for $ROUTER_VERSION_MAIN_RELEASE"
		eval "wget ftp://ftp.sangoma.com/linux/sangoma_prid/wanpipe-$ROUTER_VERSION_MAIN_RELEASE/sangoma_prid-*-$ROUTER_VERSION_MAIN_RELEASE.$MY_ARCH.tgz"
		PRI_PACKAGE_FORMAT="sangoma_prid-*-$ROUTER_VERSION_MAIN_RELEASE.$MY_ARCH.tgz"
		if [ $? -ne 0 ]; then
			echo "Failed to obtain PRI package for this release"
			if [ "$PRI_PACKAGE_NAME" != "" ]; then
				echo "Using old sangoma_pri package $PRI_PACKAGE_NAME"
				eval "mv -f $PRI_PACKAGE_NAME.old $PRI_PACKAGE_NAME"
			else
				exit 1
			fi
		fi
	fi
	PRI_PACKAGE_NAME="`find -name \"$PRI_PACKAGE_FORMAT\" | cut -d "/" -f2`"
fi
echo "Extracting $PRI_PACKAGE_NAME"
echo " "
eval "tar xfz $PRI_PACKAGE_NAME"
if [ $? -ne 0 ]; then
	echo "Error: Invalid/Corrupt PRI package tarball"
	exit 1
fi
