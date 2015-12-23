/*****************************************************************************
* sdla_atm_iface.c	WANPIPE(tm) ATM ALL5 SAR Interface Header
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
* Jan 07, 2003	Nenad Corbic	Based on ADSL ATM SAR
*****************************************************************************/


#ifndef _SDLA_ATM_IFACE_H
#define _SDLA_ATM_IFACE_H

#include "wanpipe_cfg.h"

extern int  wp_sar_register_device  (void *card, 
		                     unsigned char *devname,
		                     void **atm_device_ptr,
				     void *rx_data_list,
				     void *tx_data_list,
				     void *trace_info);

extern void wp_sar_unregister_device(void *atm_device);

extern int  wp_sar_register_pvc(void *atm_device, 
				void **pAdapter_ptr, 
				void **tx_cells_skb,
				void *chan,
				void *devname,
				void *dev,
				wan_atm_conf_if_t *atm_cfg,
				unsigned int mtu);

extern void wp_sar_unregister_pvc(void *atm_device, void *pAdapter, void *tx_skb);
extern int wanpipe_sar_rx (void *atm_device, void *rx_cell_block);
extern int wanpipe_sar_tx (void *pAdapter, void* tx_skb, void **tx_cell_skb);

extern int wanpipe_sar_poll(void *pAdapter, int timeout);

extern int wanpipe_get_atm_state (void *pAdapter);
extern int wanpipe_set_atm_state (void *pAdapter, int state);


#endif
