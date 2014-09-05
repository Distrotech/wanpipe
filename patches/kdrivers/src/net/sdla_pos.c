/*****************************************************************************
* sdla_temp.c	WANPIPE(tm) Multiprotocol WAN Link Driver.
* 		
* 		Template Driver
* 		
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 07, 2002	Nenad Corbic	Initial version.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"

#include "wanproc.h"
#include "sdla_pos.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */

#if defined(__LINUX__)
#include "if_wanpipe.h"
#endif

/****** Defines & Macros ****************************************************/

/* Private critical flags */
enum { 
	POLL_CRIT = PRIV_CRIT, 
	TX_INTR,
	TASK_POLL
};

#define MAX_IP_ERRORS	10

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_RX_BUF	50
#define POLL_PERIOD	(HZ/100)

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with Protocol specific data
 */

typedef struct private_area
{
	wanpipe_common_t common;
	sdla_t		*card;
	int 		TracingEnabled;		/* For enabling Tracing */
	unsigned long 	curr_trace_addr;	/* Used for Tracing */
	unsigned long 	start_trace_addr;
	unsigned long 	end_trace_addr;
	unsigned long 	base_addr_trace_buffer;
	unsigned long 	end_addr_trace_buffer;
	unsigned short 	number_trace_elements;
	unsigned  	available_buffer_space;
	unsigned long 	router_start_time;
	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;
	u8		config_frmw;
	u8 		config_frmw_timeout;
	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned char 	udp_pkt_src;		/* udp packet processing */
	unsigned short 	timer_int_enabled;

	bh_data_t 	*bh_head;	  	  /* Circular buffer for bh */
	unsigned long  	tq_working;
	volatile int  	bh_write;
	volatile int  	bh_read;
	atomic_t  	bh_buff_used;
	
	unsigned char 	interface_down;

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	struct timer_list 	poll_timer;

	u8 			gateway;
	u8 			true_if_encoding;
	
	//FIXME: add driver stats as per frame relay!

	/* Entry in proc fs per each interface */
	struct proc_dir_entry	*dent;

	unsigned char 		udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t 		udp_pkt_len;

	char 			if_name[WAN_IFNAME_SZ+1];
	struct net_device_stats	if_stats;

}private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/* variable for keeping track of enabling/disabling FT1 monitor status */
extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

/**SECTOIN**************************************************
 *
 * Function Prototypes 
 *
 ***********************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int 	update (wan_device_t* wandev);
static int 	new_if (wan_device_t* wandev, struct net_device* dev, wanif_conf_t* conf);
static int 	del_if(wan_device_t *wandev, struct net_device *dev);

/* Network device interface */
static int 	if_init   (struct net_device* dev);
static int 	if_open   (struct net_device* dev);
static int 	if_close  (struct net_device* dev);
static int 	if_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);


static struct net_device_stats* if_stats (struct net_device* dev);
  
static int 	if_send (struct sk_buff* skb, struct net_device* dev);

/* Firmware interface functions */
static int 	frmw_error (sdla_t *card, int err, wan_mbox_t *mb);
static int 	disable_comm_shutdown (sdla_t *card);

/* Miscellaneous Functions */
static void 	disable_comm (sdla_t *card);


/**SECTION********************************************************* 
 * 
 * Public Functions 
 *
 ******************************************************************/


/*============================================================================
 * wppos_init - Cisco HDLC protocol initialization routine.
 *
 * @card:	Wanpipe card pointer
 * @conf:	User hardware/firmware/general protocol configuration
 *              pointer. 
 * 
 * This routine is called by the main WANPIPE module 
 * during setup: ROUTER_SETUP ioctl().  
 *
 * At this point adapter is completely initialized 
 * and firmware is running.
 *  o read firmware version (to make sure it's alive)
 *  o configure adapter
 *  o initialize protocol-specific fields of the adapter data space.
 *
 * Return:	0	o.k.
 *		< 0	failure.
 */

