/*****************************************************************************
* sdla_adsl.c	WANPIPE(tm) Multiprotocol WAN Link Driver. ADSL module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*		Alex Feldman <al.feldman@sangoma.com
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
* ============================================================================
* Feb 25, 2002  Nenad Corbic	Initial version
* 				Based on GlobeSpan ADSL device driver.
* 				http://www.globespan.net
*
* July 1, 2002	Alex Feldman	Porting ADSL driver to FreeBSD/OpenBSD
*
* March 9, 2006	David Rokhvarg	Ported ADSL driver to MS Windows XP/2003
*
* ============================================================================
* History:
*   2/5/01 PG: Modified to add the support for EOC handling
*-----------------------------------------------------------------------------
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* ALTERNATIVELY, this driver may be distributed under the terms of
* the following license, in which case the provisions of this license
* are required INSTEAD OF the GNU General Public License. (This clause
* is necessary due to a potential bad interaction between the GPL and
* the restrictions contained in a BSD-style copyright.)
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, and the entire permission notice in its entirety,
*    including the disclaimer of warranties.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote
*    products derived from this software without specific prior
*    written permission.
*
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************************/

#define __SDLA_ADSL

/*
**************************************************************************
**			I N C L U D E S  F I L E S			**
**************************************************************************
*/


#include "wanpipe_version.h"
#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_abstr.h"
#include "wanpipe.h"
#include "if_wanpipe_common.h"
#include "sdla_adsl.h"

#if defined(__LINUX__)
#include "if_wanpipe.h"
#include "wanpipe_syncppp.h"
#endif

#if defined(__WINDOWS__)
#include "sdladrv_private.h"	/* prototypes of global functions */
extern
void
wan_get_random_mac_address(
	OUT unsigned char *mac_address
	);

extern 
int wp_get_motherboard_enclosure_serial_number(
	OUT char *buf, 
	IN int buf_length
	);

extern
int 
wp_get_netdev_open_handles_count(
	netdevice_t *sdla_net_dev
	);

#endif

/*
**************************************************************************
**				D E F I N E S				**
**************************************************************************
*/

#define MAX_TRACE_QUEUE		100

#define MAX_TRACE_BUFFER	(MAX_LGTH_UDP_MGNT_PKT - 	\
	sizeof(iphdr_t) - 		\
	sizeof(udphdr_t) - 		\
	sizeof(wan_mgmt_t) - 		\
	sizeof(wan_trace_info_t) - 	\
	sizeof(wan_cmd_t))

/*
**************************************************************************
**				M A C R O S				**
**************************************************************************
*/
#define MAX(a,b) 	(((a)>(b))?(a):(b))


#if defined(__LINUX__)
# define ETHER_IOCTL(ifp, cmd, data)    -EOPNOTSUPP
#endif

#if defined(__LINUX__)
# define LIST_FIRST_MCLIST(dev)		dev->mc_list
# define LIST_NEXT_MCLIST(mclist)	mclist->next
#elif defined(__WINDOWS__)
# define LIST_FIRST_MCLIST(dev)		dev->mc_list
# define LIST_NEXT_MCLIST(mclist)	mclist->next
#elif defined(__FreeBSD__)
# define LIST_FIRST_MCLIST(dev)		LIST_FIRST(&dev->if_multiaddrs)
# define LIST_NEXT_MCLIST(mclist)	mclist->ifma_link.le_next
#elif defined(__OpenBSD__)
# define LIST_FIRST_MCLIST(dev)		NULL
# define LIST_NEXT_MCLIST(mclist)	NULL
#elif defined(__NetBSD__)
# define LIST_FIRST_MCLIST(dev)		NULL
# define LIST_NEXT_MCLIST(mclist)	NULL
#else
# error "Undefined LIST_FIRST_MCLIST/LIST_NEXT_MCLIST macros!"
#endif

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

enum {
	TX_BUSY_SET
};

/*
**************************************************************************
**			G L O B A L  V A R I A B L E S			**
**************************************************************************
*/
#define WAN_ADSL_VCIVPI_MAX_LEN	100
wan_adsl_vcivpi_t	adsl_vcivpi_list[WAN_ADSL_VCIVPI_MAX_LEN];
int			adsl_vcivpi_num = 0;

/*
**************************************************************************
**			F U N C T I O N  P R O T O T Y P E S		**
**************************************************************************
*/

#if defined(__LINUX__)
static int 	adsl_open(netdevice_t*);
static int 	adsl_close(netdevice_t*);
static int	adsl_output(struct sk_buff *skb, struct net_device *dev);
static int 	adsl_ioctl(netdevice_t* ifp, struct ifreq *ifr, int command);
static void	adsl_tx_timeout (netdevice_t *dev);
static void	adsl_multicast(netdevice_t* dev);
static struct 	net_device_stats* adsl_stats(netdevice_t* dev);
static int 	wanpipe_attach_sppp(sdla_t *card, netdevice_t *dev, wanif_conf_t *conf);
#elif defined(__WINDOWS__)
static int 	adsl_open(netdevice_t*);
static int 	adsl_close(netdevice_t*);
static int	adsl_output(struct sk_buff *skb, netdevice_t *dev);
static int 	adsl_ioctl(netdevice_t* ifp, struct ifreq *ifr, int command);
static void	adsl_tx_timeout (netdevice_t *dev);
static void	adsl_multicast(netdevice_t* dev);
static struct net_device_stats* adsl_stats(netdevice_t* dev);
static int 	wanpipe_attach_sppp(sdla_t *card, netdevice_t *dev, wanif_conf_t *conf);

static int netif_rx(netdevice_t	*sdla_net_dev, netskb_t *rx_skb);
static int adsl_if_send(netskb_t* skb, netdevice_t* dev);

extern DRIVER_VERSION drv_version;
#else
static void 	adsl_watchdog (netdevice_t*);
static void 	adsl_tx (netdevice_t *);
static void 	adsl_sppp_tx (netdevice_t *);
static int 	adsl_output (struct ifnet *,struct mbuf *,struct sockaddr *,struct rtentry *); 
static int 	adsl_ioctl(struct ifnet*, u_long, caddr_t);
# if defined(__FreeBSD__)
static void 	adsl_init(void* priv);
# elif defined(__NetBSD__)
static int 	adsl_init(netdevice_t* dev);
# endif
#endif

static WAN_IRQ_RETVAL 	wpa_isr (sdla_t*);
static void 	disable_comm(sdla_t* card);
static int 	new_if (wan_device_t*, netdevice_t*, wanif_conf_t*);
static int 	del_if (wan_device_t*, netdevice_t*);
static int 	process_udp_cmd(netdevice_t*,wan_udp_hdr_t*);

#if defined(__WINDOWS__)
KDEFERRED_ROUTINE process_bh;
static int adsl_init(void* priv);
#else

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void 	process_bh (void *);
# else
static void 	process_bh (struct work_struct *work);
# endif  
#endif

static int process_udp_mgmt_pkt(sdla_t*, netdevice_t*, adsl_private_area_t*, int);

#endif

unsigned int	adsl_trace_queue_len(void *trace_ptr);


/*
**************************************************************************
**			P U B L I C  P R O T O T Y P E S		**
**************************************************************************
*/

/*============================================================================
* DSL Hardware initialization routine.
*
* This routine is called by the main WANPIPE module during setup.  At this
* point adapter is completely initialized and firmware is running.
* 
* Return:	0	o.k.
*		< 0	failure.
*/
int wp_adsl_init (sdla_t* card, wandev_conf_t* conf)
{
	DEBUG_EVENT("%s: Initializing S518 ADSL card...\n",
		card->devname);

	card->isr			= &wpa_isr;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= NULL; /*&update; */
	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->wandev.udp_port   	= conf->udp_port;
	card->wandev.new_if_cnt = 0;

	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 

	conf->u.adsl.tty_minor = (unsigned char)conf->tty_minor;

	switch (conf->u.adsl.EncapMode){
		case RFC_MODE_ROUTED_IP_LLC:
		case RFC_MODE_ROUTED_IP_VC:	
			if (conf->mtu > 9188){
				conf->mtu = 9188;
			}
			break;

		default:
			if (conf->mtu > 1500){
				conf->mtu = 1500;
			}
			break;
	}
	conf->u.adsl.mtu = (unsigned short)conf->mtu;
	card->wandev.mtu = conf->mtu;

	card->u.adsl.adapter = adsl_create(&conf->u.adsl, 
		card, 
		card->devname);
	if (card->u.adsl.adapter == NULL){
		return -EINVAL;
	}

	card->u.adsl.EncapMode = conf->u.adsl.EncapMode;
	card->wandev.station = conf->u.adsl.EncapMode;

#if 0
	if (adsl_wan_interface_type(card->u.adsl.adapter)){
		int err=adsl_wan_init(card->u.adsl.adapter);
		if (err){
			adsl_disable_comm(card->u.adsl.adapter);
			return err;
		}
		MOD_INC_USE_COUNT;
	}
#endif

#if defined(__LINUX__)
	card->disable_comm		= &disable_comm;
#else
	if (adsl_wan_interface_type(card->u.adsl.adapter)){
		card->disable_comm	= &disable_comm;
	}
#endif

	card->wandev.state = WAN_CONNECTING;	

#if defined(__WINDOWS__)
	//at this point we can handle front end interrupts
	card->init_flag = 0;
#endif

	return 0;
}

/*============================================================================
* Handle transmit timeout event from netif watchdog
*/
#if defined(__NetBSD__) || defined (__FreeBSD__) || defined (__OpenBSD__)
static void adsl_watchdog(netdevice_t* dev)
{
	adsl_private_area_t*	dsl = NULL;
	sdla_t*			card = NULL;

	WAN_ASSERT1(dev == NULL);
	dsl = wan_netif_priv(dev);
	WAN_ASSERT1(dsl == NULL);
	card = dsl->common.card;
	DEBUG_TIMER("%s: Watchdog function called\n", 
		wan_netif_name(dev));

	process_udp_mgmt_pkt(card, dev, dsl,0);

}
#endif


