/*****************************************************************************
* sdla_edu.c	WANPIPE(tm) Multiprotocol WAN Link Driver. Educational kit module.
*
* Authors: 	David Rokhvarg <drokhvarg@sangoma.com>
*		 
*
* Copyright:	(c) 2001 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Sep 18, 2001  David Rokhvarg  Initial version.
*
* Jul 30, 2003  David Rokhvarg  v 1.0
* 	1. sdla_load() is not used anymore,
* 	   all the loading is done from user mode instead.
* 	   All IOCTLs used for loading are not valid anymore.
* 	2. New IOCTLs added for starting and stopping
* 	   the card.
* 	 
* Nov 17, 2004  David Rokhvarg  v 2.0
* 	1. Removed globals and put into private area
* 	2. New IOCTLs added for TCP/IP handling.	
* 	   	        
******************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe.h>		// sdla_t

#include <linux/sdla_chdlc.h>		// for 'edu_kit.sfm' (which is actually CHDLC) firmware API definitions
#include <linux/sdla_edu.h>		// educational package definitions
#include <linux/sdlapci.h>		// S514_CPU_START

#include <linux/if_wanpipe_common.h>    // Socket Driver common area
#include <linux/wanpipe_debug.h>    // Socket Driver common area

/****** Defines & Macros ****************************************************/
//#define DEBUG		//define this to get output when running general IOCTLs
//#define DEBUG_RW	//define this to get output when reading/writing

/******Data Structures*****************************************************/
typedef struct edu_private_area
{
	sdla_t*		card;
	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned long 	update_comms_stats;	/* updating comms stats */
	unsigned long 	router_start_time;
	unsigned long 	interface_down;
	unsigned long 	tick_counter;		/* For 5s timeout counter */

	u8 gateway;
	u8 true_if_encoding;

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;

	char if_name[WAN_IFNAME_SZ+1];

	//the one and only network interface created	
	netdevice_t *net_dev;
	
	//these 2 used to copy data from/to user mode
	unsigned char data_buffer[MAX_TX_RX_DATA_SIZE];
	edu_exec_cmd_t ioctl_cmd;

	//temporary buffer for tx data. stored until 'EduKit' or
	//if_tx_timeout() frees it. 
	TX_RX_DATA	tmp_tx_data;
	unsigned char tmp_tx_buff_state; //0 - free, 1 - busy

}edu_private_area_t;


/******** Globals *********************************************************/


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int wpedu_exec (struct sdla *card, void *u_cmd, void *u_data);

static void DoIoctl(sdla_t *card);
static void read_block(sdla_t *card);
static void write_block(sdla_t * card);


/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);
static struct net_device_stats* if_stats (netdevice_t* dev);
static int if_send (struct sk_buff* skb, netdevice_t* dev);
static void if_tx_timeout (netdevice_t *dev);

static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf);
static int del_if (wan_device_t* wandev, netdevice_t* dev);

static int edu_send (	sdla_t* card, edu_private_area_t *edu_priv_area,
	      		void* data, unsigned len, unsigned char tx_bits);

static void handle_rx_data(sdla_t* card, TX_RX_DATA* rx_data);

/****** Public Functions ****************************************************/

/*============================================================================
 * Module initialization routine.
 *
 * This routine is called by the main WANPIPE module during setup.  At this
 * point adapter is completely initialized and CHDLC firmware is running.
 *  o read firmware version (to make sure it's alive)
 *  o configure adapter
 *  o initialize protocol-specific fields of the adapter data space.
 *
 * Return:	0	o.k.
 *		< 0	failure.
 */
int wpedu_init (sdla_t* card, wandev_conf_t* conf)
{
	unsigned char port_num;
#ifdef DEBUG	
	volatile wan_mbox_t* mb;
	wan_mbox_t* mb1;

	DEBUG_CFG("%s: %s\n", __FUNCTION__, card->devname);
#endif
	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_EDUKIT) {
		DEBUG_EVENT("%s: invalid configuration ID %u!\n",
					card->devname, conf->config_id);
		return -EINVAL;
	}

	/* runs only on S514, check it */
	if(card->type != SDLA_S514){
	       	DEBUG_EVENT("%s:Invalid card type! 0x%X\n",
					card->devname , card->type);
		return -EFAULT;
	}
	
	/* Use primary port */
	card->u.c.comm_port = 0;
	
