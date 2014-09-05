#!/bin/sh

kill -SIGUSR1 $(pidof xdlc_api)
echo "Tx Kicked"
