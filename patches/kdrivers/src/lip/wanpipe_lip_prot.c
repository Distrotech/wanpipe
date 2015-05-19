
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <wanpipe_lip.h>
#elif defined(__WINDOWS__)
# include "wanpipe_lip.h"
#else
# include <linux/wanpipe_lip.h>
#endif

#undef WPLIP_IPX_SUPPORT

/* Function interface between LIP layer and kernel */
extern wan_iface_t wan_iface;
static wplip_prot_iface_t *wplip_prot_ops[MAX_LIP_PROTOCOLS];

/*=============================================================
 * Funciton Prototypes
 */

static wplip_prot_reg_t wplip_prot_reg_ops;

#if defined(__WINDOWS__)
static void wplip_prot_timer(IN PKDPC Dpc, void *arg, void *arg2, void *arg3);
extern void wplip_poll_carrier_status(wplip_link_t *lip_link);
#else
static void wplip_prot_timer(wan_timer_arg_t arg);
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static void wplip_port_task (void *arg, int);
#elif defined(__WINDOWS__)
static void wplip_port_task (IN PKDPC Dpc, void *arg, void *arg2, void *arg3);
#else
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 	
static void wplip_port_task (void *arg);
# else
static void wplip_port_task (struct work_struct *work);
# endif       
#endif


/*==============================================================
 * Global Functions
 */

int wplip_reg_link_prot(wplip_link_t *lip_link, wanif_conf_t *conf)
{
	wplip_prot_iface_t *prot_iface;	

	DEBUG_TEST("%s: (DEBUG) Registering Protocol %d(%s)\n",
			lip_link->name,conf->protocol,
			SDLA_DECODE_PROTOCOL(conf->protocol));
	
	lip_link->protocol = conf->protocol;

	WPLIP_PROT_ASSERT(lip_link->protocol,-EINVAL);

	wan_init_timer(&lip_link->prot_timer,
		       wplip_prot_timer,
		       (wan_timer_arg_t)lip_link);

	WAN_TASKQ_INIT((&lip_link->prot_task),0,wplip_port_task,lip_link);


#ifdef WPLIP_TTY_SUPPORT
	if (lip_link->protocol == WANCONFIG_TTY){
		conf->port=0;
		if (is_digit(conf->addr[0])) {
			conf->port = dec_to_uint(conf->addr, 0);
		}
		return wplip_reg_tty(lip_link,conf);
	}
#endif	
	
	WPLIP_PROT_EXIST(lip_link->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_link->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,prot_link_register,-EFAULT);

	
	lip_link->prot=prot_iface->prot_link_register(lip_link,
			 	 	       	      lip_link->name,
				       	              &conf->u, 
				        	      &wplip_prot_reg_ops);
		
	if (!lip_link->prot){
		DEBUG_EVENT("%s: Failed to register protocol %d(%s)\n",
			lip_link->name,lip_link->protocol,
		       	SDLA_DECODE_PROTOCOL(lip_link->protocol));
		return -EINVAL;
	}

	wan_add_timer (&lip_link->prot_timer,HZ);


	return 0;
}



int wplip_unreg_link_prot(wplip_link_t *lip_link)
{
	wplip_prot_iface_t *prot_iface;	

	WPLIP_PROT_ASSERT(lip_link->protocol,-EINVAL);

	DEBUG_TEST("%s: (DEBUG) UNRegistering Protocol 0x%X\n",
			lip_link->name,lip_link->protocol);

#ifdef WPLIP_TTY_SUPPORT
	if (lip_link->protocol == WANCONFIG_TTY){
		return wplip_unreg_tty(lip_link);
	}
#endif

	if (!lip_link->prot){
		return 0;
	}

	WPLIP_PROT_EXIST(lip_link->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_link->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,prot_link_unregister,-EFAULT);

	WAN_TASKLET_KILL((&lip_link->task));
	wan_del_timer(&lip_link->prot_timer);

	prot_iface->prot_link_unregister(lip_link->prot);		
	lip_link->prot=NULL;

	return 0;
}



int wplip_reg_lipdev_prot(wplip_dev_t *lip_dev, wanif_conf_t *conf)
{
	wplip_prot_iface_t *prot_iface;	
	
	lip_dev->protocol = conf->protocol;

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);

	if (lip_dev->protocol != lip_dev->lip_link->protocol){
		DEBUG_EVENT("%s: Config error: link prot 0x%x != lipdev prot 0x%X\n",
				lip_dev->lip_link->name,
				lip_dev->lip_link->protocol,
				lip_dev->protocol);
		return -EINVAL;
	}
	
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,prot_chan_register,-EFAULT);

