/******************************************************************************
 * wanpipe_ec_kernel.h	
 *
 * Author: 	Alex Feldman  <al.feldman@sangoma.com>
 *
 * Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 ******************************************************************************
 */

#ifndef __WANPIPE_EC_KERNEL_H
# define __WANPIPE_EC_KERNEL_H

#if defined(__LINUX__)
# include "wanpipe_tdm_api.h"
#endif

#if defined(WAN_KERNEL)
typedef struct wanec_iface_ 
{
	unsigned long init;
	
	void*      (*reg) (void*, u_int32_t, int, int, void*);
	int      (*unreg) (void*, void*);
	
	int      (*ioctl) (void);
	int        (*isr) (void*);
	int       (*poll) (void*, void*);
	int (*event_ctrl) (void*, void*, wan_event_ctrl_t*);

} wanec_iface_t;

#endif	/* WAN_KERNEL */

#endif /* __WANPIPE_EC_KERNEL_H */
