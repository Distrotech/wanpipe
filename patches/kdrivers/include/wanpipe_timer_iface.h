/*****************************************************************************
* wanpipe_timer_iface.h
*
* 		WANPIPE(tm) TIMER Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright (c) 2007 - 08, Sangoma Technologies
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY <copyright holder> ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/


#ifndef __WANPIPE_TIMER_IFACE_H
#define __WANPIPE_TIMER_IFACE_H

#define WANPIPE_TIMER_API_CMD_SZ 128

#include "wanpipe_kernel.h"

typedef struct wanpipe_timer_api{
	u8	operational_status;
	u32 cmd;
	union {
		struct {
			u32 sequence;
			wan_time_t sec;
			wan_suseconds_t usec;
		};
		char data[WANPIPE_TIMER_API_CMD_SZ-sizeof(u8)+sizeof(u32)];
	};
}wanpipe_timer_api_t;


enum {
	WANPIPE_IOCTL_TIMER_INVALID,
	WANPIPE_IOCTL_TIMER_EVENT,
};

#if defined(__KERNEL__)
int wanpipe_wandev_timer_create(void);
int wanpipe_wandev_timer_free(void);
int wp_timer_device_reg_tick(sdla_t *card);
int wp_timer_device_unreg(sdla_t *card);
#endif

#endif


