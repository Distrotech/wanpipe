/*************************************************************
 * wanpipe_lip.c   WANPIPE Link Interface Protocol Layer (LIP)
 *
 *
 *
 * ===========================================================
 *
 * Oct 06 2009  David Rokhvarg	Modifications for Sangoma
 *				Windows Device driver.
 *
 * Feb 09 2007  Joel M. Pareja	Added link state notification 
 *				for NETGRAPH failover support.
 *
 * Dec 02 2003	Nenad Corbic	Initial Driver
 */


/*=============================================================
 * Includes
 */

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <wanpipe_lip.h>
#elif defined(__LINUX__)
#include <linux/wanpipe_lip.h>
#elif defined(__WINDOWS__)
#include "wanpipe_lip.h"
#endif


/*=============================================================
 * Definitions
 */

/*=============================================================
 * Global Parameters
 */
/* Function interface between LIP layer and kernel */
extern wan_iface_t wan_iface;

#if defined(NETGRAPH)
extern void wan_ng_link_state(wanpipe_common_t *common, int state);
#endif

struct wplip_link_list list_head_link;
wan_rwlock_t wplip_link_lock;
wan_bitmap_t wplip_link_num[MAX_LIP_LINKS];
#if 0
int gdbg_flag=0;
#endif

#undef WAN_DEBUG_MEM_LIP

/*=============================================================
 * Function Prototypes
 */