/*============================================================================
* Create new logical channel.
* This routine is called by the router when ROUTER_IFNEW IOCTL is being
* handled.
* o parse media- and hardware-specific configuration
* o make sure that a new channel can be created
* o allocate resources, if necessary
* o prepare network device structure for registaration.
*
* Return:	0	o.k.
*		< 0	Failure (channel will not be created)
*/
static int new_if (wan_device_t* wandev, netdevice_t* ifp, wanif_conf_t* conf)
{
	sdla_t*			card = wandev->priv;
	adsl_private_area_t*	adsl = NULL;

#if defined(__WINDOWS__)
	if ((ifp->name[0] == '\0') || (strlen(ifp->name) > WAN_IFNAME_SZ)) {
#else
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
#endif
		DEBUG_EVENT("%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	if (adsl_wan_interface_type(card->u.adsl.adapter)){
		return -EINVAL;
	}

	/* (FreeBSD/OpenBSD only) 
	* Verify that SPPP is enabled in the kernel.
	*/
#if !defined(__WINDOWS__)
	if (!WAN_SPPP_ENABLED){
		if (card->u.adsl.EncapMode == RFC_MODE_PPP_VC || 
			card->u.adsl.EncapMode == RFC_MODE_PPP_LLC){
				DEBUG_EVENT("%s: Please verify that SPPP is enabled in your kernel!\n",
					card->devname);
				return -EINVAL;
		}
	}
#endif

	/* allocate and initialize private data */
	adsl = wan_malloc(sizeof(adsl_private_area_t));
	if (adsl == NULL){ 
		DEBUG_EVENT("%s: Failed allocating private data...\n", 
			card->devname);
		return -ENOMEM;
	}

	memset(adsl,0,sizeof(adsl_private_area_t));

	DEBUG_EVENT("%s: Configuring Interface: %s\n",
		card->devname, conf->name);

	/* prepare network device data space for registration */
	memcpy(adsl->if_name, conf->name, strlen(conf->name));
	if (wan_netif_init(ifp, adsl->if_name)){
		DEBUG_EVENT("%s: Failed to initialize network interface!\n", 
			conf->name);
		wan_free(adsl);
		return -ENOMEM;
	}
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	ifp->if_softc	 = adsl;
	ifp->if_output   = adsl_output;
	ifp->if_start    = adsl_tx;
	ifp->if_watchdog = adsl_watchdog;
	ifp->if_ioctl	 = adsl_ioctl;
# if defined(__NetBSD__) || defined (__FreeBSD__)
	ifp->if_init     = adsl_init;
# endif
	ifp->if_snd.ifq_len    = 0;
#elif defined(__LINUX__) || defined(__WINDOWS__)

#if defined(__LINUX__)
	WAN_NETDEV_OPS_BIND(ifp,wan_netdev_ops);
	WAN_TASKQ_INIT((&adsl->common.wanpipe_task), 0, process_bh, ifp);
#elif defined(__WINDOWS__)
	/* # define WAN_TASKQ_INIT(task, priority, func, arg) */
	WAN_TASKQ_INIT((&adsl->common.wanpipe_task), 0, process_bh, ifp);
#endif

	wan_netif_set_priv(ifp,adsl);
	ifp->irq        = card->wandev.irq;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &ifp->mem_start);
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &ifp->mem_end);

	WAN_NETDEV_OPS_OPEN(ifp,wan_netdev_ops,&adsl_open);
	WAN_NETDEV_OPS_STOP(ifp,wan_netdev_ops,&adsl_close);

#if defined(__WINDOWS__)
	ifp->hard_start_xmit    = &adsl_if_send;/* will call adsl_output() */

	ifp->init				= &adsl_init;

	adsl->common.card = card;
	adsl->card = card;

	wpabs_trace_info_init(&adsl->trace_info, MAX_TRACE_QUEUE);
	ifp->trace_info = &adsl->trace_info;
#else
	WAN_NETDEV_OPS_XMIT(ifp,wan_netdev_ops,&adsl_output);

#endif
	
	adsl->common.dev = ifp;

	WAN_NETDEV_OPS_STATS(ifp,wan_netdev_ops,&adsl_stats);

#if !defined(__WINDOWS__)
	WAN_NETDEV_OPS_SET_MULTICAST_LIST(ifp,wan_netdev_ops,&adsl_multicast);
#endif
	WAN_NETDEV_OPS_TIMEOUT(ifp,wan_netdev_ops,&adsl_tx_timeout);
	ifp->watchdog_timeo     = (1*HZ);

	WAN_NETDEV_OPS_IOCTL(ifp,wan_netdev_ops,adsl_ioctl);
#endif/*#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)*/

	adsl->common.card	= card;
	adsl->pAdapter	= adsl_new_if(card->u.adsl.adapter, adsl->macAddr, ifp);

	if (adsl->pAdapter == NULL){
		DEBUG_EVENT("%s: Failed ADSL interface initialization\n",
			card->devname);
		del_if(wandev, ifp);
		return -EINVAL;
	}

#if !defined(__WINDOWS__)
	if(strcmp(conf->usedby, "STACK") == 0) {
		adsl->common.usedby = STACK;
		DEBUG_EVENT( "%s:%s: Running in Stack mode.\n",
			card->devname,adsl->if_name);

		card->u.adsl.EncapMode=RFC_MODE_STACK_VC;

	}else{
#endif

		adsl->common.usedby = WANPIPE;
		DEBUG_EVENT( "%s:%s: Running in Wanpipe mode.\n",
			card->devname,adsl->if_name);
#if !defined(__WINDOWS__)
	}
#endif

	/*TASK_INIT(&adsl->tq_atm_task, 0, &adsl_atm_tasklet, adsl->pAdapter);*/
	switch (card->u.adsl.EncapMode)
	{
	case RFC_MODE_BRIDGED_ETH_LLC:	
	case RFC_MODE_BRIDGED_ETH_VC:
		DEBUG_EVENT("%s: Configuring %s as Ethernet\n",
			card->devname, wan_netif_name(ifp));
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		ifp->if_flags  |= (IFF_DRV_RUNNING|IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST);
		/* if_type,if_addrlen, if_hdrlen, if_mtu inialize 
		* in ether_ifattach
		*/
# if !defined(__NetBSD__)
		bcopy(adsl->macAddr, 
			WAN_IFP2ENADDR(ifp)/*(WAN_IFP2AC(ifp))->ac_enaddr*/,
			6);
# endif
		bcopy(adsl->macAddr, wandev->macAddr, ETHER_ADDR_LEN); 
#else

#if !defined(__WINDOWS__)
		ether_setup(ifp);
#endif
		memcpy(ifp->dev_addr, adsl->macAddr, 6);
#endif
		break;

	case RFC_MODE_ROUTED_IP_LLC:
	case RFC_MODE_ROUTED_IP_VC:	
		DEBUG_EVENT("%s: Configuring %s as Raw IP\n",
			card->devname, wan_netif_name(ifp));
#if !defined(__WINDOWS__)
		/* Drop down to STACK_VC: Standard if config */
	case RFC_MODE_STACK_VC:
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		ifp->if_mtu	= card->wandev.mtu; /* ETHERMTU */
		ifp->if_type    = IFT_OTHER;
		ifp->if_flags  |= (IFF_POINTOPOINT|IFF_DRV_RUNNING);
		/*ifp->if_flags  |= IFF_NOARP;*/
		ifp->if_hdrlen	= 0;
#elif defined(__LINUX__)
		ifp->type = ARPHRD_PPP; 
		ifp->flags |= IFF_POINTOPOINT;
		ifp->flags |= IFF_NOARP;
		ifp->mtu = card->wandev.mtu;
		ifp->hard_header_len=0; /* ETH_HLEN;*/
		ifp->addr_len=0; 	/* ETH_ALEN;*/
		ifp->tx_queue_len = 100;
#endif
		break;

#if !defined(__WINDOWS__)
	case RFC_MODE_PPP_VC:
	case RFC_MODE_PPP_LLC:
		DEBUG_EVENT("%s: Attaching SPPP protocol \n", card->devname);
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		WAN_SPPP_ATTACH(ifp);
		ifp->if_start = adsl_sppp_tx;
#else
		ifp->type = ARPHRD_PPP; 
		ifp->flags |= IFF_POINTOPOINT;
		ifp->flags |= IFF_NOARP;
		ifp->mtu = card->wandev.mtu;
		ifp->hard_header_len=0; 
		ifp->addr_len=0; 	
		{
			int err=wanpipe_attach_sppp(card,ifp,conf);
			if (err != 0){
				del_if(wandev,ifp);
				return err;
			}
		}
		ifp->tx_queue_len = 100;

		WAN_NETDEV_OPS_OPEN(ifp,wan_netdev_ops,&adsl_open);
		WAN_NETDEV_OPS_STOP(ifp,wan_netdev_ops,&adsl_close);
		WAN_NETDEV_OPS_XMIT(ifp,wan_netdev_ops,&adsl_output);
		WAN_NETDEV_OPS_STATS(ifp,wan_netdev_ops,&adsl_stats);
		WAN_NETDEV_OPS_SET_MULTICAST_LIST(ifp,wan_netdev_ops,&adsl_multicast);
		WAN_NETDEV_OPS_TIMEOUT(ifp,wan_netdev_ops,&adsl_tx_timeout);
		ifp->watchdog_timeo     = (1*HZ);
		WAN_NETDEV_OPS_IOCTL(ifp,wan_netdev_ops,adsl_ioctl);

#endif
		break;
#endif /* #if !defined(__WINDOWS__) */

	default:
		DEBUG_ERROR("%s: Error invalid EncapMode %i\n",
			card->devname,card->u.adsl.EncapMode);
		del_if(wandev,ifp);
		return -EINVAL;
	}


	wan_getcurrenttime(&adsl->router_start_time, NULL);
	return 0;
}

static int del_if (wan_device_t* wandev, netdevice_t* ifp)
{
	sdla_t*			card = wandev->priv;
	adsl_private_area_t*	adsl = wan_netif_priv(ifp);

	WAN_ASSERT(adsl == NULL);

	DEBUG_EVENT("%s: Deleting Interface %s\n",
		card->devname, wan_netif_name(ifp));

	if (adsl->pAdapter != NULL){
		adsl_del_if(adsl->pAdapter);	

		/* Disable communication */
		DEBUG_EVENT("%s: Disabling ADSL communications...\n",
			card->devname);
		adsl_disable_comm(adsl->pAdapter);
	}
	card->u.adsl.adapter 	= NULL;
	adsl->pAdapter 		= NULL;

	/* Initialize interrupt handler pointer */
	card->isr = NULL;

#if defined(__WINDOWS__)
	/* free the private area */
	wan_free(adsl);
	return 0;
#else
	/* Initialize network interface */
	if (card->u.adsl.EncapMode == RFC_MODE_PPP_VC ||
		card->u.adsl.EncapMode == RFC_MODE_PPP_LLC){

			DEBUG_EVENT("%s: Detaching SPPP protocol \n", card->devname);

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
			WAN_SPPP_FLUSH(ifp);
			WAN_SPPP_DETACH(ifp);
#else
			if (adsl->common.prot_ptr){
				wp_sppp_detach(ifp);

				WAN_NETDEV_OPS_IOCTL(ifp,wan_netdev_ops,NULL);

				wan_free(adsl->common.prot_ptr);
				adsl->common.prot_ptr= NULL;
			}
#endif
	}
#endif/* #if !defined(__WINDOWS__)*/


#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#if 0
	ifp->if_output 	= NULL;
	ifp->if_start 	= NULL;
	ifp->if_ioctl 	= NULL;
	ifp->if_watchdog= NULL;
# if defined(__NetBSD__) || defined(__FreeBSD__)
	ifp->if_init 	= NULL;
# endif
	ifp->if_flags 	&= ~(IFF_UP | IFF_DRV_RUNNING | IFF_DRV_OACTIVE);
	if (ifp->if_softc){ 
		wan_free(ifp->if_softc);
		ifp->if_softc = NULL;
	}
#endif
#else
	ifp->irq        = 0;
	ifp->mem_start  = 0;
	ifp->mem_end    = 0;
	/* Private structure will be free later */ 
	wan_netif_del(ifp);
#endif

	return 0;
}

