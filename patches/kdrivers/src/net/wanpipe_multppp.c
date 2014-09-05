/*****************************************************************************
* wanpipe_multppp.c Multi-Port PPP driver module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Apr 12, 2003	Nenad Corbic	Added MPAPI Annexg Support as well as the
*                               new tracing package.
* Mar 26, 2003	David Rokhvarg  Added PAP and CHAP support. Only as a peer,
* 				not an authenticator.
* Sep 20, 2001  Nenad Corbic    The min() function has changed for 2.4.9
*                               kernel. Thus using the wp_min() defined in
*                               wanpipe.h

* May 29 2001   Nenad Corbic    Added T1/E1 support (TE1).
* Apr 15 2001   Nenad Corbic    Bug Fix (2.2.X kernel) Driver
* 				crashed on shutdown while mrouted
* 				was running. The dev->do_ioctl call
* 				was still bound to syncppp after
* 				syncpp_detach().
* Dec 15 2000   Nenad Corbic    Updated for 2.4.X kernel
* Nov 15 2000   Nenad Corbic    Fixed the SyncPPP support for
* 				kernels 2.2.16 and higher.
*   				The pppstruct has changed.
* Jul 13 2000	Nenad Corbic    Using the kernel Syncppp module on
* 			        top of RAW Wanpipe CHDLC module.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_abstr.h"
#include "wanpipe_wanrouter.h"
#include "wanpipe.h"
#include "wanpipe_snmp.h"
#include "sdla_chdlc.h"		/* CHDLC firmware API definitions */
#include "sdla_asy.h"         	/* CHDLC (async) API definitions */
#include "if_wanpipe_common.h"    /* Socket Driver common area */
#include "sdlapci.h"

#if defined(__LINUX__)
#include "wanproc.h"
# include "if_wanpipe.h"
#include "wanpipe_syncppp.h"
#endif

#include "wanpipe_iface.h"


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
# include "wanpipe_lapb_kernel.h"
#endif

/****** Defines & Macros ****************************************************/

#ifdef	_DEBUG_
#define	STATIC
#else
#define	STATIC		static
#endif

#if defined(__LINUX__)
# define ENABLE_MULTPROT_MODE_WANPIPE
#else
# undef ENABLE_MULTPROT_MODE_WANPIPE
#endif

#define CHDLC_OLD_INTR_TEST

/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   	0x01
#define TMR_INT_ENABLED_UPDATE	0x02
#define TMR_INT_ENABLED_CONFIG  0x04
#define TMR_INT_ENABLED_TE	0x20


#define	CHDLC_DFLT_DATA_LEN	1500		/* default MTU */
#define CHDLC_HDR_LEN		1

#define IFF_POINTTOPOINT 0x10

#define CHDLC_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_BH_BUFF	10

#define CRC_LENGTH 	2
#define PPP_HEADER_LEN 	4

#define MAX_TRACE_QUEUE		100

#define MAX_RX_QUEUE 100

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following
 * structure will incorporate the card structure along with CHDLC specific data
 */

typedef struct private_area
{
	wanpipe_common_t common;
	sdla_t		*card;
	netdevice_t	*dev;
	wan_time_t		router_start_time;	/*unsigned long 	router_start_time;*/
	unsigned char 	route_status;
	unsigned char 	route_removed;
	wan_ticks_t	tick_counter;		/* For 5s timeout counter */
	wan_time_t		router_up_time;	/*unsigned long 	router_up_time;*/
	unsigned char   mc;			/* Mulitcast support on/off */
	char update_comms_stats;		/* updating comms stats */

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;

	unsigned char ignore_modem;

	char udp_pkt_src;
	unsigned char udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t udp_pkt_len;

	wan_trace_t 	trace_info;
	wan_tasklet_t	tasklet;
	wan_skb_queue_t rx_queue;

	char if_name[WAN_IFNAME_SZ+1];
	netdevice_t *annexg_dev;
	unsigned char label[WAN_IF_LABEL_SZ+1];

	unsigned char prev_error;

} private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/* variable for keeping track of enabling/disabling FT1 monitor status */
static int rCount = 0;

extern wan_iface_t wan_iface;

/* variable for tracking how many interfaces to open for WANPIPE on the
   two ports */

/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);
static int del_if (wan_device_t* wandev, netdevice_t* dev);

/* Network device interface */
#if defined(__LINUX__)
static int if_init   (netdevice_t* dev);
static struct net_device_stats* if_stats (netdevice_t* dev);
#endif
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
#if defined(__LINUX__)
static int if_send (netskb_t* skb, netdevice_t* dev);
#else
static int if_send(netdevice_t *dev, netskb_t *skb, struct sockaddr *dst,struct rtentry *rt);
#endif
static void if_tx_timeout (netdevice_t *dev);

/* CHDLC Firmware interface functions */
static int chdlc_configure 	(sdla_t* card, void* data);
static int chdlc_comm_enable 	(sdla_t* card);
static int chdlc_comm_disable 	(sdla_t* card);
static int chdlc_read_version 	(sdla_t* card, char* str);
static int chdlc_set_intr_mode 	(sdla_t* card, unsigned mode);
static int set_adapter_config 	(sdla_t* card);
static int chdlc_send (sdla_t* card, void* data, unsigned len);
static int chdlc_read_configure(sdla_t *card, CHDLC_CONFIGURATION_STRUCT* cfg);
static int chdlc_read_comm_err_stats (sdla_t* card);
static int chdlc_read_op_stats (sdla_t* card);
static int config_chdlc (sdla_t *card);
#if defined(__LINUX__)
static int wp_exec_usr_mbox_cmd(sdla_t *card, wan_mbox_t *usrmbox);
#endif

/* Miscellaneous CHDLC Functions */
static int set_chdlc_config (sdla_t* card);
static int set_asy_config(sdla_t* card);
static int asy_comm_enable (sdla_t* card);
static void init_chdlc_tx_rx_buff( sdla_t* card, netdevice_t *dev );
static int chdlc_error (sdla_t *card, int err, wan_mbox_t *mb);
static int process_chdlc_exception(sdla_t *card);
static int process_global_exception(sdla_t *card);
static int update_comms_stats(sdla_t* card,
        private_area_t* chan);
static void port_set_state (sdla_t *card, int);

/* Interrupt handlers */
static WAN_IRQ_RETVAL wsppp_isr (sdla_t* card);
static void rx_intr (sdla_t* card);
static void timer_intr(sdla_t *);

/* Miscellaneous functions */
#if defined(__LINUX__) || defined(CHDLC_OLD_INTR_TEST)
static int chdlc_intr_test( sdla_t* card);
#endif
static int udp_pkt_type( netskb_t *skb , sdla_t* card);
static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                netskb_t *skb, netdevice_t* dev,
                                private_area_t* chan);
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				private_area_t* chan,int local_dev);
static void s508_lock (sdla_t *card, wan_smp_flag_t *smp_flags, wan_smp_flag_t *smp_flags1);
static void s508_unlock (sdla_t *card, wan_smp_flag_t *smp_flags, wan_smp_flag_t *smp_flags1);
static void send_ppp_term_request (netdevice_t*);


#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
static int chdlc_get_config_info(void* priv, struct seq_file* m, int*);
static int chdlc_get_status_info(void* priv, struct seq_file* m, int*);
static int chdlc_set_dev_config(struct file*, const char*, unsigned long, void *);
static int chdlc_set_if_info(struct file*, const char*, unsigned long, void *);
#endif

static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t);
static void chdlc_enable_timer (void* card_id);
#if defined(__LINUX__)
static void 	wp_bh (unsigned long);
#else
static void 	wp_bh (void*,int);
#endif


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev);
static netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t* annexg_dev_name);
static int get_map(wan_device_t*, netdevice_t*, struct seq_file* m, int*);
static void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats);
#endif

static int digital_loop_test(sdla_t* card,wan_udp_pkt_t* wan_udp_pkt);