int wp_pos_init (sdla_t* card, wandev_conf_t* conf)
{
	int err;
	volatile wan_mbox_t* mb;

	/* Verify configuration ID */

	if (conf->config_id != WANCONFIG_POS) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Initialize protocol-specific fields */
	/* Initialize the card mailbox and obtain the mailbox pointer */
	/* Set a pointer to the actual mailbox in the allocated virtual
	 * memory area */
	/* Alex Apr 8 2004 Sangom ISA card */
	card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;

	mb = &card->wan_mbox;
	/* Obtain hardware configuration parameters */
	card->wandev.clocking 			= conf->clocking;
	card->wandev.ignore_front_end_status 	= conf->ignore_front_end_status;
	card->wandev.ttl 			= conf->ttl;
	card->wandev.electrical_interface 			= conf->electrical_interface; 
	card->wandev.comm_port 			= conf->comm_port;
	card->wandev.udp_port   		= conf->udp_port;
	card->wandev.new_if_cnt 		= 0;
	wan_atomic_set(&card->wandev.if_cnt,0);
	
	err = (card->hw_iface.check_mismatch) ? 
			card->hw_iface.check_mismatch(card->hw,conf->fe_cfg.media) : -EINVAL;
	if (err){
		return err;
	}
	
	card->fe.fe_status = FE_CONNECTED;
	card->wandev.mtu = conf->mtu;

	if (card->wandev.ignore_front_end_status == WANOPT_NO){
		DEBUG_EVENT( 
		  "%s: Enabling front end link monitor\n",
				card->devname);
	}else{
		DEBUG_EVENT( 
		  "%s: Disabling front end link monitor\n",
				card->devname);
	}


	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	DEBUG_EVENT( "%s: Running POS firmware\n",card->devname); 

	card->isr			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->disable_comm		= &disable_comm;

	/* Set protocol link state to disconnected,
	 * After seting the state to DISCONNECTED this
	 * function must return 0 i.e. success */
	card->u.pos.state = WAN_DISCONNECTED;
	card->wandev.state = WAN_DISCONNECTED;

	DEBUG_EVENT( "%s: Pos Firmware Ready!\n",card->devname);
	return 0;
}



/**SECTION************************************************************** 
 *
 * 	WANPIPE Device Driver Entry Points 
 *
 * *********************************************************************/


/*============================================================================
 * update - Update wanpipe device status & statistics
 * 
 * @wandev:	Wanpipe device pointer
 *
 * This procedure is called when updating the PROC file system.
 * It returns various communications statistics.
 * 
 * cat /proc/net/wanrouter/wanpipe#  (where #=1,2,3...)
 * 
 * These statistics are accumulated from 3 
 * different locations:
 * 	1) The 'if_stats' recorded for the device.
 * 	2) Communication error statistics on the adapter.
 *      3) Operational statistics on the adapter.
 * 
 * The board level statistics are read during a timer interrupt. 
 * Note that we read the error and operational statistics 
 * during consecitive timer ticks so as to minimize the time 
 * that we are inside the interrupt handler.
 *
 */
static int update (wan_device_t* wandev)
{
	sdla_t* card = wandev->priv;
 	struct net_device* dev;
        volatile private_area_t* priv_area;

	DEBUG_EVENT("%s: %d\n",__FUNCTION__, __LINE__);
		
	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical))
                return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if(dev == NULL)
		return -ENODEV;

	if((priv_area=dev->priv) == NULL)
		return -ENODEV;


       	if(card->update_comms_stats){
		return -EAGAIN;
	}

	/* FIXME: DO THE STATS HERE */

	return -ENODEV;
}


/*============================================================================
 * new_if - Create new logical channel.
 * 
 * &wandev: 	Wanpipe device pointer
 * &dev:	Network device pointer
 * &conf:	User configuration options pointer
 *
 * This routine is called by the ROUTER_IFNEW ioctl,
 * in wanmain.c.  The ioctl passes us the user configuration
 * options which we use to configure the driver and
 * firmware.
 *
 * This functions main purpose is to allocate the
 * private structure for protocol and bind it 
 * to dev->priv pointer.  
 *
 * Also the dev->init pointer should also be initialized
 * to the if_init() function.
 *
 * Any allocation necessary for the private strucutre
 * should be done here, as well as proc/ file initializetion
 * for the network interface.
 * 
 * o parse media- and hardware-specific configuration
 * o make sure that a new channel can be created
 * o allocate resources, if necessary
 * o prepare network device structure for registaration.
 * o add network interface to the /proc/net/wanrouter
 * 
 * The opposite of this function is del_if()
 *
 * Return:	0	o.k.
 *		< 0	failure (channel will not be created)
 */
