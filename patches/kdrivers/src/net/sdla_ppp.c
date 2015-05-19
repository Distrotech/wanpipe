/*****************************************************************************
* sdla_ppp.c	WANPIPE(tm) Multiprotocol WAN Link Driver. PPP module.
*
* Author: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 03, 2003  Nenad Corbic	o Bug fix in Rx IPX packet. Possible memory leak
* 				  if an invalid IPX packet was received
* Oct 18, 2002  Nenad Corbic	0 Added BRIDGE support
* Aug 30, 2002  Nenad Corbic	o Added support for S5147 Dual TE1 card
* Jan 12, 2002  Nenad Corbic	o Removed the 2.0.X support
* 				  Added front end states
* Dec 03, 2001  Gideon Hack	o Updated for S514-5 56K adapter. 
* Sep 20, 2001  Nenad Corbic    o The min() function has changed for 2.4.9 
* 				  kernel. Thus using the wp_min() defined in
* 				  wanpipe.h
* Sept 6, 2001  Alex Feldman    o Add SNMP support.
* Aug 23, 2001  Nenad Corbic    o Removed the if_header and set the hard_header
* 			          length to zero. Caused problems with Checkpoint
* 			          firewall.
* Jul 22, 2001  Nenad Corbic    o Buf Fix make sure that in_dev ptr exists
*                                 before trying to reference it.
* Jun 07, 2001  Nenad Corbic  	o Bug Fix in retrigger_comm(): 
* 				  On a noisy line, if the 
* 				  protocol toggles UP and DOWN fast 
* 				  enough it can lead to a race condition 
* 				  that will cause it to stop working.
* May 25, 2001  Alex Feldman 	o Added T1/E1 support 
* Feb 28, 2001  Nenad Corbic	o Updated if_tx_timeout() routine for 
* 				  2.4.X kernels.
* Nov 29, 2000  Nenad Corbic	o Added the 2.4.x kernel support:
* 				  get_ip_address() function has moved
* 				  into the ppp_poll() routine. It cannot
* 				  be called from an interrupt.
* Nov 07, 2000  Nenad Corbic	o Added security features for UDP debugging:
*                                 Deny all and specify allowed requests.
* May 02, 2000  Nenad Corbic	o Added the dynamic interface shutdown
*                                 option. When the link goes down, the
*                                 network interface IFF_UP flag is reset.
* Mar 06, 2000  Nenad Corbic	o Bug Fix: corrupted mbox recovery.
* Feb 25, 2000  Nenad Corbic    o Fixed the FT1 UDP debugger problem.
* Feb 09, 2000  Nenad Coribc    o Shutdown bug fix. update() was called
*                                 with NULL dev pointer: no check.
* Jan 24, 2000  Nenad Corbic    o Disabled use of CMD complete inter.
* Dev 15, 1999  Nenad Corbic    o Fixed up header files for 2.0.X kernels
* Oct 25, 1999  Nenad Corbic    o Support for 2.0.X kernels
*                                 Moved dynamic route processing into 
*                                 a polling routine.
* Oct 07, 1999  Nenad Corbic    o Support for S514 PCI card.  
*               Gideon Hack     o UPD and Updates executed using timer interrupt
* Sep 10, 1999  Nenad Corbic    o Fixed up the /proc statistics
* Jul 20, 1999  Nenad Corbic    o Remove the polling routines and use 
*                                 interrupts instead.
* Sep 17, 1998	Jaspreet Singh	o Updates for 2.2.X Kernels.
* Aug 13, 1998	Jaspreet Singh	o Improved Line Tracing.
* Jun 22, 1998	David Fong	o Added remote IP address assignment
* Mar 15, 1998	Alan Cox	o 2.1.8x basic port.
* Apr 16, 1998	Jaspreet Singh	o using htons() for the IPX protocol.
* Dec 09, 1997	Jaspreet Singh	o Added PAP and CHAP.
*				o Implemented new routines like 
*				  ppp_set_inbnd_auth(), ppp_set_outbnd_auth(),
*				  tokenize() and str_strip().
* Nov 27, 1997	Jaspreet Singh	o Added protection against enabling of irqs 
*				  while they have been disabled.
* Nov 24, 1997  Jaspreet Singh  o Fixed another RACE condition caused by
*                                 disabling and enabling of irqs.
*                               o Added new counters for stats on disable/enable
*                                 IRQs.
* Nov 10, 1997	Jaspreet Singh	o Initialized 'skb->mac.raw' to 'skb->data'
*				  before every netif_rx().
*				o Free up the device structure in del_if().
* Nov 07, 1997	Jaspreet Singh	o Changed the delay to zero for Line tracing
*				  command.
* Oct 20, 1997 	Jaspreet Singh	o Added hooks in for Router UP time.
* Oct 16, 1997	Jaspreet Singh  o The critical flag is used to maintain flow
*				  control by avoiding RACE conditions.  The 
*				  cli() and restore_flags() are taken out.
*				  A new structure, "ppp_private_area", is added 
*				  to provide Driver Statistics.   
* Jul 21, 1997 	Jaspreet Singh	o Protected calls to sdla_peek() by adding 
*				  save_flags(), cli() and restore_flags().
* Jul 07, 1997	Jaspreet Singh  o Added configurable TTL for UDP packets
*				o Added ability to discard mulitcast and
*				  broacast source addressed packets.
* Jun 27, 1997 	Jaspreet Singh	o Added FT1 monitor capabilities
*				  New case (0x25) statement in if_send routine.
*				  Added a global variable rCount to keep track
*				  of FT1 status enabled on the board.
* May 22, 1997	Jaspreet Singh	o Added change in the PPP_SET_CONFIG command for
*				508 card to reflect changes in the new 
*				ppp508.sfm for supporting:continous transmission
*				of Configure-Request packets without receiving a
*				reply 				
*				OR-ed 0x300 to conf_flags 
*			        o Changed connect_tmout from 900 to 0
* May 21, 1997	Jaspreet Singh  o Fixed UDP Management for multiple boards
* Apr 25, 1997  Farhan Thawar    o added UDP Management stuff
* Mar 11, 1997  Farhan Thawar   Version 3.1.1
*                                o fixed (+1) bug in rx_intr()
*                                o changed if_send() to return 0 if
*                                  wandev.critical() is true
*                                o free socket buffer in if_send() if
*                                  returning 0 
* Jan 15, 1997	Gene Kozin	Version 3.1.0
*				 o implemented exec() entry point
* Jan 06, 1997	Gene Kozin	Initial version.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/wanproc.h>
#include <linux/wanpipe_snmp.h>
#include <linux/sdla_ppp.h>		/* PPP firmware API definitions */
#include <linux/sdlasfm.h>		/* S514 Type Definition */
#include <linux/sdla_front_end.h>
#include <linux/if_wanpipe.h>		
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */



/****** Defines & Macros ****************************************************/

#define	PPP_DFLT_MTU	1500		/* default MTU */
#define	PPP_MAX_MTU	4000		/* maximum MTU */
#define PPP_HDR_LEN	1

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

/* Private critical flags */
enum { 
	POLL_CRIT = PRIV_CRIT 
};

#define MAX_IP_ERRORS 100 

#define	CONNECT_TIMEOUT	(90*HZ)		/* link connection timeout */
#define	HOLD_DOWN_TIME	(5*HZ)		/* link hold down time : Changed from 30 to 5 */

/* For handle_IPXWAN() */
#define CVHexToAscii(b) (((unsigned char)(b) > (unsigned char)9) ? ((unsigned char)'A' + ((unsigned char)(b) - (unsigned char)10)) : ((unsigned char)'0' + (unsigned char)(b)))

/* Macro for enabling/disabling debugging comments */
#undef NEX_DEBUG
#ifdef NEX_DEBUG
#define NEX_PRINTK(format, a...) printk(format, ## a)
#else
#define NEX_PRINTK(format, a...)
#endif /* NEX_DEBUG */ 

#define DCD(a)   ( a & 0x08 ? "HIGH" : "LOW" )
#define CTS(a)   ( a & 0x20 ? "HIGH" : "LOW" )
#define LCP(a)   ( a == 0x09 ? "OPEN" : "CLOSED" )
#define IP(a)    ( a == 0x09 ? "ENABLED" : "DISABLED" )

#define TMR_INT_ENABLED_UPDATE  	0x01
#define TMR_INT_ENABLED_PPP_EVENT	0x02
#define TMR_INT_ENABLED_UDP		0x04
#define TMR_INT_ENABLED_CONFIG		0x20
#define TMR_INT_ENABLED_TE		0x40

/* Set Configuraton Command Definitions */
#define PERCENT_TX_BUFF			60
#define TIME_BETWEEN_CONF_REQ  		120
#define TIME_BETWEEN_PAP_CHAP_REQ	120
#define WAIT_PAP_CHAP_WITHOUT_REPLY     1200
#define WAIT_AFTER_DCD_CTS_LOW          20
#define TIME_DCD_CTS_LOW_AFTER_LNK_DOWN 40
#define WAIT_DCD_HIGH_AFTER_ENABLE_COMM 3400
#define MAX_CONF_REQ_WITHOUT_REPLY      40
#define MAX_TERM_REQ_WITHOUT_REPLY      10
#define NUM_CONF_NAK_WITHOUT_REPLY      20
#define NUM_AUTH_REQ_WITHOUT_REPLY      40


#define END_OFFSET 0x1F0
#if LINUX_VERSION_CODE < 0x020125
#define test_and_set_bit set_bit
#define net_ratelimit() 1
#endif

/* Number of times we'll retry to 
 * enable comunications.  If it fails
 * the link should be restarted */
#define MAX_COMM_BUSY_RETRY	10

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with PPP specific data
 */
  
typedef struct ppp_private_area
{
	wanpipe_common_t common;
	netdevice_t *slave;
	sdla_t* card;	
	unsigned long router_start_time;	/*router start time in sec */
	unsigned long tick_counter;		/*used for 5 second counter*/
	unsigned mc;				/*multicast support on or off*/
	unsigned char enable_IPX;
	unsigned long network_number;
	unsigned char pap;
	unsigned char chap;
	unsigned char sysname[31];		/* system name for in-bnd auth*/
	unsigned char userid[511];		/* list of user ids */
	unsigned char passwd[511];		/* list of passwords */
	unsigned protocol;			/* SKB Protocol */
	u32 ip_local;				/* Local IP Address */
	u32 ip_remote;				/* remote IP Address */

	u32 ip_local_tmp;
	u32 ip_remote_tmp;
	
	unsigned int timer_int_enabled;		/* Who enabled the timer inter*/
	unsigned long update_comms_stats;	/* Used by update function */
	unsigned long curr_trace_addr;		/* Trace information */
	unsigned long start_trace_addr;
	unsigned long end_trace_addr;

	unsigned long interface_down;		/* Brind down interface when channel 
                                                   goes down */
	unsigned long config_wait_timeout;	/* After if_open() if in dynamic if mode,
						   wait a few seconds before configuring */
	char  udp_pkt_src;

	/* PPP specific statistics */

	if_send_stat_t if_send_stat;
	rx_intr_stat_t rx_intr_stat;
	pipe_mgmt_stat_t pipe_mgmt_stat;

	unsigned long router_up_time; 

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	wan_taskq_t poll_task;
	struct timer_list poll_delay_timer;

	u8 gateway;
	unsigned long config_ppp;
	u8 ip_error;
	
	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;

	unsigned char comm_busy_retry;
	unsigned char ppp_state;

	unsigned char udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t udp_pkt_len;

	char if_name[WAN_IFNAME_SZ+1];

}ppp_private_area_t;

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

/****** Function Prototypes *************************************************/

/* WAN link driver entry points. These are called by the WAN router module. */
static int update(wan_device_t *wandev);
static int new_if(wan_device_t *wandev, netdevice_t *dev, wanif_conf_t *conf);
static int del_if(wan_device_t *wandev, netdevice_t *dev);

/* Network device interface */
static int if_init(netdevice_t *dev);
static int if_open(netdevice_t *dev);
static int if_close(netdevice_t *dev);
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);


static void if_tx_timeout (netdevice_t *dev);

static struct net_device_stats *if_stats(netdevice_t *dev);
static int if_send(struct sk_buff *skb, netdevice_t *dev);


/* PPP firmware interface functions */
static int ppp_read_version(sdla_t *card, char *str);
static int ppp_set_outbnd_auth(sdla_t *card, ppp_private_area_t *ppp_priv_area);
static int ppp_set_inbnd_auth(sdla_t *card, ppp_private_area_t *ppp_priv_area);
static int ppp_configure(sdla_t *card, void *data);
static int ppp_set_intr_mode(sdla_t *card, unsigned char mode);
static int ppp_comm_enable(sdla_t *card);
static int ppp_comm_disable(sdla_t *card);
static int ppp_comm_disable_shutdown(sdla_t *card);
static int ppp_get_err_stats(sdla_t *card);
static int ppp_send(sdla_t *card, void *data, unsigned len, unsigned proto);
static int ppp_error(sdla_t *card, int err, wan_mbox_t *mb);

static int set_adapter_config (sdla_t* card);


static WAN_IRQ_RETVAL wpp_isr(sdla_t *card);
static void rx_intr(sdla_t *card);
static void event_intr(sdla_t *card);
static void timer_intr(sdla_t *card);

/* Background polling routines */
static void process_route(sdla_t *card);
static int retrigger_comm(sdla_t *card);

/* Miscellaneous functions */
static int read_info( sdla_t *card );
static int read_connection_info (sdla_t *card);
static void remove_route( sdla_t *card );
static int config508(netdevice_t *dev, sdla_t *card);
static void show_disc_cause(sdla_t * card, unsigned cause);
static int reply_udp( unsigned char *data, unsigned int mbox_len );
static void process_udp_mgmt_pkt(sdla_t *card, netdevice_t *dev, 
				ppp_private_area_t *ppp_priv_area,int local_dev);
static void init_ppp_tx_rx_buff( sdla_t *card );
static int intr_test( sdla_t *card );
static int udp_pkt_type( struct sk_buff *skb , sdla_t *card);
static void init_ppp_priv_struct( ppp_private_area_t *ppp_priv_area);
static void init_global_statistics( sdla_t *card );
static int tokenize(char *str, char **tokens);
static char* str_strip(char *str, char *s);
static int chk_bcast_mcast_addr(sdla_t* card, netdevice_t* dev,
				struct sk_buff *skb);

static int config_ppp (sdla_t *);
static void trigger_ppp_poll(netdevice_t *);
static void ppp_poll_delay (unsigned long dev_ptr);

# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void ppp_poll (void *card_ptr);
#else
static void ppp_poll (struct work_struct *work);
#endif  

static unsigned long Read_connection_info;
static unsigned short available_buffer_space;


/* IPX functions */
static void switch_net_numbers(unsigned char *sendpacket, unsigned long network_number, 
			       unsigned char incoming);
static int handle_IPXWAN(unsigned char *sendpacket, char *devname, unsigned char enable_PX, 
			 unsigned long network_number, unsigned short proto);

/* Lock Functions */
static void s508_lock (sdla_t *card, unsigned long *smp_flags);
static void s508_unlock (sdla_t *card, unsigned long *smp_flags);

static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, netdevice_t* dev,
                                ppp_private_area_t* ppp_priv_area );
static unsigned short calc_checksum (char *data, int len);
static void disable_comm (sdla_t *card);
static int detect_and_fix_tx_bug (sdla_t *card);

static int ppp_get_config_info(void* priv, struct seq_file* m, int*);
static int ppp_get_status_info(void* priv, struct seq_file* m, int*);
static int ppp_set_dev_config(struct file*, const char*, unsigned long, void *);
static int ppp_set_if_info(struct file*, const char*, unsigned long, void *);

static void ppp_enable_timer(void* card_id);
static void ppp_handle_front_end_state(void* card_id);

/* SNMP */
static int ppp_snmp_data(sdla_t* card, netdevice_t *dev, void* data);

static int ppp_debugging(sdla_t* card);
static unsigned long ppp_crc_frames(sdla_t* card);
static unsigned long ppp_abort_frames(sdla_t * card);
static unsigned long ppp_tx_underun_frames(sdla_t* card);
/****** Public Functions ****************************************************/