#ifdef CONFIG_PRODUCT_WANPIPE_FR	
	if (lip_dev->protocol == WANCONFIG_FR){
		int dlci=0;
		if (is_digit(conf->addr[0])) {
			dlci = dec_to_uint(conf->addr, 0);
		}
		conf->u.fr.dlci_num=dlci;
		lip_dev->prot_addr=dlci;

	}
#endif
#ifdef CONFIG_PRODUCT_WANPIPE_LIP_ATM	
	if (lip_dev->protocol == WANCONFIG_LIP_ATM ||
	    lip_dev->protocol == WANCONFIG_LIP_KATM){
		int mtu=conf->u.atm.mtu;
		if (mtu) {
			lip_dev->max_mtu_sz = mtu/53 * 5;
			lip_dev->max_mtu_sz_orig=lip_dev->max_mtu_sz;
		} else {
			lip_dev->max_mtu_sz=MAX_ATM_TX_BUF;
			lip_dev->max_mtu_sz_orig=MAX_ATM_TX_BUF;
		}

	}
#endif                     	
	
#ifdef CONFIG_PRODUCT_WANPIPE_XDLC	
	if (lip_dev->protocol == WANCONFIG_XDLC){
		int address=0;
		if (is_digit(conf->addr[0])) {
			address = dec_to_uint(conf->addr, 0);
		}
		conf->u.xdlc.address=address;
		lip_dev->prot_addr=address;
	}
#endif
		
	
	lip_dev->common.prot_ptr = 
			prot_iface->prot_chan_register(lip_dev, 
						       lip_dev->lip_link->prot, 
						       lip_dev->name,
						       &conf->u,
						       lip_dev->common.usedby == API?
						       WPLIP_RAW:WPLIP_IP);
		
	if (!lip_dev->common.prot_ptr){
		DEBUG_EVENT("%s: Failed to register protocol channel!\n",
			lip_dev->name);
		return -EINVAL;
	}

	return 0;
}

int wplip_unreg_lipdev_prot(wplip_dev_t *lip_dev)
{
	int err=0;
	wplip_prot_iface_t *prot_iface;	
	

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);


	if (!lip_dev->common.prot_ptr){
		return 0;
	}


	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,prot_chan_unregister,-EFAULT);

	prot_iface->prot_chan_unregister(lip_dev->common.prot_ptr);
	lip_dev->common.prot_ptr=NULL;
	
	return err;
}


int wplip_open_lipdev_prot(wplip_dev_t *lip_dev)
{
	wplip_prot_iface_t *prot_iface;	

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,open_chan,-EFAULT);

	return prot_iface->open_chan(lip_dev->common.prot_ptr);
}


int wplip_close_lipdev_prot(wplip_dev_t *lip_dev)
{

	wplip_prot_iface_t *prot_iface;	

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,close_chan,-EFAULT);

	return prot_iface->close_chan(lip_dev->common.prot_ptr);
}

int wplip_prot_kick(wplip_link_t *lip_link, wplip_dev_t *lip_dev)
{
	wplip_prot_iface_t *prot_iface;	

	if (!lip_dev){
		DEBUG_EVENT("%s: Asserting error lip_dev=NULL \n",__FUNCTION__);
		return -EINVAL;
	}

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	if (prot_iface->bh){
		prot_iface->bh(lip_dev->common.prot_ptr);
	}

	return 0;

}

int wplip_prot_rx(wplip_link_t *lip_link, netskb_t *skb)
{
	wplip_prot_iface_t *prot_iface;	

	WPLIP_PROT_ASSERT(lip_link->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_link->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_link->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,rx,-EFAULT);

	return prot_iface->rx(lip_link->prot,skb);
}

