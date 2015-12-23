/*****************************************************************************
* aft_core_prot.h
* 		
* 		WANPIPE(tm) AFT CORE Hardware Support - Protocol/API
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================*/

#ifndef __AFT_CORE_XMTP2__
#define __AFT_CORE_XMTP2__

#include "aft_core_private.h"
#include "wanpipe_timer_iface.h"

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
# include "wanpipe_lapb_kernel.h"
#endif

#if defined(AFT_XMTP2_API_SUPPORT)
#include "xmtp2km_kiface.h"
int wp_xmtp2_callback (void *prot_ptr, unsigned char *data, int len);
#endif

int 	protocol_init (sdla_t*card,netdevice_t *dev,
		               private_area_t *chan, wanif_conf_t* conf);
int 	protocol_stop (sdla_t *card, netdevice_t *dev);
int 	protocol_shutdown (sdla_t *card, netdevice_t *dev);
void 	protocol_recv(sdla_t *card, private_area_t *chan, netskb_t *skb);


#if defined(AFT_RTP_SUPPORT)
int 	aft_rtp_config(sdla_t *card);
void 	aft_rtp_unconfig(sdla_t *card);
void 	aft_rtp_tap(void *card_ptr, u8 chan, u8* rx, u8* tx, u32 len);
#endif


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev);
netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t* annexg_dev_name);
int get_map(wan_device_t*, netdevice_t*, struct seq_file* m, int*);
void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats);
#endif

int aft_tdm_api_init(sdla_t *card, private_area_t *chan, wanif_conf_t *conf);
int aft_tdm_api_free(sdla_t *card, private_area_t *chan);

int aft_sw_hdlc_rx_data (void *priv_ptr, u8 *rx_data, int rx_len, uint32_t err_code);
int aft_sw_hdlc_rx_suerm (void *priv_ptr);
int aft_sw_hdlc_wakup (void *priv_ptr);
int aft_sw_hdlc_trace(void *priv_ptr, u8 *data, int len, int dir);

#endif