#ifdef DEBUG	
	/* Initialize protocol-specific fields */
	card->mbox_off  = PRI_BASE_ADDR_MB_STRUCT;
	mb = mb1 = &card->wan_mbox;
#endif

	card->isr			= NULL;
	card->poll			= NULL;
	card->exec			= &wpedu_exec;
	card->wandev.update		= NULL;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->wandev.state		= WAN_DUALPORT;
	card->wandev.udp_port   	= conf->udp_port;

	card->wandev.new_if_cnt = 0;

	/* This is for the ports link state */
	card->u.c.state = WAN_DISCONNECTED;
	
	/* reset the number of times the 'update()' proc has been called */
	card->u.c.update_call_count = 0;
	
	DEBUG_CFG("conf->ttl: %d\n", conf->ttl);
	DEBUG_CFG("conf->mtu: %d\n", conf->mtu);

	if(conf->ttl == 0){
		card->wandev.ttl = 0x7F;
	}else{
		card->wandev.ttl = conf->ttl;
	}
	
	if(conf->mtu == 0){
		card->wandev.mtu = 1500;
	}else{
		card->wandev.mtu = conf->mtu;
	}
	card->wandev.electrical_interface = 0; 
	card->wandev.clocking = 0;

	port_num = card->u.c.comm_port;

	/* Setup Port Bps */
       	card->wandev.bps = 0;

	card->wandev.state = WAN_FT1_READY;
	DEBUG_EVENT("%s: Educational Kit Module Ready!\n",card->devname);

	return 0;
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
 * private structure for CHDLC protocol and bind it 
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
static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	sdla_t* card = wandev->priv;
	edu_private_area_t* edu_private_area;
	int err = 0;

	DEBUG_CFG("%s: Configuring Card: %s, Interface: %s\n",
			__FUNCTION__, card->devname, conf->name);
 
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		DEBUG_EVENT("%s: Invalid interface name!\n", card->devname);
		return -EINVAL;
	}
		
	/* allocate and initialize private data */
	edu_private_area = kmalloc(sizeof(edu_private_area_t), GFP_KERNEL);

	if(edu_private_area == NULL){ 
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}

	memset(edu_private_area, 0, sizeof(edu_private_area_t));

	edu_private_area->card = card; 
	strcpy(edu_private_area->if_name, conf->name);
	//edukit always has only one interface, re-use 'tty_buf'
	//to keep the pointer to the private area.
	card->tty_buf = (unsigned char*)edu_private_area;

	if (atomic_read(&card->wandev.if_cnt) > 0){
		err=-EEXIST;
		goto new_if_error;
	}

	/* Setup wanpipe as a router (WANPIPE) or as an API */
	if( strcmp(conf->usedby, "WANPIPE") == 0) {
		DEBUG_EVENT("%s: Running in WANPIPE mode!\n", wandev->name);
	}else if( strcmp(conf->usedby, "API") == 0) {
		DEBUG_EVENT("%s: Running in API mode!\n", wandev->name);
	}else{
		DEBUG_EVENT("%s:%s: Error: Invalid operation mode should be one of: [WANPIPE|API]\n",
				card->devname,edu_private_area->if_name);
		err=-EINVAL;
		goto new_if_error;
	}
	
	/*
	 * Create interface file in proc fs.
	 * Once the proc file system is created, the new_if() function
	 * should exit successfuly.  
	 *
	 * DO NOT place code under this function that can return 
	 * anything else but 0.
	 */
	err = wanrouter_proc_add_interface(wandev, 
					   &edu_private_area->dent, 
					   edu_private_area->if_name, 
					   dev);
	if (err){	
		DEBUG_EVENT("%s: can't create /proc/net/router/edu/%s entry!\n",
			card->devname, edu_private_area->if_name);
		goto new_if_error;
	}

	/* Only setup the dev pointer once the new_if function has
	 * finished successfully.  DO NOT place any code below that
	 * can return an error */

	dev->init = &if_init;
	dev->priv = edu_private_area;
	edu_private_area->net_dev = dev;//pointer back to network device

	atomic_inc(&card->wandev.if_cnt);

	DEBUG_EVENT("\n");

	return 0;
	