int wplip_prot_oob(wplip_dev_t *lip_dev, unsigned char *data, int len)
{
#if defined(__LINUX__)
	netskb_t *skb;
#endif
	wan_api_rx_hdr_t *rx_el;

	rx_el=NULL;

#if defined(__LINUX__)

	if (!data || !len){
		return -1;
	}
	
	switch (lip_dev->protocol){
		
#if defined(CONFIG_PRODUCT_WANPIPE_LIP_LAPB) || defined(CONFIG_PRODUCT_WANPIPE_LIP_LAPD) 
	case WANCONFIG_LAPB:
	case WANCONFIG_LAPD:
		skb=wan_skb_alloc(sizeof(wan_api_rx_hdr_t));
		if (!skb){
			return -1;
		}
		rx_el=(wan_api_rx_hdr_t*)wan_skb_push(skb,sizeof(wan_api_rx_hdr_t));

		/* FIXME: */
		rx_el->lapb.exception=0;	
		skb->protocol=htons(ETH_P_X25);
		break;
#endif
		
#ifdef CONFIG_PRODUCT_WANPIPE_XDLC
	case WANCONFIG_XDLC:
		{
		wp_xdlc_excep_t *xdlc_excep = (wp_xdlc_excep_t *)data;
		skb=wan_skb_alloc(sizeof(wan_api_rx_hdr_t));
		if (!skb){
			return -1;
		}
		rx_el=(wan_api_rx_hdr_t*)wan_skb_push(skb,sizeof(wan_api_rx_hdr_t));
		rx_el->xdlc.state=xdlc_excep->state;
		rx_el->xdlc.address=xdlc_excep->address;
		rx_el->xdlc.exception=xdlc_excep->exception;
		skb->protocol=htons(ETH_P_X25);
		}
		break;
#endif
	default:
		return 0;
	}
	
	skb->pkt_type = WAN_PACKET_ERR;

	if (wan_api_rx(lip_dev,skb)!=0){
		wan_skb_free(skb);
		return -1;
	}
	return 0; 
#else
	return 0;
#endif
}
int wplip_prot_ioctl(wplip_dev_t *lip_dev, int cmd, void *arg)
{	
	wplip_prot_iface_t *prot_iface;	

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,ioctl,-EFAULT);

	return prot_iface->ioctl(lip_dev->common.prot_ptr,cmd,arg);
}

int wplip_prot_tx(wplip_dev_t *lip_dev, wan_api_tx_hdr_t *api_tx_hdr, netskb_t *skb, int type)
{
	int err;
	wplip_prot_iface_t *prot_iface;	
#ifdef WPLIP_IPX_SUPPORT	
	unsigned long dnet,snet;
#endif
	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);

	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,tx,-EFAULT);

	if (wan_skb_queue_len(&lip_dev->tx_queue) >=  lip_dev->max_mtu_sz){
		DEBUG_TEST("%s: %s() failed to tx busy qz=%i\n",lip_dev->name,__FUNCTION__,lip_dev->max_mtu_sz);
		return 1;
	}

#ifdef WPLIP_IPX_SUPPORT
	if (type==WPLIP_IPX){
		wplip_ipxwan_switch_net_num(wan_skb_data(skb),
					    lip_dev->ipx_net_num,
					    &dnet,
					    &snet,
					    0);	
	}
#endif
	
	err=prot_iface->tx(lip_dev->common.prot_ptr,skb,type);
	if (err){
#ifdef WPLIP_IPX_SUPPORT		
		if (type==WPLIP_IPX){
			wplip_ipxwan_restore_net_num(wan_skb_data(skb),
						     dnet,snet);
		}
#endif
		return err;
	}

	return 0;
}

