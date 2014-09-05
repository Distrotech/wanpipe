/*****************************************************************************
* sdla_bsc.c 	WANPIPE(tm) Multiprotocol WAN Link Driver. 
* 		BiSync module.
*
* Author: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Sep 20, 2001  Nenad Corbic    The min() function has changed for 2.4.9
*                               kernel. Thus using the wp_min() defined in
*                               wanpipe.h
* Aug 14, 2001	Nenad Corbic	Inital version, based on Chdlc module.
*      			        Using Gideons new bitstreaming firmware.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/sdlapci.h>
#include <linux/sdla_bscmp.h>		/* BSTRM firmware API definitions */
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
#include <linux/if_wanpipe.h>		


/****** Defines & Macros ****************************************************/

#define bsc_lock(a,b) spin_lock_irqsave(a,b)
#define bsc_unlock(a,b) spin_unlock_irqrestore(a,b)


/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   		0x01
#define TMR_INT_ENABLED_UPDATE		0x02
#define TMR_INT_ENABLED_CONFIG		0x10

#define MAX_IP_ERRORS	10

#define	BSTRM_DFLT_DATA_LEN	1500		/* default MTU */
#define BSTRM_HDR_LEN		1

#define BSTRM_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_BH_BUFF	10

//#define PRINT_DEBUG
#ifdef PRINT_DEBUG
#define dbg_printk(format, a...) printk(format, ## a)
#else
#define dbg_printk(format, a...)
#endif  

#define _INB(port)		(inb(port))
#define _OUTB(port, byte)	(outb((byte),(port)))

#define POLL_WAIT  (HZ/100)

#define OK 0
#define MAX_ACTIVE_CHAN 200
#define MAX_POLL_CNT	20

#define _BSC_DEBUG_ 1
/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with BSTRM specific data
 */

typedef struct bscmp_private_area
{
	wanpipe_common_t common;
	sdla_t		*card;
	unsigned long 	router_start_time;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;
	u8		config_bstrm;
	u8 		config_bscmp_timeout;
	struct timer_list poll_timer;
	u8 		true_if_encoding;
	unsigned long 	station_change;
	unsigned char   active_chan[MAX_ACTIVE_CHAN];
	unsigned char   cur_dev;
	unsigned long   dev_stop_timeout;
} bscmp_private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/* variable for tracking how many interfaces to open for WANPIPE on the
   two ports */


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);

/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_ioctl (netdevice_t *dev, struct ifreq *ifr, int cmd);
static struct net_device_stats* if_stats (netdevice_t* dev);
  
static int if_send (struct sk_buff* skb, netdevice_t* dev);

/* BSTRM Firmware interface functions */
static int bscmp_comm_disable 	(sdla_t* card);
static int bscmp_send (sdla_t* card, 
		       unsigned char station, 
		       unsigned char misc_tx_rx_bits, 
		       void* data, unsigned len);
static int bscmp_error (sdla_t *card, int err, wan_mbox_t *mb);
static int bscmp_read_link_stats (sdla_t* card);
static int bscmp_read_code_version (sdla_t* card);
static int bscmp_configure (sdla_t* card, BSC_CONFIG_STRUCT *cfg);
static int bscmp_read_config (sdla_t* card);
static int bscmp_list_stations (sdla_t* card);
static int bscmp_read (sdla_t* card, int station);

static void event_poll (unsigned long dev_ptr);
static void port_set_state (sdla_t *card, int state);
static int find_active_chan(bscmp_private_area_t* bscmp_priv_area, int start_chan);


/****** Public Functions ****************************************************/

/*============================================================================
 * Cisco HDLC protocol initialization routine.
 *
 * This routine is called by the main WANPIPE module during setup.  At this
 * point adapter is completely initialized and firmware is running.
 *  o read firmware version (to make sure it's alive)
 *  o configure adapter
 *  o initialize protocol-specific fields of the adapter data space.
 *
 * Return:	0	o.k.
 *		< 0	failure.
 */
