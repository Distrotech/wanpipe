/*****************************************************************************
* aft_a104.h	WANPIPE(tm) S51XX Xilinx Hardware Support
* 		
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Oct 18, 2005	Nenad Corbic	Initial version.
*****************************************************************************/


#ifndef __AFT_A104_H_
#define __AFT_A104_H_

#ifdef WAN_KERNEL


int a104_global_chip_config(sdla_t *card);
int a104_global_chip_unconfig(sdla_t *card);
int a104_chip_config(sdla_t *card, wandev_conf_t*);
int a104_chip_unconfig(sdla_t *card);
int a104_chan_dev_config(sdla_t *card, void *chan);
int a104_chan_dev_unconfig(sdla_t *card, void *chan);
int a104_led_ctrl(sdla_t *card, int color, int led_pos, int on);
int a104_test_sync(sdla_t *card, int tx_only);
int a104_check_ec_security(sdla_t *card);
int a104_set_digital_fe_clock(sdla_t *card);


//int a104_write_fe (sdla_t* card, unsigned short off, unsigned char value);
//unsigned char a104_read_fe (sdla_t* card, unsigned short off);
int a104_write_fe (void *pcard, ...);
unsigned char __a104_read_fe (void *pcard, ...);
unsigned char a104_read_fe (void *pcard, ...);

int a56k_write_fe (void *pcard, ...);
unsigned char __a56k_read_fe (void *pcard, ...);
unsigned char a56k_read_fe (void *pcard, ...);

int aft_te1_write_cpld(sdla_t *card, unsigned short off, u_int16_t data);
unsigned char aft_te1_read_cpld(sdla_t *card, unsigned short cpld_off);

int aft_56k_write_cpld(sdla_t *card, unsigned short off, u_int16_t data);
unsigned char aft_56k_read_cpld(sdla_t *card, unsigned short cpld_off);

int a108m_write_cpld(sdla_t *card, unsigned short off, u_int16_t data);
unsigned char a108m_read_cpld(sdla_t *card, unsigned short cpld_off);

void a104_fifo_adjust(sdla_t *card,u32 level);

#endif

#endif