int wplip_prot_udp_mgmt_pkt(wplip_dev_t * lip_dev, wan_udp_pkt_t *wan_udp_pkt)
{
	int err=-1;
	unsigned int len=0;
	int addr=0;
	wplip_prot_iface_t *prot_iface;	

	wan_udp_pkt->wan_udp_opp_flag = 0x0;

	switch (wan_udp_pkt->wan_udp_command){
	
	case WAN_GET_PROTOCOL:
		wan_udp_pkt->wan_udp_data[0] = lip_dev->protocol;
	    	wan_udp_pkt->wan_udp_return_code = 0;
	    	wan_udp_pkt->wan_udp_data_len = 1;
		return wan_udp_pkt->wan_udp_data_len;
	
	case WAN_GET_PLATFORM:
#ifdef __LINUX__
		wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
#elif defined(__FreeBSD__)
		wan_udp_pkt->wan_udp_data[0] = WAN_FREEBSD_PLATFORM;
#elif defined(__OpenBSD__)
		wan_udp_pkt->wan_udp_data[0] = WAN_OPENBSD_PLATFORM;
#elif defined(__NetBSD__)
		wan_udp_pkt->wan_udp_data[0] = WAN_NETBSD_PLATFORM;
#endif
		wan_udp_pkt->wan_udp_return_code = 0;
		wan_udp_pkt->wan_udp_data_len = 1;
		return wan_udp_pkt->wan_udp_data_len;
	}

	
	switch (lip_dev->protocol){

	case WANCONFIG_FR:
		addr=wan_udp_pkt->wan_udp_fr_dlci;
		break;
	}


	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	WPLIP_PROT_FUNC_ASSERT(prot_iface,pipemon,-EFAULT);

	err=prot_iface->pipemon(lip_dev->common.prot_ptr, 
			        wan_udp_pkt->wan_udp_command,
			        addr,
			        wan_udp_pkt->wan_udp_data,
			        &len);

	wan_udp_pkt->wan_udp_return_code = err;
	wan_udp_pkt->wan_udp_data_len = len;
	return wan_udp_pkt->wan_udp_data_len;
}

unsigned char wplip_fr_variables_oid[] = { 1,3,6,1,2,1,10,32 };
unsigned char wplip_ppp_variables_oid[] = { 1,3,6,1,2,1,10,23 };

int wplip_prot_udp_snmp_pkt(wplip_dev_t * lip_dev, int cmd, struct ifreq* ifr)
{
	wanpipe_snmp_t	snmp;
	wplip_prot_iface_t *prot_iface;	
	int err;
	
	memset(&snmp,0,sizeof(wanpipe_snmp_t));
		
	if (WAN_COPY_FROM_USER(&snmp, ifr->ifr_data, sizeof(wanpipe_snmp_t))){
		DEBUG_EVENT("%s: Failed to copy user snmp data to kernel space!\n",
				lip_dev->name);
		return -EFAULT;
	}
	
	if (strncmp((char *)wplip_fr_variables_oid,(char *)snmp.snmp_name, sizeof(wplip_fr_variables_oid)) == 0){
		/* SNMP call for frame relay */
		DEBUG_EVENT("%s: Get Frame Relay SNMP data\n", 
					lip_dev->name);

		if (lip_dev->protocol != WANCONFIG_FR){
			return 	-EAFNOSUPPORT;
		}
		
	}else if (strncmp((char *)wplip_ppp_variables_oid,(char *)snmp.snmp_name, sizeof(wplip_ppp_variables_oid)) == 0){
		/* SNMP call for PPP */
		DEBUG_EVENT("%s: Get PPP SNMP data\n", 
					lip_dev->name);

		return 	-EAFNOSUPPORT;
	}else{
		return 	-EAFNOSUPPORT;
	}

	WPLIP_PROT_ASSERT(lip_dev->protocol,-EINVAL);
	WPLIP_PROT_EXIST(lip_dev->protocol,-ENODEV);
	
	prot_iface=wplip_prot_ops[lip_dev->protocol];

	if (!prot_iface->snmp){
		return -EAFNOSUPPORT;
	}

	err=prot_iface->snmp(lip_dev->common.prot_ptr, 
			     &snmp);
	if (err){
		return -EAFNOSUPPORT;	
	}
	

	if (WAN_COPY_TO_USER(ifr->ifr_data, &snmp, sizeof(wanpipe_snmp_t))){
		DEBUG_EVENT("%s: Failed to copy kernel space to user snmp data!\n",
				lip_dev->name);
		return -EFAULT;
	}

	return 0;
}

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static void wplip_port_task (void *arg, int dummy)
#elif defined(__WINDOWS__)
static void wplip_port_task (IN PKDPC Dpc, void *arg, void *arg2, void *arg3)
#else
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 	
static void wplip_port_task (void *arg)
# else
static void wplip_port_task (struct work_struct *work)
# endif   
#endif
{

#if defined(__LINUX__)	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
	wplip_link_t	*lip_link=(wplip_link_t *)container_of(work, wplip_link_t, prot_task);
#else
	wplip_link_t	*lip_link=(wplip_link_t *)arg;
#endif
#else
	wplip_link_t	*lip_link=(wplip_link_t *)arg;
#endif	
	
	wplip_prot_iface_t *prot_iface;	

	if (wan_test_bit(WPLIP_LINK_DOWN,&lip_link->tq_working)){
		return;
	}
	
	prot_iface=wplip_prot_ops[lip_link->protocol];
	if (!prot_iface){
		return;
	}

	if (prot_iface->task){
		prot_iface->task(lip_link->prot);
	}
}

