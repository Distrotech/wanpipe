/*****************************************************************************
* sdla_atm.c	WANPIPE(tm) Multiprotocol WAN Link Driver.
* 		
* 		ATM ALL5 Driver
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
* Jul 13, 2004  David Rokhvarg	Added Idle cell trace.
* Sep 12, 2003  Nenad Corbic	Fixed the TE1 clock selection, the
* 			        conf->electrical_interfaces must be hardcoded, before
* 			        writting the value to the card structure.
* Jan 07, 2002	Nenad Corbic	Initial version.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe.h"
#include "wanproc.h"
#include "wanpipe_abstr.h"
#include "wanpipe_atm_iface.h"
#include "sdla_atm.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */

#if defined(__LINUX__)
#include <linux/if_wanpipe.h>
#endif

/****** Defines & Macros ****************************************************/
#define MAX_TRACE_QUEUE		100
#define MAX_TRACE_BUFFER	(MAX_LGTH_UDP_MGNT_PKT - 	\
				 sizeof(iphdr_t) - 		\
	 			 sizeof(udphdr_t) - 		\
				 sizeof(wan_mgmt_t) - 		\
				 sizeof(wan_trace_info_t) - 	\
		 		 sizeof(wan_cmd_t))

#define ATM_TIMER_TIMEOUT	1
#define POLL_DELAY_TIMEOUT	1

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

/* Private critical flags */
enum { 
	POLL_CRIT = PRIV_CRIT, 
	TX_INTR,
	TASK_POLL
};

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_RX_BUF	50

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with Protocol specific data
 */

typedef struct private_area
{
	wanpipe_common_t common;
	sdla_t		*card;

	unsigned char	encap_mode;
	unsigned short	vpi,vci;
	netdevice_t 	*dev;
	netskb_t  	*tx_cells_skb;
	
	int 		TracingEnabled;		/* For enabling Tracing */

	unsigned long 	router_start_time;
	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;
	
	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned char 	udp_pkt_src;		/* udp packet processing */
	unsigned short 	timer_int_enabled;

	unsigned long 	interface_down;

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	wan_taskq_t 	poll_task;

	u8 		gateway;
	u8 		true_if_encoding;
	
	//FIXME: add driver stats as per frame relay!

	/* Entry in proc fs per each interface */
	struct proc_dir_entry	*dent;

	unsigned char 		udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t 		udp_pkt_len;

	char 			if_name[WAN_IFNAME_SZ+1];
	struct net_device_stats	if_stats;

	void 	*sar_pvc;

	wan_atm_conf_if_t 	cfg;

}private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03

/* keep the idle configuration */
PHY_CONFIGURATION_STRUCT phy_idle_cfg;

/* variable for keeping track of enabling/disabling FT1 monitor status */
static int rCount;
static int Intr_test_counter;

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
  
static int 	if_send (netskb_t* skb, struct net_device* dev);

/* Firmware interface functions */
static int 	frmw_configure 	(sdla_t* card, void* data);
static int 	frmw_comm_enable 	(sdla_t* card);
static int 	frmw_read_version 	(sdla_t* card, char* str);
static int 	frmw_set_intr_mode 	(sdla_t* card, unsigned mode);
static int 	set_adapter_config 	(sdla_t* card);
static int 	frmw_send (sdla_t* card, void* data, unsigned len, unsigned char tx_bits);
static int 	frmw_read_comm_err_stats (sdla_t* card);
static int 	frmw_read_op_stats (sdla_t* card);
static int 	frmw_error (sdla_t *card, int err, wan_mbox_t *mb);
static void 	enable_timer(void* card_id);
static int 	disable_comm_shutdown (sdla_t *card);
static int	init_interrupt_status_ptrs(sdla_t *card);
static void 	if_tx_timeout (struct net_device *dev);

/* Miscellaneous Functions */
static int 	set_frmw_config (sdla_t* card);
static void 	init_tx_rx_buff( sdla_t* card);
static int 	process_exception(sdla_t *card);
static int 	process_global_exception(sdla_t *card);
static int 	update_comms_stats(sdla_t* card,private_area_t*);
static void 	port_set_state (sdla_t *card, int);

static int 	config_frmw (sdla_t *card);
static void 	disable_comm (sdla_t *card);

static void 	trigger_poll (struct net_device *);
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 	
static void frmw_poll (void *arg);
# else
static void frmw_poll (struct work_struct *work);
# endif

/* Interrupt handlers */
static WAN_IRQ_RETVAL 	wpatm_isr (sdla_t* card);
static void 	rx_intr (sdla_t* card);
static void     tx_intr (sdla_t* card);
static void 	timer_intr(sdla_t *);

/* Bottom half handlers */
static void 	wp_bh (unsigned long data);

/* Miscellaneous functions */
static int 	chk_bcast_mcast_addr(sdla_t* card, struct net_device* dev,
				netskb_t *skb);
//static int 	reply_udp( unsigned char *data, unsigned int mbox_len );
static int 	intr_test( sdla_t* card);
static int 	store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                netskb_t *skb, struct net_device* dev,
                                private_area_t*);
static int 	process_udp_mgmt_pkt(sdla_t* card, struct net_device* dev,  
				private_area_t*,
				int local_dev);

static void 	s508_lock (sdla_t *card, unsigned long *smp_flags, unsigned long *smp_flags1);
static void 	s508_unlock (sdla_t *card, unsigned long *smp_flags, unsigned long *smp_flags1);

static void 	wp_handle_rx_packets(netskb_t *skb);
static int 	wp_handle_tx_packets(sdla_t* card, netskb_t *skb);
static int 	wp_handle_out_of_sync_condition(sdla_t *card,unsigned char irq);

static int 
capture_atm_trace_packet(wan_trace_t *trace_info, void *data, int len, char direction);

#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
# warning "Enabling Proc File System Hooks"
static int 	get_config_info(void* priv, char* buf, int cnt, int, int, int*);
static int 	get_status_info(void* priv, char* buf, int cnt, int, int, int*);
#if defined(LINUX_2_4)||defined(LINUX_2_6)
static int 	get_dev_config_info(char* buf, char** start, off_t offs, int len);
static int 	get_if_info(char* buf, char** start, off_t offs, int len);
# else
static int 	get_dev_config_info(char* buf, char** start, off_t offs, int len, int dummy);
static int 	get_if_info(char* buf, char** start, off_t offs, int len, int dummy);
# endif
static int 	set_dev_config(struct file*, const char*, unsigned long, void *);
static int 	set_if_info(struct file*, const char*, unsigned long, void *);
#endif

static void 	atm_timer_poll(unsigned long data);

static netdevice_t* move_dev_to_next (sdla_t *card, netdevice_t *dev);

/* Procfs functions */
static int wan_atm_get_info(void* pcard, struct seq_file* m, int* stop_cnt); 

static void 	handle_front_end_state(void* card_id);


/**SECTION********************************************************* 
 * 
 * Public Functions 
 *
 ******************************************************************/


/*============================================================================
 * wpatm_init - Cisco HDLC protocol initialization routine.
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

int wp_atm_init (sdla_t* card, wandev_conf_t* conf)
{
	int err,i;

	union{
		char str[80];
	} u;
	volatile wan_mbox_t* mb;
	unsigned long timeout;
	netskb_t *skb;

	/* Initialize the disable_comm function
	 * because we don't want upper layers to
	 * call dislabe_comm incase we fail
	 * initializetion */
	
	card->disable_comm = NULL;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_ATM) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Initialize the card mailbox and obtain the mailbox pointer */
	/* Set a pointer to the actual mailbox in the allocated virtual 
	 * memory area */
	/* Alex Apr 8 2004 Sangom ISA card */
	card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;
	mb = &card->wan_mbox;

	if (!card->configured){
		unsigned char return_code = 0x00;

		/* The board will place an 'I' in the return 
		 * code to indicate that it is ready to accept 
		 * commands.  We expect this to be completed 
		 * in less than 1 second. */

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
			DEBUG_EVENT(
				"%s: Initialization not completed by adapter\n",
				card->devname);
			DEBUG_EVENT( "Please contact Sangoma representative.\n");
			return -EIO;
		}
	}


	memcpy(&card->u.atm.atm_cfg,&conf->u.atm,sizeof(wan_atm_conf_t));
	
	wan_atomic_set(&card->wandev.if_cnt,0);
	
	err = (card->hw_iface.check_mismatch) ? 
			card->hw_iface.check_mismatch(card->hw,conf->fe_cfg.media) : -EINVAL;
	if (err){
		return err;
	}
	
	/* TE1 Make special hardware initialization for T1/E1 board */
	if (IS_TE1_MEDIA(&conf->fe_cfg)){
		
		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->fe,&card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg		= card->hw_iface.fe_read;

		card->wandev.fe_enable_timer = enable_timer;
		card->wandev.te_link_state = handle_front_end_state;
		conf->electrical_interface = 
			(IS_T1_CARD(card)) ? WANOPT_V35 : WANOPT_RS232;

		if (card->wandev.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else if (IS_56K_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe,&card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg		= card->hw_iface.fe_read;

		if (card->wandev.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else{
		card->fe.fe_status = FE_CONNECTED;
	}

	if (card->wandev.ignore_front_end_status == WANOPT_NO){
		DEBUG_EVENT( 
		  "%s: Enabling front end link monitor\n",
				card->devname);
	}else{
		DEBUG_EVENT( 
		"%s: Disabling front end link monitor\n",
				card->devname);
	}


	/* Obtain hardware configuration parameters */
	card->wandev.clocking 			= conf->clocking;
	card->wandev.ignore_front_end_status 	= conf->ignore_front_end_status;
	card->wandev.ttl 			= conf->ttl;
	card->wandev.electrical_interface 			= conf->electrical_interface; 
	card->wandev.comm_port 			= conf->comm_port;
	card->wandev.udp_port   		= conf->udp_port;
	card->wandev.new_if_cnt 		= 0;


	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (frmw_read_version(card, u.str))
		return -EIO;


	DEBUG_EVENT( "%s: Running ATM firmware v%s\n",
		card->devname, u.str); 

	if (set_adapter_config(card)) {
		return -EIO;
	}

	card->isr			= &wpatm_isr;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	
#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
	/* Proc fs functions hooks */
	card->wandev.get_config_info 	= &get_config_info;
	card->wandev.get_status_info 	= &get_status_info;
	card->wandev.get_dev_config_info= &get_dev_config_info;
	card->wandev.get_if_info     	= &get_if_info;
	card->wandev.set_dev_config    	= &set_dev_config;
	card->wandev.set_if_info     	= &set_if_info;
#endif
	card->wandev.get_info     	= &wan_atm_get_info;


	/* Setup Port Bps */
	if(card->wandev.clocking) {

		if(card->type == SDLA_S514){
			card->wandev.bps = wp_min(conf->bps,PHY_MAX_BAUD_RATE_S514);
		}else{
			card->wandev.bps = wp_min(conf->bps,PHY_MAX_BAUD_RATE_S508);
		}

		DEBUG_EVENT ("%s: Configuring Baud rate to %i bps\n",
				card->devname,card->wandev.bps);
	}else{
        	card->wandev.bps = 0;
  	}

	
	/* Setup the Port MTU */
	if((card->wandev.comm_port == WANOPT_PRI)) {
		/* For Primary Port 0 */
		if (conf->mtu < MIN_WP_PRI_MTU){
			DEBUG_WARNING("%s: Warning: Limiting MTU to Min=%i\n", 
					card->devname,MIN_WP_PRI_MTU);
			conf->mtu=MIN_WP_PRI_MTU;
		}else if (conf->mtu > MAX_WP_PRI_MTU){
			DEBUG_WARNING("%s: Warning: Limiting MTU to Max=%i\n", 
					card->devname,MIN_WP_PRI_MTU);
			conf->mtu=MAX_WP_PRI_MTU;
		}else{
			DEBUG_WARNING("%s: Warning: Limiting MTU to Max=%i\n", 
					card->devname,MIN_WP_PRI_MTU);
		}
		card->wandev.mtu = conf->mtu;
	}

	if ((err=init_interrupt_status_ptrs(card)) != COMMAND_OK){
		return err;
	}

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
			offsetof(SHARED_MEMORY_INFO_STRUCT, FE_info_struct) + 
			offsetof(FE_INFORMATION_STRUCT, parallel_port_A_input);
	card->wandev.state = WAN_DISCONNECTED;
	
	if (!card->wandev.piggyback){	
		int err;

		/* Perform interrupt testing */
		err = intr_test(card);

		if(err || (Intr_test_counter < MAX_INTR_TEST_COUNTER)) { 
			DEBUG_EVENT( "%s: Interrupt test failed (%i)\n",
					card->devname, Intr_test_counter);
			DEBUG_EVENT( "%s: Please choose another interrupt\n",
					card->devname);

			card->wandev.state = WAN_UNCONFIGURED;
			return -EIO;
		}
		
		DEBUG_EVENT( "%s: Interrupt test passed (%i)\n", 
				card->devname, Intr_test_counter);
		card->configured = 1;
	}

	if (frmw_set_intr_mode(card, APP_INT_ON_TIMER)){
		DEBUG_EVENT( "%s: Failed to set interrupt triggers!\n",
			card->devname);
		card->wandev.state = WAN_UNCONFIGURED;
		return -EIO;	
       	}

	card->u.atm.trace_info = wan_malloc(sizeof(wan_trace_t));
	if (!card->u.atm.trace_info){
		card->wandev.state = WAN_UNCONFIGURED;
		return -ENOMEM;
	}

	wan_trace_info_init(card->u.atm.trace_info,MAX_TRACE_QUEUE);

	skb_queue_head_init(&card->u.atm.wp_rx_free_list);
	skb_queue_head_init(&card->u.atm.wp_rx_used_list);
	skb_queue_head_init(&card->u.atm.wp_rx_data_list);
	skb_queue_head_init(&card->u.atm.wp_tx_prot_list);

	err=wp_sar_register_device(card,
			           card->devname,
				   &card->u.atm.atm_device,
				   &card->u.atm.wp_rx_data_list,
				   &card->u.atm.wp_tx_prot_list,
				   card->u.atm.trace_info);
	if (err){
		wan_free(card->u.atm.trace_info);
		card->wandev.state = WAN_UNCONFIGURED;
		return -EINVAL;
	}

	/* Initialize the receive bh handler, that
	 * will be used to run the ATM SAR for rx packets */
	WAN_TASKLET_INIT(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task), 
			 0, wp_bh, (unsigned long)card);

	wan_init_timer(&card->u.atm.atm_timer,atm_timer_poll,(unsigned long)card);

	for (i=0;i<MAX_RX_BUF;i++){
		skb=wan_skb_alloc((PHY_MAX_CELLS_IN_RX_BLOCK*ATM_CELL_SIZE)+ATM_CELL_SIZE);
		if (!skb){
			while ((skb=wan_skb_dequeue(&card->u.atm.wp_rx_free_list)) != NULL){
				wan_skb_free(skb);
			}

			wp_sar_unregister_device(card->u.atm.atm_device);
			card->u.atm.atm_device=NULL;
			wan_free(card->u.atm.trace_info);
			card->u.atm.trace_info=NULL;
			card->wandev.state = WAN_UNCONFIGURED;
			return -ENOMEM;
		}
		wan_skb_queue_tail(&card->u.atm.wp_rx_free_list,skb);
	}

	/* Mask the Timer interrupt */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	
	/* If we are using backup mode, this flag will
	 * indicate not to look for IP addresses in config_frmw()*/
	card->backup = conf->backup;

	
	/* Set protocol link state to disconnected,
	 * After seting the state to DISCONNECTED this
	 * function must return 0 i.e. success */
	card->u.atm.state = WAN_DISCONNECTED;

	/* Now it is save to call disable_comm, because
	 * all structures have been initialized */
	card->disable_comm = &disable_comm;

	DEBUG_EVENT( "\n");
	return 0;
}

static void atm_timer_poll(unsigned long data)
{
	struct wan_dev_le	*devle;
	sdla_t *card = (sdla_t *)data;
	netdevice_t *dev;
	private_area_t *chan;
	int err;

	if (wan_test_bit(PERI_CRIT,&card->wandev.critical)){
		DEBUG_EVENT ("%s: ATM Poll Timer exiting due to PERI CRIT\n",
				card->devname);
		return;
	}
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!wan_atomic_read(&card->wandev.if_cnt) ||  !dev){
		DEBUG_EVENT ("%s: ATM Poll Timer exiting due to no if\n",
				card->devname);
		return;
	}

	if (card->wandev.state != WAN_CONNECTED){
		goto atm_timer_poll_exit;	
	}

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);	
		if (!dev || !wan_netif_priv(dev)) continue;

		if (!wan_netif_up(dev)){
			continue;
		}

		chan = wan_netif_priv(dev);
		if (!(chan->sar_pvc)){
			continue;
		}

		err=wanpipe_sar_poll(chan->sar_pvc, ATM_TIMER_TIMEOUT);
		if (!err){
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
		}
	}

