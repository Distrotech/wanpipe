/*****************************************************************************
* wanpipe_lip_atm_iface.h	WANPIPE(tm) ATM ALL5 SAR Interface Header
* 		
* Authors: 	David Rokhvarg <davidr@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* September 15, 2005	David Rokhvarg	Initial Version
*****************************************************************************/


#ifndef _WANPIPE_LIP_ATM_IFACE_H
#define _WANPIPE_LIP_ATM_IFACE_H

#define wplist_insert_dev(dev, list)	do{\
	                                   dev->next = list;\
					   list = dev;\
                                        }while(0)

#define wplist_remove_dev(dev,cur,prev,list)\
					do{\
	                                   if ((cur=list) == dev){\
						list=cur->next;\
						break;\
					   }else{\
						while (cur!=NULL && cur->next!=NULL){\
							prev=cur;\
							cur=cur->next;\
							if (cur==dev){\
								prev->next=cur->next;\
								break;\
							}\
						}\
					   }\
                                        }while(0)

extern void *wp_register_atm_prot(void *link_ptr, 
				 char *devname, 
				 void *cfg, 
				 wplip_prot_reg_t *atm_reg);

extern int wp_unregister_atm_prot(void *prot_ptr);

extern void *wp_register_atm_chan(void *if_ptr, 
		                 void *prot_ptr, 
				 char *devname,
				 void *cfg,
				 unsigned char type);

extern int wp_unregister_atm_chan(void *chan_ptr);

extern int wp_atm_open_chan (void *chan_ptr);
extern int wp_atm_close_chan(void *chan_ptr);
extern int wp_atm_ioctl     (void *chan_ptr, int cmd, void *arg);

extern int wp_atm_rx     (void * prot_ptr, void *rx_pkt);
extern int wp_atm_timer  (void *prot_ptr, unsigned int *period, unsigned int carrier_reliable);
extern int wp_atm_tx 	(void * chan_ptr, void *skb, int type);
extern int wp_atm_pipemon(void *chan, int cmd, int dlci, unsigned char* data, unsigned int *len);
extern int wp_atm_snmp(void* chan_ptr, void* data);

#endif