/*============================================================================
 * PPP protocol initialization routine.
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
int wpp_init(sdla_t *card, wandev_conf_t *conf)
{
	int err;
	union
	{
		char str[80];
	} u;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_PPP) {
		
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
			card->devname, conf->config_id);
		return -EINVAL;

	}

	/* Initialize miscellaneous pointers to structures on the adapter */
	switch (card->type) {

		case SDLA_S508: /* Alex Apr 8 2004 Sangoma ISA card */
		case SDLA_S514:
			card->mbox_off = PPP514_MB_OFFS;
			card->flags_off = PPP514_FLG_OFFS;
			break;

		default:
			return -EINVAL;

	}
    	card->intr_type_off = 
			card->flags_off +
			offsetof(ppp_flags_t, iflag);
	card->intr_perm_off = 
			card->flags_off +
			offsetof(ppp_flags_t, imask);
	card->wandev.ignore_front_end_status = conf->ignore_front_end_status;

	//ALEX_TODAY err=check_conf_hw_mismatch(card,conf->te_cfg.media);
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
		card->fe.write_fe_reg   = card->hw_iface.fe_write;
		card->fe.read_fe_reg	= card->hw_iface.fe_read;
	
		card->wandev.fe_enable_timer = ppp_enable_timer;
		card->wandev.te_link_state = ppp_handle_front_end_state;
		conf->electrical_interface = 
			(IS_T1_CARD(card)) ? WANOPT_V35 : WANOPT_RS232;
		conf->clocking = WANOPT_EXTERNAL;

	}else if (IS_56K_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg   = card->hw_iface.fe_write;
		card->fe.read_fe_reg	= card->hw_iface.fe_read;
		conf->clocking = WANOPT_EXTERNAL;
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
	if (ppp_read_version(card, NULL) || ppp_read_version(card, u.str))
		return -EIO;
	
	printk(KERN_INFO "%s: running PPP firmware v%s\n",card->devname, u.str); 


	if (set_adapter_config(card)) {
		return -EIO;
	}

	
	/* Adjust configuration and set defaults */
	card->wandev.mtu = (conf->mtu) ?
		wp_min(conf->mtu, PPP_MAX_MTU) : PPP_DFLT_MTU;

	card->wandev.bps	= conf->bps;
	card->wandev.electrical_interface	= conf->electrical_interface;
	card->wandev.clocking	= conf->clocking;
	card->isr		= &wpp_isr;
	card->poll		= NULL; 
	card->exec		= NULL;
	card->wandev.update	= &update;
	card->wandev.new_if	= &new_if;
	card->wandev.del_if	= &del_if;
        card->wandev.udp_port   = conf->udp_port;
	card->wandev.ttl	= conf->ttl;
	card->wandev.state      = WAN_DISCONNECTED;
	card->disable_comm	= &disable_comm;
	card->irq_dis_if_send_count = 0;
        card->irq_dis_poll_count = 0;
	card->u.p.authenticator = conf->u.ppp.authenticator;
	card->u.p.ip_mode 	= conf->u.ppp.ip_mode ?
				 conf->u.ppp.ip_mode : WANOPT_PPP_STATIC;
        card->TracingEnabled    = 0;
	Read_connection_info    = 1;

	// Proc fs functions
	card->wandev.get_config_info 	= &ppp_get_config_info;
	card->wandev.get_status_info 	= &ppp_get_status_info;
	card->wandev.set_dev_config    	= &ppp_set_dev_config;
	card->wandev.set_if_info     	= &ppp_set_if_info;

	/* Debugging */
	card->wan_debugging		= &ppp_debugging;
	card->get_crc_frames		= &ppp_crc_frames;
	card->get_abort_frames		= &ppp_abort_frames;
	card->get_tx_underun_frames	= &ppp_tx_underun_frames;

	/* SNMP data */
	card->get_snmp_data		= &ppp_snmp_data;
	/* initialize global statistics */
	init_global_statistics( card );



	if (!card->configured){
		int err;

		card->timer_int_enabled = 0;
		err = intr_test(card);

		if(err || (card->timer_int_enabled < MAX_INTR_TEST_COUNTER)) {
			printk("%s: Interrupt Test Failed, Counter: %i\n", 
				card->devname, card->timer_int_enabled);
			printk( "%s: Please choose another interrupt\n",card->devname);
			return -EIO;
		}
		
		printk(KERN_INFO "%s: Interrupt Test Passed, Counter: %i\n", 
			card->devname, card->timer_int_enabled);
		card->configured = 1;
	}

	ppp_set_intr_mode(card, PPP_INTR_TIMER); 

	/* Turn off the timer interrupt */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);

	printk(KERN_INFO "\n");

	return 0;
}

/******* WAN Device Driver Entry Points *************************************/

/*============================================================================
 * Update device status & statistics.
 */
static int update(wan_device_t *wandev)
{
	sdla_t* card = wandev->priv;
	netdevice_t	*dev;
        volatile ppp_private_area_t *ppp_priv_area;
	unsigned long smp_flags;

	/* sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if (wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;
	
	/* Shutdown bug fix. This function can be
         * called with NULL dev pointer during
         * shutdown 
	 */
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL)
		return -ENODEV;

	if ((ppp_priv_area = wan_netif_priv(dev)) == NULL){
		return -ENODEV;
	}
	
	/* TE1 Read T1/E1 alarms and PMON counters in one timer interrupt */
	if (test_and_set_bit(0,&ppp_priv_area->update_comms_stats)){
		return -EBUSY;
	}

	spin_lock_irqsave(&card->wandev.lock, smp_flags);

	ppp_get_err_stats(card);
		
	if (IS_TE1_CARD(card)) {	
		card->wandev.fe_iface.read_alarm(&card->fe, 0); 
		/* TE1 Update T1/E1 perfomance counters */
		card->wandev.fe_iface.read_pmon(&card->fe, 0); 
	}else if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1); 
	}

	ppp_priv_area->update_comms_stats=0;

	spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

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
static int new_if(wan_device_t *wandev, netdevice_t *dev, wanif_conf_t *conf)
{
	sdla_t *card = wandev->priv;
	ppp_private_area_t *ppp_priv_area;
	int err = 0;

	if (wandev->ndev)
		return -EEXIST;
	

	printk(KERN_INFO "%s: Configuring Interface: %s\n",
			card->devname, conf->name);

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {

		printk(KERN_INFO "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;

	}

	/* allocate and initialize private data */
	ppp_priv_area = kmalloc(sizeof(ppp_private_area_t), GFP_KERNEL);
	
	if( ppp_priv_area == NULL ){
		WAN_MEM_ASSERT(card->devname);
		return	-ENOMEM;
	}
	
	memset(ppp_priv_area, 0, sizeof(ppp_private_area_t));
	
	ppp_priv_area->card = card; 
	
	/* initialize data */
	strcpy(ppp_priv_area->if_name, conf->name);

	/* initialize data in ppp_private_area structure */
	
	init_ppp_priv_struct( ppp_priv_area );

	ppp_priv_area->mc = conf->mc;
	ppp_priv_area->pap = conf->pap;
	ppp_priv_area->chap = conf->chap;

	if(strcmp(conf->usedby, "WANPIPE") == 0) {
		printk(KERN_INFO "%s: Running in WANPIPE IP mode!\n",
				wandev->name);
		ppp_priv_area->common.usedby = WANPIPE;
		/* Option to bring down the interface when 
		 * the link goes down */
		if (conf->if_down){
			set_bit(DYN_OPT_ON,&ppp_priv_area->interface_down);
			printk("%s: Dynamic interface configuration enabled\n",
				card->devname);
		} 

	}else if (strcmp(conf->usedby, "BRIDGE")==0){
		printk(KERN_INFO "%s:%s Running in WANPIPE BRIDGE mode!\n",
				wandev->name,ppp_priv_area->if_name);
		ppp_priv_area->common.usedby = BRIDGE;
		
	}else if (strcmp(conf->usedby, "BRIDGE_N") == 0){
		printk(KERN_INFO "%s:%s Running in WANPIPE BRIDGE NODE mode!\n",
				wandev->name,ppp_priv_area->if_name);
		ppp_priv_area->common.usedby = BRIDGE_NODE;
	
	}else{
		printk(KERN_INFO "%s:%s: Error: Invalid operation mode [WANPIPE|BRIDGE]\n",
					card->devname,ppp_priv_area->if_name);
		err=-EINVAL;
		goto new_if_error;
	}

	/* If no user ids are specified */
	if(!strlen(conf->userid) && (ppp_priv_area->pap||ppp_priv_area->chap)){
		err=-EINVAL;
		goto new_if_error;
	}

	/* If no passwords are specified */
	if(!strlen(conf->passwd) && (ppp_priv_area->pap||ppp_priv_area->chap)){
		err=-EINVAL;
		goto new_if_error;
	}

	if(strlen(conf->sysname) > 31){
		err=-EINVAL;
		goto new_if_error;
	}

	/* If no system name is specified */
	if(!strlen(conf->sysname) && (card->u.p.authenticator)){
		err=-EINVAL;
		goto new_if_error;
	}

	/* copy the data into the ppp private structure */
	memcpy(ppp_priv_area->userid, conf->userid, strlen(conf->userid));
	memcpy(ppp_priv_area->passwd, conf->passwd, strlen(conf->passwd));
	memcpy(ppp_priv_area->sysname, conf->sysname, strlen(conf->sysname));

	
	ppp_priv_area->enable_IPX = conf->enable_IPX;
	if (conf->network_number){
		ppp_priv_area->network_number = conf->network_number;
	}else{
		ppp_priv_area->network_number = 0xDEADBEEF;
	}

	/* Tells us that if this interface is a
         * gateway or not */
	if ((ppp_priv_area->gateway = conf->gateway) == WANOPT_YES){
		printk(KERN_INFO "%s: Interface %s is set as a gateway.\n",
			card->devname,ppp_priv_area->if_name);
	}

	/* Initialize the polling task routine */
	WAN_TASKQ_INIT((&ppp_priv_area->poll_task),0,ppp_poll,dev);

	/* Initialize the polling delay timer */
	init_timer(&ppp_priv_area->poll_delay_timer);
	ppp_priv_area->poll_delay_timer.data = (unsigned long)dev;
	ppp_priv_area->poll_delay_timer.function = ppp_poll_delay;
	
	/*
	 * Create interface file in proc fs.
	 * Once the proc file system is created, the new_if() function
	 * should exit successfuly.  
	 *
	 * DO NOT place code under this function that can return 
	 * anything else but 0.
	 */
	err = wanrouter_proc_add_interface(wandev, 
					   &ppp_priv_area->dent, 
					   ppp_priv_area->if_name, 
					   dev);
	if (err){	
		printk(KERN_INFO
			"%s: can't create /proc/net/router/ppp/%s entry!\n",
			card->devname, ppp_priv_area->if_name);
		goto new_if_error;
	}

	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);
	wan_netif_set_priv(dev,ppp_priv_area);
	dev->mtu = wp_min(dev->mtu, card->wandev.mtu);
	ppp_priv_area->ppp_state=WAN_DISCONNECTED;
	
	/* Since we start with dummy IP addresses we can say
	 * that route exists */
	printk(KERN_INFO "\n");

	return 0;

new_if_error:

	kfree(ppp_priv_area);
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
static int del_if(wan_device_t *wandev, netdevice_t *dev)
{
	ppp_private_area_t* 	ppp_priv_area = wan_netif_priv(dev);
	
	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, ppp_priv_area->if_name);
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
	ppp_comm_disable_shutdown(card);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	/* TE1 unconfiging */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
	}
	return;
}

/****** Network Device Interface ********************************************/

/*============================================================================
 * Initialize Linux network interface.
 *
 * This routine is called only once for each interface, during Linux network
 * interface registration.  Returning anything but zero will fail interface
 * registration.
 */
static int if_init(netdevice_t *dev)
{
	ppp_private_area_t *ppp_priv_area = wan_netif_priv(dev);
	sdla_t *card = ppp_priv_area->card;
	wan_device_t *wandev = &card->wandev;

	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	dev->watchdog_timeo	= TX_TIMEOUT;
#endif

	if (ppp_priv_area->common.usedby == BRIDGE || 
            ppp_priv_area->common.usedby == BRIDGE_NODE){

		/* Setup the interface for Bridging */
		int hw_addr=0;
		ether_setup(dev);
		
		/* Use a random number to generate the MAC address */
		memcpy(dev->dev_addr, "\x00\xFC\x00\x00\x00\x00", 6);
		get_random_bytes(&hw_addr, sizeof(hw_addr));
		*(int *)(dev->dev_addr + 2) += hw_addr;

	}else{
		/* Initialize media-specific parameters */
		dev->type		= ARPHRD_PPP;	/* ARP h/w type */
		dev->flags		|= IFF_POINTOPOINT;
		dev->flags		|= IFF_NOARP;

		/* Enable Mulitcasting if specified by user*/
		if (ppp_priv_area->mc == WANOPT_YES){
			dev->flags	|= IFF_MULTICAST;
		}

		dev->mtu		= wandev->mtu;
		dev->hard_header_len	= 0; 	/* media header length */
	}
	
	/* Initialize hardware parameters (just for reference) */
	dev->irq		= wandev->irq;
	dev->dma		= wandev->dma;
	dev->base_addr		= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); 
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); 

        /* Set transmit buffer queue length */
        dev->tx_queue_len = 100;
   
	/* SNMP */
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);

	return 0;
}

/*============================================================================
 * Open network interface.
 * o enable communications and interrupts.
 * o prevent module from unloading by incrementing use count
 *
 * Return 0 if O.k. or errno.
 */
static int if_open (netdevice_t *dev)
{
	ppp_private_area_t *ppp_priv_area = wan_netif_priv(dev);
	sdla_t *card = ppp_priv_area->card;
	struct timeval tv;

	if (open_dev_check(dev))
		return -EBUSY;

	wanpipe_open(card);

	netif_start_queue(dev);
	
	do_gettimeofday( &tv );
	ppp_priv_area->router_start_time = tv.tv_sec;

	/* We cannot configure the card here because we don't
	 * have access to the interface IP addresses.
         * Once the interface initilization is complete, we will be
         * able to access the IP addresses.  Therefore,
         * configure the ppp link in the poll routine */
	set_bit(0,&ppp_priv_area->config_ppp);
	ppp_priv_area->config_wait_timeout=jiffies;

	/* Start the PPP configuration after 1sec delay.
	 * This will give the interface initilization time
	 * to finish its configuration */
	del_timer(&ppp_priv_area->poll_delay_timer);
	ppp_priv_area->poll_delay_timer.expires = jiffies+HZ;
	add_timer(&ppp_priv_area->poll_delay_timer);
	return 0;
}

/*============================================================================
 * Close network interface.
 * o if this is the last open, then disable communications and interrupts.
 * o reset flags.
 */
static int if_close(netdevice_t *dev)
{
	ppp_private_area_t *ppp_priv_area = wan_netif_priv(dev);
	sdla_t *card = ppp_priv_area->card;

	stop_net_queue(dev);
#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);

	del_timer (&ppp_priv_area->poll_delay_timer);
	return 0;
}

/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	ppp_private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	
	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	++ chan->if_send_stat.if_send_tbusy;
	++card->wandev.stats.collisions;

	printk (KERN_INFO "%s: Transmit timed out on %s\n", card->devname,dev->name);
	++chan->if_send_stat.if_send_tbusy_timeout;
	netif_wake_queue (dev);
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
static int if_send (struct sk_buff *skb, netdevice_t *dev)
{
	ppp_private_area_t *ppp_priv_area = wan_netif_priv(dev);
	sdla_t *card = ppp_priv_area->card;
	unsigned char *sendpacket;
	unsigned long smp_flags=0;
	int udp_type;
	int err=0;
	
	++ppp_priv_area->if_send_stat.if_send_entry;

	netif_stop_queue(dev);
	
	if (skb == NULL) {

		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		printk(KERN_INFO "%s: interface %s got kicked!\n",
			card->devname, dev->name);
		
		++ppp_priv_area->if_send_stat.if_send_skb_null;
	
		wake_net_dev(dev);
		return 0;
	}

#if defined(LINUX_2_1)
	if (dev->tbusy) {
		++ppp_priv_area->if_send_stat.if_send_tbusy;
        	++card->wandev.stats.collisions;

		if ((jiffies - ppp_priv_area->tick_counter) < (5*HZ)) {
			return 1;
		}

		if_tx_timeout(dev);
	}	
#endif
	
	sendpacket = skb->data;

	udp_type = udp_pkt_type( skb, card );


	if (udp_type == UDP_PTPIPE_TYPE){

		if(store_udp_mgmt_pkt(UDP_PKT_FRM_STACK, card, skb, dev,
                	              ppp_priv_area)){
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
		}
		++ppp_priv_area->if_send_stat.if_send_PIPE_request;
		start_net_queue(dev);
		return 0;
	}

	/* Check for broadcast and multicast addresses 
	 * If found, drop (deallocate) a packet and return.
	 */
	if(chk_bcast_mcast_addr(card, dev, skb)){
		++card->wandev.stats.tx_dropped;
		wan_dev_kfree_skb(skb,FREE_WRITE);
		start_net_queue(dev);
		return 0;
	}


 	if(card->type != SDLA_S514){
		s508_lock(card,&smp_flags);
	}

    	if (test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)) {

		printk(KERN_INFO "%s: Critical in if_send: %lx\n",
				card->wandev.name,card->wandev.critical);
		
		++card->wandev.stats.tx_dropped;
		++ppp_priv_area->if_send_stat.if_send_critical_non_ISR;
		start_net_queue(dev);
		goto if_send_exit_crit;
	}

	if (card->wandev.state != WAN_CONNECTED) {

		++ppp_priv_area->if_send_stat.if_send_wan_disconnected;
        	++card->wandev.stats.tx_dropped;
		start_net_queue(dev);
		
     	} else if (!skb->protocol) {
		++ppp_priv_area->if_send_stat.if_send_protocol_error;
        	++card->wandev.stats.tx_errors;
		start_net_queue(dev);
		
	} else {

		/*If it's IPX change the network numbers to 0 if they're ours.*/
		if( skb->protocol == htons(ETH_P_IPX) ) {
			if(ppp_priv_area->enable_IPX) {
				switch_net_numbers( skb->data, 
					ppp_priv_area->network_number, 0);
			} else {
				++card->wandev.stats.tx_dropped;
				start_net_queue(dev);
				goto if_send_exit_crit;
			}
		}

		err=ppp_send(card, skb->data, skb->len, skb->protocol);
		
		if (err) {
			err=-1;
			stop_net_queue(dev);
			++ppp_priv_area->if_send_stat.if_send_adptr_bfrs_full;
			++ppp_priv_area->if_send_stat.if_send_tx_int_enabled;
		}else{
			++ppp_priv_area->if_send_stat.if_send_bfr_passed_to_adptr;
			++card->wandev.stats.tx_packets;
			card->wandev.stats.tx_bytes += skb->len;
			start_net_queue(dev);
#if defined(LINUX_2_4)||defined(LINUX_2_6)
			dev->trans_start = jiffies;
#endif
		}
    	}
	