static int wplip_if_unreg (netdevice_t *dev);
static int wplip_bind_link(void *lip_id,netdevice_t *dev);
static int wplip_unbind_link(void *lip_id,netdevice_t *dev);
static void wplip_disconnect(void *wplip_id,int reason);
static void wplip_connect(void *wplip_id,int reason);
static int wplip_rx(void *wplip_id, void *skb);
static int wplip_unreg(void *reg_ptr);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static void wplip_if_task (void *arg, int dummy);
#elif defined(__WINDOWS__)
static void wplip_if_task (IN PKDPC Dpc, IN PVOID arg, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
#else
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 	
static void wplip_if_task (void *arg);
# else
static void wplip_if_task (struct work_struct *work);
# endif
#endif    

extern int register_wanpipe_lip_protocol (wplip_reg_t *lip_reg);
extern void unregister_wanpipe_lip_protocol (void);


/*=============================================================
 * Global Module Interface Functions
 */

/* This should only exist in sdladrv */
# if defined(WAN_DEBUG_MEM_LIP)
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define EXPORT_SYMBOL(symbol)
#endif
static int wan_debug_mem;

static wan_spinlock_t wan_debug_mem_lock;

WAN_LIST_HEAD(NAME_PLACEHOLDER_MEM, sdla_memdbg_el) sdla_memdbg_head = 
			WAN_LIST_HEAD_INITIALIZER(&sdla_memdbg_head);

typedef struct sdla_memdbg_el
{
	unsigned int len;
	unsigned int line;
	char cmd_func[128];
	void *mem;
	WAN_LIST_ENTRY(sdla_memdbg_el)	next;
}sdla_memdbg_el_t;

static int wanpipe_lip_memdbg_init(void);
static int wanpipe_lip_memdbg_free(void);

static int wanpipe_lip_memdbg_init(void)
{
	wan_spin_lock_init(&wan_debug_mem_lock,"wan_debug_mem_lock");
	WAN_LIST_INIT(&sdla_memdbg_head);
	return 0;
}


int sdla_memdbg_push(void *mem, const char *func_name, const int line, int len)
{
	sdla_memdbg_el_t *sdla_mem_el = NULL;
	wan_smp_flag_t flags;

#if defined(__LINUX__)
	sdla_mem_el = kmalloc(sizeof(sdla_memdbg_el_t),GFP_ATOMIC);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	sdla_mem_el = malloc(sizeof(sdla_memdbg_el_t), M_DEVBUF, M_NOWAIT); 
#endif
	if (!sdla_mem_el) {
		DEBUG_EVENT("%s:%d Critical failed to allocate memory!\n",
			__FUNCTION__,__LINE__);
		return -ENOMEM;
	}

	memset(sdla_mem_el,0,sizeof(sdla_memdbg_el_t));
		
	sdla_mem_el->len=len;
	sdla_mem_el->line=line;
	sdla_mem_el->mem=mem;
	strncpy(sdla_mem_el->cmd_func,func_name,sizeof(sdla_mem_el->cmd_func)-1);
	
	wplip_spin_lock_irq(&wan_debug_mem_lock,&flags);
	wan_debug_mem+=sdla_mem_el->len;
	WAN_LIST_INSERT_HEAD(&sdla_memdbg_head, sdla_mem_el, next);
	wplip_spin_unlock_irq(&wan_debug_mem_lock,&flags);

	DEBUG_EVENT("%s:%d: Alloc %p Len=%i Total=%i\n",
			sdla_mem_el->cmd_func,sdla_mem_el->line,
			 sdla_mem_el->mem, sdla_mem_el->len,wan_debug_mem);
	return 0;

}
EXPORT_SYMBOL(sdla_memdbg_push);

int sdla_memdbg_pull(void *mem, const char *func_name, const int line)
{
	sdla_memdbg_el_t *sdla_mem_el;
	wan_smp_flag_t flags;
	int err=-1;

	wplip_spin_lock_irq(&wan_debug_mem_lock,&flags);

	WAN_LIST_FOREACH(sdla_mem_el, &sdla_memdbg_head, next){
		if (sdla_mem_el->mem == mem) {
			break;
		}
	}

	if (sdla_mem_el) {
		
		WAN_LIST_REMOVE(sdla_mem_el, next);
		wan_debug_mem-=sdla_mem_el->len;
		wplip_spin_unlock_irq(&wan_debug_mem_lock,&flags);

		DEBUG_EVENT("%s:%d: DeAlloc %p Len=%i Total=%i (From %s:%d)\n",
			func_name,line,
			sdla_mem_el->mem, sdla_mem_el->len, wan_debug_mem,
			sdla_mem_el->cmd_func,sdla_mem_el->line);
#if defined(__LINUX__)
		kfree(sdla_mem_el);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
		free(sdla_mem_el, M_DEVBUF); 
#endif
		err=0;
	} else {
		wplip_spin_unlock_irq(&wan_debug_mem_lock,&flags);
	}

	if (err) {
		DEBUG_EVENT("%s:%d: Critical Error: Unknows Memeory %p\n",
			__FUNCTION__,__LINE__,mem);
	}

	return err;
}
EXPORT_SYMBOL(sdla_memdbg_pull);

static int wanpipe_lip_memdbg_free(void)
{
	sdla_memdbg_el_t *sdla_mem_el;
	int total=0;

	DEBUG_EVENT("wanpipe_lip: Memory Still Allocated=%i \n",
			 wan_debug_mem);

	DEBUG_EVENT("=====================BEGIN================================\n");

	sdla_mem_el = WAN_LIST_FIRST(&sdla_memdbg_head);
	while(sdla_mem_el){
		sdla_memdbg_el_t *tmp = sdla_mem_el;

		DEBUG_EVENT("%s:%d: Mem Leak %p Len=%i \n",
			sdla_mem_el->cmd_func,sdla_mem_el->line,
			sdla_mem_el->mem, sdla_mem_el->len);
		total+=sdla_mem_el->len;

		sdla_mem_el = WAN_LIST_NEXT(sdla_mem_el, next);
		WAN_LIST_REMOVE(tmp, next);
#if defined(__LINUX__)
		kfree(tmp);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
		free(tmp, M_DEVBUF); 
#endif
	}

	DEBUG_EVENT("=====================END==================================\n");
	DEBUG_EVENT("wanpipe_lip: Memory Still Allocated=%i  Leaks Found=%i Missing=%i\n",
			 wan_debug_mem,total,wan_debug_mem-total);

	return 0;
}

# endif


/*=============================================================
 * wplip_register
 *
 * Description:
 *	
 * Usedby:
 */
/* EXPORT_SYMBOL(wplip_register); */
static int wplip_register (void **lip_link_ptr, wanif_conf_t *conf, char *devname)
{
	wplip_link_t *lip_link = (wplip_link_t*)*lip_link_ptr;

	/* Create the new X25 link for this Lapb 
	 * connection */
	if (lip_link){
		return -EEXIST;
	}

	lip_link = wplip_create_link(devname);
	if (!lip_link){
		DEBUG_EVENT("%s: LIP register: Failed to create link\n",
				MODNAME);
		return -ENOMEM;
	}
	
	DEBUG_TEST("%s: Registering LIP Link\n",lip_link->name);

	
	if (conf){
		int err=wplip_reg_link_prot(lip_link,conf);
		if (err){
			wplip_free_link(lip_link);	
			return err;
		}
	}
	
	wplip_insert_link(lip_link);

	lip_link->state = WAN_DISCONNECTED;
	lip_link->carrier_state = WAN_DISCONNECTED;

	lip_link->latency_qlen=100;

	*lip_link_ptr = lip_link;

#if defined(__FreeBSD__) && defined(WPLIP_TQ_THREAD)
	lip_link->tq = taskqueue_create_fast("wplip_taskq",M_NOWAIT,
				taskqueue_thread_enqueue, &lip_link->tq);
	taskqueue_start_threads(&lip_link->tq,1,PI_NET,"%s taskq",devname);
#endif
	return 0;	
}

/*==============================================================
 * wplip_unreg
 *
 * Description:
 *	This function is called during system setup to 
 *	remove the whole x25 link and all x25 svc defined
 *	within the x25 link.
 *
 *	For each x25 link and x25 svc the proc file
 *	entry is removed.
 *	
 * Usedby:
 * 	Lapb layer. 
 */

static int wplip_unreg(void *lip_link_ptr)
{
	wplip_link_t *lip_link = (wplip_link_t*)lip_link_ptr;
	wplip_dev_t *lip_dev;
	wplip_dev_list_t *lip_dev_list_el; 
	int err;
	
	if (wplip_link_exists(lip_link) != 0){
		return -ENODEV;
	}

#if defined(__FreeBSD__) && defined(WPLIP_TQ_THREAD)
	if (lip_link->tq){
		WAN_TASKQUEUE_DRAIN(lip_link->tq, &lip_link->task);
		WAN_TASKQUEUE_FREE(lip_link->tq);
		lip_link->tq = NULL;
	}
#endif

#ifdef WPLIP_TTY_SUPPORT
	if (lip_link->tty_opt && lip_link->tty_open){
		tty_hangup(lip_link->tty);
		return -EBUSY;
	}
#endif
	
	wan_del_timer(&lip_link->prot_timer);

	wan_set_bit(WPLIP_LINK_DOWN,&lip_link->tq_working);
	while((lip_dev = WAN_LIST_FIRST(&lip_link->list_head_ifdev)) != NULL){
		
		DEBUG_EVENT("%s: Unregistering dev %s\n",
				lip_link->name,lip_dev->name);
	

		err=wplip_if_unreg(lip_dev->common.dev);
		if (err<0){
			wan_clear_bit(WPLIP_LINK_DOWN,&lip_link->tq_working);
			return err;
		}
	}

	
	while((lip_dev_list_el = WAN_LIST_FIRST(&lip_link->list_head_tx_ifdev)) != NULL){
	
		DEBUG_EVENT("%s: Unregistering master dev %s\n",
				lip_link->name,
				wan_netif_name(lip_dev_list_el->dev));

		WAN_DEV_PUT(lip_dev_list_el->dev);
		lip_dev_list_el->dev=NULL;
		
		WAN_LIST_REMOVE(lip_dev_list_el,list_entry);
		lip_link->tx_dev_cnt--;

		wan_free(lip_dev_list_el);
		lip_dev_list_el=NULL;
	}


	if (lip_link->protocol){
		wplip_unreg_link_prot(lip_link);
	}
	
	wplip_remove_link(lip_link);
	wplip_free_link(lip_link);

	return 0;
}

#if defined(__WINDOWS__)
static int wplip_if_reg(void *lip_link_ptr, netdevice_t *dev, wanif_conf_t *conf)
#else
static int wplip_if_reg(void *lip_link_ptr, char *dev_name, wanif_conf_t *conf)
#endif
{		
	wplip_link_t *lip_link = (wplip_link_t*)lip_link_ptr;
	wplip_dev_t *lip_dev;
	int usedby=0;
	int err;
#if defined(__WINDOWS__)
	const char *dev_name = dev->name;
#endif

	if (!conf){
		DEBUG_EVENT("%s: LIP DEV: If Registartion without configuration!\n",dev_name);
		return -EINVAL;
	}

#ifdef WPLIP_TTY_SUPPORT
	if (lip_link->tty_opt){
		return 0;
	}
#endif
	
	if (wplip_link_exists(lip_link) != 0){
		DEBUG_EVENT("%s: LIP: Invalid Link !\n",
				dev_name);
		return -EINVAL;
	}
		
	if (dev_name == NULL){
		DEBUG_EVENT("%s: LIP: Invalid device name : NULL!\n",
				lip_link->name);
		return -EINVAL;
	}

	if (!wplip_lipdev_exists(lip_link,dev_name)){
		DEBUG_EVENT("%s: LIP: Invalid lip link device %s!\n",
				__FUNCTION__,dev_name);
		return -EEXIST;
	}
	
	if(strcmp(conf->usedby, "API") == 0) {
		usedby = API;
	}else if(strcmp(conf->usedby, "WANPIPE") == 0){
		usedby = WANPIPE;
	}else if(strcmp(conf->usedby, "STACK") == 0){
		usedby = STACK;
	}else if(strcmp(conf->usedby, "BRIDGE") == 0){
		usedby = BRIDGE;
	}else if(strcmp(conf->usedby, "BRIDGE_NODE") == 0){
		usedby = BRIDGE_NODE;
#if defined(__OpenBSD__)
	}else if(strcmp(conf->usedby, "TRUNK") == 0){
		usedby = TRUNK;
#endif		
#if defined(__FreeBSD__)
	}else if(strcmp(conf->usedby, "NETGRAPH") == 0){
		usedby = WP_NETGRAPH;
#endif		
	}else{
		DEBUG_EVENT( "%s: LIP device invalid 'usedby': %s\n",
				dev_name, conf->usedby);
		return -EINVAL;
	}

#if defined(__WINDOWS__)
	if ((lip_dev = wplip_create_lipdev(dev, usedby)) == NULL){
#else
	if ((lip_dev = wplip_create_lipdev(dev_name, usedby)) == NULL){
#endif
		DEBUG_EVENT("%s: LIP: Failed to create lip priv device %s\n",
				lip_link->name,dev_name);
		return -ENOMEM;
	}

#if defined(__LINUX__)
	if (conf->mtu){
       	lip_dev->common.dev->mtu = conf->mtu; 
	}
#endif
	
	WAN_HOLD(lip_link);
	lip_dev->lip_link 	= lip_link;
	lip_dev->protocol	= conf->protocol;
	lip_dev->common.usedby 	= usedby;
	lip_dev->common.state	= WAN_DISCONNECTED;
    
   	WAN_NETIF_CARRIER_OFF(lip_dev->common.dev);

#if defined(__LINUX__)
	if (conf->true_if_encoding){
		DEBUG_EVENT("%s: LIP: Setting IF Type to Broadcast\n",dev_name);
		lip_dev->common.dev->flags &= ~IFF_POINTOPOINT;		
		lip_dev->common.dev->flags |= IFF_BROADCAST;		
	}
#endif

	switch (usedby){

	case API:
		wan_reg_api(lip_dev, lip_dev->common.dev, lip_link->name);
		DEBUG_EVENT( "%s: Running in API mode\n",
				lip_dev->name);
		break;
	case WANPIPE:
		DEBUG_EVENT( "%s: Running in WANPIPE mode\n",
				lip_dev->name);
		break;
	case STACK:
		DEBUG_EVENT( "%s: Running in STACK mode\n",
				lip_dev->name);
		break;

	case BRIDGE:
		DEBUG_EVENT( "%s: Running in BRIDGE mode\n",
				lip_dev->name);
		break;

	case BRIDGE_NODE:
		DEBUG_EVENT( "%s: Running in BRIDGE Node mode\n",
				lip_dev->name);
		break;

	case TRUNK:
		DEBUG_EVENT( "%s: Running in TRUNK mode\n",
				lip_dev->name);
		break;

	case WP_NETGRAPH:
		DEBUG_EVENT( "%s: Running in NETGRAPH mode\n",
				lip_dev->name);
		break;

	default:
		DEBUG_EVENT( "%s: LIP device invalid 'usedby': %s\n",
				lip_dev->name, conf->usedby);
		__WAN_PUT(lip_link);
		lip_dev->lip_link = NULL;
		wplip_free_lipdev(lip_dev);
		return -EINVAL;
	}

	lip_dev->ipx_net_num = conf->network_number;
	if (lip_dev->ipx_net_num) {
		DEBUG_EVENT("%s: IPX Network Number = 0x%lX\n",
				lip_dev->name, lip_dev->ipx_net_num);
	}
	
	lip_dev->max_mtu_sz=MAX_TX_BUF;
	lip_dev->max_mtu_sz_orig=MAX_TX_BUF;
	
	err=wplip_reg_lipdev_prot(lip_dev,conf);
	if (err){
		__WAN_PUT(lip_link);
		lip_dev->lip_link = NULL;
		wplip_free_lipdev(lip_dev);
		return err;
	}
	
	wplip_insert_lipdev(lip_link,lip_dev);
	WAN_TASKQ_INIT((&lip_dev->if_task),0,wplip_if_task,lip_dev);
	
	err=wplip_open_lipdev_prot(lip_dev);
	if (err){
		wplip_remove_lipdev(lip_link,lip_dev);
		__WAN_PUT(lip_link);
		lip_dev->lip_link = NULL;
		wplip_free_lipdev(lip_dev);
		return err;
	}
	
#ifdef __LINUX__	
	if (conf->if_down && usedby != API && usedby != STACK){
		wan_set_bit(DYN_OPT_ON,&lip_dev->interface_down);
		DEBUG_EVENT("%s: Dynamic interface configuration enabled\n",
			   dev_name);
	}else{
		wan_clear_bit(DYN_OPT_ON,&lip_dev->interface_down);
	}
	lip_link->latency_qlen=lip_dev->common.dev->tx_queue_len;
	
#endif

	
	if (lip_link->state == WAN_CONNECTED){
		DEBUG_TEST("%s: LIP CREATE Link already on!\n",
						lip_dev->name);
		WAN_NETIF_CARRIER_ON(lip_dev->common.dev);
	       	WAN_NETIF_WAKE_QUEUE(lip_dev->common.dev);
	       	wplip_trigger_bh(lip_dev->lip_link);
	}         

	DEBUG_TEST("%s: LIP LIPDEV Created %p Magic 0x%lX\n",
			lip_link->name,
			lip_dev,
			lip_dev->magic);

	return 0;
}

/*==============================================================
 * wplip_if_unreg
 *
 * Description:
 *	This function is called during system setup to 
 *	remove the x25 link and each x25 svc defined
 *	in the wanpipe configuration file.
 *
 *	For each x25 link and x25 svc the proc file
 *	entry is removed.
 *	
 * Usedby:
 * 	Lapb layer. 
 */

static int wplip_if_unreg (netdevice_t *dev)
{
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);
	wplip_link_t *lip_link = NULL;
	wplip_link_t *stack_lip_link = NULL;
	wan_smp_flag_t flags;

	if (!lip_dev)
		return -ENODEV;

	/* under windows interface is always up, disregard this flag */
#ifndef __WINDOWS__
	if (WAN_NETIF_UP(dev)){
		DEBUG_EVENT("%s: Failed to unregister: Device UP!\n",
				wan_netif_name(dev));
		return -EBUSY;
	}
#endif
	lip_link = lip_dev->lip_link;
	if (wplip_link_exists(lip_link) != 0){
		DEBUG_EVENT("%s: Failed to unregister: no link device\n",
				wan_netif_name(dev));
		return -ENODEV;
	}

	/* Check for a Higher Lip Link layer attached 
	 * to this device. If exists, call unregister to
	 * remove it before unregistering this device */	
	stack_lip_link=lip_dev->common.lip; /*STACK*/
	if (stack_lip_link){
		int err;
		DEBUG_TEST("%s: IF UNREG: Calling Top Layer LIP UNREG\n",
				lip_dev->name);
		err=wplip_unreg(stack_lip_link);
		if (err){
			return err;
		}
	}

	wan_set_bit(WPLIP_DEV_UNREGISTER,&lip_dev->critical);
	wan_clear_bit(WAN_DEV_READY,&lip_dev->interface_down);

	wplip_close_lipdev_prot(lip_dev);
	
	wplip_spin_lock_irq(&lip_link->bh_lock,&flags);
	lip_link->cur_tx=NULL;
	wan_skb_queue_purge(&lip_dev->tx_queue);
	wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
	
	DEBUG_EVENT("%s: Unregistering LIP device\n",
				wan_netif_name(dev));

	if (lip_dev->common.prot_ptr){
		wplip_unreg_lipdev_prot(lip_dev);	
	}

	if (lip_dev->common.usedby == API){
		wan_unreg_api(lip_dev, lip_link->name);
	}

	wplip_remove_lipdev(lip_link,lip_dev);
	
	__WAN_PUT(lip_dev->lip_link);
	lip_dev->lip_link=NULL;
	
	wplip_free_lipdev(lip_dev);
	
	return 0;
}



static int wplip_bind_link(void *lip_id,netdevice_t *dev)
{

	wplip_link_t *lip_link = (wplip_link_t*)lip_id;
	wplip_dev_list_t  *lip_dev_list_el;
	wan_smp_flag_t	flags;
	wanpipe_common_t *common = (wanpipe_common_t*)wan_netif_priv(dev);
	
	if (!lip_id){
		return -ENODEV;
	}

	if (wplip_link_exists(lip_link) != 0){
		return -ENODEV;
	}
	
	if (wan_test_bit(WPLIP_LINK_DOWN,&lip_link->tq_working)){
		return -ENETDOWN;
	}

	lip_dev_list_el=wan_malloc(sizeof(wplip_dev_list_t));
	memset(lip_dev_list_el,0,sizeof(wplip_dev_list_t));

	lip_dev_list_el->magic=WPLIP_MAGIC_DEV_EL;
	
	WAN_DEV_HOLD(dev);

	/* Check if lower layer is connected. We must do this
	   in case the lower layer is already connected on startup.
	   Otherwise we can get into a condition where LIP layer is down
	   and lower layer is UP */
	if (common && common->state == WAN_CONNECTED) {
		lip_link->carrier_state = WAN_CONNECTED;
	}

	lip_dev_list_el->dev=dev;
	
	wplip_spin_lock_irq(&lip_link->bh_lock,&flags);

	WAN_LIST_INSERT_HEAD(&lip_link->list_head_tx_ifdev,lip_dev_list_el,list_entry);
	lip_link->tx_dev_cnt++;

	wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

	return 0;
}

static int wplip_unbind_link(void *lip_id,netdevice_t *dev)
{
	wplip_link_t *lip_link = (wplip_link_t*)lip_id;
	wplip_dev_list_t *lip_dev_list_el=NULL;
	wan_smp_flag_t	flags;
	int err=-ENODEV;
	
	if (!lip_id){
		return -EFAULT;
	}

	if (wplip_link_exists(lip_link) != 0){
		return -EFAULT;
	}
	

	wplip_spin_lock_irq(&lip_link->bh_lock,&flags);

	WAN_LIST_FOREACH(lip_dev_list_el,&lip_link->list_head_tx_ifdev,list_entry){
		if (lip_dev_list_el->dev == dev){
			WAN_LIST_REMOVE(lip_dev_list_el,list_entry);
			lip_link->tx_dev_cnt--;
			err=0;
			break;
		}
	}	
	
	wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

	if (err==0){
		WAN_DEV_PUT(lip_dev_list_el->dev);
		lip_dev_list_el->dev=NULL;
		wan_free(lip_dev_list_el);
	}
	
	return err;
}





/*==============================================================
 * wplip_rx
 *
 * Description:
 *
 *	
 * Usedby:
 * 	Lower layer to pass us an rx packet.
 */

static int wplip_rx(void *wplip_id, void *skb)
{
	wplip_link_t *lip_link = (wplip_link_t*)wplip_id;

	DEBUG_RX("%s: LIP LINK %s() pkt=%d %p\n",
			lip_link->name,__FUNCTION__,
			wan_skb_len(skb),
			skb);

	if (wan_test_bit(WPLIP_LINK_DOWN,&lip_link->tq_working)){
		return -ENODEV;
	}

#ifdef WPLIP_TTY_SUPPORT
	if (lip_link->tty_opt){
		if (lip_link->tty && lip_link->tty_open){
			wplip_tty_receive(lip_link,skb);		
			wanpipe_tty_trigger_poll(lip_link);
			return 0;
		}else{
			return -ENODEV;
		}
	}
#endif	
	
	if (wan_skb_queue_len(&lip_link->rx_queue) > MAX_RX_Q){
		DEBUG_TEST("%s: Critical Rx Error: Rx buf overflow 0x%lX!\n",
				lip_link->name,
				lip_link->tq_working);
		wplip_trigger_bh(lip_link);
		return -ENOBUFS;
	}
	
	wan_skb_queue_tail(&lip_link->rx_queue,skb);	
	wplip_trigger_bh(lip_link);
	
	return 0;
}

/*==============================================================
 * wplip_data_rx_up
 *
 * Description:
 *	This function is used to pass data to the upper 
 *	layer. If the lip dev is used by TCP/IP the packet
 *	is passed up the the TCP/IP stack, otherwise
 *	it is passed to the upper protocol layer.
 *	
 * Locking:
 * 	This function will ALWAYS get called from
 * 	the BH handler with the bh_lock, thus
 * 	its protected.
 *
 * Usedby:
 */

int wplip_data_rx_up(wplip_dev_t* lip_dev, void *skb)
{
	int len=wan_skb_len(skb);

	DEBUG_TEST("LIP LINK %s() pkt=%i\n",
			__FUNCTION__,wan_skb_len(skb));

#if 0
	DEBUG_EVENT("%s: %s() Packet Len=%d (DEBUG DROPPED)\n",
			lip_dev->name, __FUNCTION__,wan_skb_len(skb));			

	wan_skb_free(skb);
	return 0;
#endif

	switch (lip_dev->common.usedby){

#if defined(__LINUX__)	
	case API:
		{
		unsigned char *buf;
		int err;

		if (wan_skb_headroom(skb) < sizeof(wan_api_rx_hdr_t)){
			DEBUG_EVENT("%s: Critical Error Rx pkt Hdrm=%d  < ApiHdrm=%d\n",
					lip_dev->name,wan_skb_headroom(skb),
					sizeof(wan_api_rx_hdr_t));
			wan_skb_free(skb);
			break;
		}
		
		buf=wan_skb_push(skb,sizeof(wan_api_rx_hdr_t));	
		memset(buf,0,sizeof(wan_api_rx_hdr_t));
	
		((netskb_t *)skb)->protocol = htons(PVC_PROT);
		wan_skb_reset_mac_header(((netskb_t *)skb));
		wan_skb_reset_network_header(((netskb_t *)skb));
		((netskb_t *)skb)->dev      = lip_dev->common.dev;
		((netskb_t *)skb)->pkt_type = WAN_PACKET_DATA;		

		if ((err=wan_api_rx(lip_dev,skb)) != 0){
#if 0
			if (net_ratelimit()){
				DEBUG_EVENT("%s: Error: Rx Socket busy err=%i!\n",
					lip_dev->name,err);
			}
#endif

#if 0
			#LAPB SHOULD PUSH BACK TO THE STACK
			wan_skb_pull(skb,sizeof(wan_api_rx_hdr_t));
#else
      			wan_skb_free(skb);
#endif
			WAN_NETIF_STATS_INC_RX_ERRORS(&lip_dev->common);	//lip_dev->ifstats.rx_errors++;		
			return 1;
		}else{
			WAN_NETIF_STATS_INC_RX_PACKETS(&lip_dev->common);	//lip_dev->ifstats.rx_packets++;
			WAN_NETIF_STATS_INC_RX_BYTES(&lip_dev->common,len);	//lip_dev->ifstats.rx_bytes+=len;
		}
		}
		break;
#endif	

#if !defined(__WINDOWS__)
	case STACK:
		if (lip_dev->common.lip){
			int err=wplip_rx(lip_dev->common.lip,skb);
			if (err){
				wan_skb_free(skb);
				WAN_NETIF_STATS_INC_RX_DROPPED(&lip_dev->common);	//lip_dev->ifstats.rx_dropped++;
			}else{
				WAN_NETIF_STATS_INC_RX_PACKETS(&lip_dev->common);	//lip_dev->ifstats.rx_packets++;
				WAN_NETIF_STATS_INC_RX_BYTES(&lip_dev->common,len);	//lip_dev->ifstats.rx_bytes+=len;
			}
		}else{
			wan_skb_free(skb);
			WAN_NETIF_STATS_INC_RX_DROPPED(&lip_dev->common);	//lip_dev->ifstats.rx_dropped++;
		}

		break;
#endif

	default:
		/* Windows note: rx data always handled by 'default' case! */
		if (wan_iface.input && wan_iface.input(lip_dev->common.dev, skb) == 0){
			WAN_NETIF_STATS_INC_RX_PACKETS(&lip_dev->common);	//lip_dev->ifstats.rx_packets++;
			WAN_NETIF_STATS_INC_RX_BYTES(&lip_dev->common,len);	//lip_dev->ifstats.rx_bytes += len;
		}else{
			wan_skb_free(skb);
			WAN_NETIF_STATS_INC_RX_DROPPED(&lip_dev->common);	//lip_dev->ifstats.rx_dropped++;
		}
			
		break;
	}
	return 0;
}


/*==============================================================
 * wplip_data_tx_down
 *
 * Description:
 *	This function is used to pass data down to the 
 *	lower layer. 
 *
 * Locking:
 * 	This function will ALWAYS get called from
 * 	the BH handler with the bh_lock, thus
 * 	its protected.
 *
 * Return Codes:
 *
 * 	0: Packet send successful, lower layer
 * 	   will deallocate packet
 *
 * 	Non 0: Packet send failed, upper layer
 * 	       must handle the packet
 *	
 */

int wplip_data_tx_down(wplip_link_t *lip_link, void *skb)
{
	wplip_dev_list_t *lip_dev_list_el; 	
	netdevice_t *dev;
	
	DEBUG_TEST("%s: LIP LINK %s() pkt=%d\n",
		lip_link->name,__FUNCTION__,wan_skb_len(skb));

	
	if (!lip_link->tx_dev_cnt){ 
		DEBUG_EVENT("%s(): %s: (1) Tx Dev List empty! dropping...\n",
				__FUNCTION__,lip_link->name);	
		return -ENODEV;
	}

	/* FIXME:
	 * For now, we can only transmit on a FIRST Tx device */
	lip_dev_list_el=WAN_LIST_FIRST(&lip_link->list_head_tx_ifdev);
	if (!lip_dev_list_el){
		DEBUG_EVENT("%s(): %s: (2) Tx Dev List empty! dropping...\n",
				__FUNCTION__,lip_link->name);	
		return -ENODEV;
	}
	
	if (lip_dev_list_el->magic != WPLIP_MAGIC_DEV_EL){
		DEBUG_EVENT("%s: %s: Error: Invalid dev magic number! dropping...\n",
				__FUNCTION__,lip_link->name);
		return -EFAULT;
	}

	dev=lip_dev_list_el->dev;
	if (!dev){
		DEBUG_EVENT("%s: %s: Error: No dev!  dropping...\n",
				__FUNCTION__,lip_link->name);
		return -ENODEV;
	}	

	if (WAN_NETIF_QUEUE_STOPPED(dev)){
		return -EBUSY;
	}

#if defined(__LINUX__) || defined(__WINDOWS__)
	if (lip_link->latency_qlen != dev->tx_queue_len) {
        	dev->tx_queue_len=lip_link->latency_qlen;
	}  

	return WAN_NETDEV_XMIT(skb,dev);
#else
	if (!(WAN_NETIF_QUEUE_STOPPED(dev)) && dev->if_output){
 		return dev->if_output(dev, skb, NULL,NULL);
	}
	wan_skb_free(skb);
	return 0;
#endif
}


int wplip_change_mtu(netdevice_t *dev, int new_mtu)
{
#if defined(__LINUX__)
	wplip_dev_t *lip_dev = wplip_netdev_get_lipdev(dev);
	wplip_link_t *lip_link = lip_dev->lip_link;
	netdevice_t *hw_dev;
	wplip_dev_list_t *lip_dev_list_el;
	int err = 0;
	
	if (!lip_link->tx_dev_cnt){ 
		DEBUG_EVENT("%s: %s: Tx Dev not available\n",
				__FUNCTION__,lip_link->name);	
		return -ENODEV;
	}

	/* FIXME:
	 * For now, we can only transmit on a FIRST Tx device */
	lip_dev_list_el=WAN_LIST_FIRST(&lip_link->list_head_tx_ifdev);
	if (!lip_dev_list_el){
		DEBUG_EVENT("%s: %s: Tx Dev List empty!\n",
				__FUNCTION__,lip_link->name);	
		return -ENODEV;
	}
	 
	if (lip_dev_list_el->magic != WPLIP_MAGIC_DEV_EL){
		DEBUG_EVENT("%s: %s: Error: Invalid dev magic number!\n",
				__FUNCTION__,lip_link->name);
		return -EFAULT;
	}

	hw_dev=lip_dev_list_el->dev;
	if (!hw_dev){
		DEBUG_EVENT("%s: %s: Error: No dev!  dropping...\n",
				__FUNCTION__,lip_link->name);
		return -ENODEV;
	}	
	
	if (WAN_NETDEV_TEST_MTU(hw_dev)) {
		err = WAN_NETDEV_CHANGE_MTU(hw_dev,new_mtu);
	}	

	if (err == 0) {
		dev->mtu = new_mtu;
	}

	return err;
#endif
	return 0;
}



/*==============================================================
 * wplip_link_prot_change_state
 *
 * Description:
 * 	The lower layer calls this function, when it
 * 	becomes disconnected.  
 */

int wplip_link_prot_change_state(void *wplip_id,int state, unsigned char *data, int len)
{
	wplip_link_t *lip_link = (wplip_link_t *)wplip_id;

	if (lip_link->prot_state != state){
		lip_link->prot_state=state;
	
		DEBUG_EVENT("%s: Lip Link Protocol State %s!\n",
			lip_link->name, STATE_DECODE(state));
		
		if (lip_link->prot_state == WAN_CONNECTED &&
		    lip_link->carrier_state == WAN_CONNECTED){
			DEBUG_EVENT("%s: Lip Link Connected!\n",
				lip_link->name);
			lip_link->state = WAN_CONNECTED;
		}

		if (lip_link->prot_state != WAN_CONNECTED){
			DEBUG_EVENT("%s: Lip Link Disconnected!\n",
				lip_link->name);
			lip_link->state = WAN_DISCONNECTED;
		}
	}
	
	return 0;
}



int wplip_lipdev_prot_update_state_change(wplip_dev_t *lip_dev, unsigned char *data, int len)
{
	int state = lip_dev->common.state;

	if (lip_dev->common.usedby == API) {

		if (data && len){
			wplip_prot_oob(lip_dev,data,len);
		}

		if (state == WAN_CONNECTED){
			WAN_NETIF_CARRIER_ON(lip_dev->common.dev);
			WAN_NETIF_START_QUEUE(lip_dev->common.dev);
		}else{
			WAN_NETIF_CARRIER_OFF(lip_dev->common.dev);
			WAN_NETIF_STOP_QUEUE(lip_dev->common.dev);
		}
			
		wan_update_api_state(lip_dev);

		wplip_trigger_bh(lip_dev->lip_link);

	}else if (lip_dev->common.lip) { /*STACK*/
		
		if (state == WAN_CONNECTED){
			WAN_NETIF_CARRIER_ON(lip_dev->common.dev);
			WAN_NETIF_START_QUEUE(lip_dev->common.dev);
			wplip_connect(lip_dev->common.lip,0);
		}else{
			WAN_NETIF_CARRIER_OFF(lip_dev->common.dev);
#if defined(WANPIPE_IFNET_QUEUE_POLICY_INIT_OFF)
			WAN_NETIF_STOP_QUEUE(lip_dev->common.dev);
#endif
			wplip_disconnect(lip_dev->common.lip,0);
		}
		
	}else{
		if (state == WAN_CONNECTED){
			WAN_NETIF_CARRIER_ON(lip_dev->common.dev);
			WAN_NETIF_WAKE_QUEUE(lip_dev->common.dev);
			wplip_trigger_bh(lip_dev->lip_link);
		}else{
			WAN_NETIF_CARRIER_OFF(lip_dev->common.dev);
#if defined(WANPIPE_IFNET_QUEUE_POLICY_INIT_OFF)
			WAN_NETIF_STOP_QUEUE(lip_dev->common.dev);
#endif
		}
		wplip_trigger_if_task(lip_dev);
#if defined(NETGRAPH)
		if (lip_dev->common.usedby == WP_NETGRAPH) {
			/* Inform netgraph of status of node */
			wan_ng_link_state(&lip_dev->common, lip_dev->common.state);
		}
#endif
	}
	
	return 0;
}




int wplip_lipdev_prot_change_state(void *wplip_id,int state, 
		                   unsigned char *data, int len)
{
	wplip_dev_t *lip_dev = (wplip_dev_t *)wplip_id;

	DEBUG_EVENT("%s: Lip Dev Prot State %s!\n",
		lip_dev->name, STATE_DECODE(state));

	lip_dev->common.state = state;

 	if (wan_test_bit(WPLIP_DEV_UNREGISTER,&lip_dev->critical) ||
            wan_test_bit(0,&lip_dev->if_down)) {
		return 0;
	}

	return wplip_lipdev_prot_update_state_change(lip_dev,data,len);
}


/*==============================================================
 * wplip_connect
 *
 * Description:
 * 	The lowyer layer calls this function, when it
 * 	becomes connected.  
 */

static void wplip_connect(void *wplip_id,int reason)
{
	wplip_link_t 		*lip_link = (wplip_link_t *)wplip_id;
#if defined(NETGRAPH)
	wanpipe_common_t	*common = NULL;
#endif

	if (lip_link->carrier_state != WAN_CONNECTED){
		wan_smp_flag_t flags;

		DEBUG_EVENT("%s: Lip Link Carrier Connected! \n",
			lip_link->name);

		wplip_spin_lock_irq(&lip_link->bh_lock,&flags);
		wan_skb_queue_purge(&lip_link->tx_queue);
		wan_skb_queue_purge(&lip_link->rx_queue);
		wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);

		wplip_kick(lip_link,0);
		
		lip_link->carrier_state = WAN_CONNECTED;
	}
	
	if (lip_link->prot_state == WAN_CONNECTED){

		DEBUG_EVENT("%s: Lip Link Connected! \n",
			lip_link->name);

		lip_link->state = WAN_CONNECTED;
	}
#if defined(NETGRAPH)
	/* Inform netgraph of status of node */
	common = &(WAN_LIST_FIRST(&lip_link->list_head_ifdev))->common;
	wan_ng_link_state(common, WAN_CONNECTED);
#endif
}

