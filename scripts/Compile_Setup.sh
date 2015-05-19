
#!/bin/bash

if [ -e kdrvcmp ]; then
	rm -rf kdrvcmp
fi

mkdir kdrvcmp
cd kdrvcmp
ln -s . common
ln -s . modinfo
mkdir tmp
mkdir mod

cp -f ../patches/kdrivers/src/net/* .
cp -f ../patches/kdrivers/src/wanrouter/* .
cp -f ../patches/kdrivers/src/lip/* .
cp -f ../patches/kdrivers/src/lip/bin/* .
cp -rf ../patches/kdrivers/include/* /usr/src/linux/include/linux
cp -rf ../patches/kdrivers/include/* /usr/include/linux
cp -f ../samples/Makefile .
cp -f ../Compile.sh .

chmod 755 Compile.sh

cd ..

echo "Compile Environmet Setup"