/*+F*************************************************************************
* Function:
*   adsl_init
*
* Description:
*-F*************************************************************************/
#if defined(__FreeBSD__)
static void adsl_init(void* priv){ return; }
#elif defined(__NetBSD__)
static int adsl_init(netdevice_t* dev){return 0; }
#elif defined(__WINDOWS__)
static int adsl_init(netdevice_t* dev){return 0; }
#endif

/*+F*************************************************************************
* Function:
*   adsl_vcivpi_update
*
* Description:
*-F*************************************************************************/
void adsl_vcivpi_update(sdla_t* card, wandev_conf_t* conf)
{
	wan_adsl_conf_t*	adsl_conf = &conf->u.adsl;
	int 			x = 0;

	for(x = 0; x < adsl_vcivpi_num; x++){
		adsl_conf->vcivpi_list[x].vci = adsl_vcivpi_list[x].vci;
		adsl_conf->vcivpi_list[x].vpi = adsl_vcivpi_list[x].vpi;
	}

	adsl_conf->vcivpi_num = (unsigned short)adsl_vcivpi_num;
}

/*+F*************************************************************************
* Function:
*   disable_comm
*
* Description:
*	Called by the LAN subsystem when the ethernet interface is
*	configured make ready to accept data (ifconfig up).
*-F*************************************************************************/
static void disable_comm(sdla_t* card)
{	
	char wan;

	if (!card->u.adsl.adapter)
		return;

	wan = (char)adsl_wan_interface_type(card->u.adsl.adapter);

	adsl_disable_comm(card->u.adsl.adapter);
	card->u.adsl.adapter 	= NULL;

	if (wan){
#if !defined(__WINDOWS__)
		MOD_DEC_USE_COUNT;
#endif
	}
}

#if defined(__LINUX__)
/*+F*************************************************************************
* Function:
*   adsl_tx_timeout
*
* Description:
* 	Handle transmit timeout event from netif watchdog
*-F*************************************************************************/
static void adsl_tx_timeout (netdevice_t *dev)
{
	adsl_private_area_t*	adsl = wan_netif_priv(dev);
	sdla_t*			card = adsl->common.card;

	/* If our device stays busy for at least 5 seconds then we will
	* kick start the device by making dev->tbusy = 0.  We expect
	* that our device never stays busy more than 5 seconds. So this 
	* is only used as a last resort.
	*/
#if defined(__LINUX__)
	dev->trans_start = SYSTEM_TICKS;
#endif
	card->wandev.stats.collisions++;

	adsl_timeout(card->u.adsl.adapter);

	WAN_NETIF_WAKE_QUEUE(dev);
}

/*+F*************************************************************************
* Function:
*   adsl_open
*
* Description:
*   Called by the LAN subsystem when the ethernet interface is configured
*   make ready to accept data (ifconfig up).
*-F*************************************************************************/
static int adsl_open(netdevice_t* ifp)
{
	int     status = 0;
	adsl_private_area_t* adsl = wan_netif_priv(ifp);
	sdla_t* card = adsl->common.card;

#if defined (__LINUX__)
	if (adsl->common.prot_ptr){
		if (wp_sppp_open(ifp)){
			return -EIO;
		}
	}
#endif	

	/* Start Tx queuing */
	WAN_NETDEVICE_START(ifp);
	if (card->wandev.state == WAN_CONNECTED) {
		WAN_NETIF_CARRIER_ON(ifp);
		WAN_NETIF_START_QUEUE(ifp); 
	} else {
                WAN_NETIF_CARRIER_OFF(ifp);
	       	WAN_NETIF_STOP_QUEUE(ifp); 
	}

	if (adsl->common.usedby == STACK){
		/* Force the lip to connect state
		* since there is no state change
		* mechanism for an interface 
		* FIXME: This is temporary */
		wanpipe_lip_connect(adsl,0);
	}

	MOD_INC_USE_COUNT;
	return status;
}

/*+F*************************************************************************
* Function:
*   adsl_close
*
* Description:
*   Called by the LAN subsystem when the ethernet interface is disabled
*   (ifconfig down).
*-F*************************************************************************/
int adsl_close(netdevice_t* ifp)
{
	int     status = 0;
#if defined (__LINUX__)
	adsl_private_area_t* adsl = wan_netif_priv(ifp);
#endif

#if defined (__LINUX__)
	if (adsl->common.prot_ptr){
		if (wp_sppp_close(ifp)){
			return -EIO;
		}
	}
#endif	

	/* Stop Tx queuing */
	WAN_NETIF_STOP_QUEUE(ifp);
	WAN_NETDEVICE_STOP(ifp);
	MOD_DEC_USE_COUNT;
	return status;
}
#elif defined(__WINDOWS__)
/*+F*************************************************************************
* Function:
*   adsl_tx_timeout
*
* Description:
* 	Handle transmit timeout event from netif watchdog
*-F*************************************************************************/
static void adsl_tx_timeout (netdevice_t *dev)
{
	adsl_private_area_t*	adsl = wan_netif_priv(dev);
	sdla_t*			card = adsl->common.card;

	DBG_DSL_NOT_IMPLD();

	/* If our device stays busy for at least 5 seconds then we will
	* kick start the device by making dev->tbusy = 0.  We expect
	* that our device never stays busy more than 5 seconds. So this 
	* is only used as a last resort.
	*/
	/*
	dev->trans_start = SYSTEM_TICKS;
	card->wandev.stats.collisions++;

	adsl_timeout(card->u.adsl.adapter);
	WAN_NETIF_WAKE_QUEUE(dev);
	*/
}

/*+F*************************************************************************
* Function:
*   adsl_open
*
* Description:
*   Called by the LAN subsystem when the ethernet interface is configured
*   make ready to accept data (ifconfig up).
*-F*************************************************************************/
static int adsl_open(netdevice_t* ifp)
{
	int     status = 0;
	
#if defined (__LINUX__) || defined(__WINDOWS__)
	adsl_private_area_t* adsl = wan_netif_priv(ifp);
#endif
	return status;
}

/*+F*************************************************************************
* Function:
*   adsl_close
*
* Description:
*   Called by the LAN subsystem when the ethernet interface is disabled
*   (ifconfig down).
*-F*************************************************************************/
int adsl_close(netdevice_t* ifp)
{
	int     status = 0;

#if defined (__LINUX__) || defined(__WINDOWS__)
	adsl_private_area_t* adsl = wan_netif_priv(ifp);
#endif

	return status;
}
#endif

/*+F*************************************************************************
* Function:
*   wpa_isr
*
* Description:
*-F*************************************************************************/
static WAN_IRQ_RETVAL wpa_isr (sdla_t* card)
{
	int ret;

	DEBUG_ISR("%s()\n", __FUNCTION__);

	if (!card->u.adsl.adapter){
		DEBUG_CFG("wpa_isr: No adapter ptr!\n");
		WAN_IRQ_RETURN(WAN_IRQ_NONE);
	}
	/*WAN_ASSERT1(card->u.adsl.adapter == NULL);*/
	ret=adsl_isr((void*)card->u.adsl.adapter);
	DEBUG_ISR("%s(): ret: %d\n", __FUNCTION__, ret);
	if (ret) {
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);    
	}

	WAN_IRQ_RETURN(WAN_IRQ_NONE);
}

/*+F*************************************************************************
* Function:
*   adsl_lan_rx
*
* Description:
*   A new LAN receive packet is available. Create a SKB for the data, and
*   give it to the operating system.
*-F*************************************************************************/

void
adsl_lan_rx(
			void*		dev_id,
			void*		pHeader,
			unsigned long	headerSize,
			unsigned char*	rx_data,
			int			rx_len
			)
{
	netdevice_t* 		dev = (netdevice_t*)dev_id;
	sdla_t*			card = NULL;
	adsl_private_area_t*	adsl = (adsl_private_area_t*)wan_netif_priv(dev);
	netskb_t*		rx_skb = NULL;
	u_int32_t 		len;
	unsigned char		hdr[2] = { 0xFF, 0x03 };
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	struct ether_header*	eh;
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__) || (defined(__FreeBSD__) && __FreeBSD_version < 501000)
#ifndef __LINUX__
	wan_smp_flag_t			s;
#endif
#endif

	WAN_ASSERT1(dev == NULL);
	WAN_ASSERT1(adsl == NULL);
	card = (sdla_t*)adsl->common.card;
	DEBUG_RX("(RX INTR) pHeader, (%p)\n", pHeader);
	DEBUG_RX("(RX INTR) headerSize, %ld\n", headerSize);
	DEBUG_RX("(RX INTR) rx_len, %d\n", rx_len);

#if defined(__WINDOWS__)
	//DBG_ADSL_RX("(RX INTR) pHeader, (%p)\n", pHeader);		//for rfc 1483 always zero
	//DBG_ADSL_RX("(RX INTR) headerSize, %ld\n", headerSize);	//for rfc 1483 always zero
	DBG_ADSL_RX("(RX INTR) rx_len, %d\n", rx_len);

	len = rx_len;
#else
	len = headerSize + rx_len;
#endif
	if (len == 0) return;

#if defined(__WINDOWS__)
	rx_skb = wan_skb_alloc(len);
#else
	rx_skb = wan_skb_alloc(len+4);
#endif
	if (rx_skb == NULL){
		DEBUG_EVENT("%s: Failed allocate memory for RX packet!\n",
			wan_netif_name(dev));
		return;
	}