atm_timer_poll_exit:
	wan_add_timer(&card->u.atm.atm_timer, ATM_TIMER_TIMEOUT*HZ);
	return;
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
	unsigned long timeout;

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	/* more sanity checks */
        if(!card->flags_off)
                return -ENODEV;

	if(wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical))
                return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if(dev == NULL)
		return -ENODEV;

	if((priv_area=wan_netif_priv(dev)) == NULL)
		return -ENODEV;


       	if(card->update_comms_stats){
		return -EAGAIN;
	}
			
	/* TE1	Change the update_comms_stats variable to 3,
	 * 	only for T1/E1 card, otherwise 2 for regular
	 *	S514/S508 card.
	 *	Each timer interrupt will update only one type
	 *	of statistics.
	 */
	card->update_comms_stats = (IS_TE1_CARD(card) || IS_56K_CARD(card)) ? 3 : 2;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	card->timer_int_enabled = TMR_INT_ENABLED_UPDATE;
  
	/* wait a maximum of 1 second for the statistics to be updated */ 
        timeout = jiffies;
        for(;;) {
		if(card->update_comms_stats == 0)
			break;

                if ((jiffies - timeout) > (1 * HZ)){
    			card->update_comms_stats = 0;
 			card->timer_int_enabled &=
				~TMR_INT_ENABLED_UPDATE; 
 			return -EAGAIN;
		}
        }

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
	int err = 0;

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		DEBUG_ERROR( "%s: Error: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	DEBUG_EVENT( "%s: Configuring Interface: %s\n",
			card->devname, conf->name);

		
	/* allocate and initialize private data */
	priv_area = wan_malloc(sizeof(private_area_t));
	if(priv_area == NULL){ 
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}
	memset(priv_area, 0, sizeof(private_area_t));
	
	strncpy(priv_area->if_name, conf->name, WAN_IFNAME_SZ);

	memcpy(&priv_area->cfg, &conf->u.atm, sizeof(wan_atm_conf_if_t));
	
	priv_area->card = card; 
	priv_area->dev = dev;

	/* Initialize the socket binding information
	 * These hooks are used by the API sockets to
	 * bind into the network interface */
	priv_area->common.sk = NULL;
	priv_area->common.state = WAN_CONNECTING;

	priv_area->TracingEnabled = 0;
	priv_area->route_status = NO_ROUTE;
	priv_area->route_removed = 0;

        /* Setup interface as:
	 *    WANPIPE 	  = IP over Protocol (Firmware)
	 *    API     	  = Raw Socket access to Protocol (Firmware)
	 *    BRIDGE  	  = Ethernet over Protocol, no ip info
	 *    BRIDGE_NODE = Ethernet over Protocol, with ip info
	 */
	if(strcmp(conf->usedby, "WANPIPE") == 0) {

		DEBUG_EVENT( "%s:%s: Running in WANPIPE mode!\n",
			wandev->name,conf->name);
		priv_area->common.usedby = WANPIPE;

		/* Option to bring down the interface when 
        	 * the link goes down */
		if (conf->if_down){
			wan_set_bit(DYN_OPT_ON,&priv_area->interface_down);
			DEBUG_EVENT( 
			 "%s:%s: Dynamic interface configuration enabled\n",
			   card->devname,priv_area->if_name);
		} 

	}else if( strcmp(conf->usedby, "API") == 0) {
		priv_area->common.usedby = API;
		DEBUG_EVENT( "%s:%s: Running in API mode !\n",
			wandev->name,priv_area->if_name);

	}else if (strcmp(conf->usedby, "BRIDGE") == 0) {
		priv_area->common.usedby = BRIDGE;
		DEBUG_EVENT( "%s:%s: Running in WANPIPE (BRIDGE) mode.\n", 
				card->devname,priv_area->if_name);
	
	}else if (strcmp(conf->usedby, "BRIDGE_N") == 0) {
		priv_area->common.usedby = BRIDGE_NODE;
		DEBUG_EVENT( "%s:%s: Running in WANPIPE (BRIDGE_NODE) mode.\n", 
				card->devname,priv_area->if_name);
	
	}else{
		DEBUG_ERROR( "%s:%s: Error: Invalid operation mode [WANPIPE|API|BRIDGE|BRIDGE_NODE]\n",
				card->devname,priv_area->if_name);
		err=-EINVAL;
		goto new_if_error;
	}


	DEBUG_EVENT("%s:%s: Configuring ATM Encap=%d, Vpi=%d, Vci=%d\n",
			card->devname,priv_area->if_name,
			priv_area->cfg.encap_mode,
			priv_area->cfg.vpi,
			priv_area->cfg.vci);
	
	err=wp_sar_register_pvc(card->u.atm.atm_device,
			        &priv_area->sar_pvc,
				(void **)&priv_area->tx_cells_skb,
				priv_area,
				priv_area->if_name,
				dev,
				&priv_area->cfg,
				card->wandev.mtu);
	if (err){
		err=-EBUSY;
		goto new_if_error;
	}

	/* If gateway option is set, then this interface is the
	 * default gateway on this system. We must know that information
	 * in case DYNAMIC interface configuration is enabled. 
	 *
	 * I.E. If the interface is brought down by the driver, the
	 *      default route will also be removed.  Once the interface
	 *      is brought back up, we must know to re-astablish the
	 *      default route.
	 */
	if ((priv_area->gateway = conf->gateway) == WANOPT_YES){
		DEBUG_EVENT( "%s:%s: Interface is set as a gateway.\n",
			card->devname,priv_area->if_name);
	}

	/* Get Multicast Information from the user
	 * FIXME: This option is not clearly defined
	 */
	priv_area->mc = conf->mc;

	
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
	
	/* Initialize the polling task routine 
	 * used to defer tasks from interrupt context.
	 * Also it is used to implement dynamic 
	 * interface configuration i.e. bringing interfaces
	 * up and down.
	 */

	WAN_TASKQ_INIT((&priv_area->poll_task),0,frmw_poll,priv_area);

#if 0
	/* Create interface file in proc fs.
	 * Once the proc file system is created, the new_if() function
	 * should exit successfuly.  
	 *
	 * DO NOT place code under this function that can return 
	 * anything else but 0.
	 */
	err = wanrouter_proc_add_interface(wandev, 
					   &priv_area->dent, 
					   priv_area->if_name, 
					   dev);
	if (err){	
		DEBUG_EVENT(
			"%s: Failed to create /proc/net/router/frmw/%s entry!\n",
			card->devname, priv_area->if_name);
		goto new_if_error;
	}
#endif
	
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
	wan_netif_set_priv(dev,priv_area);

	/* Increment the number of network interfaces 
	 * configured on this card.  
	 */
	wan_atomic_inc(&card->wandev.if_cnt);

	return 0;
	
new_if_error:

	if (priv_area->sar_pvc){
		wp_sar_unregister_pvc(card->u.atm.atm_device,
				      priv_area->sar_pvc,
				      priv_area->tx_cells_skb);
		priv_area->sar_pvc=NULL;
		priv_area->tx_cells_skb=NULL;
	}
	
	wan_free(priv_area);

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
 * NOTE: DO NOT deallocate dev->priv here! It will be
 *       done by the upper layer.
 * 
 */