/* TE1 */
static void handle_front_end_state(void* card_id);

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
int wp_mprot_init (sdla_t* card, wandev_conf_t* conf)
{
	CHDLC_CONFIGURATION_STRUCT	cfg;
	unsigned char			port_num;
	int				err;
	u_int32_t			max_permitted_baud = 0;
	union
		{
		char str[80];
		} u;
	volatile wan_mbox_t* mb;
	wan_mbox_t	*mb1;
	wan_ticks_t	timeout;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_MPPP) {
		DEBUG_EVENT("%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Find out which Port to use */
	if ((conf->comm_port == WANOPT_PRI) || (conf->comm_port == WANOPT_SEC)){
		if (card->next){

			if (conf->comm_port != card->next->u.c.comm_port){
				card->u.c.comm_port = conf->comm_port;
			}else{
				DEBUG_ERROR("%s: ERROR - %s port used!\n",
        		        	card->wandev.name, PORT(conf->comm_port));
				return -EINVAL;
			}
		}else{
			card->u.c.comm_port = conf->comm_port;
		}
	}else{
		DEBUG_ERROR("%s: ERROR - Invalid Port Selected!\n",
                			card->wandev.name);
		return -EINVAL;
	}


	/* Initialize protocol-specific fields */
	/* Set a pointer to the actual mailbox in the allocated virtual
	 * memory area */
	/* Alex Apr 8 2004 Sangoma ISA card */
	if (card->u.c.comm_port == WANOPT_PRI){
		card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;
	}else{
		card->mbox_off = SEC_BASE_ADDR_MB_STRUCT;
	}

	mb = &card->wan_mbox;
	mb1 = &card->wan_mbox;

	if (!card->configured){
		unsigned char return_code;
		/* The board will place an 'I' in the return code to indicate
	   	** that it is ready to accept commands.  We expect this to be
		** completed in less than 1 second. */

		timeout = SYSTEM_TICKS;
		/* Wait 1s for board to initialize */
		do {
			return_code = 0x00;
			card->hw_iface.peek(card->hw,
				card->mbox_off+offsetof(wan_mbox_t, wan_return_code),
				&return_code,
				sizeof(unsigned char));
			if ((SYSTEM_TICKS - timeout) > 1*HZ) break;
		}while(return_code != 'I');
		if (return_code != 'I') {
			DEBUG_EVENT("%s: Initialization not completed by adapter\n",
				card->devname);
			DEBUG_EVENT("Please contact Sangoma representative.\n");
			return -EIO;
		}
		DEBUG_EVENT("%s: Adapter initalization completed!\n",
				card->devname);
	}

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
		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg		= card->hw_iface.fe_read;

		card->wandev.fe_enable_timer = chdlc_enable_timer;
		card->wandev.te_link_state = handle_front_end_state;
		conf->electrical_interface =
			(IS_T1_CARD(card)) ? WANOPT_V35 : WANOPT_RS232;

		if (card->u.c.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else if (IS_56K_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg		= card->hw_iface.fe_read;

	}else{
		card->fe.fe_status = FE_CONNECTED;
	}

	card->wandev.ignore_front_end_status = conf->ignore_front_end_status;
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

	if (chdlc_read_version(card, u.str))
		return -EIO;

	DEBUG_EVENT("%s: Running Raw HDLC firmware v%s\n"
			 "%s: for Multi-Port protocol.\n",
			card->devname,u.str,card->devname);

	if (set_adapter_config(card)) {
		return -EIO;
	}
	card->isr			= &wsppp_isr;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->wandev.udp_port   	= conf->udp_port;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	card->wandev.bind_annexg	= &bind_annexg;
	card->wandev.un_bind_annexg	= &un_bind_annexg;
	card->wandev.get_map		= &get_map;
	card->wandev.get_active_inactive= &get_active_inactive;
#endif


	card->wandev.new_if_cnt = 0;

#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
	// Proc fs functions
	card->wandev.get_config_info 	= &chdlc_get_config_info;
	card->wandev.get_status_info 	= &chdlc_get_status_info;
	card->wandev.set_dev_config    	= &chdlc_set_dev_config;
	card->wandev.set_if_info     	= &chdlc_set_if_info;
#endif
	/* reset the number of times the 'update()' proc has been called */
	card->u.c.update_call_count = 0;

	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface;

	if ((card->u.c.comm_port == WANOPT_SEC && conf->electrical_interface == WANOPT_V35)&&
	    card->type != SDLA_S514){
		DEBUG_ERROR("%s: ERROR - V35 Interface not supported on S508 %s port \n",
			card->devname, PORT(card->u.c.comm_port));
		return -EIO;
	}


	card->wandev.clocking = conf->clocking;

	port_num = card->u.c.comm_port;

	/* Setup Port Bps */

	if(card->wandev.clocking) {
		if((port_num == WANOPT_PRI) || card->u.c.receive_only) {
			/* For Primary Port 0 */
               		max_permitted_baud =
				(card->type == SDLA_S514) ?
				PRI_MAX_BAUD_RATE_S514 :
				PRI_MAX_BAUD_RATE_S508;
		}
		else if(port_num == WANOPT_SEC) {
			/* For Secondary Port 1 */
                        max_permitted_baud =
                               (card->type == SDLA_S514) ?
                                SEC_MAX_BAUD_RATE_S514 :
                                SEC_MAX_BAUD_RATE_S508;
		}

		if(conf->bps > max_permitted_baud) {
			conf->bps = max_permitted_baud;
			DEBUG_EVENT(
			"%s: WARNING: Baud too high!\n",
				card->wandev.name);
			DEBUG_EVENT(
			"%s:          Baud rate set to %u bps\n",
				card->wandev.name, max_permitted_baud);
		}else if (conf->bps == 0){
			conf->bps = max_permitted_baud;
			DEBUG_EVENT(
			"%s: WARNING: Baud Rate is 0 bps (Internal clocking)!\n",
					card->devname);
			DEBUG_EVENT(
			"%s:          Baud rate set to %u bps\n",
				card->wandev.name, max_permitted_baud);
		}
		card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
  	}

	if (conf->u.chdlc.fast_isr == WANOPT_YES){
		DEBUG_EVENT("%s: Configuring Fast Interrupt Handlers\n",
				card->devname);
		card->u.c.protocol_options|=INSTALL_FAST_INT_HANDLERS;
	}


	/* Setup the Port MTU */
	if((port_num == WANOPT_PRI) || card->u.c.receive_only) {

		/* For Primary Port 0 */
		card->wandev.mtu =
			(conf->mtu >= MIN_LGTH_CHDLC_DATA_CFG) ?
			wp_min(conf->mtu, PRI_MAX_NO_DATA_BYTES_IN_FRAME) :
			CHDLC_DFLT_DATA_LEN;
	} else if(port_num == WANOPT_SEC) {
		/* For Secondary Port 1 */
		card->wandev.mtu =
			(conf->mtu >= MIN_LGTH_CHDLC_DATA_CFG) ?
			wp_min(conf->mtu, SEC_MAX_NO_DATA_BYTES_IN_FRAME) :
			CHDLC_DFLT_DATA_LEN;
	}

	/* Add on a PPP Header */
	card->wandev.mtu += PPP_HEADER_LEN;

	/* Set up the interrupt status area */
	/* Read the CHDLC Configuration and obtain:
	 *	Ptr to shared memory infor struct
         * Use this pointer to calculate the value of card->u.c.flags !
 	 */
	err = chdlc_read_configure(card, &cfg);
	if (err != WAN_CMD_OK){
		wan_clear_bit(1, (void*)&card->wandev.critical);
		chdlc_error(card, err, mb1);
		return -EIO;
	}

	/* Alex Apr 8 2004 Sangoma ISA card */
       	card->flags_off = cfg.ptr_shared_mem_info_struct;
    	card->intr_type_off = card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, interrupt_info_struct) +
		offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_type);
	card->intr_perm_off = card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, interrupt_info_struct) +
		offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_permission);
	card->fe_status_off = card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, FT1_info_struct) +
		offsetof(FT1_INFORMATION_STRUCT, parallel_port_A_input);

	/* This is for the ports link state */
	card->wandev.state = WAN_DUALPORT;
	card->u.c.state = WAN_DISCONNECTED;

#if defined(__LINUX__) || defined(CHDLC_OLD_INTR_TEST)
	if (!card->wandev.piggyback){
		if (chdlc_intr_test(card)){
			return -EIO;
		}
		card->configured = 1;
	}
#endif

	if (chdlc_set_intr_mode(card, APP_INT_ON_TIMER)){
		DEBUG_EVENT( "%s: Failed to set interrupt triggers!\n",
				card->devname);
		return -EIO;
        }

	/* Mask the Timer interrupt */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);

	DEBUG_EVENT("\n");
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
 *      3) CHDLC operational statistics on the adapter.
 * The board level statistics are read during a timer interrupt. Note that we
 * read the error and operational statistics during consecitive timer ticks so
 * as to minimize the time that we are inside the interrupt handler.
 *
 */
static int update (wan_device_t* wandev)
{
	sdla_t* card = wandev->priv;
 	netdevice_t* dev;
        private_area_t* chan;
        wan_smp_flag_t smp_flags;

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;

	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	/* more sanity checks */
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if(dev == NULL)
		return -ENODEV;

	if((chan=wan_netif_priv(dev)) == NULL)
		return -ENODEV;

       	if(chan->update_comms_stats){
		return -EAGAIN;
	}

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	update_comms_stats(card, chan);
	chan->update_comms_stats=0;
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

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
#if defined(ENABLE_MULTPROT_MODE_WANPIPE)
	struct ppp_device *pppdev=NULL;
	struct sppp *sp=NULL;
#endif
	sdla_t* card = wandev->priv;
	private_area_t* chan;
	int err = 0;

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		DEBUG_EVENT("%s: invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	if(++card->wandev.new_if_cnt > 1) {
		DEBUG_ERROR("%s: Error: Interface already configured!\n",
				card->devname);
		--card->wandev.new_if_cnt;
		return -EEXIST;
	}

	/* allocate and initialize private data */
	chan = wan_malloc(sizeof(private_area_t));

	if(chan == NULL){
		--card->wandev.new_if_cnt;
		return -ENOMEM;
	}

	memset(chan, 0, sizeof(private_area_t));

	chan->card = card;

	/* initialize data */
	strncpy(card->u.c.if_name, conf->name,WAN_IFNAME_SZ);
	strncpy(chan->if_name,conf->name,WAN_IFNAME_SZ);

	/* Initialize the trace queue */
	if (!conf->max_trace_queue){
		conf->max_trace_queue=MAX_TRACE_QUEUE;
	}

	wan_trace_info_init(&chan->trace_info,conf->max_trace_queue);

	/* Initialize the receive bh handler */
	WAN_TASKLET_INIT((&chan->tasklet),
			 0, wp_bh, (unsigned long)chan);

	WAN_IFQ_INIT((&chan->rx_queue),0);

	//We don't need this any more
	chan->route_status = NO_ROUTE;
	chan->route_removed = 0;

	card->u.c.async_mode = conf->async_mode;

	DEBUG_EVENT("%s: Firmware running in HDLC STREAMING Mode\n",
		wandev->name);

	chan->common.prot_ptr=NULL;

	/* setup for asynchronous mode */
	if(conf->async_mode) {
		DEBUG_EVENT("%s: Configuring for asynchronous mode\n",
			wandev->name);

		if(card->u.c.comm_port == WANOPT_PRI) {
			DEBUG_EVENT("%s:Asynchronous mode on secondary port only\n",
					wandev->name);

			err=-EINVAL;
			goto new_if_error;
		}

		if (strcmp(conf->usedby, "API") == 0){
			DEBUG_EVENT("%s: Running in API Async Mode\n",
						wandev->name);

			chan->common.usedby = API;
			wan_reg_api(chan, dev, card->devname);
		}else{
			DEBUG_EVENT("%s: Invalid Async Operation Mode: %s\n",
				chan->if_name, conf->usedby);
			err=-EINVAL;
			goto new_if_error;
		}

		if(!card->wandev.clocking) {
			DEBUG_EVENT(
				"%s: Asynch. clocking must be 'Internal'\n",
				wandev->name);

			err=-EINVAL;
			goto new_if_error;
		}

		if((card->wandev.bps < MIN_ASY_BAUD_RATE) ||
			(card->wandev.bps > MAX_ASY_BAUD_RATE)) {
			DEBUG_EVENT("%s: Selected baud rate is invalid.\n",
				wandev->name);
			DEBUG_EVENT("Must be between %u and %u bps.\n",
				MIN_ASY_BAUD_RATE, MAX_ASY_BAUD_RATE);

			err=-EINVAL;
			goto new_if_error;
		}

		card->u.c.api_options = 0;
                if (conf->asy_data_trans == WANOPT_YES) {
                        card->u.c.api_options |= ASY_RX_DATA_TRANSPARENT;
                }

		card->u.c.protocol_options = 0;
		if (conf->rts_hs_for_receive == WANOPT_YES) {
			card->u.c.protocol_options |= ASY_RTS_HS_FOR_RX;
	        }
                if (conf->xon_xoff_hs_for_receive == WANOPT_YES) {
                        card->u.c.protocol_options |= ASY_XON_XOFF_HS_FOR_RX;
                }
                if (conf->xon_xoff_hs_for_transmit == WANOPT_YES) {
                        card->u.c.protocol_options |= ASY_XON_XOFF_HS_FOR_TX;
                }
                if (conf->dcd_hs_for_transmit == WANOPT_YES) {
                        card->u.c.protocol_options |= ASY_DCD_HS_FOR_TX;
                }
                if (conf->cts_hs_for_transmit == WANOPT_YES) {
                        card->u.c.protocol_options |= ASY_CTS_HS_FOR_TX;
                }

		card->u.c.tx_bits_per_char = conf->tx_bits_per_char;
                card->u.c.rx_bits_per_char = conf->rx_bits_per_char;
                card->u.c.stop_bits = conf->stop_bits;
		card->u.c.parity = conf->parity;
		card->u.c.break_timer = conf->break_timer;
		card->u.c.inter_char_timer = conf->inter_char_timer;
		card->u.c.rx_complete_length = conf->rx_complete_length;
		card->u.c.xon_char = conf->xon_char;

	}else{

#if defined(ENABLE_MULTPROT_MODE_WANPIPE)
		/* Setup wanpipe as a router (WANPIPE) or as an API */
		if( strcmp(conf->usedby, "WANPIPE") == 0) {

			DEBUG_EVENT("%s:%s: Interface running in WANPIPE mode!\n",
				wandev->name,chan->if_name);

			chan->common.usedby=WANPIPE;

			pppdev=wan_malloc(sizeof(struct ppp_device));
			if (!pppdev){
				err = -ENOMEM;
				goto new_if_error;
			}

			memset(pppdev,0,sizeof(struct ppp_device));

			chan->common.prot_ptr=(void*)pppdev;

			/* Attach PPP protocol layer to pppdev
			 * The wp_sppp_attach() will initilize the dev structure
			 * and setup ppp layer protocols.
			 * All we have to do is to bind in:
			 *    if_open(), if_close(), if_send() and get_stats() functions.
			 */

			pppdev->dev=dev;

			/* Get authentication info. */
			if(conf->pap == WANOPT_YES){
				pppdev->sppp.myauth.proto = PPP_PAP;
			}else if(conf->chap == WANOPT_YES){
				pppdev->sppp.myauth.proto = PPP_CHAP;
			}else{
				pppdev->sppp.myauth.proto = 0;
			}

			if(pppdev->sppp.myauth.proto){
				memcpy(pppdev->sppp.myauth.name, conf->userid, AUTHNAMELEN);
				memcpy(pppdev->sppp.myauth.secret, conf->passwd, AUTHNAMELEN);
			}

			pppdev->sppp.gateway = conf->gateway;
			if (conf->if_down){
				pppdev->sppp.dynamic_ip = 1;
			}

			sprintf(pppdev->sppp.hwdevname,"%s",card->devname);

			wp_sppp_attach(pppdev);
			sp = &pppdev->sppp;

			/* Enable PPP Debugging */
			if (conf->protocol == WANCONFIG_CHDLC){
				DEBUG_EVENT("%s: Starting Kernel CISCO HDLC protocol\n",
						card->devname);
				sp->pp_flags |= PP_CISCO;
				conf->ignore_dcd = WANOPT_YES;
				conf->ignore_cts = WANOPT_YES;
			}else{
				DEBUG_EVENT("%s: Starting Kernel Sync PPP protocol\n",
						card->devname);
				sp->pp_flags &= ~PP_CISCO;
				dev->type	= ARPHRD_PPP;
			}
		}else
#endif
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
		if (strcmp(conf->usedby, "ANNEXG") == 0) {
			DEBUG_EVENT("%s:%s: Interface running in ANNEXG mode!\n",
				wandev->name,chan->if_name);
			chan->common.usedby=ANNEXG;

			if (strlen(conf->label)){
				strncpy(chan->label,conf->label,WAN_IF_LABEL_SZ);
			}
		}else
#endif
		if (strcmp(conf->usedby, "STACK") == 0) {
			chan->common.usedby = STACK;
			DEBUG_EVENT( "%s:%s: Running in Stack mode.\n",
					card->devname,chan->if_name);

		}else if (strcmp(conf->usedby, "API") == 0) {
			chan->common.usedby = API;
			wan_reg_api(chan, dev, card->devname);
			DEBUG_EVENT( "%s:%s: Running in API mode.\n",
					card->devname,chan->if_name);

		}else{
			DEBUG_EVENT(
				"%s:%s: Error: Unsupported operation mode!\n",
				wandev->name,chan->if_name);
			err=-EINVAL;
			goto new_if_error;
		}
	}

	if (conf->single_tx_buf) {
		DEBUG_EVENT("%s: Enabling Single Tx Buffer \n",chan->if_name);
		card->u.c.protocol_options|=SINGLE_TX_BUFFER;
	}

	/* Get Multicast Information */
	chan->mc = conf->mc;

	/* prepare network device data space for registration */

	if (conf->ignore_dcd == WANOPT_YES || conf->ignore_cts == WANOPT_YES){
		DEBUG_EVENT("%s: Ignore modem changes DCD/CTS\n",card->devname);
		chan->ignore_modem=1;
	}else{
		DEBUG_EVENT("%s: Restart protocol on modem changes DCD/CTS\n",
				card->devname);
	}

	wan_netif_set_priv(dev, chan);
	chan->dev=dev;
#if defined(__LINUX__)
	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);
