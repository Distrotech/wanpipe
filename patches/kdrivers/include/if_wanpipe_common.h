/*****************************************************************************
* if_wanpipe_common.h   Sangoma Driver/Socket common area definitions.
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:    (c) 2000 Sangoma Technologies Inc.
*
*               This program is free software; you can redistribute it and/or
*               modify it under the terms of the GNU General Public License
*               as published by the Free Software Foundation; either version
*               2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 13, 2000  Nenad Corbic      Initial version
*****************************************************************************/


#ifndef _WANPIPE_SOCK_DRIVER_COMMON_H
#define _WANPIPE_SOCK_DRIVER_COMMON_H

#if defined(WAN_KERNEL)

#if defined(__LINUX__)
# include <linux/version.h>
#endif

# include "wanpipe_debug.h"
# include "wanpipe_common.h"
# include "wanpipe_kernel.h"

#if defined(__LINUX__)
# include "if_wanpipe.h"
#endif


/*#define wan_next_dev(dev) *((netdevice_t**)dev->priv)*/
#define wan_next_dev(dev) *((netdevice_t**)wan_netif_priv(dev))

typedef struct {
	int	 (*open) (netdevice_t*);
	int	 (*close) (netdevice_t*);
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	int	 (*output) (netdevice_t*,netskb_t*,struct sockaddr*, struct rtentry*);
#else
	int	 (*send) (netskb_t* skb, netdevice_t*);
	struct net_device_stats* (*get_stats) (netdevice_t*);
#endif
	int	 (*ioctl) (netdevice_t*, struct ifreq*, wan_ioctl_cmd_t);
	void	 (*tx_timeout) (netdevice_t*);
#if defined (__LINUX__)
       int     (*change_mtu)(netdevice_t *dev, int new_mtu);
#endif
} wanpipe_common_iface_t;

typedef struct wanpipe_common {
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	/* !!! IMPORTANT !!! <- Do not move this parameter (GENERIC-PPP) */
	void		*prot_ptr;
/*	netdevice_t	*next;	*/	/* netdevice_t 	*slave; */
	void		*card;
	netdevice_t	*dev;
	struct mtx	ifmtx;
	unsigned char	state;
	unsigned char	usedby;
	wan_tasklet_t	bh_task;
	wan_timer_t	dev_timer;
	struct socket	*sk;	/* Wanpipe Sock bind's here  (Not used)*/ 
	unsigned int	protocol;
	unsigned short  lcn;
	void 		*lip;
	unsigned int    lip_prot;
	int		is_spppdev;	/* special mode for ADSL PPP_VC/PPP_LLC */
# if defined(NETGRAPH)
	char		ng_nodename [NG_NODELEN+1];
	int		ng_running;
	node_p		ng_node;
	hook_p		ng_upper_hook;
	hook_p		ng_lower_hook;
	hook_p		ng_debug_hook;
	int		ng_lowerhooks;
	int		ng_upperhooks;
	int		ng_datahooks;
	struct ifqueue	lo_queue;
	struct ifqueue	hi_queue;
	short		ng_timeout;
	struct callout	ng_timeout_handle;
	u_long		ng_out_deficit;	/* output since last input */
	u_char          ng_promisc;        /* promiscuous mode enabled */
	u_char          ng_autoSrcAddr;    /* always overwrite source address */
# endif
#elif defined(__LINUX__)
	/* !!! IMPORTANT !!! <- Do not move this parameter (GENERIC-PPP) */
	void*		*prot_ptr;
	netdevice_t 	*next;		/*slave;*/
	void		*card;	
	struct net_device_stats	if_stats;
	
	atomic_t receive_block;
	atomic_t command;
	atomic_t disconnect;
	
	struct sock *sk;		/* Wanpipe Sock bind's here */ 
	
	struct tq_struct wanpipe_task;    /* Immediate BH handler task */
	
	unsigned char rw_bind;			  /* Sock bind state */
	unsigned char usedby;
	unsigned char state;
	unsigned char svc;
	unsigned short lcn;
	unsigned int config_id;
	
	unsigned long	used;
	unsigned long	api_state;
	netdevice_t	*dev;
	wan_skb_queue_t rx_queue;
	wan_tasklet_t	bh_task;
	wan_timer_t	dev_timer;

	unsigned int	protocol;

	void 		*lip;
	unsigned int    lip_prot;
#elif defined(__WINDOWS__)
	/* !!! IMPORTANT !!! <- Do not move this parameter (GENERIC-PPP) */
	void*		*prot_ptr;
	netdevice_t 	*next;		/*slave;*/
	void		*card;	
	struct net_device_stats	if_stats;
	
	atomic_t receive_block;
	atomic_t command;
	atomic_t disconnect;
	
	struct sock *sk;		/* Wanpipe Sock bind's here */ 
	
	wan_tasklet_t wanpipe_task;    /* Immediate BH handler task */
	
	unsigned char rw_bind;			  /* Sock bind state */
	unsigned char usedby;
	unsigned char state;
	unsigned char svc;
	unsigned short lcn;
	unsigned int config_id;
	
	unsigned long	used;
	unsigned long	api_state;
	netdevice_t	*dev;
	wan_skb_queue_t rx_queue;
	wan_tasklet_t	bh_task;
	wan_timer_t	dev_timer;

	unsigned int	protocol;

	void 		*lip;
	unsigned int    lip_prot;

#endif
	int			is_netdev;
	wanpipe_common_iface_t	iface;
} wanpipe_common_t;