new_if_error:

	kfree(edu_private_area);

	dev->priv=NULL;

	return err;
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
 */
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	edu_private_area_t* 	edu_priv_area = dev->priv;
	sdla_t*			card = edu_priv_area->card;
	
	DEBUG_CFG("%s: deleting: %s, %s\n", __FUNCTION__,
			card->devname, dev->name);
	
	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, edu_priv_area->if_name);

	atomic_dec(&card->wandev.if_cnt);
	return 0;
}

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
static int if_init (netdevice_t* dev)
{
	edu_private_area_t* edu_private_area = dev->priv;
	sdla_t* card = edu_private_area->card;
	//wan_device_t* wandev = &card->wandev;

	DEBUG_CFG("%s: initializing %s, %s\n", __FUNCTION__,
			card->devname, dev->name);
	
	/* Initialize device driver entry points */
	dev->open		= &if_open;
	dev->stop		= &if_close;
	dev->hard_start_xmit	= &if_send;
	dev->get_stats		= &if_stats;
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	dev->tx_timeout		= &if_tx_timeout;
	dev->watchdog_timeo	= TX_TIMEOUT;
#endif
	dev->do_ioctl		= if_do_ioctl;

	/* Initialize media-specific parameters */
	dev->flags		|= IFF_POINTOPOINT;
	dev->flags		|= IFF_NOARP;

	/* Enable Mulitcasting if user selected */
	if (edu_private_area->mc == WANOPT_YES){
		dev->flags 	|= (IFF_MULTICAST|IFF_ALLMULTI);
	}
	
	if (edu_private_area->true_if_encoding){
		dev->type	= ARPHRD_HDLC; /* This breaks the tcpdump */
	}else{
		dev->type	= ARPHRD_PPP;
	}
		
	dev->mtu		= card->wandev.mtu;
	dev->hard_header_len	= 0;
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
 * chdlc_config() function. This function must be called
 * because the IP addresses could have been changed
 * for this interface.
 * 
 * Return 0 if O.k. or errno.
 */
static int if_open (netdevice_t* dev)
{
	edu_private_area_t* edu_priv_area = dev->priv;
	sdla_t* card = edu_priv_area->card;
	struct timeval tv;
	int err = 0;

	DEBUG_CFG("%s: opening %s, %s\n", __FUNCTION__,
			card->devname, dev->name);
	
	/* Only one open per interface is allowed */
	if (open_dev_check(dev)){
		DEBUG_CFG("%s: if_open(): open_dev_check() failed!\n", dev->name);
		return -EBUSY;
	}

	do_gettimeofday(&tv);
	edu_priv_area->router_start_time = tv.tv_sec;

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
 * Any private deallocation and cleanup can occur in del_if() 
 * function.  That function is called before the dev interface
 * itself is deallocated.
 *
 * Thus, we should only stop the net queue and decrement
 * the wanpipe usage counter via wanpipe_close() function.
 */

static int if_close (netdevice_t* dev)
{
	edu_private_area_t* edu_priv_area = dev->priv;
	sdla_t* card = edu_priv_area->card;

	DEBUG_CFG("%s: closing %s, %s\n", __FUNCTION__,
			card->devname, dev->name);
	
	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);

	return 0;
}


/**
 *	if_do_ioctl - Ioctl handler for fr
 *	@dev: Device subject to ioctl
 *	@ifr: Interface request block from the user
 *	@cmd: Command that is being issued
 *	
 *	This function handles the ioctls that may be issued by the user
 *	to control the settings of a FR. It does both busy
 *	and security checks. This function is intended to be wrapped by
 *	callers who wish to add additional ioctl calls of their own.
 */
/* SNMP */ 
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	int err=0;
	
	DEBUG_EVENT("%s: ioctl for: %s - Not implemented.\n",
			__FUNCTION__, dev->name);
	
	return err;
}

/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	sdla_t *my_card;
	edu_private_area_t* edu_priv_area;

	DEBUG_CFG("%s: stats for: %s\n", __FUNCTION__, dev->name);
	
	if ((edu_priv_area=dev->priv) == NULL){
		return NULL;
	}

	my_card = edu_priv_area->card;
	return &my_card->wandev.stats; 
}

