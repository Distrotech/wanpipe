#!/bin/sh

rev=$1


# ----------------------------------------------------------------------------
# Prompt user for input.
# ----------------------------------------------------------------------------
prompt()
{
	if test $NONINTERACTIVE; then
		return 0
	fi

	# BASH echo -ne "$*" >&2
	echo -n "$*" >&2
	read CMD rest
	return 0
}

# ----------------------------------------------------------------------------
# Get Yes/No
# ----------------------------------------------------------------------------
getyn()
{
	if test $NONINTERACTIVE; then
		return 0
	fi

	while prompt "$* (y/n) "
	do	case $CMD in
			[yY])	return 0
				;;
			[nN])	return 1
				;;
			*)	echo -e "\nPlease answer y or n" >&2
				;;
		esac
	done
}  

##########################
#MAIN


tmpdir=/tmp/$dir.$$

if [ -z $rev ]; then
	echo
	echo "Usage $0 <rev number>"
	echo
	exit 1
fi


dir="g3ti_api_libsangoma_v"$1

getyn "Releasing $dir"
if [ $? -ne 0 ]; then
	exit 1
fi


rm -rf $dir
rm -rf $dir.tgz

mkdir $tmpdir

make clean

cp -rf * $tmpdir

mv $tmpdir $dir

cd $dir

find . -name '*.svn' | xargs rm -rf

rm Makefile
ln -s Makefile.Linux Makefile

make clean
make
if [ $? -ne 0 ]; then
  	echo "Failed to compile!"
	exit 1
fi

make clean

cd ..


tar cfz $dir.tgz $dir


echo "Done -> $dir.tgz"

exit 0



