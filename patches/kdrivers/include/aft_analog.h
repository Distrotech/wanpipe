/*****************************************************************************
* aft_analog.h	WANPIPE(tm) S51XX Xilinx Hardware Support
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


#ifndef __AFT_A104_ANALOG_H_
#define __AFT_A104_ANALOG_H_

#ifdef WAN_KERNEL

int aft_analog_global_chip_config(sdla_t *card);
int aft_analog_global_chip_unconfig(sdla_t *card);
int aft_analog_chip_config(sdla_t *card, wandev_conf_t *);
int aft_analog_chip_unconfig(sdla_t *card);
int aft_analog_chan_dev_config(sdla_t *card, void *chan);
int aft_analog_chan_dev_unconfig(sdla_t *card, void *chan);
int aft_analog_led_ctrl(sdla_t *card, int color, int led_pos, int on);
int aft_analog_test_sync(sdla_t *card, int tx_only);
int a200_check_ec_security(sdla_t *card);

int __aft_analog_write_fe (void* card, ...);
int aft_analog_write_fe (void* card, ...);
unsigned char __aft_analog_read_fe (void* card, ...);
unsigned char aft_analog_read_fe (void* card, ...);

int aft_analog_write_cpld(sdla_t *card, unsigned short off,u_int16_t data);
unsigned char aft_analog_read_cpld(sdla_t *card, unsigned short cpld_off);

void aft_analog_fifo_adjust(sdla_t *card,u32 level);

#define IS_A700_ANALOG_CARD(card) (card->adptr_type == AFT_ADPTR_FLEXBRI && card->comm_port == 1)

#endif

#endif
