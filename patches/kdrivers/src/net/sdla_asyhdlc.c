/*****************************************************************************
* sdla_asyhdlc.c	WANPIPE(tm) Multiprotocol WAN Link Driver. Cisco HDLC module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*		Gideon Hack  
*
* Copyright:	(c) 1995-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 03, 2002  Nenad Corbic	Memory leak bug fix under Bridge Mode.
* 				If not an ethernet frame skb buffer was
* 				not deallocated.
* Oct 18, 2002  Nenad Corbic	Added BRIDGE support
* Jan 14, 2002  Nenad Corbic	Removed the 2.0.X kernel support and added
*                               front end state handling.
* Dec 12, 2001  Nenad Corbic	Re-wrote the tty receive algorithm, using
*                               task queues, because ISA cards cannot call
*                               receive_buf() from the rx interrupt.
* Dec 03, 2001  Gideon Hack	Updated for S514-5 56K adapter.
* Sep 20, 2001  Nenad Corbic    The min() function has changed for 2.4.9
*                               kernel. Thus using the wp_min() defined in
*                               wanpipe.h
* Sept 6, 2001  Alex Feldman    Add SNMP support.
* Aug 23, 2001  Nenad Corbic    Removed the if_header and set the hard_header
* 			        length to zero. Caused problems with Checkpoint
* 			        firewall.
* May 13, 2001  Alex Feldman	Added T1/E1 support (TE1). 
* Feb 28, 2001  Nenad Corbic	Updated if_tx_timeout() routine for 
* 				2.4.X kernels.
* Jan 25, 2001  Nenad Corbic	Added a TTY Sync serial driver over the
* 				HDLC streaming protocol
* 				Added a TTY Async serial driver over the
* 				Async protocol.
* Dec 15, 2000  Nenad Corbic    Updated for 2.4.X Kernel support
* Nov 13, 2000  Nenad Corbic    Added true interface type encoding option.
* 				Tcpdump doesn't support CHDLC inteface
* 				types, to fix this "true type" option will set
* 				the interface type to RAW IP mode.
* Nov 07, 2000  Nenad Corbic	Added security features for UDP debugging:
*                               Deny all and specify allowed requests.
* Jun 20, 2000  Nenad Corbic	Fixed the API IP ERROR bug. Caused by the 
*                               latest update.
* May 09, 2000	Nenad Corbic	Option to bring down an interface
*                               upon disconnect.
* Mar 23, 2000  Nenad Corbic	Improved task queue, bh handling.
* Mar 16, 2000	Nenad Corbic	Fixed the SLARP Dynamic IP addressing.
* Mar 06, 2000  Nenad Corbic	Bug Fix: corrupted mbox recovery.
* Feb 10, 2000  Gideon Hack     Added ASYNC support.
* Feb 09, 2000  Nenad Corbic    Fixed two shutdown bugs in update() and
*                               if_stats() functions.
* Jan 24, 2000  Nenad Corbic    Fixed a startup wanpipe state racing,  
*                               condition between if_open and isr. 
* Jan 10, 2000  Nenad Corbic    Added new socket API support.
* Dev 15, 1999  Nenad Corbic    Fixed up header files for 2.0.X kernels
* Nov 20, 1999  Nenad Corbic 	Fixed zero length API bug.
* Sep 30, 1999  Nenad Corbic    Fixed dynamic IP and route setup.
* Sep 23, 1999  Nenad Corbic    Added SMP support, fixed tracing 
* Sep 13, 1999  Nenad Corbic	Split up Port 0 and 1 into separate devices.
* Jun 02, 1999  Gideon Hack     Added support for the S514 adapter.
* Oct 30, 1998	Jaspreet Singh	Added Support for CHDLC API (HDLC STREAMING).
* Oct 28, 1998	Jaspreet Singh	Added Support for Dual Port CHDLC.
* Aug 07, 1998	David Fong	Initial version.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/wanpipe_abstr.h>
#include <linux/wanproc.h>
#include <linux/sdla_asyhdlc.h>		/* CHDLC firmware API definitions */
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
#include <linux/if_wanpipe.h>		

/****** Defines & Macros ****************************************************/

/* Private critical flags */
enum { 
	POLL_CRIT = PRIV_CRIT, 
	TX_INTR,
	TASK_POLL,
	TTY_HANGUP 	
};


/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   		0x01
#define TMR_INT_ENABLED_UPDATE		0x02
#define TMR_INT_ENABLED_CONFIG		0x10
#define TMR_INT_ENABLED_TE		0x20

#define MAX_IP_ERRORS	10

#define TTY_CHDLC_MAX_MTU	2000
#define	CHDLC_DFLT_DATA_LEN	1500		/* default MTU */
#define CHDLC_HDR_LEN		1

#define CHDLC_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_BH_BUFF	10

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)


/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with CHDLC specific data
 */

typedef struct chdlc_private_area
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
        u32             IP_address;		/* IP addressing */
        u32             IP_netmask;
	u32		ip_local;
	u32		ip_remote;
	u32 		ip_local_tmp;
	u32		ip_remote_tmp;
	u8		ip_error;
	unsigned long	config_chdlc;
	u8 		config_chdlc_timeout;
	unsigned char  mc;			/* Mulitcast support on/off */
	unsigned char udp_pkt_src;		/* udp packet processing */
	unsigned short timer_int_enabled;
	unsigned long update_comms_stats;		/* updating comms stats */

	unsigned long interface_down;

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	struct tq_struct poll_task;
	struct timer_list poll_delay_timer;

	u8 gateway;
	u8 true_if_encoding;
	//FIXME: add driver stats as per frame relay!

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;

	unsigned char udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t udp_pkt_len;

	char if_name[WAN_IFNAME_SZ+1];
	
	netdevice_t *annexg_dev;
	unsigned char label[WAN_IF_LABEL_SZ+1];
	
} chdlc_private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/* variable for tracking how many interfaces to open for WANPIPE on the
   two ports */

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);
static int del_if(wan_device_t *wandev, netdevice_t *dev);

/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);


static struct net_device_stats* if_stats (netdevice_t* dev);
  
static int if_send (struct sk_buff* skb, netdevice_t* dev);

/* CHDLC Firmware interface functions */
static int chdlc_comm_enable 	(sdla_t* card);
static int chdlc_comm_disable 	(sdla_t* card);
static int chdlc_read_version 	(sdla_t* card, char* str, int str_size);
static int chdlc_set_intr_mode 	(sdla_t* card, unsigned mode);
static int set_adapter_config 	(sdla_t* card);
static int chdlc_send (sdla_t* card, void* data, unsigned len, unsigned char tx_bits);
static int chdlc_read_comm_err_stats (sdla_t* card);
static int chdlc_error (sdla_t *card, int err, wan_mbox_t *mb);


static int chdlc_disable_comm_shutdown (sdla_t *card);
static void if_tx_timeout (netdevice_t *dev);


/* Miscellaneous CHDLC Functions */
static void init_chdlc_tx_rx_buff( sdla_t* card);
static int process_global_exception(sdla_t *card);
static int update_comms_stats(sdla_t* card,
        chdlc_private_area_t* chdlc_priv_area);
static void port_set_state (sdla_t *card, int);
static int config_chdlc (sdla_t *card, netdevice_t *dev);
static void disable_comm (sdla_t *card);


/* Miscellaneous asynchronous interface Functions */
static int set_asy_config (sdla_t* card);
static int asy_comm_enable (sdla_t* card);

/* Interrupt handlers */
static WAN_IRQ_RETVAL wpc_isr (sdla_t* card);
static void rx_intr (sdla_t* card);
static void timer_intr(sdla_t *);

/* Bottom half handlers */
static void chdlc_bh (unsigned long data);

/* Miscellaneous functions */
static int chk_bcast_mcast_addr(sdla_t* card, netdevice_t* dev,
				struct sk_buff *skb);
static int reply_udp( unsigned char *data, unsigned int mbox_len );
static int intr_test( sdla_t* card);
static int udp_pkt_type( struct sk_buff *skb , sdla_t* card);
static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, netdevice_t* dev,
                                chdlc_private_area_t* chdlc_priv_area);
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,  
				chdlc_private_area_t* chdlc_priv_area,
				int local_dev);
static unsigned short calc_checksum (char *, int);
static void s508_lock (sdla_t *card, unsigned long *smp_flags);
static void s508_unlock (sdla_t *card, unsigned long *smp_flags);

#define CRC_LENGTH 2


static int chdlc_get_config_info(void* priv, struct seq_file* m, int*);
static int chdlc_get_status_info(void* priv, struct seq_file* m, int*);
static int chdlc_set_dev_config(struct file*, const char*, unsigned long, void *);
static int chdlc_set_if_info(struct file*, const char*, unsigned long, void *);

static void chdlc_enable_timer(void* card_id);
static void chdlc_handle_front_end_state(void* card_id);

/****** Public Functions ****************************************************/

