/*****************************************************************************
* sdla_bscstrm.c WANPIPE(tm) Multiprotocol WAN Link Driver. 
* 		 BiSync Streaming module.
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
* Mar , 2001	Nenad Corbic	Inital version, based on bsc module.
*      			        Using Gideons new bisync streaming firmware.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/sdlapci.h>
#include <linux/sdla_bscstrm.h>		/* BSTRM firmware API definitions */
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
#include <linux/if_wanpipe.h>		



/****** Defines & Macros ****************************************************/

#undef _BSC_DEBUG_

#if defined(LINUX_2_4)||defined(LINUX_2_6)
 #define bsc_lock(a,b)   {b=0;spin_lock_bh(a);}	
 #define bsc_unlock(a,b) {b=0;spin_unlock_bh(a);}
#elif defined (LINUX_2_1)
 #define bsc_lock(a,b) spin_lock_irqsave(a,b)
 #define bsc_unlock(a,b) spin_unlock_irqrestore(a,b)
#else
 #define bsc_lock(a,b)
 #define bsc_unlock(a,b)		 
#endif


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


#define OK 0
#define MAX_ACTIVE_CHAN 200
#define MAX_POLL_CNT	20
/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with BSTRM specific data
 */

typedef struct bscstrm_private_area
{
	wanpipe_common_t common;
	sdla_t		*card;
	unsigned long 	router_start_time;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;
	u8		config_bstrm;
	u8 		config_bscstrm_timeout;
	u8 		true_if_encoding;
	unsigned char   active_chan[MAX_ACTIVE_CHAN];
	unsigned char   cur_dev;
} bscstrm_private_area_t;

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
static int del_if (wan_device_t* wandev, netdevice_t* dev);

/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_ioctl (netdevice_t *dev, struct ifreq *ifr, int cmd);
static struct net_device_stats* if_stats (netdevice_t* dev);
  
static int if_send (struct sk_buff* skb, netdevice_t* dev);

/* BSTRM Firmware interface functions */
static int bscstrm_comm_enable (sdla_t* card);
static int bscstrm_comm_disable 	(sdla_t* card);
static int bscstrm_error (sdla_t *card, int err, wan_mbox_t *mb);
static int bscstrm_read_code_version (sdla_t* card);
static int bscstrm_configure (sdla_t* card, CONFIGURATION_STRUCT *cfg);
static int bscstrm_read_config (sdla_t* card);
static int bscstrm_enable_rx_isr (sdla_t* card);