#if defined(__WINDOWS__)
/* This timer runs all the time. The delay is 1 second. */
static void wplip_prot_timer(IN PKDPC Dpc, void *arg, void *arg2, void *arg3)
#else
static void wplip_prot_timer(wan_timer_arg_t arg)
#endif
{
	wplip_link_t	*lip_link=(wplip_link_t *)arg;
	unsigned int	period=HZ;
	wan_smp_flag_t	flags;
	wplip_prot_iface_t *prot_iface;	

	DEBUG_TEST("%s: PROT TIMER: %s  devcnt=%d \n",
			__FUNCTION__,lip_link->name,
			lip_link->dev_cnt);

	if (wan_test_bit(WPLIP_LINK_DOWN,&lip_link->tq_working)){
		return;
	}

	prot_iface=wplip_prot_ops[lip_link->protocol];
	if (!prot_iface){
		return;
	}

	if (!prot_iface->timer) {
		return;
	}

#if defined(__WINDOWS__)
	/* Front End often gets connected before Protocol is fully
	 * initialized (especially on 56k), so "Connected" event is lost. 
	 * Poll the Carrier for FE Status and, if different from what
	 * 'lip_link->carrier_state', call wplip_connect()/wplip_disconnect(). */
	wplip_poll_carrier_status(lip_link);
#endif

	wplip_spin_lock_irq(&lip_link->bh_lock,&flags);

	if (lip_link->carrier_state == WAN_CONNECTED){
		prot_iface->timer(lip_link->prot,&period,1);
	}else{
		prot_iface->timer(lip_link->prot,&period,0);
	}

	wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

	if (wan_skb_queue_len(&lip_link->tx_queue)){
		wplip_trigger_bh(lip_link);
	}

	if (period < 1){
		period=HZ;
	}

	wan_add_timer(&lip_link->prot_timer,period);

}

static int wplip_prot_rx_up(void *lip_dev_ptr, void *skb, int type)
{
	int err;
	wplip_dev_t *lip_dev = (wplip_dev_t *)lip_dev_ptr;

	/* Default case for WANPIPE Interfaces 
	 * wplip_data_rx_up() must be called with
	 * these fileds initialized.
	 *
	 * Only in WANPIPE mode. */

	wan_skb_set_dev(skb,lip_dev->common.dev);

#if defined(__LINUX__)
	if (lip_dev->common.usedby == BRIDGE || 
            lip_dev->common.usedby == BRIDGE_NODE){
		((netskb_t*)skb)->protocol = eth_type_trans(skb,lip_dev->common.dev);
		wplip_data_rx_up(lip_dev,skb);
		return 0;
	}	
#endif

	wan_skb_set_raw(skb);

	switch (type){

	case WPLIP_RAW:

                if ((lip_dev->protocol == WANCONFIG_LIP_ATM) &&
		     lip_dev->common.usedby != API) {
			/* If ATM is not in API mode
			   Dont pass RAW Cells up the stack */
                	wan_skb_free(skb);   
			break;	
		}
		
		wplip_data_rx_up(lip_dev,skb);
		break;
		
	case WPLIP_IP:
		wan_skb_set_protocol(skb,ETH_P_IP);
		wplip_data_rx_up(lip_dev,skb);
		break;

	case WPLIP_IPV6:
		wan_skb_set_protocol(skb,ETH_P_IPV6);
		wplip_data_rx_up(lip_dev,skb);
		break;

#if defined(__LINUX__)
	case WPLIP_PPP:
		wan_skb_set_protocol(skb,ETH_P_WAN_PPP);
		wplip_data_rx_up(lip_dev,skb);
		break;
#endif

	case WPLIP_IPX:

#ifdef WPLIP_IPX_SUPPORT
		err=wplip_handle_ipxwan(lip_dev, skb);
		if (err > 0){
			/* IPX request we must re-send
			 * the skb buffer*/
			
			err=wplip_prot_tx(lip_dev, NULL, skb, WPLIP_IPX);
			if (err){
				if (WAN_NET_RATELIMIT()){
					DEBUG_EVENT("%s: IPX Reply dropped, if busy!\n",lip_dev->name);
				}
				wan_skb_free(skb);
				break;
			}
			break;
		}

		wan_skb_set_protocol(skb,ETH_P_IPX);
		
		wplip_data_rx_up(lip_dev,skb);
#else
		err=0;
		DEBUG_EVENT("%s: Rx IPX pkt not supported \n",lip_dev->name);
		wan_skb_free(skb);
#endif
		break;
		
	case WPLIP_FR_ARP:

		DEBUG_EVENT("%s: FR ARP not supported \n",lip_dev->name);
		wan_skb_free(skb);
		break;

	default:

		DEBUG_EVENT("%s: Unsupported Rx Packet 0x%X \n",
				lip_dev->name,type);
		wan_skb_free(skb);
		break;
	}

	return 0;
}