/*============================================================================
 * wp_asyhdlc_init - Cisco HDLC protocol initialization routine.
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
int wp_asyhdlc_init (sdla_t* card, wandev_conf_t* conf)
{
	unsigned char port_num;
	int err;
	unsigned long max_permitted_baud = 0;

	char str[80];
	volatile wan_mbox_t* mb;
	wan_mbox_t* mb1;
	unsigned long timeout;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_ASYHDLC) {
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}
	/* Find out which Port to use */
	if ((conf->comm_port == WANOPT_PRI) || (conf->comm_port == WANOPT_SEC)){
		if (card->next){

			if (conf->comm_port != card->next->u.c.comm_port){
				card->u.c.comm_port = conf->comm_port;
			}else{
				printk(KERN_INFO "%s: ERROR - %s port used!\n",
        		        	card->wandev.name, PORT(conf->comm_port));
				return -EINVAL;
			}
		}else{
			card->u.c.comm_port = conf->comm_port;
		}
	}else{
		printk(KERN_INFO "%s: ERROR - Invalid Port Selected!\n",
                			card->wandev.name);
		return -EINVAL;
	}
	

	/* Initialize protocol-specific fields */
	/* ALEX: Apr 8 2004 SANGOMA ISA CARD */
	/* for a PCI/ISA adapters, set a pointer to the actual mailbox in the */
	/* allocated virtual memory area */
	if (card->u.c.comm_port == WANOPT_PRI){
		card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;
	}else{
		card->mbox_off = SEC_BASE_ADDR_MB_STRUCT;
	}

	mb = &card->wan_mbox;
	mb1 = &card->wan_mbox;

	if (!card->configured){
		unsigned char return_code = 0x00;
		/* The board will place an 'I' in the return code to indicate that it is
	   	ready to accept commands.  We expect this to be completed in less
           	than 1 second. */

		timeout = jiffies;
		do {
			return_code = 0x00;
			card->hw_iface.peek(card->hw, 
				card->mbox_off+offsetof(wan_mbox_t, wan_return_code),
				&return_code, 
				sizeof(unsigned char));
			if ((jiffies - timeout) > 1*HZ) break;
		}while(return_code != 'I');
		if (return_code != 'I') {
			printk(KERN_INFO
				"%s: Initialization not completed by adapter\n",
				card->devname);
			printk(KERN_INFO "Please contact Sangoma representative.\n");
			return -EIO;
		}
	}

	card->wandev.clocking = conf->clocking;
	card->wandev.ignore_front_end_status = conf->ignore_front_end_status;

	card->wandev.line_idle = conf->line_idle;
	card->wandev.line_coding = conf->line_coding;
	card->wandev.connection = conf->connection;

	err = (card->hw_iface.check_mismatch) ? 
			card->hw_iface.check_mismatch(card->hw,conf->fe_cfg.media) : -EINVAL;
	if (err){
		return err;
	}
	
	/* TE1 Make special hardware initialization for T1/E1 board */
	if (IS_TE1_MEDIA(&conf->fe_cfg)){
		
		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
		card->wandev.fe_enable_timer = chdlc_enable_timer;
		card->wandev.te_link_state = chdlc_handle_front_end_state;
		conf->electrical_interface = 
			(IS_T1_CARD(card)) ? WANOPT_V35 : WANOPT_RS232;

		if (card->u.c.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else if (IS_56K_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe,&card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
		
		if (card->u.c.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else{
		card->fe.fe_status = FE_CONNECTED;
	}

	if (card->wandev.ignore_front_end_status == WANOPT_NO){
		printk(KERN_INFO 
		  "%s: Enabling front end link monitor\n",
				card->devname);
	}else{
		printk(KERN_INFO 
		"%s: Disabling front end link monitor\n",
				card->devname);
	}


	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (chdlc_read_version(card, str, sizeof(str)))
		return -EIO;

	printk(KERN_INFO "%s: Running Cisco HDLC firmware v%s\n",
		card->devname, str); 

	if (set_adapter_config(card)) {
		return -EIO;
	}

	
	card->isr			= &wpc_isr;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;

	card->wandev.udp_port   	= conf->udp_port;
	card->wandev.new_if_cnt = 0;
	atomic_set(&card->wandev.if_cnt,0);

	// Proc fs functions
	card->wandev.get_config_info 	= &chdlc_get_config_info;
	card->wandev.get_status_info 	= &chdlc_get_status_info;
	card->wandev.set_dev_config    	= &chdlc_set_dev_config;
	card->wandev.set_if_info     	= &chdlc_set_if_info;

	/* reset the number of times the 'update()' proc has been called */
	card->u.c.update_call_count = 0;
	
	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 

	if ((card->u.c.comm_port == WANOPT_SEC && conf->electrical_interface == WANOPT_V35)&&
	    card->type != SDLA_S514){
		printk(KERN_INFO "%s: ERROR - V35 Interface not supported on S508 %s port \n",
			card->devname, PORT(card->u.c.comm_port));
		return -EIO;
	}


	port_num = card->u.c.comm_port;

	/* in API mode, we can configure for "receive only" buffering */
	if(card->type == SDLA_S514) {
		card->u.c.receive_only = conf->receive_only;
		if(conf->receive_only) {
			printk(KERN_INFO
				"%s: Configured for 'receive only' mode\n",
                                card->devname);
		}
	}

	/* Setup Port Bps */

	if(card->wandev.clocking) {
               	max_permitted_baud = MAX_ASY_BAUD_RATE;

		if(conf->bps > max_permitted_baud) {
			conf->bps = max_permitted_baud;
			printk(KERN_INFO "%s: Baud too high!\n",
				card->wandev.name);
 			printk(KERN_INFO "%s: Baud rate set to %lu bps\n", 
				card->wandev.name, max_permitted_baud);
		}
		card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
  	}

	card->wandev.mtu = conf->mtu;
	

	/* Set up the interrupt status area */
	/* Read the CHDLC Configuration and obtain: 
	 *	Ptr to shared memory infor struct
         * Use this pointer to calculate the value of card->u.c.flags !
 	 */
	mb1->wan_data_len = 0;
	mb1->wan_command = READ_ASY_CONFIGURATION;

	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb1);
	if(err != COMMAND_OK) {

		DEBUG_EVENT("%s: CHDLC READ CONFIG cmd rc=0x%X\n",
				card->devname,err);

                if(card->type != SDLA_S514)
                	enable_irq(card->wandev.irq);

		chdlc_error(card, err, mb1);

		return -EIO;
	}

	/* ALEX: Apr 8 2004 SANGOMA ISA CARD */
	card->flags_off = 
		((ASY_CONFIGURATION_STRUCT *)mb1->wan_data)->
			ptr_shared_mem_info_struct;

    	card->intr_type_off = 
		card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, interrupt_info_struct) +
		offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_type);
	card->intr_perm_off = 
		card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, interrupt_info_struct) +
		offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_permission);
	card->fe_status_off = 
		card->flags_off + 
		offsetof(SHARED_MEMORY_INFO_STRUCT, FT1_info_struct) + 
		offsetof(FT1_INFORMATION_STRUCT, parallel_port_A_input);

	/* This is for the ports link state */
	card->wandev.state = WAN_DUALPORT;


	if (!card->wandev.piggyback){	
		int err;

		/* Perform interrupt testing */
		err = intr_test(card);

		if(err || (card->timer_int_enabled < 1)) { 
			printk(KERN_INFO "%s: Interrupt test failed (%i)\n",
					card->devname, card->timer_int_enabled);
			printk(KERN_INFO "%s: Please choose another interrupt\n",
					card->devname);
			return -EIO;
		}
		printk(KERN_INFO "%s: Interrupt test passed (%i)\n", 
				card->devname, card->timer_int_enabled);
		card->configured = 1;
	}

	if (chdlc_set_intr_mode(card, APP_INT_ON_TIMER)){
		printk (KERN_INFO "%s: Failed to set interrupt triggers!\n",
			card->devname);
		return -EIO;	
       	}
	
	/* Mask the Timer interrupt */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);

	/* If we are using CHDLC in backup mode, this flag will
	 * indicate not to look for IP addresses in config_chdlc()*/
	card->u.c.backup = conf->backup;
	card->disable_comm = &disable_comm;

	printk(KERN_INFO "\n");

	return 0;
}

/******* WAN Device Driver Entry Points *************************************/

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
 *      3) CHDLC operational statistics on the adapter.
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
	netdevice_t	*dev;
        chdlc_private_area_t* chdlc_priv_area;
	unsigned long smp_flags;
#if 0
        SHARED_MEMORY_INFO_STRUCT *flags;
	unsigned long timeout;	
#endif

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	/* more sanity checks */
	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical))
                return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL)
		return -ENODEV;

	if((chdlc_priv_area = wan_netif_priv(dev)) == NULL)
		return -ENODEV;

#if 0
      	flags = card->u.c.flags;
#endif
       
	if(test_and_set_bit(0,&chdlc_priv_area->update_comms_stats)){
		return -EAGAIN;
	}
			
	/* TE1	Change the update_comms_stats variable to 3,
	 * 	only for T1/E1 card, otherwise 2 for regular
	 *	S514/S508 card.
	 *	Each timer interrupt will update only one type
	 *	of statistics.
	 */

	
#if 0
	chdlc_priv_area->update_comms_stats = 
		(IS_TE1_CARD(card) || IS_56K_CARD(card)) ? 3 : 2;
       	flags->interrupt_info_struct.interrupt_permission |= APP_INT_ON_TIMER;
	card->u.c.timer_int_enabled = TMR_INT_ENABLED_UPDATE;
  
	/* wait a maximum of 1 second for the statistics to be updated */ 
        timeout = jiffies;
        for(;;) {
		if(chdlc_priv_area->update_comms_stats == 0)
			break;
                if ((jiffies - timeout) > (1 * HZ)){
    			chdlc_priv_area->update_comms_stats = 0;
 			card->u.c.timer_int_enabled &=
				~TMR_INT_ENABLED_UPDATE; 
 			return -EAGAIN;
		}
        }
#else
	spin_lock_irqsave(&card->wandev.lock, smp_flags);
	update_comms_stats(card, chdlc_priv_area);	
	chdlc_priv_area->update_comms_stats=0;
	spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