/* Used flags: Resources */
enum {
LCN_USED,
LCN_DEV,
LCN_TX_DEV,
LCN_X25_LINK,
LCN_SK_ID,
LCN_DSP_ID,
WAN_API_INIT
};

#define SK_ID LCN_SK_ID


enum {
	WANSOCK_UNCONFIGURED,	/* link/channel is not configured */
	WANSOCK_DISCONNECTED,	/* link/channel is disconnected */
	WANSOCK_CONNECTING,		/* connection is in progress */
	WANSOCK_CONNECTED,		/* link/channel is operational */
	WANSOCK_LIMIT,		/* for verification only */
	WANSOCK_DUALPORT,		/* for Dual Port cards */
	WANSOCK_DISCONNECTING,
	WANSOCK_BINDED,
	WANSOCK_BIND_LISTEN,
	WANSOCK_LISTEN
};


static __inline void *wan_get_lip_ptr(netdevice_t *dev)
{
	if (wan_netif_priv(dev)){
		return ((wanpipe_common_t*)wan_netif_priv(dev))->lip;	
	}
	return NULL;
}

static __inline int wan_set_lip_ptr(netdevice_t *dev, void *lipreg)
{
	if (wan_netif_priv(dev)){
		((wanpipe_common_t*)wan_netif_priv(dev))->lip = lipreg;	
		return 0;
	}
	return -ENODEV;
}

static __inline int wan_set_lip_prot(netdevice_t *dev, int protocol)
{
	if (wan_netif_priv(dev)){
		((wanpipe_common_t*)wan_netif_priv(dev))->lip_prot = protocol;	
		return 0;
	}
	return -ENODEV;
}


static __inline int wan_api_rx(void *chan_ptr,netskb_t *skb)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (test_bit(SK_ID,&chan->used) && chan->sk){
		return wanpipe_api_sock_rx(skb,chan->dev,chan->sk);
	}
#endif
	return -EINVAL;
}

static __inline int wan_api_rx_dtmf(void *chan_ptr,netskb_t *skb)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	if (test_bit(SK_ID,&chan->used) && chan->sk){
		return wanpipe_api_sock_rx(skb,chan->dev,chan->sk);
	}
#endif
	return -EINVAL;
}

static __inline void wan_wakeup_api(void *chan_ptr)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;

	if (test_bit(SK_ID,&chan->used) && chan->sk){
		wanpipe_api_poll_wake(chan->sk);
	}
#endif
}



static __inline int wan_reg_api(void *chan_ptr, void *dev, char *devname)
{
#if defined(__LINUX__)
	wanpipe_common_t *common = (wanpipe_common_t*)chan_ptr;

	if (wan_test_and_set_bit(WAN_API_INIT,&common->used)){
		DEBUG_EVENT("%s: Error: Failed to initialize API!\n",
				devname);
		return -EINVAL;
	}

	DEBUG_TEST("%s:    Initializing API\n",
			devname);

	common->sk=NULL;
	common->state = WAN_CONNECTING;
	common->dev = dev;
	common->api_state=0;
	
	WAN_IFQ_INIT(&common->rx_queue,10);

	/*WAN_TASKLET_INIT((&common->task),0,func,(unsigned long)common);*/

	wan_set_bit(WAN_API_INIT,&common->used);
#else
	DEBUG_EVENT("%s: Initializing API\n",
			devname);

#endif
	return 0;
}

static __inline int wan_unreg_api(void *chan_ptr, char *devname)
{
#if defined(__LINUX__)
	wanpipe_common_t *common = (wanpipe_common_t*)chan_ptr;
	
	if (!wan_test_and_clear_bit(WAN_API_INIT,&common->used)){
		DEBUG_TEST("%s: Error: Failed to unregister API!\n",
				devname);
		return -EINVAL;
	}

	DEBUG_EVENT("%s: Unregistering API\n",
			devname);

	common->state = WAN_DISCONNECTED;
//	WAN_TASKLET_KILL((&common->task));
	wan_skb_queue_purge(&common->rx_queue);	
#else
	DEBUG_EVENT("%s: Unregistering API\n",
			devname);

#endif
	return 0;
}