#else
	chan->common.iface.open      = &if_open;
        chan->common.iface.close     = &if_close;
        chan->common.iface.output    = &if_send;
        chan->common.iface.ioctl     = &if_do_ioctl;
        chan->common.iface.tx_timeout= &if_tx_timeout;
	if (wan_iface.attach){
		if (!ifunit(wan_netif_name(dev))){
			wan_iface.attach(dev, NULL, chan->common.is_netdev);
		}
	}else{
		DEBUG_EVENT("%s: Failed to attach interface %s!\n",
				card->devname, wan_netif_name(dev));
		wan_netif_set_priv(dev, NULL);
		err = -EINVAL;
		goto new_if_error;
	}
	wan_netif_set_mtu(dev, card->wandev.mtu);
#endif

#if defined(__LINUX__)
	/*
	 * Create interface file in proc fs.
	 */
	err = wanrouter_proc_add_interface(wandev,
					   &chan->dent,
					   card->u.c.if_name,
					   dev);
	if (err){
		DEBUG_EVENT(
			"%s: can't create /proc/net/router/mprot/%s entry!\n",
			card->devname, card->u.c.if_name);
		err = -ENOMEM;
		goto new_if_error;
	}
#endif
	return 0;


new_if_error:

	--card->wandev.new_if_cnt;
	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

	if (chan){
		chan->common.prot_ptr=NULL;
		wan_free(chan);
	}

#if defined(__LINUX__)
	if (pppdev){
		wp_sppp_detach(dev);
		wan_free(pppdev);
	}
#endif
	wan_netif_set_priv(dev, NULL);

	return err;
}




/*============================================================================
 * Delete logical channel.
 */
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	private_area_t	*chan = wan_netif_priv(dev);
	sdla_t		*card = chan->card;
	wan_smp_flag_t	smp_flags=0;

	chdlc_set_intr_mode(card, 0);

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		netdevice_t *tmp_dev;
		int err;

		DEBUG_EVENT("%s: Unregistering Lapb Protocol\n",wandev->name);

		if (!IS_FUNC_CALL(lapb_protocol,lapb_unregister)){
			wan_spin_lock_irq(&wandev->lock, &smp_flags);
			chan->annexg_dev = NULL;
			wan_spin_unlock_irq(&wandev->lock, &smp_flags);
			return 0;
		}

		wan_spin_lock_irq(&wandev->lock, &smp_flags);
		tmp_dev=chan->annexg_dev;
		chan->annexg_dev=NULL;
		wan_spin_unlock_irq(&wandev->lock, &smp_flags);

		if ((err=lapb_protocol.lapb_unregister(tmp_dev))){
			wan_spin_lock_irq(&wandev->lock, &smp_flags);
			chan->annexg_dev=tmp_dev;
			wan_spin_unlock_irq(&wandev->lock, &smp_flags);
			return err;
		}
	}
#endif

#if defined(__LINUX__)
	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, card->u.c.if_name);
#endif
	wan_spin_lock_irq(&wandev->lock,&smp_flags);

	if (chan->common.prot_ptr){
		/* Detach the PPP layer */
		DEBUG_EVENT("%s: Detaching SyncPPP Module from %s\n",
				wandev->name,wan_netif_name(dev));
#if defined(__LINUX__)
		wp_sppp_detach(dev);
#endif
	}

#if defined(__LINUX__)
	/* Bug Fix: Kernel 2.2.X
	 * We must manually remove the ioctl call binding
	 * since in some cases (mrouted) daemons continue
	 * to call ioctl() after the device has gone down */
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,NULL);

#endif

	if (chan->common.prot_ptr){
		wan_free(chan->common.prot_ptr);
		chan->common.prot_ptr= NULL;
	}

	chdlc_set_intr_mode(card, 0);

	if (card->u.c.comm_enabled){
		chdlc_comm_disable(card);
	}

	wan_spin_unlock_irq(&wandev->lock,&smp_flags);

	port_set_state(card, WAN_DISCONNECTED);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	if (wan_iface.detach){
		wan_iface.detach(dev, chan->common.is_netdev);
	}
#endif

	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
	}
	return 0;
}


/****** Network Device Interface ********************************************/

#if defined(__LINUX__)
/*============================================================================
 * Initialize Linux network interface.
 *
 * This routine is called only once for each interface, during Linux network
 * interface registration.  Returning anything but zero will fail interface
 * registration.
 */
static int if_init (netdevice_t* dev)
	{
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	wan_device_t* wandev = &card->wandev;

	/* NOTE: Most of the dev initialization was
         *       done in wp_sppp_attach(), called by new_if()
         *       function. All we have to do here is
         *       to link four major routines below.
         */

	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	dev->watchdog_timeo	= TX_TIMEOUT;
#endif

	if (!chan->common.prot_ptr){
		dev->flags     |= IFF_POINTOPOINT;
		dev->flags     |= IFF_NOARP;
		dev->type	= ARPHRD_PPP;
		dev->mtu		= card->wandev.mtu;
		dev->hard_header_len	= 0;
	}

	/* Overwrite the sppp ioctl, because we need to run
	 * our debugging commands via ioctl(). However
	 * call syncppp ioctl with in it :) */
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);

	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

	/* Set transmit buffer queue length
         * If we over fill this queue the packets will
         * be droped by the kernel.
         * wp_sppp_attach() sets this to 10, but
         * 100 will give us more room at low speeds.
	 */
        dev->tx_queue_len = 100;

	return 0;
}
#endif /* __LINUX__ */

/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this
	 * is only used as a last resort.
	 */

	++card->wandev.stats.collisions;

	chan->tick_counter = SYSTEM_TICKS;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);

	DEBUG_EVENT( "%s: Transmit timed out on %s\n",
				card->devname,wan_netif_name(dev));
	WAN_NETIF_WAKE_QUEUE (dev);
#if defined(__LINUX__)
	dev->trans_start = SYSTEM_TICKS;
#endif
	if (chan->common.usedby == STACK){
		wanpipe_lip_kick(chan,0);
	}
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
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;

	/* Only one open per interface is allowed */
#if defined(__LINUX__)
	if (open_dev_check(dev))
		return -EBUSY;
#endif

	/* Start PPP Layer */
	if (chan->common.prot_ptr){
#if defined(__LINUX__)
		if (wp_sppp_open(dev)){
			return -EIO;
		}
#endif
	}

	wan_getcurrenttime(&chan->router_start_time, NULL);

	WAN_NETIF_START_QUEUE(dev);
	WAN_NETIF_CARRIER_OFF(dev);

	if (chan->common.state == WAN_CONNECTED){
		WAN_NETIF_CARRIER_ON(dev);
	}

	wanpipe_open(card);

	card->u.c.timer_int_enabled |= TMR_INT_ENABLED_CONFIG;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);

	return 0;
}

/*============================================================================
 * Close network interface.
 * o if this is the last close, then disable communications and interrupts.
 * o reset flags.
 */
