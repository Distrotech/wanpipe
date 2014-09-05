/**************************************************************************
 * wanpipe_codec_iface.h
 * 		WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *		TDM voice board configuration.
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


#ifndef __WANPIPE_CODEC_IFACE_
#define __WANPIPE_CODEC_IFACE_

#define WP_TDM_HW_CODING_MAX 	4
#define WP_TDM_CODEC_MAX 	10

enum wan_codec_source_format{
	WP_MULAW,
	WP_ALAW,
	WP_LIN16
};

#define WP_CODEC_FORMAT_DECODE(codec)	\
codec == WP_MULAW	? "WP_MULAW" :	\
codec == WP_ALAW	? "WP_ALAW"  :	\
codec == WP_LIN16	? "WP_LIN16" :	\
"Invalid Codec"

#if 0
enum wan_codec_format{
	WP_NONE,
	WP_SLINEAR
};
#endif

#ifdef WAN_KERNEL

typedef struct {
	int init;
	int (*encode)(u8 *data, 
		      int len,
		      u16 *buf, 
	              u32 *power_ptr,
		      u8* gain,
		      u8 user);
		      
	int (*decode)(u16 *data,
		      int len, 
		      u8  *buf, 
		      u8 *gain,
		      u8 user);
}wanpipe_codec_ops_t;



static __inline int wanpipe_codec_calc_new_mtu(u32 codec, u32 mtu)
{
	if (codec >= WP_TDM_CODEC_MAX){
		DEBUG_EVENT("%s:%d Critical: Invalid Codec \n",
			__FUNCTION__,__LINE__);
		return mtu;
	}

	switch (codec){

	case WP_SLINEAR:
		mtu*=2;
		break;

	default:
		/* If no codec is specified 
                 * there is no MTU multiple */
		break;
	}

	return mtu;
}


extern int wanpipe_codec_init(void);
extern int wanpipe_codec_free(void);

extern wanpipe_codec_ops_t *WANPIPE_CODEC_OPS[WP_TDM_HW_CODING_MAX][WP_TDM_CODEC_MAX];

extern int wanpipe_codec_convert_to_linear(u8 pcm_value,u8 codecType);

#endif

#endif