/*============================================================================
 * if_send - Send a packet on a network interface.
 * 
 * @dev:	Network interface pointer
 * @skb:	Packet obtained from the stack or API
 *              that should be sent out the port.
 *              
 * o set tbusy flag (marks start of the transmission) to 
 *   block a timer-based transmit from overlapping.
 * o check link state. If link is not up, then drop the packet.
 * o execute adapter send command.
 * o free socket buffer if the send is successful, otherwise
 *   return non zero value and push the packet back into
 *   the stack. Also set the tx interrupt to wake up the
 *   stack when the firmware is able to send.
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
	edu_private_area_t *edu_priv_area = dev->priv;
	sdla_t *card = edu_priv_area->card;
	int err = 0;
	unsigned char misc_Tx_bits = 0;
	void* data = skb->data;
	unsigned len = skb->len;

	DEBUG_TX("%s: tx called for: %s: interface %s.\n", __FUNCTION__,
			card->devname, dev->name);

#if defined(LINUX_2_4)||defined(LINUX_2_6) 
	netif_stop_queue(dev);
#endif
	
	if (skb == NULL){
		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		DEBUG_EVENT("%s: interface %s got kicked!\n",
			card->devname, dev->name);

		wake_net_dev(dev);
		return 0;
	}

#if defined(LINUX_2_1)
	if (dev->tbusy){
		 ++card->wandev.stats.collisions;
		if((jiffies - edu_priv_area->tick_counter) < (5 * HZ)) {
			return 1;
		}

		if_tx_timeout (dev);
	}
#endif

    	if(test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
	
		DEBUG_EVENT("%s: Critical in if_send: %lx\n",
					card->wandev.name,card->wandev.critical);
                ++card->wandev.stats.tx_dropped;
		goto if_send_exit_crit;
	}

	err = edu_send(card, edu_priv_area, data, len, misc_Tx_bits);
		
	if(err) {
		err=-1;
	}else{
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		dev->trans_start = jiffies;
#endif
	}

if_send_exit_crit:	

	if (err==0){
		wan_skb_free(skb);
		start_net_queue(dev);
	}else{
		stop_net_queue(dev);
		edu_priv_area->tick_counter = jiffies;
	}

	clear_bit(SEND_CRIT, (void*)&card->wandev.critical);
	
	return err;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int edu_send (	sdla_t* card, edu_private_area_t *edu_priv_area,
	      		void* data, unsigned len, unsigned char tx_bits)
{
	unsigned char* ptr_data;

	ptr_data = (unsigned char*)data;

	DEBUG_TX("%s: called for: %s.\n", __FUNCTION__, card->devname);

	//check data length
	if(len >= MAX_TX_RX_DATA_SIZE){
		DEBUG_EVENT("%s: tx data length (%u) greater than maximum (%u)!\n",
			card->devname, len, MAX_TX_RX_DATA_SIZE);
		return 1;
	}
	
#if 0	
	//print out contents of the tx buffer passed from the stack
	{
		int i;

		for(i=0; i < len; i++){
			DEBUG_TX("data[%d]: 0x%02X\n", i, ptr_data[i]);
		}
		DEBUG_TX("\n");
	}
#endif

	if(edu_priv_area->tmp_tx_buff_state == 1){
		//the buffer was not freed yet by the "edukut" app.
		DEBUG_TX("%s: tx buff busy!!\n", card->devname);
		return 2;
	}
	//set to busy
	edu_priv_area->tmp_tx_buff_state = 1;
	//store tx data for "edukit" app
	memcpy(edu_priv_area->tmp_tx_data.data, data, len);
	//store tx data length
	edu_priv_area->tmp_tx_data.buffer_length = len;

	return 0;
}

/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	edu_private_area_t* chan = dev->priv;
	sdla_t *card = chan->card;

	DEBUG_TX("%s: called for: %s: interface %s.\n", __FUNCTION__,
			card->devname, dev->name);
	
	chan->tmp_tx_buff_state = 0;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	++card->wandev.stats.collisions;

	DEBUG_EVENT("%s: Transmit timed out on %s\n", card->devname,dev->name);
	netif_wake_queue (dev);
}

static int wpedu_exec(sdla_t *card, void *u_cmd, void *u_data)
{
	edu_private_area_t* edu_private_area = (edu_private_area_t*)card->tty_buf;
	edu_exec_cmd_t* ioctl_cmd = &edu_private_area->ioctl_cmd;
	unsigned char* data_buffer = edu_private_area->data_buffer;

	//copy command area
	if (copy_from_user((void*)ioctl_cmd, u_cmd, sizeof(edu_exec_cmd_t))){
		DEBUG_EVENT("copy_from_user((void*)&ioctl_cmd...) Failed!\n");
		return -EFAULT;
	}

	//copy data
	if (copy_from_user((void*)data_buffer, u_data, MAX_TX_RX_DATA_SIZE)){
		DEBUG_EVENT("wpedu_exec():copy_from_user((void*)data_buffer...) Failed!\n");
		return -EFAULT;
	}
	
	DoIoctl(card);

	// return result
	if( copy_to_user(u_cmd, (void*)ioctl_cmd, sizeof(edu_exec_cmd_t))){
		DEBUG_EVENT("wpedu_exec():copy_to_user(u_cmd, ...) Failed!\n");
		return -EFAULT;
	}

	if( copy_to_user(u_data, (void*)data_buffer, MAX_TX_RX_DATA_SIZE)){
		DEBUG_EVENT("wpedu_exec():copy_to_user(u_data, ...) Failed!\n");
		return -EFAULT;
	}

	return 0;
}

/*============================================================================
 * Receive data handler.
 */
