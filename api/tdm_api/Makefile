# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif

# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -I$(WANINCDIR) -DWANPIPE_TDM_API -D_DEBUG_=$(DEBUG) -D_GNUC_ -I../lib/hdlc -I../lib 

TARGETS = aft_tdm_voice_api
TARGETS += aft_tdm_voice_api_switch
#TARGETS += aft_tdm_ctrl_api
#TARGETS += aft_tdm_ctrl_api_nolib
#TARGETS += aft_tdm_voice_api_softloop
TARGETS += aft_tdm_hdlc_test
#TARGETS += aft_tdm_hdlc_test_zap
TARGETS += aft_tdm_voice_api_rbs
#TARGETS += aft_tdm_voice_api_dtmf
TARGETS += aft_tdm_voice_api_rm_events
#TARGETS += aft_tdm_voice_api_rm_dtmf
#TARGETS += aft_tdm_voice_api_rxhook

####### RULES ################################################################

all:	$(TARGETS)  
	@echo "Ok."

aft_tdm_voice_api: aft_tdm_voice_api.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_ctrl_api: aft_tdm_ctrl_api.c  
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_ctrl_api_nolib: aft_tdm_ctrl_api_nolib.c 
	$(CC) $(CFLAGS) -o $@ $^ 

aft_tdm_voice_api_switch: aft_tdm_voice_api_switch.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_voice_api_softloop: aft_tdm_voice_api_softloop.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_hdlc_test:	aft_tdm_hdlc_test.c ../lib/hdlc/wanpipe_hdlc.c
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm  

aft_tdm_hdlc_test_zap:	aft_tdm_hdlc_test_zap.c ../lib/hdlc/wanpipe_hdlc.c
	$(CC) $(CFLAGS) -o $@ $^ -lm  

aft_tdm_voice_api_rbs: aft_tdm_voice_api_rbs.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_voice_api_dtmf: aft_tdm_voice_api_dtmf.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_voice_api_rm_events: aft_tdm_voice_api_rm_events.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_voice_api_rm_dtmf: aft_tdm_voice_api_rm_dtmf.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

aft_tdm_voice_api_rxhook: aft_tdm_voice_api_rxhook.c ../lib/lib_api.c 
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma -lm

clean:
	rm -f $(TARGETS)
