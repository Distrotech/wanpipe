/*****************************************************************************
* aft_gsm.c 
* 		
* 		WANPIPE(tm) AFT W400 Hardware Support
*
* Authors: 	Moises Silva <moy@sangoma.com>
*
* Copyright:	(c) 2011 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Oct 06, 2011  Moises Silva Initial Version
*****************************************************************************/


#ifndef __AFT_GSM_H_
#define __AFT_GSM_H_

#ifdef WAN_KERNEL

int aft_gsm_global_chip_config(sdla_t *card);
int aft_gsm_global_chip_unconfig(sdla_t *card);
int aft_gsm_chip_config(sdla_t *card, wandev_conf_t *);
int aft_gsm_chip_unconfig(sdla_t *card);
int aft_gsm_chan_dev_config(sdla_t *card, void *chan);
int aft_gsm_chan_dev_unconfig(sdla_t *card, void *chan);
int aft_gsm_led_ctrl(sdla_t *card, int color, int led_pos, int on);
int aft_gsm_test_sync(sdla_t *card, int tx_only);
unsigned char aft_gsm_read_cpld(sdla_t *card, unsigned short cpld_off);
int aft_gsm_write_cpld(sdla_t *card, unsigned short off,u_int16_t data);
void aft_gsm_fifo_adjust(sdla_t *card,u32 level);
int w400_check_ec_security(sdla_t *card);

#endif

#endif