static void handle_rx_data(sdla_t* card, TX_RX_DATA* rx_data)
{
	netdevice_t *dev;
	edu_private_area_t* edu_private_area;
	struct sk_buff *skb;
	void *buf;

	edu_private_area = (edu_private_area_t*)card->tty_buf;
	dev = edu_private_area->net_dev;

	/* Allocate socket buffer */
	skb = dev_alloc_skb(rx_data->buffer_length + 2);

	if (skb == NULL) {
		DEBUG_EVENT("%s: no socket buffers available!\n",
					card->devname);
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

	/* Align IP on 16 byte */
	skb_reserve(skb, 2);

	/* Copy data to the socket buffer */
	buf = skb_put(skb, rx_data->buffer_length);
	
	memcpy(buf, rx_data->data, rx_data->buffer_length);
	
	skb->protocol = htons(ETH_P_IP);

	/* Pass it up the protocol stack */
	card->wandev.stats.rx_packets ++;
	card->wandev.stats.rx_bytes += skb->len;

	skb->protocol = htons(ETH_P_IP);
        skb->dev = dev;
        wan_skb_reset_mac_header(skb);
        netif_rx(skb);
rx_exit:
	return;
}


static void DoIoctl(sdla_t *card)
{
	edu_private_area_t* edu_private_area = (edu_private_area_t*)card->tty_buf;
	edu_exec_cmd_t* ioctl_cmd = &edu_private_area->ioctl_cmd;
	TX_RX_DATA* tx_data = (TX_RX_DATA*)edu_private_area->data_buffer;
	TX_RX_DATA* rx_data = (TX_RX_DATA*)edu_private_area->data_buffer;
        unsigned short adapter_type;

	switch(ioctl_cmd->ioctl)
	{
	case SDLA_CMD_READ_BLOCK:
		DEBUG_IOCTL("SDLA_CMD_READ_BLOCK\n");
		
		read_block(card);
		break;
		
	case SDLA_CMD_WRITE_BLOCK:
		DEBUG_IOCTL("SDLA_CMD_WRITE_BLOCK\n");
		
		write_block(card);
		break;

	case SDLA_CMD_REGISTER:
		DEBUG_IOCTL("SDLA_CMD_REGISTER\n");

		//This field will be used as a flag for determining if
		//it is safe to (re)load the card.
		//The flag is set to one before an application will load the card.
		//No application will be able to reload the card until the flag is reset by
		//SDLA_CMD_DEREGISTER.
		if(card->wandev.electrical_interface == 0)
		{	card->wandev.electrical_interface = 1;
			ioctl_cmd->return_code = 0;			
		}else
		{	//indicate failure to the caller
			ioctl_cmd->return_code = 1;
		}
		break;

	case SDLA_CMD_DEREGISTER:
		DEBUG_IOCTL("SDLA_CMD_DEREGISTER\n");

		card->wandev.electrical_interface = 0;
		ioctl_cmd->return_code = 0;
		break;


	case SDLA_CMD_START_S514:
		DEBUG_IOCTL("SDLA_CMD_START_S514\n");

		//start the CPU directly
		card->hw_iface.start(card->hw, 0x100);
		// ALEX writeb (S514_CPU_START, card->hw.vector);
		break;

	case SDLA_CMD_STOP_S514:
		DEBUG_IOCTL("SDLA_CMD_STOP_S514\n");

		ioctl_cmd->return_code = card->hw_iface.hw_halt(card->hw);
		break;
	
	case SDLA_CMD_IS_TX_DATA_AVAILABLE:
		DEBUG_IOCTL("SDLA_CMD_IS_TX_DATA_AVAILABLE\n");

		if(edu_private_area->tmp_tx_buff_state == 1){
		
			tx_data->status = TX_SUCCESS;

			tx_data->buffer_length = edu_private_area->tmp_tx_data.buffer_length;
			memcpy(tx_data->data, edu_private_area->tmp_tx_data.data, 
				edu_private_area->tmp_tx_data.buffer_length);
		}
		break;

	case SDLA_CMD_COPLETE_TX_REQUEST:
		DEBUG_IOCTL("SDLA_CMD_COPLETE_TX_REQUEST\n");

		if(edu_private_area->tmp_tx_buff_state == 1){
			DEBUG_IOCTL("tx reseting internal state...\n");
			
			++card->wandev.stats.tx_packets;
                        card->wandev.stats.tx_bytes += edu_private_area->tmp_tx_data.buffer_length;
			edu_private_area->tmp_tx_buff_state = 0;
			netif_wake_queue (edu_private_area->net_dev);
		}
		break;

	case SDLA_CMD_INDICATE_RX_DATA_AVAILABLE:
		DEBUG_IOCTL("SDLA_CMD_INDICATE_RX_DATA_AVAILABLE\n");

		handle_rx_data(card, rx_data);
		break;

	case SDLA_CMD_GET_PCI_ADAPTER_TYPE:
		DEBUG_IOCTL("SDLA_CMD_GET_PCI_ADAPTER_TYPE\n");

		card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &adapter_type);

		DEBUG_IOCTL("adapter_type: 0x%X (%d)\n", adapter_type, adapter_type);

		if(adapter_type == S5147_ADPTR_2_CPU_T1E1){
			//for TE1 code to work adjust the card type
			adapter_type = S5144_ADPTR_1_CPU_T1E1;
		}

		*(unsigned short*)edu_private_area->data_buffer = adapter_type;
	
		DEBUG_IOCTL("Returning 0x%02X from SDLA_CMD_GET_PCI_ADAPTER_TYPE.\n", 
			*(unsigned short*)edu_private_area->data_buffer);
		break;

	default:
		DEBUG_EVENT("%s: ********* Unknown IOCTL : 0x%x *********\n", 
			card->devname, ioctl_cmd->ioctl);	
		ioctl_cmd->return_code = 1;
		break;
	}
}

