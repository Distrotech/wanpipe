/**************************************************************************
 * wanpipe_codec.h	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *			TDM voice board configuration.
 *
 * Author: 	Nenad Corbic  <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 1995-2005 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 ******************************************************************************
 */
/*
 ******************************************************************************
			   INCLUDE FILES
 ******************************************************************************
*/

#ifndef __WANPIPE_CODEC_H_
#define __WANPIPE_CODEC_H_


#include "wanpipe_codec_iface.h"


#ifdef CONFIG_PRODUCT_WANPIPE_CODEC_SLINEAR_LAW

/*! The A-law alternate mark inversion mask */
#define G711_ALAW_AMI_MASK      0x55
/*! Bias for u-law encoding from linear. */
#define G711_ULAW_BIAS      0x84

extern int wanpipe_codec_law_init(void);

extern int wanpipe_codec_convert_s_2_alaw(u16 *data,
				   int len, 
			     	   u8  *buf,
				   u8  *gain,
				   u8  user);

extern int wanpipe_codec_convert_s_2_ulaw(u16 *data,
				   int len, 
			     	   u8  *buf,
				   u8  *gain,
				   u8 user);

extern int wanpipe_codec_convert_alaw_2_s(u8 *data, 
				   int len,
			           u16 *buf, 
				   u32 *power_ptr,
				   u8  *gain,
				   u8 user);

extern int wanpipe_codec_convert_ulaw_2_s(u8 *data, 
				       int len,
			               u16 *buf, 
				       u32 *power_ptr,
				       u8  *gain,
				       u8 user);

extern int wanpipe_codec_get_mtu_law_2_s(u32 mtu);
extern int wanpipe_codec_get_mtu_s_2_law(u32 mtu);

extern int wanpipe_codec_get_alaw_to_linear(u8 alaw);
extern int wanpipe_codec_get_ulaw_to_linear(u8 ulaw);

#endif /*CONFIG_PRODUCT_WANPIPE_CODEC_SLINEAR_LAW*/


#endif /*__WANPIPE_CODEC_H_*/
