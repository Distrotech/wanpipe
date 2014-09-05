/*****************************************************************************
* aft_t116.h	WANPIPE(tm) S51XX Xilinx Hardware Support
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


#ifndef __AFT_T116_H_
#define __AFT_T116_H_

#ifdef WAN_KERNEL


int t116_global_chip_config(sdla_t *card);
int t116_global_chip_unconfig(sdla_t *card);
int t116_chip_config(sdla_t *card, wandev_conf_t*);
int t116_chip_unconfig(sdla_t *card);
int t116_chan_dev_config(sdla_t *card, void *chan);
int t116_chan_dev_unconfig(sdla_t *card, void *chan);
int t116_led_ctrl(sdla_t *card, int color, int led_pos, int on);
int t116_test_sync(sdla_t *card, int tx_only);
int t116_check_ec_security(sdla_t *card);


//int a104_write_fe (sdla_t* card, unsigned short off, unsigned char value);
//unsigned char a104_read_fe (sdla_t* card, unsigned short off);
int t116_write_fe (void *pcard, ...);
unsigned char __t116_read_fe (void *pcard, ...);
unsigned char t116_read_fe (void *pcard, ...);

int a56k_write_fe (void *pcard, ...);
unsigned char __a56k_read_fe (void *pcard, ...);
unsigned char a56k_read_fe (void *pcard, ...);

int aft_te1_write_cpld(sdla_t *card, unsigned short off, u_int16_t data);
unsigned char aft_te1_read_cpld(sdla_t *card, unsigned short cpld_off);

int aft_56k_write_cpld(sdla_t *card, unsigned short off, u_int16_t data);
unsigned char aft_56k_read_cpld(sdla_t *card, unsigned short cpld_off);

int a108m_write_cpld(sdla_t *card, unsigned short off, u_int16_t data);
unsigned char a108m_read_cpld(sdla_t *card, unsigned short cpld_off);

void t116_fifo_adjust(sdla_t *card,u32 level);

#define SDLA_HW_T116_FE_ACCESS_BLOCK { u32 breg=0; card->hw_iface.bus_read_4(card->hw, 0x40, &breg); \
								  if (breg == (u32)-1) { \
									if (WAN_NET_RATELIMIT()) { \
										DEBUG_ERROR("%s:%d: wanpipe PCI Error: Illegal Register read: 0x40 = 0xFFFFFFFF\n", \
											__FUNCTION__,__LINE__);  \
									} \
								 } \
}

/* Helper function for T116 TAP card */
static __inline int read_reg_ds26519_fpga (sdla_t* card, u32 addr){
	int data= 0;
	/* Clearing Bits 31-14 */
	addr = addr & 0X3FFF; 
	addr = addr | 0x8000;
	SDLA_HW_T116_FE_ACCESS_BLOCK;
	card->hw_iface.bus_write_2(card->hw,0x44,addr);
	SDLA_HW_T116_FE_ACCESS_BLOCK;
	card->hw_iface.bus_read_4(card->hw,0x44,&data);
	SDLA_HW_T116_FE_ACCESS_BLOCK;
	data = (data >> 16)& 0xFF;
	return data;
}

static __inline int write_reg_ds26519_fpga (sdla_t* card, u32 addr, u32 data){
	u32 shifted_data = 0;
	/* Clearing Bits 31-14 */	
	addr = addr & 0X3FFF; 
	addr = addr | 0x8000;
	SDLA_HW_T116_FE_ACCESS_BLOCK;
	card->hw_iface.bus_write_2(card->hw,0x44,addr);
	SDLA_HW_T116_FE_ACCESS_BLOCK;
	shifted_data = (data << 16) & 0x00FF0000;
	card->hw_iface.bus_write_4(card->hw,0x44,shifted_data);
	SDLA_HW_T116_FE_ACCESS_BLOCK;
	return 0;
}

#endif

#endif
