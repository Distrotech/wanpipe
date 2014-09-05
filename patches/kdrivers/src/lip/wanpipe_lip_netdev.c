/*************************************************************
 * wanpipe_lip_netdev.c   WANPIPE Link Interface Protocol Layer (LIP)
 *
 *
 *
 * ===========================================================
 *
 * Mar 30 2004	Nenad Corbic	Initial Driver
 */


/*=============================================================
 * Includes
 */

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <wanpipe_lip.h>
#elif defined(__WINDOWS__)
#include "wanpipe_lip.h"
#else
# include <linux/wanpipe_lip.h>
#endif


/*=============================================================
 * Definitions
 */
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
int wplip_if_output (netdevice_t* dev,netskb_t* skb,struct sockaddr* sa, struct rtentry* rt);
#endif

char* get_master_dev_name(wplip_link_t 	*lip_link);

/*==============================================================
 * wplip_open_dev
 *
 * Description:
 *	This function enables the svc network device, and
 *	sets it up for data transfer.
 *
 *	It is called by the kernel during interfaces
 *	startup via ifconfig:
 *		ex: ifconfig wp1_svc up
 *
 *  	If the network device contains IP data, its
 *  	operation mode is set to TCP/IP otherwise,
 *  	the opteration mode is API.
 * 
 * Usedby:
 * 	Kernel during ifconfig system call.
 */

int wplip_open_dev(netdevice_t *dev)
{
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);
	
	if (!lip_dev || !lip_dev->lip_link){
		return -ENODEV;
	}

	if (wan_test_bit(WPLIP_DEV_UNREGISTER,&lip_dev->critical)) {
		return -ENODEV;
	}
	
#if defined(__LINUX__)
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18)
	if (netif_running(dev))
		return -EBUSY;
# endif
	wplip_change_mtu(dev,dev->mtu);
#endif

	wan_clear_bit(0,&lip_dev->if_down);

#if 0
	/* Done in if register now, do it interface up down
	 * feature on 2.4 kernels */
	err=wplip_open_lipdev_prot(lip_dev);
	if (err){
		return err;
	}
#endif

#if defined(WANPIPE_IFNET_QUEUE_POLICY_INIT_OFF)
	if (lip_dev->lip_link->state == WAN_CONNECTED){
		WAN_NETIF_CARRIER_ON(dev);		
		WAN_NETIF_WAKE_QUEUE(dev);
	}
#else
	if (lip_dev->lip_link->state == WAN_CONNECTED){
		WAN_NETIF_CARRIER_ON(dev);		
	}
	WAN_NETIF_WAKE_QUEUE(dev);
#endif

	/* Its possible for state update to be skipped if interface was down */
        wplip_lipdev_prot_update_state_change(lip_dev,NULL,0);


	if (!wan_test_bit(WAN_DEV_READY,&lip_dev->interface_down)) {
		wan_set_bit(WAN_DEV_READY,&lip_dev->interface_down);
		wplip_trigger_if_task(lip_dev);
	}
	return 0;
}

/*==============================================================
 * wplip_stop_dev
 *
 * Description:
 *	This function disables the svc network device.
 *
 *	It is called by the kernel during interfaces
 *	shutdown via ifconfig:
 *		ex: ifconfig wp1_svc down
 *
 *	If the svc is in connected state, the call
 *	will be disconnected.
 *	
 * Usedby:
 * 	Kernel during ifconfig system call.
 */

int wplip_stop_dev(netdevice_t *dev)
{
	
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);

	if (!lip_dev || !lip_dev->lip_link){
		return 0;
	}

#ifdef WPLIP_TTY_SUPPORT
	if (lip_dev->lip_link->tty_opt && lip_dev->lip_link->tty_open){
		tty_hangup(lip_dev->lip_link->tty);
		return -EBUSY;
	}
#endif

	wan_set_bit(0,&lip_dev->if_down);

#if 0
	/* Done in if register now, do it interface up down
	 * feature on 2.4 kernels */
	wplip_close_lipdev_prot(lip_dev);
#endif

	return 0;
}


#if defined(__LINUX__)
/*==============================================================
 * wplip_ifstats
 *
 * Description:
 * 	This fucntion interfaces the /proc file system
 * 	to the svc device.  The svc keeps protocol and
 * 	packet statistcs.  This function passes these
 * 	statistics to the /proc file system.
 *	
 * Usedby:
 * 	Kernel /proc/net/dev file system
 */