static void read_block(sdla_t *card)
{
	edu_private_area_t* edu_private_area = (edu_private_area_t*)card->tty_buf;
	edu_exec_cmd_t* ioctl_cmd = &edu_private_area->ioctl_cmd;
	unsigned char* data_buffer = edu_private_area->data_buffer;

	if(ioctl_cmd->buffer_length <= MAX_TX_RX_DATA_SIZE){
		card->hw_iface.peek(card->hw, ioctl_cmd->offset, data_buffer, ioctl_cmd->buffer_length);
		ioctl_cmd->return_code = 0;
	}else{
		DEBUG_EVENT("%s: SDLA_CMD_READ_BLOCK: invalid data length of %u!\n",
			  card->devname, 
			  ioctl_cmd->buffer_length
			  );
		ioctl_cmd->return_code = 1;
	}
}

static void write_block(sdla_t * card)
{
	edu_private_area_t* edu_private_area = (edu_private_area_t*)card->tty_buf;
	edu_exec_cmd_t* ioctl_cmd = &edu_private_area->ioctl_cmd;
	unsigned char* data_buffer = edu_private_area->data_buffer;

	if( ioctl_cmd->buffer_length <= MAX_TX_RX_DATA_SIZE){
		card->hw_iface.poke(card->hw, ioctl_cmd->offset, data_buffer, ioctl_cmd->buffer_length);
		ioctl_cmd->return_code = 0;
	}else{
		DEBUG_EVENT("%s: SDLA_CMD_WRITE_BLOCK: invalid data length of %u!\n",
			card->devname, 
			ioctl_cmd->buffer_length
			);
		ioctl_cmd->return_code = 1;
	}
}