#endif
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
 * to wan_netif_priv(dev) pointer.  
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
	chdlc_private_area_t* chdlc_priv_area;
	int err = 0;

	printk(KERN_INFO "%s: Configuring Interface: %s\n",
			card->devname, conf->name);
 
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		printk(KERN_INFO "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}
		
	/* allocate and initialize private data */
	chdlc_priv_area = kmalloc(sizeof(chdlc_private_area_t), GFP_KERNEL);
	
	if(chdlc_priv_area == NULL){ 
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}

	memset(chdlc_priv_area, 0, sizeof(chdlc_private_area_t));

	chdlc_priv_area->card = card; 
	strcpy(chdlc_priv_area->if_name, conf->name);

	WAN_TASKLET_INIT((&chdlc_priv_area->common.bh_task),0,chdlc_bh,(unsigned long)chdlc_priv_area);
	if (atomic_read(&card->wandev.if_cnt) > 0){
		err=-EEXIST;
		goto new_if_error;
	}

	chdlc_priv_area->TracingEnabled = 0;
	chdlc_priv_area->route_status = NO_ROUTE;
	chdlc_priv_area->route_removed = 0;

	card->u.c.async_mode = 1; //conf->async_mode;

	/* setup for asynchronous mode */
	if(card->u.c.async_mode) {
		printk(KERN_INFO "%s: Configuring for asynchronous mode\n",
			wandev->name);

	       	if(strcmp(conf->usedby, "WANPIPE") == 0) {
			printk(KERN_INFO "%s: Running in WANIPE Async Mode\n",
						wandev->name);
			chdlc_priv_area->common.usedby = WANPIPE;
		}else{
			printk(KERN_INFO "%s: Running in API Async Mode\n",
						wandev->name);
			chdlc_priv_area->common.usedby = API;
			wan_reg_api(chdlc_priv_area, dev, card->devname);
		}

		if(!card->wandev.clocking) {
			printk(KERN_INFO
				"%s: Asynch. clocking must be 'Internal'\n",
				wandev->name);

			err=-EINVAL;
			goto new_if_error;
		}

		if((card->wandev.bps < MIN_ASY_BAUD_RATE) ||
			(card->wandev.bps > MAX_ASY_BAUD_RATE)) {
			printk(KERN_INFO "%s: Selected baud rate is invalid.\n",
				wandev->name);
			printk(KERN_INFO "Must be between %u and %u bps.\n",
				MIN_ASY_BAUD_RATE, MAX_ASY_BAUD_RATE);

			err=-EINVAL;
			goto new_if_error;
		}

		card->u.c.api_options = 0;
		
		card->u.c.protocol_options = conf->u.chdlc.protocol_options;

		DEBUG_EVENT("%s: Asy Prtocol Options: 0x%04X\n",
				card->devname,card->u.c.protocol_options);
		
		card->u.c.tx_bits_per_char = conf->tx_bits_per_char;
                card->u.c.rx_bits_per_char = conf->rx_bits_per_char;
                card->u.c.stop_bits = conf->stop_bits;
		card->u.c.parity = conf->parity;
		card->u.c.break_timer = conf->break_timer;
		card->u.c.inter_char_timer = conf->inter_char_timer;
		card->u.c.rx_complete_length = conf->rx_complete_length;
		card->u.c.xon_char = conf->xon_char;

	} 

	/* Tells us that if this interface is a
         * gateway or not */
	if ((chdlc_priv_area->gateway = conf->gateway) == WANOPT_YES){
		printk(KERN_INFO "%s: Interface %s is set as a gateway.\n",
			card->devname,chdlc_priv_area->if_name);
	}

	/* Get Multicast Information */
	chdlc_priv_area->mc = conf->mc;

	/*
	 * Create interface file in proc fs.
	 * Once the proc file system is created, the new_if() function
	 * should exit successfuly.  
	 *
	 * DO NOT place code under this function that can return 
	 * anything else but 0.
	 */
	err = wanrouter_proc_add_interface(wandev, 
					   &chdlc_priv_area->dent, 
					   chdlc_priv_area->if_name, 
					   dev);
	if (err){	
		printk(KERN_INFO
			"%s: can't create /proc/net/router/fr/%s entry!\n",
			card->devname, chdlc_priv_area->if_name);
		goto new_if_error;
	}

	/* Only setup the dev pointer once the new_if function has
	 * finished successfully.  DO NOT place any code below that
	 * can return an error */

	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,if_do_ioctl);

	wan_netif_set_priv(dev,chdlc_priv_area);

	set_bit(0,&chdlc_priv_area->config_chdlc);

	atomic_inc(&card->wandev.if_cnt);

	printk(KERN_INFO "\n");

	return 0;
	
new_if_error:

	WAN_TASKLET_KILL(&chdlc_priv_area->common.bh_task);
	wan_unreg_api(chdlc_priv_area, card->devname);
	
	kfree(chdlc_priv_area);

	wan_netif_set_priv(dev,NULL);

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
	chdlc_private_area_t* 	chdlc_priv_area = wan_netif_priv(dev);
	sdla_t*			card = chdlc_priv_area->card;

	WAN_TASKLET_KILL(&chdlc_priv_area->common.bh_task);
	wan_unreg_api(chdlc_priv_area, card->devname);
	
	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, chdlc_priv_area->if_name);

	atomic_dec(&card->wandev.if_cnt);
	return 0;
}

/****** Network Device Interface ********************************************/

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
	chdlc_private_area_t* chdlc_priv_area = wan_netif_priv(dev);
	sdla_t* card = chdlc_priv_area->card;
	wan_device_t* wandev = &card->wandev;

	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	dev->watchdog_timeo	= TX_TIMEOUT;
#endif
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,if_do_ioctl);

	if (chdlc_priv_area->common.usedby == BRIDGE || 
            chdlc_priv_area->common.usedby == BRIDGE_NODE){

		/* Setup the interface for Bridging */
		int hw_addr=0;
		ether_setup(dev);
		
		/* Use a random number to generate the MAC address */
		memcpy(dev->dev_addr, "\xFE\xFC\x00\x00\x00\x00", 6);
		get_random_bytes(&hw_addr, sizeof(hw_addr));
		*(int *)(dev->dev_addr + 2) += hw_addr;

	}else{
		/* Initialize media-specific parameters */
		dev->flags		|= IFF_POINTOPOINT;
		dev->flags		|= IFF_NOARP;

		/* Enable Mulitcasting if user selected */
		if (chdlc_priv_area->mc == WANOPT_YES){
			dev->flags 	|= (IFF_MULTICAST|IFF_ALLMULTI);
		}
		
		if (chdlc_priv_area->true_if_encoding){
			dev->type	= ARPHRD_HDLC; /* This breaks the tcpdump */
		}else{
			dev->type	= ARPHRD_PPP;
		}
		
		dev->mtu		= card->wandev.mtu;
		/* for API usage, add the API header size to the requested MTU size */
		if(chdlc_priv_area->common.usedby == API) {
			dev->mtu += sizeof(api_tx_hdr_t);
		}
	 
		dev->hard_header_len	= 0;
	}
	
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
 * chdlc_config() function. This function must be called
 * because the IP addresses could have been changed
 * for this interface.
 * 
 * Return 0 if O.k. or errno.
 */
static int if_open (netdevice_t* dev)
{
	chdlc_private_area_t* chdlc_priv_area = wan_netif_priv(dev);
	sdla_t* card = chdlc_priv_area->card;
	struct timeval tv;
	int err = 0;

	/* Only one open per interface is allowed */
	if (open_dev_check(dev))
		return -EBUSY;

	do_gettimeofday(&tv);
	chdlc_priv_area->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

	/* Increment the module usage count */
	wanpipe_open(card);

	chdlc_priv_area->config_chdlc_timeout=jiffies;

	if (wan_test_bit(0,&chdlc_priv_area->config_chdlc)){
		SHARED_MEMORY_INFO_STRUCT flags;
		card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
		
		wan_clear_bit(0,&chdlc_priv_area->config_chdlc);
		card->u.c.timer_int_enabled |= TMR_INT_ENABLED_CONFIG;
		flags.interrupt_info_struct.interrupt_permission |= APP_INT_ON_TIMER;
		card->hw_iface.poke(card->hw, card->flags_off, &flags, sizeof(flags));
	}
	
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
	chdlc_private_area_t* chdlc_priv_area = wan_netif_priv(dev);
	sdla_t* card = chdlc_priv_area->card;

	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);
	del_timer(&chdlc_priv_area->poll_delay_timer);

	if (chdlc_priv_area->common.usedby == API){	
		unsigned long smp_flags;
		spin_lock_irqsave(&card->wandev.lock,smp_flags);
		wan_unbind_api_from_svc(chdlc_priv_area,
				chdlc_priv_area->common.sk);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
	}
	
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
 * o Deallocate memory used
 * o Unconfigure TE1 card
 */ 

static void disable_comm (sdla_t *card)
{
	unsigned long smp_flags;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (card->u.c.comm_enabled){
		chdlc_disable_comm_shutdown (card);
	}else{
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
	}

	return;
}



/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	chdlc_private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	
	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	++card->wandev.stats.collisions;

	printk (KERN_INFO "%s: Transmit timed out on %s\n", card->devname,dev->name);
	netif_wake_queue (dev);
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
	chdlc_private_area_t *chdlc_priv_area = wan_netif_priv(dev);
	sdla_t *card = chdlc_priv_area->card;
	int udp_type = 0;
	unsigned long smp_flags;
	int err=0;
	unsigned char misc_Tx_bits = 0;

	smp_flags=0;

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
		 ++card->wandev.stats.collisions;
		if((jiffies - chdlc_priv_area->tick_counter) < (5 * HZ)) {
			return 1;
		}

		if_tx_timeout (dev);
	}
#endif

   	if (chdlc_priv_area->common.usedby != ANNEXG && 
	    skb->protocol != htons(PVC_PROT)){

		/* check the udp packet type */
		
		udp_type = udp_pkt_type(skb, card);

		if (udp_type == UDP_CPIPE_TYPE){
                        if(store_udp_mgmt_pkt(UDP_PKT_FRM_STACK, card, skb, dev,
                                chdlc_priv_area)){
				card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
			}
			start_net_queue(dev);
			return 0;
		}

		/* check to see if the source IP address is a broadcast or */
		/* multicast IP address */
                if(chdlc_priv_area->common.usedby == WANPIPE && chk_bcast_mcast_addr(card, dev, skb)){
			++card->wandev.stats.tx_dropped;
			wan_skb_free(skb);
			start_net_queue(dev);
			return 0;
		}
        }

	/* Lock the 508 Card: SMP is supported */
      	if(card->type != SDLA_S514){
		s508_lock(card,&smp_flags);
	} 

    	if(test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
	
		printk(KERN_INFO "%s: Critical in if_send: %lx\n",
					card->wandev.name,card->wandev.critical);
                ++card->wandev.stats.tx_dropped;
		goto if_send_exit_crit;
	}

	if(card->wandev.state != WAN_CONNECTED){
       		++card->wandev.stats.tx_dropped;

	}else if(!skb->protocol){
        	++card->wandev.stats.tx_errors;
		
	}else {
		void* data = skb->data;
		unsigned len = skb->len;
		unsigned char attr;

		/* If it's an API packet pull off the API
		 * header. Also check that the packet size
		 * is larger than the API header
	         */
		if (chdlc_priv_area->common.usedby == API){
			api_tx_hdr_t* api_tx_hdr;

			/* discard the frame if we are configured for */
			/* 'receive only' mode or if there is no data */
			if (card->u.c.receive_only ||
				(len <= sizeof(api_tx_hdr_t))) {
				
				++card->wandev.stats.tx_dropped;
				goto if_send_exit_crit;
			}
				
			api_tx_hdr = (api_tx_hdr_t *)data;
			attr = api_tx_hdr->wp_api_tx_hdr_chdlc_attr;
			misc_Tx_bits = api_tx_hdr->wp_api_tx_hdr_chdlc_misc_tx_bits;
			data += sizeof(api_tx_hdr_t);
			len -= sizeof(api_tx_hdr_t);
		}

		err=chdlc_send(card, data, len, misc_Tx_bits);
		
		if(err) {
			err=-1;
		}else{
			++card->wandev.stats.tx_packets;
                        card->wandev.stats.tx_bytes += len;
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		 	dev->trans_start = jiffies;
#endif
		}	
	}