static WAN_IRQ_RETVAL wp_bscstrm_isr (sdla_t *card);
static void port_set_state (sdla_t *card, int state);

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
#undef NASDAQ_BSC
int wp_bscstrm_init (sdla_t* card, wandev_conf_t* conf)
{
	int err;
	unsigned long timeout;
	wan_mbox_t*	mbox;

#ifdef NASDAQ_BSC
	
	CONFIGURATION_STRUCT cfg = {
		0,		/* baud rate */
		0,		/* the adapter frequecy */
       		1000,		/* the maximum length of a BSC data block */
        	0,		/* EBCDIC/ASCII encoding */
               	RX_BLOCK_TYPE_3,/* the type of BSC block to be received */
       		3,		/* the number of consecutive PADs indicating 
				 * the end of the block */
    		0,		/* the number of additional leading transmit 
				 *  SYN characters */
          	8,		/* the number of bits per character */
                ODD_PARITY,	/* parity */
        	0,		/* miscellaneous configuration options */
       		(RX_STATISTICS| 
		 RX_TIME_STAMP| 
		 TX_STATISTICS),	/* statistic options */
        	0 };		/* modem configuration options */

	printk(KERN_INFO "%s: Configuring for NASDAQ BiSync streaming\n",
			card->devname);

#else

	CONFIGURATION_STRUCT cfg = {
		0,		/* baud rate */
		0,		/* the adapter frequecy */
       		1000,		/* the maximum length of a BSC data block */
        	1,		/* EBCDIC/ASCII encoding */
               	RX_BLOCK_TYPE_1,/* the type of BSC block to be received */
       		3,		/* the number of consecutive PADs indicating 
				 * the end of the block */
    		0,		/* the number of additional leading transmit 
				 *  SYN characters */
          	8,		/* the number of bits per character */
                NO_PARITY,	/* parity */
        	0,		/* miscellaneous configuration options */
       		(RX_STATISTICS| 
		 RX_TIME_STAMP| 
		 TX_STATISTICS),	/* statistic options */
        	0 };		/* modem configuration options */

	printk(KERN_INFO "%s: Configuring for BiSync streaming\n",
			card->devname);

#endif	
	
	
	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_BSCSTRM) {
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Find out which Port to use */
	card->wandev.comm_port = conf->comm_port;

	/* Initialize protocol-specific fields */
	if(card->type != SDLA_S514){

		//FIXME: CHECK THAT MBOX IS RIGHT !!!!
		/* Alex Apr 8 2004 Sangoma ISA card -> card->mbox_off  = 0; */
		card->mbox_off = BASE_ADDR_SEND_MB;
		if (card->hw_iface.mapmem){
			err = card->hw_iface.mapmem(card->hw, BASE_ADDR_SEND_MB);
		}else{
			err = -EINVAL;
		}
		if (err){ 
			return err;	
		}
		
	}else{ 
		card->mbox_off = BASE_ADDR_SEND_MB;
	}
	
	mbox=&card->wan_mbox;
	
	timeout=jiffies;
	while((jiffies-timeout) < (HZ)){
		schedule();
	}

	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (bscstrm_read_code_version(card)){
		card->wandev.state = WAN_DISCONNECTED;
		return -EIO;
	}

	printk(KERN_INFO "%s: Running Bisync Streaming firmware v%s\n",
		card->devname, mbox->wan_data); 

	card->isr			= &wp_bscstrm_isr;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->wandev.udp_port   	= conf->udp_port;
	card->disable_comm		= NULL;
	card->wandev.new_if_cnt = 0;

	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 

	card->wandev.clocking = conf->clocking;
	card->wandev.mtu = 1000;


	if (card->wandev.clocking == WANOPT_EXTERNAL){
		card->wandev.bscstrm_cfg.baud_rate = 0;
	}else{
		card->wandev.bscstrm_cfg.baud_rate = conf->bps;
	}

	memcpy(&card->wandev.bscstrm_cfg, &conf->u.bscstrm, sizeof(wan_bscstrm_conf_t));

	cfg.baud_rate 			= card->wandev.bscstrm_cfg.baud_rate;
   	cfg.adapter_frequency		= card->wandev.bscstrm_cfg.adapter_frequency; 
   	cfg.max_data_length		= card->wandev.bscstrm_cfg.max_data_length;	
   	cfg.EBCDIC_encoding		= card->wandev.bscstrm_cfg.EBCDIC_encoding;	
   	cfg.Rx_block_type		= card->wandev.bscstrm_cfg.Rx_block_type;	
   	cfg.no_consec_PADs_EOB		= card->wandev.bscstrm_cfg.no_consec_PADs_EOB;
   	cfg.no_add_lead_Tx_SYN_chars	= card->wandev.bscstrm_cfg.no_add_lead_Tx_SYN_chars;
   	cfg.no_bits_per_char		= card->wandev.bscstrm_cfg.no_bits_per_char;
   	cfg.parity			= card->wandev.bscstrm_cfg.parity;
   	cfg.misc_config_options		= card->wandev.bscstrm_cfg.misc_config_options;
   	cfg.statistics_options		= card->wandev.bscstrm_cfg.statistics_options;
   	cfg.modem_config_options	= card->wandev.bscstrm_cfg.modem_config_options;

	printk(KERN_INFO "%s: Bisync Streaming Config: \n",card->devname);
	printk(KERN_INFO "%s:     Comm Port         = %s\n",
			card->devname,card->wandev.comm_port==0?"PRI":"SEC");
	printk(KERN_INFO "%s:     Baud Rate         = %lu\n",
			card->devname,card->wandev.bscstrm_cfg.baud_rate);
	printk(KERN_INFO "%s:     Adapter Frequency = %lu\n",
			card->devname,card->wandev.bscstrm_cfg.adapter_frequency);
	printk(KERN_INFO "%s:     Max Data Len      = %u\n",
			card->devname,card->wandev.bscstrm_cfg.max_data_length);
	printk(KERN_INFO "%s:     EBCDIC Encoding   = %u\n",
			card->devname,card->wandev.bscstrm_cfg.EBCDIC_encoding);
	printk(KERN_INFO "%s:     Rx Block Type     = %u\n",
			card->devname,card->wandev.bscstrm_cfg.Rx_block_type);
	printk(KERN_INFO "%s:     Num Cons PADs Eob = %u\n",
			card->devname,card->wandev.bscstrm_cfg.no_consec_PADs_EOB);
	printk(KERN_INFO "%s:     Num Lead Tx Syn Ch= %u\n",
			card->devname,card->wandev.bscstrm_cfg.no_add_lead_Tx_SYN_chars);
	printk(KERN_INFO "%s:     Num Bits Per Char = %u\n",
			card->devname,card->wandev.bscstrm_cfg.no_bits_per_char);
	printk(KERN_INFO "%s:     Parity            = %u\n",
			card->devname,card->wandev.bscstrm_cfg.parity);
	printk(KERN_INFO "%s:     Misc Config Opt   = 0x%X\n",
			card->devname,card->wandev.bscstrm_cfg.misc_config_options);
	printk(KERN_INFO "%s:     Statistics Opt    = 0x%X\n",
			card->devname,card->wandev.bscstrm_cfg.statistics_options);
	printk(KERN_INFO "%s:     Modem Config Opt  = 0x%X\n",
			card->devname,card->wandev.bscstrm_cfg.modem_config_options);
	

	printk(KERN_INFO "%s: Configuring Bisync Streaming!\n",card->devname);
	if (bscstrm_configure (card,&cfg) != 0){
		printk(KERN_INFO "%s: Failed to configure BiSync Streaming\n",
				card->devname);
		return -EINVAL;
	}
	
	/* This is for the ports link state */
	card->wandev.state = WAN_DISCONNECTED;

	bscstrm_read_config(card);

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
        volatile bscstrm_private_area_t* bscstrm_priv_area;

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

	if((bscstrm_priv_area=dev->priv) == NULL)
		return -ENODEV;


	return 0;
}