/*==============================================================
 * wplip_disconnect
 *
 * Description:
 * 	The lowyer layer calls this function, when it
 * 	becomes disconnected.  
 */

static void wplip_disconnect(void *wplip_id,int reason)
{
	wplip_link_t *lip_link = (wplip_link_t *)wplip_id;
#if defined(NETGRAPH)
	wanpipe_common_t	*common = NULL;
#endif

	if (lip_link->carrier_state != WAN_DISCONNECTED){
		DEBUG_EVENT("%s: Lip Link Carrier Disconnected!\n",
			lip_link->name);
		lip_link->carrier_state = WAN_DISCONNECTED;
	}

	/* state = carrier_state & prot_state 
	 * Therefore, the overall state is down */
	if (lip_link->state != WAN_DISCONNECTED){
		lip_link->state = WAN_DISCONNECTED;
		DEBUG_EVENT("%s: Lip Link Disconnected!\n",lip_link->name);
	}
#if defined(NETGRAPH)
	common = &(WAN_LIST_FIRST(&lip_link->list_head_ifdev))->common;
	wan_ng_link_state(common, WAN_DISCONNECTED);
#endif
}



/*==============================================================
 * wplip_kick
 *
 * Description:
 */

void wplip_kick(void *wplip_id,int reason)
{
	wplip_link_t *lip_link = (wplip_link_t *)wplip_id;
	
	DEBUG_TEST("%s: LIP Kick!\n",
			lip_link->name);

#ifdef WPLIP_TTY_SUPPORT
	if (lip_link->tty_opt){ 
		wanpipe_tty_trigger_poll(lip_link);
	}else
#endif
	{	
		wan_clear_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working);
		wan_set_bit(WPLIP_KICK,&lip_link->tq_working);
		wplip_kick_trigger_bh(lip_link);
	}
}