if_send_exit_crit:	

	if (err==0){
		wan_skb_free(skb);
		start_net_queue(dev);
	}else{
		stop_net_queue(dev);
		chdlc_priv_area->tick_counter = jiffies;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX);
	}

	clear_bit(SEND_CRIT, (void*)&card->wandev.critical);
	if(card->type != SDLA_S514){
		s508_unlock(card,&smp_flags);
	}
	
	return err;
}


/*============================================================================
 * chk_bcast_mcast_addr - Check for source broadcast addresses
 *
 * Check to see if the packet to be transmitted contains a broadcast or
 * multicast source IP address.
 */

static int chk_bcast_mcast_addr(sdla_t *card, netdevice_t* dev,
				struct sk_buff *skb)
{
	u32 src_ip_addr;
        u32 broadcast_ip_addr = 0;
	chdlc_private_area_t *chdlc_priv_area=wan_netif_priv(dev);
        struct in_device *in_dev;
        /* read the IP source address from the outgoing packet */
        src_ip_addr = *(u32 *)(skb->data + 12);

	if (chdlc_priv_area->common.usedby != WANPIPE){
		return 0;
	}
	
	/* read the IP broadcast address for the device */
        in_dev = dev->ip_ptr;
        if(in_dev != NULL) {
                struct in_ifaddr *ifa= in_dev->ifa_list;
                if(ifa != NULL)
                        broadcast_ip_addr = ifa->ifa_broadcast;
                else
                        return 0;
        }
 
        /* check if the IP Source Address is a Broadcast address */
        if((dev->flags & IFF_BROADCAST) && (src_ip_addr == broadcast_ip_addr)) {
                printk(KERN_INFO "%s: Broadcast Source Address silently discarded\n",
				card->devname);
                return 1;
        } 

        /* check if the IP Source Address is a Multicast address */
        if((ntohl(src_ip_addr) >= 0xE0000001) &&
		(ntohl(src_ip_addr) <= 0xFFFFFFFE)) {
                printk(KERN_INFO "%s: Multicast Source Address silently discarded\n",
				card->devname);
                return 1;
        }

        return 0;
}


/*============================================================================
 * Reply to UDP Management system.
 * Return length of reply.
 */
static int reply_udp( unsigned char *data, unsigned int mbox_len )
{

	unsigned short len, udp_length, temp, ip_length;
	unsigned long ip_temp;
	int even_bound = 0;
  	wan_udp_pkt_t *c_udp_pkt = (wan_udp_pkt_t *)data;
	 
	/* Set length of packet */
	len = sizeof(struct iphdr)+ 
	      sizeof(struct udphdr)+
	      sizeof(wan_mgmt_t)+
	      sizeof(wan_cmd_t)+
	      sizeof(wan_trace_info_t)+
	      mbox_len;

	/* fill in UDP reply */
	c_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
   
	/* fill in UDP length */
	udp_length = sizeof(struct udphdr)+ 
		     sizeof(wan_mgmt_t)+
	      	     sizeof(wan_cmd_t)+
	             sizeof(wan_trace_info_t)+
		     mbox_len; 

 	/* put it on an even boundary */
	if ( udp_length & 0x0001 ) {
		udp_length += 1;
		len += 1;
		even_bound = 1;
	}  

	temp = (udp_length<<8)|(udp_length>>8);
	c_udp_pkt->wan_udp_len = temp;
		 
	/* swap UDP ports */
	temp = c_udp_pkt->wan_udp_sport;
	c_udp_pkt->wan_udp_sport = 
			c_udp_pkt->wan_udp_dport; 
	c_udp_pkt->wan_udp_dport = temp;

	/* add UDP pseudo header */
	temp = 0x1100;
	*((unsigned short *)(c_udp_pkt->wan_udp_data+mbox_len+even_bound)) = temp;	
	temp = (udp_length<<8)|(udp_length>>8);
	*((unsigned short *)(c_udp_pkt->wan_udp_data+mbox_len+even_bound+2)) = temp;

		 
	/* calculate UDP checksum */
	c_udp_pkt->wan_udp_sum = 0;
	c_udp_pkt->wan_udp_sum = calc_checksum(&data[UDP_OFFSET],udp_length+UDP_OFFSET);

	/* fill in IP length */
	ip_length = len;
	temp = (ip_length<<8)|(ip_length>>8);
	c_udp_pkt->wan_ip_len = temp;
  
	/* swap IP addresses */
	ip_temp = c_udp_pkt->wan_ip_src;
	c_udp_pkt->wan_ip_src = c_udp_pkt->wan_ip_dst;
	c_udp_pkt->wan_ip_dst = ip_temp;

	/* fill in IP checksum */
	c_udp_pkt->wan_ip_sum = 0;
	c_udp_pkt->wan_ip_sum = calc_checksum(data,sizeof(struct iphdr));

	return len;

} /* reply_udp */

unsigned short calc_checksum (char *data, int len)
{
	unsigned short temp; 
	unsigned long sum=0;
	int i;

	for( i = 0; i <len; i+=2 ) {
		memcpy(&temp,&data[i],2);
		sum += (unsigned long)temp;
	}

	while (sum >> 16 ) {
		sum = (sum & 0xffffUL) + (sum >> 16);
	}

	temp = (unsigned short)sum;
	temp = ~temp;

	if( temp == 0 ) 
		temp = 0xffff;

	return temp;	
}


/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	sdla_t *my_card;
	chdlc_private_area_t* chdlc_priv_area;

	if ((chdlc_priv_area=wan_netif_priv(dev)) == NULL)
		return NULL;

	my_card = chdlc_priv_area->card;
	return &my_card->wandev.stats; 
}

/****** Cisco HDLC Firmware Interface Functions *******************************/

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str. 
 */
static int chdlc_read_version (sdla_t* card, char* str, int str_size)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int len;
	char err;
	
	mb->wan_data_len = 0;
	mb->wan_command = READ_ASY_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		chdlc_error(card,err,mb);
	}else if (str) {  /* is not null */
		len = mb->wan_data_len;
		if (len < str_size){
			memcpy(str, mb->wan_data, len);
			str[len] = '\0';
		}else{
			DEBUG_EVENT("%s: Error: Version Length greater than max %i>%i!\n",
					card->devname,len,str_size);
		}
	}
	return (err);
}

/*============================================================================
 * Set interrupt mode -- HDLC Version.
 */

static int chdlc_set_intr_mode (sdla_t* card, unsigned mode)
{
	wan_mbox_t* mb = &card->wan_mbox;
	ASY_INT_TRIGGERS_STRUCT* int_data =
		 (ASY_INT_TRIGGERS_STRUCT *)mb->wan_data;
	int err;

	int_data->interrupt_triggers 	= mode;
	int_data->IRQ				= card->wandev.irq;
	int_data->interrupt_timer               = 1;

	mb->wan_data_len = sizeof(ASY_INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_ASY_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error (card, err, mb);
	return err;
}


/*===========================================================
 * chdlc_disable_comm_shutdown
 *
 * Shutdown() disables the communications. We must
 * have a sparate functions, because we must not
 * call chdlc_error() hander since the private
 * area has already been replaced */

static int chdlc_disable_comm_shutdown (sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	ASY_INT_TRIGGERS_STRUCT* int_data =
		 (ASY_INT_TRIGGERS_STRUCT *)mb->wan_data;

	/* Disable Interrutps */
	int_data->interrupt_triggers 	= 0;
	int_data->IRQ				= card->wandev.irq;	
	int_data->interrupt_timer               = 1;
   
	mb->wan_data_len = sizeof(ASY_INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_ASY_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	/* Disable Communications */
	if (card->u.c.async_mode) {
		mb->wan_command = DISABLE_ASY_COMMUNICATIONS;
	}else{
		mb->wan_command = DISABLE_ASY_COMMUNICATIONS;
	}
	
	mb->wan_data_len = 0;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	card->u.c.comm_enabled = 0;

	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
	}

	return 0;
}

/*============================================================================
 * Enable communications.
 */

static int chdlc_comm_enable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_ASY_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error(card, err, mb);
	else
		card->u.c.comm_enabled = 1;
	
	return err;
}

/*============================================================================
 * Enable communications.
 */

static int chdlc_comm_disable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = DISABLE_ASY_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error(card, err, mb);
	else
		card->u.c.comm_enabled = 0;
	
	return err;
}


/*============================================================================
 * Read communication error statistics.
 */
static int chdlc_read_comm_err_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_COMMS_ERROR_STATS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                chdlc_error(card,err,mb);
        return err;
}



/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card,
	chdlc_private_area_t* chdlc_priv_area)
{
        wan_mbox_t* mb = &card->wan_mbox;
  	COMMS_ERROR_STATS_STRUCT* err_stats;

	/* 1. On the first timer interrupt, update T1/E1 alarms 
	 * and PMON counters (only for T1/E1 card) (TE1) 
	 */
	/* TE1 Update T1/E1 alarms */
	if (IS_TE1_CARD(card)) {	
		card->wandev.fe_iface.read_alarm(&card->fe, 0); 
		/* TE1 Update T1/E1 perfomance counters */
		card->wandev.fe_iface.read_pmon(&card->fe,0); 
	}else if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1); 
	}

	if(chdlc_read_comm_err_stats(card))
		return 1;
	
	err_stats = (COMMS_ERROR_STATS_STRUCT *)mb->wan_data;
	card->wandev.stats.rx_over_errors = 
		err_stats->Rx_overrun_err_count;
	card->wandev.stats.rx_crc_errors = 
		err_stats->Rx_parity_err_count;
	card->wandev.stats.rx_frame_errors = 
		err_stats->Rx_framing_err_count;
       	
	return 0;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int chdlc_send (sdla_t* card, void* data, unsigned len, unsigned char tx_bits)
{
	ASY_DATA_TX_STATUS_EL_STRUCT txbuf;
	
	card->hw_iface.peek(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));
	if (txbuf.opp_flag){
		return 1;
	}
	
	card->hw_iface.poke(card->hw, txbuf.ptr_data_bfr, data, len);

	txbuf.data_length = len;
	card->hw_iface.poke(card->hw,
		       		card->u.c.txbuf_off,
				&txbuf,
				sizeof(txbuf));
	txbuf.opp_flag = 1;		/* start transmission */
	card->hw_iface.poke_byte(card->hw,
		       		card->u.c.txbuf_off+
				offsetof(ASY_DATA_TX_STATUS_EL_STRUCT, opp_flag),
				txbuf.opp_flag);
	
	/* Update transmit buffer control fields */
	card->u.c.txbuf_off += sizeof(txbuf);
	if (card->u.c.txbuf_off > card->u.c.txbuf_last_off){
		card->u.c.txbuf_off = card->u.c.txbuf_base_off;
	}
	
	return 0;
}

