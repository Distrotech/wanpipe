/*
 * Copyright (c) 2002
 *	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
 *
 *	$Id: sdla_adsl.h,v 1.10 2008-02-04 18:03:54 sangoma Exp $
 */

/*************************************************************************
 * sdla_adsl.h	WANPIPE(tm)
 *
 * Author:	Alex Feldman <al.feldman@sangoma.com>
 *
 *
 * ========================================================================
 * Jun 17, 2002	Alex Feldman	Initial version.
 * ========================================================================
 *
 **************************************************************************
 */

#ifndef __SDLA_ADSL_H
#  define __SDLA_ADSL_H


#include "sdla_adsl_iface.h"

#define ADSL_TEST_DRIVER_RESPONSE	0x01
#define	ADSL_READ_DRIVER_VERSION	0x02    	
#define	ADSL_ROUTER_UP_TIME		0x03    		
	      	
#define	ADSL_ENABLE_TRACING		0x06  
#define	ADSL_DISABLE_TRACING		0x07 
#define	ADSL_GET_TRACE_INFO		0x08 

#define MAX_TRACE_TIMEOUT         	(HZ*10)


typedef struct adsl_cfg {
	unsigned char	adsl_framing;
	unsigned char	adsl_trellis;
	unsigned char	adsl_as0;
	unsigned char	adsl_as0_latency;
	unsigned char	adsl_as1;
	unsigned char	adsl_as1_latency;
	unsigned char	adsl_ls0;
	unsigned char	adsl_ls0_latency;
	unsigned char	adsl_ls1;
	unsigned char	adsl_ls1_latency;
	unsigned char	adsl_redundant_bytes;
	unsigned char	adsl_interleave_s_up;
	unsigned char	adsl_interleave_d_up;
	unsigned char	adsl_interleave_r_up;
	unsigned char	adsl_fast_r_up;
	unsigned char	adsl_interleave_s_down;
	unsigned char	adsl_interleave_d_down;
	unsigned char	adsl_interleave_r_down;
	unsigned char	adsl_selected_standard;
} adsl_cfg_t;

#if !defined(__WINDOWS__)
#undef  wan_udphdr_data
#define wan_udphdr_data	wan_udphdr_adsl_data
#undef	wan_udp_data
#define wan_udp_data	wan_udp_hdr.wan_udphdr_adsl_data
#endif

#ifdef WAN_KERNEL

typedef struct adsl_private_area
{
	wanpipe_common_t 	common;/* MUST be at the top */
	void*			pAdapter;
	char			if_name[WAN_IFNAME_SZ];
	u_char			macAddr[6];
	atomic_t		udp_pkt_len;
	unsigned char 		udp_pkt_data[sizeof(wan_udp_pkt_t)];
	unsigned char		udp_pkt_src;
	unsigned char		remote_eth_addr[6];
	wan_time_t		router_start_time;		/*unsigned long 		router_start_time;*/
	wan_time_t		router_up_time;		/*unsigned long 		router_up_time;*/
	unsigned long  		trace_timeout;
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	struct ifmedia media;	/* media information */
#endif
#if defined(__WINDOWS__)
	void			*card;
	struct net_device_stats	if_stats;
	wan_trace_t		trace_info;
#endif
} adsl_private_area_t;


typedef struct adsl_trace_info
{
	unsigned long  		tracing_enabled;
	wan_skb_queue_t		trace_queue;
	unsigned long  		trace_timeout;
	unsigned int   		max_trace_queue;
} adsl_trace_info_t;


#endif  /* WAN_KERNEL */

/*extern void*	adsl_create(void* dsl_cfg, void* card, void* virt_addr, char* devname);
extern void*	adsl_new_if(void*, u_char*, void*);
extern int	adsl_del_if(void*);
extern int	adsl_can_tx(void*);
extern int	adsl_send(void*, void* tx_skb,unsigned int);
extern void	adsl_timeout(void*);
extern void	adsl_isr(void*);
extern void	adsl_udp_cmd(void*, unsigned char, unsigned char*, unsigned short*);
extern void	adsl_disable_comm(void*);
*/
extern void	adsl_lan_multicast(void*, short,char*,int);
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
extern void	adsl_atm_tasklet(void*, int);
#else
extern void	adsl_atm_tasklet(unsigned long);
#endif
/*
extern void	adsl_task_schedule(void*);
extern void	adsl_task_kill(void*);
extern void	adsl_lan_rx(void*,void*,u_int32_t,unsigned char*, int);
extern int	adsl_tracing_enabled(void*);
extern void	adsl_tx_complete(void*, int, int);
extern void	adsl_rx_complete(void*);
extern int GpWanWriteRoom(void *pChan_ptr);
extern int GpWanOpen(void *pAdapter_ptr, unsigned char line, void *tty, void **data);
extern void GpWanClose(void *pAdapter_ptr, void *pChan_ptr);
extern int GpWanTx(void *pChan_ptr, int fromUser, const unsigned char *buffer, int bufferLen);

extern void* adsl_get_trace_ptr(void *pAdapter_ptr);
extern int adsl_wan_interface_type(void *);
*/
extern int	adsl_queue_trace(void*, void*);

#endif /* __SDLA_ADSL_H_ */