#define INTERFACES_FRM	 "%-15s| %-12s| %-6u| %-18s|\n"
static int wplip_get_if_status(void *wplip_id, void *mptr)
{
#if defined(__LINUX__)
	wplip_link_t *lip_link = (wplip_link_t *)wplip_id;
	struct seq_file *m = (struct seq_file *)mptr;
	wplip_dev_t *cur_dev;
	unsigned long flag;

	WP_READ_LOCK(&lip_link->dev_list_lock,flag);

	WAN_LIST_FOREACH(cur_dev,&lip_link->list_head_ifdev,list_entry){
		PROC_ADD_LINE(m, 
			      INTERFACES_FRM,
			      cur_dev->name, lip_link->name, 
			      cur_dev->prot_addr,
			      STATE_DECODE(cur_dev->common.state));
	}

	WP_READ_UNLOCK(&lip_link->dev_list_lock,flag);

	return m->count;
#else
	return -EINVAL;
#endif
}


int wplip_link_callback_tx_down(void *wplink_id, void *skb)
{
	wplip_link_t *lip_link   = (wplip_link_t *)wplink_id;	

	if (!lip_link || !skb){
		DEBUG_EVENT("%s: Assertion error lip_dev=NULL!\n",
				__FUNCTION__);
		return 1;
	}

	DEBUG_TEST("%s:%s: Protocol Packet Len=%i\n",
			lip_link->name,__FUNCTION__,wan_skb_len(skb));


	if (lip_link->carrier_state != WAN_CONNECTED){
		DEBUG_TEST("%s: %s() Error Lip Link Carrier not connected !\n",
				lip_link->name,__FUNCTION__);
		wan_skb_free(skb);
		return 0;
	}

	if (wan_skb_queue_len(&lip_link->tx_queue) >= MAX_TX_BUF){
		DEBUG_TEST("%s: %s() Error Protocol Tx queue full !\n",
				lip_link->name,__FUNCTION__);
		wplip_trigger_bh(lip_link);
		return 1;
	}

	wan_skb_unlink(skb);
	wan_skb_queue_tail(&lip_link->tx_queue,skb);
	wplip_trigger_bh(lip_link);

	return 0;
}
int wplip_callback_tx_down(void *wplip_id, void *skb)
{
	wplip_dev_t *lip_dev   = (wplip_dev_t *)wplip_id;	

	if (!lip_dev){
		DEBUG_EVENT("%s: Assertion error lip_dev=NULL!\n",
				__FUNCTION__);
		return 1;
	}

	if (!skb){
		int free_space=lip_dev->max_mtu_sz - wan_skb_queue_len(&lip_dev->tx_queue);
		if (free_space < 0) {
			return 0;
		} else {
			return free_space;
		}    
	}


	DEBUG_TEST("%s:%s: Packet Len=%i\n",
			lip_dev->name,__FUNCTION__,wan_skb_len(skb));


	if (lip_dev->lip_link->carrier_state != WAN_CONNECTED){
		DEBUG_TEST("%s: %s() Error Lip Link Carrier not connected !\n",
				lip_dev->name,__FUNCTION__);
		wan_skb_free(skb);
		WAN_NETIF_STATS_INC_TX_CARRIER_ERRORS(&lip_dev->common);	//lip_dev->ifstats.tx_carrier_errors++;
		return 0;
	}

	if (wan_skb_queue_len(&lip_dev->tx_queue) >= lip_dev->max_mtu_sz){
		wplip_trigger_bh(lip_dev->lip_link);
		DEBUG_TEST("%s: %s() Error  Tx queue full Kick=%d!\n",
				lip_dev->name,__FUNCTION__,
				wan_test_bit(WPLIP_BH_AWAITING_KICK,&lip_dev->lip_link->tq_working));
		return 1;
	}

	wan_skb_unlink(skb);
	wan_skb_queue_tail(&lip_dev->tx_queue,skb);
	wplip_trigger_bh(lip_dev->lip_link);

	return 0;
}