/*============================================================================
 * Enable timer interrupt  
 */
static void chdlc_enable_timer (void* card_id)
{
	sdla_t		*card = (sdla_t*)card_id;
	netdevice_t	*dev;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	DEBUG_TEST("%s: Chdlc enabling timer %s\n",card->devname,
				dev ? wan_netif_name(dev) : "No DEV");
		
	card->u.c.timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	return;
}

/****** Firmware Error Handler **********************************************/

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int chdlc_error (sdla_t *card, int err, wan_mbox_t *mb)
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

/********** Bottom Half Handlers ********************************************/

/* NOTE: There is no API, BH support for Kernels lower than 2.2.X.
 *       DO NOT INSERT ANY CODE HERE, NOTICE THE 
 *       PREPROCESSOR STATEMENT ABOVE, UNLESS YOU KNOW WHAT YOU ARE
 *       DOING */

static void chdlc_bh (unsigned long data)
{
	chdlc_private_area_t* chan = (chdlc_private_area_t*)data;
	sdla_t *card = chan->card;
	struct sk_buff *skb;
	int len=0;

	while ((skb=wan_api_dequeue_skb(chan)) != NULL){

		len=skb->len;
		if (chan->common.usedby == API){
			if (wan_api_rx(chan,skb) == 0){
				card->wandev.stats.rx_packets++;
				card->wandev.stats.rx_bytes += len;
			}else{
				++card->wandev.stats.rx_dropped;
				wan_skb_free(skb);
			}
		}else{
			wan_skb_free(skb);
			++card->wandev.stats.rx_dropped;
		}
	}	

	WAN_TASKLET_END((&chan->common.bh_task));

	return;
}

/* END OF API BH Support */


/****** Interrupt Handlers **************************************************/

/*============================================================================
 * Cisco HDLC interrupt service routine.
 */
static WAN_IRQ_RETVAL wpc_isr (sdla_t* card)
{
	netdevice_t* dev;
	SHARED_MEMORY_INFO_STRUCT	flags;
	int i;

	/* Check for which port the interrupt has been generated
	 * Since Secondary Port is piggybacking on the Primary
         * the check must be done here. 
	 */

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}
	
	card->hw_iface.peek(card->hw, card->flags_off,
			&flags, sizeof(flags));

	/* Start card isr critical area */
	set_bit(0,&card->in_isr);
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	
	/* If we get an interrupt with no network device, stop the interrupts
	 * and issue an error */
	if (!card->tty_opt && !dev && 
	    flags.interrupt_info_struct.interrupt_type != 
	    	COMMAND_COMPLETE_APP_INT_PEND){
		goto isr_done;
	}
	
	/* if critical due to peripheral operations
	 * ie. update() or getstats() then reset the interrupt and
	 * wait for the board to retrigger.
	 */
	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		printk(KERN_INFO "%s: Chdlc ISR:  Critical with PERI_CRIT!\n",
				card->devname);
		goto isr_done;
	}

	/* On a 508 Card, if critical due to if_send 
         * Major Error !!! */
	if(card->type != SDLA_S514) {
		if(test_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
			printk(KERN_INFO "%s: Chdlc ISR: Critical with SEND_CRIT!\n",
				card->devname);
			goto isr_done;
		}
	}

	

	switch(flags.interrupt_info_struct.interrupt_type){

	case RX_APP_INT_PEND:	/* 0x01: receive interrupt */
		rx_intr(card);
		break;

	case TX_APP_INT_PEND:	/* 0x02: transmit interrupt */
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX);

		if (dev && is_queue_stopped(dev)){
			chdlc_private_area_t* chdlc_priv_area=wan_netif_priv(dev);
			
			if (chdlc_priv_area->common.usedby == API){
				start_net_queue(dev);
				wan_wakeup_api(chdlc_priv_area);
			}else{
				wake_net_dev(dev);
			}
		}
		break;

	case COMMAND_COMPLETE_APP_INT_PEND:/* 0x04: cmd cplt */
		++ card->timer_int_enabled;
		break;

	case GLOBAL_EXCEP_COND_APP_INT_PEND:
		process_global_exception(card);
		
		/* Reset the 56k or T1/E1 front end exception condition */
		if(IS_56K_CARD(card) || IS_TE1_CARD(card)) {
			FRONT_END_STATUS_STRUCT	FE_status;

			card->hw_iface.peek(card->hw, 
					    card->fe_status_off,
				       	    &FE_status, 
					    sizeof(FE_status));
			FE_status.opp_flag = 0x01;	
			card->hw_iface.poke(card->hw, 
					    card->fe_status_off,
				       	    &FE_status, 
					    sizeof(FE_status));
		}
		break;

	case TIMER_APP_INT_PEND:
		timer_intr(card);
		break;

	default:

		if (card->next){
			set_bit(0,&card->spurious);
		}else{
			printk(KERN_INFO "%s: spurious interrupt 0x%02X!\n", 
				card->devname,
				flags.interrupt_info_struct.interrupt_type);
			printk(KERN_INFO "Code name: ");
			for(i = 0; i < 4; i ++){
				printk("%c",
					flags.global_info_struct.code_name[i]); 
			}
			printk("\n");
			printk(KERN_INFO "Code version: ");
			for(i = 0; i < 4; i ++){
				printk("%c", 
					flags.global_info_struct.code_version[i]); 
			}
			printk("\n");	
		}
		break;
	}

isr_done:

	card->in_isr = 0;
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);

	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}

/*============================================================================
 * Receive interrupt handler.
 */
static void rx_intr (sdla_t* card)
{
	netdevice_t *dev;
	chdlc_private_area_t *chdlc_priv_area;
	SHARED_MEMORY_INFO_STRUCT 	flags;
	ASY_DATA_RX_STATUS_EL_STRUCT	rxbuf;
	struct sk_buff *skb;
	unsigned addr;
	unsigned len;
	void *buf;
	int i,udp_type;

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	card->hw_iface.peek(card->hw, card->u.c.rxmb_off, &rxbuf, sizeof(rxbuf));
	addr = rxbuf.ptr_data_bfr;
	if (rxbuf.opp_flag != 0x01) {
		printk(KERN_INFO 
			"%s: corrupted Rx buffer @ 0x%X, flag = 0x%02X!\n", 
			card->devname, (unsigned)card->u.c.rxmb_off, rxbuf.opp_flag);
                printk(KERN_INFO "Code name: ");
                for(i = 0; i < 4; i ++)
                        printk(KERN_INFO "%c",
                                flags.global_info_struct.code_name[i]);
		printk(KERN_INFO "\n");
                printk(KERN_INFO "Code version: ");
                for(i = 0; i < 4; i ++)
                        printk(KERN_INFO "%c",
                                flags.global_info_struct.code_version[i]);
                printk(KERN_INFO "\n");

		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it measn that driver 
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash. 
                 */
		printk(KERN_INFO "%s: Critical router failure ...!!!\n", card->devname);
		printk(KERN_INFO "Please contact Sangoma Technologies !\n");
		chdlc_set_intr_mode(card,0);	
		return;
	}

	len  = rxbuf.data_length;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		goto rx_exit;

	if (!is_dev_running(dev))
		goto rx_exit;

	chdlc_priv_area = wan_netif_priv(dev);

	if (chdlc_priv_area->common.usedby == ANNEXG){
		
		/* Take off two CRC bytes */
		if (rxbuf.data_length <= 2 || rxbuf.data_length > 1506 ){
			printk(KERN_INFO "Bad Rx Frame Length %i\n",
					rxbuf.data_length);
			goto rx_exit;
		}	
		len = rxbuf.data_length - CRC_LENGTH;
	}
	
	/* Allocate socket buffer */
	skb = dev_alloc_skb(len+2);

	if (skb == NULL) {
		printk(KERN_INFO "%s: no socket buffers available!\n",
					card->devname);
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

	/* Align IP on 16 byte */
	skb_reserve(skb,2);

	/* Copy data to the socket buffer */
	if((addr + len) > card->u.c.rx_top_off + 1) {
		unsigned tmp = card->u.c.rx_top_off - addr + 1;
		buf = skb_put(skb, tmp);
		card->hw_iface.peek(card->hw, addr, buf, tmp);
		addr = card->u.c.rx_base_off;
		len -= tmp;
	}
		
	buf = skb_put(skb, len);
	card->hw_iface.peek(card->hw, addr, buf, len);

	skb->protocol = htons(ETH_P_IP);

	udp_type = udp_pkt_type( skb, card );

	if(udp_type == UDP_CPIPE_TYPE) {
		if(store_udp_mgmt_pkt(UDP_PKT_FRM_NETWORK,
   				      card, skb, dev, chdlc_priv_area)) {
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
		}

	} else if(chdlc_priv_area->common.usedby == API) {

		api_rx_hdr_t* api_rx_hdr;
       		skb_push(skb, sizeof(api_rx_hdr_t));
                api_rx_hdr = (api_rx_hdr_t*)&skb->data[0x00];
		api_rx_hdr->wan_hdr_hdlc_error_flag=0;
     		api_rx_hdr->wan_hdr_hdlc_time_stamp = rxbuf.time_stamp;

                skb->protocol = htons(WP_PVC_PROT);
     		wan_skb_reset_mac_header(skb);
		skb->dev      = dev;
               	skb->pkt_type = WAN_PACKET_DATA;

		if (wan_api_enqueue_skb(chdlc_priv_area,skb) < 0){
			wan_skb_free(skb);
			++card->wandev.stats.rx_dropped;
			goto rx_exit;
		}

		WAN_TASKLET_SCHEDULE(&chdlc_priv_area->common.bh_task);

	} else if (chdlc_priv_area->common.usedby == ANNEXG){

		if (chdlc_priv_area->annexg_dev){
			skb->protocol = htons(ETH_P_X25);
			skb->dev = chdlc_priv_area->annexg_dev;
                	wan_skb_reset_mac_header(skb);

			if (wan_api_enqueue_skb(chdlc_priv_area,skb) < 0){
				wan_skb_free(skb);
				++card->wandev.stats.rx_dropped;
				goto rx_exit;
			}

			WAN_TASKLET_SCHEDULE(&chdlc_priv_area->common.bh_task);

		}else{
			wan_skb_free(skb);
			++card->wandev.stats.rx_errors;
		}

	}else if (chdlc_priv_area->common.usedby == BRIDGE ||
		  chdlc_priv_area->common.usedby == BRIDGE_NODE){
		
		/* Make sure it's an Ethernet frame, otherwise drop it */
		if (skb->len <= ETH_ALEN) {
			wan_skb_free(skb);
			++card->wandev.stats.rx_errors;
			goto rx_exit;
		}

		card->wandev.stats.rx_packets ++;
		card->wandev.stats.rx_bytes += skb->len;

		skb->dev = dev;
		skb->protocol=eth_type_trans(skb,dev);
		netif_rx(skb);
	}else{
		/* FIXME: we should check to see if the received packet is a 
                          multicast packet so that we can increment the multicast 
                          statistic
                          ++ chdlc_priv_area->if_stats.multicast;
		*/
               	/* Pass it up the protocol stack */

		card->wandev.stats.rx_packets ++;
		card->wandev.stats.rx_bytes += skb->len;

		skb->protocol = htons(ETH_P_IP);
                skb->dev = dev;
                wan_skb_reset_mac_header(skb);
                netif_rx(skb);
	}

rx_exit:
	/* Release buffer element and calculate a pointer to the next one */
	rxbuf.opp_flag = 0x00;
	card->hw_iface.poke_byte(card->hw, 
			card->u.c.rxmb_off+offsetof(ASY_DATA_RX_STATUS_EL_STRUCT, opp_flag),
		        rxbuf.opp_flag);
	card->u.c.rxmb_off += sizeof(rxbuf);
	if (card->u.c.rxmb_off > card->u.c.rxbuf_last_off){
		card->u.c.rxmb_off = card->u.c.rxbuf_base_off;
	}
}

/*============================================================================
 * Timer interrupt handler.
 * The timer interrupt is used for two purposes:
 *    1) Processing udp calls from 'cpipemon'.
 *    2) Reading board-level statistics for updating the proc file system.
 */
void timer_intr(sdla_t *card)
{
        netdevice_t* dev=NULL;
        chdlc_private_area_t* chdlc_priv_area = NULL;
	/* TE timer interrupt */
	if (card->u.c.timer_int_enabled & TMR_INT_ENABLED_TE) {
		card->wandev.fe_iface.polling(&card->fe);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}

   	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev)){
		goto timer_isr_exit;
	}
	
        chdlc_priv_area = wan_netif_priv(dev);

	/* Configure hardware */
	if (card->u.c.timer_int_enabled & TMR_INT_ENABLED_CONFIG) {
		config_chdlc(card, dev);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_CONFIG;
	}
	
	/* process a udp call if pending */
       	if(card->u.c.timer_int_enabled & TMR_INT_ENABLED_UDP) {
               	process_udp_mgmt_pkt(card, dev,
                       chdlc_priv_area,0);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_UDP;
        }

	/* read the communications statistics if required */
	if(card->u.c.timer_int_enabled & TMR_INT_ENABLED_UPDATE) {
		update_comms_stats(card, chdlc_priv_area);
                if(!(-- chdlc_priv_area->update_comms_stats)) {
			card->u.c.timer_int_enabled &= 
				~TMR_INT_ENABLED_UPDATE;
		}
        }