int wpbsc_init (sdla_t* card, wandev_conf_t* conf)
{
	int err;
	unsigned long timeout;
	volatile wan_mbox_t*	mbox;

	BSC_CONFIG_STRUCT cfg = { 3, 		//line_speed_number
				  500, 		//max_data_frame_size
				  1,		//secondary_station
				  5, 		//num_consec_PAD_eof
				  2, 		//num_add_lead_SYN
				  0, //1	//conversational_mode
				  0,		//pp_dial_up_operation
				  0, 		//switched_CTS_RTS
				  1, 		//EBCDIC_encoding
				  1, 		//auto_open
				  0, 		//misc_bits
				  1, //0	//protocol_options1  
				  0, 		//protocol_options2
				  0,		//reserved_pp
				  10, 		//max_retransmissions
				  10, 		//fast_poll_retries
				  3, 		//TTD_retries
				  10, 		//restart_timer
				  1000, 	//pp_slow_restart_timer
				  50, 		//TTD_timer
				  100, 		//pp_delay_between_EOT_ENQ
				  50, 		//response_timer
				  100, 		//rx_data_timer
				  40, 		//NAK_retrans_delay_timer
				  50, 		//wait_CTS_timer
				  5, 		//mp_max_consec_ETX
				  0x7f, 	//mp_general_poll_address
				  2000, 	//sec_poll_timeout
				  20, 		//pri_poll_skips_inactive
				  3, 		//sec_additional_stn_send_gpoll
				  10, 		//pri_select_retries
				  0, 		//mp_multipoint_options
				  0		//reserved 0=RS232 1=V35
				};


	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_BSC) {
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Find out which Port to use */
	conf->comm_port = WANOPT_PRI;

	/* Initialize protocol-specific fields */
	if(card->type != SDLA_S514){
		unsigned char tmp=0;

		//FIXME: CHECK THAT MBOX IS RIGHT !!!!
		/* Alex Apr 8 2004 Sangoma ISA card -> card->mbox_off  = 0; */
		card->mbox_off  = BSC_SENDBOX;

		
		card->hw_iface.isa_write_1(card->hw, 0, 0);	
		if (card->hw_iface.mapmem){
			err = card->hw_iface.mapmem(card->hw, 0);
		}else{
			err = -EINVAL;
		}
		if (err) return err;	
	
		card->hw_iface.poke(card->hw,0x10,&cfg,sizeof(cfg));
			
		card->hw_iface.bus_write_1(card->hw, 0x00, 0xC3);   /* Z80: 'jp' opcode */
		card->hw_iface.bus_write_2(card->hw, 0x01, 0x100); 

		card->hw_iface.isa_read_1(card->hw, 0, &tmp);
		card->hw_iface.isa_write_1(card->hw, 0, tmp | 0x02);
		
		timeout=jiffies;
		while((jiffies-timeout) < (HZ/10))
			schedule();
		

		card->hw_iface.isa_read_1(card->hw, 1, &tmp);
		if (!(tmp & 0x02)){	/* verify */
			return -EIO;
		}
		err = (card->hw_iface.mapmem) ?
			card->hw_iface.mapmem(card->hw, BSC_SENDBOX) : -EINVAL; 
		if (err) return err;	
		
	}else{ 
		card->mbox_off  = BSC_SENDBOX;

		card->hw_iface.io_write_1(card->hw, 0x00, S514_CPU_HALT);  

		card->hw_iface.poke(card->hw,0x10,&cfg,sizeof(cfg));

		card->hw_iface.bus_write_1(card->hw, 0x00, 0xC3);   /* Z80: 'jp' opcode */
		card->hw_iface.bus_write_2(card->hw, 0x01, 0x100); 
		card->hw_iface.io_write_1(card->hw, 0x00, S514_CPU_START);

	}
	
	mbox = &card->wan_mbox;

	timeout=jiffies;
	while((jiffies-timeout) < (HZ))
		schedule();

	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (bscmp_read_code_version(card)){
		return -EIO;
	}

	printk(KERN_INFO "%s: Running Bisync firmware v%s\n",
		card->devname, mbox->wan_data); 

	card->isr			= NULL;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= NULL;
	card->wandev.udp_port   	= conf->udp_port;
	card->disable_comm		= NULL;
	card->wandev.new_if_cnt = 0;

	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 

	card->wandev.clocking = conf->clocking;
	card->wandev.mtu = cfg.max_data_frame_size;
	

	printk(KERN_INFO "%s: Configuring Bisync!\n",card->devname);
	bscmp_configure (card,&cfg);	

	
	/* This is for the ports link state */
	card->wandev.state = WAN_DISCONNECTED;

	bscmp_read_config (card);

	printk(KERN_INFO "\n");
	
	return 0;
}