static int if_close (netdevice_t* dev)
{
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;

	WAN_TASKLET_KILL((&chan->tasklet));

	/* Stop the PPP Layer */
	if (chan->common.prot_ptr){
#if defined(__LINUX__)
		wp_sppp_close(dev);
#endif
	}

	WAN_NETIF_STOP_QUEUE(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif

	wanpipe_close(card);

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
#if defined(__LINUX__)
static int if_send (netskb_t* skb, netdevice_t* dev)
#else
static int if_send(netdevice_t *dev, netskb_t *skb, struct sockaddr *dst,struct rtentry *rt)
#endif
{
	private_area_t *chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	int udp_type = 0;
	wan_smp_flag_t smp_flags=0, smp_flags1=0;
	int err=0;

	if (skb == NULL){
		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		DEBUG_EVENT("%s: Received NULL skb buffer! interface %s got kicked!\n",
			card->devname, wan_netif_name(dev));

		WAN_NETIF_WAKE_QUEUE(dev);
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

		if((SYSTEM_TICKS - chan->tick_counter) < (5 * HZ)) {
			return 1;
		}
		if_tx_timeout(dev);
	}
#else
	WAN_NETIF_STOP_QUEUE(dev);
#endif

   	if (chan->common.usedby != ANNEXG &&
	    chan->common.usedby != STACK
#if defined(__LINUX__)
		 &&
	    ntohs(skb->protocol) != htons(PVC_PROT)
#endif
		){
		/* check the udp packet type */

		udp_type = udp_pkt_type(skb, card);
		if (udp_type == UDP_CPIPE_TYPE){
			if (store_udp_mgmt_pkt(UDP_PKT_FRM_STACK, card, skb, dev,
                                chan)){
				card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
			}
			WAN_NETIF_START_QUEUE(dev);
			return 0;
		}
        }

	/* Lock the 508 Card: SMP is supported */
      	if(card->type != SDLA_S514){
		s508_lock(card,&smp_flags,&smp_flags1);
	}

	err=0;

    	if (wan_test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)){

		DEBUG_EVENT("%s: Critical in if_send: %lx\n",
					card->wandev.name,card->wandev.critical);
                ++card->wandev.stats.tx_dropped;
		WAN_NETIF_START_QUEUE(dev);
		goto if_send_crit_exit;
	}

	if (card->u.c.state != WAN_CONNECTED){
		++card->wandev.stats.tx_carrier_errors;
		WAN_NETIF_START_QUEUE(dev);
		goto if_send_crit_exit;
	}

	/* If it's an API packet pull off the API
	 * header. Also check that the packet size
	 * is larger than the API header
	 */
	if (chan->common.usedby == API){
		/* discard the frame if we are configured for */
		/* 'receive only' mode or if there is no data */
		if (card->u.c.receive_only ||
			(wan_skb_len(skb) <= sizeof(api_tx_hdr_t))) {

			++card->wandev.stats.tx_dropped;
			goto if_send_crit_exit;
		}

		wan_skb_pull(skb,sizeof(api_tx_hdr_t));
	}

	if ((err=chdlc_send(card, wan_skb_data(skb), wan_skb_len(skb)))){
		if(card->type == SDLA_S514){
			wan_smp_flag_t flags1;
			wan_spin_lock_irq(&card->wandev.lock,&flags1);
			err=1;
			WAN_NETIF_STOP_QUEUE(dev);
			wan_spin_unlock_irq(&card->wandev.lock,&flags1);
		} else {
			err=1;
			WAN_NETIF_STOP_QUEUE(dev);
		}
	}else{
		++card->wandev.stats.tx_packets;
       		card->wandev.stats.tx_bytes += wan_skb_len(skb);
#if defined(__LINUX__)
		dev->trans_start = SYSTEM_TICKS;
#endif
		WAN_NETIF_START_QUEUE(dev);

		wan_capture_trace_packet(card, &chan->trace_info, skb, TRC_OUTGOING_FRM);
	}

if_send_crit_exit:
	if (err==0){
                wan_skb_free(skb);
	}else{
		chan->tick_counter = SYSTEM_TICKS;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
	}

	wan_clear_bit(SEND_CRIT, (void*)&card->wandev.critical);
	if(card->type != SDLA_S514){
		s508_unlock(card,&smp_flags,&smp_flags1);
	}

	return err;
}

/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
#if defined(__LINUX__)
static struct net_device_stats gstats;
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	sdla_t *my_card;
	private_area_t* chan;

	/* Shutdown bug fix. In del_if() we kill
         * dev->priv pointer. This function, gets
         * called after del_if(), thus check
         * if pointer has been deleted */
	if ((chan=wan_netif_priv(dev)) == NULL)
		return &gstats;

	my_card = chan->card;
	if (!my_card){
		return &gstats;
	}

	return &my_card->wandev.stats;
}
#endif

/****** Cisco HDLC Firmware Interface Functions *******************************/

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str.
 */
static int chdlc_read_version (sdla_t* card, char* str)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int len;
	char err;
	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		chdlc_error(card,err,mb);
	}
	else if (str) {  /* is not null */
		len = mb->wan_data_len;
		memcpy(str, mb->wan_data, len);
		str[len] = '\0';
	}
	return (err);
}

/*-----------------------------------------------------------------------------
 *  Configure CHDLC firmware.
 */
static int chdlc_configure (sdla_t* card, void* data)
{
	int err;
	wan_mbox_t *mb = &card->wan_mbox;
	int data_length = sizeof(CHDLC_CONFIGURATION_STRUCT);

	mb->wan_data_len = data_length;
	memcpy(mb->wan_data, data, data_length);
	mb->wan_command = SET_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if (err != COMMAND_OK) chdlc_error (card, err, mb);

	return err;
}


/*============================================================================
 * Set interrupt mode -- HDLC Version.
 */

static int chdlc_set_intr_mode (sdla_t* card, unsigned mode)
{
	wan_mbox_t* mb = &card->wan_mbox;
	CHDLC_INT_TRIGGERS_STRUCT* int_data =
		 (CHDLC_INT_TRIGGERS_STRUCT *)mb->wan_data;
	int err;

	DEBUG_TEST("%s: Setting interrupt mode %02X\n", card->devname, mode);
	int_data->CHDLC_interrupt_triggers 	= mode;
	int_data->IRQ				= card->wandev.irq;	//ALEX_TODAY card->hw.irq;
	int_data->interrupt_timer               = 1;

	mb->wan_data_len = sizeof(CHDLC_INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_CHDLC_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error (card, err, mb);
	return err;
}


/*============================================================================
 * Enable communications.
 */

static int chdlc_comm_enable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_CHDLC_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error(card, err, mb);
	else
		card->u.c.comm_enabled=1;

	return err;
}

/*============================================================================
 * Disable communications and Drop the Modem lines (DCD and RTS).
 */
static int chdlc_comm_disable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = DISABLE_CHDLC_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error(card,err,mb);

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

/*
 ************************************************************************
 * Read CHDLC configureation.
 */
static int chdlc_read_configure(sdla_t *card, CHDLC_CONFIGURATION_STRUCT* cfg)
{
	wan_mbox_t*	mbox = &card->wan_mbox;
	int		err;

	WAN_MBOX_INIT(mbox);
	mbox->wan_data_len = 0;
	mbox->wan_command = READ_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	if(err != WAN_CMD_OK){
		chdlc_error(card, err, mbox);
	}else{
		DEBUG_CMD("%s: Read CHDLC configuration...ok\n",
				card->devname);
		//bcopy(mbox->wan_data, cfg, sizeof(CHDLC_CONFIGURATION_STRUCT));
		memcpy(cfg, mbox->wan_data, sizeof(CHDLC_CONFIGURATION_STRUCT));
	}
	return err;
}


/*============================================================================
 * Read CHDLC operational statistics.
 */
static int chdlc_read_op_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_CHDLC_OPERATIONAL_STATS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                chdlc_error(card,err,mb);
        return err;
}


