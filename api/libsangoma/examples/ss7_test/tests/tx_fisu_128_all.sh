#!/bin/sh

timeout=${1:-10}

cd ..
./tdm_hdlc_test.sh "1 2 3 4" all $timeout 