/******* WAN Device Driver Entry Points *************************************/

/*============================================================================
 * Update device status & statistics
 * This procedure is called when updating the PROC file system and returns
 * various communications statistics. These statistics are accumulated from 3 
 * different locations:
 * 	1) The 'if_stats' recorded for the device.
 * 	2) Communication error statistics on the adapter.
 *      3) BSTRM operational statistics on the adapter.
 * The board level statistics are read during a timer interrupt. Note that we 
 * read the error and operational statistics during consecitive timer ticks so
 * as to minimize the time that we are inside the interrupt handler.
 *
 */
static int update (wan_device_t* wandev)
{
	sdla_t* card = wandev->priv;
 	netdevice_t* dev;
        volatile bscmp_private_area_t* bscmp_priv_area;

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical))
                return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL)
		return -ENODEV;

	if((bscmp_priv_area=dev->priv) == NULL)
		return -ENODEV;


	return 0;
}


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
 *		< 0	failure (channel will not be created)
 */
static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	sdla_t* card = wandev->priv;
	bscmp_private_area_t* bscmp_priv_area;


	printk(KERN_INFO "%s: Configuring Interface: %s\n",
			card->devname, conf->name);
 
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		printk(KERN_INFO "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}
		
	/* allocate and initialize private data */
	bscmp_priv_area = kmalloc(sizeof(bscmp_private_area_t), GFP_KERNEL);
	
	if(bscmp_priv_area == NULL) 
		return -ENOMEM;

	memset(bscmp_priv_area, 0, sizeof(bscmp_private_area_t));

	bscmp_priv_area->card = card; 

	bscmp_priv_area->common.sk=NULL;
	bscmp_priv_area->common.state = WAN_CONNECTING;
	bscmp_priv_area->common.dev = dev;

	if(card->wandev.new_if_cnt > 0) {
                kfree(bscmp_priv_area);
		return -EEXIST;
	}

	card->wandev.new_if_cnt++;

	/* Setup wanpipe as a router (WANPIPE) or as an API */
	if (strcmp(conf->usedby, "API") == 0) {
		printk(KERN_INFO "%s: Running in API mode !\n",
			wandev->name);
	}else{
		printk(KERN_INFO "%s: Bisync protocol doesn't support WANPIPE mode! API only!\n",
					wandev->name);
		kfree(bscmp_priv_area);
		return -EINVAL;
	}

	/* prepare network device data space for registration */
	dev->init = &if_init;
	dev->priv = bscmp_priv_area;

	return 0;
}


/****** Network Device Interface ********************************************/

/*============================================================================
 * Initialize Linux network interface.
 *
 * This routine is called only once for each interface, during Linux network
 * interface registration.  Returning anything but zero will fail interface
 * registration.
 */
static int if_init (netdevice_t* dev)
	{
	bscmp_private_area_t* bscmp_priv_area = dev->priv;
	sdla_t* card = bscmp_priv_area->card;
	wan_device_t* wandev = &card->wandev;

	/* Initialize device driver entry points */
	dev->open		= &if_open;
	dev->stop		= &if_close;
	dev->hard_start_xmit	= &if_send;
	dev->get_stats		= &if_stats;
	dev->do_ioctl		= &if_ioctl;
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	dev->tx_timeout		= NULL;
	dev->watchdog_timeo	= TX_TIMEOUT;
#endif

	
	/* Initialize media-specific parameters */
	dev->flags		|= IFF_POINTOPOINT;
	dev->flags		|= IFF_NOARP;

	
	if (bscmp_priv_area->true_if_encoding){
		dev->type	= ARPHRD_HDLC; /* This breaks the tcpdump */
	}else{
		dev->type	= ARPHRD_PPP;
	}
	
	dev->mtu		= card->wandev.mtu;
	/* for API usage, add the API header size to the requested MTU size */
	dev->mtu += sizeof(api_tx_hdr_t);
 
	dev->hard_header_len	= BSTRM_HDR_LEN;

	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMSIZE, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

	/* Set transmit buffer queue length 
	 * If too low packets will not be retransmitted 
         * by stack.
	 */
        dev->tx_queue_len = 100;
   
	return 0;
}

/*============================================================================
 * Open network interface.
 * o enable communications and interrupts.
 * o prevent module from unloading by incrementing use count
 *
 * Return 0 if O.k. or errno.
 */