static int del_if (wan_device_t* wandev, struct net_device* dev)
{
	private_area_t* 	priv_area = wan_netif_priv(dev);
	sdla_t*			card = wandev->priv;
	unsigned long		flags;

	wan_atomic_dec(&card->wandev.if_cnt);

	wan_spin_lock_irq(&card->wandev.lock,&flags);

	card->u.atm.tx_dev=NULL;
	
	if (priv_area->sar_pvc){
		wp_sar_unregister_pvc(card->u.atm.atm_device,
				      priv_area->sar_pvc,
				      priv_area->tx_cells_skb);

		priv_area->sar_pvc=NULL;
		priv_area->tx_cells_skb=NULL;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&flags);

#if 0
	/* Delete interface name from proc fs. */
 	wanrouter_proc_delete_interface(wandev, priv_area->if_name);
#endif

	/* Decrement the number of network interfaces 
	 * configured on this card.  
	 */
	if (wan_atomic_read(&card->wandev.if_cnt) <= 0){
		wan_del_timer(&card->u.atm.atm_timer);
	}

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
	private_area_t* priv_area = wan_netif_priv(dev);
	sdla_t* card = priv_area->card;
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
	if (priv_area->cfg.encap_mode == RFC_MODE_BRIDGED_ETH_LLC ||
	    priv_area->cfg.encap_mode == RFC_MODE_BRIDGED_ETH_VC){

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
		if (priv_area->mc == WANOPT_YES){
			dev->flags 	|= IFF_MULTICAST;
		}
		
		if (priv_area->true_if_encoding){
			dev->type	= ARPHRD_HDLC; /* This breaks the tcpdump */
		}else{
			dev->type	= ARPHRD_PPP;
		}
		
		dev->mtu		= card->wandev.mtu;
	 
		dev->hard_header_len	= 0;
		dev->tx_queue_len = 100;
	}
	
	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start);
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end);

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
	private_area_t* priv_area = wan_netif_priv(dev);
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

	priv_area->common.state=WAN_CONNECTED;

	if (card->open_cnt == 1){
		wan_add_timer(&card->u.atm.atm_timer,ATM_TIMER_TIMEOUT*HZ);	
	}

	if (!card->comm_enabled){
		card->timer_int_enabled |= TMR_INT_ENABLED_CONFIG;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
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
 * Any deallocation and cleanup can occur in del_if() 
 * function.  That function is called before the dev interface
 * itself is deallocated.
 *
 * Thus, we should only stop the net queue and decrement
 * the wanpipe usage counter via wanpipe_close() function.
 */

static int if_close (struct net_device* dev)
{
	private_area_t* priv_area = wan_netif_priv(dev);
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
	netskb_t *skb;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (card->comm_enabled){
		disable_comm_shutdown (card);
	}else{
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	/* Stop the ATM bh handler. We must do this
	 * outside the critical area because the kill
	 * function can sleep! */

	WAN_TASKLET_KILL(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task));	

	wan_del_timer(&card->u.atm.atm_timer);

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);

	/* Free up ATM receive buffers */
	while ((skb=wan_skb_dequeue(&card->u.atm.wp_rx_free_list)) != NULL){
		wan_skb_free(skb);
	}
	while ((skb=wan_skb_dequeue(&card->u.atm.wp_rx_used_list)) != NULL){
		wan_skb_free(skb);
	}
	while ((skb=wan_skb_dequeue(&card->u.atm.wp_rx_data_list)) != NULL){
		wan_skb_free(skb);
	}
	while ((skb=wan_skb_dequeue(&card->u.atm.wp_tx_prot_list)) != NULL){
		wan_skb_free(skb);
	}

	if (card->u.atm.atm_device){
		wp_sar_unregister_device(card->u.atm.atm_device);
		card->u.atm.atm_device=NULL;
	}

	if (card->u.atm.trace_info){
		wan_trace_purge(card->u.atm.trace_info);
		wan_free(card->u.atm.trace_info);
		card->u.atm.trace_info=NULL;
	}

	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
	}
	return;
}



/*============================================================================
 * if_tx_timeout
 *
 * Kernel networking stack calls this function in case
 * the interface has been stopped for TX_TIMEOUT seconds.
 *
 * This would occur if we lost TX interrupts or the
 * card has stopped working for some reason.  
 * 
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (struct net_device *dev)
{
    	private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	
	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	++chan->if_stats.collisions;

	DEBUG_EVENT( "%s: Transmit timed out on %s\n", card->devname,dev->name);
	netif_wake_queue (dev);
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
static int if_send (netskb_t* skb, struct net_device* dev)
{
	private_area_t *chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	unsigned long smp_flags;
	unsigned long smp_flags1;
	int err=0;

	smp_flags=0;
	smp_flags1=0;

	/* Mark interface as busy. The kernel will not
	 * attempt to send any more packets until we clear
	 * this condition */
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	netif_stop_queue(dev);
#endif
	
	if (skb == NULL){
		/* This should never happen. Just a sanity check.
		 */
		DEBUG_EVENT( "%s: interface %s got kicked!\n",
			card->devname, dev->name);

		WAN_NETIF_WAKE_QUEUE(dev);
		return 0;
	}

	/* Non 2.4 kernels used to call if_send() 
	 * after TX_TIMEOUT seconds have passed of interface 
	 * being busy. Same as if_tx_timeout() in 2.4 kernels */
#if defined(LINUX_2_1)
	if (dev->tbusy){

		/* If our device stays busy for at least 5 seconds then we will
		 * kick start the device by making dev->tbusy = 0.  We expect 
		 * that our device never stays busy more than 5 seconds. So this
		 * is only used as a last resort. 
		 */
		if((jiffies - chan->tick_counter) < (5 * HZ)) {
			return 1;
		}

		if_tx_timeout(dev);
	}
#endif

	/* PVC_PROT protocol is used by the API sockets
	 * to indicate to the driver that this packet
	 * is a custom RAW packet and no protocol checks
	 * should be done on it */
   	if (ntohs(skb->protocol) != htons(PVC_PROT)){

		/* check the udp packet type */
		if (wan_udp_pkt_type(card,wan_skb_data(skb)) == 0){
                        if(store_udp_mgmt_pkt(UDP_PKT_FRM_STACK, card, skb, dev,
                                chan)){
				card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
			}
			start_net_queue(dev);
			return 0;
		}

		/* check to see if the source IP address is a broadcast or */
		/* multicast IP address */
                if(chan->common.usedby == WANPIPE && chk_bcast_mcast_addr(card, dev, skb)){
			++chan->if_stats.tx_dropped;
			wan_skb_free(skb);
			start_net_queue(dev);
			return 0;
		}
        }

      	if(card->type != SDLA_S514){
		s508_lock(card,&smp_flags,&smp_flags1);
	} 

	if(card->wandev.state != WAN_CONNECTED){
       		++chan->if_stats.tx_dropped;
		start_net_queue(dev);

	}else if(!skb->protocol){
		/* Skb buffer without protocol should never
		 * occur in normal operation */
        	++chan->if_stats.tx_errors;
		start_net_queue(dev);
		
	}else {

		err=wanpipe_sar_tx(chan->sar_pvc, skb, (void*)chan->tx_cells_skb);
		err=0;
		if (err != 0){
			++chan->if_stats.tx_errors;
			start_net_queue(dev);
			goto if_send_exit_crit;
		}
		
		DEBUG_TX("IF_SEND Orig SKB=%d Tx Cells=%d Tx Cell Erro=%i \n",
				wan_skb_len(skb),wan_skb_len(chan->tx_cells_skb),
				wan_skb_len(chan->tx_cells_skb)%53);

		err=wp_handle_tx_packets(card,chan->tx_cells_skb);
		if (err) {
			/* Failed to send, mark queue as busy
			 * and let the stack retry */
			stop_net_queue(dev);
		}else{
			
			/* Send successful, update stats
			 * and mark queue as ready */
			++chan->if_stats.tx_packets;
                        chan->if_stats.tx_bytes += wan_skb_len(skb);
			start_net_queue(dev);
		
			wan_netif_set_ticks(dev, SYSTEM_TICKS);
		}
		
	}

if_send_exit_crit:
	
	/* If the queue is still stopped here, then we
	 * have failed to send! Turn on interrutps and
	 * return the skb buffer to the stack by
	 * exiting with non-zero value.  
	 *
	 * Otherwise, free the skb buffer and return 0 
	 */
	if (!(err=WAN_NETIF_QUEUE_STOPPED(dev))) {
		wan_skb_free(skb);
	}else{
		chan->tick_counter = jiffies;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
	}

	/* End of critical area for re-entry and for S508 card */
	if(card->type != SDLA_S514){
		s508_unlock(card,&smp_flags,&smp_flags1);
	}
	
	return err;
}


/*============================================================================
 * chk_bcast_mcast_addr - Check for source broadcast addresses
 *
 * Check to see if the packet to be transmitted contains a broadcast or
 * multicast source IP address.
 */

static int chk_bcast_mcast_addr(sdla_t *card, struct net_device* dev,
				netskb_t *skb)
{
	u32 src_ip_addr;
        u32 broadcast_ip_addr = 0;
	private_area_t *priv_area=wan_netif_priv(dev);
        struct in_device *in_dev;
        /* read the IP source address from the outgoing packet */
        src_ip_addr = *(u32 *)(wan_skb_data(skb) + 12);

	if (priv_area->common.usedby != WANPIPE){
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
                DEBUG_EVENT( "%s: Broadcast Source Address silently discarded\n",
				card->devname);
                return 1;
        } 

        /* check if the IP Source Address is a Multicast address */
        if((ntohl(src_ip_addr) >= 0xE0000001) &&
		(ntohl(src_ip_addr) <= 0xFFFFFFFE)) {
                DEBUG_EVENT( "%s: Multicast Source Address silently discarded\n",
				card->devname);
                return 1;
        }

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

	if ((priv_area=wan_netif_priv(dev)) == NULL)
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
	private_area_t* chan= (private_area_t*)wan_netif_priv(dev);
	unsigned long smp_flags;
	wan_udp_pkt_t *wan_udp_pkt;
	sdla_t *card;
	
	if (!chan){
		return -ENODEV;
	}
	card=chan->card;

	NET_ADMIN_CHECK();

	switch(cmd)
	{
		case SIOC_WANPIPE_PIPEMON: 
			
			if (wan_atomic_read(&chan->udp_pkt_len) != 0){
				return -EBUSY;
			}
	
			wan_atomic_set(&chan->udp_pkt_len,MAX_LGTH_UDP_MGNT_PKT);
			
			wan_udp_pkt=(wan_udp_pkt_t*)chan->udp_pkt_data;
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (wan_test_bit(0,&card->in_isr)) {
				DEBUG_EVENT( "%s:%s Pipemon command failed, Driver busy: try again.\n",
						card->devname,dev->name);
				wan_atomic_set(&chan->udp_pkt_len,0);
				wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
				return -EBUSY;
			}
			
			process_udp_mgmt_pkt(card,dev,chan,1);
		
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */
			
			if (wan_atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				DEBUG_ERROR( "%s: Error: Pipemon buf too bit on the way up! %i\n",
						card->devname,wan_atomic_read(&chan->udp_pkt_len));
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EINVAL;
			}

			if (copy_to_user(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}
			
			wan_atomic_set(&chan->udp_pkt_len,0);
			return 0;
	
			
		default:
			return -EOPNOTSUPP;
	}
	return 0;
}





/**SECTION**********************************************************
 *
 * 	FIRMWARE Specific Interface Functions 
 *
 *******************************************************************/

static int init_interrupt_status_ptrs(sdla_t *card)
{
	wan_mbox_t *mb1=&card->wan_mbox;
	int err;

	/* Set up the interrupt status area */
	/* Read the Configuration and obtain: 
	 *	Ptr to shared memory infor struct
         * Use this pointer to calculate the value of get_card_flags(card) !
 	 */
	mb1->wan_data_len = 0;
	mb1->wan_command = READ_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb1);
	if(err != COMMAND_OK) {
		frmw_error(card, err, mb1);
		return -EIO;
	}

	/* Alex Apr 8 2004 Sangom ISA card */
	card->flags_off = ((PHY_CONFIGURATION_STRUCT *)mb1->wan_data)->
			           ptr_shared_mem_info_struct;
	return 0;
}



/*============================================================
 * set_adapter_config
 * 
 * Set adapter config configures the firmware for
 * the specific front end hardware.  
 * Front end hardware: T1/E1/56K/V32/RS232
 *
 */ 

static int set_adapter_config (sdla_t* card)
{
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
		frmw_error(card,err,mb);
	}
	return (err);
}


/*============================================================================
 * frmw_read_version
 * 
 * Read firmware code version.
 * Put code version as ASCII string in str. 
 */
static int frmw_read_version (sdla_t* card, char* str)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int len;
	char err;
	mb->wan_data_len = 0;
	mb->wan_command = READ_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		frmw_error(card,err,mb);
	}
	else if (str) {  /* is not null */
		len = mb->wan_data_len;
		memcpy(str, mb->wan_data, len);
		str[len] = '\0';
	}
	return (err);
}

/*-----------------------------------------------------------------------------
 *  Configure firmware.
 */
static int frmw_configure (sdla_t* card, void* data)
{
	int err;
	wan_mbox_t *mb = &card->wan_mbox;
	int data_length = sizeof(PHY_CONFIGURATION_STRUCT);
	
	mb->wan_data_len = data_length;  
	memcpy(mb->wan_data, data, data_length);
	mb->wan_command = SET_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
		
	if (err != COMMAND_OK) frmw_error (card, err, mb);
                           
	return err;
}


/*============================================================================
 * Set interrupt mode -- HDLC Version.
 */