/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card,
	private_area_t* chan)
{
        wan_mbox_t* mb = &card->wan_mbox;
  	COMMS_ERROR_STATS_STRUCT* err_stats;
        CHDLC_OPERATIONAL_STATS_STRUCT *op_stats;

	/* 1. On the first timer interrupt, update T1/E1 alarms
	 * and PMON counters (only for T1/E1 card) (TE1)
	 */
	/* TE1 Update T1/E1 alarms */
	if (IS_TE1_CARD(card)) {
		card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		/* TE1 Update T1/E1 perfomance counters */
		card->wandev.fe_iface.read_pmon(&card->fe, 0);
	}else if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
	}

	if(chdlc_read_comm_err_stats(card))
		return 1;
	err_stats = (COMMS_ERROR_STATS_STRUCT *)mb->wan_data;
	card->wandev.stats.rx_over_errors =
			err_stats->Rx_overrun_err_count;
	card->wandev.stats.rx_crc_errors =
			err_stats->CRC_err_count;
	card->wandev.stats.rx_frame_errors =
			err_stats->Rx_abort_count;
	card->wandev.stats.rx_fifo_errors =
			err_stats->Rx_dis_pri_bfrs_full_count;
	card->wandev.stats.rx_missed_errors =
			card->wandev.stats.rx_fifo_errors;
	card->wandev.stats.tx_aborted_errors =
			err_stats->sec_Tx_abort_count;

        /* on the second timer interrupt, read the operational statistics */
       	if(chdlc_read_op_stats(card))
               	return 1;

	op_stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)mb->wan_data;
	card->wandev.stats.rx_length_errors =
		(op_stats->Rx_Data_discard_short_count +
		op_stats->Rx_Data_discard_long_count);

	return 0;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int chdlc_send (sdla_t* card, void* data, unsigned len)
{
	CHDLC_DATA_TX_STATUS_EL_STRUCT txbuf;

	card->hw_iface.peek(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));

	if (txbuf.opp_flag){
		return 1;
	}

	card->hw_iface.poke(card->hw, txbuf.ptr_data_bfr, data, len);

	txbuf.frame_length = len;
	card->hw_iface.poke(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));

	txbuf.opp_flag = 1;		/* start transmission */
	card->hw_iface.poke_byte(card->hw,
				 card->u.c.txbuf_off+offsetof(CHDLC_DATA_TX_STATUS_EL_STRUCT, opp_flag),
				 txbuf.opp_flag);

	/* Update transmit buffer control fields */
	card->u.c.txbuf_off += sizeof(txbuf);

	if (card->u.c.txbuf_off > card->u.c.txbuf_last_off)
		card->u.c.txbuf_off = card->u.c.txbuf_base_off;
	return 0;
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
		DEBUG_EVENT("%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		break;

	case S514_BOTH_PORTS_SAME_CLK_MODE:
		if(cmd == SET_CHDLC_CONFIGURATION) {
			DEBUG_EVENT(
			 "%s: Configure both ports for the same clock source\n",
				card->devname);
			break;
		}

	default:
		DEBUG_EVENT("%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

/****** Interrupt Handlers **************************************************/

/*============================================================================
 * Cisco HDLC interrupt service routine.
 */
STATIC WAN_IRQ_RETVAL wsppp_isr (sdla_t* card)
{
	netdevice_t* dev;
	SHARED_MEMORY_INFO_STRUCT flags;
	private_area_t *chan;
	int i;

	/* Check for which port the interrupt has been generated
	 * Since Secondary Port is piggybacking on the Primary
         * the check must be done here.
	 */

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	/* Start card isr critical area */
	wan_set_bit(0,&card->in_isr);
	card->spurious = 0;

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	/* If we get an interrupt with no network device, stop the interrupts
	 * and issue an error */
	if ((!dev || !wan_netif_priv(dev)) && flags.interrupt_info_struct.interrupt_type !=
	    	COMMAND_COMPLETE_APP_INT_PEND){
		goto isr_done;
	}

	/* if critical due to peripheral operations
	 * ie. update() or getstats() then reset the interrupt and
	 * wait for the board to retrigger.
	 */
	if(wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
		goto isr_done;
	}


	/* On a 508 Card, if critical due to if_send
         * Major Error !!!
	 */
	if(card->type != SDLA_S514) {
		if(wan_test_bit(0, (void*)&card->wandev.critical)) {
			DEBUG_EVENT("%s: Critical while in ISR: %lx\n",
				card->devname, card->wandev.critical);
			goto isr_done;
		}
	}

	switch(flags.interrupt_info_struct.interrupt_type) {

		case RX_APP_INT_PEND:	/* 0x01: receive interrupt */
			rx_intr(card);
			break;

		case TX_APP_INT_PEND:	/* 0x02: transmit interrupt */

			chan=wan_netif_priv(dev);

			card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
			if (chan->common.usedby == STACK){
				WAN_NETIF_START_QUEUE(dev);
				wanpipe_lip_kick(chan,0);
				break;
			}

			if (chan->common.usedby == API){
				WAN_NETIF_START_QUEUE(dev);
				wan_wakeup_api(chan);
				break;
			}

			if (chan->common.usedby == API){
				WAN_NETIF_WAKE_QUEUE(dev);
                         	wan_wakeup_api(chan);
			}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			if (chan->common.usedby == ANNEXG &&
				  chan->annexg_dev){

				WAN_NETIF_START_QUEUE(dev);
				if (IS_FUNC_CALL(lapb_protocol,lapb_mark_bh)){
					lapb_protocol.lapb_mark_bh(chan->annexg_dev);
				}
				break;
			}
#endif

			WAN_NETIF_WAKE_QUEUE(dev);
			break;

		case COMMAND_COMPLETE_APP_INT_PEND:/* 0x04: cmd cplt */
			++card->timer_int_enabled;
			break;

		case CHDLC_EXCEP_COND_APP_INT_PEND:	/* 0x20 */
			process_chdlc_exception(card);
			break;

		case GLOBAL_EXCEP_COND_APP_INT_PEND:

			process_global_exception(card);

			/* Reset the 56k or T1/E1 front end exception condition */
			if(IS_56K_CARD(card) || IS_TE1_CARD(card)) {
				FRONT_END_STATUS_STRUCT FE_status;
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
				wan_set_bit(0,&card->spurious);
				break;
			}

			DEBUG_EVENT("%s: spurious interrupt 0x%02X!\n",
				card->devname,
				flags.interrupt_info_struct.interrupt_type);
			DEBUG_EVENT("Code name: ");
			for(i = 0; i < 4; i ++)
				_DEBUG_EVENT("%c",
					flags.global_info_struct.codename[i]);
			DEBUG_EVENT("\n");
			DEBUG_EVENT("Code version: ");
			for(i = 0; i < 4; i ++)
				_DEBUG_EVENT("%c",
					flags.global_info_struct.codeversion[i]);
			_DEBUG_EVENT("\n");
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
	private_area_t *chan;
	SHARED_MEMORY_INFO_STRUCT flags;
	CHDLC_DATA_RX_STATUS_EL_STRUCT rxbuf;
	netskb_t *skb;
	unsigned len;
	unsigned addr;
	void *buf;
	int i,udp_type;
	unsigned int tmp_prev_error;


	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	card->hw_iface.peek(card->hw, card->u.c.rxmb_off, &rxbuf, sizeof(rxbuf));
	addr = rxbuf.ptr_data_bfr;
	if (rxbuf.opp_flag != 0x01) {
		DEBUG_EVENT(
			"%s: corrupted Rx buffer @ 0x%X, flag = 0x%02X!\n",
			card->devname, (unsigned)card->u.c.rxmb_off, rxbuf.opp_flag);
                DEBUG_EVENT("Code name: ");
                for(i = 0; i < 4; i ++)
                        DEBUG_EVENT("%c",
                                flags.global_info_struct.codename[i]);
		DEBUG_EVENT("\n");
                DEBUG_EVENT("Code version: ");
                for(i = 0; i < 4; i ++)
                        DEBUG_EVENT("%c",
                                flags.global_info_struct.codeversion[i]);
                DEBUG_EVENT("\n");

		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it measn that driver
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash.
                 */
		DEBUG_EVENT("%s: Critical router failure ...!!!\n",
				card->devname);
		DEBUG_EVENT("Please contact Sangoma Technologies !\n");
		chdlc_set_intr_mode(card,0);
		return;
	}

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (!netif_running(dev)){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}
#elif defined(__LINUX__)
	if (!dev->start){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}
#endif

	chan = wan_netif_priv(dev);

	tmp_prev_error=chan->prev_error;
	chan->prev_error=rxbuf.error_flag;

	if (rxbuf.error_flag){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

	if (tmp_prev_error){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (chan->common.usedby == ANNEXG){
		if (rxbuf.frame_length <= 2 || rxbuf.frame_length > 4103){
			DEBUG_EVENT("%s: Bad Rx Frame Length %i\n",
					card->devname,rxbuf.frame_length);
			++card->wandev.stats.rx_dropped;
			goto rx_exit;
		}
	} else
#endif
	if (chan->common.usedby == STACK){
		if (rxbuf.frame_length <= 2 || rxbuf.frame_length > 4103){
			++card->wandev.stats.rx_dropped;
			DEBUG_EVENT("%s: Bad Rx Frame Length %i\n",
					card->devname,rxbuf.frame_length);
			goto rx_exit;
		}
	}else{
		if (rxbuf.frame_length <= 2 || rxbuf.frame_length > 4103){
			++card->wandev.stats.rx_dropped;
			goto rx_exit;
		}
	}

	/* Make sure that data len is greater than CRC_LEN
	 * before we remove the CRC bytes */
	if (rxbuf.frame_length <= CRC_LENGTH){
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

	/* Take off two CRC bytes */
	if (chan->common.usedby == API){
		len = rxbuf.frame_length;
	}else{
		len = rxbuf.frame_length - CRC_LENGTH;
	}

	/* Allocate socket buffer */
	skb = wan_skb_alloc(len+2);
	if (skb == NULL) {
		if (WAN_NET_RATELIMIT()){
			DEBUG_ERROR("%s: Error: No memory available!\n",
						card->devname);
		}
		++card->wandev.stats.rx_dropped;
		goto rx_exit;
	}

	/* Copy data to the socket buffer */
	if ((addr + len) > card->u.c.rx_top_off + 1) {
		unsigned tmp = card->u.c.rx_top_off - addr + 1;
		buf = wan_skb_put(skb, tmp);
		card->hw_iface.peek(card->hw, addr, buf, tmp);
		addr = card->u.c.rx_base_off;
		len -= tmp;
	}

	buf = wan_skb_put(skb, len);
	card->hw_iface.peek(card->hw, addr, buf, len);

	wan_capture_trace_packet(card, &chan->trace_info, skb, TRC_INCOMING_FRM);

	udp_type = udp_pkt_type(skb, card);

	if(udp_type == UDP_CPIPE_TYPE) {

		card->wandev.stats.rx_packets ++;
		card->wandev.stats.rx_bytes += wan_skb_len(skb);
#if defined(__LINUX__)
		skb->protocol = htons(ETH_P_IP);
#endif
		if(store_udp_mgmt_pkt(UDP_PKT_FRM_NETWORK,
   				      card, skb, dev, chan)) {
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
		}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	}else if (chan->common.usedby == ANNEXG){

		if (chan->annexg_dev){
			skb->protocol = htons(ETH_P_X25);
			skb->dev = chan->annexg_dev;
			wan_skb_reset_mac_header(skb);

			if (wan_skb_queue_len(&chan->rx_queue) > MAX_RX_QUEUE){
				wan_skb_free(skb);
				if (WAN_NET_RATELIMIT()){
					DEBUG_ERROR("%s: Error Rx queue full, dropping pkt!\n",
							card->devname);
				}
				++card->wandev.stats.rx_dropped;
				goto rx_exit;
			}

			wan_skb_queue_tail(&chan->rx_queue,skb);

			WAN_TASKLET_SCHEDULE((&chan->tasklet));

		}else{
			wan_skb_free(skb);
			++card->wandev.stats.rx_errors;
		}
#endif
	}else if (chan->common.usedby == STACK){
		wan_skb_set_dev(skb, chan->annexg_dev);	//FIXME: Nenad please check this line!!!!
		wan_skb_reset_mac_header(skb);

		if (wan_skb_queue_len(&chan->rx_queue) > MAX_RX_QUEUE){
			wan_skb_free(skb);
			if (WAN_NET_RATELIMIT()){
				DEBUG_ERROR("%s: Error:(STACK) Rx queue full, dropping pkt!\n",
						card->devname);
			}
			++card->wandev.stats.rx_dropped;
			goto rx_exit;
		}

		wan_skb_queue_tail(&chan->rx_queue,skb);

		WAN_TASKLET_SCHEDULE((&chan->tasklet));
	}else{

		if (wan_skb_queue_len(&chan->rx_queue) > MAX_RX_QUEUE){
			wan_skb_free(skb);
			if (WAN_NET_RATELIMIT()){
				DEBUG_ERROR("%s: Error Rx queue full, dropping pkt!\n",
						card->devname);
			}
			++card->wandev.stats.rx_dropped;
			goto rx_exit;
		}

		/* Pass it up the protocol stack */
#if defined(__LINUX__)
		wan_skb_set_protocol(skb, ETH_P_WAN_PPP);
#endif
		wan_skb_set_dev(skb, dev);
		wan_skb_reset_mac_header(skb);

		wan_skb_queue_tail(&chan->rx_queue,skb);
		WAN_TASKLET_SCHEDULE((&chan->tasklet));
	}

rx_exit:
	/* Release buffer element and calculate a pointer to the next one */
	rxbuf.opp_flag = 0x00;
	card->hw_iface.poke(card->hw, card->u.c.rxmb_off, &rxbuf, sizeof(rxbuf));
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
        netdevice_t	*dev;
        private_area_t	*chan = NULL;

        dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){
		return;
	}

        if (!(chan = wan_netif_priv(dev))){
		return;
	}

	/* TE timer interrupt */
	if (card->u.c.timer_int_enabled & TMR_INT_ENABLED_TE) {
		card->wandev.fe_iface.polling(&card->fe);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}

	if (card->u.c.timer_int_enabled & TMR_INT_ENABLED_CONFIG) {
		config_chdlc(card);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_CONFIG;
	}

	/* process a udp call if pending */
       	if(card->u.c.timer_int_enabled & TMR_INT_ENABLED_UDP) {
               	process_udp_mgmt_pkt(card, dev,
                       chan,0);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_UDP;
        }

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
	cfg.asy_API_options 		= card->u.c.api_options;
	cfg.asy_protocol_options	= card->u.c.protocol_options;
	cfg.Tx_bits_per_char		= card->u.c.tx_bits_per_char;
	cfg.Rx_bits_per_char		= card->u.c.rx_bits_per_char;
	cfg.stop_bits			= card->u.c.stop_bits;
	cfg.parity			= card->u.c.parity;
	cfg.break_timer			= card->u.c.break_timer;
	cfg.asy_Rx_inter_char_timer	= card->u.c.inter_char_timer;
	cfg.asy_Rx_complete_length	= card->u.c.rx_complete_length;
	cfg.XON_char			= card->u.c.xon_char;
	cfg.XOFF_char			= card->u.c.xoff_char;
	cfg.asy_statistics_options	= (CHDLC_TX_DATA_BYTE_COUNT_STAT |
		CHDLC_RX_DATA_BYTE_COUNT_STAT);

	mb->wan_data_len = sizeof(ASY_CONFIGURATION_STRUCT);
	memcpy(mb->wan_data, &cfg, mb->wan_data_len);
	mb->wan_command = SET_ASY_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		chdlc_error (card, err, mb);

	if (err == 0x4F){
		DEBUG_ERROR("%s: Error: ASYNC Not Supported by Firmware!\n",
				card->devname);
	}
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




/*------------------------------------------------------------------------------
  Miscellaneous Functions
	- set_chdlc_config() used to set configuration options on the board
------------------------------------------------------------------------------*/

static int set_chdlc_config(sdla_t* card)
{

	CHDLC_CONFIGURATION_STRUCT cfg;

	memset(&cfg, 0, sizeof(CHDLC_CONFIGURATION_STRUCT));

	if(card->wandev.clocking)
		cfg.baud_rate = card->wandev.bps;

	cfg.line_config_options = (card->wandev.electrical_interface == WANOPT_RS232) ?
		INTERFACE_LEVEL_RS232 : INTERFACE_LEVEL_V35;

	cfg.modem_config_options	= 0;
	//API OPTIONS
	cfg.CHDLC_API_options		= 0;
	cfg.modem_status_timer		= 100;
	cfg.CHDLC_protocol_options	= card->u.c.protocol_options | HDLC_STREAMING_MODE;
	cfg.percent_data_buffer_for_Tx  = 50;
	cfg.CHDLC_statistics_options	= (CHDLC_TX_DATA_BYTE_COUNT_STAT |
		CHDLC_RX_DATA_BYTE_COUNT_STAT);
	cfg.max_CHDLC_data_field_length	= card->wandev.mtu+CRC_LENGTH;

	cfg.transmit_keepalive_timer	= 0;
	cfg.receive_keepalive_timer	= 0;
	cfg.keepalive_error_tolerance	= 0;
	cfg.SLARP_request_timer		= 0;

	cfg.IP_address		= 0;
	cfg.IP_netmask		= 0;

	return chdlc_configure(card, &cfg);
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
				card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);

				handle_front_end_state(card);
				break;
			}

			if (IS_TE1_CARD(card)) {
				/* TE_INTR */
				card->wandev.fe_iface.isr(&card->fe);
				handle_front_end_state(card);
				break;
			}

			if ((mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) == (DCD_HIGH | CTS_HIGH)){
				card->fe.fe_status = FE_CONNECTED;
			}else{
				card->fe.fe_status = FE_DISCONNECTED;
			}

			DEBUG_EVENT("%s: Modem status change\n",
				card->devname);

			switch(mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) {
				case (DCD_HIGH):
					DEBUG_EVENT("%s: DCD high, CTS low\n",card->devname);
					break;
				case (CTS_HIGH):
                                        DEBUG_EVENT("%s: DCD low, CTS high\n",card->devname);
                                        break;
                                case ((DCD_HIGH | CTS_HIGH)):
                                        DEBUG_EVENT("%s: DCD high, CTS high\n",card->devname);
                                        break;
				default:
                                        DEBUG_EVENT("%s: DCD low, CTS low\n",card->devname);
                                        break;
			}

			handle_front_end_state(card);
			break;

                case EXCEP_TRC_DISABLED:
                        DEBUG_EVENT("%s: Line trace disabled\n",
				card->devname);
                        break;

		case EXCEP_IRQ_TIMEOUT:
			DEBUG_EVENT("%s: IRQ timeout occurred\n",
				card->devname);
			break;

                default:
                        DEBUG_EVENT("%s: Global exception %x\n",
				card->devname, mb->wan_return_code);
                        break;
                }
	}
	return 0;
}

static void handle_front_end_state(void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	if (card->fe.fe_status == FE_CONNECTED){
		if (card->u.c.state == WAN_CONNECTED){
			port_set_state(card,WAN_CONNECTED);
		}
	}else{
		if (!IS_56K_CARD(card)) {
			netdevice_t	*dev;
			dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			send_ppp_term_request(dev);
			port_set_state(card,WAN_DISCONNECTED);
		} else {
			card->wandev.state = WAN_DISCONNECTED;
		}
	}
}




/*============================================================================
 * Process chdlc exception condition
 */
static int process_chdlc_exception(sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_EXCEPTION_CONDITION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(err != CMD_TIMEOUT) {

		switch (err) {

		case EXCEP_LINK_ACTIVE:
			card->u.c.state = WAN_CONNECTED;
			DEBUG_EVENT("%s: Exception condition: Link active!\n",
					card->devname);

			if (IS_56K_CARD(card)) {
				port_set_state(card, WAN_CONNECTED);
				if (card->fe.fe_status != FE_CONNECTED) {
					card->wandev.state = WAN_DISCONNECTED;
				}
			} else {
				if (card->wandev.ignore_front_end_status == WANOPT_YES ||
				card->fe.fe_status == FE_CONNECTED){
					port_set_state(card, WAN_CONNECTED);
				}
			}
			break;

		case EXCEP_LINK_INACTIVE_MODEM:
			DEBUG_EVENT("%s: Exception condition: Link In-active!\n",
					card->devname);
			card->u.c.state = WAN_DISCONNECTED;
			port_set_state(card, WAN_DISCONNECTED);
			break;

		case EXCEP_LOOPBACK_CONDITION:
			DEBUG_EVENT("%s: Loopback Condition Detected.\n",
						card->devname);
			break;

		case NO_CHDLC_EXCEP_COND_TO_REPORT:
			DEBUG_EVENT("%s: No exceptions reported.\n",
						card->devname);
			break;
		default:
			DEBUG_EVENT("%s: Exception Condition %x!\n",
					card->devname,err);
			break;
		}

	}
	return 0;
}


/*=============================================================================
 * Store a UDP management packet for later processing.
 */

static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                netskb_t *skb, netdevice_t* dev,
                                private_area_t* chan )
{
	int udp_pkt_stored = 0;

	if(!wan_atomic_read(&chan->udp_pkt_len) &&
	  (wan_skb_len(skb) <= MAX_LGTH_UDP_MGNT_PKT)) {
		wan_atomic_set(&chan->udp_pkt_len, wan_skb_len(skb));
		chan->udp_pkt_src = udp_pkt_src;
       		memcpy(chan->udp_pkt_data, wan_skb_data(skb)+4, wan_skb_len(skb)-4);
		card->u.c.timer_int_enabled = TMR_INT_ENABLED_UDP;
		udp_pkt_stored = 1;
	}

	wan_skb_free(skb);

	return(udp_pkt_stored);
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
static int
if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t cmd)
{
	private_area_t* chan= (private_area_t*)wan_netif_priv(dev);
	wan_smp_flag_t smp_flags;
	sdla_t *card;
	int err=-EINVAL;
	wan_udp_pkt_t *wan_udp_pkt;

	if (!chan){
		return -ENODEV;
	}
	card=chan->card;

	switch(cmd)
	{
#if defined(__LINUX__)
		case SIOC_WANPIPE_BIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}

			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
			break;

		case SIOC_WANPIPE_UNBIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}

			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
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

		case SIOC_MBOX_CMD:

			NET_ADMIN_CHECK();

			{
				wan_mbox_t *usr_mbox = wan_kmalloc(sizeof(wan_mbox_t));
				if (!usr_mbox) {
					return -ENOMEM;
				}
				if (WAN_COPY_FROM_USER(usr_mbox,ifr->ifr_data,sizeof(wan_mbox_t))){
					wan_free(usr_mbox);
					return -EFAULT;
				}

				wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
				err=wp_exec_usr_mbox_cmd(card, usr_mbox);
				wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

				if (WAN_COPY_TO_USER(ifr->ifr_data,usr_mbox,sizeof(wan_mbox_t))){
					wan_free(usr_mbox);
					return -EFAULT;
				}
				wan_free(usr_mbox);
			}


			return err;

#endif
		case SIOC_WANPIPE_PIPEMON:

			NET_ADMIN_CHECK();

			if (wan_atomic_read(&chan->udp_pkt_len) != 0){
				return -EBUSY;
			}

			wan_atomic_set(&chan->udp_pkt_len,MAX_LGTH_UDP_MGNT_PKT);

			/* For performance reasons test the critical
			 * here before spin lock */
			if (wan_test_bit(0,&card->in_isr)){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EBUSY;
			}


			wan_udp_pkt=(wan_udp_pkt_t*)chan->udp_pkt_data;
			if (WAN_COPY_FROM_USER(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			if (wan_udp_pkt->wan_udp_command == DIGITAL_LOOPTEST) {
				process_udp_mgmt_pkt(card,dev,chan,1);
			} else {
				wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

				process_udp_mgmt_pkt(card,dev,chan,1);

				wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
			}

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */

			if (wan_atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				DEBUG_ERROR("%s: Error: Pipemon buf too bit on the way up! %i\n",
						card->devname,wan_atomic_read(&chan->udp_pkt_len));
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EINVAL;
			}

			if (WAN_COPY_TO_USER(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			wan_atomic_set(&chan->udp_pkt_len,0);
			return 0;


		default:
#if defined(__LINUX__)
			if (chan->common.prot_ptr){
				return wp_sppp_do_ioctl(dev,ifr,cmd);
			}
#endif
			return -EINVAL;
	}
	return err;
}


/*=============================================================================
 * Process UDP management packet.
 */

static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				private_area_t* chan, int local_dev )
{
#if defined(__LINUX__)
	unsigned char *buf;
	unsigned int len;
	netskb_t *new_skb;
#endif
	unsigned short buffer_length;
	int udp_mgmt_req_valid = 1;
	wan_mbox_t *mb = &card->wan_mbox;
	SHARED_MEMORY_INFO_STRUCT flags;
	wan_udp_pkt_t *wan_udp_pkt;
	int err;
	netskb_t *skb;

	wan_udp_pkt = (wan_udp_pkt_t *) chan->udp_pkt_data;

	if (!local_dev){

		if(chan->udp_pkt_src == UDP_PKT_FRM_NETWORK){

			/* Only these commands are support for remote debugging.
			 * All others are not */
			switch(wan_udp_pkt->wan_udp_command) {

				case READ_GLOBAL_STATISTICS:
				case READ_MODEM_STATUS:
				case READ_CHDLC_LINK_STATUS:
				case CPIPE_ROUTER_UP_TIME:
				case READ_COMMS_ERROR_STATS:
				case READ_CHDLC_OPERATIONAL_STATS:
				/* These two commands are executed for
				 * each request */
				case READ_CHDLC_CONFIGURATION:
				case READ_CHDLC_CODE_VERSION:
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

		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT(
			"%s: Warning, Illegal UDP command attempted from network: %x\n",
			card->devname,wan_udp_pkt->wan_udp_command);
		}

   	} else {

		wan_udp_hdr_t* udp_hdr = &wan_udp_pkt->wan_udp_hdr;
		wan_trace_t *trace_info = &chan->trace_info;

		wan_udp_pkt->wan_udp_opp_flag = 0;

		switch(wan_udp_pkt->wan_udp_command) {

		case DIGITAL_LOOPTEST:
			wan_udp_pkt->wan_udp_return_code =
					digital_loop_test(card,wan_udp_pkt);
			break;

		case CPIPE_ENABLE_TRACING:

			udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
			udp_hdr->wan_udphdr_data_len = 0;

			if (!wan_test_bit(0,&trace_info->tracing_enabled)){

				trace_info->trace_timeout = SYSTEM_TICKS;

				wan_trace_purge(trace_info);
				DEBUG_UDP("%s: Traceing enabled!\n",
							card->devname);
				wan_set_bit (0,&trace_info->tracing_enabled);

			}else{
				DEBUG_ERROR("%s: Error: Trace running!\n",
						card->devname);
				udp_hdr->wan_udphdr_return_code = 2;
			}

			break;


		case CPIPE_DISABLE_TRACING:


			udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;

			if(wan_test_bit(0,&trace_info->tracing_enabled)) {

				wan_clear_bit(0,&trace_info->tracing_enabled);
				wan_trace_purge(trace_info);

				DEBUG_UDP("%s: Disabling ADSL trace\n",
							card->devname);

			}else{
				/* set return code to line trace already
				   disabled */
				udp_hdr->wan_udphdr_return_code = 1;
			}

			break;

		case CPIPE_GET_TRACE_INFO:

			if(wan_test_bit(0,&trace_info->tracing_enabled)){
				trace_info->trace_timeout = SYSTEM_TICKS;
			}else{
				DEBUG_ERROR("%s: Error trace not enabled\n",
						card->devname);
				/* set return code */
				udp_hdr->wan_udphdr_return_code = 1;
				break;
			}

			buffer_length = 0;
			udp_hdr->wan_udphdr_chdlc_num_frames = 0;
			udp_hdr->wan_udphdr_chdlc_ismoredata = 0;

#if defined(__FreeBSD__) || defined(__OpenBSD__)
			while (wan_skb_queue_len(&trace_info->trace_queue)){
				WAN_IFQ_POLL(&trace_info->trace_queue, skb);
				if (skb == NULL){
					DEBUG_EVENT("%s: No more trace packets in trace queue!\n",
								card->devname);
					break;
				}
				if ((WAN_MAX_DATA_SIZE - buffer_length) < skb->m_pkthdr.len){
					/* indicate there are more frames on board & exit */
					udp_hdr->wan_udphdr_chdlc_ismoredata = 0x01;
					break;
				}

				m_copydata(skb,
					   0,
					   skb->m_pkthdr.len,
					   &udp_hdr->wan_udphdr_data[buffer_length]);
				buffer_length += skb->m_pkthdr.len;
				WAN_IFQ_DEQUEUE(&trace_info->trace_queue, skb);
				if (skb){
					m_freem(skb);
				}
				udp_hdr->wan_udphdr_chdlc_num_frames++;
			}
#elif defined(__LINUX__)
			while ((skb=skb_dequeue(&trace_info->trace_queue)) != NULL){

				if((MAX_TRACE_BUFFER - buffer_length) < wan_skb_len(skb)){

					/* indicate there are more frames on board & exit */
					udp_hdr->wan_udphdr_chdlc_ismoredata = 0x01;

					if (buffer_length != 0){
						wan_skb_queue_head(&trace_info->trace_queue, skb);
					}else{

						/* If rx buffer length is greater than the
						 * whole udp buffer copy only the trace
						 * header and drop the trace packet */

						memcpy(&udp_hdr->wan_udphdr_chdlc_data[buffer_length],
							wan_skb_data(skb),
							sizeof(wan_trace_pkt_t));

						buffer_length = sizeof(wan_trace_pkt_t);
						udp_hdr->wan_udphdr_chdlc_num_frames++;
						wan_skb_free(skb);
					}

					break;
				}

				memcpy(&udp_hdr->wan_udphdr_chdlc_data[buffer_length],
				       wan_skb_data(skb),
				       wan_skb_len(skb));

				buffer_length += wan_skb_len(skb);
				wan_skb_free(skb);
				udp_hdr->wan_udphdr_chdlc_num_frames++;
			}
#endif
			/* set the data length and return code */
			udp_hdr->wan_udphdr_data_len = buffer_length;
			udp_hdr->wan_udphdr_return_code = WAN_CMD_OK;
			break;

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
			wan_getcurrenttime(&chan->router_up_time, NULL);
			chan->router_up_time -= chan->router_start_time;
			*(u_int32_t *)&wan_udp_pkt->wan_udp_data =
					chan->router_up_time;
			mb->wan_data_len = sizeof(u_int32_t);
			wan_udp_pkt->wan_udp_data_len = sizeof(u_int32_t);
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
		   	wan_udp_pkt->wan_udp_chdlc_num_frames = WANCONFIG_CHDLC;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_MASTER_DEV_NAME:
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
			break;

		default:
dflt_1:
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

#if defined(__LINUX__)
	len = wan_reply_udp(card,chan->udp_pkt_data, mb->wan_data_len);

     	if(chan->udp_pkt_src == UDP_PKT_FRM_NETWORK){

		/* Must check if we interrupted if_send() routine. The
		 * tx buffers might be used. If so drop the packet */
	   	if (!wan_test_bit(SEND_CRIT,&card->wandev.critical)) {

			if(!chdlc_send(card, chan->udp_pkt_data, len)) {
				++ card->wandev.stats.tx_packets;
				card->wandev.stats.tx_bytes += len;
			}
		}
	} else {

		/* Pass it up the stack
    		   Allocate socket buffer */
		if ((new_skb = wan_skb_alloc(len)) != NULL) {
			/* copy data into new_skb */

 	    		buf = wan_skb_put(new_skb, len);
  	    		memcpy(buf, chan->udp_pkt_data, len);

            		/* Decapsulate pkt and pass it up the protocol stack */
#if defined(__LINUX__)
	    		wan_skb_set_protocol(new_skb, ETH_P_IP);
#endif
            		wan_skb_set_dev(new_skb, dev);
			wan_skb_reset_mac_header(new_skb);

			netif_rx(new_skb);
		} else {

			DEBUG_EVENT("%s: no socket buffers available!\n",
					card->devname);
  		}
    	}
#endif
	wan_atomic_set(&chan->udp_pkt_len,0);

	return 0;
}



/*============================================================================
 * Initialize Receive and Transmit Buffers.
 */

static void init_chdlc_tx_rx_buff( sdla_t* card, netdevice_t *dev )
{
	wan_mbox_t* mb = &card->wan_mbox;
	unsigned long tx_config_off;
	unsigned long rx_config_off;
	CHDLC_TX_STATUS_EL_CFG_STRUCT tx_config;
	CHDLC_RX_STATUS_EL_CFG_STRUCT rx_config;
	char err;

	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		chdlc_error(card,err,mb);
		return;
	}

	/* Alex Apr 8 2004 Sangoma ISA card */
	tx_config_off =
			((CHDLC_CONFIGURATION_STRUCT *)mb->wan_data)->
       				ptr_CHDLC_Tx_stat_el_cfg_struct;
       	rx_config_off =
			((CHDLC_CONFIGURATION_STRUCT *)mb->wan_data)->
				ptr_CHDLC_Rx_stat_el_cfg_struct;
	/* Setup Head and Tails for buffers */
	card->hw_iface.peek(card->hw, tx_config_off, &tx_config, sizeof(tx_config));
	card->u.c.txbuf_base_off =
			tx_config.base_addr_Tx_status_elements;
	card->u.c.txbuf_last_off =
			card->u.c.txbuf_base_off +
			(tx_config.number_Tx_status_elements - 1) *
					sizeof(CHDLC_DATA_TX_STATUS_EL_STRUCT);
	card->hw_iface.peek(card->hw, rx_config_off, &rx_config, sizeof(rx_config));
	card->u.c.rxbuf_base_off =
			rx_config.base_addr_Rx_status_elements;
	card->u.c.rxbuf_last_off =
			card->u.c.rxbuf_base_off +
			(rx_config.number_Rx_status_elements - 1) *
					sizeof(CHDLC_DATA_RX_STATUS_EL_STRUCT);
	/* Set up next pointer to be used */
       	card->u.c.txbuf_off =
                	tx_config.next_Tx_status_element_to_use;
       	card->u.c.rxmb_off =
			rx_config.next_Rx_status_element_to_use;

        /* Setup Actual Buffer Start and end addresses */
        card->u.c.rx_base_off = rx_config.base_addr_Rx_buffer;
        card->u.c.rx_top_off  = rx_config.end_addr_Rx_buffer;

}

/*=============================================================================
 * Perform Interrupt Test by running READ_CHDLC_CODE_VERSION command MAX_INTR
 * _TEST_COUNTER times.
 */
#if defined(__LINUX__) || defined(CHDLC_OLD_INTR_TEST)
static int chdlc_intr_test( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err,i;

	card->timer_int_enabled = 0;

	/* The critical flag is unset because during intialization (if_open)
	 * we want the interrupts to be enabled so that when the wpc_isr is
	 * called it does not exit due to critical flag set.
	 */

	err = chdlc_set_intr_mode(card, APP_INT_ON_COMMAND_COMPLETE);

	if (err == CMD_OK) {
#if defined(__FreeBSD__)
		int	delay = HZ/10;
#endif
		for (i = 0; i < MAX_NEW_INTR_TEST_COUNTER; i ++) {
			memset(mb,0,sizeof(wan_mbox_t));
			mb->wan_data_len  = 0;
			mb->wan_command = READ_CHDLC_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

			if (err != CMD_OK)
				chdlc_error(card, err, mb);

			WP_DELAY(10000);
			WP_SCHEDULE(delay,"multppp");
		}
	}else{
		return err;
	}

	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);

	err = chdlc_set_intr_mode(card, 0);
	if (err != CMD_OK)
		return err;

	if(err || (card->timer_int_enabled < MAX_NEW_INTR_TEST_COUNTER)) {
		DEBUG_EVENT("%s: Interrupt test failed (%d)\n",
				card->devname, card->timer_int_enabled);
		DEBUG_EVENT("%s: Please choose another interrupt\n",
				card->devname);
		return  -EIO;
	}

	DEBUG_EVENT("%s: Interrupt test passed (%i)\n",
				card->devname, card->timer_int_enabled);

	return 0;
}
#endif

/*==============================================================================
 * Determine what type of UDP call it is. CPIPEAB ?
 */
static int udp_pkt_type(netskb_t *skb, sdla_t* card)
{

	wan_udp_pkt_t *wan_udp_pkt = (wan_udp_pkt_t *)(wan_skb_data(skb)+4);

	if (wan_skb_len(skb) < sizeof(wan_udp_pkt_t)){
		return UDP_INVALID_TYPE;
	}


#ifdef _WAN_UDP_DEBUG
		DEBUG_EVENT("SIG %s = %s\n\
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
	netdevice_t	*dev;
	private_area_t	*chan;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev)){
		return;
	}

	chan=wan_netif_priv(dev);

        if (card->wandev.state != state)
        {
                switch (state)
                {
                case WAN_CONNECTED:
                        DEBUG_EVENT( "%s: HDLC link connected!\n",
                                card->devname);

			WAN_NETIF_WAKE_QUEUE(dev);
			WAN_NETIF_CARRIER_ON(dev);

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			if (chan->common.usedby == ANNEXG &&
			    chan->annexg_dev){
				if (IS_FUNC_CALL(lapb_protocol,lapb_link_up)){
					lapb_protocol.lapb_link_up(chan->annexg_dev);
				}
			}
#endif


			if (chan->common.usedby == STACK){
				wanpipe_lip_connect(chan,0);
				break;
			}

                      break;

                case WAN_CONNECTING:
                        DEBUG_EVENT( "%s: HDLC link connecting...\n",
                                card->devname);
			WAN_NETIF_STOP_QUEUE(dev);
			WAN_NETIF_CARRIER_OFF(dev);
                        break;

                case WAN_DISCONNECTED:
                        DEBUG_EVENT( "%s: HDLC link disconnected!\n",
                                card->devname);

			WAN_NETIF_STOP_QUEUE(dev);
			WAN_NETIF_CARRIER_OFF(dev);

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			if (chan->common.usedby == ANNEXG &&
			    chan->annexg_dev){
				if (IS_FUNC_CALL(lapb_protocol,lapb_link_down))
					lapb_protocol.lapb_link_down(chan->annexg_dev);
				break;
			}
#endif
			if (chan->common.usedby == STACK){
				wanpipe_lip_disconnect(chan,0);
				break;
			}

			break;
                }

                card->wandev.state = state;
		chan->common.state = state;
        }
}

void s508_lock (sdla_t *card, wan_smp_flag_t *smp_flags,wan_smp_flag_t *smp_flags1)
{
	wan_spin_lock_irq(&card->wandev.lock, smp_flags);
        if (card->next){
		/* It is ok to use spin_lock here, since we
		 * already turned off interrupts */
        	wan_spin_lock(&card->next->wandev.lock,smp_flags1);
	}
}

void s508_unlock (sdla_t *card, wan_smp_flag_t *smp_flags,wan_smp_flag_t *smp_flags1)
{
	if (card->next){
		wan_spin_unlock(&card->next->wandev.lock,smp_flags1);
	}
	wan_spin_unlock_irq(&card->wandev.lock, smp_flags);
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

static int config_chdlc (sdla_t *card)
{
	netdevice_t	*dev;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (card->u.c.comm_enabled){
		chdlc_comm_disable(card);
		port_set_state(card, WAN_DISCONNECTED);
	}

	/* Setup the Board for asynchronous mode */
	if (card->u.c.async_mode){
		if (set_asy_config(card)) {
			DEBUG_EVENT( "%s: Failed Async configuration!\n",
				card->devname);
			port_set_state(card, WAN_DISCONNECTED);
			return -EINVAL;
		}
	}else{
		if (set_chdlc_config(card)) {
			DEBUG_EVENT("%s: HDLC Configuration Failed!\n",
					card->devname);
			port_set_state(card, WAN_DISCONNECTED);
			return -EINVAL;
		}
	}

	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
	if (IS_TE1_CARD(card)) {
		int	err = -EINVAL;
		DEBUG_EVENT("%s: Configuring onboard %s CSU/DSU\n",
			card->devname,
			(IS_T1_CARD(card))?"T1":"E1");
		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			DEBUG_EVENT("%s: Failed %s configuratoin!\n",
					card->devname,
					(IS_T1_CARD(card))?"T1":"E1");
			return -EINVAL;
		}
		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}

	if (IS_56K_CARD(card)) {
		int	err = -EINVAL;
		DEBUG_EVENT("%s: Configuring 56K onboard CSU/DSU\n",
			card->devname);
		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if(err){
			DEBUG_EVENT( "%s: Failed 56K configuration!\n",
				card->devname);
			return -EINVAL;
		}
		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}


	/* Set interrupt mode and mask */
        if (chdlc_set_intr_mode(card, APP_INT_ON_RX_FRAME |
                		APP_INT_ON_GLOBAL_EXCEP_COND |
                		APP_INT_ON_TX_FRAME |
                		APP_INT_ON_CHDLC_EXCEP_COND | APP_INT_ON_TIMER)){
		DEBUG_EVENT( "%s: Failed to set interrupt triggers!\n",
				card->devname);
		return 0;
        }

	/* Mask All interrupts  */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off,
			(APP_INT_ON_RX_FRAME | APP_INT_ON_TX_FRAME |
			 APP_INT_ON_TIMER    | APP_INT_ON_GLOBAL_EXCEP_COND |
			 APP_INT_ON_CHDLC_EXCEP_COND));

	init_chdlc_tx_rx_buff(card, dev);

	if (card->u.c.async_mode){
		if (asy_comm_enable(card) != 0) {
			DEBUG_EVENT("%s: Failed to enable async commnunication!\n",
					card->devname);
			card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
			card->u.c.comm_enabled=0;
			chdlc_set_intr_mode(card,0);
			port_set_state(card, WAN_DISCONNECTED);
			return 0;
		}
	}else{
		if (chdlc_comm_enable(card) != 0) {
			DEBUG_EVENT("%s: Failed to enable chdlc communications!\n",
					card->devname);
			card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
			card->u.c.comm_enabled=0;
			chdlc_set_intr_mode(card,0);
			port_set_state(card, WAN_DISCONNECTED);
			return 0;
		}
	}

	/* Initialize Rx/Tx buffer control fields */
	init_chdlc_tx_rx_buff(card, dev);
	card->u.c.state = WAN_CONNECTING;
	port_set_state(card, WAN_CONNECTING);

		/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
	}

	/* Unmask all interrupts except the Transmit and Timer interrupts */
	card->hw_iface.set_bit(card->hw, card->intr_perm_off,
		(APP_INT_ON_RX_FRAME | APP_INT_ON_GLOBAL_EXCEP_COND |
		APP_INT_ON_CHDLC_EXCEP_COND));
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0);
	return 0;
}