if_send_exit_crit:
	
	if (err==0){
      		wan_dev_kfree_skb(skb, FREE_WRITE);
	}else{
		ppp_priv_area->tick_counter = jiffies;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TXRDY);
	}
	
	clear_bit(SEND_CRIT,&card->wandev.critical);
	if(card->type != SDLA_S514){	
		s508_unlock(card,&smp_flags);
	}

	return err;
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
	ppp_private_area_t* chan= (ppp_private_area_t*)wan_netif_priv(dev);
	unsigned long smp_flags;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	
	if (!chan){
		return -ENODEV;
	}
	card=chan->card;
	
	NET_ADMIN_CHECK();

	switch(cmd)
	{
		case SIOC_WANPIPE_SNMP:
		case SIOC_WANPIPE_SNMP_IFSPEED:
			return wan_snmp_data(card, dev, cmd, ifr);


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
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,MAX_LGTH_UDP_MGNT_PKT)){
				atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)){
				printk(KERN_INFO "%s:%s Pipemon failed, driver busy: try again.\n",
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
			
			if (atomic_read(&chan->udp_pkt_len) > MAX_LGTH_UDP_MGNT_PKT){
				printk(KERN_INFO "%s: Error, pipemon buf too big on the way up! %i\n",
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
	return 0;
}

/*=============================================================================
 * Store a UDP management packet for later processing.
 */

static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, netdevice_t* dev,
                                ppp_private_area_t* ppp_priv_area )
{
	int udp_pkt_stored = 0;

	if(!atomic_read(&ppp_priv_area->udp_pkt_len) && (skb->len<=MAX_LGTH_UDP_MGNT_PKT)){
		atomic_set(&ppp_priv_area->udp_pkt_len,skb->len);
		ppp_priv_area->udp_pkt_src = udp_pkt_src;
		
       		memcpy(ppp_priv_area->udp_pkt_data, skb->data, skb->len);
		ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_UDP;
		
		ppp_priv_area->protocol = skb->protocol;
		udp_pkt_stored = 1;
	}else{
		if (skb->len > MAX_LGTH_UDP_MGNT_PKT){
			printk(KERN_INFO "%s: Pipemon UDP request too long : %i\n",
				card->devname, skb->len);
		}else{
			printk(KERN_INFO "%s: Pipemon UPD request already pending\n",
				card->devname);
		}
	}

	if(udp_pkt_src == UDP_PKT_FRM_STACK){
		wan_dev_kfree_skb(skb, FREE_WRITE);
	}else{
                wan_dev_kfree_skb(skb, FREE_READ);
	}

	return(udp_pkt_stored);
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
	wan_udp_pkt_t *p_udp_pkt = (wan_udp_pkt_t *)data;
 
	/* Set length of packet */
	len = sizeof(struct iphdr)+ 
	      sizeof(struct udphdr)+
	      sizeof(wan_mgmt_t)+
	      sizeof(wan_cmd_t)+
	      mbox_len;

	/* fill in UDP reply */
  	p_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY; 

	/* fill in UDP length */
	udp_length = sizeof(struct udphdr)+ 
		     sizeof(wan_mgmt_t)+
		     sizeof(wan_cmd_t)+
		     mbox_len; 
  
 
	/* put it on an even boundary */
	if ( udp_length & 0x0001 ) {
		udp_length += 1;
		len += 1;
		even_bound=1;
	} 
	
	temp = (udp_length<<8)|(udp_length>>8);
	p_udp_pkt->wan_udp_len = temp;		

 
	/* swap UDP ports */
	temp = p_udp_pkt->wan_udp_sport;
	p_udp_pkt->wan_udp_sport = 
			p_udp_pkt->wan_udp_dport; 
	p_udp_pkt->wan_udp_dport = temp;


	/* add UDP pseudo header */
	temp = 0x1100;
	*((unsigned short *)(p_udp_pkt->wan_udp_data+mbox_len+even_bound)) = temp;
	temp = (udp_length<<8)|(udp_length>>8);
	*((unsigned short *)(p_udp_pkt->wan_udp_data+mbox_len+even_bound+2)) = temp;
 
	/* calculate UDP checksum */
	p_udp_pkt->wan_udp_sum = 0;
	p_udp_pkt->wan_udp_sum = 
		calc_checksum(&data[UDP_OFFSET],udp_length+UDP_OFFSET);

	/* fill in IP length */
	ip_length = udp_length + sizeof(struct iphdr);
	temp = (ip_length<<8)|(ip_length>>8);
  	p_udp_pkt->wan_ip_len = temp;
 
	/* swap IP addresses */
	ip_temp = p_udp_pkt->wan_ip_src;
	p_udp_pkt->wan_ip_src = p_udp_pkt->wan_ip_dst;
	p_udp_pkt->wan_ip_dst = ip_temp;

	/* fill in IP checksum */
	p_udp_pkt->wan_ip_sum = 0;
	p_udp_pkt->wan_ip_sum = calc_checksum(data,sizeof(struct iphdr));

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

/*
   If incoming is 0 (outgoing)- if the net numbers is ours make it 0
   if incoming is 1 - if the net number is 0 make it ours 

*/
static void switch_net_numbers(unsigned char *sendpacket, unsigned long network_number, unsigned char incoming)
{
	unsigned long pnetwork_number;

	pnetwork_number = (unsigned long)((sendpacket[6] << 24) + 
			  (sendpacket[7] << 16) + (sendpacket[8] << 8) + 
			  sendpacket[9]);

	if (!incoming) {
		//If the destination network number is ours, make it 0
		if( pnetwork_number == network_number) {
			sendpacket[6] = sendpacket[7] = sendpacket[8] = 
					 sendpacket[9] = 0x00;
		}
	} else {
		//If the incoming network is 0, make it ours
		if( pnetwork_number == 0) {
			sendpacket[6] = (unsigned char)(network_number >> 24);
			sendpacket[7] = (unsigned char)((network_number & 
					 0x00FF0000) >> 16);
			sendpacket[8] = (unsigned char)((network_number & 
					 0x0000FF00) >> 8);
			sendpacket[9] = (unsigned char)(network_number & 
					 0x000000FF);
		}
	}


	pnetwork_number = (unsigned long)((sendpacket[18] << 24) + 
			  (sendpacket[19] << 16) + (sendpacket[20] << 8) + 
			  sendpacket[21]);

	if( !incoming ) {
		//If the source network is ours, make it 0
		if( pnetwork_number == network_number) {
			sendpacket[18] = sendpacket[19] = sendpacket[20] = 
					 sendpacket[21] = 0x00;
		}
	} else {
		//If the source network is 0, make it ours
		if( pnetwork_number == 0 ) {
			sendpacket[18] = (unsigned char)(network_number >> 24);
			sendpacket[19] = (unsigned char)((network_number & 
					 0x00FF0000) >> 16);
			sendpacket[20] = (unsigned char)((network_number & 
					 0x0000FF00) >> 8);
			sendpacket[21] = (unsigned char)(network_number & 
					 0x000000FF);
		}
	}
} /* switch_net_numbers */

/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct net_device_stats.
 */
static struct net_device_stats *if_stats(netdevice_t *dev)
{

	ppp_private_area_t *ppp_priv_area = wan_netif_priv(dev);
	sdla_t* card;
	
	if( ppp_priv_area == NULL )
		return NULL;

	card = ppp_priv_area->card;
	return &card->wandev.stats;
}


/****** PPP Firmware Interface Functions ************************************/

/*============================================================================
 * Enable timer interrupt  
 */
static void ppp_enable_timer (void* card_id)
{
	netdevice_t		*dev;
	sdla_t* 		card = (sdla_t*)card_id;
	ppp_private_area_t*	ppp_priv_area;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	
	ppp_priv_area = wan_netif_priv(dev);
	ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
	return;
}

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str. 
 */
static int ppp_read_version(sdla_t *card, char *str)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_READ_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if (err != CMD_OK)
 
		ppp_error(card, err, mb);

	else if (str) {

		int len = mb->wan_data_len;

		memcpy(str, mb->wan_data, len);
		str[len] = '\0';

	}

	return err;
}
/*===========================================================================
 * Set Out-Bound Authentication.
*/
static int ppp_set_outbnd_auth (sdla_t *card, ppp_private_area_t *ppp_priv_area)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	memset(&mb->wan_data, 0, (strlen(ppp_priv_area->sysname) + strlen(ppp_priv_area->userid) + 
					strlen(ppp_priv_area->passwd) + 3));
	memcpy(mb->wan_data, ppp_priv_area->sysname, strlen(ppp_priv_area->sysname));
	memcpy((mb->wan_data + strlen(ppp_priv_area->sysname) + 1), 
			ppp_priv_area->userid, strlen(ppp_priv_area->userid));
	memcpy((mb->wan_data + strlen(ppp_priv_area->sysname) + strlen(ppp_priv_area->userid) + 2), 
		ppp_priv_area->passwd, strlen(ppp_priv_area->passwd));	
	
	mb->wan_data_len  = strlen(ppp_priv_area->sysname) + strlen(ppp_priv_area->userid) + 
					strlen(ppp_priv_area->passwd) + 3 ;
	
	mb->wan_command = PPP_SET_OUTBOUND_AUTH;

	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if (err != CMD_OK)
		ppp_error(card, err, mb);

	return err;
}

/*===========================================================================
 * Set In-Bound Authentication.
*/
static int ppp_set_inbnd_auth (sdla_t *card, ppp_private_area_t *ppp_priv_area)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err, i;
	char* user_tokens[32];
	char* pass_tokens[32];
	int userids, passwds;
	int add_ptr;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	memset(&mb->wan_data, 0, 1008);
	memcpy(mb->wan_data, ppp_priv_area->sysname, 
						strlen(ppp_priv_area->sysname));
	
	/* Parse the userid string and the password string and build a string
	   to copy it to the data area of the command structure.   The string
	   will look like "SYS_NAME<NULL>USER1<NULL>PASS1<NULL>USER2<NULL>PASS2
	   ....<NULL> " 
	 */
	userids = tokenize(ppp_priv_area->userid, user_tokens);
	passwds = tokenize(ppp_priv_area->passwd, pass_tokens);
	
	if (userids != passwds){
		printk(KERN_INFO "%s: Number of passwords does not equal the number of user ids\n",
			       		card->devname);
		return 1;	
	}

	add_ptr = strlen(ppp_priv_area->sysname) + 1;
	for (i=0; i<userids; i++){
		memcpy((mb->wan_data + add_ptr), user_tokens[i], 
							strlen(user_tokens[i]));
		memcpy((mb->wan_data + add_ptr + strlen(user_tokens[i]) + 1), 
					pass_tokens[i], strlen(pass_tokens[i]));
		add_ptr = add_ptr + strlen(user_tokens[i]) + 1 + 
						strlen(pass_tokens[i]) + 1;
	}

	mb->wan_data_len  = add_ptr + 1;
	mb->wan_command = PPP_SET_INBOUND_AUTH;

	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if (err != CMD_OK)
		ppp_error(card, err, mb);

	return err;
}


/*============================================================================
 * Tokenize string.
 *      Parse a string of the following syntax:
 *              <arg1>,<arg2>,...
 *      and fill array of tokens with pointers to string elements.
 *
 */
static int tokenize (char *str, char **tokens)
{
        int cnt = 0;
	tokens[0] = strsep(&str, "/");
        while (tokens[cnt] && (cnt < 32 - 1))
        {
                tokens[cnt] = str_strip(tokens[cnt], " \t");
                tokens[++cnt] = strsep(&str, "/");
        }
	return cnt;
}

/*============================================================================
 * Strip leading and trailing spaces off the string str.
 */
static char* str_strip (char *str, char* s)
{
        char *eos = str + strlen(str);          /* -> end of string */

        while (*str && strchr(s, *str))
                ++str                           /* strip leading spaces */
        ;
        while ((eos > str) && strchr(s, *(eos - 1)))
                --eos                           /* strip trailing spaces */
        ;
        *eos = '\0';
        return str;
}
/*============================================================================
 * Configure PPP firmware.
 */
static int ppp_configure(sdla_t *card, void *data)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int data_len = sizeof(ppp508_conf_t); 
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	memcpy(mb->wan_data, data, data_len);
	mb->wan_data_len  = data_len;
	mb->wan_command = PPP_SET_CONFIG;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if (err != CMD_OK) 
		ppp_error(card, err, mb);
	
	return err;
}

/*============================================================================
 * Set interrupt mode.
 */
static int ppp_set_intr_mode(sdla_t *card, unsigned char mode)
{
	wan_mbox_t *mb = &card->wan_mbox;
        ppp_intr_info_t *ppp_intr_data = (ppp_intr_info_t *) &mb->wan_data[0];
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	ppp_intr_data->i_enable = mode;

	ppp_intr_data->irq = card->wandev.irq;	// ALEX_TODAY card->hw.irq;
	mb->wan_data_len = 2;

       /* If timer has been enabled, set the timer delay to 1sec */
       if (mode & 0x80){
       		ppp_intr_data->timer_len = 20; //5;//100; //250;
                mb->wan_data_len = 4;
        }
	
	mb->wan_command = PPP_SET_INTR_FLAGS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	if (err != CMD_OK) 
		ppp_error(card, err, mb);
 		

	return err;
}

/*============================================================================
 * Enable communications.
 */
static int ppp_comm_enable(sdla_t *card)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_COMM_ENABLE;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	if (err != CMD_OK) 
		ppp_error(card, err, mb);
	else	
		card->u.p.comm_enabled = 1;	

	return err;
}

/*============================================================================
 * Disable communications.
 */
static int ppp_comm_disable(sdla_t *card)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_COMM_DISABLE;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != CMD_OK) 
		ppp_error(card, err, mb);
	else
		card->u.p.comm_enabled = 0;

	return err;
}

static int ppp_comm_disable_shutdown(sdla_t *card)
{
	wan_mbox_t *mb = &card->wan_mbox;
	ppp_intr_info_t *ppp_intr_data;
	int err;

	if (!mb){
		return 1;
	}
	
	ppp_intr_data = (ppp_intr_info_t *) &mb->wan_data[0];
	
	/* Disable all interrupts */
	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	ppp_intr_data->i_enable = 0;

	ppp_intr_data->irq = card->wandev.irq;	// ALEX_TODAY card->hw.irq;
	mb->wan_data_len = 2;

	mb->wan_command = PPP_SET_INTR_FLAGS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	/* Disable communicatinons */
	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_COMM_DISABLE;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	card->u.p.comm_enabled = 0;

	return 0;
}



/*============================================================================
 * Get communications error statistics.
 */
static int ppp_get_err_stats(sdla_t *card)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err;

	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_READ_ERROR_STATS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	if (err == CMD_OK) {
		
		ppp_err_stats_t* stats = (void*)mb->wan_data;
		card->wandev.stats.rx_over_errors    = stats->rx_overrun;
		card->wandev.stats.rx_crc_errors     = stats->rx_bad_crc;
		card->wandev.stats.rx_missed_errors  = stats->rx_abort;
		card->wandev.stats.rx_length_errors  = stats->rx_lost;
		card->wandev.stats.tx_aborted_errors = stats->tx_abort;
	
	} else 
		ppp_error(card, err, mb);
	
	return err;
}

/*============================================================================
 * Get communications error statistics.
 */
#if 0
static int ppp_get_stats(sdla_t *card)
{
	wan_mbox_t *mb = card->mbox;
    	int err;
	
	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_READ_STATISTICS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != WAN_CMD_OK){ 
		ppp_error(card, err, mb);
	} 
	
	return err;
}
#endif

static int ppp_get_pkt_stats(sdla_t* card)
{
	wan_mbox_t *mb = &card->wan_mbox;
    	int err;
	
	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_READ_PACKET_STATS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
    	if (err != WAN_CMD_OK){
		ppp_error(card, err, mb);
    	}
    	return err;
}