static int frmw_set_intr_mode (sdla_t* card, unsigned mode)
{
	wan_mbox_t* mb = &card->wan_mbox;
	INT_TRIGGERS_STRUCT* int_data =
		 (INT_TRIGGERS_STRUCT *)mb->wan_data;
	int err;

	int_data->interrupt_triggers 	= mode;
	card->hw_iface.getcfg(card->hw, SDLA_IRQ, &int_data->IRQ);
	int_data->interrupt_timer       = 1;
   
	mb->wan_data_len = sizeof(INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		frmw_error (card, err, mb);
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
	INT_TRIGGERS_STRUCT* int_data =
		 (INT_TRIGGERS_STRUCT *)mb->wan_data;
	int err;

	/* Disable Interrutps */
	int_data->interrupt_triggers 	= 0;
	card->hw_iface.getcfg(card->hw, SDLA_IRQ, &int_data->IRQ);
	int_data->interrupt_timer	= 1;
   
	mb->wan_data_len = sizeof(INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	/* Disable Communications */

	mb->wan_command = DISABLE_COMMUNICATIONS;
	
	mb->wan_data_len = 0;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	card->comm_enabled = 0;

	return 0;
}

/*============================================================================
 * Enable communications.
 */

static int frmw_comm_enable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		frmw_error(card, err, mb);
	else
		card->comm_enabled = 1;
	
	return err;
}

/*============================================================================
 * Read communication error statistics.
 */
static int frmw_read_comm_err_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_COMMS_ERROR_STATS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                frmw_error(card,err,mb);
        return err;
}


/*============================================================================
 * Read operational statistics.
 */
static int frmw_read_op_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_OPERATIONAL_STATS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                frmw_error(card,err,mb);
        return err;
}


/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card, private_area_t* priv_area)
{
        wan_mbox_t* mb = &card->wan_mbox;
  	COMMS_ERROR_STATS_STRUCT* err_stats;
        OPERATIONAL_STATS_STRUCT* op_stats;

	if(card->update_comms_stats == 3) {
		/* 1. On the first timer interrupt, update T1/E1 alarms 
		 * and PMON counters (only for T1/E1 card) (TE1) 
		 */
		/* TE1 Update T1/E1 alarms */
		if (IS_TE1_CARD(card)) {	
			card->wandev.fe_iface.read_alarm(&card->fe, 0); 
			/* TE1 Update T1/E1 perfomance counters */
			card->wandev.fe_iface.read_pmon(&card->fe, 0); 

		}else if (IS_56K_CARD(card)) {
			/* 56K Update CSU/DSU alarms */
			card->wandev.fe_iface.read_alarm(&card->fe, 0); 
		}
	 } else { 
		/* 2. On the second timer interrupt, read the comms error 
	 	 * statistics 
	 	 */
		if(card->update_comms_stats == 2) {
			if(frmw_read_comm_err_stats(card))
				return 1;
			
			err_stats = (COMMS_ERROR_STATS_STRUCT *)mb->wan_data;
			card->wandev.stats.rx_over_errors = 
				err_stats->Rx_overrun_err_count;
			
		} else {

        		/* on the third timer interrupt, read the operational 
			 * statistics 
		 	 */
        		if(frmw_read_op_stats(card))
                		return 1;
			op_stats = (OPERATIONAL_STATS_STRUCT *)mb->wan_data;
			card->wandev.stats.rx_length_errors =
				op_stats->Rx_blocks_discard_count;
		
			card->wandev.stats.rx_crc_errors = 
				op_stats->Rx_bad_HEC_count;
			
			card->wandev.stats.rx_frame_errors = 
				op_stats->Rx_sync_failure_count;
			
			card->wandev.stats.rx_fifo_errors = 
				op_stats->Rx_hunt_timeout_count; 
			
			card->wandev.stats.rx_missed_errors =
				op_stats->Rx_resync_reception_loss_count;
			
			card->wandev.stats.tx_aborted_errors =
				op_stats->Tx_underrun_cell_count+
				op_stats->Tx_length_error_count;

		
		}
	}

	return 0;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int frmw_send (sdla_t* card, void* data, unsigned len, unsigned char tx_bits)
{
	DATA_TX_STATUS_EL_STRUCT txbuf;
	card->hw_iface.peek(card->hw, card->u.atm.txbuf_off, &txbuf, sizeof(txbuf));
	if (txbuf.opp_flag){
		return 1;
	}

	card->hw_iface.poke(card->hw, txbuf.ptr_data_bfr, data, len);

	txbuf.block_length = len;
	txbuf.misc_Tx_bits = tx_bits;
	txbuf.opp_flag = 1;		/* start transmission */
	card->hw_iface.poke(card->hw, card->u.atm.txbuf_off, &txbuf, sizeof(txbuf));
	
	/* Update transmit buffer control fields */
	card->u.atm.txbuf_off += sizeof(DATA_TX_STATUS_EL_STRUCT);
	if (card->u.atm.txbuf_off > card->u.atm.txbuf_last_off){
		card->u.atm.txbuf_off = card->u.atm.txbuf_base_off;
	}
	
	return 0;
}

/*============================================================================
 * Enable timer interrupt  
 */
static void enable_timer (void* card_id)
{
	sdla_t		*card = (sdla_t*)card_id;
	netdevice_t	*dev;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	DEBUG_EVENT( "%s: enabling timer %s\n",card->devname,
			dev ? dev->name:"No DEV");
		
	card->timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	return;
}

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 *
 * WARNING: This function might be called in critical region.
 *          Thus no waiting should be performed here
 */
static int frmw_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

	default:
		DEBUG_EVENT( "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}


/*===============================================================
 * set_frmw_config() 
 *
 * used to set configuration options on the board
 *
 */

static int set_frmw_config(sdla_t* card)
{
	PHY_CONFIGURATION_STRUCT cfg;

	memset(&cfg, 0, sizeof(PHY_CONFIGURATION_STRUCT));

	if(card->wandev.clocking){
		cfg.baud_rate = card->wandev.bps;
	}
		
	cfg.line_config_options = (card->wandev.electrical_interface == WANOPT_RS232) ?
		INTERFACE_LEVEL_RS232 : INTERFACE_LEVEL_V35;

	/* Automatic DTR/RTS and notify modem status changes */
	cfg.modem_config_options	= 0;
	
	cfg.modem_status_timer		= 10;

	/* Cells are passed transparently between the firmware
	 * and driver.  This allows the transfer of multiple cells
	 * in a single block */
	cfg.API_options			= PHY_TRANSPARENT_TX_RX_CELLS | PHY_APP_REVERSES_BIT_ORDER;
		
	cfg.protocol_options		= PHY_UNI;
	
	cfg.HEC_options			= PHY_DISABLE_RX_HEC_CHECK;

	cfg.custom_Rx_COSET		= 0;
	cfg.custom_Tx_COSET		= 0;
	
	cfg.buffer_options		= 0;

	/* Maximum number of cells that may be transmitted
	 * by the application in a single block */
	cfg.max_cells_in_Tx_block	= PHY_MAX_CELLS_IN_TX_BLOCK;

	cfg.Tx_underrun_cell_GFC	= card->u.atm.atm_cfg.atm_cell_cfg;
	cfg.Tx_underrun_cell_PT		= card->u.atm.atm_cfg.atm_cell_pt;
	cfg.Tx_underrun_cell_CLP	= card->u.atm.atm_cfg.atm_cell_clp;
	cfg.Tx_underrun_cell_payload	= card->u.atm.atm_cfg.atm_cell_payload;

	DEBUG_EVENT("%s: Configuring ATM Cells: GFC=0x%X PT=0x%X CLP=0x%X Payload=0x%X\n",
			card->devname,
			cfg.Tx_underrun_cell_GFC,
			cfg.Tx_underrun_cell_PT,
			cfg.Tx_underrun_cell_CLP,
			cfg.Tx_underrun_cell_payload);
	
	memcpy(&phy_idle_cfg, &cfg, sizeof(PHY_CONFIGURATION_STRUCT));
	
	cfg.number_cells_Tx_underrun	= PHY_MAX_CELLS_TX_UNDERRUN;
	
	cfg.Rx_hunt_timer		= 100;			
	cfg.Rx_hunt_timer		= card->u.atm.atm_cfg.atm_hunt_timer;

	cfg.Rx_sync_bytes		= 0x0152;	
	cfg.Rx_sync_offset		= 0x03;		

	if (card->u.atm.atm_cfg.atm_sync_mode){
		cfg.protocol_options |= PHY_MANUAL_RX_SYNC;
	}
	
	cfg.Rx_sync_bytes = card->u.atm.atm_cfg.atm_sync_data;
	cfg.Rx_sync_offset = card->u.atm.atm_cfg.atm_sync_offset;

	
	DEBUG_EVENT("%s: Configuring ATM Sync: %s bytes=0x%04X offset=%i hunt=%i\n",
			card->devname,
			card->u.atm.atm_cfg.atm_sync_mode?"Manual":"Auto",
			cfg.Rx_sync_bytes,
			cfg.Rx_sync_offset,
			cfg.Rx_hunt_timer);
	
	cfg.max_cells_in_Rx_block	= PHY_MAX_CELLS_IN_RX_BLOCK; 
	cfg.cell_Rx_sync_loss_timer	= 500;	
	cfg.Rx_HEC_check_timer		= 100;
	cfg.Rx_bad_HEC_timer		= 100;
	cfg.Rx_max_bad_HEC_count	= 10;
	cfg.number_cells_Rx_discard	= PHY_MAX_CELLS_RX_DISCARD;
	
	cfg.statistics_options		= (PHY_TX_BYTE_COUNT_STAT | 
			                   PHY_RX_BYTE_COUNT_STAT |	
					   PHY_TX_THROUGHPUT_STAT |
					   PHY_RX_THROUGHPUT_STAT | 
					   PHY_INCL_UNDERRUN_TX_THRUPUT |
					   PHY_INCL_DISC_RX_THRUPUT)	;	

	return frmw_configure(card, &cfg);
}

/*============================================================================
 * init_tx_rx_buff
 *
 * Initialize hardware Receive and Transmit Buffers.
 *
 */

static void init_tx_rx_buff( sdla_t* card)
{
	netdevice_t	*dev;
	wan_mbox_t* mb = &card->wan_mbox;
	unsigned long tx_config_off;
	unsigned long rx_config_off;
	TX_STATUS_EL_CFG_STRUCT tx_config;
	RX_STATUS_EL_CFG_STRUCT rx_config;
	char err;
	
	mb->wan_data_len = 0;
	mb->wan_command = READ_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			frmw_error(card,err,mb);
		}
		return;
	}

	/* Alex Apr 8 2004 Sangom ISA card */
	tx_config_off = ((PHY_CONFIGURATION_STRUCT *)mb->wan_data)->
                         		ptr_Tx_stat_el_cfg_struct;
	card->hw_iface.peek(card->hw, tx_config_off, &tx_config, sizeof(tx_config));
       	rx_config_off = ((PHY_CONFIGURATION_STRUCT *)mb->wan_data)->
                            		ptr_Rx_stat_el_cfg_struct;
	card->hw_iface.peek(card->hw, rx_config_off, &rx_config, sizeof(rx_config));

       	/* Setup Head and Tails for buffers */
       	card->u.atm.txbuf_base_off = tx_config.base_addr_Tx_status_els;
       	card->u.atm.txbuf_last_off = 
               		card->u.atm.txbuf_base_off +
			(tx_config.number_Tx_status_els - 1) * sizeof(DATA_TX_STATUS_EL_STRUCT);

       	card->u.atm.rxbuf_base_off = rx_config.base_addr_Rx_status_els;
       	card->u.atm.rxbuf_last_off = 
               		card->u.atm.rxbuf_base_off +
			(rx_config.number_Rx_status_els - 1) * sizeof(DATA_RX_STATUS_EL_STRUCT);

 	/* Set up next pointer to be used */
       	card->u.atm.txbuf_off = tx_config.next_Tx_status_el_to_use;
	card->u.atm.rxmb_off = rx_config.next_Rx_status_el_to_use;
        return;
}

/*=============================================================================
 * intr_test
 * 
 * Perform Interrupt Test by running READ_CODE_VERSION command MAX_INTR
 * _TEST_COUNTER times.
 * 
 */
static int intr_test( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err,i;
	
	Intr_test_counter = 0;
	
	err = frmw_set_intr_mode(card, APP_INT_ON_COMMAND_COMPLETE);

	if (err == CMD_OK) { 
		for (i = 0; i < MAX_INTR_TEST_COUNTER; i ++) {	
			mb->wan_data_len  = 0;
			mb->wan_command = READ_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != CMD_OK) 
				frmw_error(card, err, mb);
		}
	}
	else {
		return err;
	}
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	err = frmw_set_intr_mode(card, 0);

	if (err != CMD_OK)
		return err;

	return 0;
}





/**SECTION************************************************** 
 *
 * 	Bottom Half Handlers 
 *
 **********************************************************/

