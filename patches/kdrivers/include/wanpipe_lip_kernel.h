/*****************************************************************************
* wanpipe_lip_kernel.h	
*
* 		Header file for the Sangoma lip network layer 	
*
* Author: 	Nenad Corbic 	
*
* Copyright:	(c) 2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
* Feb 16, 2004	Nenad Corbic 	Initial Version
*
*****************************************************************************/

#ifndef __WP_LIP_KERNEL_H_
#define __WP_LIP_KERNEL_H

typedef struct wplip_reg 
{
	unsigned long init;
	
	int  (*wplip_register) 		(void**,wanif_conf_t *,char *);
	int  (*wplip_prot_reg)   	(void*, char*, wanif_conf_t *);
	int  (*wplip_prot_unreg) 	(void*, char*);

	int  (*wplip_bind_link)	     	(void*, netdevice_t *);
	int  (*wplip_unbind_link)     	(void*, netdevice_t *);
	
#if defined(__WINDOWS__)	
	int  (*wplip_if_reg)		(void *lip_link_ptr, netdevice_t *dev, wanif_conf_t *conf);
	int  (*wplip_if_unreg)   	(void *);
#else
	int  (*wplip_if_reg)     	(void*, char*, wanif_conf_t *);
	int  (*wplip_if_unreg)   	(netdevice_t *);
#endif

	void (*wplip_disconnect)      	(void*, int);
	void (*wplip_connect)		(void*, int);

	int  (*wplip_rx) 		(void*, void *);
	void (*wplip_kick)		(void*, int);
	
	int  (*wplip_unreg)		(void*);

	int  (*wplip_get_if_status)	(void *, void *);

}wplip_reg_t;


enum {
	LIP_RAW,
	LIP_FRAME_RELAY,
	LIP_PPP,
	LIP_CHDLC,
};

#endif  
/* __WP_LIP_KERNEL_H_ */