static int ppp_get_lcp_stats(sdla_t* card)
{
	wan_mbox_t *mb = &card->wan_mbox;
    	int err;
	
	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_command = PPP_READ_LCP_STATS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
    	if (err != WAN_CMD_OK){
		ppp_error(card, err, mb);
	}
    	return err;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int ppp_send (sdla_t *card, void *data, unsigned len, unsigned proto)
{
	ppp_buf_ctl_t txbuf;

	card->hw_iface.peek(card->hw, card->u.p.txbuf_off, &txbuf, sizeof(txbuf));
	if (txbuf.flag)
                return 1;
	
	card->hw_iface.poke(card->hw, txbuf.buf.ptr, data, len);

	txbuf.length = len;		/* frame length */
	if (proto == htons(ETH_P_IPX)){
		txbuf.proto = 0x01;	/* protocol ID */
	}else{
		txbuf.proto = 0x00;	/* protocol ID */
	}
	card->hw_iface.poke(card->hw, card->u.p.txbuf_off, &txbuf, sizeof(txbuf));
	
	txbuf.flag = 1;		/* start transmission */
	card->hw_iface.poke(card->hw, card->u.p.txbuf_off, &txbuf, sizeof(txbuf));

	/* Update transmit buffer control fields */
	card->u.p.txbuf_off += sizeof(txbuf);

	if (card->u.p.txbuf_off > card->u.p.txbuf_last_off)
		card->u.p.txbuf_off = card->u.p.txbuf_base_off;
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
static int ppp_error(sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

		case CMD_TIMEOUT:
			printk(KERN_INFO "%s: command 0x%02X timed out!\n",
				card->devname, cmd);
			break;

		default:
			printk(KERN_INFO "%s: command 0x%02X returned 0x%02X!\n"
				, card->devname, cmd, err);
	}

	return 0;
}

/****** Interrupt Handlers **************************************************/
/*============================================================================
 * PPP interrupt service routine.
 */
static WAN_IRQ_RETVAL wpp_isr (sdla_t *card)
{
	ppp_flags_t		flags;
	netdevice_t		*dev;
	int i;

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	set_bit(0,&card->in_isr);
	++card->statistics.isr_entry;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	
	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	if (!dev && flags.iflag != PPP_INTR_CMD){
		goto wpp_isr_done;
	}
	
	if (test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		goto wpp_isr_done;
	}
	
	
	if(card->type != SDLA_S514){
		if (test_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
			++card->statistics.isr_already_critical;
			printk (KERN_INFO "%s: Critical while in ISR!\n",
					card->devname);
			goto wpp_isr_done;
		}
	}

	switch (flags.iflag) {

		case PPP_INTR_RXRDY:	/* receive interrupt  0x01  (bit 0)*/
			++card->statistics.isr_rx;
			rx_intr(card);
			break;

		case PPP_INTR_TXRDY:	/* transmit interrupt  0x02 (bit 1)*/
			++card->statistics.isr_tx;
			card->hw_iface.clear_bit(card->hw, card->intr_perm_off, PPP_INTR_TXRDY);
			wake_net_dev(dev);
			break;

		case PPP_INTR_CMD:      /* interface command completed */
			++card->timer_int_enabled;
			++card->statistics.isr_intr_test;
			break;

		case PPP_INTR_MODEM:    /* modem status change (DCD, CTS) 0x04 (bit 2)*/
		case PPP_INTR_DISC:  	/* Data link disconnected 0x10  (bit 4)*/	
		case PPP_INTR_OPEN:   	/* Data link open 0x20  (bit 5)*/
		case PPP_INTR_DROP_DTR:	/* DTR drop timeout expired  0x40 bit 6 */
			event_intr(card);
			break;
	
		case PPP_INTR_TIMER:
			timer_intr(card);
			break;	 

		default:	/* unexpected interrupt */
			++card->statistics.isr_spurious;
			printk(KERN_INFO "%s: spurious interrupt 0x%02X!\n", 
				card->devname, flags.iflag);
			printk(KERN_INFO "%s: ID Bytes = ",card->devname);
			{
				unsigned char str[8];
				card->hw_iface.peek(card->hw, card->intr_type_off + 0x28, str, 8);
		 		for(i = 0; i < 8; i ++){
					printk("0x%02X ", str[i]);
				}
			}
			printk("\n");	
	}
	
wpp_isr_done:
	clear_bit(0,&card->in_isr);
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}

/*============================================================================
 * Receive interrupt handler.
 */
static void rx_intr(sdla_t *card)
{
	netdevice_t *dev;
	ppp_private_area_t *ppp_priv_area;
	struct sk_buff *skb;
	unsigned len;
	void *buf;
	int i;
	ppp_buf_ctl_t rxbuf;
	int udp_type;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));

	card->hw_iface.peek(card->hw, card->rxmb_off, &rxbuf, sizeof(rxbuf));
	if (rxbuf.flag != 0x01) {
		printk(KERN_INFO 
			"%s: corrupted Rx buffer @ 0x%lX, flag = 0x%02X!\n", 
			card->devname, card->rxmb_off, rxbuf.flag);
		printk(KERN_INFO "%s: ID Bytes = ",card->devname);
		{
			unsigned char str[8];
			card->hw_iface.peek(card->hw, card->intr_type_off + 0x28, str, 8);
			for(i = 0; i < 8; i ++){
				printk(KERN_INFO "0x%02X ", str[i]);
			}
		}
		printk(KERN_INFO "\n");	
		
		++card->statistics.rx_intr_corrupt_rx_bfr;


		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it means that driver 
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash. 
                 */
		printk(KERN_INFO "%s: Critical router failure ...!!!\n", card->devname);
		printk(KERN_INFO "Please contact Sangoma Technologies !\n");
		ppp_set_intr_mode(card,0);
		return;
	}
      
	if (dev && is_dev_running(dev) && wan_netif_priv(dev)){
		len  = rxbuf.length;
		ppp_priv_area = wan_netif_priv(dev);

		/* Allocate socket buffer */
		skb = dev_alloc_skb(len+2);

		if (skb != NULL) {
			/* Copy data to the socket buffer */
			unsigned addr = rxbuf.buf.ptr;

			if ((addr + len) > card->u.p.rx_top_off + 1) {
				unsigned tmp = card->u.p.rx_top_off - addr + 1;
				buf = skb_put(skb, tmp);
				card->hw_iface.peek(card->hw, addr, buf, tmp);
				addr = card->u.p.rx_base_off;
				len -= tmp;
			}
			buf = skb_put(skb, len);
			card->hw_iface.peek(card->hw, addr, buf, len);

			/* Decapsulate packet */
        		switch (rxbuf.proto) {
				case 0x00:
					skb->protocol = htons(ETH_P_IP);
					break;

				case 0x01:
					skb->protocol = htons(ETH_P_IPX);
					break;

				default:
					skb->protocol = htons(ETH_P_IP);
					break;
			}

			udp_type = udp_pkt_type( skb, card );

			if (udp_type == UDP_PTPIPE_TYPE){
				/* Handle a UDP Request in Timer Interrupt */
				if(store_udp_mgmt_pkt(UDP_PKT_FRM_NETWORK, card, skb, dev,
                	              			ppp_priv_area)){
					printk(KERN_INFO "Storing Network UDP!\n");
					card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
				}
				++ppp_priv_area->rx_intr_stat.rx_intr_PIPE_request;

			} else if (handle_IPXWAN(skb->data,card->devname, 
						 ppp_priv_area->enable_IPX, 
						 ppp_priv_area->network_number, 
						 skb->protocol)) {
			
				/* Handle an IPXWAN packet */
				if( ppp_priv_area->enable_IPX) {
					/* Make sure we are not already sending */
					if (!test_bit(SEND_CRIT, &card->wandev.critical)){
					 	int err=ppp_send(card, skb->data, skb->len, 
							     htons(ETH_P_IPX));
						if (err != 0){
							++card->wandev.stats.tx_dropped;
						}
					}else{
						++card->wandev.stats.tx_dropped;
					}
				}else{
					++card->wandev.stats.rx_dropped;
				}
				wan_dev_kfree_skb(skb,FREE_READ);

			}else if (ppp_priv_area->common.usedby == BRIDGE ||
				  ppp_priv_area->common.usedby == BRIDGE_NODE){
				
				/* Make sure it's an Ethernet frame, otherwise drop it */
				if (skb->len <= ETH_ALEN) {
					wan_dev_kfree_skb(skb,FREE_READ);
					++card->wandev.stats.rx_errors;
					goto rx_exit;
				}
		
				++card->wandev.stats.rx_packets;
				card->wandev.stats.rx_bytes += skb->len;
		    		++ppp_priv_area->rx_intr_stat.rx_intr_bfr_passed_to_stack;

				skb->dev = dev;
				skb->protocol=eth_type_trans(skb,dev);
				netif_rx(skb);

			}else{
				/* Pass data up the protocol stack */
	    			skb->dev = dev;
				wan_skb_reset_mac_header(skb);

			    	++card->wandev.stats.rx_packets;
				card->wandev.stats.rx_bytes += skb->len;
		    		++ppp_priv_area->rx_intr_stat.rx_intr_bfr_passed_to_stack;	
				netif_rx(skb);
			}

		} else {
	
			if (net_ratelimit()){
				printk(KERN_INFO "%s: no socket buffers available!\n",
					card->devname);
			}
			++card->wandev.stats.rx_dropped;
			++ppp_priv_area->rx_intr_stat.rx_intr_no_socket;
		}

	} else {
		++card->statistics.rx_intr_dev_not_started;
	}

rx_exit:
	/* Release buffer element and calculate a pointer to the next one */
	rxbuf.flag = 0x00;
	card->hw_iface.poke(card->hw, card->rxmb_off, &rxbuf, sizeof(rxbuf));
	card->rxmb_off += sizeof(rxbuf);
	if (card->rxmb_off > card->u.p.rxbuf_last_off)
		card->rxmb_off = card->u.p.rxbuf_base_off;
}


void event_intr (sdla_t *card)
{
 	netdevice_t		*dev;
        ppp_private_area_t	*ppp_priv_area;
	ppp_flags_t	flags;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	ppp_priv_area = wan_netif_priv(dev);
	
	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	switch (flags.iflag){

		case PPP_INTR_MODEM:    /* modem status change (DCD, CTS) 0x04  (bit 2)*/

			if (IS_56K_CARD(card)) {
				FRONT_END_STATUS_STRUCT	FE_status;
				unsigned long 		FE_status_off = 0xF020;
				card->hw_iface.peek(card->hw, FE_status_off, &FE_status, sizeof(FE_status));
				card->fe.fe_param.k56_param.RR8_reg_56k = 
					FE_status.FE_U.stat_56k.RR8_56k;	
				card->fe.fe_param.k56_param.RRA_reg_56k = 
					FE_status.FE_U.stat_56k.RRA_56k;	
				card->fe.fe_param.k56_param.RRC_reg_56k = 
					FE_status.FE_U.stat_56k.RRC_56k;	
				card->wandev.fe_iface.read_alarm(&card->fe, 0);

				FE_status.opp_flag = 0x01;
				card->hw_iface.poke(card->hw, FE_status_off, &FE_status, sizeof(FE_status));
				ppp_handle_front_end_state(card);	
				break;
			}

			if (IS_TE1_CARD(card)) {
				FRONT_END_STATUS_STRUCT	FE_status;
				unsigned long 		FE_status_off = 0xF020;
				card->hw_iface.peek(card->hw, FE_status_off, &FE_status, sizeof(FE_status));

				/* TE_INTR */
				card->wandev.fe_iface.isr(&card->fe);
				FE_status.opp_flag = 0x01;
				card->hw_iface.poke(card->hw, FE_status_off, &FE_status, sizeof(FE_status));
				ppp_handle_front_end_state(card);
				break;
			}	
			if (flags.mstatus == 0x28){
				card->fe.fe_status = FE_CONNECTED;			
			}else{
				card->fe.fe_status = FE_DISCONNECTED;	
			}
		
			if (net_ratelimit()){
				printk (KERN_INFO "\n");
				printk (KERN_INFO "%s: Modem status: DCD=%s CTS=%s\n",
					card->devname, DCD(flags.mstatus), CTS(flags.mstatus));
			}

			ppp_handle_front_end_state(card);
			break;

		case PPP_INTR_DISC:  	/* Data link disconnected 0x10  (bit 4)*/	

			printk (KERN_INFO "\n");
			printk (KERN_INFO "%s: Data link disconnected intr\n",
					       card->devname);
			show_disc_cause(card, flags.disc_cause);

			if (flags.disc_cause &
				(PPP_LOCAL_TERMINATION |
				 PPP_REMOTE_TERMINATION)) {
				if (card->u.p.ip_mode == WANOPT_PPP_PEER) { 
					set_bit(0,&Read_connection_info);
				}
				ppp_priv_area->ppp_state = WAN_DISCONNECTED;
				wanpipe_set_state(card, WAN_DISCONNECTED);

				ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_PPP_EVENT;
				card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
				trigger_ppp_poll(dev);

				/* Start debugging */
				WAN_DEBUG_START(card);
			}
			break;

		case PPP_INTR_OPEN:   	/* Data link open 0x20  (bit 5)*/

			NEX_PRINTK (KERN_INFO "%s: PPP Link Open, LCP=%s IP=%s\n",
					card->devname,LCP(flags.lcp_state),
					IP(flags.ip_state));

			if (flags.lcp_state == 0x09 && 
                           (flags.ip_state == 0x09 || flags.ipx_state == 0x09)){
                                /* Initialize the polling timer and set the state
                                 * to WAN_CONNNECTED */


				/* BUG FIX: When the protocol restarts, during heavy 
                                 * traffic, board tx buffers and driver tx buffers
                                 * can go out of sync.  This checks the condition
                                 * and if the tx buffers are out of sync, the 
                                 * protocols are restarted. 
                                 * I don't know why the board tx buffer is out
                                 * of sync. It could be that a packets is tx
                                 * while the link is down, but that is not 
                                 * possible. The other possiblility is that the
                                 * firmware doesn't reinitialize properly.
                                 * FIXME: A better fix should be found.
                                 */ 
				if (detect_and_fix_tx_bug(card)){

					ppp_comm_disable(card);

					ppp_priv_area->ppp_state = WAN_DISCONNECTED;
					wanpipe_set_state(card, WAN_DISCONNECTED);

					ppp_priv_area->timer_int_enabled |= 
						TMR_INT_ENABLED_PPP_EVENT;
					card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
					/* Start debugging */
					WAN_DEBUG_START(card);
					break;	
				}

				card->state_tick = jiffies;
				ppp_priv_area->ppp_state = WAN_CONNECTED;

				if (card->wandev.ignore_front_end_status == WANOPT_YES || 
				    card->fe.fe_status == FE_CONNECTED){
					u32 tmp;
					
					wanpipe_set_state(card, WAN_CONNECTED);

					card->hw_iface.peek(card->hw, card->u.p.txbuf_next_off, &tmp, 4);
					NEX_PRINTK(KERN_INFO "CON: L Tx: %lx  B Tx: %lx || L Rx %lx B Rx %lx\n",
						card->u.p.txbuf_off, tmp,
						card->rxmb_off, card->u.p.rxbuf_next_off);

					/* Tell timer interrupt that PPP event occured */
					ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_PPP_EVENT;
					card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);

					/* If we are in PEER mode, we must first obtain the
					 * IP information and then go into the poll routine */
					if (card->u.p.ip_mode != WANOPT_PPP_PEER){	
						trigger_ppp_poll(dev);
					}
				}
			}
                   	break;

		case PPP_INTR_DROP_DTR:		/* DTR drop timeout expired  0x40 bit 6 */

			printk (KERN_INFO "\n");
			printk(KERN_INFO "%s: DTR Drop Timeout Interrupt \n",card->devname); 

			if (card->u.p.ip_mode == WANOPT_PPP_PEER) { 
				set_bit(0,&Read_connection_info);
			}
	
			ppp_priv_area->ppp_state = WAN_DISCONNECTED;
			wanpipe_set_state(card, WAN_DISCONNECTED);

			ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_PPP_EVENT;
			show_disc_cause(card, flags.disc_cause);
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
			trigger_ppp_poll(dev);
					
			/* Start debugging */
			WAN_DEBUG_START(card);
			break;
		
		default:
			printk(KERN_INFO "%s: Error, Invalid PPP Event\n",card->devname);
	}
}



/* TIMER INTERRUPT */