static void wp_bh (unsigned long data)
{
	netdevice_t	*dev;
	sdla_t *card = (sdla_t*)data;
	netskb_t *skb;
	int err=0;

	if (wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical)){
		DEBUG_EVENT("%s: WpBH PERI Critical\n",
				card->devname);
		WAN_TASKLET_END(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task));
		return;
	}

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!card->u.atm.atm_device || !dev){
		DEBUG_EVENT("%s: WpBH Dev done\n",
				card->devname);
		WAN_TASKLET_END(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task));
		return;
	}

	while ((skb=wan_skb_dequeue(&card->u.atm.wp_rx_used_list))){

		err=wanpipe_sar_rx(card->u.atm.atm_device,skb);
		
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		skb->len=1;
		wan_skb_trim(skb,0);

		wan_skb_queue_tail(&card->u.atm.wp_rx_free_list,skb);
		
		if (err < 0 && card->wandev.state==WAN_CONNECTED){
			/* We are out of sync */
			wp_handle_out_of_sync_condition(card,0);
			goto wp_bh_exit;
		}
	}	

	while((skb=wan_skb_dequeue(&card->u.atm.wp_rx_data_list))){
		wp_handle_rx_packets(skb);
	}	

	while((skb=wan_skb_dequeue(&card->u.atm.wp_tx_prot_list))){
		err=wp_handle_tx_packets(card,skb);
		if (err){

			wan_skb_queue_head(&card->u.atm.wp_tx_prot_list,skb);
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
			break;
		}
		wan_skb_free(skb);
	}

wp_bh_exit:
	WAN_TASKLET_END(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task));
	return;
}

static int wp_handle_out_of_sync_condition(sdla_t *card, unsigned char irq)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;
	netskb_t* skb;
	unsigned long flags=0;

	/* We should only try to re-sync if we are in
	 * CONNECTED state, otherwise we are probably
	 * in re-sync state already */
	if (card->u.atm.state != WAN_CONNECTED){
		return 0;
	}
	
	DEBUG_EVENT("%s: ATM PHY out of sync, re-syncing!\n",
			card->devname);

	if (!irq){
		wan_spin_lock_irq(&card->wandev.lock,&flags);
	}
	
	mb->wan_data_len = 0;
	mb->wan_command = PHY_RESYNCHRONIZE_RECEIVER;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != CMD_OK){ 
		frmw_error(card, err, mb);
		DEBUG_ERROR("%s: Error: Failed to re-syncing adapter !\n",
			card->devname);

		/* We failed to resync for some reason, get out
		 * and let the rx sar trigger the re-sync again */
		if (!irq){
			wan_spin_unlock_irq(&card->wandev.lock,&flags);
		}
		return -1;
	}
	port_set_state(card, WAN_DISCONNECTED);


	while ((skb=wan_skb_dequeue(&card->u.atm.wp_rx_used_list))){
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		skb->len=1;
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&card->u.atm.wp_rx_free_list,skb);
	}	

	wan_skb_queue_purge(&card->u.atm.wp_rx_data_list);
	wan_skb_queue_purge(&card->u.atm.wp_tx_prot_list);

	if (!irq){
		wan_spin_unlock_irq(&card->wandev.lock,&flags);
	}

	return 0;
}

static int wp_handle_tx_packets(sdla_t* card, netskb_t *skb)
{
	void *data = wan_skb_data(skb);
	unsigned long len =  wan_skb_len(skb);
	int err;
	unsigned long flags;

	wan_spin_lock_irq(&card->wandev.lock,&flags);
	err=frmw_send(card, data, len, 0);
	wan_spin_unlock_irq(&card->wandev.lock,&flags);

	return err;
}

static void wp_handle_rx_packets(netskb_t *skb)
{
	private_area_t *chan = (private_area_t *)skb->dev;
	sdla_t *card;
	
	if (!chan){
		DEBUG_ERROR("%s:%d Error, Rx packet has no dev pointer (skb->dev==NULL)\n",
				__FUNCTION__,__LINE__);
		wan_skb_free(skb);
		return;
	}

	card=chan->card;
	skb->dev=chan->dev;
	
	switch (chan->cfg.encap_mode){
	case RFC_MODE_BRIDGED_ETH_LLC:
	case RFC_MODE_BRIDGED_ETH_VC:
		skb->protocol = eth_type_trans(skb, skb->dev);
		break;
	case RFC_MODE_ROUTED_IP_LLC:
	case RFC_MODE_ROUTED_IP_VC:	
		skb->protocol = htons(ETH_P_IP);
		wan_skb_reset_mac_header(skb);
		break;
	}

	++chan->if_stats.rx_packets;
	chan->if_stats.rx_bytes += skb->len;

	netif_rx(skb);
}


/**SECTION*************************************************************** 
 * 
 * 	HARDWARE Interrupt Handlers 
 *
 ***********************************************************************/


/*============================================================================
 * wpfw_isr
 *
 * Main interrupt service routine. 
 * Determin the interrupt received and handle it.
 *
 */
static WAN_IRQ_RETVAL wpatm_isr (sdla_t* card)
{
	SHARED_MEMORY_INFO_STRUCT flags;
	int i;

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}
	
	wan_set_bit(0,&card->in_isr);
	
	/* if critical due to peripheral operations
	 * ie. update() or getstats() then reset the interrupt and
	 * wait for the board to retrigger.
	 */
	if(wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		DEBUG_EVENT( "%s: ISR:  Critical with PERI_CRIT!\n",
				card->devname);
		goto isr_done;
	}

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	switch(flags.interrupt_info_struct.interrupt_type) {

	case RX_APP_INT_PEND:	/* 0x01: receive interrupt */
		rx_intr(card);
		break;

	case TX_APP_INT_PEND:	/* 0x02: transmit interrupt */
		tx_intr(card);
		break;

	case COMMAND_COMPLETE_APP_INT_PEND:/* 0x04: cmd complete */
		++ Intr_test_counter;
		break;

	case EXCEP_COND_APP_INT_PEND:	/* 0x20: Exception */
		process_exception(card);
		break;

	case GLOBAL_EXCEP_COND_APP_INT_PEND:  
		process_global_exception(card);
		
		/* Reset the 56k or T1/E1 front end exception condition */
		if(IS_56K_CARD(card) || IS_TE1_CARD(card)) {
			card->hw_iface.poke_byte(card->hw, card->fe_status_off, 0x01);
		}
		break;

	case TIMER_APP_INT_PEND:	/* Timer interrupt */
		timer_intr(card);
		break;

	default:
		DEBUG_EVENT( "%s: spurious interrupt 0x%02X!\n", 
			card->devname,
			flags.interrupt_info_struct.interrupt_type);
		DEBUG_EVENT( "Code name: ");
		for(i = 0; i < 4; i ++){
			printk("%c",
				flags.global_info_struct.code_name[i]); 
		}
		printk("\n");
		DEBUG_EVENT( "Code version: ");
	 	for(i = 0; i < 4; i ++){
			printk("%c", 
				flags.global_info_struct.code_version[i]); 
		}
		printk("\n");	
		break;
	}

isr_done:

	wan_clear_bit(0,&card->in_isr);
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}


/*===============================================================
 * tx_intr
 *  
 * 	Transmit interrupt handler.
 *	For each dev, check that there is something to send.
 *	If data available, transmit. 	
 *
 *===============================================================*/

static void tx_intr (sdla_t* card)
{
	netdevice_t *dev;
	private_area_t *chan=NULL;
	unsigned char dev_kicked=0, more_to_tx=0;
	int i=0;	

	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);

	if (card->u.atm.tx_dev == NULL){
		card->u.atm.tx_dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	}

	dev = card->u.atm.tx_dev;

	for (;;){

		if (!WAN_NETIF_UP(dev)){	
			goto tx_intr_dev_skip;
		}
		
		chan = wan_netif_priv(dev);
		if (WAN_NETIF_QUEUE_STOPPED(dev)){

			if (dev_kicked){
				more_to_tx=1;
				break;
			}
			
			if (chan->common.usedby == API){
				WAN_NETIF_START_QUEUE(dev);
				wan_wakeup_api(chan);
			}else{
				WAN_NETIF_WAKE_QUEUE(dev);
			}

			dev_kicked=1;
		}

tx_intr_dev_skip:
		dev = move_dev_to_next(card,dev);
		if (++i >= wan_atomic_read(&card->wandev.if_cnt)){
			break;
		}

	}//End of FOR

	card->u.atm.tx_dev = dev;

	if (more_to_tx){
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
	}
	
	if (wan_skb_queue_len(&card->u.atm.wp_tx_prot_list)){
		WAN_TASKLET_SCHEDULE(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task));	
	}

	return;
}

/*===============================================================
 * move_dev_to_next
 *  
 *
 *===============================================================*/


static netdevice_t * move_dev_to_next (sdla_t *card, netdevice_t *dev)
{
	struct wan_dev_le	*devle;
	if (wan_atomic_read(&card->wandev.if_cnt) == 1){
		return dev;
	}

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		if (devle->dev == dev){
			dev = WAN_DEVLE2DEV(WAN_LIST_NEXT(devle, dev_link));
			if (!dev){
				dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			}
			return dev;
		}
	}
	return dev;
}



/*============================================================================
 * rx_intr
 *
 * Receive interrupt handler.
 */
static void rx_intr (sdla_t* card)
{
	SHARED_MEMORY_INFO_STRUCT flags;
	DATA_RX_STATUS_EL_STRUCT rxbuf;
	unsigned long addr;
	netskb_t *skb;
	unsigned len;
	void *buf;
	int i;

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	card->hw_iface.peek(card->hw, card->u.atm.rxmb_off, &rxbuf, sizeof(rxbuf));
	addr = rxbuf.ptr_data_bfr;
	if (rxbuf.opp_flag != 0x01) {
		DEBUG_EVENT( 
			"%s: corrupted Rx buffer @ 0x%lX, flag = 0x%02X!\n", 
			card->devname, card->u.atm.rxmb_off, rxbuf.opp_flag);
                DEBUG_EVENT( "Code name: ");
                for(i = 0; i < 4; i ++)
                        DEBUG_EVENT( "%c",
                                flags.global_info_struct.code_name[i]);
		DEBUG_EVENT( "\n");
                DEBUG_EVENT( "Code version: ");
                for(i = 0; i < 4; i ++)
                        DEBUG_EVENT( "%c",
                                flags.global_info_struct.code_version[i]);
                DEBUG_EVENT( "\n");


		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it measn that driver 
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash. 
                 */
		DEBUG_EVENT( "%s: Critical router failure ...!!!\n", card->devname);
		DEBUG_EVENT( "Please contact Sangoma Technologies !\n");
		frmw_set_intr_mode(card,0);	
		return;
	}

	if (card->wandev.state != WAN_CONNECTED){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}
	
	len  = rxbuf.block_length;

	/* Get a free/unused socket buffer from the ATM
	 * rx pool. */
	skb = wan_skb_dequeue(&card->u.atm.wp_rx_free_list);
	if (skb == NULL) {
		DEBUG_EVENT( "%s: No ATM rx buffers available!\n",
					card->devname);
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

	buf = wan_skb_put(skb, len);
	card->hw_iface.peek(card->hw, addr, buf, len);

	wan_skb_queue_tail(&card->u.atm.wp_rx_used_list,skb);

	WAN_TASKLET_SCHEDULE(((wan_tasklet_t*)&card->u.atm.wanpipe_rx_task));

rx_exit:
	/* Release buffer element and calculate a pointer to the next one */
	rxbuf.opp_flag = 0x00;
	card->hw_iface.poke(card->hw, card->u.atm.rxmb_off, &rxbuf, sizeof(rxbuf));
	card->u.atm.rxmb_off += sizeof(rxbuf);
	if(card->u.atm.rxmb_off > card->u.atm.rxbuf_last_off){
		card->u.atm.rxmb_off = card->u.atm.rxbuf_base_off;
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
        struct net_device* dev=NULL;
        private_area_t* priv_area = NULL;
	/* TE timer interrupt */
	if (card->timer_int_enabled & TMR_INT_ENABLED_TE) {
		card->wandev.fe_iface.polling(&card->fe);
		card->timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}

	/* Configure hardware */
	if (card->timer_int_enabled & TMR_INT_ENABLED_CONFIG) {
		config_frmw(card);
		card->timer_int_enabled &= ~TMR_INT_ENABLED_CONFIG;
	}

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
        if (dev == NULL){
		card->timer_int_enabled=0;
		goto timer_isr_exit;
	}
	
        priv_area = wan_netif_priv(dev);

	/* process a udp call if pending */
       	if(card->timer_int_enabled & TMR_INT_ENABLED_UDP) {
               	process_udp_mgmt_pkt(card, dev,
                       priv_area,0);
		card->timer_int_enabled &= ~TMR_INT_ENABLED_UDP;
        }

	/* read the communications statistics if required */
	if(card->timer_int_enabled & TMR_INT_ENABLED_UPDATE) {
		update_comms_stats(card, priv_area);
                if(!(-- card->update_comms_stats)) {
			card->timer_int_enabled &= 
				~TMR_INT_ENABLED_UPDATE;
		}
        }

timer_isr_exit:

	/* only disable the timer interrupt if there are no udp or statistic */
	/* updates pending */
        if(!card->timer_int_enabled) {
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
        }
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

				handle_front_end_state(card);
				break;
			
			}
			
			if (IS_TE1_CARD(card)) {
				/* TE1 T1/E1 interrupt */
				card->wandev.fe_iface.isr(&card->fe);
				handle_front_end_state(card);
				break;
			}	

			if ((mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) == (DCD_HIGH | CTS_HIGH)){
				card->fe.fe_status = FE_CONNECTED;
			}else{
				card->fe.fe_status = FE_DISCONNECTED;
			}
			
			DEBUG_EVENT( "%s: Modem status change\n",
				card->devname);

			switch(mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) {
				
				case ((DCD_HIGH | CTS_HIGH)):
                                        DEBUG_EVENT( "%s: DCD high, CTS high\n",card->devname);
                                        break;
				
				case (DCD_HIGH):
					DEBUG_EVENT( "%s: DCD high, CTS low\n",card->devname);
					break;
					
				case (CTS_HIGH):
                                        DEBUG_EVENT( "%s: DCD low, CTS high\n",card->devname); 
					break;
				
				default:
                                        DEBUG_EVENT( "%s: DCD low, CTS low\n",card->devname);
                                        break;
			}

			handle_front_end_state(card);
			break;

		case EXCEP_IRQ_TIMEOUT:
			DEBUG_EVENT( "%s: IRQ timeout occurred\n",
				card->devname); 
			break;

                default:
                        DEBUG_EVENT( "%s: Global exception %x\n",
				card->devname, mb->wan_return_code);
                        break;
                }
	}
	return 0;
}