timer_isr_exit:

	/* only disable the timer interrupt if there are no udp or statistic */
	/* updates pending */
        if(!card->u.c.timer_int_enabled) {
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
        }
}


/*-----------------------------------------------------------------------------
   set_asy_config() used to set asynchronous configuration options on the board
------------------------------------------------------------------------------*/

static int set_asy_config(sdla_t* card)
{

        ASY_CONFIGURATION_STRUCT cfg;
 	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memset(&cfg, 0, sizeof(ASY_CONFIGURATION_STRUCT));

	if(card->wandev.clocking)
		cfg.baud_rate = card->wandev.bps;

	cfg.line_config_options = (card->wandev.electrical_interface == WANOPT_RS232) ?
		INTERFACE_LEVEL_RS232 : INTERFACE_LEVEL_V35;

	cfg.modem_config_options	= 0;
	cfg.API_options 		= card->u.c.api_options;
	cfg.protocol_options		= card->u.c.protocol_options;
	cfg.Tx_bits_per_char		= card->u.c.tx_bits_per_char;
	cfg.Rx_bits_per_char		= card->u.c.rx_bits_per_char;
	cfg.stop_bits			= card->u.c.stop_bits;
	cfg.parity			= card->u.c.parity;
	cfg.break_timer			= card->u.c.break_timer;
	cfg.Rx_inter_char_timer		= card->u.c.inter_char_timer; 
	cfg.Rx_complete_length		= card->u.c.rx_complete_length; 
	cfg.XON_char			= card->u.c.xon_char;
	cfg.XOFF_char			= card->u.c.xoff_char;
	cfg.statistics_options		= 0x001F;

	mb->wan_data_len = sizeof(ASY_CONFIGURATION_STRUCT);
	memcpy(mb->wan_data, &cfg, mb->wan_data_len);
	mb->wan_command = SET_ASY_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK) 
		chdlc_error (card, err, mb);
	return err;
}

/*============================================================================
 * Enable asynchronous communications.
 */

static int asy_comm_enable (sdla_t* card)
{
	netdevice_t		*dev;
	int 			err;
	wan_mbox_t		*mb = &card->wan_mbox;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL){
		return -EINVAL;
	}
	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_ASY_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK && dev)
		chdlc_error(card, err, mb);
	
	if (!err)
		card->u.c.comm_enabled = 1;

	return err;
}

/*============================================================================
 * Process global exception condition
 */
static int process_global_exception(sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_data_len = 0;
	mb->wan_command = READ_GLOBAL_EXCEPTION_CONDITION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != CMD_TIMEOUT ){
	
		switch(mb->wan_return_code) {
         
	      	case EXCEP_MODEM_STATUS_CHANGE:

			if (IS_56K_CARD(card)) {

				FRONT_END_STATUS_STRUCT FE_status;

				card->hw_iface.peek(card->hw, 
					    	    card->fe_status_off,
						    &FE_status, 
						    sizeof(FE_status));

				card->fe.fe_param.k56_param.RR8_reg_56k = 
					FE_status.FE_U.stat_56k.RR8_56k;	
				card->fe.fe_param.k56_param.RRA_reg_56k = 
					FE_status.FE_U.stat_56k.RRA_56k;	
				card->fe.fe_param.k56_param.RRC_reg_56k = 
					FE_status.FE_U.stat_56k.RRC_56k;	
				card->wandev.fe_iface.read_alarm(&card->fe, 0); 

				chdlc_handle_front_end_state(card);
				break;
			
			}
			
			if (IS_TE1_CARD(card)) {
				/* TE1 T1/E1 interrupt */
				card->wandev.fe_iface.isr(&card->fe);
				chdlc_handle_front_end_state(card);
				break;
			}	

			if ((mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) == (DCD_HIGH | CTS_HIGH)){
				card->fe.fe_status = FE_CONNECTED;
			}else{
				card->fe.fe_status = FE_DISCONNECTED;
			}
			
			printk(KERN_INFO "%s: Modem status change\n",
				card->devname);

			switch(mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) {
				
				case ((DCD_HIGH | CTS_HIGH)):
                                        printk(KERN_INFO "%s: DCD high, CTS high\n",card->devname);
                                        break;
				
				case (DCD_HIGH):
					printk(KERN_INFO "%s: DCD high, CTS low\n",card->devname);
					break;
					
				case (CTS_HIGH):
                                        printk(KERN_INFO "%s: DCD low, CTS high\n",card->devname); 
					break;
				
				default:
                                        printk(KERN_INFO "%s: DCD low, CTS low\n",card->devname);
                                        break;
			}

			break;

                default:
                        printk(KERN_INFO "%s: Global exception %x\n",
				card->devname, mb->wan_return_code);
                        break;
                }
	}
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
	chdlc_private_area_t* chan= (chdlc_private_area_t*)wan_netif_priv(dev);
	unsigned long smp_flags;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	int err=0;
	
	if (!chan){
		return -ENODEV;
	}
	card=chan->card;

	NET_ADMIN_CHECK();

	switch(cmd)
	{

		case SIOC_WANPIPE_BIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}
			
			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
			break;

		case SIOC_WANPIPE_UNBIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}

			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
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

		case SIOC_ASYHDLC_RX_AVAIL_CMD:
			{
			SHARED_MEMORY_INFO_STRUCT flags;
			card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
			if (flags.async_info_struct.asy_rx_avail){
				err=HDLC_RX_IN_PROCESS;
			}else{
				err=NO_HDLC_RX_IN_PROCESS;
			}
			}
			break;
			
		case SIOC_WANPIPE_PIPEMON: 
			
			if (atomic_read(&chan->udp_pkt_len) != 0){
				return -EBUSY;
			}
	
			atomic_set(&chan->udp_pkt_len,MAX_LGTH_UDP_MGNT_PKT);
			
			/* For performance reasons test the critical
			 * here before spin lock */
			if (test_bit(0,&card->in_isr)){
				atomic_set(&chan->udp_pkt_len,0);
				return -EBUSY;
			}
		
			
			wan_udp_pkt=(wan_udp_pkt_t*)chan->udp_pkt_data;
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)) {
				printk(KERN_INFO "%s:%s Pipemon command failed, Driver busy: try again.\n",
						card->devname,dev->name);
				atomic_set(&chan->udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EBUSY;
			}
			
			process_udp_mgmt_pkt(card,dev,chan,1);
			
			spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */
			
			if (atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				printk(KERN_INFO "%s: Error: Pipemon buf too bit on the way up! %i\n",
						card->devname,atomic_read(&chan->udp_pkt_len));
				atomic_set(&chan->udp_pkt_len,0);
				return -EINVAL;
			}

			if (copy_to_user(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}
			
			atomic_set(&chan->udp_pkt_len,0);
			return 0;
	
			
		default:
			return -EOPNOTSUPP;
	}
	return err;
}



/*=============================================================================
 * Store a UDP management packet for later processing.
 */

static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, netdevice_t* dev,
                                chdlc_private_area_t* chdlc_priv_area )
{
	int udp_pkt_stored = 0;

	if(!atomic_read(&chdlc_priv_area->udp_pkt_len) &&
	  (skb->len <= MAX_LGTH_UDP_MGNT_PKT)) {
		atomic_set(&chdlc_priv_area->udp_pkt_len, skb->len);
		chdlc_priv_area->udp_pkt_src = udp_pkt_src;
       		memcpy(chdlc_priv_area->udp_pkt_data, skb->data, skb->len);
		card->u.c.timer_int_enabled = TMR_INT_ENABLED_UDP;
		udp_pkt_stored = 1;
	}

	wan_skb_free(skb);
		
	return(udp_pkt_stored);
}