void timer_intr (sdla_t *card)
{
        netdevice_t		*dev;
        ppp_private_area_t	*ppp_priv_area;


	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev)){
		DEBUG_EVENT("%s: Error: No dev\n",__FUNCTION__);
		return;
	}

	ppp_priv_area = wan_netif_priv(dev);
	
	if (ppp_priv_area->timer_int_enabled & TMR_INT_ENABLED_CONFIG){
		config_ppp(card);
		ppp_priv_area->timer_int_enabled &= 
					~TMR_INT_ENABLED_CONFIG;	
	}

	/* Update statistics */
	if (ppp_priv_area->timer_int_enabled & TMR_INT_ENABLED_UPDATE){
		
		ppp_get_err_stats(card);
		
		if (IS_TE1_CARD(card)) {	
			card->wandev.fe_iface.read_alarm(&card->fe, 0); 
			/* TE1 Update T1/E1 perfomance counters */
			card->wandev.fe_iface.read_pmon(&card->fe, 0); 
		}else if (IS_56K_CARD(card)) {
			/* 56K Update CSU/DSU alarms */
			card->wandev.fe_iface.read_alarm(&card->fe, 1);
		}

                if(!(--ppp_priv_area->update_comms_stats)){
			ppp_priv_area->timer_int_enabled &= 
				~TMR_INT_ENABLED_UPDATE;
		}
	}

	/* TE timer interrupt */
	if (ppp_priv_area->timer_int_enabled & TMR_INT_ENABLED_TE){
		card->wandev.fe_iface.polling(&card->fe);
		ppp_priv_area->timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}

	/* PPIPEMON UDP request */
	if (ppp_priv_area->timer_int_enabled & TMR_INT_ENABLED_UDP){
		process_udp_mgmt_pkt(card,dev, ppp_priv_area,0);
		ppp_priv_area->timer_int_enabled &= ~TMR_INT_ENABLED_UDP;
	}

	/* PPP Event */
	if (ppp_priv_area->timer_int_enabled & TMR_INT_ENABLED_PPP_EVENT){

		if (ppp_priv_area->ppp_state == WAN_DISCONNECTED){
			int err;
			if ((err=retrigger_comm(card)) < 0){
				/* DTR has been dropped.  The card will 
				 * NOT give us an interrupt, thus do not turn off
				 * interrupt, let it retry the command again */
				if (err != -EBUSY){
					ppp_priv_area->timer_int_enabled &= ~TMR_INT_ENABLED_PPP_EVENT;
				}
			}
		}

		/* If the state is CONNECTING, it means that communicatins were
	 	 * enabled. When the remote side enables its comminication we
	 	 * should get an interrupt PPP_INTR_OPEN, thus turn off polling 
		 */

		else if (ppp_priv_area->ppp_state == WAN_CONNECTING){
			/* Turn off the timer interrupt */
			ppp_priv_area->timer_int_enabled &= ~TMR_INT_ENABLED_PPP_EVENT;
		}

		/* If state is connected and we are in PEER mode 
	 	 * poll for an IP address which will be provided by remote end.
		 *
		 * We must wait for the main state to become connected,
		 * because the front end might be disconnected still,
		 * and the interface might be down.
	 	 */
		else if ((card->wandev.state == WAN_CONNECTED && 
		  	  card->u.p.ip_mode == WANOPT_PPP_PEER) && 
		  	  test_bit(0,&Read_connection_info)){

			card->state_tick = jiffies;
			if (read_connection_info (card)){
				printk(KERN_INFO "%s: Failed to read PEER IP Addresses\n",
					card->devname);
			}else{
				clear_bit(0,&Read_connection_info);
				set_bit(1,&Read_connection_info);
				trigger_ppp_poll(dev);
			}
		}else{
			//FIXME Put the comment back int
			ppp_priv_area->timer_int_enabled &= ~TMR_INT_ENABLED_PPP_EVENT;
		}

	}/* End of PPP_EVENT */


	/* Only disable the timer interrupt if there are no udp, statistic */
	/* updates or events pending */
        if(!ppp_priv_area->timer_int_enabled) {
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
        }
}


static int handle_IPXWAN(unsigned char *sendpacket, char *devname, unsigned char enable_IPX, unsigned long network_number, unsigned short proto)
{
	int i;

	if( proto == htons(ETH_P_IPX) ) {
		//It's an IPX packet
		if(!enable_IPX) {
			//Return 1 so we don't pass it up the stack.
			return 1;
		}
	} else {
		//It's not IPX so pass it up the stack.
		return 0;
	}

	if( sendpacket[16] == 0x90 &&
	    sendpacket[17] == 0x04)
	{
		//It's IPXWAN

		if( sendpacket[2] == 0x02 &&
		    sendpacket[34] == 0x00)
		{
			//It's a timer request packet
			printk(KERN_INFO "%s: Received IPXWAN Timer Request packet\n",devname);

			//Go through the routing options and answer no to every
			//option except Unnumbered RIP/SAP
			for(i = 41; sendpacket[i] == 0x00; i += 5)
			{
				//0x02 is the option for Unnumbered RIP/SAP
				if( sendpacket[i + 4] != 0x02)
				{
					sendpacket[i + 1] = 0;
				}
			}

			//Skip over the extended Node ID option
			if( sendpacket[i] == 0x04 )
			{
				i += 8;
			}

			//We also want to turn off all header compression opt.
			for(; sendpacket[i] == 0x80 ;)
			{
				sendpacket[i + 1] = 0;
				i += (sendpacket[i + 2] << 8) + (sendpacket[i + 3]) + 4;
			}

			//Set the packet type to timer response
			sendpacket[34] = 0x01;

			printk(KERN_INFO "%s: Sending IPXWAN Timer Response\n",devname);
		}
		else if( sendpacket[34] == 0x02 )
		{
			//This is an information request packet
			printk(KERN_INFO "%s: Received IPXWAN Information Request packet\n",devname);

			//Set the packet type to information response
			sendpacket[34] = 0x03;

			//Set the router name
			sendpacket[51] = 'P';
			sendpacket[52] = 'T';
			sendpacket[53] = 'P';
			sendpacket[54] = 'I';
			sendpacket[55] = 'P';
			sendpacket[56] = 'E';
			sendpacket[57] = '-';
			sendpacket[58] = CVHexToAscii(network_number >> 28);
			sendpacket[59] = CVHexToAscii((network_number & 0x0F000000)>> 24);
			sendpacket[60] = CVHexToAscii((network_number & 0x00F00000)>> 20);
			sendpacket[61] = CVHexToAscii((network_number & 0x000F0000)>> 16);
			sendpacket[62] = CVHexToAscii((network_number & 0x0000F000)>> 12);
			sendpacket[63] = CVHexToAscii((network_number & 0x00000F00)>> 8);
			sendpacket[64] = CVHexToAscii((network_number & 0x000000F0)>> 4);
			sendpacket[65] = CVHexToAscii(network_number & 0x0000000F);
			for(i = 65; i < 99; i+= 1)
			{
				sendpacket[i] = 0;
			}

			printk(KERN_INFO "%s: Sending IPXWAN Information Response packet\n",devname);
		}
		else
		{
			printk(KERN_INFO "%s: Unknown IPXWAN packet!\n",devname);
			return 0;
		}

		//Set the WNodeID to our network address
		sendpacket[35] = (unsigned char)(network_number >> 24);
		sendpacket[36] = (unsigned char)((network_number & 0x00FF0000) >> 16);
		sendpacket[37] = (unsigned char)((network_number & 0x0000FF00) >> 8);
		sendpacket[38] = (unsigned char)(network_number & 0x000000FF);

		return 1;
	} else {
		//If we get here's its an IPX-data packet, so it'll get passed up the stack.

		//switch the network numbers
		switch_net_numbers(sendpacket, network_number, 1);	
		return 0;
	}
}

/****** Background Polling Routines  ****************************************/

/* All polling functions are invoked by the TIMER interrupt in the wpp_isr 
 * routine.  
 */

/*============================================================================
 * Monitor active link phase.
 */
static void process_route (sdla_t *card)
{
	ppp_flags_t 		flags;
	netdevice_t 		*dev;
	ppp_private_area_t 	*ppp_priv_area;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	ppp_priv_area = wan_netif_priv(dev);
	
	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	if ((card->u.p.ip_mode == WANOPT_PPP_PEER) &&
	    (flags.ip_state == 0x09)){ 

		/* We get ip_local from the firmware in PEER mode.
	         * Therefore, if ip_local is 0, we failed to obtain
         	 * the remote IP address. */
		if (ppp_priv_area->ip_local == 0) 
			return;
		
		printk(KERN_INFO "%s: IPCP State Opened.\n", card->devname);
		if (read_info( card )) {
   			printk(KERN_INFO 
				"%s: An error occurred in IP assignment.\n", 
				card->devname);
		} else {
			struct in_device *in_dev = dev->ip_ptr;
			if (in_dev != NULL ) {
				struct in_ifaddr *ifa = in_dev->ifa_list;
				if (ifa){
					printk(KERN_INFO "%s: Assigned Lcl. Addr: %u.%u.%u.%u\n", 
						card->devname, NIPQUAD(ifa->ifa_local));
					printk(KERN_INFO "%s: Assigned Rmt. Addr: %u.%u.%u.%u\n", 
							card->devname, NIPQUAD(ifa->ifa_address));
				}
			}else{
				printk(KERN_INFO 
				"%s: Error: Failed to add a route for PPP interface %s\n",
					card->devname,dev->name);	
			}
		}
	}
}

/*============================================================================
 * Monitor physical link disconnected phase.
 *  o if interface is up and the hold-down timeout has expired, then retry
 *    connection.
 */
static int retrigger_comm(sdla_t *card)
{
	netdevice_t 		*dev;
	ppp_private_area_t	*ppp_priv_area;
	int err=0;

	dev= WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return 0;
		
	ppp_priv_area = wan_netif_priv(dev);
	if ((jiffies - card->state_tick) > HOLD_DOWN_TIME){

		/*Bug Fix: May 24 2001
		 * Only set state if the communicatins are
		 * enabled */
		err = ppp_comm_enable(card);
	
		switch (err){

		case 0x00: 
			/* Communications are back up, change
			 * state to connecting and reinitialize
			 * buffers */
			ppp_priv_area->ppp_state = WAN_CONNECTING;
			wanpipe_set_state(card, WAN_CONNECTING);
			init_ppp_tx_rx_buff( card );
			ppp_priv_area->comm_busy_retry=0;
			break;
			
		case 0x0A:  
			printk(KERN_INFO "%s: Comm enable busy, retrying in %i sec.\n",
					card->devname,HOLD_DOWN_TIME);
			ppp_comm_disable(card);	
			card->state_tick=jiffies;
			
			/* We are in process of handing up, retry
			 * again later by leaving the wanpipe  
			 * device in DISCONNECTED state */
			
			return -EBUSY;

		/* NB: 
		 * By setting wanpipe state to connecting, the
		 * communicatins will stay in disabled state, and the
		 * retries will fail */
			
		case 0x01:
			printk(KERN_INFO "%s: ERROR PPP Com-Enable failed: protocol not configured!\n",
					card->devname);
			return -EFAULT;

		case 0x09:
			printk(KERN_INFO "%s: ERROR PPP Com-Enable failed: already enabled!\n",
					card->devname);
			ppp_priv_area->ppp_state = WAN_CONNECTING;
			wanpipe_set_state(card, WAN_CONNECTING);
			break;

		default:
			printk(KERN_INFO "%s: ERROR PPP Com-Enable failed: unknown rc=0x%X!\n",
					card->devname,err);
			return -EINVAL;
		}		
	}
	return 0;
}

/****** Miscellaneous Functions *********************************************/

/*============================================================================
 * Configure S508 adapter.
 */
static int config508(netdevice_t *dev, sdla_t *card)
{
	ppp508_conf_t cfg;
	ppp_private_area_t *ppp_priv_area = wan_netif_priv(dev);

	/* Prepare PPP configuration structure */
	memset(&cfg, 0, sizeof(ppp508_conf_t));

	if (card->wandev.clocking)
		cfg.line_speed = card->wandev.bps;

	if (card->wandev.electrical_interface == WANOPT_RS232)
		cfg.conf_flags |= INTERFACE_LEVEL_RS232;


        cfg.conf_flags 	|= DONT_TERMINATE_LNK_MAX_CONFIG; /*send Configure-Request packets forever*/
	cfg.txbuf_percent	= PERCENT_TX_BUFF;	/* % of Tx bufs */
	cfg.mtu_local		= card->wandev.mtu;
	cfg.mtu_remote		= card->wandev.mtu;                  /*    Default   */
	cfg.restart_tmr		= TIME_BETWEEN_CONF_REQ;  	     /*    30 = 3sec */
	cfg.auth_rsrt_tmr	= TIME_BETWEEN_PAP_CHAP_REQ;         /*    30 = 3sec */
	cfg.auth_wait_tmr	= WAIT_PAP_CHAP_WITHOUT_REPLY;       /*   300 = 30s  */
	cfg.mdm_fail_tmr	= WAIT_AFTER_DCD_CTS_LOW;            /*     5 = 0.5s */
	cfg.dtr_drop_tmr	= TIME_DCD_CTS_LOW_AFTER_LNK_DOWN;   /*    10 = 1s   */
	cfg.connect_tmout	= WAIT_DCD_HIGH_AFTER_ENABLE_COMM;   /*   900 = 90s  */
	cfg.conf_retry		= MAX_CONF_REQ_WITHOUT_REPLY;        /*    10 = 1s   */
	cfg.term_retry		= MAX_TERM_REQ_WITHOUT_REPLY;	     /*     2 times  */
	cfg.fail_retry		= NUM_CONF_NAK_WITHOUT_REPLY;        /*     5 times  */
	cfg.auth_retry		= NUM_AUTH_REQ_WITHOUT_REPLY;        /*     10 times */   


	if( !card->u.p.authenticator ) {
		printk(KERN_INFO "%s: Device is not configured as an authenticator\n", 
				card->devname);
		cfg.auth_options = NO_AUTHENTICATION;
	}else{
		printk(KERN_INFO "%s: Device is configured as an authenticator\n", 
				card->devname);
		cfg.auth_options = INBOUND_AUTH;
	}

	if( ppp_priv_area->pap == WANOPT_YES){
		cfg.auth_options |=PAP_AUTH;
		printk(KERN_INFO "%s: Pap enabled\n", card->devname);
	}
	if( ppp_priv_area->chap == WANOPT_YES){
		cfg.auth_options |= CHAP_AUTH;
		printk(KERN_INFO "%s: Chap enabled\n", card->devname);
	}


	if (ppp_priv_area->enable_IPX == WANOPT_YES){
		printk(KERN_INFO "%s: Enabling IPX Protocol\n",card->devname);
		cfg.ipx_options		= ENABLE_IPX | ROUTING_PROT_DEFAULT;
	}else{
		cfg.ipx_options 	= DISABLE_IPX;
	}

	switch (card->u.p.ip_mode) {
	
		case WANOPT_PPP_STATIC:

			printk(KERN_INFO "%s: PPP IP Mode: STATIC\n",card->devname);
			cfg.ip_options		= L_AND_R_IP_NO_ASSIG | 
							    ENABLE_IP;
			cfg.ip_local		= ppp_priv_area->ip_local;
			cfg.ip_remote		= ppp_priv_area->ip_remote;
			break;

		case WANOPT_PPP_HOST:

			printk(KERN_INFO "%s: PPP IP Mode: HOST\n",card->devname);
			cfg.ip_options		= L_IP_LOCAL_ASSIG |
						  R_IP_LOCAL_ASSIG | 
						  ENABLE_IP;

			cfg.ip_local		= ppp_priv_area->ip_local;
			cfg.ip_remote		= ppp_priv_area->ip_remote;
			break;
	
		case WANOPT_PPP_PEER:

			printk(KERN_INFO "%s: PPP IP Mode: PEER\n",card->devname);
			cfg.ip_options		= L_IP_REMOTE_ASSIG | 
						  R_IP_REMOTE_ASSIG | 
							  ENABLE_IP;
			cfg.ip_local		= 0x00;
			cfg.ip_remote		= 0x00;
			break;

		default:
			printk(KERN_INFO "%s: ERROR: Unsuported PPP Mode Selected\n",
					card->devname);
			printk(KERN_INFO "%s:        PPP IP Modes: STATIC, PEER or HOST\n",
					card->devname);	
			return 1;
	}

	return ppp_configure(card, &cfg);
}

/*============================================================================
 * Show disconnection cause.
 */
static void show_disc_cause(sdla_t *card, unsigned cause)
{
	if (cause & 0x0802) 

		printk(KERN_INFO "%s: link terminated by peer\n", 
			card->devname);

	else if (cause & 0x0004) 

		printk(KERN_INFO "%s: link terminated by user\n", 
			card->devname);

	else if (cause & 0x0008) 

		printk(KERN_INFO "%s: authentication failed\n", card->devname);
	
	else if (cause & 0x0010) 

		printk(KERN_INFO 
			"%s: authentication protocol negotiation failed\n", 
			card->devname);

	else if (cause & 0x0020) 
		
		printk(KERN_INFO
		"%s: peer's request for authentication rejected\n",
		card->devname);

	else if (cause & 0x0040) 
	
		printk(KERN_INFO "%s: MRU option rejected by peer\n", 
		card->devname);

	else if (cause & 0x0080) 
	
		printk(KERN_INFO "%s: peer's MRU was too small\n", 
		card->devname);

	else if (cause & 0x0100) 

		printk(KERN_INFO "%s: failed to negotiate peer's LCP options\n",
		card->devname);

	else if (cause & 0x0200) 
		
		printk(KERN_INFO "%s: failed to negotiate peer's IPCP options\n"
		, card->devname);

	else if (cause & 0x0400) 

		printk(KERN_INFO 
			"%s: failed to negotiate peer's IPXCP options\n",
			card->devname);
}

/*=============================================================================
 * Process UDP call of type PTPIPEAB.
 */
