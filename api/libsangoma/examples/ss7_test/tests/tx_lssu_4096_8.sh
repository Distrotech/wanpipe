#!/bin/sh

#./tdm_hdlc_test.sh "1" 1 0 "-verbose 3 -tx_delay 100000 -ss7_mode_4096 -ss7_lssu_sz 8 -tx_lssu"
./tdm_hdlc_test.sh "1 2 3 4" all 0 "-ss7_mode_4096 -ss7_lssu_sz 8 -tx_lssu"