/*=============================================================================
 * Process UDP management packet.
 */

static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				chdlc_private_area_t* chdlc_priv_area, int local_dev ) 
{
	unsigned char *buf;
	unsigned int len;
	struct sk_buff *new_skb;
	int udp_mgmt_req_valid = 1;
	SHARED_MEMORY_INFO_STRUCT flags;
	wan_mbox_t *mb = &card->wan_mbox;
	wan_udp_pkt_t *wan_udp_pkt;
	struct timeval tv;
	int err;

	wan_udp_pkt = (wan_udp_pkt_t *) chdlc_priv_area->udp_pkt_data;

	if (!local_dev){

		if(chdlc_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK){

			/* Only these commands are support for remote debugging.
			 * All others are not */
			switch(wan_udp_pkt->wan_udp_command) {

				case READ_GLOBAL_STATISTICS:
				case READ_MODEM_STATUS:  
				case CPIPE_ROUTER_UP_TIME:
				case READ_COMMS_ERROR_STATS:
				case READ_ASY_OPERATIONAL_STATS:
				/* These two commands are executed for
				 * each request */
				case READ_ASY_CONFIGURATION:
				case READ_ASY_CODE_VERSION:
				case WAN_GET_MEDIA_TYPE:
				case WAN_FE_GET_CFG:
				case WAN_FE_GET_STAT:
				case WAN_GET_PROTOCOL:
				case WAN_GET_PLATFORM:
					udp_mgmt_req_valid = 1;
					break;
				default:
					udp_mgmt_req_valid = 0;
					break;
			} 
		}
	}

  	if(!udp_mgmt_req_valid) {

		/* set length to 0 */
		wan_udp_pkt->wan_udp_data_len = 0;

    		/* set return code */
		wan_udp_pkt->wan_udp_return_code = 0xCD;

		if (net_ratelimit()){	
			printk(KERN_INFO 
			"%s: Warning, Illegal UDP command attempted from network: %x\n",
			card->devname,wan_udp_pkt->wan_udp_command);
		}

   	} else {
		wan_udp_pkt->wan_udp_opp_flag = 0;

		switch(wan_udp_pkt->wan_udp_command) {

		case CPIPE_FT1_READ_STATUS:
			card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
			((unsigned char *)wan_udp_pkt->wan_udp_data )[0] =
				flags.FT1_info_struct.parallel_port_A_input;

			((unsigned char *)wan_udp_pkt->wan_udp_data )[1] =
				flags.FT1_info_struct.parallel_port_B_input;
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			wan_udp_pkt->wan_udp_data_len = 2;
			mb->wan_data_len = 2;
			break;

		case CPIPE_ROUTER_UP_TIME:
			do_gettimeofday( &tv );
			chdlc_priv_area->router_up_time = tv.tv_sec - 
					chdlc_priv_area->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					chdlc_priv_area->router_up_time;	
			mb->wan_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			break;

		case WAN_GET_MEDIA_TYPE:
		case WAN_FE_GET_STAT:
		case WAN_FE_LB_MODE:
 		case WAN_FE_FLUSH_PMON:
		case WAN_FE_GET_CFG:
			if (IS_TE1_CARD(card)){
				card->wandev.fe_iface.process_udp(
						&card->fe, 
						&wan_udp_pkt->wan_udp_cmd,
						&wan_udp_pkt->wan_udp_data[0]);
			}else if (IS_56K_CARD(card)){
				card->wandev.fe_iface.process_udp(
						&card->fe, 
						&wan_udp_pkt->wan_udp_cmd,
						&wan_udp_pkt->wan_udp_data[0]);
			}else{
				if (wan_udp_pkt->wan_udp_command == WAN_GET_MEDIA_TYPE){
		    			wan_udp_pkt->wan_udp_data_len = sizeof(wan_femedia_t); 
					wan_udp_pkt->wan_udp_return_code = CMD_OK;
				}else{
					wan_udp_pkt->wan_udp_return_code = WAN_UDP_INVALID_CMD;
				}
			}
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			break;
			
		case WAN_GET_PROTOCOL:
		   	wan_udp_pkt->wan_udp_chdlc_num_frames = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		default:
			/* it's a board command */
			mb->wan_command = wan_udp_pkt->wan_udp_command;
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			if (mb->wan_data_len) {
				memcpy(&mb->wan_data, (unsigned char *) wan_udp_pkt->
							wan_udp_data, mb->wan_data_len);
	      		} 

			/* run the command on the board */
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != COMMAND_OK) {
				chdlc_error(card,err,mb);
				wan_udp_pkt->wan_udp_return_code = mb->wan_return_code;
				break;
			}
			
			/* copy the result back to our buffer */
	         	memcpy(&wan_udp_pkt->wan_udp_hdr.wan_cmd, mb, sizeof(wan_cmd_t)); 
			
			if (mb->wan_data_len) {
	         		memcpy(&wan_udp_pkt->wan_udp_data, &mb->wan_data, 
								mb->wan_data_len); 
	      		}

		} /* end of switch */
     	} /* end of else */

     	/* Fill UDP TTL */
	wan_udp_pkt->wan_ip_ttl= card->wandev.ttl; 

	if (local_dev){
		wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
		return 1;
	}
	
	len = reply_udp(chdlc_priv_area->udp_pkt_data, mb->wan_data_len);

     	if(chdlc_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK){

		/* Must check if we interrupted if_send() routine. The
		 * tx buffers might be used. If so drop the packet */
	   	if (!test_bit(SEND_CRIT,&card->wandev.critical)) {
		
			if(!chdlc_send(card, chdlc_priv_area->udp_pkt_data, len, 0)) {
				++ card->wandev.stats.tx_packets;
				card->wandev.stats.tx_bytes += len;
			}
		}
	} else {	
	
		/* Pass it up the stack
    		   Allocate socket buffer */
		if ((new_skb = dev_alloc_skb(len)) != NULL) {
			/* copy data into new_skb */

 	    		buf = skb_put(new_skb, len);
  	    		memcpy(buf, chdlc_priv_area->udp_pkt_data, len);

            		/* Decapsulate pkt and pass it up the protocol stack */
	    		new_skb->protocol = htons(ETH_P_IP);
            		new_skb->dev = dev;
	    		wan_skb_reset_mac_header(new_skb);

			netif_rx(new_skb);
		} else {
	    	
			printk(KERN_INFO "%s: no socket buffers available!\n",
					card->devname);
  		}
    	}

	atomic_set(&chdlc_priv_area->udp_pkt_len,0);
 	
	return 0;
}

/*============================================================================
 * Initialize Receive and Transmit Buffers.
 */

static void init_chdlc_tx_rx_buff( sdla_t* card)
{
	wan_mbox_t			*mb = &card->wan_mbox;
	unsigned long			tx_config_off;
	unsigned long			rx_config_off;
	ASY_TX_STATUS_EL_CFG_STRUCT	tx_config;
	ASY_RX_STATUS_EL_CFG_STRUCT	rx_config;
	char				err;
	
	mb->wan_data_len = 0;
	mb->wan_command = READ_ASY_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		netdevice_t	*dev;
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			chdlc_error(card,err,mb);
		}
		return;
	}

	/* ALEX: Apr 8 2004 SANGOMA ISA CARD */
	tx_config_off =
		((ASY_CONFIGURATION_STRUCT *)mb->wan_data)->
       	                    ptr_asy_Tx_stat_el_cfg_struct;
       	rx_config_off = 
		((ASY_CONFIGURATION_STRUCT *)mb->wan_data)->
			ptr_asy_Rx_stat_el_cfg_struct;
       	/* Setup Head and Tails for buffers */
	card->hw_iface.peek(card->hw, tx_config_off, &tx_config, sizeof(tx_config));
       	card->u.c.txbuf_base_off = tx_config.base_addr_Tx_status_elements;
       	card->u.c.txbuf_last_off = 
               	card->u.c.txbuf_base_off +
		(tx_config.number_Tx_status_elements - 1) * 
			sizeof(ASY_DATA_TX_STATUS_EL_STRUCT);
	card->hw_iface.peek(card->hw, rx_config_off, &rx_config, sizeof(rx_config));
       	card->u.c.rxbuf_base_off =
               	rx_config.base_addr_Rx_status_elements;
       	card->u.c.rxbuf_last_off =
		card->u.c.rxbuf_base_off +
		(rx_config.number_Rx_status_elements - 1) * 
			sizeof(ASY_DATA_RX_STATUS_EL_STRUCT);
 	/* Set up next pointer to be used */
       	card->u.c.txbuf_off = tx_config.next_Tx_status_element_to_use;
       	card->u.c.rxmb_off =
                rx_config.next_Rx_status_element_to_use;
 
        /* Setup Actual Buffer Start and end addresses */
        card->u.c.rx_base_off = rx_config.base_addr_Rx_buffer;
        card->u.c.rx_top_off  = rx_config.end_addr_Rx_buffer;

}

/*=============================================================================
 * Perform Interrupt Test by running READ_ASY_CODE_VERSION command MAX_INTR
 * _TEST_COUNTER times.
 */
static int intr_test( sdla_t* card)
{
	wan_mbox_t* 	mb = &card->wan_mbox;
	int err,i;

	card->timer_int_enabled = 0;
	
	err = chdlc_set_intr_mode(card, APP_INT_ON_COMMAND_COMPLETE);

	if (err == CMD_OK) { 
		for (i = 0; i < MAX_INTR_TEST_COUNTER; i ++) {	
				
			mb->wan_data_len  = 0;
			mb->wan_command = READ_ASY_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != CMD_OK) 
				chdlc_error(card, err, mb);
		}
	}else{
		return err;
	}

//	udelay(2000);
//	schedule();
//	udelay(2000);

	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	err = chdlc_set_intr_mode(card, 0);

	if (err != CMD_OK)
		return err;

	return 0;
}

/*==============================================================================
 * Determine what type of UDP call it is. CPIPEAB ?
 */