static int new_if (wan_device_t* wandev, struct net_device* dev, wanif_conf_t* conf)
{
	sdla_t* card = wandev->priv;
	private_area_t* priv_area;

	DEBUG_EVENT("%s: %d\n",__FUNCTION__, __LINE__);

	DEBUG_EVENT( "%s: Configuring Interface: %s\n",
			card->devname, conf->name);
 
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		DEBUG_EVENT( "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	/* This protocol only supports a single network interface
	 * If developing a multi-interface protocol, one should
	 * alter this check to allow multiple interfaces */
	if (wan_atomic_read(&card->wandev.if_cnt) > 0){
		return -EEXIST;
	}
		
	/* allocate and initialize private data */
	priv_area = wan_malloc(sizeof(private_area_t));
	if(priv_area == NULL){ 
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}
	memset(priv_area, 0, sizeof(private_area_t));
	
	strncpy(priv_area->if_name, conf->name, WAN_IFNAME_SZ);
	
	priv_area->card = card; 

        /* Setup interface as:
	 *    WANPIPE 	  = IP over Protocol (Firmware)
	 *    API     	  = Raw Socket access to Protocol (Firmware)
	 *    BRIDGE  	  = Ethernet over Protocol, no ip info
	 *    BRIDGE_NODE = Ethernet over Protocol, with ip info
	 */
	priv_area->common.usedby = API;
	DEBUG_EVENT( "%s:%s: Running in API mode !\n",
			wandev->name,priv_area->if_name);

	
	/* The network interface "dev" has been passed as
	 * an argument from the above layer. We must initialize
	 * it so it can be registered into the kernel.
	 *
	 * The "dev" structure is the link between the kernel
	 * stack and the wanpipe driver.  It contains all 
	 * access hooks that kernel uses to communicate to 
	 * the our driver.
	 *
	 * For now, just set the "dev" name to the user
	 * defined name and initialize:
	 * 	dev->if_init : function that will be called
	 * 	               to further initialize
	 * 	               dev structure on "ifconfig up"
	 *
	 * 	dev->priv    : private structure allocated above
	 * 	
	 */
	
	/* Only setup the dev pointer once the new_if function has
	 * finished successfully.  DO NOT place any code below that
	 * can return an error */
	dev->init = &if_init;
	dev->priv = priv_area;

	/* Increment the number of network interfaces 
	 * configured on this card.  
	 */
	wan_atomic_inc(&card->wandev.if_cnt);

	DEBUG_EVENT( "\n");

	return 0;
}

/*============================================================================
 * del_if - Delete logical channel.
 *
 * @wandev: 	Wanpipe private device pointer
 * @dev:	Netowrk interface pointer
 *
 * This function is called by ROUTER_DELIF ioctl call
 * to deallocate the network interface.
 * 
 * The network interface and the private structure are
 * about to be deallocated by the upper layer. 
 * We have to clean and deallocate any allocated memory.
 *
 * NOTE: DO NOT deallocate dev->priv here! It will be
 *       done by the upper layer.
 * 
 */
static int del_if (wan_device_t* wandev, struct net_device* dev)
{
	private_area_t* 	priv_area = dev->priv;
	sdla_t*			card = priv_area->card;

	/* Decrement the number of network interfaces 
	 * configured on this card.  
	 */
	wan_atomic_dec(&card->wandev.if_cnt);

	DEBUG_SUB_MEM(sizeof(private_area_t));
	return 0;
}


/**SECTION***********************************************************
 *
 * 	KERNEL Device Entry Interfaces 
 *
 ********************************************************************/


 
/*============================================================================
 * if_init - Initialize Linux network interface.
 *
 * @dev:	Network interface pointer
 * 
 * During "ifconfig up" the upper layer calls this function
 * to initialize dev access pointers.  Such as transmit,
 * stats and header.
 * 
 * It is called only once for each interface, 
 * during Linux network interface registration.  
 *
 * Returning anything but zero will fail interface
 * registration.
 */
static int if_init (struct net_device* dev)
{
	private_area_t* priv_area = dev->priv;
	sdla_t* card = priv_area->card;
	wan_device_t* wandev = &card->wandev;

	/* Initialize device driver entry points */
	dev->open		= &if_open;
	dev->stop		= &if_close;
	dev->hard_header	= NULL; 
	dev->rebuild_header	= NULL;
	dev->hard_start_xmit	= &if_send;
	dev->get_stats		= &if_stats;
	dev->do_ioctl		= if_do_ioctl;

	/* Initialize media-specific parameters */
	dev->flags		|= IFF_POINTOPOINT;
	dev->flags		|= IFF_NOARP;
	dev->type		= ARPHRD_PPP;
	
	dev->mtu		= card->wandev.mtu;
 
	dev->hard_header_len	= 0;
	
	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start);
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end);

	/* Set transmit buffer queue length 
	 * If too low packets will not be retransmitted 
         * by stack.
	 */
        dev->tx_queue_len = 100;
   
	return 0;
}