#if !defined(__WINDOWS__)
	wpabs_skb_reserve(rx_skb, 2);
	wpabs_skb_set_dev(rx_skb, dev);
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	eh = (struct ether_header*)wan_skb_data(rx_skb);
#endif

#if !defined(__WINDOWS__)
	if (card->u.adsl.EncapMode == RFC_MODE_PPP_VC	||
		card->u.adsl.EncapMode == RFC_MODE_PPP_LLC	||
		card->u.adsl.EncapMode == RFC_MODE_STACK_VC	){

			if (rx_data[0] != hdr[0] || rx_data[1] != hdr[1]){
				wpabs_skb_copyback(rx_skb,
					wan_skb_len(rx_skb), 
					2, 
# if defined(__LINUX__)
					(unsigned long)hdr);
# else
					(caddr_t)hdr);
# endif
			}
	}
#endif

	if (pHeader && headerSize){
		wpabs_skb_copyback(rx_skb, 
			wan_skb_len(rx_skb), 
			headerSize, 
#if defined(__LINUX__)
			(unsigned long)pHeader);
#else
			(caddr_t)pHeader);
#endif
	}

#if defined(__LINUX__)
	wpabs_skb_copyback(rx_skb, wan_skb_len(rx_skb), len, (unsigned long)rx_data);
#else
	wpabs_skb_copyback(rx_skb, wan_skb_len(rx_skb), len, (caddr_t)rx_data);
#endif


#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# if defined(__FreeBSD__) && (__FreeBSD_version < 500000)
	wan_bpf_report(dev, rx_skb, 0, WAN_BPF_DIR_IN);
# elif defined(__NetBSD__) || defined(__OpenBSD__)
	wan_bpf_report(dev, rx_skb, 0, WAN_BPF_DIR_IN);
# endif

	card->wandev.stats.rx_packets ++;
	card->wandev.stats.rx_bytes += wan_skb_len(rx_skb);

	switch(card->u.adsl.EncapMode){
	case RFC_MODE_STACK_VC:
		if (wanpipe_lip_rx(adsl,rx_skb) != 0){
			card->wandev.stats.rx_packets--;
			card->wandev.stats.rx_bytes -= wan_skb_len(rx_skb);
			wan_skb_free(rx_skb);
		}
		break;

	case RFC_MODE_PPP_VC:
	case RFC_MODE_PPP_LLC:
		wan_skb_clear_mark(rx_skb);
		WAN_SPPP_INPUT(dev, rx_skb);
		break;
	case RFC_MODE_ROUTED_IP_LLC:
	case RFC_MODE_ROUTED_IP_VC:	
		wan_skb_clear_mark(rx_skb);
#if (__FreeBSD_version >= 501000)
		wan_bpf_report(dev, rx_skb, 0, WAN_BPF_DIR_IN);
		netisr_queue(NETISR_IP, rx_skb);
#else
		wan_spin_lock_irq(NULL, &s);
		schednetisr(NETISR_IP);
		if (IF_QFULL(&ipintrq)){
			DEBUG_RX("%s: IP queue is full, drop packet!\n",
				card->devname);
			/* oh, no - IP queue is full - well - we'll
			** try again later 
			*/ 
			card->wandev.stats.rx_packets--;
			card->wandev.stats.rx_bytes -= wan_skb_len(rx_skb);

			IF_DROP(&ipintrq);
			wan_skb_free(rx_skb);
			wan_spin_unlock_irq(NULL, &s);
			break;
		}

		/* ok enqueue the packet */
		IF_ENQUEUE(&ipintrq, rx_skb);
		wan_spin_unlock_irq(NULL, &s);
#endif
		break;
	case RFC_MODE_BRIDGED_ETH_LLC:
	case RFC_MODE_BRIDGED_ETH_VC:
		wan_skb_clear_mark(rx_skb);
# if defined(__NetBSD__) || defined(__FreeBSD__) && (__FreeBSD_version > 500000)
		dev->if_input(dev, rx_skb);
# else
		wan_skb_pull(rx_skb, sizeof(struct ether_header));
		ether_input(dev, eh, rx_skb);
# endif
		break;
	}

#else /* LINUX, WINDOWS */

	card->wandev.stats.rx_packets ++;
	card->wandev.stats.rx_bytes += wan_skb_len(rx_skb);

	switch (card->u.adsl.EncapMode){

#if !defined(__WINDOWS__)
	case RFC_MODE_STACK_VC:
		if (wanpipe_lip_rx(adsl,rx_skb) != 0){
			card->wandev.stats.rx_packets--;
			card->wandev.stats.rx_bytes -= wan_skb_len(rx_skb);
			wan_skb_free(rx_skb);
		}
		break;
#endif

	case RFC_MODE_BRIDGED_ETH_LLC:
	case RFC_MODE_BRIDGED_ETH_VC:
#if defined(__WINDOWS__)
		netif_rx(dev, rx_skb);
#else
		rx_skb->protocol = eth_type_trans(rx_skb, rx_skb->dev);
		netif_rx(rx_skb);
#endif
		break;


	case RFC_MODE_ROUTED_IP_LLC:
	case RFC_MODE_ROUTED_IP_VC:	
#if defined(__WINDOWS__)
		netif_rx(dev, rx_skb);
#else
		rx_skb->protocol = htons(ETH_P_IP);
		__wan_skb_reset_mac_header(rx_skb);
		netif_rx(rx_skb);
#endif
		break;

#if !defined(__WINDOWS__)
	case RFC_MODE_PPP_VC:
	case RFC_MODE_PPP_LLC:
		rx_skb->protocol = htons(ETH_P_WAN_PPP);
		rx_skb->dev = dev;
		__wan_skb_reset_mac_header(rx_skb);
		wp_sppp_input(rx_skb->dev,rx_skb);
		break;
#endif
	}

#endif


	adsl_rx_complete(adsl->pAdapter);
}

/*+F*************************************************************************
* Function:
*   adsl_output
*
* Description:
*   Handle a LAN interface buffer (SKB) transmit request. Allocate an NDIS
*   buffer, copy the data into the NDIS packet, and give it to the RFC layer.
*-F*************************************************************************/
#if defined(__LINUX__) || defined(__WINDOWS__)
int adsl_output(netskb_t* skb, netdevice_t* dev)
#else
static int 
adsl_output(netdevice_t* dev, netskb_t* skb, struct sockaddr* dst, struct rtentry* rt0)
#endif
{
	sdla_t*			card = NULL;
	adsl_private_area_t*	adsl = wan_netif_priv(dev);
	int			status = 0;
#if defined(__LINUX__) || defined(__WINDOWS__)
	wan_smp_flag_t		smp_flags;
#endif
#if defined(ALTQ)
	WAN_PKTATTR_DECL(pktattr);
#endif

	if (!skb){
		WAN_NETIF_START_QUEUE(dev);
		return 0;
	}

	if (!adsl){
		DEBUG_ERROR("%s: Error: TxLan: No private adapter !\n",
			wan_netif_name(dev));
#if defined(__LINUX__) || defined(__WINDOWS__)
		wan_skb_free(skb);
#endif
		WAN_NETIF_START_QUEUE(dev);
		return 0;	
	}
	card = (sdla_t*)adsl->common.card;

#if !defined(__WINDOWS__)
	switch (card->u.adsl.EncapMode)
	{
	case RFC_MODE_PPP_VC:
	case RFC_MODE_PPP_LLC:
	case RFC_MODE_STACK_VC:
		if (wan_skb_len(skb) <= 2){
			DEBUG_ERROR("%s: Error: TxLan: PPP pkt len <= 2! (len=%i)\n",
					wan_netif_name(dev),wan_skb_len(skb));

			wan_skb_print(skb);	
			wan_skb_free(skb);
			WAN_NETIF_START_QUEUE(dev);
			card->wandev.stats.tx_errors++;
			return 0;
		}
		wan_skb_pull(skb,2);
		break;
	}/* switch() */
#endif

	if (adsl_can_tx(adsl->pAdapter)){
		wpabs_skb_free(skb);
		WAN_NETIF_START_QUEUE(dev);
		card->wandev.stats.tx_carrier_errors++;
#if defined(__LINUX__) || defined(__WINDOWS__)
		dev->trans_start = SYSTEM_TICKS;
		return 0;
#else
		return -EINVAL;
#endif
	}

	DEBUG_TX("%s: TxLan %d bytes...\n", 
		card->devname, wan_skb_len(skb));

#if 0
	DBG_ASSERT(skb->len < GSI_LAN_NDIS_BUFFER_SIZE);

	{
		int r;
		printk(KERN_INFO "TX RAW : %i: ",skb->len);
		//memcpy(skb->data,tmp_frame,sizeof(tmp_frame));
		for (r=0;r<skb->len;r++){
			printk("%02X ",skb->data[r]);
		}
		printk("\n");
	}
#endif

#if defined(__LINUX__)
	dev->trans_start = SYSTEM_TICKS;
	status = adsl_send(adsl->pAdapter, skb, 0);
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);	
	if (status == 1){
		WAN_NETIF_STOP_QUEUE(dev);
		wan_set_bit(TX_BUSY_SET,&card->wandev.critical);
		status = 1;
	}else{
		if (status == 2){
			card->wandev.stats.rx_dropped++;
		}
		wan_skb_free(skb);
		WAN_NETIF_START_QUEUE(dev);
		status = 0;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);	
#elif defined(__WINDOWS__)
	dev->trans_start = SYSTEM_TICKS;
	status = adsl_send(adsl->pAdapter, skb, 0);
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (status == 1){
		WAN_NETIF_STOP_QUEUE(dev);
		wan_set_bit(TX_BUSY_SET,&card->wandev.critical);
		status = 1;
	}else{
		if (status == 2){
			card->wandev.stats.tx_dropped++;
		}
		WAN_NETIF_START_QUEUE(dev);
		status = 0;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);	

	if(status == 0){
		/* free skb OUTSIDE of IRQ spinlock! */
		wpabs_skb_free(skb);
	}