static void phy_sync_state_change(sdla_t *card, PHY_RX_SYNC_EXCEP_STRUCT *rx_sync)
{
	switch (rx_sync->Rx_sync_status){

	case PHY_RX_SYNC_LOST:
		DEBUG_EVENT("%s: Phy state change: Rx sync lost \n",card->devname);
		goto state_ch_disconnected;
			
	case PHY_RX_HUNT:
		DEBUG_EVENT("%s: Phy state change: Rx hunt \n",card->devname);
		goto state_ch_disconnected;
		
	case PHY_RX_PRESYNC:
		DEBUG_EVENT("%s: Phy state change: Rx presync \n",card->devname);
		goto state_ch_disconnected;
		
	case PHY_RX_SYNCHRONIZED:
		DEBUG_EVENT("%s: Phy state change: Rx sync successful \n",card->devname);
		card->u.atm.state=WAN_CONNECTED;
		if (card->fe.fe_status == FE_CONNECTED){
			port_set_state(card, WAN_CONNECTED);
		}
		break;
		
	default:
		DEBUG_EVENT("%s: Unknown phy state change 0x%x\n",
				card->devname, rx_sync->Rx_sync_status);
		goto state_ch_disconnected;
	}

	return;
	
state_ch_disconnected:
	card->u.atm.state=WAN_DISCONNECTED;
	port_set_state(card, WAN_DISCONNECTED);
	return;
}


/*============================================================================
 * Process  exception condition
 */
static int process_exception(sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_data_len = 0;
	mb->wan_command = READ_EXCEPTION_CONDITION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err == CMD_TIMEOUT) {
		return 0;
	}
	
	switch (err) {

		case PHY_EXCEP_RX_SYNC_STATE_CHANGE:	
			/* the PHY receiver has changed state */
			phy_sync_state_change(card,(PHY_RX_SYNC_EXCEP_STRUCT *)mb->wan_data);	
			break;
			
		case PHY_EXCEP_INVALID_HEC: 
			/* the Rx consecutive incorrect HEC counter has expired */
			DEBUG_EVENT("%s: PHY Exception: Rx consecutive invalid HEC\n",
					card->devname); 	
			break;
		case PHY_EXCEP_RECEP_LOSS:  
			/* the cell reception sync loss timer has expired */
			DEBUG_EVENT("%s: PHY Exception: Cell rx sync loss timer expired\n",
					card->devname);
			break;
		case PHY_EXCEP_RX_DISCARD:
			/* incoming cells were discarded */
			DEBUG_EVENT("%s: PHY Exception: Incoming cells were discarded\n",
					card->devname);
			break;
		case PHY_EXCEP_TX_LENGTH_ERROR:
			/* a transmit buffer of invalid length was detected */
			DEBUG_EVENT("%s: PHY Exception: Tx buffer of invalid length detected\n",
					card->devname);
			break;


		default:
			DEBUG_EVENT("%s: PHY Exception: Unknown 0x%x\n",
					card->devname, err);

			break;
	}
	return 0;
}




/**SECTION***********************************************************
 * 
 * WANPIPE Debugging Interfaces
 *
 ********************************************************************/



/*=============================================================================
 * store_udp_mgmt_pkt
 *
 * Store a UDP management packet for later processing.
 *
 * When isr or tx task receives a UDP debug packet from 
 * wanpipemon utility, or from the remote network, the 
 * debug process must be defered to a kernel task 
 * which will execute the debug command at a later time. 
 *
 */

static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                netskb_t *skb, struct net_device* dev,
                                private_area_t* priv_area )
{
	int udp_pkt_stored = 0;

	if(!wan_atomic_read(&priv_area->udp_pkt_len) &&
	  (skb->len <= MAX_LGTH_UDP_MGNT_PKT)) {
		wan_atomic_set(&priv_area->udp_pkt_len, skb->len);
		priv_area->udp_pkt_src = udp_pkt_src;
       		memcpy(priv_area->udp_pkt_data, skb->data, skb->len);
		card->timer_int_enabled = TMR_INT_ENABLED_UDP;
		udp_pkt_stored = 1;
	}

	wan_skb_free(skb);
		
	return(udp_pkt_stored);
}

#define ATM_HEADER_SZ 		5
#define ATM_PAYLOAD_SZ		48
typedef struct wp_atm_cell
{
	unsigned char hdr[ATM_HEADER_SZ];      /* ATM header */
	unsigned char info[ATM_PAYLOAD_SZ];    /* ATM payload */
} wp_atm_cell_t;


/*=============================================================================
 * process_udp_mgmt_pkt
 * 
 * Process all "wanpipemon" debugger commands.  This function
 * performs all debugging tasks: 
 *
 * 	Line Tracing
 * 	Line/Hardware Statistics
 * 	Protocol Statistics
 * 
 * "wanpipemon" utility is a user-space program that 
 * is used to debug the WANPIPE product.
 *
 */

