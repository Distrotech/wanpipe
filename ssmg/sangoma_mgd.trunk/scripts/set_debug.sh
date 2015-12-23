#!/bin/bash

LEVEL=${1:-1}

host=127.0.0.1
port=42420

echo open ${host} ${port}
sleep 1
echo "DEBUG $LEVEL"
echo
echo
sleep 1
echo "BYE"
echo
echo

echo "SMG Debug level set to $LEVEL"

echo exit

