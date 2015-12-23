/*****************************************************************************
* aft_bri.h	WANPIPE(tm) BRI Hardware Support
* 		
* Authors: 	David Rokhvarg <davidr@sangoma.com>
*
* Copyright:	(c) 1984-2007 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* March 15, 2007	David Rokhvarg	Initial version.
*****************************************************************************/

#ifndef _AFT_BRI_H
#define _AFT_BRI_H

#ifdef WAN_KERNEL

#if 0
#define A200_SECURITY_16_ECCHAN	0x00
#define A200_SECURITY_32_ECCHAN	0x01
#define A200_SECURITY_0_ECCHAN	0x05
#define A200_ECCHAN(val)				\
	((val) == A200_SECURITY_16_ECCHAN) ? 16 :	\
	((val) == A200_SECURITY_32_ECCHAN) ? 32 : 0
#endif

int aft_bri_global_chip_config(sdla_t *card);
int aft_bri_global_chip_unconfig(sdla_t *card);
int aft_bri_chip_config(sdla_t *card, wandev_conf_t*);
int aft_bri_chip_unconfig(sdla_t *card);
int aft_bri_chan_dev_config(sdla_t *card, void *chan);
int aft_bri_chan_dev_unconfig(sdla_t *card, void *chan);
int aft_bri_led_ctrl(sdla_t *card, int color, int led_pos, int on);
int aft_bri_test_sync(sdla_t *card, int tx_only);
int bri_check_ec_security(sdla_t *card);

int __aft_bri_write_fe (void* card, ...);
int aft_bri_write_fe (void* card, ...);
unsigned char __aft_bri_read_fe (void* card, ...);
unsigned char aft_bri_read_fe (void* card, ...);

int aft_bri_write_cpld(sdla_t *card, unsigned short off,unsigned short data);
unsigned char aft_bri_read_cpld(sdla_t *card, unsigned short cpld_off);

void aft_bri_fifo_adjust(sdla_t *card,u32 level);

int aft_bri_dchan_transmit(sdla_t *card, void *chan_ptr, void *src_data_buffer, unsigned int buffer_len);
int aft_bri_dchan_receive( sdla_t *card, void *chan_ptr, void *dst_data_buffer, unsigned int buffer_len);


static __inline int
aft_is_bri_nt_card(sdla_t *card)
{
	return IS_BRI_NT_MOD(&(card)->fe.bri_param, WAN_FE_LINENO(&card->fe));
}

static __inline int
aft_is_bri_te_card(sdla_t *card)
{
	return IS_BRI_TE_MOD(&(card)->fe.bri_param, WAN_FE_LINENO(&card->fe));
}

static __inline int
aft_is_bri_512khz_card(sdla_t *card)
{
	return card->fe.bri_param.use_512khz_recovery_clock;	
}

#define IS_A700_CARD(card) (card->adptr_type == AFT_ADPTR_FLEXBRI) 

#endif/* WAN_KERNEL */

#endif/* _AFT_BRI_H */