int wplip_callback_kick_prot_task (void *wplink_id)
{
	wplip_link_t *lip_link   = (wplip_link_t *)wplink_id;	

	if (!lip_link){
		DEBUG_EVENT("%s: Assertion error lip_dev=NULL!\n",
				__FUNCTION__);
		return 1;
	}

	WAN_TASKQ_SCHEDULE((&lip_link->prot_task));

	return 0;
}

unsigned int wplip_get_ipv4_addr (void *wplip_id, int type)
{
	wplip_dev_t *lip_dev   = (wplip_dev_t *)wplip_id;	
#ifdef __LINUX__

	struct in_ifaddr *ifaddr;
	struct in_device *in_dev;

	if ((in_dev = in_dev_get(lip_dev->common.dev)) == NULL){
		return 0;
	}

	if ((ifaddr = in_dev->ifa_list)== NULL ){
		in_dev_put(in_dev);
		return 0;
	}
	in_dev_put(in_dev);
	
	switch (type){

	case WAN_LOCAL_IP:
		return ifaddr->ifa_local;
		break;
	
	case WAN_POINTOPOINT_IP:
		return ifaddr->ifa_address;
		break;	

	case WAN_NETMASK_IP:
		return ifaddr->ifa_mask;
		break;

	case WAN_BROADCAST_IP:
		return ifaddr->ifa_broadcast;
		break;
	default:
		return 0;
	}
#elif defined(__WINDOWS__)
	/* not used */
#else
	netdevice_t		*ifp = NULL;
	struct ifaddr		*ifa = NULL;
	struct sockaddr_in	*si;

	ifp = lip_dev->common.dev;
	for (ifa = WAN_TAILQ_FIRST(ifp); ifa; ifa = WAN_TAILQ_NEXT(ifa)){
		if (ifa->ifa_addr->sa_family == AF_INET) {
			si = (struct sockaddr_in *)ifa->ifa_addr;
			if (si) break;
		}
	}

	if (ifa) {
		switch (type){
		case WAN_LOCAL_IP:
			if (ifa->ifa_addr){
				return (satosin(ifa->ifa_addr))->sin_addr.s_addr;
			}
			break;
		case WAN_POINTOPOINT_IP:
			if (ifa->ifa_dstaddr){
				return (satosin(ifa->ifa_dstaddr))->sin_addr.s_addr;
			}
			break;
		case WAN_NETMASK_IP:
			if (ifa->ifa_netmask){
				return (satosin(ifa->ifa_netmask))->sin_addr.s_addr;
			}
			break;
		case WAN_BROADCAST_IP:
			if (ifa->ifa_broadaddr){
				return (satosin(ifa->ifa_broadaddr))->sin_addr.s_addr;
			}
			break;
		}
	}
#endif
	return 0;
}