static struct net_device_stats gstats;
struct net_device_stats* wplip_ifstats (netdevice_t *dev)
{
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);

	DEBUG_TEST("%s: LIP %s()\n",
		wan_netif_name(dev),__FUNCTION__);

	if (lip_dev){
		return &lip_dev->common.if_stats;
	}

	return &gstats;
}
#endif

#if defined(__WINDOWS__)
/*==============================================================
 * wplip_ifstats
 *
 * Description:
 * 	This fucntion interfaces the /proc file system
 * 	to the svc device.  The svc keeps protocol and
 * 	packet statistcs.  This function passes these
 * 	statistics to the /proc file system.
 *	
 * Usedby:
 * 	Kernel /proc/net/dev file system
 */
static struct net_device_stats gstats;
struct net_device_stats* wplip_ifstats (netdevice_t *dev)
{
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);

	DEBUG_TEST("%s: LIP %s()\n",
		wan_netif_name(dev),__FUNCTION__);

	if (lip_dev){
		return &lip_dev->common.if_stats;
	}

	return &gstats;
}
#endif


/*==============================================================
 * wplip_if_send
 *
 * Description:
 * 	Call back function used by the kernel or the
 * 	upper protocol layer to transmit data for each
 * 	svc withing the x25 link.
 *	
 *	Data can only be tranmsitted if the svc state
 *	is connected.
 *
 *	If state != CONNECTED && If svc mode=TCP/IP
 *		place an x25 call and try to establish
 *		connection.
 *		
 *	If state != CONNECTED && If svc mode=API
 *		refuse the packet, and indicate to 
 *		the upper layer that the connection has
 *		not been made.
 * 	
 *	
 * Usedby:
 * 	Kernel TCP/IP stack or upper layers to transmit data.
 */