static int process_udp_mgmt_pkt(sdla_t* card, struct net_device* dev,
				private_area_t* priv_area, int local_dev ) 
{
	unsigned short buffer_length;
	int udp_mgmt_req_valid = 1;
	wan_mbox_t *mb = &card->wan_mbox;
	SHARED_MEMORY_INFO_STRUCT flags;
	wan_udp_pkt_t *wan_udp_pkt;
	struct timeval tv;
	int err;
	netskb_t* skb;
	wp_atm_cell_t tx_idle_data;

	wan_udp_pkt = (wan_udp_pkt_t *) priv_area->udp_pkt_data;

	//DEBUG_EVENT("%s: process_udp_mgmt_pkt(): command 0x%x\n",
	//	card->devname,wan_udp_pkt->wan_udp_command);

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	if (!local_dev){

		if(priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK){

			/* Only these commands are support for remote debugging.
			 * All others are not */
			switch(wan_udp_pkt->wan_udp_command) {

				case READ_GLOBAL_STATISTICS:
				case READ_MODEM_STATUS:  
				case ROUTER_UP_TIME:
				case READ_COMMS_ERROR_STATS:
				case READ_OPERATIONAL_STATS:
				/* These two commands are executed for
				 * each request */
				case READ_CONFIGURATION:
				case READ_CODE_VERSION:
				case WAN_GET_MEDIA_TYPE:
				case WAN_FE_GET_STAT:
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
			DEBUG_EVENT( 
			"%s: Warning, Illegal UDP command attempted from network: %x\n",
			card->devname,wan_udp_pkt->wan_udp_command);
		}

   	}else{ 
		wan_udp_hdr_t* udp_hdr = &wan_udp_pkt->wan_udp_hdr;
		wan_trace_t *trace_info = card->u.atm.trace_info;
	
		if (!trace_info){
			DEBUG_EVENT( 
			"%s: Critical error, no trace info ptr in udp dbg\n",
				card->devname);

			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
			goto process_udp_cmd_exit;
		}
		
		wan_udp_pkt->wan_udp_opp_flag = 0;

		switch(wan_udp_pkt->wan_udp_command) {

		case ENABLE_TRACING:
	
			udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
			udp_hdr->wan_udphdr_data_len = 0;
			
			if (!wan_test_bit(0,&trace_info->tracing_enabled)){
						
				trace_info->trace_timeout = SYSTEM_TICKS;
					
				wan_trace_purge(trace_info);
					
				//DEBUG_EVENT("%s: ATM ENABLE_TRACING : mode %d\n", card->devname,
				//	udp_hdr->wan_udphdr_data[0]);
				
				switch(udp_hdr->wan_udphdr_data[0])
				{
				case 0:
					wan_clear_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L3 trace enabled!\n",
						card->devname);
					break;
				case 1:
					wan_clear_bit(2,&trace_info->tracing_enabled);
					wan_set_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L2 trace enabled!\n",
						card->devname);
					break;
				case 3:
					wan_clear_bit(1,&trace_info->tracing_enabled);
					//user wants to see all cells, including idle
					wan_set_bit(2,&trace_info->tracing_enabled);
					wan_set_bit(3,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ATM 'All Cells' trace enabled!\n",
						card->devname);
					break;
				default:
					wan_clear_bit(1,&trace_info->tracing_enabled);
					wan_set_bit(2,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L1 trace enabled!\n",
						card->devname);
				}
				wan_set_bit (0,&trace_info->tracing_enabled);

			}else{
				DEBUG_ERROR("%s: Error: ATM trace running!\n",
						card->devname);
				udp_hdr->wan_udphdr_return_code = 2;
			}
					
			break;

		case DISABLE_TRACING:
			
			udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
			
			if(wan_test_bit(0,&trace_info->tracing_enabled)) {
					
				wan_clear_bit(0,&trace_info->tracing_enabled);
				wan_clear_bit(1,&trace_info->tracing_enabled);
				wan_clear_bit(2,&trace_info->tracing_enabled);
				wan_clear_bit(3,&trace_info->tracing_enabled);
				
				wan_trace_purge(trace_info);
				
				DEBUG_UDP("%s: Disabling ADSL trace\n",
							card->devname);
					
			}else{
				/* set return code to line trace already 
				   disabled */
				udp_hdr->wan_udphdr_return_code = 1;
			}

			break;

	        case GET_TRACE_INFO:

			if(wan_test_bit(0,&trace_info->tracing_enabled)){
				trace_info->trace_timeout = SYSTEM_TICKS;
			}else{
				DEBUG_ERROR("%s: Error ATM trace not enabled\n",
						card->devname);
				/* set return code */
				udp_hdr->wan_udphdr_return_code = 1;
				break;
			}

			buffer_length = 0;
			udp_hdr->wan_udphdr_atm_num_frames = 0;	
			udp_hdr->wan_udphdr_atm_ismoredata = 0;
					
#if defined(__FreeBSD__) || defined(__OpenBSD__)
			while (wan_trace_queue_len(trace_info)){
				WAN_IFQ_POLL(&trace_info->trace_queue, skb);
				if (skb == NULL){	
					DEBUG_EVENT("%s: No more trace packets in trace queue!\n",
								card->devname);
					break;
				}
				if ((WAN_MAX_DATA_SIZE - buffer_length) < skb->m_pkthdr.len){
					/* indicate there are more frames on board & exit */
					udp_hdr->wan_udphdr_atm_ismoredata = 0x01;
					break;
				}

				m_copydata(skb, 
					   0, 
					   skb->m_pkthdr.len, 
					   &udp_hdr->wan_udphdr_data[buffer_length]);
				buffer_length += skb->m_pkthdr.len;
				WAN_IFQ_DEQUEUE(&trace_info->trace_queue, skb);
				if (skb){
					wan_skb_fee(skb);
				}
				udp_hdr->wan_udphdr_atm_num_frames++;
			}
#elif defined(__LINUX__)

			if( (wan_test_bit(3,&trace_info->tracing_enabled)) ){
					
				memset(&tx_idle_data, 0x00, sizeof(wp_atm_cell_t));
		
				//set CLP
				tx_idle_data.hdr[3] = phy_idle_cfg.Tx_underrun_cell_CLP;
				//set HEC					
				tx_idle_data.hdr[4] = calculate_hec_crc(tx_idle_data.hdr);
					
				//set the payload
				memset(tx_idle_data.info, phy_idle_cfg.Tx_underrun_cell_payload, ATM_PAYLOAD_SZ);
					
				if(wpabs_get_last_trace_direction(trace_info) == TRC_INCOMING_FRM ){

					capture_atm_trace_packet(trace_info,
								&tx_idle_data,
							       	sizeof(wp_atm_cell_t), TRC_OUTGOING_FRM);

					wpabs_set_last_trace_direction(trace_info, TRC_OUTGOING_FRM);
				}else{
					
					trace_info->missed_idle_rx_counter++;
					if(trace_info->missed_idle_rx_counter > 10){

						//This condition is possible if no idle cells received
						//for a long period of time. It usualy means line is disconnected.
						//Despite that the tx idle cells may still be transmitted.
						
						capture_atm_trace_packet(trace_info,
								&tx_idle_data,
							       	sizeof(wp_atm_cell_t), TRC_OUTGOING_FRM);

						wpabs_set_last_trace_direction(trace_info, TRC_OUTGOING_FRM);
						trace_info->missed_idle_rx_counter = 0;
					}
				}
			}
			
					
			while ((skb=skb_dequeue(&trace_info->trace_queue)) != NULL){

				if((MAX_TRACE_BUFFER - buffer_length) < wan_skb_len(skb)){
					/* indicate there are more frames on board & exit */
					udp_hdr->wan_udphdr_atm_ismoredata = 0x01;
					if (buffer_length != 0){
						wan_skb_queue_head(&trace_info->trace_queue, skb);
					}else{
						/* If rx buffer length is greater than the
						 * whole udp buffer copy only the trace
						 * header and drop the trace packet */

						memcpy(&udp_hdr->wan_udphdr_atm_data[buffer_length], 
							wan_skb_data(skb),
							sizeof(wan_trace_pkt_t));

						buffer_length = sizeof(wan_trace_pkt_t);
						udp_hdr->wan_udphdr_atm_num_frames++;
						wan_skb_free(skb);	
					}
					break;
				}

				memcpy(&udp_hdr->wan_udphdr_atm_data[buffer_length], 
				       wan_skb_data(skb),
				       wan_skb_len(skb));
		     
				buffer_length += wan_skb_len(skb);
				wan_skb_free(skb);
				udp_hdr->wan_udphdr_atm_num_frames++;
			}
#endif                      
			/* set the data length and return code */
			udp_hdr->wan_udphdr_data_len = buffer_length;
			udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
			break;

		case FT1_READ_STATUS:
			
			((unsigned char *)wan_udp_pkt->wan_udp_data )[0] =
				flags.FE_info_struct.parallel_port_A_input;

			((unsigned char *)wan_udp_pkt->wan_udp_data )[1] =
				flags.FE_info_struct.parallel_port_B_input;
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			wan_udp_pkt->wan_udp_data_len = 2;
			mb->wan_data_len = 2;
			break;

		case ROUTER_UP_TIME:
			do_gettimeofday( &tv );
			priv_area->router_up_time = tv.tv_sec - 
					priv_area->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					priv_area->router_up_time;	
			mb->wan_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			break;

		case ATM_LINK_STATUS:

			wan_udp_pkt->wan_udp_data[0] = (card->wandev.state==WAN_CONNECTED)?1:0;	
			wan_udp_pkt->wan_udp_data[1] = wanpipe_get_atm_state(priv_area->sar_pvc);
			wan_udp_pkt->wan_udp_data_len = 2;
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			break;

   		case FT1_MONITOR_STATUS_CTRL:
			/* Enable FT1 MONITOR STATUS */
	        	if ((wan_udp_pkt->wan_udp_data[0] & ENABLE_READ_FT1_STATUS) ||  
				(wan_udp_pkt->wan_udp_data[0] & ENABLE_READ_FT1_OP_STATS)) {
			
			     	if( rCount++ != 0 ) {
					wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
					mb->wan_data_len = 1;
		  			break;
		    	     	}
	      		}

	      		/* Disable FT1 MONITOR STATUS */
	      		if( wan_udp_pkt->wan_udp_data[0] == 0) {

	      	   	     	if( --rCount != 0) {
		  			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
					mb->wan_data_len = 1;
		  			break;
	   	    	     	} 
	      		} 	
			goto dflt_1;

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
		   	wan_udp_pkt->wan_udp_atm_num_frames = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		default:
dflt_1:
			DEBUG_EVENT("%s: onboard command: 0x%X\n",
				card->devname, wan_udp_pkt->wan_udp_command);

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
				frmw_error(card,err,mb);
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

process_udp_cmd_exit:
	
     	/* Fill UDP TTL */
	wan_udp_pkt->wan_ip_ttl= card->wandev.ttl; 

	if (local_dev){
		wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
		return 1;
	}

/* FIXME ?????????????????????? */
#if 0
	len = reply_udp(priv_area->udp_pkt_data, mb->wan_data_len);

     	if(priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK){

		/* Must check if we interrupted if_send() routine. The
		 * tx buffers might be used. If so drop the packet */
	   	if (!wan_test_bit(SEND_CRIT,&card->wandev.critical)) {
		
			if(!frmw_send(card, priv_area->udp_pkt_data, len, 0)) {
				++ chan->if_stats.tx_packets;
				chan->if_stats.tx_bytes += len;
			}
		}
	} else {	
	
		/* Pass it up the stack
    		   Allocate socket buffer */
		if ((new_skb = wan_skb_alloc(len) != NULL) {
			/* copy data into new_skb */

 	    		buf = wan_skb_put(new_skb, len);
  	    		memcpy(buf, priv_area->udp_pkt_data, len);

            		/* Decapsulate pkt and pass it up the protocol stack */
	    		new_skb->protocol = htons(ETH_P_IP);
            		new_skb->dev = dev;
	    		wan_skb_reset_mac_header(new_skb);

			netif_rx(new_skb);
		} else {
	    	
			DEBUG_EVENT( "%s: no socket buffers available!\n",
					card->devname);
  		}
    	}
#endif
	wan_atomic_set(&priv_area->udp_pkt_len,0);
 	
	return 0;
}

static int 
capture_atm_trace_packet(
		wan_trace_t *trace_info,
		void 		*data,
		int		len,
		char		direction)
{
	int atm_packets=1;
	int i;
	wan_trace_pkt_t trc_el;		
	volatile wp_atm_cell_t *atm_cell_ptr=(wp_atm_cell_t*)data;
	void *new_skb=NULL;
	
	atm_packets = len/sizeof(wp_atm_cell_t);

	if (atm_packets <= 0){
		return -1;
	}
	
	if(wpabs_tracing_enabled(trace_info) != 3){
		return -1;
	}
	
	for (i=0;i<atm_packets;i++){

		new_skb = wpabs_skb_alloc(sizeof(wp_atm_cell_t)+sizeof(wan_trace_pkt_t)); 
		if (new_skb == NULL) 
			return -1;
		
		trc_el.status		= direction;
		trc_el.data_avail	= 1;
		trc_el.time_stamp	= (unsigned short)(wpabs_get_systemticks() % 0xFFFF);
		trc_el.real_length	= sizeof(wp_atm_cell_t);
	
		wpabs_skb_copyback(new_skb, 
				  wpabs_skb_len(new_skb), 
				  sizeof(wan_trace_pkt_t), 
				  (unsigned long)&trc_el);

		wpabs_skb_copyback(new_skb, 
				  wpabs_skb_len(new_skb), 
				  sizeof(wp_atm_cell_t),
				  (unsigned long)atm_cell_ptr);

		wpabs_trace_enqueue(trace_info, new_skb);

		atm_cell_ptr++;
	}

	return 0;
}

/**SECTION*************************************************************
 *
 * 	TASK Functions and Triggers
 *
 **********************************************************************/


/*============================================================================
 * port_set_state
 * 
 * Set PORT state.
 *
 */
static void port_set_state (sdla_t *card, int state)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev;

        if (card->wandev.state != state)
        {
                switch (state)
                {
                case WAN_CONNECTED:
                        DEBUG_EVENT( "%s: Link connected!\n",
                                card->devname);
                      	break;

                case WAN_CONNECTING:
                        DEBUG_EVENT( "%s: Link connecting...\n",
                                card->devname);
                        break;

                case WAN_DISCONNECTED:
                        DEBUG_EVENT( "%s: Link disconnected!\n",
                                card->devname);
                        break;
                }

                card->wandev.state = state;
		WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
			private_area_t *priv_area;

			dev = WAN_DEVLE2DEV(devle);
			if (!dev || !wan_netif_priv(dev))
				continue;
			priv_area = wan_netif_priv(dev);
			priv_area->common.state = state;

			if (priv_area->sar_pvc){
				if (state == WAN_CONNECTED){
					wanpipe_set_atm_state(priv_area->sar_pvc,ATM_CONNECTED);
				}else{
					wanpipe_set_atm_state(priv_area->sar_pvc,ATM_DISCONNECTED);
				}
			}

			netif_wake_queue(dev);	
			
			if (priv_area->common.usedby == API){
				wan_wakeup_api(priv_area);
			}
		}
        }
}

/*===========================================================================
 * config_frmw
 *
 *	Configure the protocol and enable communications.		
 *
 *   	The if_open() function binds this function to the poll routine.
 *      Therefore, this function will run every time the interface
 *      is brought up. We cannot run this function from the if_open 
 *      because if_open does not have access to the remote IP address.
 *      
 *	If the communications are not enabled, proceed to configure
 *      the card and enable communications.
 *
 *      If the communications are enabled, it means that the interface
 *      was shutdown by either the user or driver. In this case, we 
 *      have to check that the IP addresses have not changed.  If
 *      the IP addresses have changed, we have to reconfigure the firmware
 *      and update the changed IP addresses.  Otherwise, just exit.
 *
 */

static int config_frmw (sdla_t *card)
{

	DEBUG_EVENT("%s: Configuring ATM Protocol\n",
			card->devname);

	/* Setup the Board for */
	if (set_frmw_config(card)) {
		DEBUG_EVENT( "%s: Failed configuration!\n",
			card->devname);
		return 0;
	}

	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
	if (IS_TE1_CARD(card)) {
		int	err = -EINVAL;
		DEBUG_EVENT( "%s: Configuring onboard %s CSU/DSU\n",
			card->devname, 
			(IS_T1_CARD(card))?"T1":"E1");
		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			DEBUG_EVENT( "%s: Failed %s configuratoin!\n",
					card->devname,
					(IS_T1_CARD(card))?"T1":"E1");
			return -EINVAL;
		}
	}

	 
	if (IS_56K_CARD(card)) {
		int	err = -EINVAL;
		DEBUG_EVENT( "%s: Configuring 56K onboard CSU/DSU\n",
			card->devname);

		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			DEBUG_EVENT( "%s: Failed 56K configuration!\n",
				card->devname);
			return -EINVAL;
		}
	}

	
	/* Set interrupt mode and mask */
        if (frmw_set_intr_mode(card, APP_INT_ON_RX_FRAME |
                		APP_INT_ON_GLOBAL_EXCEP_COND |
                		APP_INT_ON_TX_FRAME |
                		APP_INT_ON_EXCEP_COND | APP_INT_ON_TIMER)){
		DEBUG_EVENT( "%s: Failed to set interrupt triggers!\n",
				card->devname);
		return -EINVAL;	
        }
	
	/* Mask All interrupts  */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, 
			(APP_INT_ON_RX_FRAME | APP_INT_ON_TX_FRAME | 
			 APP_INT_ON_TIMER    | APP_INT_ON_GLOBAL_EXCEP_COND | 
			 APP_INT_ON_EXCEP_COND));

	if (frmw_comm_enable(card) != 0) {
		DEBUG_EVENT( "%s: Failed to enable communications!\n",
				card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
		card->comm_enabled=0;
		frmw_set_intr_mode(card,0);
		return -EINVAL;
	}

	/* Initialize Rx/Tx buffer control fields */
	init_tx_rx_buff(card);
	card->u.atm.state = WAN_CONNECTING;
	port_set_state(card, WAN_CONNECTING);

	/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1); 
	}

	/* Unmask all interrupts except the Transmit and Timer interrupts */
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, 
			(APP_INT_ON_RX_FRAME | APP_INT_ON_GLOBAL_EXCEP_COND | 
			APP_INT_ON_EXCEP_COND));
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	return 0; 
}