/* PROTOCOL MUST CALL THIS ONLY 
 * FROM A PROT TASK */

int wplip_set_ipv4_addr (void *wplip_id, 
		         unsigned int local,
		         unsigned int remote,
		         unsigned int netmask,
		         unsigned int dns)
{
	wplip_dev_t *lip_dev   = (wplip_dev_t *)wplip_id;	
#if defined(__LINUX__)
	struct sockaddr_in *if_data;
	struct ifreq if_info;	
	int err=0;
	mm_segment_t fs;
	netdevice_t *dev=lip_dev->common.dev;
	
	 /* Setup a structure for adding/removing routes */
        memset(&if_info, 0, sizeof(if_info));
        strcpy(if_info.ifr_name, dev->name);

	if_data = (struct sockaddr_in *)&if_info.ifr_addr;
	if_data->sin_addr.s_addr = local;
	if_data->sin_family = AF_INET;

	fs = get_fs();                  /* Save file system  */
       	set_fs(get_ds());    
	err = wp_devinet_ioctl(SIOCSIFADDR, &if_info);
	set_fs(fs);
        
	memset(&if_info, 0, sizeof(if_info));
        strcpy(if_info.ifr_name, dev->name);

	if_data = (struct sockaddr_in *)&if_info.ifr_dstaddr;
	if_data->sin_addr.s_addr = remote;
	if_data->sin_family = AF_INET;

	fs = get_fs();                  /* Save file system  */
       	set_fs(get_ds());  
	err = wp_devinet_ioctl(SIOCSIFDSTADDR, &if_info);
	set_fs(fs);

	return 0;
#elif defined(__WINDOWS__)
	/* not used */
	return 0;
#else
	netdevice_t		*ifp = NULL;
	struct ifaddr		*ifa = NULL;
	struct sockaddr_in	*si = NULL;

	ifp = lip_dev->common.dev;
	for (ifa = WAN_TAILQ_FIRST(ifp); ifa; ifa = WAN_TAILQ_NEXT(ifa)){
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET){
			si = (struct sockaddr_in *)ifa->ifa_addr;
			if (si) break;
		}
	}
	if (ifa && si){
		int error;
#if defined(__FreeBSD__)
		struct in_ifaddr *ia;
#endif

#if defined(__NetBSD__) && (__NetBSD_Version__ >= 103080000)
		struct sockaddr_in new_sin = *si;

		new_sin.sin_addr.s_addr = htonl(src);
		error = in_ifinit(ifp, ifatoia(ifa), &new_sin, 1);
#else
		/* delete old route */
		error = rtinit(ifa, (int)RTM_DELETE, RTF_HOST);

		/* set new address */
		si->sin_addr.s_addr = local;
#if defined(__FreeBSD__)
		ia = ifatoia(ifa);
		LIST_REMOVE(ia, ia_hash);
		LIST_INSERT_HEAD(INADDR_HASH(si->sin_addr.s_addr), ia, ia_hash);
#endif
#if 0
		/* Local IP address */
		si = (struct sockaddr_in*)ifa->ifa_addr;
		if (si){
			/* write new local IP address */
			si->sin_addr.s_addr = local;
		}
#endif
		/* Remote IP address */
		si = (struct sockaddr_in*)ifa->ifa_dstaddr;
		if (si){
			/* write new remote IP address */
			si->sin_addr.s_addr = remote;
		}
		/* Netmask */
		si = (struct sockaddr_in*)ifa->ifa_netmask;
		if (si){
			/* write new remote IP address */
			si->sin_addr.s_addr = htonl(0xFFFFFFFC);
		}

		/* add new route */
		error = rtinit(ifa, (int)RTM_ADD, RTF_HOST);
#endif
	}
	return 0;