static int if_open (netdevice_t* dev)
{
	bscmp_private_area_t* bscmp_priv_area = dev->priv;
	sdla_t* card = bscmp_priv_area->card;
	struct timeval tv;
	int err = 0;
	
	/* Only one open per interface is allowed */
	if (open_dev_check(dev))
		return -EBUSY;

	do_gettimeofday(&tv);
	bscmp_priv_area->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

	init_timer(&bscmp_priv_area->poll_timer);
	bscmp_priv_area->poll_timer.data = (unsigned long)dev;
	bscmp_priv_area->poll_timer.function = event_poll;
	bscmp_priv_area->poll_timer.expires=jiffies+POLL_WAIT;
	add_timer(&bscmp_priv_area->poll_timer);
	wanpipe_open(card);
	
	return err;
}

/*============================================================================
 * Close network interface.
 * o if this is the last close, then disable communications and interrupts.
 * o reset flags.
 */
static int if_close (netdevice_t* dev)
{
	bscmp_private_area_t* bscmp_priv_area = dev->priv;
	sdla_t* card = bscmp_priv_area->card;
		
	del_timer(&bscmp_priv_area->poll_timer);

	stop_net_queue(dev);
#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);

	bscmp_comm_disable(card);
	card->wandev.state = WAN_DISCONNECTED;
	return 0;
}



/*============================================================================
 * Send a packet on a network interface.
 * o set tbusy flag (marks start of the transmission) to block a timer-based
 *   transmit from overlapping.
 * o check link state. If link is not up, then drop the packet.
 * o execute adapter send command.
 * o free socket buffer
 *
 * Return:	0	complete (socket buffer must be freed)
 *		non-0	packet may be re-transmitted (tbusy must be set)
 *
 * Notes:
 * 1. This routine is called either by the protocol stack or by the "net
 *    bottom half" (with interrupts enabled).
 * 2. Setting tbusy flag will inhibit further transmit requests from the
 *    protocol stack and can be used for flow control with protocol layer.
 */
static int if_send (struct sk_buff* skb, netdevice_t* dev)
{
	bscmp_private_area_t *bscmp_priv_area = dev->priv;
	sdla_t *card = bscmp_priv_area->card;
	unsigned long smp_flag;
	int err=-EBUSY;

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	netif_stop_queue(dev);
#endif
	
	if (skb == NULL){
		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		printk(KERN_INFO "%s: interface %s got kicked!\n",
			card->devname, dev->name);

		wake_net_dev(dev);
		return 0;
	}
	
#if defined(LINUX_2_1)
	if (dev->tbusy){

		/* If our device stays busy for at least 5 seconds then we will
		 * kick start the device by making dev->tbusy = 0.  We expect 
		 * that our device never stays busy more than 5 seconds. So this
		 * is only used as a last resort. 
		 */
                ++card->wandev.stats.collisions;
		if((jiffies - bscmp_priv_area->tick_counter) < (5 * HZ)) {
			return 1;
		}

		printk (KERN_INFO "%s: Transmit timeout !\n",
			card->devname);

		/* unbusy the interface */
		clear_bit(0,&dev->tbusy);
	}
#endif

	bsc_lock(&card->wandev.lock,smp_flag);
    	if(test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
	
		printk(KERN_INFO "%s: Critical in if_send: %lx\n",
					card->wandev.name,card->wandev.critical);
                ++card->wandev.stats.tx_dropped;
		start_net_queue(dev);
		goto if_send_exit_crit;
	}

	if(card->wandev.state != WAN_CONNECTED){
       		++card->wandev.stats.tx_dropped;
		
	}else if(!skb->protocol){
        	++card->wandev.stats.tx_errors;
		
	}else {
		void* data = skb->data;
		unsigned len = skb->len;
		unsigned char station=0;
		unsigned char misc_tx_rx_bits=0;
		api_tx_hdr_t* api_tx_hdr;

		/* discard the frame if we are configured for */
		/* 'receive only' mode or if there is no data */
		if (len <= sizeof(api_tx_hdr_t)) {
			++card->wandev.stats.tx_dropped;
			goto if_send_exit_crit;
		}
				
		api_tx_hdr = (api_tx_hdr_t *)data;
		station = api_tx_hdr->station;
		misc_tx_rx_bits = api_tx_hdr->misc_tx_rx_bits;
		data += sizeof(api_tx_hdr_t);
		len -= sizeof(api_tx_hdr_t);

		err=bscmp_send(card, station, misc_tx_rx_bits, data, len);
		if (err==0){
			++card->wandev.stats.tx_packets;
                        card->wandev.stats.tx_bytes += len;
			
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		 	dev->trans_start = jiffies;
#endif
		}
	}

	start_net_queue(dev);

if_send_exit_crit:
	
	if (!err) {
		wan_dev_kfree_skb(skb, FREE_WRITE);
	}else{
		stop_net_queue(dev);
		bscmp_priv_area->dev_stop_timeout=jiffies;
	}

	clear_bit(SEND_CRIT, (void*)&card->wandev.critical);
	bsc_unlock(&card->wandev.lock,smp_flag);
	
	return err;
}