static char tmp[500];

int wplip_init_prot(void)
{
	int  offset=0;

	wplip_prot_iface_t *prot_iface;	

	wplip_prot_reg_ops.prot_set_state	= wplip_link_prot_change_state;
	wplip_prot_reg_ops.chan_set_state	= wplip_lipdev_prot_change_state;
	wplip_prot_reg_ops.mtu			= 1500;
	wplip_prot_reg_ops.tx_link_down		= wplip_link_callback_tx_down;
	wplip_prot_reg_ops.tx_chan_down		= wplip_callback_tx_down;
	wplip_prot_reg_ops.rx_up		= wplip_prot_rx_up;
	wplip_prot_reg_ops.get_ipv4_addr	= wplip_get_ipv4_addr;
	wplip_prot_reg_ops.set_ipv4_addr	= wplip_set_ipv4_addr;
	wplip_prot_reg_ops.kick_task	        = wplip_callback_kick_prot_task;
#if 0
	wplip_prot_reg_ops.set_hw_idle_frame 	= wplip_set_hw_idle_frame;
#endif
	
	sprintf(&tmp[0]," No Protocol Compiled\n");
	memset(&wplip_prot_ops,0,sizeof(wplip_prot_ops));

	offset = 0;
	prot_iface = NULL;
	/* FR initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_FR
	offset+=sprintf(&tmp[offset],"%s ","FR");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_FR]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_fr_prot;
	prot_iface->prot_link_unregister	= wp_unregister_fr_prot; 
	prot_iface->prot_chan_register		= wp_register_fr_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_fr_chan;
	prot_iface->open_chan			= wp_fr_open_chan;
	prot_iface->close_chan			= wp_fr_close_chan;
	prot_iface->tx				= wp_fr_tx;
	prot_iface->ioctl			= wp_fr_ioctl; 
	prot_iface->pipemon			= wp_fr_pipemon;
	prot_iface->rx				= wp_fr_rx; 
	prot_iface->timer			= wp_fr_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= wp_fr_snmp;
#endif

	/* PPP initialization */

#if defined(CONFIG_PRODUCT_WANPIPE_CHDLC) || defined(CONFIG_PRODUCT_WANPIPE_PPP)
	offset+=sprintf(&tmp[offset],"%s ","PPP");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_PPP]=prot_iface;

	prot_iface->init			=1;
	prot_iface->prot_link_register 		= wp_register_ppp_prot;
	prot_iface->prot_link_unregister	= wp_unregister_sppp_prot; 
	prot_iface->prot_chan_register		= wp_register_sppp_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_sppp_chan;
	prot_iface->open_chan			= wp_sppp_open;
	prot_iface->close_chan			= wp_sppp_close; 
	prot_iface->tx				= wp_sppp_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= wp_sppp_pipemon;
	prot_iface->rx				= wp_sppp_rx; 
	prot_iface->timer			= wp_sppp_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
	prot_iface->task			= wp_sppp_task;


	/* CHDLC initialization */

	offset+=sprintf(&tmp[offset],"%s ","CHDLC");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_CHDLC]=prot_iface;

	prot_iface->init			=1;
	prot_iface->prot_link_register 		= wp_register_chdlc_prot;
	prot_iface->prot_link_unregister	= wp_unregister_sppp_prot; 
	prot_iface->prot_chan_register		= wp_register_sppp_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_sppp_chan;
	prot_iface->open_chan			= wp_sppp_open;
	prot_iface->close_chan			= wp_sppp_close; 
	prot_iface->tx				= wp_sppp_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= wp_sppp_pipemon;
	prot_iface->rx				= wp_sppp_rx; 
	prot_iface->timer			= wp_sppp_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