#endif
}

void wplip_add_gateway(void *wplip_id)
{

#if 0
	wplip_dev_t *lip_dev   = (wplip_dev_t *)wplip_id;	
#endif
	return;
}


int wplip_set_hw_idle_frame (void *liplink_ptr, unsigned char *data, int len)
{

     	wplip_link_t *lip_link   = (wplip_link_t *)liplink_ptr;	

	if (!lip_link){
		DEBUG_EVENT("%s: Assertion error lip_dev=NULL!\n",
				__FUNCTION__);
		return 1;
	}            

        DEBUG_EVENT("%s: Warning: %s Not supported!\n",
		      lip_link->name,__FUNCTION__);  
	
	return 0;
}


/***************************************************************
 * Private Device Functions
 */
 
#ifdef __LINUX__
static int wplip_change_dev_flags (netdevice_t *dev, unsigned flags)
{
	struct ifreq if_info;
	mm_segment_t fs = get_fs();
	int err;

	memset(&if_info, 0, sizeof(if_info));
	strcpy(if_info.ifr_name, dev->name);
	if_info.ifr_flags = flags;	

	set_fs(get_ds());     /* get user space block */ 
	err = wp_devinet_ioctl(SIOCSIFFLAGS, &if_info);
	set_fs(fs);

	return err;
}        
#endif