static int if_ioctl (netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	bscmp_private_area_t *chan;
	wan_mbox_t	*mbox, *user_mbox;
	sdla_t *card;
	unsigned long smp_flag;
	int err=0;

	if (!dev || !(dev->flags & IFF_UP)){
		return -ENODEV;
	}
	
	if (!(chan = dev->priv)){
		return -ENODEV;
	}

	if (!(card = chan->card)){
		return -ENODEV;
	}

	
	mbox = &card->wan_mbox;
		
	switch(cmd){

	case SIOC_WANPIPE_BIND_SK:
		if (!ifr){
			err= -EINVAL;
			break;
		}
		
		DEBUG_TEST("%s: SIOC_WANPIPE_BIND_SK \n",__FUNCTION__);
		spin_lock_irqsave(&card->wandev.lock,smp_flag);
		err=wan_bind_api_to_svc(chan,ifr->ifr_data);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flag);
		break;

	case SIOC_WANPIPE_UNBIND_SK:
		if (!ifr){
			err= -EINVAL;
			break;
		}

		DEBUG_TEST("%s: SIOC_WANPIPE_UNBIND_SK \n",__FUNCTION__);
		
		spin_lock_irqsave(&card->wandev.lock,smp_flag);
		err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flag);
		break;

	case SIOC_WANPIPE_CHECK_TX:
	case SIOC_ANNEXG_CHECK_TX:
		err=0;
		break;

	case SIOC_WANPIPE_DEV_STATE:
		err = chan->common.state;
		break;

	case SIOC_ANNEXG_KICK:
		err=0;
		break;


	case SIOC_WANPIPE_BSC_CMD:

		user_mbox= (wan_mbox_t*)ifr->ifr_data;
		if (!user_mbox){
			err=-EINVAL;
			break;
		}


		if (copy_from_user(&mbox->wan_command,&user_mbox->wan_command,MBOX_HEADER_SZ)){
			goto if_ioctl_err;
		}
	
		if (mbox->wan_data_len){
			if (copy_from_user(mbox->wan_data,user_mbox->wan_data,mbox->wan_data_len)){
				goto if_ioctl_err;	
			}
		}

		bsc_lock(&card->wandev.lock,smp_flag);

		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

		if (err != OK){
			bscmp_error(card, err, mbox);
			bsc_unlock(&card->wandev.lock,smp_flag);
			goto if_ioctl_err;
		}
		bsc_unlock(&card->wandev.lock,smp_flag);
		

		if (copy_to_user(&user_mbox->wan_command,&mbox->wan_command,MBOX_HEADER_SZ)){
			goto if_ioctl_err;
		}

		if (mbox->wan_data_len){
			if (copy_to_user(user_mbox->wan_data,mbox->wan_data,mbox->wan_data_len)){
				err=-EFAULT;
				goto if_ioctl_err;
			}
		}

		switch (mbox->wan_command){
	
		case ADD_STATION:
		case DELETE_STATION:
		case DELETE_ALL_STATIONS:
		case SET_CONFIGURATION:
			set_bit(0,&chan->station_change);
			break;
		}

if_ioctl_err:
		break;

	default:
		err = -EOPNOTSUPP;
		break;
	}
			
	return err;
}