static void bscstrm_bh (unsigned long data)
{
	DEBUG_EVENT("%s: Not used!\n",__FUNCTION__);
	return;
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
	bscstrm_private_area_t* bscstrm_priv_area;


	printk(KERN_INFO "%s: Configuring Interface: %s\n",
			card->devname, conf->name);

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		printk(KERN_INFO "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	if (card->wandev.new_if_cnt){
		return -EEXIST;
	}
	
	/* allocate and initialize private data */
	bscstrm_priv_area = kmalloc(sizeof(bscstrm_private_area_t), GFP_KERNEL);
	
	if(bscstrm_priv_area == NULL) 
		return -ENOMEM;

	memset(bscstrm_priv_area, 0, sizeof(bscstrm_private_area_t));

	bscstrm_priv_area->card = card; 

	bscstrm_priv_area->common.sk = NULL;
	bscstrm_priv_area->common.state = WAN_DISCONNECTED;
	bscstrm_priv_area->common.dev = dev;

	if(card->wandev.new_if_cnt > 0) {
                kfree(bscstrm_priv_area);
		return -EEXIST;
	}


	/* Setup wanpipe as a router (WANPIPE) or as an API */
	if (strcmp(conf->usedby, "API") == 0) {
		printk(KERN_INFO "%s: Running in API mode !\n",
			wandev->name);
	}else{
		printk(KERN_INFO "%s: Bisync protocol doesn't support WANPIPE mode! API only!\n",
					wandev->name);
		kfree(bscstrm_priv_area);
		return -EINVAL;
	}

	wan_reg_api(bscstrm_priv_area, dev, card->devname);
	WAN_TASKLET_INIT((&bscstrm_priv_area->common.bh_task),0,bscstrm_bh,(unsigned long)bscstrm_priv_area);

	/* prepare network device data space for registration */
	card->rxmb_off = card->mbox_off + 0x1000;

	dev->init = &if_init;
	dev->priv = bscstrm_priv_area;

	card->wandev.new_if_cnt++;
	return 0;
}