static void process_udp_mgmt_pkt(sdla_t *card, netdevice_t *dev, 
				 ppp_private_area_t *ppp_priv_area, int local_dev) 
{
	unsigned char buf2[5];
	unsigned char *buf;
	unsigned int frames, len;
	struct sk_buff *new_skb;
	unsigned short data_length, buffer_length, real_len;
	unsigned long data_ptr;
	int udp_mgmt_req_valid = 1;
	wan_mbox_t *mb = &card->wan_mbox;
	struct timeval tv;
	int err;
	wan_udp_pkt_t *wan_udp_pkt;
	
	memcpy(&buf2, &card->wandev.udp_port, 2 );
	wan_udp_pkt= (wan_udp_pkt_t*)&ppp_priv_area->udp_pkt_data;
	
	if (!local_dev){

		if(ppp_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK) {

			switch(wan_udp_pkt->wan_udp_command) {

				case PPIPE_GET_IBA_DATA:
				case PPP_READ_CONFIG:
				case PPP_GET_CONNECTION_INFO:
				case PPIPE_ROUTER_UP_TIME:
				case PPP_READ_STATISTICS:
				case PPP_READ_ERROR_STATS:
				case PPP_READ_PACKET_STATS:
				case PPP_READ_LCP_STATS:
				case PPP_READ_IPCP_STATS:
				case PPP_READ_IPXCP_STATS:
				case PPP_READ_PAP_STATS:
				case PPP_READ_CHAP_STATS:
				case PPP_READ_CODE_VERSION:
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
	
    	wan_udp_pkt->wan_udp_opp_flag = 0x00;
  	if(!udp_mgmt_req_valid) {
		/* set length to 0 */
    		wan_udp_pkt->wan_udp_data_len = 0x00;

    		/* set return code */
    		wan_udp_pkt->wan_udp_return_code = 0xCD; 
		++ppp_priv_area->pipe_mgmt_stat.UDP_PIPE_mgmt_direction_err;
	
		if (net_ratelimit()){	
			printk(KERN_INFO 
			"%s: Warning, Illegal UDP command attempted from network: %x\n",
			card->devname,wan_udp_pkt->wan_udp_command);
		}
   	} else {
		/* Initialize the trace element */
		trace_element_t trace_element;		    

		switch (wan_udp_pkt->wan_udp_command){

		/* PPIPE_ENABLE_TRACING */
    		case PPIPE_ENABLE_TRACING:
			if (!card->TracingEnabled) {
    			
				/* OPERATE_DATALINE_MONITOR */
    				mb->wan_command = PPP_DATALINE_MONITOR;
    				mb->wan_data_len = 0x01;
    				mb->wan_data[0] = wan_udp_pkt->wan_udp_data[0];
	    			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	   
				if (err != CMD_OK) { 
	        			
					ppp_error(card, err, mb);
	        			card->TracingEnabled = 0;
	        		
					/* set the return code */

		        		wan_udp_pkt->wan_udp_return_code = mb->wan_return_code;
					wan_udp_pkt->wan_udp_data_len=0;
	        			break;
	    			} 

				card->hw_iface.peek(card->hw, 0xC000, &buf2, 2);
		    
				ppp_priv_area->curr_trace_addr = 0;
		    		memcpy(&ppp_priv_area->curr_trace_addr, &buf2, 2);
		    		ppp_priv_area->start_trace_addr = 
						ppp_priv_area->curr_trace_addr;
				ppp_priv_area->end_trace_addr = 
					ppp_priv_area->start_trace_addr + END_OFFSET;
		    	
				/* MAX_SEND_BUFFER_SIZE - 28 (IP header) 
				   - 32 (ppipemon CBLOCK) */
		    		available_buffer_space = MAX_LGTH_UDP_MGNT_PKT - 
							 sizeof(struct iphdr)-
							 sizeof(struct udphdr)-
							 sizeof(wan_mgmt_t)-
							 sizeof(wan_cmd_t);
	       	  	}
	       	  	wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len=0;
	       	  	card->TracingEnabled = 1;
	       	  	break;
	   
		/* PPIPE_DISABLE_TRACING */
		case PPIPE_DISABLE_TRACING:
	      		
			if(card->TracingEnabled) {
		   	
				/* OPERATE_DATALINE_MONITOR */
		    		mb->wan_command = 0x33;
		    		mb->wan_data_len = 1;
		    		mb->wan_data[0] = 0x00;
		    		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			} 
		
			/*set return code*/
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len=0;
			card->TracingEnabled = 0;
			break;
	   
		/* PPIPE_GET_TRACE_INFO */
		case PPIPE_GET_TRACE_INFO:

			if(!card->TracingEnabled) {
				/* set return code */
	    			wan_udp_pkt->wan_udp_return_code = 1;
				wan_udp_pkt->wan_udp_data_len=0;
				printk(KERN_INFO "%s: PPP Tracing not enabled!\n",
						card->devname);
				break;
			}		    

			buffer_length = 0;
			
			/* frames < 62, where 62 is the number of trace
			   information elements.  There is in total 496
			   bytes of space and each trace information
			   element is 8 bytes. 
			 */
			for ( frames=0; frames<62; frames++) {
	
				trace_pkt_t *trace_pkt = (trace_pkt_t *)
					&wan_udp_pkt->wan_udp_data[buffer_length];

				/* Read the whole trace packet */
				card->hw_iface.peek(card->hw, ppp_priv_area->curr_trace_addr, 
					  &trace_element, sizeof(trace_element_t));
	
				/* no data on board so exit */
				if( trace_element.opp_flag == 0x00 ) 
					break;
	      
				data_ptr = trace_element.trace_data_ptr;

				/* See if there is actual data on the trace buffer */
				if (data_ptr){
					data_length = trace_element.trace_length;
				}else{
					data_length = 0;
					wan_udp_pkt->wan_udp_data[0] |= 0x02;
				}

				//FIXME: Do we need this check
				if ((available_buffer_space - buffer_length) 
				     < (sizeof(trace_pkt_t)+1)){
					
					/*indicate we have more frames 
					 * on board and exit 
					 */
					wan_udp_pkt->wan_udp_data[0] |= 0x02;
					break;
				}
				
				trace_pkt->status = trace_element.trace_type;
				trace_pkt->time_stamp = trace_element.trace_time_stamp;
				trace_pkt->real_length = trace_element.trace_length;

				real_len = trace_element.trace_length;	
				
				if(data_ptr == 0){
					trace_pkt->data_avail = 0x00;
				}else{
					/* we can take it next time */
					if ((available_buffer_space - buffer_length)<
						(real_len + sizeof(trace_pkt_t))){
					
						wan_udp_pkt->wan_udp_data[0] |= 0x02;
						break;
					} 
					trace_pkt->data_avail = 0x01;
				
					/* get the data */
					card->hw_iface.peek(card->hw, data_ptr, 
						  &trace_pkt->data,
						  real_len);
				}	
				/* zero the opp flag to 
				   show we got the frame */
				buf2[0] = 0x00;
				card->hw_iface.poke_byte(card->hw, ppp_priv_area->curr_trace_addr, 0x00);

				/* now move onto the next 
				   frame */
				ppp_priv_area->curr_trace_addr += sizeof(trace_element_t);

				/* check if we passed the last address */
				if ( ppp_priv_area->curr_trace_addr >= 
					ppp_priv_area->end_trace_addr){

					ppp_priv_area->curr_trace_addr = 
						ppp_priv_area->start_trace_addr;
				}
 
				/* update buffer length and make sure its even */ 

				if ( trace_pkt->data_avail == 0x01 ) {
					buffer_length += real_len - 1;
				}
 
				/* for the header */
				buffer_length += sizeof(trace_pkt_t);

				if( buffer_length & 0x0001 )
					buffer_length += 1;
			}

			/* ok now set the total number of frames passed
			   in the high 5 bits */
			wan_udp_pkt->wan_udp_data[0] |= (frames << 2);
	 
			/* set the data length */
			wan_udp_pkt->wan_udp_data_len = buffer_length;
	 
			/* set return code */
			wan_udp_pkt->wan_udp_return_code = 0;
	      	  	break;

   		/* PPIPE_GET_IBA_DATA */
		case PPIPE_GET_IBA_DATA:
	        
			card->hw_iface.peek(card->hw, 0xF003, &wan_udp_pkt->wan_udp_data, 
					0x09);
	        
			/* set the length of the data */
			wan_udp_pkt->wan_udp_data_len = 0x09;

			/* set return code */
			wan_udp_pkt->wan_udp_return_code = 0x00;
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		/* PPIPE_FT1_READ_STATUS */
		case PPIPE_FT1_READ_STATUS:
			card->hw_iface.peek(card->hw, 0xF020, &wan_udp_pkt->wan_udp_data[0], 2);
			wan_udp_pkt->wan_udp_data_len = 2;
			wan_udp_pkt->wan_udp_return_code = 0;
			break;
		
		case PPIPE_FLUSH_DRIVER_STATS:   
			init_ppp_priv_struct( ppp_priv_area );
			init_global_statistics( card );
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len=0;
			break;

		
		case PPIPE_ROUTER_UP_TIME:

			do_gettimeofday( &tv );
			ppp_priv_area->router_up_time = tv.tv_sec - 
					ppp_priv_area->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = ppp_priv_area->router_up_time;
			wan_udp_pkt->wan_udp_data_len=4;
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		/* PPIPE_DRIVER_STATISTICS */   
		case PPIPE_DRIVER_STAT_IFSEND:
			memcpy(&wan_udp_pkt->wan_udp_data, &ppp_priv_area->if_send_stat, 
				sizeof(if_send_stat_t));

			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len = sizeof(if_send_stat_t);
			break;

		case PPIPE_DRIVER_STAT_INTR:
			memcpy(&wan_udp_pkt->wan_udp_data, &card->statistics, 
				sizeof(global_stats_t));

			memcpy(&wan_udp_pkt->wan_udp_data+sizeof(global_stats_t),
				&ppp_priv_area->rx_intr_stat,
				sizeof(rx_intr_stat_t));

			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len = sizeof(global_stats_t)+
						     sizeof(rx_intr_stat_t);
			break;

		case PPIPE_DRIVER_STAT_GEN:
			memcpy( &wan_udp_pkt->wan_udp_data,
				&ppp_priv_area->pipe_mgmt_stat,
				sizeof(pipe_mgmt_stat_t));

			memcpy(&wan_udp_pkt->wan_udp_data+sizeof(pipe_mgmt_stat_t), 
			       &card->statistics, sizeof(global_stats_t));

			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len = sizeof(global_stats_t)+
						     sizeof(rx_intr_stat_t);
			break;


		/* FT1 MONITOR STATUS */
   		case FT1_MONITOR_STATUS_CTRL:

			wan_udp_pkt->wan_udp_return_code = 0;

			/* Enable FT1 MONITOR STATUS */
	        	if( wan_udp_pkt->wan_udp_data[0] == 1) {
			
				if(card->rCount++ != 0 ) {
		        		wan_udp_pkt->wan_udp_return_code = 0;
					wan_udp_pkt->wan_udp_data_len=1;
		  			break;
		    		}	
	      		}

	      		/* Disable FT1 MONITOR STATUS */
	      		if( wan_udp_pkt->wan_udp_data[0] == 0) {

	      	   		if(--card->rCount != 0) {
		  			wan_udp_pkt->wan_udp_return_code = 0;
					wan_udp_pkt->wan_udp_data_len=1;
		  			break;
	   	    		} 
	      		} 	
			goto udp_dflt_cmd;
		
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
			break;

		case WAN_GET_PROTOCOL:
		   	wan_udp_pkt->wan_udp_data[0] = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		default:
udp_dflt_cmd:
			/* it's a board command */
			mb->wan_command = wan_udp_pkt->wan_udp_command;
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
 
			if(mb->wan_data_len) {
				memcpy(&mb->wan_data,(unsigned char *)wan_udp_pkt->wan_udp_data,
				       mb->wan_data_len);
	      		} 
	          
			/* run the command on the board */
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
				
			if (err != CMD_OK) {
		
				wan_udp_pkt->wan_udp_return_code = mb->wan_return_code;
		    		ppp_error(card, err, mb);
		    		++ppp_priv_area->pipe_mgmt_stat.
					 UDP_PIPE_mgmt_adptr_cmnd_timeout;
				break;
			}
	          
		  	++ppp_priv_area->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_cmnd_OK;
		
			/* copy the result back to our buffer */
			memcpy(&wan_udp_pkt->wan_udp_hdr.wan_cmd, mb, sizeof(wan_cmd_t)); 
	          
			if(mb->wan_data_len) {
				memcpy(&wan_udp_pkt->wan_udp_data,&mb->wan_data,mb->wan_data_len);
			} 

			wan_udp_pkt->wan_udp_data_len = mb->wan_data_len;

		} /* end of switch */
     	} /* end of else */

     	/* Fill UDP TTL */
     	wan_udp_pkt->wan_ip_ttl = card->wandev.ttl; 

	if (local_dev){
		wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY; 
		return;
	}
     
	len = reply_udp(ppp_priv_area->udp_pkt_data, wan_udp_pkt->wan_udp_data_len);

     	if (ppp_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK) {

		/* Make sure we are not already sending */
		if (!test_bit(SEND_CRIT,&card->wandev.critical)){
			++ppp_priv_area->pipe_mgmt_stat.UDP_PIPE_mgmt_passed_to_adptr;
			ppp_send(card,ppp_priv_area->udp_pkt_data,len,ppp_priv_area->protocol);
		}

	} else {	
	
		/* Pass it up the stack
    		   Allocate socket buffer */
		if ((new_skb = dev_alloc_skb(len)) != NULL) {
	    	
			/* copy data into new_skb */

  	    		buf = skb_put(new_skb, len);
  	    		memcpy(buf,ppp_priv_area->udp_pkt_data, len);

	    		++ppp_priv_area->pipe_mgmt_stat.UDP_PIPE_mgmt_passed_to_stack;
			
            		/* Decapsulate packet and pass it up the protocol 
			   stack */
	    		new_skb->protocol = ppp_priv_area->protocol;
            		new_skb->dev = dev;
			wan_skb_reset_mac_header(new_skb);

			netif_rx(new_skb);
		
		} else {
	    	
			++ppp_priv_area->pipe_mgmt_stat.UDP_PIPE_mgmt_no_socket;
			printk(KERN_INFO "no socket buffers available!\n");
  		}
    	}	

	atomic_set(&ppp_priv_area->udp_pkt_len,0);
	return; 
}

/*=============================================================================
 * Initial the ppp_private_area structure.
 */
static void init_ppp_priv_struct( ppp_private_area_t *ppp_priv_area )
{

	memset(&ppp_priv_area->if_send_stat, 0, sizeof(if_send_stat_t));
	memset(&ppp_priv_area->rx_intr_stat, 0, sizeof(rx_intr_stat_t));
	memset(&ppp_priv_area->pipe_mgmt_stat, 0, sizeof(pipe_mgmt_stat_t));	
}

/*============================================================================
 * Initialize Global Statistics
 */
static void init_global_statistics( sdla_t *card )
{
	memset(&card->statistics, 0, sizeof(global_stats_t));
}

/*============================================================================
 * Initialize Receive and Transmit Buffers.
 */
static void init_ppp_tx_rx_buff( sdla_t *card )
{
	ppp508_buf_info_t	info;
	unsigned long 		info_off;

	/* Alex Apr 8 2004 Sangoma ISA card */
	info_off = PPP514_BUF_OFFS;
	card->hw_iface.peek(card->hw, info_off, &info, sizeof(info));

       	card->u.p.txbuf_base_off = info.txb_ptr;

	card->u.p.txbuf_last_off = 
		card->u.p.txbuf_base_off + 
		(info.txb_num - 1) * sizeof(ppp_buf_ctl_t);
	card->u.p.rxbuf_base_off = info.rxb_ptr;
        card->u.p.rxbuf_last_off = 
		card->u.p.rxbuf_base_off +
                (info.rxb_num - 1) * sizeof(ppp_buf_ctl_t);

	card->u.p.txbuf_next_off = info.txb_nxt; 
	card->u.p.rxbuf_next_off = info.rxb1_ptr;

	card->u.p.rx_base_off = info.rxb_base;
        card->u.p.rx_top_off  = info.rxb_end;
      
	card->u.p.txbuf_off = card->u.p.txbuf_base_off;
	card->rxmb_off = card->u.p.rxbuf_base_off;

}
/*=============================================================================
 * Read Connection Information (ie for Remote IP address assginment).
 * Called when ppp interface connected.
 */
static int read_info( sdla_t *card )
{
	netdevice_t *dev;
	ppp_private_area_t *ppp_priv_area;
	int err;
	struct ifreq if_info;
	struct sockaddr_in *if_data1, *if_data2;
	mm_segment_t fs;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return 0;
	ppp_priv_area = wan_netif_priv(dev);

	
	/* Set Local and remote addresses */
	memset(&if_info, 0, sizeof(if_info));
	strcpy(if_info.ifr_name, dev->name);


	fs = get_fs();
	set_fs(get_ds());     /* get user space block */ 

	/* Change the local and remote ip address of the interface.
	 * This will also add in the destination route.
	 */	
	if_data1 = (struct sockaddr_in *)&if_info.ifr_addr;
	if_data1->sin_addr.s_addr = ppp_priv_area->ip_local;
	if_data1->sin_family = AF_INET;
	err = wp_devinet_ioctl( SIOCSIFADDR, &if_info );
	if_data2 = (struct sockaddr_in *)&if_info.ifr_dstaddr;
	if_data2->sin_addr.s_addr = ppp_priv_area->ip_remote;
	if_data2->sin_family = AF_INET;
	err = wp_devinet_ioctl( SIOCSIFDSTADDR, &if_info );

	set_fs(fs);           /* restore old block */
	
	if (err) {
		printk (KERN_INFO "%s: Adding of route failed: %i\n",
			card->devname,err);
		printk (KERN_INFO "%s:	Local : %u.%u.%u.%u\n",
			card->devname,NIPQUAD(ppp_priv_area->ip_local));
		printk (KERN_INFO "%s:	Remote: %u.%u.%u.%u\n",
			card->devname,NIPQUAD(ppp_priv_area->ip_remote));
	}
	return err;
}

/*=============================================================================
 * Remove Dynamic Route.
 * Called when ppp interface disconnected.
 */

static void remove_route( sdla_t *card )
{
	netdevice_t 		*dev;
	long ip_addr;
	int err;
        mm_segment_t fs;
	struct ifreq if_info;
	struct sockaddr_in *if_data1;
        struct in_device *in_dev;
        struct in_ifaddr *ifa;	

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev)
		return;
	in_dev = dev->ip_ptr;
	if (!in_dev)
		return;
	
	if (!(ifa = in_dev->ifa_list))
		return;

	ip_addr = ifa->ifa_local;

	/* Set Local and remote addresses */
	memset(&if_info, 0, sizeof(if_info));
	strcpy(if_info.ifr_name, dev->name);

	fs = get_fs();
       	set_fs(get_ds());     /* get user space block */ 


	/* Change the local ip address of the interface to 0.
	 * This will also delete the destination route.
	 */	
	if_data1 = (struct sockaddr_in *)&if_info.ifr_addr;
	if_data1->sin_addr.s_addr = 0;
	if_data1->sin_family = AF_INET;
	err = wp_devinet_ioctl( SIOCSIFADDR, &if_info );

        set_fs(fs);           /* restore old block */

	
	if (err) {
		printk (KERN_INFO "%s: Deleting dynamic route failed %d!\n",
			 card->devname, err);
		return;
	}else{
		printk (KERN_INFO "%s: PPP Deleting dynamic route %u.%u.%u.%u successfuly\n",
			card->devname, NIPQUAD(ip_addr));
	}
	return;
}

/*=============================================================================
 * Perform the Interrupt Test by running the READ_CODE_VERSION command MAX_INTR
 * _TEST_COUNTER times.
 */
static int intr_test( sdla_t *card )
{
	wan_mbox_t *mb = &card->wan_mbox;
	int err,i;

	err = ppp_set_intr_mode( card, 0x08 );
	
	if (err == CMD_OK) { 
		
		for (i = 0; i < MAX_INTR_TEST_COUNTER; i ++) {	
			/* Run command READ_CODE_VERSION */
			memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
			mb->wan_data_len  = 0;
			mb->wan_command = PPP_READ_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != CMD_OK) 
				ppp_error(card, err, mb);
		}
	}
	else return err;

	err = ppp_set_intr_mode( card, 0 );
	if (err != CMD_OK) 
		return err;

	return 0;
}

/*==============================================================================
 * Determine what type of UDP call it is. DRVSTATS or PTPIPEAB ?
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
			return UDP_PTPIPE_TYPE;
		}
		if (!strncmp(wan_udp_pkt->wan_udp_signature,GLOBAL_UDP_SIGNATURE,8)){	
			return UDP_PTPIPE_TYPE;
		}
		
		if (!strncmp(wan_udp_pkt->wan_udp_signature,UDPDRV_SIGNATURE,8)){	
			return UDP_DRVSTATS_TYPE;
		}

	}

	return UDP_INVALID_TYPE;
}

/*============================================================================
 * Check to see if the packet to be transmitted contains a broadcast or
 * multicast source IP address.
 */

static int chk_bcast_mcast_addr(sdla_t *card, netdevice_t* dev,
				struct sk_buff *skb)
{
	u32 src_ip_addr;
        u32 broadcast_ip_addr = 0;
        struct in_device *in_dev;
        /* read the IP source address from the outgoing packet */
        src_ip_addr = *(u32 *)(skb->data + 12);

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

void s508_lock (sdla_t *card, unsigned long *smp_flags)
{
	spin_lock_irqsave(&card->wandev.lock, *smp_flags);
}

void s508_unlock (sdla_t *card, unsigned long *smp_flags)
{
        spin_unlock_irqrestore(&card->wandev.lock, *smp_flags);
}

static int read_connection_info (sdla_t *card)
{
	wan_mbox_t *mb = &card->wan_mbox;
	netdevice_t 		*dev;
	ppp_private_area_t 	*ppp_priv_area;
	ppp508_connect_info_t *ppp508_connect_info;
	int err;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return 0;
	ppp_priv_area = wan_netif_priv(dev);
	
	memset(&mb->wan_cmd, 0, sizeof(ppp_cmd_t));
	mb->wan_data_len  = 0;
	mb->wan_command = PPP_GET_CONNECTION_INFO;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != CMD_OK) { 
		ppp_error(card, err, mb);
		ppp_priv_area->ip_remote = 0;
		ppp_priv_area->ip_local = 0;
	}
	else {
		ppp508_connect_info = (ppp508_connect_info_t *)mb->wan_data;
		ppp_priv_area->ip_remote = ppp508_connect_info->ip_remote;
		ppp_priv_area->ip_local = ppp508_connect_info->ip_local;

		NEX_PRINTK(KERN_INFO "READ CONNECTION GOT IP ADDRESS %x, %x\n",
				ppp_priv_area->ip_remote,
				ppp_priv_area->ip_local);
	}

	return err;
}

/*===============================================================================
 * config_ppp
 *
 *	Configure the ppp protocol and enable communications.		
 *
 *   	The if_open function binds this function to the poll routine.
 *      Therefore, this function will run every time the ppp interface
 *      is brought up.  
 *      
 *	If the communications are not enabled, proceed to configure
 *      the card and enable communications.
 *
 *      If the communications are enabled, it means that the interface
 *      was shutdown by ether the user or driver. In this case, we 
 *      have to check that the IP addresses have not changed.  If
 *      the IP addresses changed, we have to reconfigure the firmware
 *      and update the changed IP addresses.  Otherwise, just exit.
 */
static int config_ppp (sdla_t *card)
{
	netdevice_t		*dev;
	ppp_private_area_t	*ppp_priv_area;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return 0;
	ppp_priv_area = wan_netif_priv(dev);
	
	if (card->u.p.comm_enabled){

		if (ppp_priv_area->ip_local_tmp != ppp_priv_area->ip_local ||
		    ppp_priv_area->ip_remote_tmp != ppp_priv_area->ip_remote){
			
			/* The IP addersses have changed, we must
                         * stop the communications and reconfigure
                         * the card. Reason: the firmware must know
                         * the local and remote IP addresses. */
			disable_comm(card);
			ppp_priv_area->ppp_state = WAN_DISCONNECTED;
			wanpipe_set_state(card, WAN_DISCONNECTED);
			printk(KERN_INFO 
				"%s: IP addresses changed!\n",
					card->devname);
			printk(KERN_INFO "%s: Restarting communications ...\n",
					card->devname);
		}else{ 
			/* IP addresses are the same and the link is up, 
                         * we dont have to do anything here. Therefore, exit */
			return 0;
		}
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
			return 0;
		}
		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
		
	}else if (IS_56K_CARD(card)) {
		int	err = -EINVAL;
		printk(KERN_INFO "%s: Configuring 56K onboard CSU/DSU\n",
			card->devname);

		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			printk (KERN_INFO "%s: Failed 56K configuration!\n",
				card->devname);
			return 0;
		}
		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}

	
	/* Record the new IP addreses */
	ppp_priv_area->ip_local = ppp_priv_area->ip_local_tmp;
	ppp_priv_area->ip_remote = ppp_priv_area->ip_remote_tmp;

	if (config508(dev, card)){
		printk(KERN_INFO "%s: Failed to configure PPP device\n",
			card->devname);
		return 0;
	}

	if (ppp_set_intr_mode(card, PPP_INTR_RXRDY|
			    		PPP_INTR_TXRDY|
				    	PPP_INTR_MODEM|
				    	PPP_INTR_DISC |
				    	PPP_INTR_OPEN |
				    	PPP_INTR_DROP_DTR |
					PPP_INTR_TIMER)) {

		printk(KERN_INFO "%s: Failed to configure board interrupts !\n", 
			card->devname);
		return 0;
	}

        /* Turn off the transmit and timer interrupt */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, (PPP_INTR_RXRDY|
			    		PPP_INTR_TXRDY|
				    	PPP_INTR_MODEM|
				    	PPP_INTR_DISC |
				    	PPP_INTR_OPEN |
				    	PPP_INTR_DROP_DTR |
					PPP_INTR_TIMER));

	/* If you are not the authenticator and any one of the protocol is 
	 * enabled then we call the set_out_bound_authentication.
	 */
	if ( !card->u.p.authenticator  && (ppp_priv_area->pap || ppp_priv_area->chap)) {
		if ( ppp_set_outbnd_auth(card, ppp_priv_area) ){
			printk(KERN_INFO "%s: Outbound authentication failed !\n",
				card->devname);
			return 0;
		}
	} 
	
	/* If you are the authenticator and any one of the protocol is enabled
	 * then we call the set_in_bound_authentication.
	 */
	if (card->u.p.authenticator && (ppp_priv_area->pap || ppp_priv_area->chap)){
		if (ppp_set_inbnd_auth(card, ppp_priv_area)){
			printk(KERN_INFO "%s: Inbound authentication failed !\n",
				card->devname);	
			return 0;
		}
	}


	/* Turn on all masked interrupts except for
	 * tx and timer interrupts */
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, 
			(PPP_INTR_RXRDY| PPP_INTR_MODEM|
			PPP_INTR_DISC | PPP_INTR_OPEN | PPP_INTR_DROP_DTR));

	/* If we fail to enable communications here it's OK,
	 * since the DTR timer will cause a disconnected, which
	 * will retrigger communication in timer_intr() */
	if (ppp_comm_enable(card) == CMD_OK) {
		ppp_priv_area->ppp_state = WAN_CONNECTING;
		wanpipe_set_state(card, WAN_CONNECTING);
		init_ppp_tx_rx_buff(card);
	}

	/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1);
	}

	/* Run WANPIPE debugging */
	WAN_DEBUG_START(card);
	return 0; 
}