#endif

	/* XDLC initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_XDLC
	offset+=sprintf(&tmp[offset],"%s ","XDLC");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_XDLC]=prot_iface;
	
	
	prot_iface->init			= 1;	
	prot_iface->prot_link_register 		= wp_reg_xdlc_prot;
	prot_iface->prot_link_unregister	= wp_unreg_xdlc_prot; 
	prot_iface->prot_chan_register		= wp_reg_xdlc_addr;	
	prot_iface->prot_chan_unregister	= wp_unreg_xdlc_addr;
	prot_iface->open_chan			= wp_xdlc_open;
	prot_iface->close_chan			= wp_xdlc_close; 
	prot_iface->tx				= wp_xdlc_tx;
	prot_iface->ioctl			= wp_xdlc_ioctl; 
	prot_iface->pipemon			= NULL;
	prot_iface->rx				= wp_xdlc_rx; 
	prot_iface->timer			= wp_xdlc_timer;
	prot_iface->bh				= wp_xdlc_bh;
	prot_iface->snmp			= NULL;
#endif

	/* LAPB initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_LIP_LAPB
	offset+=sprintf(&tmp[offset],"%s ","LIP_LAPB");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_LAPB]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_lapb_prot;
	prot_iface->prot_link_unregister	= wp_unregister_lapb_prot; 
	prot_iface->prot_chan_register		= wp_register_lapb_chan_prot;	
	prot_iface->prot_chan_unregister	= wp_unregister_lapb_chan_prot;
	prot_iface->open_chan			= wp_lapb_open;
	prot_iface->close_chan			= wp_lapb_close; 
	prot_iface->tx				= wp_lapb_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= NULL;
	prot_iface->rx				= wp_lapb_rx; 
	prot_iface->timer			= wp_lapb_timer;
	prot_iface->bh				= wp_lapb_bh;
	prot_iface->snmp			= NULL;
#endif
	
	/* LAPD initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_LIP_LAPD
	offset+=sprintf(&tmp[offset],"%s ","LIP_LAPD");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_LAPD]=prot_iface;
	
	prot_iface->init                        = 1;
	prot_iface->prot_link_register          = wp_register_lapd_prot;
	prot_iface->prot_link_unregister        = wp_unregister_lapb_prot;
	prot_iface->prot_chan_register          = wp_register_lapb_chan_prot;
	prot_iface->prot_chan_unregister        = wp_unregister_lapb_chan_prot;
	prot_iface->open_chan                   = wp_lapb_open;
	prot_iface->close_chan                  = wp_lapb_close;
	prot_iface->tx                          = wp_lapb_tx;
	prot_iface->ioctl                       = NULL;
	prot_iface->pipemon                     = NULL;
	prot_iface->rx                          = wp_lapb_rx;
	prot_iface->timer                       = wp_lapb_timer;
	prot_iface->bh                          = wp_lapb_bh;
	prot_iface->snmp                        = NULL;
#endif
	

	/* XMTP2 initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_XMTP2
	offset+=sprintf(&tmp[offset],"%s ","XMTP2");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_XMTP2]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_xmtp2_prot;
	prot_iface->prot_link_unregister	= wp_unregister_xmtp2_prot; 
	prot_iface->prot_chan_register		= wp_register_xmtp2_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_xmtp2_chan;
	prot_iface->open_chan			= wp_xmtp2_open;
	prot_iface->close_chan			= wp_xmtp2_close; 
	prot_iface->tx				= wp_xmtp2_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= NULL;
	prot_iface->rx				= wp_xmtp2_rx; 
	prot_iface->timer			= wp_xmtp2_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
#endif


	/* LIP HDLC initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_LIP_HDLC
	offset+=sprintf(&tmp[offset],"%s ","LIP_HDLC");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_LIP_HDLC]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_lip_hdlc_prot;
	prot_iface->prot_link_unregister	= wp_unregister_lip_hdlc_prot; 
	prot_iface->prot_chan_register		= wp_register_lip_hdlc_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_lip_hdlc_chan;
	prot_iface->open_chan			= wp_lip_hdlc_open;
	prot_iface->close_chan			= wp_lip_hdlc_close; 
	prot_iface->tx				= wp_lip_hdlc_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= wp_lip_hdlc_pipemon;
	prot_iface->rx				= wp_lip_hdlc_rx; 
	prot_iface->timer			= wp_lip_hdlc_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
#endif


	/* XMTP2 initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_VISDN_LAPD
	offset+=sprintf(&tmp[offset],"%s ","VISDN_LAPD");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_LAPD]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_lapd_prot;
	prot_iface->prot_link_unregister	= wp_unregister_lapd_prot; 
	prot_iface->prot_chan_register		= wp_register_lapd_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_lapd_chan;
	prot_iface->open_chan			= wp_lapd_open;
	prot_iface->close_chan			= wp_lapd_close; 
	prot_iface->tx				= wp_lapd_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= NULL;
	prot_iface->rx				= wp_lapd_rx; 
	prot_iface->timer			= wp_lapd_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
#endif
 
	/* KATM initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_LIP_KATM
	offset+=sprintf(&tmp[offset],"%s ","LIP_KATM");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_LIP_KATM]=prot_iface;
	 
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_katm_prot;
	prot_iface->prot_link_unregister	= wp_unregister_katm_prot; 
	prot_iface->prot_chan_register		= wp_register_katm_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_katm_chan;
	prot_iface->open_chan			= wp_katm_open;
	prot_iface->close_chan			= wp_katm_close; 
	prot_iface->tx				= wp_katm_tx;
	prot_iface->ioctl			= NULL; 
	prot_iface->pipemon			= NULL;
	prot_iface->rx				= wp_katm_rx; 
	prot_iface->timer			= wp_katm_timer;
	prot_iface->bh				= wpkatm_bh;
	prot_iface->snmp			= NULL;
#endif   

	/* X25 initialization */