static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	bscstrm_private_area_t*	chan = dev->priv;
	sdla_t*			card = chan->card;

	WAN_TASKLET_KILL(&chan->common.bh_task);
	wan_unreg_api(chan, card->devname);
	
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
	bscstrm_private_area_t* bscstrm_priv_area = dev->priv;
	sdla_t* card = bscstrm_priv_area->card;
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

	
	if (bscstrm_priv_area->true_if_encoding){
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
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

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
	bscstrm_private_area_t* bscstrm_priv_area = dev->priv;
	sdla_t* card = bscstrm_priv_area->card;
	struct timeval tv;
	int err = 0;
	
	/* Only one open per interface is allowed */
	if (open_dev_check(dev))
		return -EBUSY;

	do_gettimeofday(&tv);
	bscstrm_priv_area->router_start_time = tv.tv_sec;

	err=bscstrm_enable_rx_isr(card);
	if (err){
		printk(KERN_INFO "%s: Failed to enable BiSync Streaming Rx ISR.\n",
				card->devname);
		return -EIO;
	}
	printk(KERN_INFO "%s: Enabling bscstrm rx isr\n",card->devname);

	
	err=bscstrm_comm_enable (card);
	if (err){
		printk(KERN_INFO "%s: Failed to enable BiSync Streaming Communications.\n",
				card->devname);
		return -EINVAL;
	}
	
	printk(KERN_INFO "%s: Enabling bscstrm communication\n",card->devname);

	
	netif_start_queue(dev);

	port_set_state(card,WAN_CONNECTED);

	wanpipe_open(card);
	return 0;
}

/*============================================================================
 * Close network interface.
 * o if this is the last close, then disable communications and interrupts.
 * o reset flags.
 */
static int if_close (netdevice_t* dev)
{
	bscstrm_private_area_t* bscstrm_priv_area = dev->priv;
	sdla_t* card = bscstrm_priv_area->card;
		
	stop_net_queue(dev);
#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);

	bscstrm_comm_disable(card);
	card->wandev.state = WAN_DISCONNECTED;
	return 0;
}


/*============================================================================
 * Send a packet on a network interface.
 *
 * This routine is not being used
 * since the bscstrmp protocol work with
 * ioctl() calls.
 *
 * Furthermore, bscstrm driver is receive only.
 */
static int if_send (struct sk_buff* skb, netdevice_t* dev)
{
	bscstrm_private_area_t *chan = dev->priv;
	sdla_t *card = chan->card;
	wan_mbox_t *mbox = &card->wan_mbox;
	unsigned long smp_flags;
	api_tx_element_t *api_tx_el = (api_tx_element_t *)&skb->data[0];
	int err;
	int len = skb->len-sizeof(api_tx_hdr_t);

	if (len <= 0){
		DEBUG_EVENT("%s: Error: Tx size (%i) <= tx element size (%i)\n",
				card->devname,skb->len,sizeof(api_tx_hdr_t));
		start_net_queue(dev);
		return 1;
	}
	
	if (chan->common.state != WAN_CONNECTED){
		stop_net_queue(dev);
		++card->wandev.stats.tx_carrier_errors;
		return 1;
	}

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	if (card->next){
		wan_spin_lock(&card->next->wandev.lock);
	}

	mbox->wan_command=BSC_WRITE;
	mbox->wan_data_len=len;
	mbox->wan_bscstrm_port=card->wandev.comm_port;
	mbox->wan_bscstrm_misc_bits=api_tx_el->api_tx_hdr.misc_tx_rx_bits;
	memcpy(mbox->wan_data,api_tx_el->data,len);

	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	if (card->next){
		wan_spin_unlock(&card->next->wandev.lock);
	}
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	if (err != 0){
	
		if (err != TX_BUFFERS_FULL){
			DEBUG_EVENT("%s: Bisync Tx error 0x%X\n",
					card->devname,err);
		}
		
		start_net_queue(dev);
		return 1;
	}	
		
	++card->wandev.stats.tx_packets;
	card->wandev.stats.tx_bytes=len;

	start_net_queue(dev);
	wan_dev_kfree_skb(skb, FREE_WRITE);

	return 0;
}

