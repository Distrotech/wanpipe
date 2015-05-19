#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <wanpipe_lip.h>
#elif defined(__WINDOWS__)
#include "wanpipe_lip.h"
#else
# include <linux/wanpipe_lip.h>
#endif

/* Function interface between LIP layer and kernel */
extern wan_iface_t wan_iface;

/*=============================================================
 * Funciton Prototypes
 */

static netdevice_t *wplip_create_netif(char *dev_name, int usedby);
static int wplip_free_netif(netdevice_t *dev);
static int wplip_register_netif(netdevice_t *dev, wplip_dev_t *lip_dev, int iftype);
static void wplip_unregister_netif(netdevice_t *dev, wplip_dev_t *lip_dev);
#if defined(__LINUX__)
extern void wplip_link_bh(unsigned long data);
#elif defined(__WINDOWS__)
extern void wplip_link_bh(IN PKDPC Dpc, IN PVOID data, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
#else
extern void wplip_link_bh(void *data, int pending);
#endif

/*==============================================================
 * wplip_create_link
 *
 * Description:
 * 	Create and initialize the wplip link 
 * 	private structure.  
 *
 * 	This structure is created for each
 * 	wplip link. Each wplip link can have multiple
 * 	logic channels.  Thus it contins the logic
 * 	channel device link lists as well as link configuration
 * 	profiles.
 *
 * Used by: 
 *	wplip_register 
 */
wplip_link_t *wplip_create_link(char *devname)
{
	wplip_link_t *lip_link;
	int i;
	
	lip_link=wan_kmalloc(sizeof(wplip_link_t));
	if (lip_link==NULL){
		WAN_MEM_ASSERT("LIP Link Alloc: ");
		return NULL;
	}

	memset(lip_link,0,sizeof(wplip_link_t));

	lip_link->magic=WPLIP_MAGIC_LINK;

	/* FIXME: No entry initializer !!! */
	/* WAN_LIST_HEAD_INITIALIZER(&lip_link->list_entry); */
	WAN_LIST_INIT(&lip_link->list_head_ifdev);
	WAN_LIST_INIT(&lip_link->list_head_tx_ifdev);

	wan_skb_queue_init(&lip_link->tx_queue);
	wan_skb_queue_init(&lip_link->rx_queue);

	wplip_spin_lock_irq_init(&lip_link->bh_lock, "wan_lip_bh_lock");

	wan_atomic_set(&lip_link->refcnt,0);

	lip_link->link_num=-1;
	for (i=0;i<MAX_LIP_LINKS;i++){
		if (!wan_test_and_set_bit(0,&wplip_link_num[i])){
			lip_link->link_num=i;
			break;
		}
	}
	
	if (lip_link->link_num == -1){
		wan_free(lip_link);
		DEBUG_EVENT("%s: No LIP links available! Max=%d\n",
				__FUNCTION__,MAX_LIP_LINKS);
		return NULL;
	}	

#if 0
	sprintf(lip_link->name,"link%04u",lip_link->link_num);
#else
	sprintf(lip_link->name,"%s",devname);	
#endif
	lip_link->state = WAN_DISCONNECTED;

	wan_rwlock_init(&lip_link->dev_list_lock);
	wan_rwlock_init(&lip_link->tx_dev_list_lock);
	wan_rwlock_init(&lip_link->map_lock);

	WAN_TASKLET_INIT(&lip_link->task,0,wplip_link_bh,lip_link);

#ifndef LINUX_2_6
	MOD_INC_USE_COUNT;
#endif
	WAN_HOLD(lip_link);

	return lip_link;
}

/*==============================================================
 * wplip_link_ok
 *
 * 	Make sure that the link in question exists
 * 	in the main x25 link list.  
 * 	Used as a sanity check.
 */

int wplip_link_exists(wplip_link_t *lip_link)
{
	wplip_link_t *cur_link;
	wan_rwlock_flag_t flag;

	if (!lip_link){
		return -ENODEV;
	}

	WPLIP_ASSERT_MAGIC(lip_link,WPLIP_MAGIC_LINK,-EINVAL);
	
	WP_READ_LOCK(&wplip_link_lock,flag);

	WAN_LIST_FOREACH(cur_link,&list_head_link,list_entry) {
		if (cur_link == lip_link){
			WP_READ_UNLOCK(&wplip_link_lock,flag);
			return 0;
		}
	}

	WP_READ_UNLOCK(&wplip_link_lock,flag);
	return -ENODEV;
}



void wplip_free_link(wplip_link_t *lip_link)
{
	unsigned long timeout=SYSTEM_TICKS;

	WPLIP_ASSERT_VOID(wplip_liplink_magic(lip_link));

	if (lip_link->link_num != -1){
		wan_clear_bit(0,&wplip_link_num[lip_link->link_num]);
		lip_link->link_num=-1;
	}

	__WAN_PUT(lip_link);

	wan_skb_queue_purge(&lip_link->tx_queue);
	wan_skb_queue_purge(&lip_link->rx_queue);

lip_free_link_wait:
	if (wan_atomic_read(&lip_link->refcnt)){
		DEBUG_EVENT("%s: Error lip link use count %d, waiting 2 sec...\n",
				lip_link->name, wan_atomic_read(&lip_link->refcnt));
		
		if ((SYSTEM_TICKS-timeout) < (HZ*2)){
#if defined(__LINUX__)
			schedule();
#endif
			goto lip_free_link_wait;
		}

		DEBUG_EVENT("%s: Warning continuing with deallocation: refcnt=%d\n",
				lip_link->name, wan_atomic_read(&lip_link->refcnt));
		wan_atomic_set(&lip_link->refcnt,1);
	}

	wan_free(lip_link);
#ifndef LINUX_2_6
	MOD_DEC_USE_COUNT;
#endif
}

void wplip_insert_link(wplip_link_t *lip_link)
{
	wan_rwlock_flag_t flag;

	WP_WRITE_LOCK(&wplip_link_lock,flag);
	WAN_LIST_INSERT_HEAD(&list_head_link,lip_link,list_entry);
	WAN_HOLD(lip_link);
	WP_WRITE_UNLOCK(&wplip_link_lock,flag);
}


void wplip_remove_link(wplip_link_t *lip_link)
{
	wan_rwlock_flag_t flag;
	
	WP_READ_LOCK(&wplip_link_lock,flag);
	__WAN_PUT(lip_link);	
	WAN_LIST_REMOVE(lip_link,list_entry);
	WP_READ_UNLOCK(&wplip_link_lock,flag);
}

/*==============================================================
 * wplip_lipdev_latency_change
 *
 * Description:
 *
 * Purpose:
 * 	Indicate a latency queue len change
 *      to the link.
 * 
 * Used by: 
 */

int wplip_lipdev_latency_change(wplip_link_t *lip_link)
{
	wplip_dev_t *cur_dev;
	wan_rwlock_flag_t flag;
	unsigned int latency_qlen=0xFFFF;
	
	WP_READ_LOCK(&lip_link->dev_list_lock,flag);

	/* Get the smallest queue latency out of all 
	 * protocol interfaces. The smallest value will be
	 * used as the new latency for the master device */

	WAN_LIST_FOREACH(cur_dev,&lip_link->list_head_ifdev,list_entry){
		if (cur_dev->max_mtu_sz < latency_qlen) {
			latency_qlen=cur_dev->max_mtu_sz;	
		}
	}

	WP_READ_UNLOCK(&lip_link->dev_list_lock,flag);

	if (latency_qlen > 0 && latency_qlen < 0xFFFF) {
		wan_smp_flag_t flags;
		wplip_spin_lock_irq(&lip_link->bh_lock,&flags);
        	lip_link->latency_qlen = latency_qlen;
		wplip_spin_unlock_irq(&lip_link->bh_lock,&flags);
	}
	
	return -ENODEV;
}



/**************************************************************
 * DEV SPECIFIC SUB CALLS
 **************************************************************/


/*==============================================================
 * wplip_create_lipdev
 *
 * Description:
 * 	Create and initialize the svc/lcn 
 * 	private structure.  
 *
 * 	This structure is created for each
 * 	svc on the x25 link.  It is bound into
 * 	dev->priv pointer of the svc network 
 * 	interface device.
 *
 * Used by: 
 *	x25_register 
 */

#if defined(__WINDOWS__)
/*	Under Windows the order of structure creation is opposite to
	Linux order. The netdev already exist when svc is created.
	So the binding between the two structures is done a little
	differently. */
wplip_dev_t *wplip_create_lipdev(netdevice_t *dev, int usedby)
#else
wplip_dev_t *wplip_create_lipdev(char *dev_name, int usedby)
#endif
{
	wplip_dev_t	*lip_dev;
#if defined(__WINDOWS__)
	char *dev_name = dev->name;
#endif

	lip_dev=wan_kmalloc(sizeof(wplip_dev_t));
	if (lip_dev == NULL){
		return NULL;
	}

	memset(lip_dev, 0x00, sizeof(wplip_dev_t));
	
	lip_dev->magic=WPLIP_MAGIC_DEV;
	lip_dev->common.state = WAN_DISCONNECTED;
	lip_dev->common.usedby = usedby;

#if defined(__WINDOWS__)
	snprintf(lip_dev->name, sizeof(lip_dev->name), "%s", dev_name);
#else
	strncpy(lip_dev->name,dev_name,MAX_PROC_NAME); 
#endif

	/* FIXME: No Entry Intializer */	
	/*WPLIP_INIT_LIST_HEAD(&lip_dev->list_entry);*/

	wan_skb_queue_init(&lip_dev->tx_queue);

	wan_atomic_set(&lip_dev->refcnt,0);

#if defined(__WINDOWS__)
	lip_dev->common.dev = dev;
#else
	lip_dev->common.dev=wplip_create_netif(dev_name, usedby);
	if (!lip_dev->common.dev){
		wan_free(lip_dev);
		return NULL;
	}
#endif

	if (wplip_register_netif(lip_dev->common.dev, lip_dev, usedby)){
#ifndef __WINDOWS__
		wplip_free_netif(lip_dev->common.dev);	
#endif
		lip_dev->common.dev=NULL;
		wan_free(lip_dev);
		return NULL;
	}

	
	WAN_HOLD(lip_dev);
	
	return lip_dev;
}




/*
 * Free an allocated lapb control block. This is done to centralise
 * the MOD count code.
 */
void wplip_free_lipdev(wplip_dev_t *lip_dev)
{
	if (lip_dev->common.dev){
		/* FIXME: PRIV attached during unregister is it OK?*/ 
		WAN_DEV_PUT(lip_dev->common.dev);
		/*wplip_netdev_set_lipdev(lip_dev->common.dev, NULL);*/
		wplip_unregister_netif(lip_dev->common.dev, lip_dev);	
		lip_dev->common.dev=NULL;
	}

	
	__WAN_PUT(lip_dev);
		
	if (wan_atomic_read(&lip_dev->refcnt)){
		DEBUG_EVENT("%s: Major Error lip dev is still in use=%d!\n",
				  lip_dev->name,wan_atomic_read(&lip_dev->refcnt));
	}

	wan_free(lip_dev);
}




/*==============================================================
 * wplip_insert_dev_to_link
 * 
 * 	Insert the x25 svc network interface dev into 
 * 	the x25 link device list.
 */

void wplip_insert_lipdev(wplip_link_t *lip_link, wplip_dev_t *lip_dev)
{

	unsigned long flags;

	WP_WRITE_LOCK(&lip_link->dev_list_lock,flags);
	WAN_LIST_INSERT_HEAD(&lip_link->list_head_ifdev,lip_dev,list_entry);
	WAN_HOLD(lip_dev);
	lip_link->dev_cnt++;

	WP_WRITE_UNLOCK(&lip_link->dev_list_lock,flags);
}



/*==============================================================
 * wplip_lipdev_exists
 *
 * Description:
 *
 * Purpose:
 *	Prevents duplicate registration.
 * 
 * Used by: 
 */

int wplip_lipdev_exists(wplip_link_t *lip_link, char *dev_name)
{
	wplip_dev_t *cur_dev;
	wan_rwlock_flag_t flag;
	
	WP_READ_LOCK(&lip_link->dev_list_lock,flag);

	WAN_LIST_FOREACH(cur_dev,&lip_link->list_head_ifdev,list_entry){
		if (strcmp(wan_netif_name(cur_dev->common.dev),dev_name) == LIP_OK){
			WP_READ_UNLOCK(&lip_link->dev_list_lock,flag);
			return 0;
		}
	}
	WP_READ_UNLOCK(&lip_link->dev_list_lock,flag);
	return -ENODEV;
}


/*==============================================================
 * wplip_remove_lipdev
 * 
 * 	Remove the lip network device from the lip link
 * 	device list.
 * 
 */
void wplip_remove_lipdev(wplip_link_t *lip_link, wplip_dev_t *lip_dev)
{
	unsigned long flags;
	
	WP_WRITE_LOCK(&lip_link->dev_list_lock,flags);
	__WAN_PUT(lip_dev);
	WAN_LIST_REMOVE(lip_dev,list_entry);
	lip_link->dev_cnt--;
	WP_WRITE_UNLOCK(&lip_link->dev_list_lock,flags);

	return;
}



#ifndef __WINDOWS__
unsigned int dec_to_uint (unsigned char* str, int len)
{
	unsigned val;

	if (!len) 
		len = strlen(str);

	for (val = 0; len && is_digit(*str); ++str, --len)
		val = (val * 10) + (*str - (unsigned)'0');
	
	return val;
}
#endif


/******************************************************
 * PRIVATE FUNCTIONS
 ******************************************************/



/*==============================================================
 * wplip_create_netif
 *
 * Description:
 * 	Allocate and initialize the svc network interface
 * 	device.  This structure will be used to bind the
 * 	svc to the TCP/IP stack, or to the API socket.
 *	
 *	The dev->priv pointer will contain the svc/lcn
 *	private structure.
 *
 * Used by: 
 *	x25_register 
 */


static netdevice_t *wplip_create_netif(char *dev_name, int usedby)
{
	netdevice_t	*dev;
	int		iftype = WAN_IFT_OTHER;
	int		err;

#if defined(__OpenBSD__)
	if (usedby == TRUNK) iftype = WAN_IFT_ETHER;
#endif	
	
	dev = wan_netif_alloc(dev_name, iftype, &err);
	if (dev == NULL){
		return NULL;
	}

#if defined(__LINUX__) && !defined(LINUX_2639)
	wan_atomic_set(&dev->refcnt,0);
#endif
	WAN_DEV_HOLD(dev);

	return dev;
}

static int wplip_free_netif(netdevice_t *dev)
{
	WAN_DEV_PUT(dev);
#if defined(__LINUX__) && !defined(LINUX_2639)
	if (wan_atomic_read(&dev->refcnt)){
		DEBUG_EVENT("%s: Dev=%s Major error dev is still in use %d\n",
				__FUNCTION__,
				wan_netif_name(dev),
				wan_atomic_read(&dev->refcnt));
	}
#endif

	wan_netif_free(dev);

	return 0;
}


static int wplip_register_netif(netdevice_t *dev, wplip_dev_t *lip_dev, int usedby)
{
	int err = -EINVAL;

	wplip_netdev_set_lipdev(dev, lip_dev);
	wplip_if_init(dev);

	/* From this point forward, wplip_free_if() will deallocate
	 * both dev and lip_dev */
	lip_dev->common.is_netdev = 1;

	if (usedby == BRIDGE ||
            usedby == BRIDGE_NODE ||
            usedby == TRUNK){
		if (wan_iface.attach_eth){
			err = wan_iface.attach_eth(dev, NULL, lip_dev->common.is_netdev);
		}
	}else{
		if (wan_iface.attach){
			err = wan_iface.attach(dev, NULL, lip_dev->common.is_netdev);
		}
	}

	if (err){
		wplip_netdev_set_lipdev(dev, NULL);
	}
	return err;
}


static void wplip_unregister_netif(netdevice_t *dev, wplip_dev_t *lip_dev)
{
	if (wan_iface.detach){
		wan_iface.detach(dev, lip_dev->common.is_netdev);
	}
	
	if (wan_iface.free){
#ifndef __WINDOWS__
		wan_iface.free(dev);
#endif
	}

	return;
}