static void send_ppp_term_request (netdevice_t *dev)
{
	netskb_t *new_skb;
	unsigned char *buf;
	private_area_t *chan=wan_netif_priv(dev);

	if (!chan->common.prot_ptr){
		return;
	}

	if (chan->ignore_modem)
		return;

	if ((new_skb = wan_skb_alloc(8)) != NULL) {
		/* copy data into new_skb */

		buf = wan_skb_put(new_skb, 8);
		sprintf(buf,"%c%c%c%c%c%c%c%c", 0xFF,0x03,0xC0,0x21,0x05,0x98,0x00,0x07);

#if defined(__LINUX__)
		/* Decapsulate pkt and pass it up the protocol stack */
		new_skb->protocol = htons(ETH_P_WAN_PPP);
		new_skb->dev = dev;
		wan_skb_reset_mac_header(new_skb);
#endif
		wan_skb_queue_tail(&chan->rx_queue,new_skb);
		WAN_TASKLET_SCHEDULE((&chan->tasklet));
	}
}

#if defined(__LINUX__)
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
	private_area_t*	chan = priv;
	sdla_t*		card = NULL;

	if (chan == NULL)
		return m->count;
	card = chan->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", chdlc_config_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_CFG_FRM, card->u.c.if_name, card->devname);
	return m->count;
}