/*============================================================================
 * if_open - Open network interface.
 *
 * @dev: Network device pointer
 *
 * On ifconfig up, this function gets called in order
 * to initialize and configure the private area.  
 * Driver should be configured to send and receive data.
 *
 * This functions starts a timer that will call
 * frmw_config() function. This function must be called
 * because the IP addresses could have been changed
 * for this interface.
 *
 * Return 0 if O.k. or errno.
 */
static int if_open (struct net_device* dev)
{
	private_area_t* priv_area = dev->priv;
	sdla_t* card = priv_area->card;
	struct timeval tv;
	int err = 0;

	/* Only one open per interface is allowed */
	if (open_dev_check(dev))
		return -EBUSY;

	/* Initialize the router start time.
	 * Used by wanpipemon debugger to indicate
	 * how long has the interface been up */
	do_gettimeofday(&tv);
	priv_area->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

	/* Increment the module usage count */
	wanpipe_open(card);

	return err;
}

/*============================================================================
 * if_close - Close network interface.
 * 
 * @dev: Network device pointer
 * 
 * On ifconfig down, this function gets called in order
 * to cleanup interace private area.  
 *
 * IMPORTANT: 
 *
 * No deallocation or unconfiguration should ever occur in this 
 * function, because the interface can come back up 
 * (via ifconfig up).  
 * 
 * Furthermore, in dynamic interfacace configuration mode, the 
 * interface will come up and down to reflect the protocol state.
 *
 * Any deallocation and cleanup can occur in del_if() 
 * function.  That function is called before the dev interface
 * itself is deallocated.
 *
 * Thus, we should only stop the net queue and decrement
 * the wanpipe usage counter via wanpipe_close() function.
 */

static int if_close (struct net_device* dev)
{
	private_area_t* priv_area = dev->priv;
	sdla_t* card = priv_area->card;

	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);
	return 0;
}


/*=============================================================
 * disable_comm - Main shutdown function
 *
 * @card: Wanpipe device pointer
 *
 * The command 'wanrouter stop' has been called
 * and the whole wanpipe device is going down.
 * This is the last function called to disable 
 * all comunications and deallocate any memory
 * that is still allocated.
 *
 * o Disable communications, turn off interrupts
 * o Deallocate memory used, if any
 * o Unconfigure TE1 card
 */ 