static int if_ioctl (netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	bscstrm_private_area_t *chan;
	wan_mbox_t	*mbox, *user_mbox;
	sdla_t *card;
	int err=0;
	unsigned long smp_flags;

	if (!dev || !(dev->flags & IFF_UP)){
		return -ENODEV;
	}
	
	if (!(chan = dev->priv)){
		return -ENODEV;
	}

	if (!(card = chan->card)){
		return -ENODEV;
	}

	mbox=&card->wan_mbox;

	switch(cmd){

	case SIOC_WANPIPE_BIND_SK:
		if (!ifr){
			err= -EINVAL;
			break;
		}
		
		DEBUG_TEST("%s: SIOC_WANPIPE_BIND_SK \n",__FUNCTION__);
		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
		err=wan_bind_api_to_svc(chan,ifr->ifr_data);
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		break;

	case SIOC_WANPIPE_UNBIND_SK:
		if (!ifr){
			err= -EINVAL;
			break;
		}

		DEBUG_TEST("%s: SIOC_WANPIPE_UNBIND_SK \n",__FUNCTION__);
		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
		err=wan_unbind_api_from_svc(chan, ifr->ifr_data);
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
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

	default:

		user_mbox= (wan_mbox_t*)ifr->ifr_data;

		if (!user_mbox){
			err=-EINVAL;
			break;
		}

		if (cmd != SIOC_WANPIPE_BSC_CMD){
			return -EOPNOTSUPP;
		}

		memset(mbox,0,sizeof(wan_cmd_t));

		if (copy_from_user(&mbox->wan_command,&user_mbox->wan_command,sizeof(wan_cmd_t))){
			return -EFAULT;
		}
	
		if (mbox->wan_data_len){
			if (copy_from_user(mbox->wan_data,user_mbox->wan_data,mbox->wan_data_len)){
				return -EFAULT;
			}
		}

		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

		if (err != OK){
			bscstrm_error(card, err, mbox);
			return -EINVAL;
		}

		if (copy_to_user(&user_mbox->wan_command,&mbox->wan_command,sizeof(wan_cmd_t))){
			return -EFAULT;
		}
	
		if (mbox->wan_data_len){
			if (copy_to_user(user_mbox->wan_data,mbox->wan_data,mbox->wan_data_len)){
				return -EFAULT;
			}
		}
		break;
	}

	return err;
}



static WAN_IRQ_RETVAL wp_bscstrm_isr (sdla_t *card)
{
	netdevice_t		*dev;
	bscstrm_private_area_t	*bscstrm_priv_area;
	wan_mbox_t		*mbox = &card->wan_mbox;
	struct sk_buff		*skb;
	api_rx_element_t	*rx_el;
	unsigned char		reset_byte=0;
	void *buf;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	if ((bscstrm_priv_area = dev->priv) == NULL){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	card->hw_iface.peek(card->hw, card->rxmb_off, mbox, sizeof(wan_cmd_t));
	if (mbox->wan_data_len){
		card->hw_iface.peek(card->hw, 
				    card->rxmb_off+offsetof(wan_mbox_t, wan_data), 
				    mbox->wan_data, mbox->wan_data_len);
	}

	DEBUG_TEST("%s: RX Intr: Flag=0x%X, Port=%d, Sk=%p\n",
			card->devname,
			mbox->wan_opp_flag,mbox->wan_bscstrm_port,
			bscstrm_priv_area->common.sk);
	
	set_bit(0,&card->wandev.critical);

	if (!bscstrm_priv_area->common.sk){
		++card->wandev.stats.rx_dropped;
		goto event_exit;
	}
	
	if (mbox->wan_opp_flag && mbox->wan_bscstrm_port == card->wandev.comm_port){
	
#ifdef _BSC_DEBUG_
		printk(KERN_INFO "Rx Pkt : Size %i: Gcnt %i\n",
				mbox->wan_data_len,++gcount);
#endif
		
		skb = dev_alloc_skb(mbox->wan_data_len+sizeof(api_rx_hdr_t)+5);
		if (!skb){
			printk(KERN_INFO "%s: Failed to allocate skb!\n",
					card->devname);
			mbox->wan_opp_flag=0;
			card->hw_iface.poke(card->hw, card->rxmb_off, mbox, sizeof(wan_cmd_t)); 
			card->hw_iface.poke(card->hw,0xFFF0,&reset_byte,sizeof(reset_byte));
			++card->wandev.stats.rx_dropped;
			goto event_exit;
		}

		rx_el=(api_rx_element_t*)skb_put(skb,sizeof(api_rx_hdr_t));
		memset(rx_el,0,sizeof(api_rx_hdr_t));

		buf=(api_rx_element_t*)skb_put(skb,mbox->wan_data_len);
		memcpy(buf,mbox->wan_data,mbox->wan_data_len);
		skb->protocol = htons(PVC_PROT);
		wan_skb_reset_mac_header(skb);
		skb->dev      = dev;
		skb->pkt_type = WAN_PACKET_DATA;

		if (wan_api_rx(bscstrm_priv_area,skb) != 0){
			dev_kfree_skb(skb);
			++card->wandev.stats.rx_dropped;
			mbox->wan_opp_flag=0;
			card->hw_iface.poke(card->hw, card->rxmb_off, mbox, sizeof(wan_cmd_t)); 
			card->hw_iface.poke(card->hw,0xFFF0,&reset_byte,sizeof(reset_byte));
			goto event_exit;
		}else{
			++card->wandev.stats.rx_packets;
			card->wandev.stats.rx_bytes += mbox->wan_data_len;
		}

		mbox->wan_opp_flag=0;
		card->hw_iface.poke(card->hw, card->rxmb_off, mbox, sizeof(wan_cmd_t)); 
		card->hw_iface.poke(card->hw,0xFFF0,&reset_byte,sizeof(reset_byte));
	}

event_exit:
	if (mbox->wan_opp_flag && mbox->wan_bscstrm_port == card->wandev.comm_port){
		mbox->wan_opp_flag=0;
		card->hw_iface.poke(card->hw, card->rxmb_off, mbox, sizeof(wan_cmd_t));
		card->hw_iface.poke(card->hw,0xFFF0,&reset_byte,sizeof(reset_byte));
	}

	clear_bit(0,&card->wandev.critical);
	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);	
}