#else
	if (dst->sa_family != AF_INET){
		DEBUG_EVENT("%s: Protocol family is not supported!\n", 
			card->devname); 
		wan_skb_free(skb);
		return -EAFNOSUPPORT;
	}

	if (card->wandev.state != WAN_CONNECTED){
		DEBUG_TX("%s: Device is not connected!\n", card->devname);
		wan_skb_free(skb);
		return -EINVAL;
	}

	switch (card->u.adsl.EncapMode){
	case RFC_MODE_ROUTED_IP_LLC:
	case RFC_MODE_ROUTED_IP_VC:	
		/* classify the packet before prepanding link-headers */
		WAN_IFQ_CLASSIFY(&dev->if_snd, skb, dst->sa_family, &pktattr);
		WAN_IFQ_ENQUEUE(&dev->if_snd, skb, &pktattr, status);
		if (status){
			DEBUG_TX("%s: Send queue is full!\n",
				card->devname);
			wan_skb_free(skb);
			break;
		}

		if ((dev->if_flags & IFF_DRV_OACTIVE) == 0){ 
			adsl_tx(dev);
		}

		break;
	default:
# if defined(__NetBSD__)
		status = dev->if_output(dev, skb, dst, rt0); 
# else
		status = ether_output(dev, skb, dst, rt0); 
# endif
		break;
	}
#endif
	return status;
}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
/*+F*************************************************************************
* Function:
*   adsl_sppp_tx
*
* Description:
*   Handle a LAN interface buffer (SKB) transmit request. Allocate an NDIS
*   buffer, copy the data into the NDIS packet, and give it to the RFC layer.
*-F*************************************************************************/
void adsl_sppp_tx (netdevice_t *ifp)
{
	netskb_t*		tx_mbuf = NULL;
	adsl_private_area_t*	adsl = wan_netif_priv(ifp);

	WAN_ASSERT1(ifp == NULL);
	WAN_ASSERT1(adsl == NULL);
	if (adsl_can_tx(adsl->pAdapter)){
		return;
	}

	while((tx_mbuf = WAN_SPPP_DEQUEUE(ifp)) != NULL){
#if 0
		if (wan_skb_check(tx_mbuf)){
#endif
			if (wan_skb2buffer((void**)&tx_mbuf)){
				DEBUG_EVENT("%s: Failed to correct mbuf list!\n",
					wan_netif_name(ifp));
				wan_skb_free(tx_mbuf);
				continue;
			}
#if 0
		}
#endif
		DEBUG_TX("%s: TxLan %d bytes...\n", 
			wan_netif_name(ifp), 
			wan_skb_len(tx_mbuf));
		wan_bpf_report(ifp, tx_mbuf, 0, WAN_BPF_DIR_OUT);
		wan_skb_pull(tx_mbuf, 2);	
		if (adsl_send(adsl->pAdapter, tx_mbuf, 0)){
			DEBUG_TX("%s: TX failed to send %d bytes!\n", 
				wan_netif_name(ifp), 
				wan_skb_len(tx_mbuf));
			ifp->if_iqdrops++;
		}

		if (tx_mbuf){
			wan_skb_free(tx_mbuf);
		}
	}

	return;
}

/*+F*************************************************************************
* Function:
*   adsl_tx
*
* Description:
*   Handle a LAN interface buffer (SKB) transmit request. Allocate an NDIS
*   buffer, copy the data into the NDIS packet, and give it to the RFC layer.
*-F*************************************************************************/
void adsl_tx (netdevice_t *ifp)
{
	netskb_t*		tx_mbuf = NULL;
	adsl_private_area_t*	adsl = wan_netif_priv(ifp);
#if 0
	sdla_t*			card = NULL;
#endif

	WAN_ASSERT1(ifp == NULL);
	WAN_ASSERT1(adsl == NULL);
	WAN_ASSERT1(adsl->common.card == NULL);

#if defined(NETGRAPH)
#if 0
	card = adsl->card;
	while(!WAN_IFQ_IS_EMPTY(&card->xmitq_hipri)){
		WAN_IFQ_POLL(&card->xmitq_hipri, tx_mbuf);
		if (tx_mbuf == NULL) break;
		DEBUG_TX("%s: TxLan %d bytes...\n",
			wan_netif_name(ifp), wan_skb_len(tx_mbuf));
		if (adsl_send(adsl->pAdapter, tx_mbuf, 0)){
			DEBUG_TX("%s: TX failed to send %d bytes!\n",
				wan_netif_name(ifp), wan_skb_len(tx_mbuf));
		}

		WAN_IFQ_DEQUEUE(&card->xmitq_hipri, tx_mbuf);
		if (tx_mbuf){
			wan_skb_free(tx_mbuf);
		}
	}
#endif
#endif
	while(!WAN_IFQ_IS_EMPTY(&ifp->if_snd)){

		WAN_IFQ_DEQUEUE(&ifp->if_snd, tx_mbuf);
		if (tx_mbuf == NULL) break;

#if 0
		if (wan_skb_check(tx_mbuf)){
#endif
			if (wan_skb2buffer((void**)&tx_mbuf)){
				DEBUG_EVENT("%s: Failed to correct mbuf list!\n",
					wan_netif_name(ifp));
				wan_skb_free(tx_mbuf);
				continue;
			}
#if 0
		}
#endif

		DEBUG_TX("%s: TxLan %d bytes...\n", 
			wan_netif_name(ifp), 
			wan_skb_len(tx_mbuf));
		if (adsl_send(adsl->pAdapter, tx_mbuf, 0)){
			DEBUG_TX("%s: TX failed to send %d bytes!\n", 
				wan_netif_name(ifp), 
				wan_skb_len(tx_mbuf));
			ifp->if_iqdrops++;
		}

		wan_bpf_report(ifp, tx_mbuf, 0, WAN_BPF_DIR_OUT);

		if (tx_mbuf){
			wan_skb_free(tx_mbuf);
		}
	}
	return;
}
#endif

/*+F*************************************************************************
* Function:
*   aadsl_tx_complete
*
* Description:
*-F*************************************************************************/
void adsl_tx_complete(void* dev_id, int length, int txStatus)
{
	netdevice_t*		ifp = (netdevice_t*)dev_id;
	adsl_private_area_t*	adsl;
	sdla_t*			card = NULL;

	WAN_ASSERT1(ifp == NULL);

	adsl = wan_netif_priv(ifp);
	WAN_ASSERT1(adsl == NULL);	

	card = (sdla_t*)adsl->common.card;
	WAN_ASSERT1(card == NULL);

	if (txStatus == 0){
		DEBUG_TX("%s: TxLan tx successful.\n", 
			wan_netif_name(ifp));
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		ifp->if_opackets++;
		ifp->if_obytes += length;
#endif
		card->wandev.stats.tx_packets ++;
		card->wandev.stats.tx_bytes += length;
	}else{
		DEBUG_TX("%s: TxLan tx failure.\n", 
			wan_netif_name(ifp));
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		ifp->if_oerrors++;
#endif
	}

#if defined(__LINUX__) || defined(__WINDOWS__)
	ifp->trans_start = SYSTEM_TICKS;
#endif

#if defined(__WINDOWS__)
	wan_clear_bit(TX_BUSY_SET,&card->wandev.critical);
	WAN_NETIF_WAKE_QUEUE(ifp);
#else
	if (WAN_NETIF_QUEUE_STOPPED(ifp) ||
		wan_test_bit(TX_BUSY_SET,&card->wandev.critical)){

			wan_clear_bit(TX_BUSY_SET,&card->wandev.critical);
			DEBUG_TX("%s: adsl_tx_complete, waking dev %s\n",
				card->devname, wan_netif_name(ifp));
			WAN_NETIF_WAKE_QUEUE(ifp);
			if (adsl->common.usedby == STACK){
				wanpipe_lip_kick(adsl,0);
			}
	}
#endif
	return ;
}