#ifdef CONFIG_PRODUCT_WANPIPE_LIP_X25
	offset+=sprintf(&tmp[offset],"%s ","X25");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_X25]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_x25_prot;
	prot_iface->prot_link_unregister	= wp_unregister_x25_prot; 
	prot_iface->prot_chan_register		= wp_register_x25_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_x25_chan;
	prot_iface->open_chan			= wp_x25_open_chan;
	prot_iface->close_chan			= wp_x25_close_chan;
	prot_iface->tx				= wp_x25_tx;
	prot_iface->ioctl			= wp_x25_ioctl; 
	prot_iface->pipemon			= wp_x25_pipemon;
	prot_iface->rx				= wp_x25_rx; 
	prot_iface->timer			= wp_x25_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
#endif

	/* LIP ATM initialization */
#ifdef 	CONFIG_PRODUCT_WANPIPE_LIP_ATM
	offset+=sprintf(&tmp[offset],"%s ","LIP_ATM");
	prot_iface=wan_kmalloc(sizeof(wplip_prot_iface_t));
	if (!prot_iface){
		return -ENOMEM;
	}
	memset(prot_iface,0,sizeof(wplip_prot_iface_t));
	wplip_prot_ops[WANCONFIG_LIP_ATM]=prot_iface;
	
	prot_iface->init			= 1;
	prot_iface->prot_link_register 		= wp_register_atm_prot;
	prot_iface->prot_link_unregister	= wp_unregister_atm_prot; 
	prot_iface->prot_chan_register		= wp_register_atm_chan;	
	prot_iface->prot_chan_unregister	= wp_unregister_atm_chan;
	prot_iface->open_chan			= wp_atm_open_chan;
	prot_iface->close_chan			= wp_atm_close_chan;
	prot_iface->tx				= wp_atm_tx;
	prot_iface->ioctl			= wp_atm_ioctl; 
	prot_iface->pipemon			= wp_atm_pipemon;
	prot_iface->rx				= wp_atm_rx; 
	prot_iface->timer			= wp_atm_timer;
	prot_iface->bh				= NULL;
	prot_iface->snmp			= NULL;
#endif

	DEBUG_EVENT("WanpipeLIP: Protocols: %s\n",tmp);
	return 0;
}


int wplip_free_prot(void)
{
	int i;

	for (i=0;i<MAX_LIP_PROTOCOLS;i++){
		if (wplip_prot_ops[i]){
			wan_free(wplip_prot_ops[i]);
			wplip_prot_ops[i]=NULL;
		}
	}

	return 0;
}