#if defined(__LINUX__) || defined(__WINDOWS__)
int wplip_if_send (netskb_t* skb, netdevice_t* dev)
#else
int wplip_if_output (netdevice_t* dev,netskb_t* skb,struct sockaddr* sa, struct rtentry* rt)
#endif
{
	wplip_dev_t 	 *lip_dev		= wplip_netdev_get_lipdev(dev);
	wan_api_tx_hdr_t *api_tx_hdr	= NULL;
	int err, type;
	int len = skb?wan_skb_len(skb):0;

	if (!lip_dev || !lip_dev->lip_link){
		WAN_NETIF_STOP_QUEUE(dev);
		return 1;
	}

#if 1
	if (lip_dev->common.state != WAN_CONNECTED){
#else
	if (lip_dev->lip_link->carrier_state != WAN_CONNECTED || 
	    lip_dev->common.state != WAN_CONNECTED){
#endif

#if defined(WANPIPE_IFNET_QUEUE_POLICY_INIT_OFF)		
		/* This causes a buffer starvations on some 
                 * applications like OSPF, since packets are
                 * trapped in the Interface TX queue */

		WAN_NETIF_STOP_QUEUE(dev);
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		WAN_NETIF_STATS_INC_TX_CARRIER_ERRORS(&lip_dev->common);	//++lip_dev->ifstats.tx_carrier_errors;
		return 1;
#else
		wan_skb_free(skb);
		WAN_NETIF_STATS_INC_TX_CARRIER_ERRORS(&lip_dev->common);	//lip_dev->ifstats.tx_carrier_errors++;
		WAN_NETIF_START_QUEUE(dev);
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		return 0;
#endif
	}

	/*if (wan_skb_check(skb)){
	**	if (wan_skb2buffer((void**)&skb)){
	**		wan_skb_free(skb);
	**		lip_dev->ifstats.tx_errors++;
	**		WAN_NETIF_START_QUEUE(dev);
	**		wan_netif_set_ticks(dev, SYSTEM_TICKS);
	**		return 0;
	**		
	**	}
	**} */

	if (lip_dev->common.usedby == API){
		if (wan_skb_len(skb) <= sizeof(wan_api_tx_hdr_t)){
			wan_skb_free(skb);
			WAN_NETIF_STATS_INC_TX_ERRORS(&lip_dev->common);	//lip_dev->ifstats.tx_errors++;
			WAN_NETIF_START_QUEUE(dev);
			wan_netif_set_ticks(dev, SYSTEM_TICKS);
			return 0;
		}
		api_tx_hdr=(wan_api_tx_hdr_t*)wan_skb_pull(skb,sizeof(wan_api_tx_hdr_t));
			
		type = WPLIP_RAW;
	}else{
#if defined(__LINUX__) || defined(__WINDOWS__)
		type = wplip_decode_protocol(lip_dev,skb);
#else
		type = wplip_decode_protocol(lip_dev,sa);
#endif
	}

	
	err=wplip_prot_tx(lip_dev, api_tx_hdr, skb, type);
	switch (err){

	case 0:
		/* Packet queued ok */
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		WAN_NETIF_START_QUEUE(dev);

		WAN_NETIF_STATS_INC_TX_PACKETS(&lip_dev->common);	//lip_dev->ifstats.tx_packets++;
		WAN_NETIF_STATS_INC_TX_BYTES(&lip_dev->common,len);	//lip_dev->ifstats.tx_bytes += len;
	
		err=0;
		break;
		
	case 1:
		DEBUG_TEST("%s: %s() failed to tx busy\n",lip_dev->name,__FUNCTION__);
		/* Packet failed to queue layer busy */
		WAN_NETIF_STOP_QUEUE(dev);
		err=1;
		break;

	default:
		/* Packet dropped due to error */
		DEBUG_TEST	("%s: %s() failed to tx error\n",lip_dev->name,__FUNCTION__);
		WAN_NETIF_START_QUEUE(dev);
		WAN_NETIF_STATS_INC_TX_ERRORS(&lip_dev->common);	//lip_dev->ifstats.tx_errors++;
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		wan_skb_free(skb);
		err=0;
		break;
	}

	wplip_trigger_bh(lip_dev->lip_link);

#if defined(__LINUX__)
        if (lip_dev->protocol != WANCONFIG_LIP_ATM) {
          	if (dev->tx_queue_len < lip_dev->max_mtu_sz && 
	    	    dev->tx_queue_len > 0) {
        		DEBUG_EVENT("%s: Resizing Tx Queue Len to %li\n",
				lip_dev->name,dev->tx_queue_len);
			lip_dev->max_mtu_sz = dev->tx_queue_len;      	  
			wplip_lipdev_latency_change(lip_dev->lip_link);

		} else if (dev->tx_queue_len > lip_dev->max_mtu_sz &&
	    	    	lip_dev->max_mtu_sz != lip_dev->max_mtu_sz_orig) {
         		DEBUG_EVENT("%s: Resizing Tx Queue Len to %i\n",
				lip_dev->name,lip_dev->max_mtu_sz_orig);
		        lip_dev->max_mtu_sz = lip_dev->max_mtu_sz_orig;
			wplip_lipdev_latency_change(lip_dev->lip_link);
		}
	}
#endif     
	
	return err;
}

#if defined(__LINUX__)
static void wplip_tx_timeout (netdevice_t *dev)
{
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);
	wplip_link_t *lip_link = lip_dev->lip_link;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this
	 * is only used as a last resort.
	 */

#if 0
		gdbg_flag=1;
#endif		
	wan_clear_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working);
	wplip_kick_trigger_bh(lip_link);

	WAN_NETIF_WAKE_QUEUE (dev);

	if (lip_dev->common.usedby == API){
		wan_update_api_state(lip_dev);	
	}

	wan_netif_set_ticks(dev, SYSTEM_TICKS);
}
#endif/* __LINUX__ */

#if defined(__WINDOWS__)
static void wplip_tx_timeout (netdevice_t *dev)
{
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);
	wplip_link_t *lip_link = lip_dev->lip_link;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this
	 * is only used as a last resort.
	 */

#if 0
		gdbg_flag=1;
#endif		
	wan_clear_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working);
	wplip_kick_trigger_bh(lip_link);

	WAN_NETIF_WAKE_QUEUE (dev);

	if (lip_dev->common.usedby == API){
		wan_update_api_state(lip_dev);	
	}

	wan_netif_set_ticks(dev, SYSTEM_TICKS);
}
#endif/* __WINDOWS__ */