#if defined(__LINUX__)
/*+F*************************************************************************
* Function:
*   adsl_multicast
*
* Description:
*   Handle the LAN subsystem multicast, broadcast, and promiscous updates.
*   These values can be modified by using the ifconfig command or other
*   applications like network sniffers.
*-F*************************************************************************/
static void adsl_multicast(netdevice_t* dev)
{
	adsl_private_area_t*	adsl = wan_netif_priv(dev);
	short			flags = wan_netif_flags(dev);
	int			mcount = wan_netif_mcount(dev);
	char*			mcaddrs = NULL;

	WAN_ASSERT1(adsl == NULL);

	DEBUG_TX("%s:  IF Flags: %s%s%s%s!\n",
		adsl->if_name,
		(flags & IFF_BROADCAST)	? "Broadcast "	: "",
		(flags & IFF_PROMISC)	? "Promiscous "	: "",
		(flags & IFF_MULTICAST)	? "Multicast "	: "",
		(flags & IFF_ALLMULTI)	? "All-Multicast" : "");

	DEBUG_TX("%s: HwAddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
		adsl->if_name,
		adsl->macAddr[0], adsl->macAddr[1], 
		adsl->macAddr[2], adsl->macAddr[3],
		adsl->macAddr[4], adsl->macAddr[5]);

	if ((flags & IFF_MULTICAST) && mcount != 0){
#if !defined(__OpenBSD__)
# if defined(__FreeBSD__)
		struct ifmultiaddr*	mclist = NULL;
# else
		int x = 0;
#ifdef CONFIG_RPS
		struct netdev_hw_addr*	mclist = NULL;
		netdev_for_each_mc_addr(mclist, dev) {
		x++;
#else
		struct dev_mc_list* mclist = NULL;
		for (mclist = LIST_FIRST_MCLIST(dev);mclist != NULL;x++, mclist = LIST_NEXT_MCLIST(mclist)){
#endif
# endif
		mcaddrs = wan_malloc(mcount * 6);
# if defined(__FreeBSD__)
				memcpy(&(mcaddrs[x * 6]), (void*)mclist->ifma_addr, 6);
# else
				memcpy(&(mcaddrs[x * 6]), WAN_MC_LIST_ADDR(mclist), 6);
# endif
		}
#endif
	}

	/*adsl_lan_multicast(adsl->pAdapter, flags, mcaddrs, mcount);
	*/

	if (mcaddrs){
		wan_free(mcaddrs);
	}
	return;
}


/****************************************************************************
* Function:
*   GpLanStats
*
* Description:
*   Return the current LAN device statistics.
*
* Note: For 2.6 kernels we are not allowed to return NULL
*-F*************************************************************************/
static struct net_device_stats gstats;
static struct net_device_stats* adsl_stats(netdevice_t* dev)
{
	sdla_t*			card = NULL;
	adsl_private_area_t*	adsl = wan_netif_priv(dev);

	if (adsl == NULL){
		return &gstats;
	}

	card = (sdla_t*)adsl->common.card;

	if (card == NULL){
		return &gstats;
	}

	return &card->wandev.stats;
}
#elif defined(__WINDOWS__)
static struct net_device_stats gstats;
static struct net_device_stats* adsl_stats(netdevice_t* dev)
{
	sdla_t*			card = NULL;
	adsl_private_area_t*	adsl = wan_netif_priv(dev);

	if (adsl == NULL){
		return &gstats;
	}

	card = (sdla_t*)adsl->common.card;

	if (card == NULL){
		return &gstats;
	}

	return &card->wandev.stats;
}

#endif

/*+F*************************************************************************
* Function:
*   adsl_tracing_enabled
*
* Description:
*
*
*-F*************************************************************************/
int adsl_tracing_enabled(void *trace_ptr)
{
	adsl_trace_info_t *trace_info = (adsl_trace_info_t*)trace_ptr;

	WAN_ASSERT(trace_info == NULL);
	if (wan_test_bit(0,&trace_info->tracing_enabled)){

		if ((SYSTEM_TICKS - trace_info->trace_timeout) > MAX_TRACE_TIMEOUT){
			DEBUG_EVENT("wanpipe: Disabling trace, timeout!\n");
			wan_clear_bit(0,&trace_info->tracing_enabled);
			wan_clear_bit(1,&trace_info->tracing_enabled);
			return -EINVAL;
		}

		if (adsl_trace_queue_len(trace_info) < trace_info->max_trace_queue){
			if (wan_test_bit(1,&trace_info->tracing_enabled)){
				return 1;
			}else if (wan_test_bit(2,&trace_info->tracing_enabled)){
				return 2;
			}else{
				return 0;
			}
		}

		DEBUG_UDP("wanpipe: Too many packet in trace queue %d (max=%d)!\n",
			adsl_trace_queue_len(trace_info),
			trace_info->max_trace_queue);
		return -ENOBUFS;
	}
	return -EINVAL;
}

void* adsl_trace_info_alloc(void)
{
	return wan_malloc(sizeof(adsl_trace_info_t));
}

void adsl_trace_info_init(void *trace_ptr)
{
	adsl_trace_info_t*	trace = (adsl_trace_info_t *)trace_ptr;
	wan_skb_queue_t*	trace_queue = NULL;

	trace_queue = &trace->trace_queue;
	WAN_IFQ_INIT(trace_queue, MAX_TRACE_QUEUE);
	trace->trace_timeout	= SYSTEM_TICKS;
	trace->tracing_enabled	= 0;
	trace->max_trace_queue	= MAX_TRACE_QUEUE;
}

int adsl_trace_purge (void *trace_ptr)
{
	adsl_trace_info_t*	trace = (adsl_trace_info_t *)trace_ptr;
	wan_skb_queue_t*	trace_queue = NULL;

	WAN_ASSERT(trace == NULL);
	trace_queue = &trace->trace_queue;
	WAN_IFQ_PURGE(trace_queue);
	return 0;
}

int adsl_trace_enqueue(void *trace_ptr, void *skb_ptr)
{
	adsl_trace_info_t*	trace = (adsl_trace_info_t *)trace_ptr;
	wan_skb_queue_t*	trace_queue = NULL;
	netskb_t*		skb = (netskb_t*)skb_ptr;
	int			err = 0;

	WAN_ASSERT(trace == NULL);
	trace_queue = &trace->trace_queue;
	WAN_IFQ_ENQUEUE(trace_queue, skb, NULL, err);
	return err;
}

unsigned int adsl_trace_queue_len(void *trace_ptr)
{
	adsl_trace_info_t*	trace = (adsl_trace_info_t *)trace_ptr;
	wan_skb_queue_t*	trace_queue = NULL;

	WAN_ASSERT(trace == NULL);
	trace_queue = &trace->trace_queue;
	return WAN_IFQ_LEN(trace_queue);
}

/*+F*************************************************************************
* Function:
*   adsl_ioctl
*
* Description:
*
*
*-F*************************************************************************/
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static int adsl_ioctl(netdevice_t* ifp, u_long command, caddr_t data)
#else
static int adsl_ioctl(netdevice_t* ifp, struct ifreq *ifr, int command)
#endif
{
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	struct ifreq*		ifr = (struct ifreq*)data;
	struct ifmediareq*	ifmr = (struct ifmediareq*)data;
	struct ifaddr*	      	ifa = (struct ifaddr *)data;
# if defined(__FreeBSD__)
	struct ifstat*		ifs = (struct ifstat *)data;
# endif
#endif
	adsl_private_area_t*	adsl = wan_netif_priv(ifp);
	wan_udp_pkt_t*		wan_udp_pkt = NULL;
	sdla_t *card;
	int 		      	error = 0; 

	if (!wan_netif_up(ifp)) {
		DEBUG_ERROR("%s: Error: ADSL Device is not up\n",
				wan_netif_name(ifp));
		return -ENETDOWN;
	}

	if (!adsl || !adsl->common.card ) {
		DEBUG_ERROR("%s: Error: ADSL Device is not configured\n",
				wan_netif_name(ifp));
		return -EFAULT;
	}

	card = (sdla_t*)adsl->common.card;

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	wan_smp_flag_t	s;
	wan_spin_lock_irq(NULL, &s);
#endif

	switch (command){ 
#if !defined(__WINDOWS__)
		case SIOC_WANPIPE_PIPEMON:
			DEBUG_IOCTL("%s - ioctl(WANPIPE_PIPEMON) called.\n", card->devname);
			if (wan_atomic_read(&adsl->udp_pkt_len) != 0){
				error = -EBUSY;
				goto adsl_ioctl_done;
			}

			wan_atomic_set(&adsl->udp_pkt_len,sizeof(wan_udp_hdr_t));

			/* For performance reasons test the critical
			* here before spin lock */
			if (wan_test_bit(0,&card->in_isr)){
				wan_atomic_set(&adsl->udp_pkt_len,0);
				error = -EBUSY;
				goto adsl_ioctl_done;
			}

			wan_udp_pkt=(wan_udp_pkt_t*)&adsl->udp_pkt_data[0];
			if (WAN_COPY_FROM_USER(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				DEBUG_ERROR("%s: Error: Failed to copy memory from USER space!\n",
					card->devname);
				wan_atomic_set(&adsl->udp_pkt_len,0);
				error = -EFAULT;
				goto adsl_ioctl_done;
			}

			error = process_udp_cmd(ifp, &wan_udp_pkt->wan_udp_hdr);

			/* This area will still be critical to other
			* PIPEMON commands due to udp_pkt_len
			* thus we can release the irq */

			if (wan_atomic_read(&adsl->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				DEBUG_ERROR("%s: Error: Pipemon buf too big on the way up! %i\n",
					card->devname,wan_atomic_read(&adsl->udp_pkt_len));
				wan_atomic_set(&adsl->udp_pkt_len,0);
				error = -EINVAL;
				goto adsl_ioctl_done;
			}

			if (WAN_COPY_TO_USER(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				DEBUG_ERROR("%s: Error: Failed to copy memory to USER space!\n",
					card->devname);
				wan_atomic_set(&adsl->udp_pkt_len,0);
				error = -EFAULT;
				goto adsl_ioctl_done;
			}

			wan_atomic_set(&adsl->udp_pkt_len,0);
			break;

#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		case SIOCGIFMEDIA:
			switch (card->u.adsl.EncapMode){
		case RFC_MODE_BRIDGED_ETH_LLC:	
		case RFC_MODE_BRIDGED_ETH_VC:
			ifmr->ifm_current = IFM_ETHER;
			break;
		default:
			error = -EOPNOTSUPP;
			goto adsl_ioctl_done;
			}
			ifmr->ifm_active = ifmr->ifm_current;
			ifmr->ifm_mask = 0;
			ifmr->ifm_status = 0;
			ifmr->ifm_count = 1;
			break;

# if defined(__FreeBSD__)
		case SIOCGIFSTATUS:
			sprintf(ifs->ascii + strlen(ifs->ascii),
				"\tstatus: %s\n", wanpipe_get_state_string(card));
			break;
# endif
		case SIOCSIFMTU:
			ifp->if_mtu = ifr->ifr_mtu;
			break;

		case SIOCSIFADDR:
		case SIOCGIFADDR:
			if (card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_LLC || 
				card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_VC){
					error = ETHER_IOCTL(ifp, command, data);
					break;
			} else if (card->u.adsl.EncapMode == RFC_MODE_ROUTED_IP_LLC || 
				card->u.adsl.EncapMode == RFC_MODE_ROUTED_IP_VC){
					if (ifa->ifa_addr->sa_family == AF_INET){
						break;
					}
			}
			/* For all other modes fall through */
#endif

		default:
			if (card->u.adsl.EncapMode == RFC_MODE_PPP_VC ||
				card->u.adsl.EncapMode == RFC_MODE_PPP_LLC){
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
					error = WAN_SPPP_IOCTL(ifp, command, data);
#elif defined(__LINUX__)
					error = wp_sppp_do_ioctl(ifp,ifr,command);
#elif defined(__WINDOWS__)
					error = -EOPNOTSUPP;
#endif
			}else if (card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_LLC || 
				card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_VC){
#if defined(__LINUX__)
					error = ETHER_IOCTL(ifp, command, data);
#else
					error = -EOPNOTSUPP;
#endif
			}else{
				error = EINVAL;
			}
			break; /* NEW */
	}

#if !defined(__WINDOWS__)
adsl_ioctl_done:
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	wan_spin_unlock_irq(NULL, &s);
#endif
	/* done */
	return error;
}

#if defined(__LINUX__)
/* FIXME update for BSD */

# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))  
static void process_bh (struct work_struct *work)
#else
static void process_bh (void *dev_ptr)
#endif  
{
	netdevice_t		*dev;
	adsl_private_area_t	*adsl;
	sdla_t*			card;

# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	adsl = container_of(work, adsl_private_area_t, common.wanpipe_task);
	dev = adsl->common.dev;
	if (!dev) {
		return;
	}  
#else
	dev=dev_ptr;
	if (!dev || (adsl = wan_netif_priv(dev)) == NULL){
		return;
	}
#endif
	card = adsl->common.card;  
             
	process_udp_mgmt_pkt(card, dev, adsl,0);
}
#elif defined(__WINDOWS__)
VOID process_bh(
	IN PKDPC	Dpc, 
	IN PVOID	arg, 
	IN PVOID	not_used1, 
	IN PVOID	not_used2
	)
{
	DBG_DSL_NOT_IMPLD();
}
#endif

#if defined(__WINDOWS__)
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* netdev, adsl_private_area_t* chan)
{
	wan_udp_pkt_t *wan_udp_pkt;
	wan_time_t tv;
	int	rc = SANG_STATUS_SUCCESS;

	wan_udp_pkt = (wan_udp_pkt_t*)chan->udp_pkt_data;

	wan_udp_pkt->wan_udp_opp_flag = 0;
	
	switch(wan_udp_pkt->wan_udp_command) {
		
	case READ_CODE_VERSION:
		wpabs_memcpy(wan_udp_pkt->wan_udp_data, &drv_version, sizeof(DRIVER_VERSION));
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = sizeof(DRIVER_VERSION);
		break;

	case GET_OPEN_HANDLES_COUNTER:
		*(int*)&wan_udp_pkt->wan_udp_data[0] = wp_get_netdev_open_handles_count(netdev);
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = sizeof(int);
		break;

	case ROUTER_UP_TIME:
		wan_getcurrenttime( &tv, NULL );

		chan->router_up_time = tv - chan->router_start_time;
		*(wan_time_t *)&wan_udp_pkt->wan_udp_data = 
			chan->router_up_time;	
		wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
		wan_udp_pkt->wan_udp_return_code = 0;
		break;
	
	case READ_OPERATIONAL_STATS:
		DEBUG_UDP("%s: READ_OPERATIONAL_STATS\n",	netdev->name);

		wpabs_memcpy(wan_udp_pkt->wan_udp_data, &chan->if_stats, sizeof(net_device_stats_t));
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = sizeof(net_device_stats_t);
		break;

	case FLUSH_OPERATIONAL_STATS:
		DEBUG_UDP("%s: FLUSH_OPERATIONAL_STATS\n",	netdev->name);

		memset(&chan->if_stats, 0x00, sizeof(net_device_stats_t));
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = sizeof(net_device_stats_t);
		break;

	case WAN_GET_MEDIA_TYPE:
		wan_udp_pkt->wan_udp_data[0] = WAN_MEDIA_NONE;
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = sizeof(unsigned char); 
		break;

	case WAN_GET_PROTOCOL:
		DEBUG_UDP("%s: WAN_GET_PROTOCOL\n",	netdev->name);

		wan_udp_pkt->wan_udp_data[0] = (unsigned char)card->wandev.config_id;
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = 1;
		break;
		
	case WAN_GET_PLATFORM:
		wan_udp_pkt->wan_udp_data[0] = WAN_WIN2K_PLATFORM;
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = 1;
		break;

	case WAN_GET_HW_MAC_ADDR:
		DEBUG_UDP("WAN_GET_HW_MAC_ADDR\n");
#if 0
		/* Some S518 cards have duplicate MAC addresses - can NOT use them
		 * on Bridged networks. Use randomly generated MAC address instead. */
		wan_get_random_mac_address(wan_udp_pkt->wan_udp_data);
#else
		{
			int i;
			for(i = 0; i < ETHER_ADDR_LEN; i++){
				wan_udp_pkt->wan_udp_data[i] = chan->macAddr[i];
			}
		}
#endif
		wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
		wan_udp_pkt->wan_udp_data_len = ETHER_ADDR_LEN;
		break;

	case WANPIPEMON_GET_BIOS_ENCLOSURE3_SERIAL_NUMBER:
		wan_udp_pkt->wan_udp_return_code = 
			wp_get_motherboard_enclosure_serial_number(wan_udp_pkt->wan_udp_data, 
						sizeof(wan_udp_pkt->wan_udp_data));

		wan_udp_pkt->wan_udp_data_len = sizeof(wan_udp_pkt->wan_udp_data);
		break;

	default:
		wan_udp_pkt->wan_udp_data_len = 0;
		wan_udp_pkt->wan_udp_return_code = 0xCD;
		DEBUG_WARNING("%s(): %s: Warning: Unknown Management command: 0x%X\n",
			__FUNCTION__, netdev->name, wan_udp_pkt->wan_udp_command);
		break;
	}/* switch () */

	return rc;
}

int adsl_wan_user_process_udp_mgmt_pkt(void* card_ptr, void* chan_ptr, void *udata)
{
	sdla_t *card = (sdla_t *)card_ptr;
	adsl_private_area_t *adsl = (adsl_private_area_t*)chan_ptr;
	wan_udp_pkt_t *wan_udp_pkt;
	
	DEBUG_UDP("%s(): line: %d\n", __FUNCTION__, __LINE__);

	if (wan_atomic_read(&adsl->udp_pkt_len) != 0){
		return -EBUSY;
	}

	wan_atomic_set(&adsl->udp_pkt_len, MAX_LGTH_UDP_MGNT_PKT);

	wan_udp_pkt=(wan_udp_pkt_t*)adsl->udp_pkt_data;

	/* udata IS a pointer to wan_udp_hdr_t. copy data from user's buffer */
	wpabs_memcpy(&wan_udp_pkt->wan_udp_hdr, udata, sizeof(wan_udp_hdr_t));


	process_udp_mgmt_pkt(card, adsl->common.dev, adsl);


	/* udata IS a pointer to wan_udp_hdr_t. copy data into user's buffer */
	wpabs_memcpy(udata, &wan_udp_pkt->wan_udp_hdr, sizeof(wan_udp_hdr_t));

	wan_atomic_set(&adsl->udp_pkt_len,0);

	return 0;
}
#else
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
								adsl_private_area_t* adsl, int local_dev) 
{
	wan_udp_pkt_t*	wan_udp_pkt = NULL;
	netskb_t*	new_skb = NULL;
	int 		error = 0;
	int 		len = 0;

	wan_udp_pkt = (wan_udp_pkt_t*)adsl->udp_pkt_data;

	error = process_udp_cmd(dev, &wan_udp_pkt->wan_udp_hdr);	

	wan_udp_pkt->wan_ip_ttl = card->wandev.ttl; 

	if (local_dev){
		return 0;
	}

	len = wan_reply_udp(card, adsl->udp_pkt_data, wan_udp_pkt->wan_udp_data_len);

	if (card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_LLC ||
		card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_VC){
			len += sizeof(ethhdr_t);
	}

	new_skb = wan_skb_alloc(len);
	if (new_skb  != NULL){

		wan_skb_set_dev(new_skb, dev);
		if (card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_LLC ||
			card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_VC){
				unsigned short	ether_type = 0x0008;

				if (adsl->udp_pkt_src == UDP_PKT_FRM_NETWORK){
					wan_skb_copyback(new_skb, 
						wan_skb_len(new_skb),
						ETHER_ADDR_LEN,
						&adsl->macAddr[0]);
					wan_skb_copyback(new_skb,
						wan_skb_len(new_skb),
						ETHER_ADDR_LEN,
						&adsl->remote_eth_addr[0]);
				}else{
					wan_skb_copyback(new_skb,
						wan_skb_len(new_skb),
						ETHER_ADDR_LEN,
						&adsl->remote_eth_addr[0]);
					wan_skb_copyback(new_skb, 
						wan_skb_len(new_skb),
						ETHER_ADDR_LEN,
						&adsl->macAddr[0]);
				}
				wan_skb_copyback(new_skb, 
					wan_skb_len(new_skb),
					sizeof(ether_type),
					(caddr_t)&ether_type);
				len -= sizeof(ethhdr_t);
		}

		/* copy data into new_skb */
		wan_skb_copyback(new_skb, 
			wan_skb_len(new_skb),
			len,
			adsl->udp_pkt_data);
#if defined(__LINUX__)
		if (card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_LLC ||
			card->u.adsl.EncapMode == RFC_MODE_BRIDGED_ETH_VC){
				new_skb->protocol = eth_type_trans(new_skb, new_skb->dev);
		}else{
			/* Decapsulate pkt and pass it up the protocol stack */
			new_skb->protocol = htons(ETH_P_IP);
			__wan_skb_reset_mac_header(new_skb);
		}
		if (adsl->udp_pkt_src == UDP_PKT_FRM_NETWORK){
			dev_queue_xmit(new_skb);
		}else{
			netif_rx(new_skb);
		}
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		if (adsl->udp_pkt_src == UDP_PKT_FRM_NETWORK){
#if defined(__FreeBSD__)
			ether_output_frame(dev, new_skb);
#else
			wan_smp_flag_t	s;
			wan_spin_lock_irq(&card->wandev.lock, &s);
			WAN_IFQ_ENQUEUE(&dev->if_snd, new_skb, NULL, error);
			if (!error){
				dev->if_obytes += wan_skb_len(new_skb) + sizeof(ethhdr_t);
				if ((dev->if_flags & IFF_DRV_OACTIVE) == 0){
					(*dev->if_start)(dev);
				}
			}
			wan_spin_unlock_irq(&card->wandev.lock, &s);
#endif
		}else{
# if defined(__OpenBSD__) || defined(__FreeBSD__) && (__FreeBSD_version <= 500000)
			ethhdr_t*	eh = (ethhdr_t*)wan_skb_data(new_skb);
# endif

# if defined(__FreeBSD__) && (__FreeBSD_version < 500000)
			wan_bpf_report(dev, new_skb, 0, WAN_BPF_DIR_IN);
# elif defined(__NetBSD__) || defined(__OpenBSD__)
			wan_bpf_report(dev, new_skb, 0, WAN_BPF_DIR_IN);
# endif
# if defined(__NetBSD__) || defined(__FreeBSD__) && (__FreeBSD_version > 500000)
			dev->if_input(dev, new_skb);
# else
			wan_skb_pull(new_skb, sizeof(ethhdr_t));
			ether_input(dev, eh, new_skb);
# endif
		}
#endif

	}else{

		DEBUG_EVENT("%s: no socket buffers available!\n",
			card->devname);
	}

	wan_atomic_set(&adsl->udp_pkt_len,0);
	return error;
}

/*
*
*
*/
static int process_udp_cmd(netdevice_t* ifp, wan_udp_hdr_t* udp_hdr)
{
	netskb_t* 		skb = NULL;
	adsl_private_area_t*	adsl = wan_netif_priv(ifp);
	sdla_t* 		card = adsl->common.card;
	int 			buffer_length=0;
	adsl_trace_info_t *trace_info = adsl_get_trace_ptr(adsl->pAdapter);


	udp_hdr->wan_udphdr_return_code=0;
	if (card->u.adsl.adapter == NULL){
		return 0;
	}
	switch(udp_hdr->wan_udphdr_command){
	case WAN_GET_PROTOCOL:
		udp_hdr->wan_udphdr_data[0] = (unsigned char)card->wandev.config_id;
        udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
        udp_hdr->wan_udphdr_data_len = 1;
		break;

	case WAN_GET_PLATFORM:
		udp_hdr->wan_udphdr_data[0] = WAN_FREEBSD_PLATFORM;
		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		udp_hdr->wan_udphdr_data_len = 1;
		break;

	case ADSL_TEST_DRIVER_RESPONSE:
		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		break;

	case ADSL_READ_DRIVER_VERSION:
		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		sprintf(udp_hdr->wan_udphdr_data, "%s", WANPIPE_VERSION);
		udp_hdr->wan_udphdr_data_len = (unsigned short)strlen(udp_hdr->wan_udphdr_data);
		break;

	case ADSL_ROUTER_UP_TIME:
		wan_getcurrenttime(&adsl->router_up_time, NULL);
		adsl->router_up_time -= adsl->router_start_time;
		*(unsigned long *)&udp_hdr->wan_udphdr_data = 
			adsl->router_up_time;	
		udp_hdr->wan_udphdr_data_len = sizeof(unsigned long);
		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		break;

	case ADSL_ENABLE_TRACING:

		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		udp_hdr->wan_udphdr_data_len = 0;

		if (!wan_test_bit(0,&trace_info->tracing_enabled)){

			trace_info->trace_timeout = SYSTEM_TICKS;

			adsl_trace_purge(trace_info);

			if (udp_hdr->wan_udphdr_data[0] == 0){
				wan_clear_bit(1,&trace_info->tracing_enabled);
				DEBUG_UDP("%s: ADSL L3 trace enabled!\n",
					card->devname);
			}else if (udp_hdr->wan_udphdr_data[0] == 1){
				wan_clear_bit(2,&trace_info->tracing_enabled);
				wan_set_bit(1,&trace_info->tracing_enabled);
				DEBUG_UDP("%s: ADSL L2 trace enabled!\n",
					card->devname);
			}else{
				wan_clear_bit(1,&trace_info->tracing_enabled);
				wan_set_bit(2,&trace_info->tracing_enabled);
				DEBUG_UDP("%s: ADSL L1 trace enabled!\n",
					card->devname);
			}
			wan_set_bit (0,&trace_info->tracing_enabled);

		}else{
			DEBUG_ERROR("%s: Error: ADSL trace running!\n",
				card->devname);
			udp_hdr->wan_udphdr_return_code = 2;
		}

		break;

	case ADSL_DISABLE_TRACING:

		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;

		if(wan_test_bit(0,&trace_info->tracing_enabled)) {

			wan_clear_bit(0,&trace_info->tracing_enabled);
			wan_clear_bit(1,&trace_info->tracing_enabled);
			wan_clear_bit(2,&trace_info->tracing_enabled);
			adsl_trace_purge(trace_info);
			DEBUG_UDP("%s: Disabling ADSL trace\n",
				card->devname);

		}else{
			/* set return code to line trace already 
			disabled */
			udp_hdr->wan_udphdr_return_code = 1;
		}

		break;

	case ADSL_GET_TRACE_INFO:

		if(wan_test_bit(0,&trace_info->tracing_enabled)){
			trace_info->trace_timeout = SYSTEM_TICKS;
		}else{
			DEBUG_ERROR("%s: Error ADSL trace not enabled\n",
				card->devname);
			/* set return code */
			udp_hdr->wan_udphdr_return_code = 1;
			break;
		}

		buffer_length = 0;
		udp_hdr->wan_udphdr_adsl_num_frames = 0;	
		udp_hdr->wan_udphdr_adsl_ismoredata = 0;

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		while (adsl_trace_queue_len(trace_info)){
			WAN_IFQ_POLL(&trace_info->trace_queue, skb);
			if (skb == NULL){	
				DEBUG_EVENT("%s: No more trace packets in trace queue!\n",
					card->devname);
				break;
			}
			if ((WAN_MAX_DATA_SIZE - buffer_length) < skb->m_pkthdr.len){
				/* indicate there are more frames on board & exit */
				udp_hdr->wan_udphdr_adsl_ismoredata = 0x01;
				break;
			}

			m_copydata(skb, 
				0, 
				skb->m_pkthdr.len, 
				&udp_hdr->wan_udphdr_data[buffer_length]);
			buffer_length += skb->m_pkthdr.len;
			WAN_IFQ_DEQUEUE(&trace_info->trace_queue, skb);
			if (skb){
				wan_skb_free(skb);
			}
			udp_hdr->wan_udphdr_adsl_num_frames++;
		}
#elif defined(__LINUX__)
		while ((skb=skb_dequeue(&trace_info->trace_queue)) != NULL){

			if((MAX_TRACE_BUFFER - buffer_length) < wan_skb_len(skb)){
				/* indicate there are more frames on board & exit */
				udp_hdr->wan_udphdr_adsl_ismoredata = 0x01;
				if (buffer_length != 0){
					skb_queue_head(&trace_info->trace_queue, skb);
				}else{
					memcpy(&udp_hdr->wan_udphdr_adsl_data[buffer_length], 
						wan_skb_data(skb),
						sizeof(wan_trace_pkt_t));

					buffer_length = sizeof(wan_trace_pkt_t);
					udp_hdr->wan_udphdr_adsl_num_frames++;
					wan_skb_free(skb);	
				}
				break;
			}

			memcpy(&udp_hdr->wan_udphdr_adsl_data[buffer_length], 
				wan_skb_data(skb),
				wan_skb_len(skb));

			buffer_length += wan_skb_len(skb);
			wan_skb_free(skb);
			udp_hdr->wan_udphdr_adsl_num_frames++;
		}
#endif                      
		/* set the data length and return code */
		udp_hdr->wan_udphdr_data_len = (unsigned short)buffer_length;
		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		break;

	default:
		adsl_udp_cmd(	
			card->u.adsl.adapter, 
			udp_hdr->wan_udphdr_command,
			&udp_hdr->wan_udphdr_data[0],
			&udp_hdr->wan_udphdr_data_len);
		udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
		break;
	}

	wan_atomic_set(&adsl->udp_pkt_len, 
		sizeof(wan_mgmt_t) + sizeof(wan_trace_info_t) +
		sizeof(wan_cmd_t) + udp_hdr->wan_udphdr_data_len);
	udp_hdr->wan_udphdr_request_reply = UDPMGMT_REPLY;

	return 0;
}
#endif

#if defined (__LINUX__)
static int wanpipe_attach_sppp(sdla_t *card, netdevice_t *dev, wanif_conf_t *conf)
{
	adsl_private_area_t *adsl=wan_netif_priv(dev);
	struct ppp_device *pppdev=NULL;
	struct sppp *sp=NULL;

	pppdev=kmalloc(sizeof(struct ppp_device),GFP_KERNEL);
	if (!pppdev){
		return -ENOMEM;
	}
	memset(pppdev,0,sizeof(struct ppp_device));

	adsl->common.prot_ptr=(void*)pppdev;

	/* Attach PPP protocol layer to pppdev
	* The wp_sppp_attach() will initilize the dev structure
	* and setup ppp layer protocols.
	* All we have to do is to bind in:
	*    if_open(), if_close(), if_send() and get_stats() functions.
	*/

	pppdev->dev=dev;

	/* Get authentication info. */
	if(conf->pap == WANOPT_YES){
		pppdev->sppp.myauth.proto = PPP_PAP;
		DEBUG_EVENT("%s: Enableing PAP Protocol\n",
			card->devname); 
	}else if(conf->chap == WANOPT_YES){
		pppdev->sppp.myauth.proto = PPP_CHAP;
		DEBUG_EVENT("%s: Enableing CHAP Protocol\n",
			card->devname); 
	}else{
		pppdev->sppp.myauth.proto = 0;
		DEBUG_EVENT("%s: Authentication protocols disabled\n",
			card->devname);
	}

	if(pppdev->sppp.myauth.proto){
		memcpy(pppdev->sppp.myauth.name, conf->userid, AUTHNAMELEN);
		memcpy(pppdev->sppp.myauth.secret, conf->passwd, AUTHNAMELEN);

		DEBUG_TX("%s: %s Username=%s Passwd=*****\n",
			card->devname, 
			(pppdev->sppp.myauth.proto==PPP_PAP)?"PAP":"CHAP",
			conf->userid);
	}

	pppdev->sppp.gateway = conf->gateway;
	if (conf->if_down){
		pppdev->sppp.dynamic_ip = 1;
	}

	sprintf(pppdev->sppp.hwdevname,"%s",card->devname);

	wp_sppp_attach(pppdev);
	sp = &pppdev->sppp;


	/* Enable PPP Debugging */
	if (conf->protocol == WANCONFIG_CHDLC){
		printk(KERN_INFO "%s: Starting Kernel CISCO HDLC protocol\n",
			card->devname);
		sp->pp_flags |= PP_CISCO;
		conf->ignore_dcd = WANOPT_YES;
		conf->ignore_cts = WANOPT_YES;
	}else{
		printk(KERN_INFO "%s: Starting Kernel Sync PPP protocol\n",
			card->devname);
		sp->pp_flags &= ~PP_CISCO;
		dev->type	= ARPHRD_PPP;
	}

	return 0;
}

#endif

#if defined(__WINDOWS__)

static int netif_rx(netdevice_t	*sdla_net_dev, netskb_t *rx_skb)
{
	adsl_private_area_t *chan = (adsl_private_area_t*)wan_netif_priv(sdla_net_dev);

	DBG_ADSL_RX("%s()\n", __FUNCTION__);

	if(wanpipe_lip_rx(chan, rx_skb)){
		chan->if_stats.rx_dropped++;
		wan_skb_free(rx_skb);
	}else{
		chan->if_stats.rx_packets++;
		chan->if_stats.rx_bytes += wan_skb_len(rx_skb);
	}

	return 0;
}

static int adsl_if_send(netskb_t* skb, netdevice_t* dev)
{
	adsl_private_area_t *chan = (adsl_private_area_t*)wan_netif_priv(dev);
	sdla_t				*card = (sdla_t*)chan->common.card;

	DBG_ADSL_TX("%s()\n", __FUNCTION__);

	if(wan_test_bit(TX_BUSY_SET,&card->wandev.critical)){
		DBG_ADSL_FAST_TX("%s: return 'SANG_STATUS_DEVICE_BUSY'\n", dev->name);
		return SANG_STATUS_DEVICE_BUSY;
	}

//	wan_skb_print(skb);

	if(adsl_output(skb, dev)){

		DBG_ADSL_TX("%s():%s: Warning: adsl_output() failed. Dropping TX data!\n",
			__FUNCTION__, dev->name);

		WAN_NETIF_START_QUEUE(dev);
		return 1;

	}else{
		chan->if_stats.tx_packets++;
		chan->if_stats.tx_bytes += wan_skb_len(skb);
		return 0;
	}
}
#endif /* #if defined(__WINDOWS__) */