void event_poll (unsigned long dev_ptr)
{
	netdevice_t *dev = (netdevice_t *)dev_ptr;
	bscmp_private_area_t *bscmp_priv_area = dev->priv;
	sdla_t *card = bscmp_priv_area->card;
	wan_mbox_t*	mbox = &card->wan_mbox;
	struct sk_buff *skb;
	int i, start_dev=0, cur_dev=0;
	api_rx_element_t *rx_el;
	void *buf;
	unsigned long smp_flag;

	bsc_lock(&card->wandev.lock,smp_flag);

	set_bit(SEND_CRIT,&card->wandev.critical);

	if (is_queue_stopped(dev)){
		if ((jiffies-bscmp_priv_area->dev_stop_timeout) > (HZ/20)){
			start_net_queue(dev);	
			wan_wakeup_api(bscmp_priv_area);
		}
	}

	if (bscmp_read_link_stats (card) != 0){		
		goto event_exit;
	}	

	if (mbox->wan_data[0] == 0){
		port_set_state (card, WAN_DISCONNECTED);
		goto event_exit;
	}else{
		port_set_state (card, WAN_CONNECTED);
	}
	
	/* Data available for read */
	if (!mbox->wan_data[1]){
		goto event_exit;
	}

	if (test_bit(0,&bscmp_priv_area->station_change)){
	
		memset(bscmp_priv_area->active_chan, 0,
		       sizeof(bscmp_priv_area->active_chan));
		
		if (bscmp_list_stations(card) != 0){
			goto event_exit;
		}
		clear_bit(0,&bscmp_priv_area->station_change);
		
		for (i=0;i<mbox->wan_data_len;){
			bscmp_priv_area->active_chan[mbox->wan_data[i]]=1;
			i+=10;
		}
		bscmp_priv_area->cur_dev=mbox->wan_data[0];
	}
#if _BSC_DEBUG_
	printk(KERN_INFO "Starting Rx with station %i\n",bscmp_priv_area->cur_dev);
#endif
	start_dev=bscmp_priv_area->cur_dev;
	
	for (i=0;i<MAX_POLL_CNT;i++){

		cur_dev=find_active_chan(bscmp_priv_area, start_dev);
		if (cur_dev < 0){
#if _BSC_DEBUG_
			printk(KERN_INFO "No Active Channel!\n");
#endif
			break;
		}
		
#if _BSC_DEBUG_
		printk(KERN_INFO "Rx for station %i\n",cur_dev);
#endif
		
		if (bscmp_read (card, cur_dev) == 0){

#if _BSC_DEBUG_
			printk(KERN_INFO "Station %i : Rx Pkt : Size %i\n",
					cur_dev,mbox->wan_data_len);
#endif
			
			skb = dev_alloc_skb(mbox->wan_data_len+sizeof(api_rx_hdr_t)+5);
			if (!skb){
				printk(KERN_INFO "%s: Failed to allocate skb!\n",
						card->devname);
				goto event_exit;
			}
	
			rx_el=(api_rx_element_t*)skb_put(skb,sizeof(api_rx_hdr_t));
			memset(rx_el,0,sizeof(api_rx_hdr_t));
			rx_el->api_rx_hdr.station = cur_dev;
	
			buf=(api_rx_element_t*)skb_put(skb,mbox->wan_data_len);
			memcpy(buf,mbox->wan_data,mbox->wan_data_len);
		
			skb->protocol = htons(PVC_PROT);
     			wan_skb_reset_mac_header(skb);
			skb->dev      = dev;
               		skb->pkt_type = WAN_PACKET_DATA;

			if (wan_api_rx(bscmp_priv_area,skb) != 0){
				printk(KERN_INFO "%s: Failed to rx data via socket, socket full!\n",
						card->devname);
				dev_kfree_skb(skb);
				++card->wandev.stats.rx_dropped;
				goto event_exit;
			}else{
				++card->wandev.stats.rx_packets;
                       	 	card->wandev.stats.rx_bytes += mbox->wan_data_len;
			}
			skb=NULL;
		}
	}

event_exit:
	clear_bit(SEND_CRIT,&card->wandev.critical);
	bscmp_priv_area->poll_timer.expires=jiffies+POLL_WAIT;
	add_timer(&bscmp_priv_area->poll_timer);

	bsc_unlock(&card->wandev.lock,smp_flag);
	return;	
}


/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	sdla_t *my_card;
	bscmp_private_area_t* bscmp_priv_area;

	if ((bscmp_priv_area=dev->priv) == NULL)
		return NULL;

	my_card = bscmp_priv_area->card;
	return &my_card->wandev.stats; 
}



/****** BiSync Firmware Interface Functions *******************************/