static int chdlc_get_status_info(void* priv, struct seq_file* m, int* stop_cnt)
{
	private_area_t*	chan = priv;
	sdla_t*		card = NULL;

	if (chan == NULL)
		return m->count;
	card = chan->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", chdlc_status_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_STAT_FRM,
		card->u.c.if_name, card->devname, STATE_DECODE(chan->common.state));
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

	DEBUG_EVENT("%s: New device config (%s)\n",
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
	private_area_t* 	chan = NULL;
	sdla_t*			card = NULL;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return count;
	chan = (private_area_t*)wan_netif_priv(dev);
	if (chan->card == NULL)
		return count;
	card = chan->card;

	DEBUG_EVENT("%s: New interface config (%s)\n",
			card->u.c.if_name, buffer);
	/* Parse string */

	return count;
}
#endif

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
		chdlc_error(card,err,mb);
	}
	return (err);
}

/*============================================================================
 * Enable timer interrupt
 */
static void chdlc_enable_timer (void* card_id)
{
	sdla_t* 		card = (sdla_t*)card_id;

	card->u.c.timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	return;
}



/**SECTION**************************************************
 *
 * 	Bottom Half Handlers
 *
 **********************************************************/

#if defined(__LINUX__)
static void wp_bh (unsigned long data)
#else
static void wp_bh (void *data, int pending)
#endif
{
	private_area_t *chan = (private_area_t *)data;
	sdla_t *card = chan->card;
	netskb_t *skb;
	int len;

	if (wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical)){
		DEBUG_EVENT("%s: WpBH PERI Critical\n",
				card->devname);
		WAN_TASKLET_END((&chan->tasklet));
		return;
	}

	if (!WAN_NETIF_UP(chan->dev)){
		DEBUG_EVENT("%s: WpBH Dev done\n",
				card->devname);
		WAN_TASKLET_END((&chan->tasklet));
		return;
	}

	while ((skb=wan_skb_dequeue(&chan->rx_queue))){

		len=wan_skb_len(skb);

#if defined(ENABLE_MULTPROT_MODE_WANPIPE)
		if (chan->common.usedby == WANPIPE){

			card->wandev.stats.rx_packets ++;
			card->wandev.stats.rx_bytes += wan_skb_len(skb);

			wp_sppp_input(skb->dev,skb);

		}else
#endif
		if (chan->common.usedby == API){

			if (wan_skb_headroom(skb) >= sizeof(api_rx_hdr_t)){
				api_rx_hdr_t *rx_hdr=
					(api_rx_hdr_t*)wan_skb_push(skb,sizeof(api_rx_hdr_t));
				memset(rx_hdr,0,sizeof(api_rx_hdr_t));
			}else{
				if (WAN_NET_RATELIMIT()){
				DEBUG_ERROR("%s: Error Rx pkt headroom %d < %d\n",
						chan->if_name,
						wan_skb_headroom(skb),
						(int)sizeof(api_rx_hdr_t));
				}
				++card->wandev.stats.rx_dropped;
				wan_skb_free(skb);
				continue;
			}

			wan_skb_reset_mac_header(skb);
			wan_skb_set_dev(skb, chan->common.dev);
#if defined(__LINUX__)
			wan_skb_set_protocol(skb, PVC_PROT);
			skb->pkt_type = WAN_PACKET_DATA;
#endif
			if (wan_api_rx(chan,skb) == 0){
				card->wandev.stats.rx_packets++;
				card->wandev.stats.rx_bytes += len;
			}else{
				++card->wandev.stats.rx_dropped;
				wan_skb_free(skb);
			}

		}else if (chan->common.usedby == STACK){

			if (wanpipe_lip_rx(chan,skb) != 0){
				++card->wandev.stats.rx_dropped;
				wan_skb_free(skb);
			}else{
				card->wandev.stats.rx_packets++;
				card->wandev.stats.rx_bytes += len;
			}

		}else{

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			if (chan->annexg_dev){
				if (IS_FUNC_CALL(lapb_protocol,lapb_rx)){
					lapb_protocol.lapb_rx(chan->annexg_dev,skb);
					card->wandev.stats.rx_packets++;
					card->wandev.stats.rx_bytes += len;
				}else{
					wan_skb_free(skb);
					++card->wandev.stats.rx_dropped;
				}
			}else{
				wan_skb_free(skb);
				++card->wandev.stats.rx_dropped;
			}
#else
			wan_skb_free(skb);
			++card->wandev.stats.rx_dropped;
#endif
		}
	}

	WAN_TASKLET_END((&chan->tasklet));
	return;
}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev)
{
	wan_smp_flag_t smp_flags=0;
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	if (!chan)
		return -EINVAL;

	if (chan->common.usedby != ANNEXG)
		return -EPROTONOSUPPORT;

	if (chan->annexg_dev)
		return -EBUSY;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	chan->annexg_dev = annexg_dev;
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return 0;
}


static netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t *annexg_dev)
{
	struct wan_dev_le	*devle;
	netdevice_t *dev;
	wan_smp_flag_t smp_flags=0;
	sdla_t *card = wandev->priv;

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		private_area_t* chan;

		dev = WAN_DEVLE2DEV(devle);
		if (dev == NULL || (chan = wan_netif_priv(dev)) == NULL)
			continue;

		if (!chan->annexg_dev || chan->common.usedby != ANNEXG)
			continue;

		if (chan->annexg_dev == annexg_dev){
			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			chan->annexg_dev = NULL;
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
			return dev;
		}
	}
	return NULL;
}


static void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats_ptr)
{
	private_area_t* 	chan = wan_netif_priv(dev);
	wp_stack_stats_t *wp_stats = (wp_stack_stats_t *)wp_stats_ptr;

	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		if (IS_FUNC_CALL(lapb_protocol,lapb_get_active_inactive)){
			lapb_protocol.lapb_get_active_inactive(chan->annexg_dev,wp_stats);
		}
	}

	if (chan->common.state == WAN_CONNECTED){
		wp_stats->fr_active++;
	}else{
		wp_stats->fr_inactive++;
	}
}

static int
get_map(wan_device_t *wandev, netdevice_t *dev, struct seq_file* m, int* stop_cnt)
{
	private_area_t*	chan = wan_netif_priv(dev);

	if (!(dev->flags&IFF_UP)){
		return m->count;
	}

	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		if (IS_FUNC_CALL(lapb_protocol,lapb_get_map)){
			return lapb_protocol.lapb_get_map(chan->annexg_dev,
							 m);
		}
	}

	PROC_ADD_LINE(m,
		"%15s:%s:%c:%s:%c\n",
		chan->label,
		wandev->name,(wandev->state == WAN_CONNECTED) ? '*' : ' ',
		dev->name,(chan->common.state == WAN_CONNECTED) ? '*' : ' ');

	return m->count;
}

#endif


static int digital_loop_test(sdla_t* card,wan_udp_pkt_t* wan_udp_pkt)
{
#if defined(__LINUX__)
	netskb_t* skb;
	netdevice_t* dev;
	char* buf;
	private_area_t *chan;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL) {
		return 1;
	}
	chan = wan_netif_priv(dev);
	if (chan == NULL) {
		return 1;
	}

	if (chan->common.state != WAN_CONNECTED) {
		DEBUG_EVENT("%s: Loop test failed: dev not connected!\n",
		                        card->devname);
		return 2;
	}

	skb = wan_skb_alloc(wan_udp_pkt->wan_udp_data_len+100);
	if (skb == NULL) {
		return 3;
	}

	switch (chan->common.usedby) {

	case API:
		wan_skb_push(skb, sizeof(api_rx_hdr_t));
		break;

	case STACK:
	case WANPIPE:
		break;

	default:
		DEBUG_EVENT("%s: Loop test failed: invalid operation mode!\n",
			card->devname);
		wan_skb_free(skb);
		return 4;
	}

	buf = wan_skb_put(skb, wan_udp_pkt->wan_udp_data_len);
	memcpy(buf, wan_udp_pkt->wan_udp_data, wan_udp_pkt->wan_udp_data_len);

	skb->next = skb->prev = NULL;
        skb->dev = dev;
        skb->protocol = htons(ETH_P_IP);
	wan_skb_reset_mac_header(skb);
        dev_queue_xmit(skb);
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	DEBUG_EVENT("%s: WARNING: Digital loop test mode is not supported!\n",
				card->devname);
#endif

	return 0;
}

#if defined(__LINUX__)
static int wp_exec_usr_mbox_cmd(sdla_t *card, wan_mbox_t *usrmbox)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memcpy(mb,usrmbox,sizeof(wan_mbox_t));

	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK) {
		chdlc_error (card, err, mb);
	}

	memcpy(usrmbox,mb,sizeof(wan_mbox_t));

	return err;
}
#endif

/****** End ****************************************************************/