static void disable_comm (sdla_t *card)
{
	unsigned long smp_flags;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (card->comm_enabled){
		disable_comm_shutdown (card);
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	return;
}


/*============================================================================
 * if_send - Send a packet on a network interface.
 * 
 * @dev:	Network interface pointer
 * @skb:	Packet obtained from the stack or API
 *              that should be sent out the port.
 *              
 * o Mark interface as stopped
 * 	(marks start of the transmission) to indicate 
 * 	to the stack that the interface is busy. 
 *
 * o Check link state. 
 * 	If link is not up, then drop the packet.
 * 
 * o Copy the tx packet into the protocol tx buffers on
 *   the adapter.
 *   
 * o If tx successful:
 * 	Free the skb buffer and mark interface as running
 * 	and return 0.
 *
 * o If tx failed, busy:
 * 	Keep interface marked as busy 
 * 	Do not free skb buffer
 * 	Enable Tx interrupt (which will tell the stack
 * 	                     that interace is not busy)
 * 	Return a non-zero value to tell the stack
 * 	that the tx should be retried.
 * 	
 * Return:	0	complete (socket buffer must be freed)
 *		non-0	packet may be re-transmitted 
 *
 */
static int if_send (struct sk_buff* skb, struct net_device* dev)
{
	private_area_t *chan = dev->priv;
	sdla_t *card = chan->card;

	if (skb){
		wan_skb_free(skb);
	}
	DEBUG_EVENT("%s: if_send() dropping packet!\n",card->devname);

	start_net_queue(dev);
	
	return 0;
}


/*============================================================================
 * if_stats
 *
 * Used by /proc/net/dev and ifconfig to obtain interface
 * statistics.
 *
 * Return a pointer to struct net_device_stats.
 */
static struct net_device_stats* if_stats (struct net_device* dev)
{
	private_area_t* priv_area;

	if ((priv_area=dev->priv) == NULL)
		return NULL;

	return &priv_area->if_stats;
}




/*========================================================================
 * 
 * if_do_ioctl - Ioctl handler for fr
 *
 * 	@dev: Device subject to ioctl
 * 	@ifr: Interface request block from the user
 *	@cmd: Command that is being issued
 *	
 *	This function handles the ioctls that may be issued by the user
 *	to control or debug the protocol or hardware . 
 *
 *	It does both busy and security checks. 
 *	This function is intended to be wrapped by callers who wish to 
 *	add additional ioctl calls of their own.
 *
 * Used by:  SNMP Mibs
 * 	     wanpipemon debugger
 *
 */
static int if_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	private_area_t* chan= (private_area_t*)dev->priv;
	//unsigned long smp_flags;
	wan_mbox_t *mb,*usr_mb;
	sdla_t *card;
	int err=0;
	
	if (!chan){
		return -ENODEV;
	}
	card=chan->card;
	mb = &card->wan_mbox;
	
	NET_ADMIN_CHECK();

	if (test_and_set_bit(0,&card->wandev.critical)){
		return -EBUSY;
	}
	
	switch(cmd)
	{
		case SIOC_WANPIPE_POS_STATUS_CMD:

			usr_mb=(wan_mbox_t*)ifr->ifr_data;
			if (!usr_mb){
				DEBUG_EVENT ( 
				"%s: Ioctl command %x, has no Mbox attached\n",
					card->devname, cmd);
				err=-EINVAL;
				break;
			}
			
			if (copy_to_user((unsigned char*)usr_mb+1, (unsigned char*)mb+1,2)){
				err= -EFAULT;
				break;
			}

			break;	
		
		case SIOC_WANPIPE_EXEC_CMD:

			usr_mb=(wan_mbox_t*)ifr->ifr_data;
			if (!usr_mb){
				DEBUG_EVENT ( 
				"%s: Ioctl command %x, has no Mbox attached\n",
					card->devname, cmd);
				err=-EINVAL;
				break;
			}

			if (copy_from_user((unsigned char*)mb, (unsigned char *)usr_mb, 0x16)){
				printk(KERN_INFO "%s: SDLC Cmd: Failed to copy mb \n",card->devname);
				err = -EFAULT;
				break;
			}
		
			if (mb->wan_pos_data_len > 0){

				if (mb->wan_pos_data_len > 1030){
					DEBUG_ERROR("MAJORE ERROR !! DATA LEN > 1030\n");
					err=-EFAULT;
					break;
				}
				

				if (copy_from_user((unsigned char*)&mb->wan_pos_data[0], 
					           (unsigned char*)&usr_mb->wan_pos_data[0],
						   mb->wan_pos_data_len)){
					printk(KERN_INFO "%s: SDLC Cmd: Failed to copy mb data: len=%i\n",
							card->devname, mb->wan_pos_data_len);
					err = -EFAULT;
					break;
				}
			}

			//spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			//spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

			if (err != COMMAND_OK) {
				err = frmw_error(card,err,mb);
				break;
			}
		
			if (!err && mb->wan_pos_command == CONFIGURE){
				CONFIGURATION_STRUCT *cfg=(CONFIGURATION_STRUCT *)mb->wan_pos_data;
				dev->mtu = cfg->sdlc_maxdata; 
			}

			/* copy the result back to our buffer */
			if (copy_to_user((unsigned char*)usr_mb, (unsigned char*)mb,0x16)){
				err= -EFAULT;
				break;
			}
		
			if (mb->wan_pos_data_len>0) {
		
				if (mb->wan_pos_data_len > 1030){
					DEBUG_ERROR("MAJORE ERROR !! DATA LEN > 1030\n");
					err=-EFAULT;
					break;
				}
				
				if (copy_to_user(&usr_mb->wan_pos_data[0], 
						 &mb->wan_pos_data[0], 
						 mb->wan_pos_data_len)){
					err= -EFAULT;
					break;
				}
	      		}

			break;

			
		default:
			err= -EOPNOTSUPP;
			break;
	}

	clear_bit(0,&card->wandev.critical);
	return err;
}



/*===========================================================
 * disable_comm_shutdown
 *
 * Shutdown() disables the communications. We must
 * have a sparate functions, because we must not
 * call frmw_error() hander since the private
 * area has already been replaced */

static int disable_comm_shutdown (sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_pos_data_len = 0;
	mb->wan_pos_port_num = card->wandev.comm_port;;
	mb->wan_pos_command = DISABLE_POS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	card->comm_enabled = 0;

	return 0;
}


/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int frmw_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_pos_command;

	switch (err) {

	case 0x33:

		switch(cmd){
		case SEND_ASYNC:
			return -EBUSY;
		}
		
	default:
		DEBUG_EVENT( "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

