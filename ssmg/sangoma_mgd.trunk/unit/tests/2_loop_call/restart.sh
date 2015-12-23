#!/bin/sh

ulimit -n 65000
ulimit -c unlimited

./smg_unit -term
sangoma_mgd -term
sleep 1

nice -n -5 ./smg_unit -bg
sleep 1
sangoma_mgd -bg