/*============================================================
 * ppp_poll
 *	
 * Rationale:
 * 	We cannot manipulate the routing tables, or
 *      ip addresses withing the interrupt. Therefore
 *      we must perform such actons outside an interrupt 
 *      at a later time. 
 *
 * Description:	
 *	PPP polling routine, responsible for 
 *     	shutting down interfaces upon disconnect
 *     	and adding/removing routes. 
 *      
 * Usage:        
 * 	This function is executed for each ppp  
 * 	interface through a tq_schedule bottom half.
 *      
 *      trigger_ppp_poll() function is used to kick
 *      the ppp_poll routine.  
 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void ppp_poll (void *dev_ptr)
#else
static void ppp_poll (struct work_struct *work)
#endif
{
	sdla_t *card;
	u8 check_gateway=0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
	netdevice_t *dev;
        ppp_private_area_t *ppp_priv_area = container_of(work, ppp_private_area_t, poll_task);
	dev=ppp_priv_area->common.dev;
	if (!dev) {
		return;
	}	
#else
	netdevice_t *dev=dev_ptr;
	ppp_private_area_t *ppp_priv_area; 	
	if (!dev || (ppp_priv_area = wan_netif_priv(dev)) == NULL)
		return;
#endif

	card = ppp_priv_area->card;

	/* Shutdown is in progress, stop what you are 
	 * doing and get out */
	if (test_bit(PERI_CRIT,&card->wandev.critical)){
		clear_bit(POLL_CRIT,&card->wandev.critical);
		return;
	}

	/* if_open() function has triggered the polling routine
	 * to determine the configured IP addresses.  Once the
	 * addresses are found, trigger the chdlc configuration */
	if (test_bit(0,&ppp_priv_area->config_ppp)){

		ppp_priv_area->ip_local_tmp  = get_ip_address(dev,WAN_LOCAL_IP);
		ppp_priv_area->ip_remote_tmp = get_ip_address(dev,WAN_POINTOPOINT_IP);

		if (ppp_priv_area->ip_local_tmp == ppp_priv_area->ip_remote_tmp && 
	            card->u.p.ip_mode == WANOPT_PPP_HOST){
			
			if (++ppp_priv_area->ip_error > MAX_IP_ERRORS){
				printk(KERN_INFO "\n");
				printk(KERN_INFO "%s: --- WARNING ---\n",
						card->devname);
				printk(KERN_INFO "%s: The local IP address is the same as the\n",
						card->devname);
				printk(KERN_INFO "%s: Point-to-Point IP address.\n",
						card->devname);
				printk(KERN_INFO "%s: --- WARNING ---\n\n",
						card->devname);
			}else{
				clear_bit(POLL_CRIT,&card->wandev.critical);
				ppp_priv_area->poll_delay_timer.expires = jiffies+HZ;
				add_timer(&ppp_priv_area->poll_delay_timer);
				return;
			}
		}

		ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_CONFIG;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
		ppp_priv_area->ip_error=0;	
		
		clear_bit(0,&ppp_priv_area->config_ppp);
		clear_bit(POLL_CRIT,&card->wandev.critical);
		return;
	}

	/* Dynamic interface implementation, as well as dynamic
	 * routing.  */
	
	switch (card->wandev.state) {
	
	case WAN_DISCONNECTED:

		/* If the dynamic interface configuration is on, and interface 
		 * is up, then bring down the netowrk interface */

		if (test_bit(DYN_OPT_ON,&ppp_priv_area->interface_down) &&
		    !test_bit(DEV_DOWN,&ppp_priv_area->interface_down)	&&	
		    dev->flags & IFF_UP){	

			printk(KERN_INFO "%s: Interface %s down.\n",
				card->devname, dev->name);
			change_dev_flags(dev, (dev->flags&~IFF_UP));
			set_bit(DEV_DOWN,&ppp_priv_area->interface_down);
		}else{
			/* We need to check if the local IP address is
               	   	 * zero. If it is, we shouldn't try to remove it.
                 	 * For some reason the kernel crashes badly if 
                 	 * we try to remove the route twice */

			if (dev->flags & IFF_UP && 
		    	    get_ip_address(dev,WAN_LOCAL_IP) &&
		    	    card->u.p.ip_mode == WANOPT_PPP_PEER){ 

				remove_route(card);
			}
		}
		break;

	case WAN_CONNECTED:
		
		/* In SMP machine this code can execute before the interface
		 * comes up.  In this case, we must make sure that we do not
		 * try to bring up the interface before dev_open() is finished */

		/* DEV_DOWN will be set only when we bring down the interface
		 * for the very first time. This way we know that it was us
		 * that brought the interface down */
		
		if (test_bit(DYN_OPT_ON,&ppp_priv_area->interface_down) &&
	            test_bit(DEV_DOWN,  &ppp_priv_area->interface_down) &&
 		    !(dev->flags & IFF_UP)){
			
			printk(KERN_INFO "%s: Interface %s up.\n",
				card->devname,dev->name);
			
			change_dev_flags(dev,(dev->flags|IFF_UP));
			clear_bit(DEV_DOWN,&ppp_priv_area->interface_down);
			check_gateway=1;
		}

		if ((card->u.p.ip_mode == WANOPT_PPP_PEER) && 
		    test_bit(1,&Read_connection_info)) { 
			
			process_route(card);
			clear_bit(1,&Read_connection_info);
			check_gateway=1;
		}

		if (ppp_priv_area->gateway && check_gateway)
			add_gateway(card,dev);

		break;
	}
	clear_bit(POLL_CRIT,&card->wandev.critical);
	return;
}

/*============================================================
 * trigger_ppp_poll
 *
 * Description:
 * 	Add a ppp_poll() task into a tq_scheduler bh handler
 *      for a specific interface.  This will kick
 *      the ppp_poll() routine at a later time. 
 *
 * Usage:
 * 	Interrupts use this to defer a taks to 
 *      a polling routine.
 *
 */	

static void trigger_ppp_poll (netdevice_t *dev)
{
	ppp_private_area_t *ppp_priv_area;

	if ((ppp_priv_area=wan_netif_priv(dev)) != NULL){
		
		sdla_t *card = ppp_priv_area->card;

		if (test_bit(PERI_CRIT,&card->wandev.critical)){
			return;
		}
		
		if (test_and_set_bit(POLL_CRIT,&card->wandev.critical)){
			return;
		}

		WAN_TASKQ_SCHEDULE((&ppp_priv_area->poll_task));
	}
	return;
}

static void ppp_poll_delay (unsigned long dev_ptr)
{
	netdevice_t *dev = (netdevice_t *)dev_ptr;
	trigger_ppp_poll(dev);
}

/*============================================================
 * detect_and_fix_tx_bug
 *
 * Description:
 *	On connect, if the board tx buffer ptr is not the same
 *      as the driver tx buffer ptr, we found a firmware bug.
 *      Report the bug to the above layer.  To fix the
 *      error restart communications again.
 *
 * Usage:
 *
 */	