static int udp_pkt_type(struct sk_buff *skb, sdla_t* card)
{
	 wan_udp_pkt_t *wan_udp_pkt = (wan_udp_pkt_t *)skb->data;

	 if (skb->len < sizeof(wan_udp_pkt_t)){
		return UDP_INVALID_TYPE;
	 }

#ifdef _WAN_UDP_DEBUG
		printk(KERN_INFO "SIG %s = %s\n\
				  UPP %x = %x\n\
				  PRT %x = %x\n\
				  REQ %i = %i\n\
				  36 th = %x 37th = %x\n",
				  wan_udp_pkt->wan_udp_signature,
				  UDPMGMT_SIGNATURE,
				  wan_udp_pkt->wan_udp_dport,
				  ntohs(card->wandev.udp_port),
				  wan_udp_pkt->wan_ip_p,
				  UDPMGMT_UDP_PROTOCOL,
				  wan_udp_pkt->wan_udp_request_reply,
				  UDPMGMT_REQUEST,
				  skb->data[36], skb->data[37]);
#endif	
		
	if ((wan_udp_pkt->wan_udp_dport == ntohs(card->wandev.udp_port)) &&
	   (wan_udp_pkt->wan_ip_p == UDPMGMT_UDP_PROTOCOL) &&
	   (wan_udp_pkt->wan_udp_request_reply == UDPMGMT_REQUEST)) {
		if (!strncmp(wan_udp_pkt->wan_udp_signature,UDPMGMT_SIGNATURE,8)){
			return UDP_CPIPE_TYPE;
		}
		if (!strncmp(wan_udp_pkt->wan_udp_signature,GLOBAL_UDP_SIGNATURE,8)){
			return UDP_CPIPE_TYPE;
		}
	}
	return UDP_INVALID_TYPE;
}

/*============================================================================
 * Set PORT state.
 */
static void port_set_state (sdla_t *card, int state)
{
	netdevice_t		*dev;
       
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

                card->wandev.state = state;
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev && wan_netif_priv(dev)){
			chdlc_private_area_t *chdlc_priv_area = wan_netif_priv(dev);
			chdlc_priv_area->common.state = state;
			if (chdlc_priv_area->common.usedby == API){
				wan_update_api_state(chdlc_priv_area);
			}
		}
        }
}

/*===========================================================================
 * config_chdlc
 *
 *	Configure the chdlc protocol and enable communications.		
 *
 *   	The if_open() function binds this function to the poll routine.
 *      Therefore, this function will run every time the chdlc interface
 *      is brought up. We cannot run this function from the if_open 
 *      because if_open does not have access to the remote IP address.
 *      
 *	If the communications are not enabled, proceed to configure
 *      the card and enable communications.
 *
 *      If the communications are enabled, it means that the interface
 *      was shutdown by ether the user or driver. In this case, we 
 *      have to check that the IP addresses have not changed.  If
 *      the IP addresses have changed, we have to reconfigure the firmware
 *      and update the changed IP addresses.  Otherwise, just exit.
 *
 */

static int config_chdlc (sdla_t *card, netdevice_t *dev)
{
	if (card->u.c.comm_enabled){
		return 0;
	}
	
	/* Setup the Board for asynchronous mode */
	if (set_asy_config(card)) {
		printk (KERN_INFO "%s: Failed CHDLC Async configuration!\n",
			card->devname);
		return 0;
	}

	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);

	if (IS_TE1_CARD(card)) {
		int	err = -EINVAL;
		printk(KERN_INFO "%s: Configuring onboard %s CSU/DSU\n",
			card->devname, 
			(IS_T1_CARD(card))?"T1":"E1");
		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			printk(KERN_INFO "%s: Failed %s configuratoin!\n",
					card->devname,
					(IS_T1_CARD(card))?"T1":"E1");
			return -EINVAL;
		}
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}

	 
	if (IS_56K_CARD(card)) {
		int	err = -EINVAL;
		printk(KERN_INFO "%s: Configuring 56K onboard CSU/DSU\n",
			card->devname);

		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			printk (KERN_INFO "%s: Failed 56K configuration!\n",
				card->devname);
			return -EINVAL;
		}
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}

	
	/* Set interrupt mode and mask */
        if (chdlc_set_intr_mode(card, APP_INT_ON_RX_FRAME |
                		APP_INT_ON_GLOBAL_EXCEP_COND |
                		APP_INT_ON_TX |
                		APP_INT_ON_TIMER)){
		printk (KERN_INFO "%s: Failed to set interrupt triggers!\n",
				card->devname);
		return -EINVAL;	
        }
	
	/* Mask All interrupts  */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off,
			    (APP_INT_ON_RX_FRAME | APP_INT_ON_TX | 
			     APP_INT_ON_TIMER    | APP_INT_ON_GLOBAL_EXCEP_COND));


	/* Enable communications */
	if (asy_comm_enable(card) != 0) {
		printk(KERN_INFO "%s: Failed to enable async commnunication!\n",
				card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
		card->u.c.comm_enabled=0;
		chdlc_set_intr_mode(card,0);
		return -EINVAL;
	}

	DEBUG_EVENT("%s: Communications enabled on startup\n",
					card->devname);

	/* Initialize Rx/Tx buffer control fields */
	init_chdlc_tx_rx_buff(card);
	port_set_state(card, WAN_CONNECTED);

	/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1); 
	}

	/* Unmask all interrupts except the Transmit and Timer interrupts */
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, 
			(APP_INT_ON_RX_FRAME | APP_INT_ON_GLOBAL_EXCEP_COND));
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);		
	return 0; 
}


static int set_adapter_config (sdla_t* card)
{
#if 0
	wan_mbox_t* mb = &card->wan_mbox;
	ADAPTER_CONFIGURATION_STRUCT* cfg = (ADAPTER_CONFIGURATION_STRUCT*)mb->wan_data;
	int err;

	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &cfg->adapter_type);
	cfg->adapter_config = 0x00; 
	cfg->operating_frequency = 00; 
	mb->wan_data_len = sizeof(ADAPTER_CONFIGURATION_STRUCT);
	mb->wan_command = SET_ADAPTER_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		chdlc_error(card,err,mb);
	}
	return (err);
#else
	return 0;
#endif
}


void s508_lock (sdla_t *card, unsigned long *smp_flags)
{
	spin_lock_irqsave(&card->wandev.lock, *smp_flags);
        if (card->next){
        	spin_lock(&card->next->wandev.lock);
	}
}

void s508_unlock (sdla_t *card, unsigned long *smp_flags)
{
        if (card->next){
        	spin_unlock(&card->next->wandev.lock);
        }
        spin_unlock_irqrestore(&card->wandev.lock, *smp_flags);
}

/*
 * ******************************************************************
 * Proc FS function 
 */
#define PROC_CFG_FRM	"%-15s| %-12s|\n"
#define PROC_STAT_FRM	"%-15s| %-12s| %-14s|\n"
static char chdlc_config_hdr[] =
	"Interface name | Device name |\n";
static char chdlc_status_hdr[] =
	"Interface name | Device name | Status        |\n";

static int chdlc_get_config_info(void* priv, struct seq_file* m, int* stop_cnt) 
{
	chdlc_private_area_t*	chdlc_priv_area = priv;
	sdla_t*			card = NULL;

	if (chdlc_priv_area == NULL)
		return m->count;
	card = chdlc_priv_area->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", chdlc_config_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_CFG_FRM, chdlc_priv_area->if_name, card->devname);
	return m->count;
}

static int chdlc_get_status_info(void* priv, struct seq_file* m, int* stop_cnt)
{
	chdlc_private_area_t*	chdlc_priv_area = priv;
	sdla_t*			card = NULL;

	if (chdlc_priv_area == NULL)
		return m->count;
	card = chdlc_priv_area->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", chdlc_status_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_STAT_FRM, 
		chdlc_priv_area->if_name,
		card->devname,
		STATE_DECODE(chdlc_priv_area->common.state));

	return m->count;
}

#define PROC_DEV_FR_S_FRM	"%-20s| %-14s|\n"
#define PROC_DEV_FR_D_FRM	"%-20s| %-14d|\n"
#define PROC_DEV_SEPARATE	"=====================================\n"


static int chdlc_set_dev_config(struct file *file, 
			     	const char *buffer,
			     	unsigned long count, 
			     	void *data)
{
	int 		cnt = 0;
	wan_device_t*	wandev = (void*)data;
	sdla_t*		card = NULL;

	if (wandev == NULL)
		return cnt;

	card = (sdla_t*)wandev->priv;

	printk(KERN_INFO "%s: New device config (%s)\n",
			wandev->name, buffer);
	/* Parse string */

	return count;
}


#define PROC_IF_FR_S_FRM	"%-30s\t%-14s\n"
#define PROC_IF_FR_D_FRM	"%-30s\t%-14d\n"
#define PROC_IF_FR_L_FRM	"%-30s\t%-14ld\n"
#define PROC_IF_SEPARATE	"====================================================\n"

static int chdlc_set_if_info(struct file *file, 
			     const char *buffer,
			     unsigned long count, 
			     void *data)
{
	netdevice_t*		dev = (void*)data;
	chdlc_private_area_t* 	chdlc_priv_area = NULL;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return count;

	chdlc_priv_area = (chdlc_private_area_t*)wan_netif_priv(dev);


	printk(KERN_INFO "%s: New interface config (%s)\n",
			chdlc_priv_area->if_name, buffer);
	/* Parse string */

	return count;
}

static void chdlc_handle_front_end_state(void* card_id)
{
	int rc;
	sdla_t*	card = (sdla_t*)card_id;

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	if (IS_TE1_CARD(card)){
		if (card->fe.fe_status == FE_CONNECTED) {
			if(card->u.c.comm_enabled == 0) {

				rc=chdlc_comm_enable(card);
				if (rc==0){
					init_chdlc_tx_rx_buff(card);
					DEBUG_EVENT("%s: Communications enabled\n",
							card->devname);

					port_set_state(card, WAN_CONNECTED);
				}else{
					DEBUG_EVENT("%s: Critical Error: Failed to enable comms 0x%X\n",
							card->devname,rc);
					port_set_state(card, WAN_DISCONNECTED);
				}
			}
			
		}else{
			port_set_state(card, WAN_DISCONNECTED);
			if (card->u.c.comm_enabled){
				printk(KERN_INFO "%s: Communications disabled\n",
							card->devname);
				chdlc_comm_disable (card);
			}
		}

	}else{
		if (card->fe.fe_status == FE_CONNECTED){
			if (card->u.c.state == WAN_CONNECTED){
				port_set_state(card,WAN_CONNECTED);
			}
		}else{
			port_set_state(card,WAN_DISCONNECTED);
		}
	}
}



/****** End ****************************************************************/