static int
wplip_ioctl (netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t cmd)
{

	wplip_dev_t	*lip_dev = wplip_netdev_get_lipdev(dev);
	wplip_link_t 	*lip_link;
	wan_smp_flag_t	flags;
	int		err=0;
	wan_udp_pkt_t   *wan_udp_pkt;

	if (!lip_dev || !lip_dev->lip_link){
		DEBUG_EVENT("%s:%d: Assertion Error on lip_dev (%s)!\n",
				__FUNCTION__,__LINE__, wan_netif_name(dev));
		return -EINVAL;
	}

	lip_link = lip_dev->lip_link;
	if (lip_link == NULL){
		DEBUG_EVENT("%s:%d: Assertion Error on lip_dev (%s)!\n",
				__FUNCTION__,__LINE__, wan_netif_name(dev));
		return -EINVAL;
	}
	
	switch (cmd){

#if defined(__LINUX__)
		case SIOC_WANPIPE_BIND_SK:
			if (ifr == NULL){
				err= -EINVAL;
				break;
			}

			wplip_spin_lock_irq(&lip_link->bh_lock,&flags);

			if (lip_dev->common.usedby == API){
				dev->watchdog_timeo=HZ*60;		
			}
			
			err=wan_bind_api_to_svc(lip_dev,ifr->ifr_data);
			wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
			break;

		case SIOC_WANPIPE_UNBIND_SK:
			if (ifr == NULL){
				err= -EINVAL;
				break;
			}

			wplip_spin_lock_irq(&lip_link->bh_lock,&flags);
			err=wan_unbind_api_from_svc(lip_dev,ifr->ifr_data);
			if (lip_dev->common.usedby == API && 
			    lip_dev->protocol == WANCONFIG_XDLC){
				wplip_close_lipdev_prot(lip_dev);
			}
			wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
			break;

		case SIOC_WANPIPE_CHECK_TX:
		case SIOC_ANNEXG_CHECK_TX:
			err=0;
			break;

		case SIOC_WANPIPE_DEV_STATE:
			err = lip_dev->common.state;
			break;

		case SIOC_ANNEXG_KICK:
			break;
#endif

		case SIOC_WANPIPE_PIPEMON:

			wplip_spin_lock_irq(&lip_link->bh_lock,&flags);
			if (lip_dev->udp_pkt_len != 0){
				wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
				return -EBUSY;
			}
			lip_dev->udp_pkt_len = sizeof(wan_udp_hdr_t);
			wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

			wan_udp_pkt=(wan_udp_pkt_t*)&lip_dev->udp_pkt_data;
			if (WAN_COPY_FROM_USER(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				lip_dev->udp_pkt_len=0;
				return -EFAULT;
			}

			wplip_spin_lock_irq(&lip_link->bh_lock,&flags);

			if(wan_udp_pkt->wan_udp_command == WAN_GET_MASTER_DEV_NAME){
				char* master_dev_name;

				master_dev_name = get_master_dev_name(lip_link);
				if(master_dev_name == NULL){
							
					wan_udp_pkt->wan_udp_return_code = 1;
					wan_udp_pkt->wan_udp_data_len = 1;
				}else{
					strncpy(&wan_udp_pkt->wan_udp_data[0],
						master_dev_name,
					      	strlen(master_dev_name));
					wan_udp_pkt->wan_udp_return_code = 0;
					wan_udp_pkt->wan_udp_data_len = strlen(master_dev_name);
				}
			}else{
				if (wplip_prot_udp_mgmt_pkt(lip_dev,wan_udp_pkt) <= 0){
					lip_dev->udp_pkt_len=0;
					wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
					return -EINVAL;
				}
			}

			if (lip_dev->udp_pkt_len > sizeof(wan_udp_pkt_t)){
				DEBUG_EVENT("%s: Error: Pipemon buf too bit on the way up! %i\n",
						lip_dev->name,lip_dev->udp_pkt_len);
				lip_dev->udp_pkt_len=0;
				wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
				return -EINVAL;
			}

			wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */

			if (WAN_COPY_TO_USER(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				lip_dev->udp_pkt_len=0;
				return -EFAULT;
			}
		
			lip_dev->udp_pkt_len=0;
			return 0;

		case SIOC_WANPIPE_SNMP:
			wplip_prot_udp_snmp_pkt(lip_dev,cmd,ifr);
			return 0;
		
		case SIOC_WANPIPE_SNMP_IFSPEED:
			DEBUG_EVENT("%s: SNMP Speed not supported on protocol interface!\n",
					lip_dev->name);
			return -1;
		
		default:

#if defined(__LINUX__)
			if (cmd >= SIOC_WANPIPE_DEVPRIVATE)
			{

				wplip_spin_lock_irq(&lip_link->bh_lock,&flags);
				cmd-=SIOC_WANPIPE_DEVPRIVATE;
				if (ifr == NULL){
					err=wplip_prot_ioctl(lip_dev,cmd,NULL);
				}else{
					err=wplip_prot_ioctl(lip_dev,cmd,ifr->ifr_data);
				}
				wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

				return err;
			}
#endif
			
# if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
			return 1;
#else
			DEBUG_TEST("%s: Command %x not supported!\n",
				lip_link->name,cmd);
			return -EOPNOTSUPP;
#endif
	}

	return err;

}

char* get_master_dev_name(wplip_link_t 	*lip_link)
{
	wplip_dev_list_t *lip_dev_list_el; 	
	netdevice_t *dev;
		
	if (!lip_link->tx_dev_cnt){ 
		DEBUG_EVENT("%s: %s: Tx Dev List empty!\n",
			__FUNCTION__,lip_link->name);	
		return NULL;
	}

	lip_dev_list_el=WAN_LIST_FIRST(&lip_link->list_head_tx_ifdev);
	if (!lip_dev_list_el){
		DEBUG_EVENT("%s: %s: Tx Dev List empty!\n",
			__FUNCTION__,lip_link->name);	
		return NULL;
	}
	
	if (lip_dev_list_el->magic != WPLIP_MAGIC_DEV_EL){
		DEBUG_EVENT("%s: %s: Error: Invalid dev magic number!\n",
			__FUNCTION__,lip_link->name);
		return NULL;
	}

	dev=lip_dev_list_el->dev;
	if (!dev){
		DEBUG_EVENT("%s: %s: Error: No dev!\n",
			__FUNCTION__,lip_link->name);
		return NULL;
	}	
				
	return wan_netif_name(dev);
}

/*==============================================================
 * wplip_if_init
 *
 * Description:
 *	During device registration, this function is
 *	called to fill in the call back functions
 *	used in nework device setup and operation.
 *	
 *	The kernel interfaces the driver, using the
 *	call back functions below.
 *	
 * Usedby:
 * 	Kernel during register_netdevice() in x25_register
 * 	function.
 */
int wplip_if_init(netdevice_t *dev)
{
	wplip_dev_t* lip_dev = wplip_netdev_get_lipdev(dev);

#if defined(__LINUX__)
	lip_dev->common.is_netdev	= 1;
	lip_dev->common.iface.open	= &wplip_open_dev;
	lip_dev->common.iface.close	= &wplip_stop_dev;
	lip_dev->common.iface.send	= &wplip_if_send;
	lip_dev->common.iface.ioctl	= &wplip_ioctl;
	lip_dev->common.iface.tx_timeout= &wplip_tx_timeout;
	lip_dev->common.iface.get_stats	= &wplip_ifstats;
	lip_dev->common.iface.change_mtu = &wplip_change_mtu;

	return 0;
#elif defined(__WINDOWS__)
	lip_dev->common.is_netdev	= 1;
	lip_dev->common.iface.open	= &wplip_open_dev;
	lip_dev->common.iface.close	= &wplip_stop_dev;
	lip_dev->common.iface.send	= &wplip_if_send;
	lip_dev->common.iface.ioctl	= &wplip_ioctl;
	lip_dev->common.iface.tx_timeout= &wplip_tx_timeout;
	lip_dev->common.iface.get_stats	= &wplip_ifstats;
//	lip_dev->common.iface.change_mtu = &wplip_change_mtu;

	return 0;
#else
	DEBUG_EVENT("%s: Initialize network interface...\n",
				wan_netif_name(dev));

	lip_dev->common.is_netdev	= 1;
	lip_dev->common.iface.open	= &wplip_open_dev;
	lip_dev->common.iface.close	= &wplip_stop_dev;
	lip_dev->common.iface.output	= &wplip_if_output;
	lip_dev->common.iface.ioctl	= &wplip_ioctl;

	dev->if_type = IFT_PPP;
	dev->if_mtu = 1500;
#if 0
/* Remove this later (wanpipe_bsd_iface.c doing this) */
	dev->if_output		= NULL;
	dev->if_start		= &wplip_if_start;
	dev->if_ioctl		= NULL; /* &wplip_ioctl; */
	
	/* Initialize media-specific parameters */
	dev->if_flags		|= IFF_POINTOPOINT;
	dev->if_flags		|= IFF_NOARP;

	dev->if_mtu = 1500;
	WAN_IFQ_SET_MAXLEN(&dev->if_snd, 100);
	dev->if_snd.ifq_len = 0;
	dev->if_type = IFT_PPP;
#endif
	return 0;
#endif
}