/*============================================================
 * frmw_poll
 *	
 * Rationale:
 * 	We cannot manipulate the routing tables, or
 *      ip addresses withing the interrupt. Therefore
 *      we must perform such actons outside an interrupt 
 *      at a later time. 
 *
 * Description:	
 *	polling routine, responsible for 
 *     	shutting down interfaces upon disconnect
 *     	and adding/removing routes. 
 *      
 * Usage:        
 * 	This function is executed for each   
 * 	interface through a tq_schedule bottom half.
 *      
 *      trigger_poll() function is used to kick
 *      the chldc_poll routine.  
 *
 *      if_open() calls the timer delay function which
 *      in turn calls the trigger_poll() to kick
 *      this task. 
 */


# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 	
static void frmw_poll (void *arg)
# else
static void frmw_poll (struct work_struct *work)
# endif
{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
	private_area_t* priv_area = (private_area_t*)container_of(work, private_area_t, poll_task);  
#else
	private_area_t* priv_area = (private_area_t*)arg;  
#endif          
	netdevice_t *dev;
	sdla_t *card;
	u8 check_gateway=0;	

	if (!priv_area || (dev=priv_area->dev) == NULL){
		return;
	}

	card = priv_area->card;

	/* (Re)Configuraiton is in progress, stop what you are 
	 * doing and get out */
	if (wan_test_bit(PERI_CRIT,&card->wandev.critical)){
		wan_clear_bit(POLL_CRIT,&card->wandev.critical);
		return;
	}
	
	switch (card->wandev.state){

	case WAN_DISCONNECTED:

		/* If the dynamic interface configuration is on, and interface 
		 * is up, then bring down the network interface */
		
		if (wan_test_bit(DYN_OPT_ON,&priv_area->interface_down) && 
		    !wan_test_bit(DEV_DOWN,  &priv_area->interface_down) &&		
		    dev->flags & IFF_UP){	

			DEBUG_EVENT( "%s: Interface %s down.\n",
				card->devname,dev->name);
			change_dev_flags(dev,(dev->flags&~IFF_UP));
			set_bit(DEV_DOWN,&priv_area->interface_down);
			priv_area->route_status = NO_ROUTE;

		}

		break;

	case WAN_CONNECTED:

		/* In SMP machine this code can execute before the interface
		 * comes up.  In this case, we must make sure that we do not
		 * try to bring up the interface before dev_open() is finished */


		/* DEV_DOWN will be set only when we bring down the interface
		 * for the very first time. This way we know that it was us
		 * that brought the interface down */
		
		if (wan_test_bit(DYN_OPT_ON,&priv_area->interface_down) &&
		    wan_test_bit(DEV_DOWN,  &priv_area->interface_down) &&
		    !(dev->flags & IFF_UP)){
			
			DEBUG_EVENT( "%s: Interface %s up.\n",
				card->devname,dev->name);
			change_dev_flags(dev,(dev->flags|IFF_UP));
			wan_clear_bit(DEV_DOWN,&priv_area->interface_down);
			check_gateway=1;
		}

		if (priv_area->gateway && check_gateway)
			add_gateway(card,dev);

		break;
	}	

	wan_clear_bit(POLL_CRIT,&card->wandev.critical);
}

/*============================================================
 * trigger_poll
 *
 * Description:
 * 	Add a frmw_poll() task into a tq_scheduler bh handler
 *      for a specific interface.  This will kick
 *      the fr_poll() routine at a later time. 
 *
 * Usage:
 * 	Interrupts use this to defer a taks to 
 *      a polling routine.
 *
 *      To execute tasks out of interrupt context.
 *
 */	

static void trigger_poll (struct net_device *dev)
{
	private_area_t *priv_area;
	sdla_t *card;

	/* FIXME Poll routine is not used */
	return;
	
	if (!dev)
		return;
	
	if ((priv_area = wan_netif_priv(dev))==NULL)
		return;

	card = priv_area->card;
	
	if (wan_test_and_set_bit(POLL_CRIT,&card->wandev.critical)){
		return;
	}
	if (wan_test_bit(PERI_CRIT,&card->wandev.critical)){
		return; 
	}

	
	WAN_TASKQ_SCHEDULE((&priv_area->poll_task));
	return;
}


/*============================================================
 * handle_front_end_state
 *
 * Front end state indicates the physical medium that
 * the Z80 backend connects to.  
 * 
 * S514-1/2/3:		V32/RS232/FT1 Front End
 * 	  		Front end state is determined via 
 * 	 		Modem/Status.
 * S514-4/5/7/8:	56K/T1/E1 Front End
 * 			Front end state is determined via
 * 			link status interrupt received
 * 			from the front end hardware.
 *
 * If the front end state handler is enabed by the
 * user.  The interface state will follow the 
 * front end state. I.E. If the front end goes down
 * the protocol and interface will be declared down.
 *
 * If the front end state is UP, then the interface
 * and protocol will be up ONLY if the protocol is
 * also UP.
 *
 * Therefore, we must have three state variables
 * 1. Front End State (card->wandev.front_end_status)
 * 2. Protocol State  (card->wandev.state)
 * 3. Interface State (dev->flags & IFF_UP)
 *
 */

static void handle_front_end_state(void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;
	netdevice_t	*dev;

	/* FIXME: The is a bug fix to gideons firmware
	 *        If we ever loose the front end, put
	 *        the card into re-sync mode */
	if (card->fe.fe_status != FE_CONNECTED){
		wp_handle_out_of_sync_condition(card,1);
	}

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (card->fe.fe_status == FE_CONNECTED){
		if (card->u.atm.state == WAN_CONNECTED){
			port_set_state(card,WAN_CONNECTED);
			trigger_poll(dev);
		}
	}else{
		port_set_state(card,WAN_DISCONNECTED);
		trigger_poll(dev);
	}
}

/*============================================================
 * s508_lock and s508_unlock
 *
 * Used to lock and unlock critical code areas. By
 * default only Interrupt is allowed to execute
 * board commands.  If the non-interrupt process
 * tries to execute a command these locks must
 * be used to turn off the interrupts.
 *
 * Otherwise race conditions can occur between
 * the interrupt and non-interrupt kernel 
 * processes.
 *
 */

void s508_lock (sdla_t *card, unsigned long *smp_flags, unsigned long *smp_flags1)
{
	wan_spin_lock_irq(&card->wandev.lock, smp_flags);
        if (card->next){
        	wan_spin_lock(&card->next->wandev.lock,smp_flags1);
	}
}

void s508_unlock (sdla_t *card, unsigned long *smp_flags, unsigned long *smp_flags1)
{
        if (card->next){
        	wan_spin_unlock(&card->next->wandev.lock,smp_flags1);
        }
        wan_spin_unlock_irq(&card->wandev.lock, smp_flags);
}




/**SECTION**********************************************************
 * 
 * 		PROC FILE SYSTEM SUPPORT 
 * 
 *******************************************************************/



#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
#warning "Enabling Proc File System Hooks"


#define PROC_CFG_FRM	"%-15s| %-12s|\n"
#define PROC_STAT_FRM	"%-15s| %-12s| %-14s|\n"
static char config_hdr[] =
	"Interface name | Device name |\n";
static char status_hdr[] =
	"Interface name | Device name | Status        |\n";

static int get_config_info(void* priv, char* buf, int cnt, int len, int offs, int* stop_cnt) 
{
	private_area_t*	priv_area = priv;
	sdla_t*			card = NULL;
	int			size = 0;

	if (priv_area == NULL)
		return cnt;
	card = priv_area->card;

	if ((offs == 0 && cnt == 0) || (offs && offs == *stop_cnt)){
		PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size, "%s", config_hdr));
	}

	PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size,
			    PROC_CFG_FRM, priv_area->if_name, card->devname));
	return cnt;
}

static int get_status_info(void* priv, char* buf, int cnt, int len, int offs, int* stop_cnt)
{
	private_area_t*	priv_area = priv;
	sdla_t*			card = NULL;
	int			size = 0;

	if (priv_area == NULL)
		return cnt;
	card = priv_area->card;

	if ((offs == 0 && cnt == 0) || (offs && offs == *stop_cnt)){
		PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size, "%s", status_hdr));
	}

	PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size, PROC_STAT_FRM, 
			    priv_area->if_name, card->devname, STATE_DECODE(priv_area->common.state)));

	return cnt;
}

#define PROC_DEV_FR_S_FRM	"%-20s| %-14s|\n"
#define PROC_DEV_FR_D_FRM	"%-20s| %-14d|\n"
#define PROC_DEV_SEPARATE	"=====================================\n"

#if defined(LINUX_2_4)||defined(LINUX_2_6)
static int get_dev_config_info(char* buf, char** start, off_t offs, int len)
#else
static int get_dev_config_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
{
	int 		cnt = 0;
	wan_device_t*	wandev = (void*)start;
	sdla_t*		card = NULL;
	int 		size = 0;
	PROC_ADD_DECL(stop_cnt);

	if (wandev == NULL)
		return cnt;

	PROC_ADD_INIT(offs, stop_cnt);
	card = (sdla_t*)wandev->priv;

	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, "Configuration for %s device\n",
		 wandev->name));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_S_FRM,
		 "Comm Port", COMPORT_DECODE(card->wandev.comm_port)));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_S_FRM,
		 "Interface", INT_DECODE(wandev->interface)));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_S_FRM,
		 "Clocking", CLK_DECODE(wandev->clocking)));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_D_FRM,
		 "BaudRate",wandev->bps));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_D_FRM,
		 "MTU", wandev->mtu));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_D_FRM,
		 "UDP Port",  wandev->udp_port));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_D_FRM,
		 "TTL", wandev->ttl));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE));

	PROC_ADD_RET(cnt, offs, stop_cnt);
}

static int set_dev_config(struct file *file, 
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

	DEBUG_EVENT( "%s: New device config (%s)\n",
			wandev->name, buffer);
	/* Parse string */

	return count;
}


#define PROC_IF_FR_S_FRM	"%-30s\t%-14s\n"
#define PROC_IF_FR_D_FRM	"%-30s\t%-14d\n"
#define PROC_IF_FR_L_FRM	"%-30s\t%-14ld\n"
#define PROC_IF_SEPARATE	"====================================================\n"

#if defined(LINUX_2_4)||defined(LINUX_2_6)
static int get_if_info(char* buf, char** start, off_t offs, int len)
#else
static int get_if_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
{
	int 			cnt = 0;
	struct net_device*	dev = (void*)start;
	private_area_t* 	priv_area = wan_netif_priv(dev);
	sdla_t*			card = priv_area->card;
	int 			size = 0;
	PROC_ADD_DECL(stop_cnt);

	goto get_if_info_end;
	PROC_ADD_INIT(offs, stop_cnt);
	/* Update device statistics */
	if (!offs && card->wandev.update) {
		int rslt = 0;
		rslt = card->wandev.update(&card->wandev);
		if(rslt) {
			switch (rslt) {
			case -EAGAIN:
				PROC_ADD_LINE(cnt, 
					(buf, &cnt, len, offs, &stop_cnt, &size, 
					 "Device is busy!\n"));
				break;

			default:
				PROC_ADD_LINE(cnt, 
					(buf, &cnt, len, offs, &stop_cnt, &size,
					 "Device is not configured!\n"));
				break;
			}
			goto get_if_info_end;
		}
	}

get_if_info_end:
	PROC_ADD_RET(cnt, offs, stop_cnt);
}
	
static int set_if_info(struct file *file, 
		       const char *buffer,
		       unsigned long count, 
		       void *data)
{
	struct net_device*	dev = (void*)data;
	private_area_t* 	priv_area = NULL;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return count;

	priv_area = (private_area_t*)wan_netif_priv(dev);


	DEBUG_EVENT( "%s: New interface config (%s)\n",
			priv_area->if_name, buffer);
	/* Parse string */

	return count;
}

/* WANPIPE_ENABLE_PROC_FILE_HOOKS */
#endif

/*
 * ******************************************************************
 * Proc FS function 
 */
static int wan_atm_get_info(void* pcard, struct seq_file *m, int *stop_cnt)
{
	sdla_t	*card = (sdla_t*)pcard;

	if (card->wandev.fe_iface.update_alarm_info){
		m->count = 
			WAN_FECALL(
				&card->wandev, 
				update_alarm_info, 
				(&card->fe, m, stop_cnt)); 
	}
	if (card->wandev.fe_iface.update_pmon_info){
		m->count = 
			WAN_FECALL(
				&card->wandev, 
				update_pmon_info, 
				(&card->fe, m, stop_cnt)); 
	}

	return m->count;
}


/****** End ****************************************************************/