#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static void wplip_if_task (void *arg, int dummy)
#elif defined(__WINDOWS__)
static void wplip_if_task (IN PKDPC Dpc, IN PVOID arg, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
#else
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 	
static void wplip_if_task (void *arg)
# else
static void wplip_if_task (struct work_struct *work)
# endif
#endif
{
#if defined(__LINUX__)
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
	wplip_dev_t	*lip_dev=(wplip_dev_t *)container_of(work, wplip_dev_t, if_task);
#else
	wplip_dev_t	*lip_dev=(wplip_dev_t *)arg;
#endif
	wplip_link_t	*lip_link;
	netdevice_t 	*dev;
	
	if (!lip_dev || !lip_dev->common.dev){
		return;
	}
	
	if (wan_test_bit(WPLIP_DEV_UNREGISTER,&lip_dev->critical)){
		return;
	}
	
	if (!wan_test_bit(DYN_OPT_ON,&lip_dev->interface_down) ||
	    !wan_test_bit(WAN_DEV_READY,&lip_dev->interface_down)){
		return;
	}
	
	DEBUG_TEST("%s():line:%d: LIP Device %s\n",__FUNCTION__,__LINE__,lip_dev->name);
	
	lip_link = lip_dev->lip_link;
	dev=lip_dev->common.dev;
	
	switch (lip_dev->common.state){

	case WAN_DISCONNECTED:

		/* If the dynamic interface configuration is on, and interface 
		 * is up, then bring down the netowrk interface */
		
		if (wan_test_bit(DYN_OPT_ON,&lip_dev->interface_down) && 
		    !wan_test_bit(DEV_DOWN,  &lip_dev->interface_down) &&		
		    dev->flags & IFF_UP){	

			DEBUG_EVENT("%s: Interface %s down.\n",
				lip_link->name,dev->name);
			wplip_change_dev_flags(dev,(dev->flags&~IFF_UP));
			wan_set_bit(DEV_DOWN,&lip_dev->interface_down);

		}

		break;

	case WAN_CONNECTED:

		/* In SMP machine this code can execute before the interface
		 * comes up.  In this case, we must make sure that we do not
		 * try to bring up the interface before dev_open() is finished */


		/* DEV_DOWN will be set only when we bring down the interface
		 * for the very first time. This way we know that it was us
		 * that brought the interface down */

		if (wan_test_bit(DYN_OPT_ON,&lip_dev->interface_down) &&
		    wan_test_bit(DEV_DOWN,  &lip_dev->interface_down) &&
		    !(dev->flags & IFF_UP)){
			
			DEBUG_EVENT("%s: Interface %s up.\n",
				lip_link->name,dev->name);
			wplip_change_dev_flags(dev,(dev->flags|IFF_UP));
			wan_clear_bit(DEV_DOWN,&lip_dev->interface_down);
		}
		
		break;
	}	

#endif
	
}

void wplip_trigger_if_task(wplip_dev_t *lip_dev)
{
	
	if (wan_test_bit(WPLIP_DEV_UNREGISTER,&lip_dev->critical)){
		return;
	}
	
	if (wan_test_bit(DYN_OPT_ON,&lip_dev->interface_down) && 
	    wan_test_bit(WAN_DEV_READY,&lip_dev->interface_down)){
		WAN_TASKQ_SCHEDULE((&lip_dev->if_task));
	}	

	return;
}

/***************************************************************
 * Module Interface Functions
 *
 */

/*==============================================================
 * wanpipe_lip_init
 *
 * Description:
 * 	This function is called on module startup.
 * 	(ex: modprobe wanpipe_lip)
 *
 * 	Register the lip callback functions to the
 * 	wanmain which are used by the socket or
 * 	upper layer.
 */


static unsigned char wplip_fullname[]="WANPIPE(tm) L.I.P Network Layer";
static unsigned char wplip_copyright[]="(c) 1995-2004 Sangoma Technologies Inc.";

int wanpipe_lip_init(void*);
int wanpipe_lip_exit(void*);
int wanpipe_lip_shutdown(void*);
int wanpipe_lip_ready_unload(void*);

int wanpipe_lip_init(void *arg)
{
	wplip_reg_t reg;	
	int err;

#if defined(WAN_DEBUG_MEM_LIP)
	wanpipe_lip_memdbg_init();
#endif

   	DEBUG_EVENT("%s %s.%s %s\n",
			wplip_fullname, WANPIPE_VERSION, WANPIPE_SUB_VERSION, wplip_copyright);

	err=wplip_init_prot();
	if (err){
		wplip_free_prot();
		return err;
	}

	wan_rwlock_init(&wplip_link_lock);

	memset(&reg,0,sizeof(wplip_reg_t));
	
	reg.wplip_bind_link 	= wplip_bind_link;
	reg.wplip_unbind_link	= wplip_unbind_link;
	
	reg.wplip_if_reg 	= wplip_if_reg;
	reg.wplip_if_unreg   	= wplip_if_unreg;

	reg.wplip_disconnect 	= wplip_disconnect;
	reg.wplip_connect 	= wplip_connect;

	reg.wplip_rx 	 	= wplip_rx;
	reg.wplip_kick 		= wplip_kick;
	
	reg.wplip_register 	= wplip_register;
	reg.wplip_unreg 	= wplip_unreg;

	reg.wplip_get_if_status = wplip_get_if_status;

	register_wanpipe_lip_protocol(&reg);
	 
	memset(&wplip_link_num,0,sizeof(wplip_link_num));
	
	return 0;
}

int wanpipe_lip_exit (void *arg)
{
	if (!WAN_LIST_EMPTY(&list_head_link)){
		DEBUG_EVENT("%s: Major error: List not empty!\n",__FUNCTION__);
	}

	unregister_wanpipe_lip_protocol();
	wplip_free_prot();	

#if defined(WAN_DEBUG_MEM_LIP)
	wanpipe_lip_memdbg_free();
#endif

	DEBUG_EVENT("WANPIPE L.I.P: Unloaded\n");
	return 0;
}

#if 0
int wanpipe_lip_shutdown(void *arg)
{
	DEBUG_EVENT("Shutting down WANPIPE LIP module ...\n");
	return 0;
}

int wanpipe_lip_ready_unload(void *arg)
{
	DEBUG_EVENT("Is WANPIPE LIP module ready to unload...\n");
	return 0;
}
#endif

WAN_MODULE_DEFINE(
		wanpipe_lip,"wanpipe_lip", 
		"Nenad Corbic <ncorbic@sangoma.com>",
		"Wanpipe L.I.P Network Layer - Sangoma Tech. Copyright 2004", 
		"GPL",
		wanpipe_lip_init, wanpipe_lip_exit,/* wanpipe_lip_shutdown, wanpipe_lip_ready_unload,*/
		NULL);
WAN_MODULE_DEPEND(wanpipe_lip, wanrouter, 1,
			WANROUTER_MAJOR_VER, WANROUTER_MAJOR_VER);
WAN_MODULE_DEPEND(wanpipe_lip, wanpipe, 1,
			WANPIPE_MAJOR_VER, WANPIPE_MAJOR_VER);