static int detect_and_fix_tx_bug (sdla_t *card)
{
	if ((card->u.p.txbuf_base_off & 0xFFF) != (card->u.p.txbuf_next_off & 0xFFF)){
		NEX_PRINTK(KERN_INFO "Major Error, Fix the bug\n");
		return 1;
	}
	return 0;
}


static int set_adapter_config (sdla_t* card)
{
	wan_mbox_t * mb = &card->wan_mbox;
	ADAPTER_CONFIGURATION_STRUCT* cfg = (ADAPTER_CONFIGURATION_STRUCT*)mb->wan_data;
	int err;

	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &cfg->adapter_type); 
	cfg->adapter_config = 0x00; 
	cfg->operating_frequency = 00; 
	mb->wan_data_len = sizeof(ADAPTER_CONFIGURATION_STRUCT);
	mb->wan_command = SET_ADAPTER_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != 0) {
		ppp_error(card,err,mb);
	}
	return (err);
}


/*
 * ******************************************************************
 * Proc FS function 
 */
#define PROC_CFG_FRM	"%-15s| %-12s|\n"
#define PROC_STAT_FRM	"%-15s| %-12s| %-14s|\n"
static char ppp_config_hdr[] =
	"Interface name | Device name |\n";
static char ppp_status_hdr[] =
	"Interface name | Device name | Status        |\n";

static int ppp_get_config_info(void* priv, struct seq_file* m, int* stop_cnt) 
{
	ppp_private_area_t*	ppp_priv_area = priv;
	sdla_t*			card = NULL;

	if (ppp_priv_area == NULL)
		return m->count;
	card = ppp_priv_area->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", ppp_config_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_CFG_FRM, ppp_priv_area->if_name, card->devname);
	return m->count;
}

static int ppp_get_status_info(void* priv, struct seq_file* m, int* stop_cnt)
{
	ppp_private_area_t*	ppp_priv_area = priv;
	sdla_t*			card = NULL;

	if (ppp_priv_area == NULL)
		return m->count;
	card = ppp_priv_area->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", ppp_status_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_STAT_FRM, ppp_priv_area->if_name, card->devname, STATE_DECODE(card->wandev.state));
	return m->count;
}


#define PROC_DEV_PPP_S_FRM	"%-20s| %-14s|\n"
#define PROC_DEV_PPP_D_FRM	"%-20s| %-14d|\n"
#define PROC_DEV_SEPARATE	"=====================================\n"

static int ppp_set_dev_config(struct file *file, 
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

/*
 ******************************************************************************
 *			ppp_snmp_data()	
 *
 * Description: Save snmp request and parameters in private structure, enable
 *		TIMER interrupt, put current process in sleep.
 * Arguments:   
 * Returns:
 ******************************************************************************
 */
#define   PPPLINKSTATUSPHYSICALINDEX  			3
#define   PPPLINKSTATUSBADADDRESSES  			4
#define   PPPLINKSTATUSBADCONTROLS  			5
#define   PPPLINKSTATUSPACKETTOOLONGS  			6
#define   PPPLINKSTATUSBADFCSS  			7
#define   PPPLINKSTATUSLOCALMRU  			8
#define   PPPLINKSTATUSREMOTEMRU  			9
#define   PPPLINKSTATUSLOCALTOPEERACCMAP  		10
#define   PPPLINKSTATUSPEERTOLOCALACCMAP  		11
#define   PPPLINKSTATUSLOCALTOREMOTEPROTOCOLCOMPRESSION 12
#define   PPPLINKSTATUSREMOTETOLOCALPROTOCOLCOMPRESSION 13
#define   PPPLINKSTATUSLOCALTOREMOTEACCOMPRESSION  	14
#define   PPPLINKSTATUSREMOTETOLOCALACCOMPRESSION  	15
#define   PPPLINKSTATUSTRANSMITFCSSIZE  		16
#define   PPPLINKSTATUSRECEIVEFCSSIZE  			17
#define   PPPLINKCONFIGINITIALMRU  			20
#define   PPPLINKCONFIGRECEIVEACCMAP  			21
#define   PPPLINKCONFIGTRANSMITACCMAP  			22
#define   PPPLINKCONFIGMAGICNUMBER  			23
#define   PPPLINKCONFIGFCSSIZE  			24
#define   PPPLQRQUALITY         			27
#define   PPPLQRINGOODOCTETS    			28
#define   PPPLQRLOCALPERIOD     			29
#define   PPPLQRREMOTEPERIOD    			30
#define   PPPLQROUTLQRS         			31
#define   PPPLQRINLQRS          			32
#define   PPPLQRCONFIGPERIOD    			35
#define   PPPLQRCONFIGSTATUS    			36
#define   PPPLQREXTNSLASTRECEIVEDLQRPACKET  		39
#define   PPPSECURITYCONFIGLINK  			42
#define   PPPSECURITYCONFIGPREFERENCE  			43
#define   PPPSECURITYCONFIGPROTOCOL  			44
#define   PPPSECURITYCONFIGSTATUS  			45
#define   PPPSECURITYSECRETSLINK  			48
#define   PPPSECURITYSECRETSIDINDEX  			49
#define   PPPSECURITYSECRETSDIRECTION  			50
#define   PPPSECURITYSECRETSPROTOCOL  			51
#define   PPPSECURITYSECRETSIDENTITY  			52
#define   PPPSECURITYSECRETSSECRET  			53
#define   PPPSECURITYSECRETSSTATUS  			54
#define   PPPIPOPERSTATUS       			57
#define   PPPIPLOCALTOREMOTECOMPRESSIONPROTOCOL  	58
#define   PPPIPREMOTETOLOCALCOMPRESSIONPROTOCOL  	59
#define   PPPIPREMOTEMAXSLOTID  			60
#define   PPPIPLOCALMAXSLOTID   			61
#define   PPPIPCONFIGADMINSTATUS  			64
#define   PPPIPCONFIGCOMPRESSION  			65
#define   PPPBRIDGEOPERSTATUS   			68
#define   PPPBRIDGELOCALTOREMOTETINYGRAMCOMPRESSION  	69
#define   PPPBRIDGEREMOTETOLOCALTINYGRAMCOMPRESSION  	70
#define   PPPBRIDGELOCALTOREMOTELANID  			71
#define   PPPBRIDGEREMOTETOLOCALLANID  			72
#define   PPPBRIDGECONFIGADMINSTATUS  			75
#define   PPPBRIDGECONFIGTINYGRAM  			76
#define   PPPBRIDGECONFIGRINGID  			77
#define   PPPBRIDGECONFIGLINEID  			78
#define   PPPBRIDGECONFIGLANID  			79
#define   PPPBRIDGEMEDIAMACTYPE  			82
#define   PPPBRIDGEMEDIALOCALSTATUS  			83
#define   PPPBRIDGEMEDIAREMOTESTATUS  			84
#define   PPPBRIDGEMEDIACONFIGMACTYPE  			87

//static int ppp_snmp_data(netdevice_t *dev, struct ifreq* ifr)
static int ppp_snmp_data(sdla_t* card, netdevice_t *dev, void* data)
{
	ppp_private_area_t* 	ppp_priv_area = NULL;
	wanpipe_snmp_t*		snmp;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return -EFAULT;
	ppp_priv_area = (ppp_private_area_t*)wan_netif_priv(dev);
	if (card->wandev.update) {
		int rslt = 0;
		rslt = card->wandev.update(&card->wandev);
		if(rslt) {
			return (rslt) ? (-EBUSY) : (-EINVAL);
		}
	}
	snmp = (wanpipe_snmp_t*)data;

	switch(snmp->snmp_magic){
	/* PPP Link Group */
	case PPPLINKSTATUSPHYSICALINDEX: 
	case PPPLINKSTATUSBADADDRESSES: 
	case PPPLINKSTATUSBADCONTROLS: 
            	return -EINVAL;

	case PPPLINKSTATUSPACKETTOOLONGS:
		snmp->snmp_val = card->wandev.stats.rx_length_errors;
		break;

	case PPPLINKSTATUSBADFCSS:
	case PPPLINKSTATUSLOCALMRU:
	case PPPLINKSTATUSREMOTEMRU:
	case PPPLINKSTATUSLOCALTOPEERACCMAP:
	case PPPLINKSTATUSPEERTOLOCALACCMAP:
	case PPPLINKSTATUSLOCALTOREMOTEPROTOCOLCOMPRESSION:
	case PPPLINKSTATUSREMOTETOLOCALPROTOCOLCOMPRESSION:
	case PPPLINKSTATUSLOCALTOREMOTEACCOMPRESSION:
	case PPPLINKSTATUSREMOTETOLOCALACCOMPRESSION:
	case PPPLINKSTATUSTRANSMITFCSSIZE:
	case PPPLINKSTATUSRECEIVEFCSSIZE:
	case PPPLINKCONFIGINITIALMRU:
	case PPPLINKCONFIGRECEIVEACCMAP:
	case PPPLINKCONFIGTRANSMITACCMAP:
	case PPPLINKCONFIGMAGICNUMBER:
	case PPPLINKCONFIGFCSSIZE:
	case PPPLQRQUALITY:
	case PPPLQRINGOODOCTETS:
	case PPPLQRLOCALPERIOD:
	case PPPLQRREMOTEPERIOD:
	case PPPLQROUTLQRS:
	case PPPLQRINLQRS:
	case PPPLQRCONFIGPERIOD:
	case PPPLQRCONFIGSTATUS:
	case PPPLQREXTNSLASTRECEIVEDLQRPACKET:
	case PPPSECURITYCONFIGLINK:
	case PPPSECURITYCONFIGPREFERENCE:
	case PPPSECURITYCONFIGPROTOCOL:
	case PPPSECURITYCONFIGSTATUS:
	case PPPSECURITYSECRETSLINK:
	case PPPSECURITYSECRETSIDINDEX:
	case PPPSECURITYSECRETSDIRECTION:
	case PPPSECURITYSECRETSPROTOCOL:
	case PPPSECURITYSECRETSIDENTITY:
	case PPPSECURITYSECRETSSECRET:
	case PPPSECURITYSECRETSSTATUS:
	case PPPIPOPERSTATUS:
	case PPPIPLOCALTOREMOTECOMPRESSIONPROTOCOL:
	case PPPIPREMOTETOLOCALCOMPRESSIONPROTOCOL:
	case PPPIPREMOTEMAXSLOTID:
	case PPPIPLOCALMAXSLOTID:
	case PPPIPCONFIGADMINSTATUS:
	case PPPIPCONFIGCOMPRESSION:
	case PPPBRIDGEOPERSTATUS:
	case PPPBRIDGELOCALTOREMOTETINYGRAMCOMPRESSION:
	case PPPBRIDGEREMOTETOLOCALTINYGRAMCOMPRESSION:
	case PPPBRIDGELOCALTOREMOTELANID:
	case PPPBRIDGEREMOTETOLOCALLANID:
	case PPPBRIDGECONFIGADMINSTATUS:
	case PPPBRIDGECONFIGTINYGRAM:
	case PPPBRIDGECONFIGRINGID:
	case PPPBRIDGECONFIGLINEID:
	case PPPBRIDGECONFIGLANID:
	case PPPBRIDGEMEDIAMACTYPE:
	case PPPBRIDGEMEDIALOCALSTATUS:
	case PPPBRIDGEMEDIAREMOTESTATUS:
	case PPPBRIDGEMEDIACONFIGMACTYPE:

	default:
            	return -EINVAL;
	}

	return 0;
}


#define PROC_IF_FR_S_FRM	"%-30s\t%-14s\n"
#define PROC_IF_FR_D_FRM	"%-30s\t%-14d\n"
#define PROC_IF_FR_L_FRM	"%-30s\t%-14ld\n"
#define PROC_IF_SEPARATE	"====================================================\n"

static int ppp_set_if_info(struct file *file, 
			   const char *buffer,
			   unsigned long count, 
			   void *data)
{
	netdevice_t*		dev = (void*)data;
	ppp_private_area_t* 	ppp_priv_area = NULL;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return count;
	ppp_priv_area = (ppp_private_area_t*)wan_netif_priv(dev);

	printk(KERN_INFO "%s: New interface config (%s)\n",
			ppp_priv_area->if_name, buffer);
	/* Parse string */

	return count;
}

static void ppp_handle_front_end_state(void* card_id)
{
	sdla_t			*card = (sdla_t*)card_id;
	netdevice_t		*dev;
        ppp_private_area_t	*ppp_priv_area;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	ppp_priv_area = wan_netif_priv(dev);

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	if (ppp_priv_area->ppp_state != WAN_CONNECTED){
		return;
	}
	
	if (card->fe.fe_status == FE_CONNECTED){

		if (ppp_priv_area->ppp_state == WAN_CONNECTED &&
		    card->wandev.state != WAN_CONNECTED){
		
			wanpipe_set_state(card, WAN_CONNECTED);

			ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_PPP_EVENT;
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
			trigger_ppp_poll(dev);
		}
	}else{
		wanpipe_set_state(card, WAN_DISCONNECTED);
		
		ppp_priv_area->timer_int_enabled |= TMR_INT_ENABLED_PPP_EVENT;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, PPP_INTR_TIMER);
		if (card->u.p.ip_mode == WANOPT_PPP_PEER) { 
			set_bit(0,&Read_connection_info);
		}
		trigger_ppp_poll(dev);
					
		/* Start debugging */
		WAN_DEBUG_START(card);
	}
}

/*
****************************************************************************
**
**
**
*/
static int ppp_debugging(sdla_t* card)
{
	ppp_pkt_stats_t*	pkt_stat = NULL;
	ppp_lcp_stats_t*	lcp_stat = NULL;
	wan_mbox_t*		mb = &card->wan_mbox;
	unsigned long 		smp_flags;
	static unsigned long 	last_rx_lcp = 0, last_rx_conf_nak = 0;
	unsigned long 		rx_lcp = 0, rx_conf_nak = 0;
	int			err = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!(err = ppp_get_pkt_stats(card))){
		pkt_stat = (ppp_pkt_stats_t*)mb->wan_data;
		rx_lcp = pkt_stat->rx_lcp;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	if (err)
		return 0;
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!(err = ppp_get_lcp_stats(card))){
		lcp_stat = (ppp_lcp_stats_t*)mb->wan_data;
		rx_conf_nak	= lcp_stat->rx_conf_nak;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	if (err)
		return 0;
	if (card->wan_debugging_state == WAN_DEBUGGING_CONT){
		last_rx_lcp	= rx_lcp;
		last_rx_conf_nak= rx_conf_nak;
		card->wan_debugging_state = WAN_DEBUGGING_PROTOCOL;
		return 15;
	}

	if (rx_lcp != last_rx_lcp){
		if (rx_conf_nak != last_rx_conf_nak){
			if (card->wan_debug_last_msg != WAN_DEBUG_PPP_LCP_MSG){
				DEBUG_EVENT("%s: PPP NAKs received!\n",
							card->devname);
				DEBUG_EVENT("%s:    Check remote router IP configuration!\n",
							card->devname);
				DEBUG_EVENT("%s:    If you cannot resolve this, call you dealer.\n",
						card->devname);	
			}
			card->wan_debug_last_msg = WAN_DEBUG_PPP_LCP_MSG;
		}else{
			if (card->wan_debug_last_msg != WAN_DEBUG_PPP_NAK_MSG){
				DEBUG_EVENT("%s: PPP negotiation problem!\n",
							card->devname);
				DEBUG_EVENT("%s:    Check remote router or ISP.\n",
							card->devname);	
			}
			card->wan_debug_last_msg = WAN_DEBUG_PPP_NAK_MSG;
		}
	}else{
		if (card->wan_debug_last_msg != WAN_DEBUG_PPP_NEG_MSG){
			DEBUG_EVENT("%s: No replies to LCP!\n",
						card->devname);
			DEBUG_EVENT("%s:    Check configuration of remote router.\n",
						card->devname);	
		}
		card->wan_debug_last_msg = WAN_DEBUG_PPP_NEG_MSG;
	}
	
	return 0;
}

static unsigned long ppp_crc_frames(sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	unsigned long 	smp_flags;
	unsigned long	rx_bad_crc = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!ppp_get_err_stats(card)){
		ppp_err_stats_t* stats = (ppp_err_stats_t*)mb->wan_data;
		rx_bad_crc = stats->rx_bad_crc;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return rx_bad_crc;
}

static unsigned long ppp_abort_frames(sdla_t * card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	unsigned long 	smp_flags;
	unsigned long	rx_abort = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!ppp_get_err_stats(card)){
		ppp_err_stats_t* stats = (ppp_err_stats_t*)mb->wan_data;
		rx_abort = stats->rx_abort;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return rx_abort;
}

static unsigned long ppp_tx_underun_frames(sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	unsigned long 	smp_flags;
	unsigned long	tx_underrun = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!ppp_get_err_stats(card)){
		ppp_err_stats_t* stats = (ppp_err_stats_t*)mb->wan_data;
		tx_underrun = stats->tx_missed_intr;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return tx_underrun;
}

/****** End *****************************************************************/