/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	sdla_t *my_card;
	bscstrm_private_area_t* bscstrm_priv_area;

	if ((bscstrm_priv_area=dev->priv) == NULL)
		return NULL;

	my_card = bscstrm_priv_area->card;
	return &my_card->wandev.stats; 
}



/****** BiSync Firmware Interface Functions *******************************/

static int bscstrm_comm_disable (sdla_t* card)
{
	int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);

	mb->wan_data_len = 0;
	mb->wan_command = DISABLE_COMMUNICATIONS;
	mb->wan_bscstrm_port=card->wandev.comm_port;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != OK)
		bscstrm_error(card, err, mb);
	
	return err;
}

static int bscstrm_comm_enable (sdla_t* card)
{
	int err;
	wan_mbox_t*	mb = &card->wan_mbox;
	memset(mb,0,MBOX_HEADER_SZ);

	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_COMMUNICATIONS;
	mb->wan_bscstrm_port=card->wandev.comm_port;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != OK)
		bscstrm_error(card, err, mb);
	
	return err;
}



static int bscstrm_read_code_version (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);

        mb->wan_data_len = 0;
        mb->wan_command = READ_CODE_VERSION;
	mb->wan_bscstrm_port=card->wandev.comm_port;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscstrm_error(card,err,mb);
        return err;
}

static int bscstrm_configure (sdla_t* card, CONFIGURATION_STRUCT *cfg)
{
        int err;
        wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);
        mb->wan_data_len = sizeof(CONFIGURATION_STRUCT);
        mb->wan_command = SET_CONFIGURATION;
	mb->wan_bscstrm_port=card->wandev.comm_port;
	memcpy(mb->wan_data,cfg,sizeof(CONFIGURATION_STRUCT));
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscstrm_error(card,err,mb);

        return err;
}

static int bscstrm_read_config (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);

        mb->wan_data_len = 0;
        mb->wan_command = READ_CONFIGURATION;
	mb->wan_bscstrm_port=card->wandev.comm_port;

        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscstrm_error(card,err,mb);

        return err;
}

static int bscstrm_enable_rx_isr (sdla_t* card)
{
        int err;
	wan_mbox_t*	mb = &card->wan_mbox;

	memset(mb,0,MBOX_HEADER_SZ);
        mb->wan_data_len = 4;
        mb->wan_command = SET_INTERRUPT_TRIGGERS;
	mb->wan_bscstrm_port=card->wandev.comm_port;

	mb->wan_data[0]=0x01;
	mb->wan_data[1]=0x00;
	mb->wan_data[2]=0x00;
	mb->wan_data[3]=card->wandev.irq;
	
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != OK)
                bscstrm_error(card,err,mb);

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
static int bscstrm_error (sdla_t *card, int err, wan_mbox_t *mb)
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
	netdevice_t	*dev;
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
		if (dev && wan_netif_priv(dev)){
			bscstrm_private_area_t	*bscstrm_priv_area = wan_netif_priv(dev);
			bscstrm_priv_area->common.state = state;
			wan_update_api_state(bscstrm_priv_area);
		}
        }
}

/****** End ****************************************************************/