static __inline void wan_release_svc_dev(wanpipe_common_t *chan)
{
#if defined(__LINUX__)
	wan_clear_bit(0,(void *)&chan->rw_bind);
#endif
}

static __inline void wan_get_svc_dev(wanpipe_common_t *chan)
{
#if defined(__LINUX__)
	wan_set_bit(0,(void *)&chan->rw_bind);
#endif
}


static __inline int wan_bind_api_to_svc(void *chan_ptr, void *sk_id)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;

	if (!sk_id){
		return -EINVAL;
	}
	
	if (test_bit(SK_ID,&chan->used) || chan->sk){
		return -EBUSY;
	}

	wan_get_svc_dev(chan);
	chan->sk = sk_id;
	sock_hold(chan->sk);
	wan_set_bit(SK_ID,&chan->used);	
#endif
	return 0;	
}


static __inline int wan_unbind_api_from_svc(void *chan_ptr, void *sk_id)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan=(wanpipe_common_t*)chan_ptr;

	WAN_ASSERT_EINVAL((!chan));
	WAN_ASSERT_EINVAL((!chan->dev));

	DEBUG_TEST("%s:%s: BEGIN\n",__FUNCTION__,chan->dev->name);

	if (test_bit(SK_ID,&chan->used) && chan->sk){

		if ((struct sock*)sk_id != chan->sk){
			DEBUG_TEST("%s: ERROR: API trying to unbind invalid sock api! New=%p Orig=%p\n",
					chan->dev->name,sk_id,chan->sk);
			return -ENODEV;
		}
	
		wan_clear_bit(SK_ID,&chan->used);
		__sock_put(chan->sk);
		chan->sk=NULL;

		DEBUG_TEST("%s SK UNBIND SUCCESS\n",
				__FUNCTION__);

		wan_release_svc_dev(chan);
		return 0;
	}

	DEBUG_TEST("%s: ERROR: API trying to unbind invalid sock api! New=%p Orig=%p\n",
					chan->dev->name,sk_id,chan->sk);
	return -ENODEV;
#else
	return 0;
#endif	
}


static __inline void wan_update_api_state(void *chan_ptr)
{

#if defined(__LINUX__)

	int err=0; 
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	
	/* If the LCN state changes from Connected to Disconnected, and
	 * we are in the API mode, then notify the socket that the 
	 * connection has been lost */

	if (chan->usedby != API)
		return;

	if (test_bit(SK_ID,&chan->used) && chan->sk){

		if (chan->state != WAN_CONNECTED && test_bit(0,&chan->api_state)){
			wan_clear_bit(0,&chan->api_state);
			protocol_disconnected (chan->sk);
			return;
		}

			
		if (chan->state == WAN_CONNECTED){
			wan_set_bit(0,&chan->api_state);
			err=protocol_connected (chan->dev,chan->sk);
			if (err == -EINVAL){
				printk(KERN_INFO "%s:Major Error in Socket Above: CONN!!!\n",
						chan->dev->name);
				wan_unbind_api_from_svc(chan,chan->sk);
			}
			return;
		}

		if (chan->state == WAN_DISCONNECTED){
			err = protocol_disconnected (chan->sk);
		}
	}else{
		DEBUG_TEST("%s: Error: no sk device\n",__FUNCTION__);
	}

	if (chan->state != WAN_CONNECTED){
		wan_clear_bit(0,&chan->api_state);
	}
#endif
}



#define MAX_API_RX_QUEUE 10
static __inline int wan_api_enqueue_skb(void *chan_ptr,netskb_t *skb)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;

	if (wan_skb_queue_len(&chan->rx_queue) > MAX_API_RX_QUEUE){
		return -EBUSY;
	}
	wan_skb_queue_tail(&chan->rx_queue,skb);

	if (wan_skb_queue_len(&chan->rx_queue) >= MAX_API_RX_QUEUE){
		return 1;
	}
#endif
	return 0;
}

static __inline netskb_t* wan_api_dequeue_skb(void *chan_ptr)
{
#if defined(__LINUX__)
	wanpipe_common_t *chan = (wanpipe_common_t*)chan_ptr;
	return wan_skb_dequeue(&chan->rx_queue);
#else
	return NULL;
#endif

}

#if 0
void wp_debug_func_init(void)
{
	DBG_ARRAY_CNT=0;
}
#endif


#if defined(__WINDOWS__)
extern int wanpipe_lip_rx(void *chan, void *sk_id);
extern int wanpipe_lip_connect(void *chan, int );
extern int wanpipe_lip_disconnect(void *chan, int);
extern int wanpipe_lip_kick(void *chan,int);
extern int wanpipe_lip_get_if_status(void *chan, void *m);
#endif

#endif /* WAN_KERNEL */


#endif