static int bscmp_comm_disable (sdla_t* card)
{
	int err;
	wan_mbox_t*	mb = &card->wan_mbox;
	
	memset(mb,0,MBOX_HEADER_SZ);

	mb->wan_data_len = 0;
	mb->wan_command = CLOSE_LINK;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != OK)
		bscmp_error(card, err, mb);
	
	return err;
}



static int bscmp_read (sdla_t* card, int station)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);
        mb->wan_data_len = 0;
	mb->wan_bsc_station = station;
        mb->wan_command = BSC_READ;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscmp_error(card,err,mb);
        return err;
}


static int bscmp_read_link_stats (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);

        mb->wan_data_len = 0;
        mb->wan_command = LINK_STATUS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscmp_error(card,err,mb);
        return err;
}

static int bscmp_read_code_version (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);
        mb->wan_data_len = 0;
        mb->wan_command = READ_CODE_VERSION;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscmp_error(card,err,mb);
        return err;
}

static int bscmp_configure (sdla_t* card, BSC_CONFIG_STRUCT *cfg)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);

        mb->wan_data_len = sizeof(BSC_CONFIG_STRUCT);
        mb->wan_command = SET_CONFIGURATION;
	memcpy(mb->wan_data,cfg,sizeof(BSC_CONFIG_STRUCT));
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscmp_error(card,err,mb);

        return err;
}

static int bscmp_read_config (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);
        mb->wan_data_len = 0;
        mb->wan_command = READ_CONFIGURATION;

        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscmp_error(card,err,mb);

        return err;
}

static int bscmp_list_stations (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;
	memset(mb,0,MBOX_HEADER_SZ);
        mb->wan_data_len = 0;
        mb->wan_command = LIST_STATIONS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscmp_error(card,err,mb);

        return err;
}




/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int bscmp_send (sdla_t* card, unsigned char station, 
		       unsigned char misc_tx_rx_bits, 
		       void* data, unsigned len)
{
	wan_mbox_t*	mbox = &card->wan_mbox;
	int err;

	memset(mbox,0,MBOX_HEADER_SZ);
	mbox->wan_command = BSC_WRITE;
	mbox->wan_data_len = len;
	mbox->wan_bsc_station = station;
	mbox->wan_bsc_misc_bits=misc_tx_rx_bits;

	memcpy(mbox->wan_data,data,len);

	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
        if (err != OK && err != 0x02)
                bscmp_error(card,err,mbox);
	
	return err;
}

/****** Firmware Error Handler **********************************************/

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int bscmp_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

	case CMD_TIMEOUT:
		printk(KERN_INFO "%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		break;

	default:
		printk(KERN_INFO "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

static void port_set_state (sdla_t *card, int state)
{
	netdevice_t *dev;

        if (card->wandev.state != state)
        {
                switch (state)
                {
                case WAN_CONNECTED:
                        printk (KERN_INFO "%s: Link connected!\n",
                                card->devname);
                      	break;

                case WAN_CONNECTING:
                        printk (KERN_INFO "%s: Link connecting...\n",
                                card->devname);
                        break;

                case WAN_DISCONNECTED:
                        printk (KERN_INFO "%s: Link disconnected!\n",
                                card->devname);
                        break;
                }

                card->wandev.state  = state;
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			bscmp_private_area_t* bscmp_priv_area = dev->priv;
			bscmp_priv_area->common.state = state;
	
			wan_update_api_state(bscmp_priv_area);
		}
        }
}

static int find_active_chan(bscmp_private_area_t* bscmp_priv_area, int start_chan)
{
	int i=bscmp_priv_area->cur_dev;

#if _BSC_DEBUG_
	printk(KERN_INFO "\n");
	printk(KERN_INFO "Finding Station: Start=%i Curr=%i\n",
			start_chan,i);
#endif	
	for (;;){
		if (bscmp_priv_area->active_chan[i]){
			bscmp_priv_area->cur_dev=i;
			if (++bscmp_priv_area->cur_dev >= MAX_ACTIVE_CHAN){
				bscmp_priv_area->cur_dev=0;
			}
			return i;
		}
	
		if (++i >= MAX_ACTIVE_CHAN){
			i=0;
		}

		bscmp_priv_area->cur_dev=i;

		if (i==start_chan)
			break;
	}
	return -ENODEV;
}

/****** End ****************************************************************/
