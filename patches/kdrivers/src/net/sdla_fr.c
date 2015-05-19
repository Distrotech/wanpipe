/*****************************************************************************
* sdla_fr.c	WANPIPE(tm) Multiprotocol WAN Link Driver. Frame relay module.
*
* Author(s):	Nenad Corbic  <ncorbic@sangoma.com>
*		Gideon Hack
*
* Copyright:	(c) 1995-2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Apr 16, 2003  Nenad Corbic	o Frame Relay Bug fix in IP offsets, due
*                                 to header removal out of the data packet
* Jan 03, 2003  Nenad Corbic	o Memory leak bug fix under Bridge Mode.
* 				  If not an ethernet frame skb buffer was
* 				  not deallocated.
* Aug 30, 2002  Nenad Corbic	o Addes support for S5147 Dual TE1 card
* Dec 19, 2001  Nenad Corbic	o Full status request bug fix. Removed the
*				  fr_issue_isf() firmware man not be ready for
*				  full status response.
* Dec 04, 2001  Nenad Corbic	o Bridge bug fix: the skb mac ptr was being set to
* 				  the skb data ptr.  The ethernet header was being
* 				  lost.
* Dec 03, 2001  Gideon Hack	o Updated for S514-5 56K adatper
* Sep 20, 2001  Nenad Corbic    o The min() function has changed for 2.4.9
*                                 kernel. Thus using the wp_min() defined in
*                                 wanpipe.h
* Sept 6, 2001  Alex Feldman    o Add SNMP support.
* Aug 27, 2001  Nenad Corbic	o Added the IPX support and redesigned the
* 				  ARP handling code.
* May 25, 2001  Alex Feldman	o Added T1/E1 support (TE1).
* May 22, 2001  Nenad Corbic	o Fixed the incoming invalid ARP bug.
* Nov 23, 2000  Nenad Corbic    o Added support for 2.4.X kernels
* Nov 15, 2000  David Rokavarg  
*               Nenad Corbic	o Added frame relay bridging support.
* 				  Original code from Mark Wells and Kristian Hoffmann has
* 				  been integrated into the frame relay driver.
* Nov 13, 2000  Nenad Corbic    o Added true interface type encoding option.
* 				  Tcpdump doesn't support Frame Relay inteface
* 				  types, to fix this true type option will set
* 				  the interface type to RAW IP mode.
* Nov 07, 2000  Nenad Corbic	o Added security features for UDP debugging:
*                                 Deny all and specify allowed requests.
* Nov 06, 2000  Nenad Corbic	o Wanpipe interfaces conform to raw packet interfaces.  
*                                 Moved the if_header into the if_send() routine.
*                                 The if_header() was breaking the libpcap 
*                                 support. i.e. support for tcpdump, ethereal ...
* Oct 12. 2000  Nenad Corbic    o Added error message in fr_configure
* Jul 31, 2000  Nenad Corbic	o Fixed the Router UP Time.
* Apr 28, 2000  Nenad Corbic	o Added the option to shutdown an interface
*                                 when the channel gets disconnected.
* Apr 28, 2000  Nenad Corbic 	o Added M.Grants patch: disallow duplicate
*                                 interface setups. 
* Apr 25, 2000  Nenad Corbic	o Added M.Grants patch: dynamically add/remove 
*                                 new dlcis/interfaces.
* Mar 23, 2000  Nenad Corbic 	o Improved task queue, bh handling.
* Mar 16, 2000	Nenad Corbic	o Added Inverse ARP support
* Mar 13, 2000  Nenad Corbic	o Added new socket API support.
* Mar 06, 2000  Nenad Corbic	o Bug Fix: corrupted mbox recovery.
* Feb 24, 2000  Nenad Corbic    o Fixed up FT1 UDP debugging problem.
* Dev 15, 1999  Nenad Corbic    o Fixed up header files for 2.0.X kernels
*
* Nov 08, 1999  Nenad Corbic    o Combined all debug UDP calls into one function
*                               o Removed the ARP support. This has to be done
*                                 in the next version.
*                               o Only a Node can implement NO signalling.
*                                 Initialize DLCI during if_open() if NO 
*				  signalling.
*				o Took out IPX support, implement in next
*                                 version
* Sep 29, 1999  Nenad Corbic	o Added SMP support and changed the update
*                                 function to use timer interrupt.
*				o Fixed the CIR bug:  Set the value of BC
*                                 to CIR when the CIR is enabled.
*  				o Updated comments, statistics and tracing.
* Jun 02, 1999	Gideon Hack	o Updated for S514 support.
* Sep 18, 1998	Jaspreet Singh	o Updated for 2.2.X kernels.
* Jul 31, 1998	Jaspreet Singh	o Removed wpf_poll routine.  The channel/DLCI 
*				  status is received through an event interrupt.
* Jul 08, 1998	David Fong	o Added inverse ARP support.
* Mar 26, 1997	Jaspreet Singh	o Returning return codes for failed UDP cmds.
* Jan 28, 1997	Jaspreet Singh  o Improved handling of inactive DLCIs.
* Dec 30, 1997	Jaspreet Singh	o Replaced dev_tint() with mark_bh(NET_BH)
* Dec 16, 1997	Jaspreet Singh	o Implemented Multiple IPX support.
* Nov 26, 1997	Jaspreet Singh	o Improved load sharing with multiple boards
*				o Added Cli() to protect enabling of interrupts
*				  while polling is called.
* Nov 24, 1997	Jaspreet Singh	o Added counters to avoid enabling of interrupts
*				  when they have been disabled by another
*				  interface or routine (eg. wpf_poll).
* Nov 06, 1997	Jaspreet Singh	o Added INTR_TEST_MODE to avoid polling	
*				  routine disable interrupts during interrupt
*				  testing.
* Oct 20, 1997  Jaspreet Singh  o Added hooks in for Router UP time.
* Oct 16, 1997  Jaspreet Singh  o The critical flag is used to maintain flow
*                                 control by avoiding RACE conditions.  The
*                                 cli() and restore_flags() are taken out.
*                                 The fr_channel structure is appended for 
*                                 Driver Statistics.
* Oct 15, 1997  Farhan Thawar    o updated if_send() and receive for IPX
* Aug 29, 1997  Farhan Thawar    o Removed most of the cli() and sti()
*                                o Abstracted the UDP management stuff
*                                o Now use tbusy and critical more intelligently
* Jul 21, 1997  Jaspreet Singh	 o Can configure T391, T392, N391, N392 & N393
*				   through router.conf.
*				 o Protected calls to sdla_peek() by adDing 
*				   save_flags(), cli() and restore_flags().
*				 o Added error message for Inactive DLCIs in
*				   fr_event() and update_chan_state().
*				 o Fixed freeing up of buffers using kfree() 
*			           when packets are received.
* Jul 07, 1997	Jaspreet Singh	 o Added configurable TTL for UDP packets 
*				 o Added ability to discard multicast and 
*				   broadcast source addressed packets
* Jun 27, 1997	Jaspreet Singh	 o Added FT1 monitor capabilities 
*				   New case (0x44) statement in if_send routine 
*				   Added a global variable rCount to keep track
*			 	   of FT1 status enabled on the board.
* May 29, 1997	Jaspreet Singh	 o Fixed major Flow Control Problem
*				   With multiple boards a problem was seen where
*				   the second board always stopped transmitting
*				   packet after running for a while. The code
*				   got into a stage where the interrupts were
*				   disabled and dev->tbusy was set to 1.
*                  		   This caused the If_send() routine to get into
*                                  the if clause for it(0,dev->tbusy) 
*				   forever.
*				   The code got into this stage due to an 
*				   interrupt occuring within the if clause for 
*				   set_bit(0,dev->tbusy).  Since an interrupt 
*				   disables furhter transmit interrupt and 
* 				   makes dev->tbusy = 0, this effect was undone 
*                                  by making dev->tbusy = 1 in the if clause.
*				   The Fix checks to see if Transmit interrupts
*				   are disabled then do not make dev->tbusy = 1
* 	   			   Introduced a global variable: int_occur and
*				   added tx_int_enabled in the wan_device 
*				   structure.	
* May 21, 1997  Jaspreet Singh   o Fixed UDP Management for multiple
*                                  boards.
*
* Apr 25, 1997  Farhan Thawar    o added UDP Management stuff
*                                o fixed bug in if_send() and tx_intr() to
*                                  sleep and wakeup all devices
* Mar 11, 1997  Farhan Thawar   Version 3.1.1
*                                o fixed (+1) bug in fr508_rx_intr()
*                                o changed if_send() to return 0 if
*                                  wandev.critical() is true
*                                o free socket buffer in if_send() if
*                                  returning 0 
*                                o added tx_intr() routine
* Jan 30, 1997	Gene Kozin	Version 3.1.0
*				 o implemented exec() entry point
*				 o fixed a bug causing driver configured as
*				   a FR switch to be stuck in WAN_
*				   mode
* Jan 02, 1997	Gene Kozin	Initial version.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/if_wanpipe.h>	
#include <linux/if_wanpipe_common.h>	/* Wanpipe Socket */
#include <linux/sdla_front_end.h>
#include <linux/wanpipe_snmp.h>
#include <linux/wanproc.h>
#include <linux/sdla_fr.h>		/* frame relay firmware API definitions */

/****** Defines & Macros ****************************************************/

#define	MAX_CMD_RETRY	10		/* max number of firmware retries */

#define	FR_HEADER_LEN	8		/* max encapsulation header size */
#define	FR_CHANNEL_MTU	1500		/* unfragmented logical channel MTU */

/* Private critical flags */
enum { 
	SEND_TXIRQ_CRIT = PRIV_CRIT,
	ARP_CRIT
};

/* Q.922 frame types */
#define	Q922_UI		0x03		/* Unnumbered Info frame */
#define	Q922_XID	0xAF		

/* CISCO data frame encapsulation */
#define CISCO_UI	0x08
#define CISCO_IP	0x00
#define CISCO_INV	0x20

/* DLCI configured or not */
#define DLCI_NOT_CONFIGURED	0x00
#define DLCI_CONFIG_PENDING	0x01
#define DLCI_CONFIGURED		0x02

/* CIR enabled or not */
#define CIR_ENABLED	0x00
#define CIR_DISABLED	0x01

#define FRAME_RELAY_API 1
#define MAX_BH_BUFF	10

/* For handle_IPXWAN() */
#define CVHexToAscii(b) (((unsigned char)(b) > (unsigned char)9) ? ((unsigned char)'A' + ((unsigned char)(b) - (unsigned char)10)) : ((unsigned char)'0' + (unsigned char)(b)))
 
/****** Data Structures *****************************************************/

/* This is an extention of the 'struct device' we create for each network
 * interface to keep the rest of channel-specific data.
 */
typedef struct fr_channel
{
	wanpipe_common_t common;
	char name[WAN_IFNAME_SZ+1];	/* interface name, ASCIIZ */
	unsigned dlci_configured  ;	/* check whether configured or not */
	unsigned cir_status;		/* check whether CIR enabled or not */
	unsigned dlci;			/* logical channel number */
	unsigned cir;			/* committed information rate */
	unsigned bc;			/* committed burst size */
	unsigned be;			/* excess burst size */
	unsigned mc;			/* multicast support on or off */
	unsigned tx_int_status;		/* Transmit Interrupt Status */	
	unsigned short pkt_length;	/* Packet Length */
	unsigned long router_start_time;/* Router start time in seconds */
	unsigned long tick_counter;	/* counter for transmit time out */
	char dev_pending_devtint;	/* interface pending dev_tint() */
	unsigned long dlci_int_interface_off;	/* pointer to the DLCI Interface */ 
	unsigned long dlci_int_off;	/* offset to the DLCI Interface interrupt */ 
	unsigned long IB_addr;		/* physical address of Interface Byte */
	unsigned long state_tick;	/* time of the last state change */
	unsigned char enable_IPX;	/* Enable/Disable the use of IPX */
	unsigned long network_number;	/* Internal Network Number for IPX*/
	sdla_t *card;			/* -> owner */
	unsigned route_flag;		/* Add/Rem dest addr in route tables */
	unsigned inarp;			/* Inverse Arp Request status */ 
	unsigned long inarp_ready;	/* Ready to send requests */
	unsigned char inarp_rx;		/* InArp reception support */
	int inarp_interval;		/* Time between InArp Requests */
	unsigned long inarp_tick;	/* InArp jiffies tick counter */
	unsigned long interface_down;	/* Bring interface down on disconnect */
	struct net_device_stats ifstats;	/* interface statistics */
	if_send_stat_t drvstats_if_send;
        rx_intr_stat_t drvstats_rx_intr;
        pipe_mgmt_stat_t drvstats_gen;
	unsigned long router_up_time;

	unsigned short transmit_length;
	struct sk_buff *delay_skb;

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	wan_taskq_t fr_poll_task;
	struct timer_list fr_arp_timer;

	u32 ip_local;
	u32 ip_remote;
	unsigned long  config_dlci;
	u32 unconfig_dlci;

	/* Whether this interface should be setup as a gateway.
	 * Used by dynamic route setup code */
	u8  gateway;

	/* True interface type */
	u8 true_if_encoding;
	u8 fr_header[FR_HEADER_LEN];
	int fr_header_len;

	/* Encapsulation varialbes, default
	 * is IETF, however, if we detect CISCO
	 * use its encapsultaion */
	u8 fr_encap_0;
	u8 fr_encap_1;

	/* Pending ARP packet to be transmitted */
	struct sk_buff *tx_arp;
	struct sk_buff *tx_ipx;

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;
	unsigned long 		router_last_change;
	unsigned int		dlci_type;
	unsigned long		rx_DE_set;
	unsigned long		tx_DE_set;
	unsigned long		rx_FECN;
	unsigned long		rx_BECN;
	unsigned 		err_type;
	char			err_data[SNMP_FR_ERRDATA_LEN];
	unsigned long 		err_time;
	unsigned long 		err_faults;
	unsigned		trap_state;
	unsigned long		trap_max_rate;

	/* DLCI firmware status */
	unsigned char dlci_stat;
} fr_channel_t;

/* Route Flag options */
#define NO_ROUTE	0x00
#define ADD_ROUTE 	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE 	0x03
#define ARP_REQ		0x04

/* inarp options */
#define INARP_NONE		0x00
#define INARP_REQUEST		0x01
#define INARP_CONFIGURED	0x02

/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   		0x01
#define TMR_INT_ENABLED_UPDATE 		0x02
#define TMR_INT_ENABLED_ARP		0x04
#define TMR_INT_ENABLED_UPDATE_STATE 	0x08
#define TMR_INT_ENABLED_UPDATE_DLCI	0x40
#define TMR_INT_ENABLED_TE		0x80

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

#pragma pack(1)

typedef struct dlci_status
{
	unsigned short dlci	;
	unsigned char state	;
} dlci_status_t;

typedef struct dlci_IB_mapping
{
	unsigned short dlci		;
	unsigned long  addr_value	;
} dlci_IB_mapping_t;

/* This structure is used for DLCI list Tx interrupt mode.  It is used to
   enable interrupt bit and set the packet length for transmission
 */
typedef struct fr_dlci_interface 
{
	unsigned char gen_interrupt	;
	unsigned short packet_length	;
	unsigned char reserved		;
} fr_dlci_interface_t; 

#pragma pack()

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

/****** Function Prototypes *************************************************/

/* WAN link driver entry points. These are called by the WAN router module. */
static int update(wan_device_t *wandev);
static int new_if(wan_device_t *wandev, netdevice_t *dev, wanif_conf_t *conf);
static int del_if(wan_device_t *wandev, netdevice_t *dev);
static void disable_comm (sdla_t *card);

/* Network device interface */
static int if_init(netdevice_t *dev);
static int if_open(netdevice_t *dev);
static int if_close(netdevice_t *dev);
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);

static void if_tx_timeout (netdevice_t *dev);

static int if_send(struct sk_buff *skb, netdevice_t *dev);
static int chk_bcast_mcast_addr(sdla_t *card, netdevice_t* dev,
                                struct sk_buff *skb);
static struct net_device_stats *if_stats(netdevice_t *dev);


/* Interrupt handlers */
static WAN_IRQ_RETVAL fr_isr(sdla_t *card);
static void rx_intr(sdla_t *card);
static void tx_intr(sdla_t *card);
static void timer_intr(sdla_t *card);
static void spur_intr(sdla_t *card);

/* Frame relay firmware interface functions */
static int fr_read_version(sdla_t *card, char *str);
static int fr_configure(sdla_t *card, fr_conf_t *conf);
static int fr_dlci_configure(sdla_t *card, fr_dlc_conf_t *conf, unsigned dlci);
static int fr_init_dlci (sdla_t *card, fr_channel_t *chan);
static int fr_set_intr_mode (sdla_t *card, unsigned mode, unsigned mtu, unsigned short timeout);
static int fr_comm_enable(sdla_t *card);
static void fr_comm_disable(sdla_t *card);
static int fr_get_err_stats(sdla_t *card);
static int fr_get_stats(sdla_t *card, fr_channel_t* chan);
static int fr_add_dlci(sdla_t *card, int dlci);
static int fr_activate_dlci(sdla_t *card, int dlci);
static int fr_delete_dlci (sdla_t* card, int dlci);
static int fr_send(sdla_t *card, int dlci, unsigned char attr, int len,
	void *buf);
static int fr_send_data_header(sdla_t *card, int dlci, unsigned char attr, int len,
	void *buf,unsigned char hdr_len, char lock);
static unsigned int fr_send_hdr(sdla_t *card, int dlci, unsigned int offset);
static int fr_issue_isf (sdla_t* card, int isf);

static int check_dlci_config (sdla_t *card, fr_channel_t *chan);
static void initialize_rx_tx_buffers (sdla_t *card);


/* Firmware asynchronous event handlers */
static int fr_event(sdla_t *card, int event, wan_mbox_t *mb);
static int fr_modem_failure(sdla_t *card, wan_mbox_t *mb);
static int fr_dlci_change(sdla_t *card, wan_mbox_t *mb, int mbox_offset);

/* Miscellaneous functions */
static int update_chan_state(netdevice_t *dev);
static void set_chan_state(netdevice_t *dev, int state);
static netdevice_t *find_channel(sdla_t *card, unsigned dlci);
static int is_tx_ready(sdla_t *card, fr_channel_t *chan);
static unsigned int dec_to_uint(unsigned char *str, int len);
static int reply_udp( unsigned char *data, unsigned int mbox_len );

static int intr_test( sdla_t* card );
static void init_chan_statistics( fr_channel_t* chan );
static void init_global_statistics( sdla_t* card );
static void read_DLCI_IB_mapping( sdla_t* card, fr_channel_t* chan );
static int setup_for_delayed_transmit(netdevice_t* dev, struct sk_buff *skb);

netdevice_t * move_dev_to_next (sdla_t *, netdevice_t *);
static int check_tx_status(sdla_t *, netdevice_t *);

/* Frame Relay Socket API */
static void fr_bh (unsigned long data);

static void trigger_fr_poll (netdevice_t *);

# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void fr_poll (void *card_ptr);
#else
static void fr_poll (struct work_struct *work);
#endif

static void unconfig_fr_dev (sdla_t *card, netdevice_t *dev);


static void config_fr_dev (sdla_t *card,netdevice_t *dev);
static int set_adapter_config (sdla_t* card);


/* Inverse ARP and Dynamic routing functions */
int process_ARP(arphdr_1490_t *ArpPacket, sdla_t *card, netdevice_t *dev);
int is_arp(void *buf);
int send_inarp_request(sdla_t *card, netdevice_t *dev);

static void trigger_fr_arp (netdevice_t *);
static void fr_arp (unsigned long data);


/* Udp management functions */
static int process_udp_mgmt_pkt(sdla_t *card, void *local_dev);
static int udp_pkt_type( struct sk_buff *skb, sdla_t *card, int);
static int store_udp_mgmt_pkt(int udp_type, char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, int dlci);

/* IPX functions */
static void switch_net_numbers(unsigned char *sendpacket,
	unsigned long network_number, unsigned char incoming);

static int handle_IPXWAN(unsigned char *sendpacket, char *devname,
	unsigned char enable_IPX, unsigned long network_number);

/* Lock Functions: SMP supported */
void 	s508_s514_unlock(sdla_t *card, unsigned long *smp_flags);
void 	s508_s514_lock(sdla_t *card, unsigned long *smp_flags);
void s508_s514_send_event_lock(sdla_t *card, unsigned long *smp_flags);
void s508_s514_send_event_unlock(sdla_t *card, unsigned long *smp_flags);



static unsigned short calc_checksum (char *, int);
static int setup_fr_header(struct sk_buff** skb, netdevice_t* dev, char op_mode);

static int fr_get_config_info(void* priv, struct seq_file* m, int*);
static int fr_get_status_info(void* priv, struct seq_file* m, int*);
static int fr_set_dev_config(struct file*, const char*, unsigned long, void *);
static int fr_set_if_info(struct file*, const char*, unsigned long, void *);

static void fr_enable_timer(void* card_id);
static void fr_handle_front_end_state (void* card_id);

/* SNMP */
static int fr_snmp_data(sdla_t* card, netdevice_t *dev, void* data);

/* Debugging */
static int fr_debugging(sdla_t* card);
static unsigned long fr_crc_frames(sdla_t* card);
static unsigned long fr_abort_frames(sdla_t * card);
static unsigned long fr_tx_underun_frames(sdla_t* card);

/****** Public Functions ****************************************************/

/*============================================================================
 * wpf_init - Frame relay protocol initialization routine.
 *
 * @card:	Wanpipe card pointer
 * @conf:	User hardware/firmware/general protocol configuration
 *              pointer. 
 *
 * This routine is called by the main WANPIPE module 
 * during setup: ROUTER_SETUP ioctl().  
 *
 *
 * At this point adapter is completely initialized and 
 * firmware is running.
 *  o read firmware version (to make sure it's alive)
 *  o configure adapter
 *  o initialize protocol-specific fields of the adapter data space.
 *
 * Return:	0	o.k.
 *		< 0	failure.
 */
int wpf_init(sdla_t *card, wandev_conf_t *conf)
{

	int err;
	fr_buf_info_t	buf_info;
	unsigned long	buf_info_off;

	union
	{
		char str[80];
		fr_conf_t cfg;
	} u;

	int i;


	printk(KERN_INFO "\n");

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_FR) {
		
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
			card->devname, conf->config_id);
		return -EINVAL;
	
	}

	/* Initialize protocol-specific fields of adapter data space */
	/* Alex Apr 8 2004 Sangoma ISA card */
	card->mbox_off  = FR_MB_VECTOR + FR508_MBOX_OFFS; 
	card->flags_off = FR_MB_VECTOR + FR508_FLAG_OFFS;

        card->isr = &fr_isr;

    	card->intr_type_off = 
			card->flags_off +
			offsetof(fr508_flags_t, iflag);
	card->intr_perm_off = 
			card->flags_off +
			offsetof(fr508_flags_t, imask);

	card->wandev.ignore_front_end_status = conf->ignore_front_end_status;

	//ALEX_TODAY err=check_conf_hw_mismatch(card,conf->te_cfg.media);
	err = (card->hw_iface.check_mismatch) ? 
			card->hw_iface.check_mismatch(card->hw,conf->fe_cfg.media) : -EINVAL;
	if (err){
		return err;
	}
	
	/* Determine the board type user is trying to configure
	 * and make sure that the HW matches */
	if (IS_TE1_MEDIA(&conf->fe_cfg)) {

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
		card->wandev.fe_enable_timer = fr_enable_timer;
		card->wandev.te_link_state = fr_handle_front_end_state;
		conf->electrical_interface = 
			(IS_T1_FEMEDIA(&card->fe)) ? WANOPT_V35 : WANOPT_RS232;
		conf->clocking = WANOPT_EXTERNAL;
		
        }else if (IS_56K_MEDIA(&conf->fe_cfg)) {
                
		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
	}else{

		card->fe.fe_status = FE_CONNECTED;

		/* Current frame relay firmware doesn't support
		 * this for S508 and S514 1-2-3 cards */
		card->wandev.ignore_front_end_status = WANOPT_YES;
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

	if (fr_read_version(card, NULL) || fr_read_version(card, u.str)){
		return -EIO;
	}

	printk(KERN_INFO "%s: running frame relay firmware v%s\n",
		card->devname, u.str);


	if (set_adapter_config(card)) {
		return -EIO;
	}


	/* Adjust configuration */
	conf->mtu += FR_HEADER_LEN;
	conf->mtu = (conf->mtu >= MIN_LGTH_FR_DATA_CFG) ?
			wp_min(conf->mtu, FR_MAX_NO_DATA_BYTES_IN_FRAME) :
                        FR_CHANNEL_MTU + FR_HEADER_LEN;
     
	conf->bps = wp_min(conf->bps, 2048000);

	/* Initialze the configuration structure sent to the board to zero */
	memset(&u.cfg, 0, sizeof(u.cfg));

	memset(card->u.f.dlci_to_dev_map, 0, sizeof(card->u.f.dlci_to_dev_map));
 	
	/* Configure adapter firmware */

	u.cfg.mtu	= conf->mtu;
	u.cfg.kbps	= conf->bps / 1000;

    	u.cfg.cir_fwd = u.cfg.cir_bwd = 16;
        u.cfg.bc_fwd  = u.cfg.bc_bwd = 16;
	
	u.cfg.options	= 0x0000;
	printk(KERN_INFO "%s: Global CIR enabled by Default\n", card->devname);
	
	switch (conf->u.fr.signalling) {

		case WANOPT_FR_ANSI:
			u.cfg.options = 0x0000; 
			break;		
	
		case WANOPT_FR_Q933:	
			u.cfg.options |= 0x0200; 
			break;
	
		case WANOPT_FR_LMI:	
			u.cfg.options |= 0x0400; 
			break;

		case WANOPT_NO:
			u.cfg.options |= 0x0800; 
			break;
		default:
			printk(KERN_INFO "%s: Illegal Signalling option\n",
					card->wandev.name);
			return -EINVAL;
	}


	card->wandev.signalling = conf->u.fr.signalling;

	if (conf->u.fr.station == WANOPT_CPE) {


		if (conf->u.fr.signalling == WANOPT_NO){
			printk(KERN_INFO 
				"%s: ERROR - For NO signalling, station must be set to Node!",
				 	 card->devname);
			return -EINVAL;
		}

		u.cfg.station = 0;
		u.cfg.options |= 0x8000;	/* auto config DLCI */
		card->u.f.dlci_num  = 0;
	
	} else {

		u.cfg.station = 1;	/* switch emulation mode */

		/* For switch emulation we have to create a list of dlci(s)
		 * that will be sent to be global SET_DLCI_CONFIGURATION 
		 * command in fr_configure() routine. 
		 */

		card->u.f.dlci_num  = wp_min(wp_max(conf->u.fr.dlci_num, 1), 100);
	
		for ( i = 0; i < card->u.f.dlci_num; i++) {

			card->u.f.node_dlci[i] = (unsigned short) 
				conf->u.fr.dlci[i] ? conf->u.fr.dlci[i] : 16;
	
		}
	}

	if (conf->clocking == WANOPT_INTERNAL)
		u.cfg.port |= 0x0001;

	if (conf->electrical_interface == WANOPT_RS232)
		u.cfg.port |= 0x0002;

	if (conf->u.fr.t391)
		u.cfg.t391 = wp_min(conf->u.fr.t391, 30);
	else
		u.cfg.t391 = 5;
	card->u.f.t391 = u.cfg.t391;

	if (conf->u.fr.t392)
		u.cfg.t392 = wp_min(conf->u.fr.t392, 30);
	else
		u.cfg.t392 = 15;
	card->u.f.t392 = u.cfg.t392;

	if (conf->u.fr.n391)
		u.cfg.n391 = wp_min(conf->u.fr.n391, 255);
	else
		u.cfg.n391 = 2;
	card->u.f.n391 = u.cfg.n391;

	if (conf->u.fr.n392)
		u.cfg.n392 = wp_min(conf->u.fr.n392, 10);
	else
		u.cfg.n392 = 3;	
	card->u.f.n392 = u.cfg.n392;

	if (conf->u.fr.n393)
		u.cfg.n393 = wp_min(conf->u.fr.n393, 10);
	else
		u.cfg.n393 = 4;
	card->u.f.n393 = u.cfg.n393;

	if (fr_configure(card, &u.cfg))
		return -EIO;

	/* Alex Apr 8 2004 Sangoma ISA card (the same code as for PCI) */
	buf_info_off = FR_MB_VECTOR + FR508_RXBC_OFFS;
	card->hw_iface.peek(card->hw, buf_info_off, (unsigned char*)&buf_info, sizeof(buf_info));

        card->rxmb_off = buf_info.rse_next;
        card->u.f.rxmb_base_off = buf_info.rse_base; 
	card->u.f.rxmb_last_off =
        		buf_info.rse_base + 
			(buf_info.rse_num - 1) * sizeof(fr_rx_buf_ctl_t);

	card->u.f.rx_base_off = buf_info.buf_base;
	card->u.f.rx_top_off  = buf_info.buf_top;
	atomic_set(&card->u.f.tx_interrupts_pending,0);

	card->wandev.mtu	= conf->mtu;
	card->wandev.bps	= conf->bps;
	card->wandev.electrical_interface	= conf->electrical_interface;
	card->wandev.clocking	= conf->clocking;
	card->wandev.station	= conf->u.fr.station;
	card->poll		= NULL; 
	card->exec		= NULL;
	card->wandev.update	= &update;
	card->wandev.new_if	= &new_if;
	card->wandev.del_if	= &del_if;
	
	// Proc fs functions
	card->wandev.get_config_info 	= &fr_get_config_info;
	card->wandev.get_status_info 	= &fr_get_status_info;
	card->wandev.set_dev_config    	= &fr_set_dev_config;
	card->wandev.set_if_info     	= &fr_set_if_info;

	/* Debugging */
	card->wan_debugging	= &fr_debugging;
	card->get_crc_frames	= &fr_crc_frames;
	card->get_abort_frames	= &fr_abort_frames;
	card->get_tx_underun_frames	= &fr_tx_underun_frames;

    	card->wandev.state	= WAN_DISCONNECTED;
	card->wandev.ttl	= conf->ttl;
        card->wandev.udp_port 	= conf->udp_port;       
	card->disable_comm	= &disable_comm;	
	card->u.f.arp_dev 	= NULL;

	/* SNMP data */
	card->get_snmp_data	= &fr_snmp_data;

	/* Intialize global statistics for a card */
	init_global_statistics( card );

        card->TracingEnabled          = 0;
        
	/* Interrupt Test */
	card->timer_int_enabled = 0;
	card->intr_mode = INTR_TEST_MODE;
	err = intr_test(card);

	printk(KERN_INFO "%s: End of Interrupt Test rc=0x%x  count=%i\n",
			card->devname,err,card->timer_int_enabled); 
	
	if (err || (card->timer_int_enabled < MAX_INTR_TEST_COUNTER)) {
		printk(KERN_INFO "%s: Interrupt Test Failed, Counter: %i\n", 
			card->devname, card->timer_int_enabled);
		printk(KERN_INFO "%s: Please choose another interrupt\n",
				card->devname);
		return -EIO;
	}

	printk(KERN_INFO "%s: Interrupt Test Passed, Counter: %i\n",
			card->devname, card->timer_int_enabled);

	card->u.f.issue_fs_on_startup = conf->u.fr.issue_fs_on_startup;
	if (card->u.f.issue_fs_on_startup){
		printk(KERN_INFO "%s: Configured for full status on startup\n",
				card->devname);
	}else{
		printk(KERN_INFO "%s: Disabled full status on startup\n",
				card->devname);
	}

	/* Configure the front end.
	 * If the onboard CSU/DSU exists, configure via
	 * user defined options */
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
			return -EIO;
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
			return -EIO;
		}
		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}
	

	/* Apr 28 2000. Nenad Corbic
	 * Enable commnunications here, not in if_open or new_if, since
         * interfaces come down when the link is disconnected. 
         */
	 
	/* If you enable comms and then set ints, you get a Tx int as you
	 * perform the SET_INT_TRIGGERS command. So, we only set int
	 * triggers and then adjust the interrupt mask (to disable Tx ints)
	 * before enabling comms. 
	 */	
        if (fr_set_intr_mode(card, (FR_INTR_RXRDY | FR_INTR_TXRDY |
		FR_INTR_DLC | FR_INTR_TIMER | FR_INTR_TX_MULT_DLCIs | FR_INTR_MODEM) ,
		card->wandev.mtu, 0)) {
		return -EIO;
	}

	/* Mask - Disable all interrupts */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, 
			(FR_INTR_RXRDY | FR_INTR_TXRDY | FR_INTR_DLC | 
			 FR_INTR_TIMER | FR_INTR_TX_MULT_DLCIs | FR_INTR_MODEM));


	if (fr_comm_enable(card)) {
		return -EIO;
	}	
	wanpipe_set_state(card, WAN_CONNECTED);
	spin_lock_init(&card->u.f.if_send_lock);


	/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1);
	}

	/* Unmask - Enable all interrupts */
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, 
			(FR_INTR_RXRDY | FR_INTR_TXRDY | FR_INTR_DLC | 
			 FR_INTR_TIMER | FR_INTR_TX_MULT_DLCIs | FR_INTR_MODEM));
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	atomic_set(&card->wandev.if_cnt,0);
	atomic_set(&card->wandev.if_up_cnt,0);

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
 *      3) Frame Relay operational statistics on the adapter.
 * 
 * The board level statistics are read during a timer interrupt. 
 * Note that we read the error and operational statistics 
 * during consecitive timer ticks so as to minimize the time 
 * that we are inside the interrupt handler.
 *
 */
static int update (wan_device_t* wandev)
{
	sdla_t* card;
	unsigned long smp_flags;

	/* sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;

	if (wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	card = wandev->priv;
	
	if (test_and_set_bit(0,&card->update_comms_stats)){
		return -EBUSY;	
	}

	spin_lock_irqsave(&card->wandev.lock, smp_flags);

	fr_get_err_stats(card);
	fr_get_stats(card, card->u.f.update_dlci);

	if (card->u.f.update_dlci == NULL){
		if (IS_TE1_CARD(card)) {
			/* TE1 Update T1/E1 alarms */
			card->wandev.fe_iface.read_alarm(&card->fe, 0); 
			/* TE1 Update T1/E1 perfomance counters */
			card->wandev.fe_iface.read_pmon(&card->fe, 0); 
		}else if (IS_56K_CARD(card)) {
			/* 56K Update CSU/DSU alarms */
			card->wandev.fe_iface.read_alarm(&card->fe, 1); 
		}	
	}

	card->u.f.update_dlci = NULL;
	card->update_comms_stats = 0;

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
 * private structure for Frame Relay protocol and bind it 
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
	fr_channel_t* chan;
	int dlci = 0;
	int err = 0;
	
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		
		printk(KERN_INFO "%s: Error, Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	if (atomic_read(&wandev->if_cnt) > 0 && card->u.f.auto_dlci_cfg){
		printk(KERN_INFO 
			   "%s:%s: Error, DLCI autoconfig only allowed on a single DLCI\n",
			    card->devname,conf->name);
		printk(KERN_INFO "%s: Please turn off DLCI autoconfig in %s.conf\n",
				card->devname,card->devname);
		printk(KERN_INFO "\n");
		return -EINVAL;
	}

	/* allocate and initialize private data */
	chan = wan_malloc(sizeof(fr_channel_t));
	if (chan == NULL){
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}

	memset(chan, 0, sizeof(fr_channel_t));
	strcpy(chan->name, conf->name);
	chan->card = card;

	WAN_TASKLET_INIT((&chan->common.bh_task),0,fr_bh,(unsigned long)chan);
	chan->common.state = WAN_CONNECTING;

	/* verify media address */
	if (strcmp(conf->addr,"auto")){
		
		if (is_digit(conf->addr[0])) {

			dlci = dec_to_uint(conf->addr, 0);

			if (dlci && (dlci <= HIGHEST_VALID_DLCI)) {
			
				chan->dlci = dlci;
				chan->common.lcn = dlci;
			
			} else {
			
				printk(KERN_INFO
					"%s: Error, Invalid DLCI %u on interface %s!\n",
					wandev->name, dlci, chan->name);
				err = -EINVAL;
				goto new_if_error;
			}

		} else {
			printk(KERN_INFO
				"%s: Error, Invalid media address on interface %s!\n",
				wandev->name, chan->name);
			err = -EINVAL;
			goto new_if_error;
		}

	}else{

		if (card->wandev.station == WANOPT_NODE){
			printk(KERN_INFO "%s: Error, DLCI autoconfig only allowed for CPE!\n",
					card->devname);
			printk(KERN_INFO "%s: Please turn off DLCI autoconfig in %s.conf\n",
				card->devname,card->devname);

			err = -EINVAL;
			goto new_if_error;
		}
		
		if (atomic_read(&wandev->if_cnt) != 0){
			printk(KERN_INFO 
			   "%s:%s: Error, DLCI autoconfig only allowed on a single DLCI\n",
			    card->devname,chan->name);
			printk(KERN_INFO "%s: Please turn off DLCI autoconfig in %s.conf\n",
				card->devname,card->devname);

			err = -EINVAL;
			goto new_if_error;
		}

		card->u.f.auto_dlci_cfg=1;

		dlci=0xFFFF;
		chan->dlci=0xFFFF;
	}

	if (find_channel (card, chan->dlci) != NULL){
		printk(KERN_INFO "%s: Warning: Interface %s already bound to dlci %i\n",
				card->devname, chan->name, dlci);
		err = -EEXIST;
		goto new_if_error;
	}

	if (card->u.f.auto_dlci_cfg){
		printk(KERN_INFO "%s: Interface %s configured for AUTO DLCI\n",
			card->devname,conf->name);
	}else{
		printk(KERN_INFO "%s: Interface %s bound to DLCI %i\n",
			card->devname,conf->name,dlci);
	}

	if ((chan->true_if_encoding = conf->true_if_encoding) == WANOPT_YES){
		printk(KERN_INFO 
			"%s:%s : Enabling, true interface type encoding.\n",
			card->devname,chan->name);
	}


        /* Setup wanpipe as a router (WANPIPE) even if it is
	 * a bridged DLCI, or as an API 
	 */
        if (strcmp(conf->usedby, "WANPIPE")  == 0  || 
	    strcmp(conf->usedby, "BRIDGE")   == 0  ||
	    strcmp(conf->usedby, "BRIDGE_N") == 0){
		
		if(strcmp(conf->usedby, "WANPIPE") == 0){
			chan->common.usedby = WANPIPE;
			
	                printk(KERN_INFO "%s:%s: Running in WANPIPE mode.\n", 
					card->devname,chan->name);
			
		}else if(strcmp(conf->usedby, "BRIDGE") == 0){
			
			chan->common.usedby = BRIDGE;
			
			printk(KERN_INFO "%s:%s: Running in WANPIPE (BRIDGE) mode.\n", 
					card->devname,chan->name);
			
		}else if(strcmp(conf->usedby, "BRIDGE_N") == 0 ){
			
			chan->common.usedby = BRIDGE_NODE;
		
			printk(KERN_INFO "%s:%s: Running in WANPIPE (BRIDGE_NODE) mode.\n", 
					card->devname,chan->name);
		}

		if (!err){
			/* Dynamic interface configuration option.
			 * On disconnect, if the options is selected,
			 * the interface will be brought down */
			if (conf->if_down == WANOPT_YES){ 
				set_bit(DYN_OPT_ON,&chan->interface_down);
				printk(KERN_INFO 
				    "%s:%s: Dynamic interface configuration enabled.\n",
					card->devname,chan->name);
			}
		}

        } else if(strcmp(conf->usedby, "API") == 0){

                chan->common.usedby = API;
                printk(KERN_INFO "%s:%s: Running in API mode.\n",
			wandev->name,chan->name);
		wan_reg_api(chan, dev, card->devname);
        }

	if (err) {
		goto new_if_error;
	}

	/* place cir,be,bc and other channel specific information into the
	 * chan structure 
         */
	if (conf->cir) {

		chan->cir = wp_max( 1, wp_min( conf->cir, 512 ) );
		chan->cir_status = CIR_ENABLED; 

		
		/* If CIR is enabled, force BC to equal CIR
                 * this solves number of potential problems if CIR is 
                 * set and BC is not 
		 */
		chan->bc = chan->cir;

		if (conf->be){
			chan->be = wp_max( 0, wp_min( conf->be, 511) ); 
		}else{	
			conf->be = 0;
		}

		printk (KERN_INFO "%s:%s:: CIR enabled\n",
				wandev->name,chan->name);

		printk (KERN_INFO "%s:     CIR = %i ; BC = %i ; BE = %i\n",
				wandev->name,chan->cir,chan->bc,chan->be);

	}else{
		chan->cir_status = CIR_DISABLED;
		printk (KERN_INFO "%s:%s: CIR disabled\n",
				wandev->name,chan->name);
	}

	chan->mc = conf->mc;

	/* Split up between INARP TX and RX.  If the TX ARP is
	 * enabled the RX arp will be enabled by default. 
	 * Otherwise the user has a choice to ignore incoming
	 * arps if the TX is turned off */
	
	chan->inarp_rx = conf->inarp_rx;
	
	if (conf->inarp == WANOPT_YES){
		printk(KERN_INFO "%s:%s: Inv. ARP Support Enabled\n",card->devname,chan->name);
		chan->inarp = conf->inarp ? INARP_REQUEST : INARP_NONE;
		chan->inarp_interval = conf->inarp_interval ? conf->inarp_interval : 10;
		chan->inarp_rx = WANOPT_YES;
	}else{
		printk(KERN_INFO "%s:%s: Inv. ARP Support Disabled\n",card->devname,chan->name);
		chan->inarp = INARP_NONE;
		chan->inarp_interval = 10;
	}

	if (chan->inarp_rx == WANOPT_YES){
		printk(KERN_INFO "%s:%s: Inv. ARP Reception Enabled!\n",card->devname,chan->name);
	}else{
		printk(KERN_INFO "%s:%s: Inv. ARP Reception Disabled!\n",card->devname,chan->name);
	}


	chan->dlci_configured = DLCI_NOT_CONFIGURED;	


	/*FIXME: IPX disabled in this WANPIPE version */
	if (conf->enable_IPX == WANOPT_YES){
		chan->enable_IPX = WANOPT_YES;
		printk(KERN_INFO "%s:%s: IPX Support Enabled!\n",card->devname,chan->name);
	}else{
		printk(KERN_INFO "%s:%s: IPX Support Disabled!\n",card->devname,chan->name);
		chan->enable_IPX = WANOPT_NO;
	}	

	if (conf->network_number && chan->enable_IPX == WANOPT_YES){
		printk(KERN_INFO "%s:%s: IPX Network Address 0x%X!\n",
				card->devname,chan->name,conf->network_number);
		chan->network_number = conf->network_number;
	}else{
		chan->network_number = 0xDEADBEEF;
	}

	chan->route_flag = NO_ROUTE;
	
	init_chan_statistics(chan);

	chan->transmit_length = 0;

	/* Initialize FR Polling Task Queue
         * We need a poll routine for each network
         * interface. 
         */

	WAN_TASKQ_INIT((&chan->fr_poll_task),0,fr_poll,dev);

	init_timer(&chan->fr_arp_timer);
	chan->fr_arp_timer.data=(unsigned long)dev;
	chan->fr_arp_timer.function = fr_arp;

	/* Tells us that if this interface is a
         * gateway or not */
	if ((chan->gateway = conf->gateway) == WANOPT_YES){
		printk(KERN_INFO "%s: Interface %s is set as a gateway.\n",
			card->devname,dev->name);
	}

	/* Initialize the encapsulation header to
	 * the IETF standard.
	 */
	chan->fr_encap_0 = Q922_UI;
	chan->fr_encap_1 = NLPID_IP;

	/*
	 * Create interface file in proc fs.
	 * Once the proc file system is created, the new_if() function
	 * should exit successfuly.  
	 *
	 * DO NOT place code under this function that can return 
	 * anything else but 0.
	 */
	err = wanrouter_proc_add_interface(wandev, &chan->dent, chan->name, dev);
	if (err){	
		printk(KERN_INFO
			"%s: can't create /proc/net/router/fr/%s entry!\n",
			card->devname, chan->name);
		goto new_if_error;
	}
	chan->dlci_type = SNMP_FR_STATIC;
	chan->trap_state = SNMP_FR_DISABLED;
	chan->trap_max_rate = 0;

	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);
	wan_netif_set_priv(dev,chan);
	chan->common.dev = dev;

	/* Configure this dlci at a later date, when
         * the interface comes up. i.e. when if_open() 
         * executes */
	set_bit(0,&chan->config_dlci);

	atomic_inc(&wandev->if_cnt);
	printk(KERN_INFO "\n");
	return 0;

new_if_error:

	WAN_TASKLET_KILL(&chan->common.bh_task);
	wan_unreg_api(chan, card->devname);

	DEBUG_SUB_MEM(sizeof(fr_channel_t));
	wan_free(chan);

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
	fr_channel_t* chan = wan_netif_priv(dev);
	unsigned long smp_flags;
	sdla_t *card=wandev->priv;

	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, chan->name);

	/* This interface is dead, make sure the 
	 * ARP timer is stopped */
	del_timer(&chan->fr_arp_timer);

	WAN_TASKLET_KILL(&chan->common.bh_task);
	wan_unreg_api(chan, card->devname);
	
	/* Unconfigure this DLCI, free any pending skb 
	 * buffers and deallocate the bh_head circular
	 * buffer. 
	 */
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	
	unconfig_fr_dev (card,dev);
	
	if (card->u.f.arp_dev == dev){
		card->u.f.arp_dev=NULL;
	}
	if (chan->delay_skb){
		wan_skb_free(chan->delay_skb);
		chan->delay_skb=NULL;	
	}
	if (chan->tx_arp){
		wan_skb_free(chan->tx_arp);
		chan->tx_arp=NULL;
	}
	if (chan->tx_ipx){
		wan_skb_free(chan->tx_ipx);
		chan->tx_ipx=NULL;
	}

	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);	

	DEBUG_SUB_MEM(sizeof(fr_channel_t));
	atomic_dec(&wandev->if_cnt);
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

	printk(KERN_INFO "%s: Disabling Communications!\n",
			card->devname);
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	fr_comm_disable(card);
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
	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	wan_device_t* wandev = &card->wandev;

	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);

#if defined(LINUX_2_4) || defined(LINUX_2_6)
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);

	dev->watchdog_timeo	= TX_TIMEOUT;
#endif
	
	if (chan->common.usedby == WANPIPE || chan->common.usedby == API){
		
		/* Initialize media-specific parameters */
		if (chan->true_if_encoding){
			dev->type 		= ARPHRD_DLCI;  /* This breaks tcpdump */
		}else{
			dev->type		= ARPHRD_PPP; 	/* ARP h/w type */
		}
		
		dev->flags		|= IFF_POINTOPOINT;
		dev->flags		|= IFF_NOARP;

		/* Enable Multicast addressing */
		if (chan->mc == WANOPT_YES){
			dev->flags 	|= IFF_MULTICAST;
		}

		dev->mtu		= wandev->mtu - FR_HEADER_LEN;
		/* For an API, the maximum number of bytes that the stack will pass
		   to the driver is (dev->mtu + dev->hard_header_len). So, adjust the
		   mtu so that a frame of maximum size can be transmitted by the API. 
		*/
		if(chan->common.usedby == API) {
			dev->mtu += (sizeof(api_tx_hdr_t) - FR_HEADER_LEN);
		}
		
		dev->hard_header_len	= 0;            /* media header length */
		dev->addr_len		= 2; 		/* hardware address length */
		*(unsigned short*)dev->dev_addr = chan->dlci;

		/* Set transmit buffer queue length */
        	dev->tx_queue_len = 100;

	}else{

		/* Setup the interface for Bridging */
		int hw_addr=0;
		ether_setup(dev);
		
		/* Use a random number to generate the MAC address */
		memcpy(dev->dev_addr, "\xFE\xFC\x00\x00\x00\x00", 6);
		get_random_bytes(&hw_addr, sizeof(hw_addr));
		*(int *)(dev->dev_addr + 2) += hw_addr;
	}
		
	/* Initialize hardware parameters (just for reference) */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

	/* SNMP */
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);

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
 * Return 0 if O.k. or errno.
 */

static int if_open (netdevice_t* dev)
{
	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	int err = 0;
	struct timeval tv;
	unsigned long smp_flags;

	if (open_dev_check(dev)){
		return -EBUSY;
	}
	
	netif_start_queue(dev);

	wanpipe_open(card);
	do_gettimeofday( &tv );
	chan->router_start_time = tv.tv_sec;
	
	atomic_inc(&card->wandev.if_up_cnt);
	
	if (test_bit(0,&chan->config_dlci)){
		netdevice_t	*dev1;
		
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		config_fr_dev(card,dev);
		clear_bit(0,&chan->config_dlci);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

		dev1 = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev1 == dev && card->wandev.station == WANOPT_CPE){
			printk(KERN_INFO 
			 	"%s: Waiting for DLCI report from frame relay switch...\n\n",
					card->devname);
		}
	}else if (chan->inarp == INARP_REQUEST){
		trigger_fr_arp(dev);
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
	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;

	if (chan->inarp == INARP_CONFIGURED) {
		chan->inarp = INARP_REQUEST;
	}

	stop_net_queue(dev);
#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);

	atomic_dec(&card->wandev.if_up_cnt);
	return 0;
}

/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	chan->drvstats_if_send.if_send_tbusy++;
	++chan->ifstats.collisions;

	printk (KERN_INFO "%s: Transmit timed out on %s\n", 
			card->devname, dev->name);

	atomic_inc(&card->u.f.tx_interrupts_pending);
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TXRDY);

#if defined(LINUX_2_4) || defined(LINUX_2_6)
	dev->trans_start = jiffies;
#endif

	chan->drvstats_if_send.if_send_tbusy_timeout++;
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
 * o set critical flag when accessing board.
 * o check link state. If link is not up, then drop the packet.
 * o check channel status. If it's down then initiate a call.
 * o pass a packet to corresponding WAN device.
 * o free socket buffer
 *
 * Return:	0	complete (socket buffer must be freed)
 *		non-0	packet may be re-transmitted (tbusy must be set)
 *
 * Notes:
 * 1. This routine is called either by the protocol stack or by the "net
 *    bottom half" (with interrupts enabled).
 * 
 * 2. Using the start_net_queue() and stop_net_queue() MACROS
 *    will inhibit further transmit requests from the protocol stack 
 *    and can be used for flow control with protocol layer.
 */
static int if_send (struct sk_buff* skb, netdevice_t* dev)
{
    	fr_channel_t* chan = wan_netif_priv(dev);
    	sdla_t* card = chan->card;
    	unsigned char *sendpacket;
        int err;
	int udp_type;
	unsigned long delay_tx_queued=0;
	unsigned long smp_flags=0;
	unsigned char attr = 0;

	chan->drvstats_if_send.if_send_entry++;
	
        if (skb == NULL) {             
		/* if we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		printk(KERN_INFO "%s: interface %s got kicked!\n", 
			card->devname, dev->name);
		chan->drvstats_if_send.if_send_skb_null ++;

		wake_net_dev(dev);
		return 0;
	}

	DEBUG_ADD_MEM(skb->truesize);

	/* If a peripheral task is running just drop packets */
	if (test_bit(PERI_CRIT, &card->wandev.critical)){
		
		printk(KERN_INFO "%s: Critical in if_send(): Peripheral running!\n",
				card->devname);
		
		wan_skb_free(skb);
		start_net_queue(dev);
		return 0;
	}

	/* We must set the 'tbusy' flag if we already have a packet queued for
	   transmission in the transmit interrupt handler. However, we must
	   ensure that the transmit interrupt does not reset the 'tbusy' flag
	   just before we set it, as this will result in a "transmit timeout".
	*/

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	set_bit(SEND_TXIRQ_CRIT, (void*)&card->wandev.critical);
        if(chan->transmit_length || chan->tx_ipx || chan->tx_arp) {
		if (card->wandev.state != WAN_CONNECTED ||
		    chan->common.state != WAN_CONNECTED){
			wan_skb_free(skb);
			start_net_queue(dev);	
			chan->drvstats_if_send.if_send_dlci_disconnected ++;
        		++chan->ifstats.tx_dropped;
        		++card->wandev.stats.tx_dropped;
			++chan->ifstats.tx_carrier_errors;
        		++card->wandev.stats.tx_carrier_errors;
			err=0;
		}else{
			DEBUG_SUB_MEM(skb->truesize);
			stop_net_queue(dev);
			chan->tick_counter = jiffies;
			err=1;
		}
 		clear_bit(SEND_TXIRQ_CRIT, (void*)&card->wandev.critical);
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		return err;
	}
       	clear_bit(SEND_TXIRQ_CRIT, (void*)&card->wandev.critical);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	

#if defined(LINUX_2_1)
    	if (dev->tbusy) {

		chan->drvstats_if_send.if_send_tbusy++;
		++chan->ifstats.collisions;

		if ((jiffies - chan->tick_counter) < (5 * HZ)) {
			return 1;
		}

		if_tx_timeout(dev);
    	}
#endif
	
	/* Move the if_header() code to here. By inserting frame
	 * relay header in if_header() we would break the
	 * tcpdump and other packet sniffers */
	chan->fr_header_len = setup_fr_header(&skb,dev,chan->common.usedby);
	if (chan->fr_header_len < 0 ){
		++chan->ifstats.tx_dropped;
		++card->wandev.stats.tx_dropped;
		++chan->ifstats.tx_errors;
		++card->wandev.stats.tx_errors;
		
		wan_skb_free(skb);
		start_net_queue(dev);	
		return 0;
	}

	sendpacket = skb->data;

	udp_type = udp_pkt_type(skb, card, UDP_PKT_FRM_STACK);

        if(udp_type != UDP_INVALID_TYPE) {
		if(store_udp_mgmt_pkt(udp_type, UDP_PKT_FRM_STACK, card, skb,
                        chan->dlci)) {
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TIMER);
                        if (udp_type == UDP_FPIPE_TYPE){
                                chan->drvstats_if_send.
					if_send_PIPE_request ++;
			}
                }
		start_net_queue(dev);
		return 0;
	}

#if 1
	//FIXME: can we do better than sendpacket[2]?
  	if ((chan->common.usedby == WANPIPE) && (sendpacket[0] == 0x45)) {
               	/* check to see if the source IP address is a broadcast or */
                /* multicast IP address */
                if(chk_bcast_mcast_addr(card, dev, skb)){
            		++chan->ifstats.tx_dropped;
			++card->wandev.stats.tx_dropped;
                	wan_skb_free(skb);
			start_net_queue(dev);
			return 0;
		}
	}
#endif
	
	/* Lock the S514/S508 card: SMP Supported */
    	s508_s514_lock(card,&smp_flags);

	if (test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
		
		chan->drvstats_if_send.if_send_critical_non_ISR ++;
		chan->ifstats.tx_errors ++;
		printk(KERN_INFO "%s Critical in IF_SEND: if_send() already running!\n", 
				card->devname);
		goto if_send_start_and_exit;
	}
	
	/* API packet check: minimum packet size must be greater than 
	 * 16 byte API header */
	if((chan->common.usedby == API) && (skb->len <= sizeof(api_tx_hdr_t))) {
		++chan->ifstats.tx_errors;
		++card->wandev.stats.tx_errors;
		goto if_send_start_and_exit;

 	}else{
		/* During API transmission, get rid of the API header */
		if (chan->common.usedby == API) {
			api_tx_hdr_t* api_tx_hdr;
			api_tx_hdr = (api_tx_hdr_t*)&skb->data[0x00];
			attr = api_tx_hdr->wp_api_tx_hdr_fr_attr;
			skb_pull(skb,sizeof(api_tx_hdr_t));
		}
	}

	if (card->wandev.state != WAN_CONNECTED) {
		chan->drvstats_if_send.if_send_wan_disconnected ++;
		++chan->ifstats.tx_dropped;
        	++card->wandev.stats.tx_dropped;
		++chan->ifstats.tx_carrier_errors;
        	++card->wandev.stats.tx_carrier_errors;
	
	} else if (chan->common.state != WAN_CONNECTED) {
		chan->drvstats_if_send.if_send_dlci_disconnected ++;

		/* Update the DLCI state in timer interrupt */
		card->u.f.timer_int_enabled |= TMR_INT_ENABLED_UPDATE_STATE;	
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TIMER);

        	++chan->ifstats.tx_dropped;
        	++card->wandev.stats.tx_dropped;
		++chan->ifstats.tx_carrier_errors;
        	++card->wandev.stats.tx_carrier_errors;
		
		
	} else if (!is_tx_ready(card, chan)) {
		/* No tx buffers available, store for delayed transmit */
		if (!setup_for_delayed_transmit(dev, skb)){
			set_bit(1,&delay_tx_queued);
		}
		chan->drvstats_if_send.if_send_no_bfrs++;
		
	} else if (!skb->protocol) {
		/* No protocols drop packet */
		printk(KERN_INFO "%s: %s: if_send() pkt no protocol: dropping!\n",
				card->devname,dev->name);
		chan->drvstats_if_send.if_send_protocol_error ++;
		++card->wandev.stats.tx_errors;
	
	} else if (test_bit(ARP_CRIT,&card->wandev.critical)){
		/* We are trying to send an ARP Packet, block IP data until
		 * ARP is sent */
		if (net_ratelimit()){
			printk(KERN_INFO "%s: %s: FR Data Tx waiting for ARP to Tx!\n",
				card->devname,dev->name);
		}
		++chan->ifstats.tx_dropped;
        	++card->wandev.stats.tx_dropped;
		
	} else {
		if(chan->common.usedby == WANPIPE &&
		   htons(skb->protocol) == ETH_P_IPX){ 
			
			if( chan->enable_IPX ) {
				switch_net_numbers(sendpacket, 
						chan->network_number, 0);
			} else {
				printk(KERN_INFO 
				"%s: WARNING: Unsupported IPX data in send, packet dropped\n",
					card->devname);
		
				++chan->ifstats.tx_dropped;
		        	++card->wandev.stats.tx_dropped;

				goto if_send_start_and_exit;
			}
			
		}

		err = fr_send_data_header(card, 
					  chan->dlci, 
					  attr, 
					  skb->len, 
					  skb->data, 
					  chan->fr_header_len,
					  1);
		if (err) {
			switch(err) {
			case FRRES_CIR_OVERFLOW:
			case FRRES_BUFFER_OVERFLOW:
				if (!setup_for_delayed_transmit(dev, skb)){
					set_bit(1,&delay_tx_queued);
				}
				chan->drvstats_if_send.
					if_send_adptr_bfrs_full ++;
				break;
				
			case FRRES_TOO_LONG:
				if (net_ratelimit()){
					printk(KERN_INFO 
					"%s: Error: Frame too long, transmission failed %i\n",
					 card->devname, (unsigned int)skb->len);
				}

				/* Drop down to default */
			default:
				chan->drvstats_if_send.
					if_send_dlci_disconnected ++;
				++chan->ifstats.tx_dropped;
				++card->wandev.stats.tx_dropped;
				++chan->ifstats.tx_carrier_errors;
				++card->wandev.stats.tx_carrier_errors;
				break;
			}

		}else{
			chan->drvstats_if_send.
				if_send_bfr_passed_to_adptr++;
			++chan->ifstats.tx_packets;
			++card->wandev.stats.tx_packets;
			
			chan->ifstats.tx_bytes += skb->len;
			card->wandev.stats.tx_bytes += skb->len;
#if defined(LINUX_2_4) || defined(LINUX_2_6)
			dev->trans_start = jiffies;
#endif
		}
	}

if_send_start_and_exit:

	start_net_queue(dev);
	
	/* If we queued the packet for transmission, we must not
	 * deallocate it. The packet is unlinked from the IP stack
	 * not copied. Therefore, we must keep the original packet */
	if (!test_bit(1,&delay_tx_queued)) {
                wan_skb_free(skb);
	}else{
		atomic_inc(&card->u.f.tx_interrupts_pending);
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TXRDY);
	}

        clear_bit(SEND_CRIT, (void*)&card->wandev.critical);

	s508_s514_unlock(card,&smp_flags);

	return 0;
}


/*=========================================================================
 * if_do_ioctl - Ioctl handler for fr
 *
 * @dev: Device subject to ioctl
 * @ifr: Interface request block from the user
 * @cmd: Command that is being issued
 *	
 * This function handles the ioctls that may be issued by the user
 * to control the settings of a FR. It does both busy
 * and security checks. This function is intended to be wrapped by
 * callers who wish to add additional ioctl calls of their own.
 *
 */
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	unsigned long smp_flags;
	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	int err=0;
	
	if (!chan)
		return -ENODEV;

	card = chan->card;

	NET_ADMIN_CHECK();

	switch(cmd)
	{

		case SIOC_WANPIPE_BIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}
			
			DEBUG_TEST("%s: SIOC_WANPIPE_BIND_SK \n",__FUNCTION__);

			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
			break;

		case SIOC_WANPIPE_UNBIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}

			DEBUG_TEST("%s: SIOC_WANPIPE_UNBIND_SK \n",__FUNCTION__);
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

		case SIOC_WANPIPE_SNMP:
		case SIOC_WANPIPE_SNMP_IFSPEED:
			return wan_snmp_data(card, dev, cmd, ifr);
			
		case SIOC_WANPIPE_PIPEMON: 
			
			if (atomic_read(&card->u.f.udp_pkt_len) != 0){
				return -EBUSY;
			}
	
			atomic_set(&card->u.f.udp_pkt_len,sizeof(wan_udp_hdr_t));
		
			wan_udp_pkt=(wan_udp_pkt_t*)&card->u.f.udp_pkt_data;
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				atomic_set(&card->u.f.udp_pkt_len,0);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)){
				printk(KERN_INFO "%s:%s pipemon command busy: try again!\n",
						card->devname,dev->name);
				atomic_set(&card->u.f.udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EBUSY;
			}
			
			if (process_udp_mgmt_pkt(card,dev) <= 0 ){
				atomic_set(&card->u.f.udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EINVAL;
			}

			
			spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */
			
			if (atomic_read(&card->u.f.udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				printk(KERN_INFO "%s: Error: Pipemon buf too bit on the way up! %i\n",
						card->devname,atomic_read(&card->u.f.udp_pkt_len));
				atomic_set(&card->u.f.udp_pkt_len,0);
				return -EINVAL;
			}

			if (copy_to_user(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				atomic_set(&card->u.f.udp_pkt_len,0);
				return -EFAULT;
			}
			
			atomic_set(&card->u.f.udp_pkt_len,0);
			return 0;
		
		default:
			return -EOPNOTSUPP;
	}
	return err;
}


/*============================================================================
 * Setup so that a frame can be transmitted on the occurence of a transmit
 * interrupt.
 */
static int setup_for_delayed_transmit (netdevice_t* dev, struct sk_buff *skb)
{
        fr_channel_t* chan = wan_netif_priv(dev);
        sdla_t* card = chan->card;
        fr_dlci_interface_t	dlci_interface;
	int len = skb->len;

	/* Check that the dlci is properly configured,
         * before using tx interrupt */
	if (!chan->dlci_int_interface_off){
		if (net_ratelimit()){ 
			printk(KERN_INFO 
				"%s: Error on DLCI %i: Not configured properly !\n",
					card->devname, chan->dlci);
			printk(KERN_INFO "%s: Please contact Sangoma Technologies\n",
					card->devname);
		}
		return 1;
	}
		
	card->hw_iface.peek(card->hw, chan->dlci_int_interface_off, &dlci_interface, sizeof(dlci_interface));

        if(chan->transmit_length) {
                printk(KERN_INFO "%s: Tx failed to queue packet: queue busy\n",
				card->devname);
                return 1;
        }

	if(len > FR_MAX_NO_DATA_BYTES_IN_FRAME) {
		printk(KERN_INFO "%s: Tx failed to queue packet: packet size %i > Max=%i\n",
				card->devname,len,FR_MAX_NO_DATA_BYTES_IN_FRAME);
		return 1;
	}

	wan_skb_unlink(skb);
	
        chan->transmit_length = len;
	chan->delay_skb = skb;
        
        dlci_interface.gen_interrupt |= FR_INTR_TXRDY;
        dlci_interface.packet_length = len;
	card->hw_iface.poke(card->hw, chan->dlci_int_interface_off, &dlci_interface, sizeof(dlci_interface));
	/* Turn on TX interrupt at the end of if_send */
	return 0;
}


/*============================================================================
 * Check to see if the packet to be transmitted contains a broadcast or
 * multicast source IP address.
 * Return 0 if not broadcast/multicast address, otherwise return 1.
 */

static int chk_bcast_mcast_addr(sdla_t *card, netdevice_t* dev,
                                struct sk_buff *skb)
{
        u32 src_ip_addr;
        u32 broadcast_ip_addr = 0;
        struct in_device *in_dev;
        fr_channel_t* chan = wan_netif_priv(dev);
	iphdr_t *iphdr = (iphdr_t *)skb->data;

	if (skb->len < sizeof(iphdr_t)){
		printk(KERN_INFO
                        "%s: Illegal IP frame len %i, silently discarded\n",
                        card->devname,skb->len);
		return 1;
	}

        /* read the IP source address from the outgoing packet */
        src_ip_addr = iphdr->w_ip_src;

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
                printk(KERN_INFO
                        "%s: Broadcast Source Address silently discarded\n",
                        card->devname);
                return 1;
        }

        /* check if the IP Source Address is a Multicast address */
        if((chan->mc == WANOPT_NO) && (ntohl(src_ip_addr) >= 0xE0000001) &&
                (ntohl(src_ip_addr) <= 0xFFFFFFFE)) {
                printk(KERN_INFO
                        "%s: Multicast Source Address silently discarded\n",
                        card->devname);
                return 1;
        }

        return 0;
}

/*============================================================================
 * Reply to UDP Management system.
 * Return nothing.
 */
static int reply_udp( unsigned char *data, unsigned int mbox_len ) 
{
	unsigned short len, udp_length, temp, ip_length;
	unsigned long ip_temp;
	int even_bound = 0;
	wan_udp_pkt_t *wan_udp_pkt = (wan_udp_pkt_t *)data; 

	/* Set length of packet */
	len = //sizeof(fr_encap_hdr_t)+
	      sizeof(struct iphdr)+ 
	      sizeof(struct udphdr)+
	      sizeof(wan_mgmt_t)+
	      sizeof(wan_cmd_t)+
	      mbox_len;
 

	/* fill in UDP reply */
	wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
  
	/* fill in UDP length */
	udp_length = sizeof(struct udphdr)+ 
		     sizeof(wan_mgmt_t)+
	      	     sizeof(wan_cmd_t)+
		     mbox_len; 


	/* put it on an even boundary */
	if ( udp_length & 0x0001 ) {
		udp_length += 1;
		len += 1;
		even_bound = 1;
	}

	temp = (udp_length<<8)|(udp_length>>8);
	wan_udp_pkt->wan_udp_len = temp;
	 
	/* swap UDP ports */
	temp = wan_udp_pkt->wan_udp_sport;
	wan_udp_pkt->wan_udp_sport = 
			wan_udp_pkt->wan_udp_dport; 
	wan_udp_pkt->wan_udp_dport = temp;



	/* add UDP pseudo header */
	temp = 0x1100;
	*((unsigned short *)
		(wan_udp_pkt->wan_udp_data+mbox_len+even_bound)) = temp;	
	temp = (udp_length<<8)|(udp_length>>8);
	*((unsigned short *)
		(wan_udp_pkt->wan_udp_data+mbox_len+even_bound+2)) = temp;
		 
	/* calculate UDP checksum */
	wan_udp_pkt->wan_udp_sum = 0;

	wan_udp_pkt->wan_udp_sum = 
		calc_checksum(&data[UDP_OFFSET/*+sizeof(fr_encap_hdr_t)*/],
			      udp_length+UDP_OFFSET);

	/* fill in IP length */
	ip_length = udp_length + sizeof(struct iphdr);
	temp = (ip_length<<8)|(ip_length>>8);
	wan_udp_pkt->wan_ip_len = temp;
  
	/* swap IP addresses */
	ip_temp = wan_udp_pkt->wan_ip_src;
	wan_udp_pkt->wan_ip_src = 
				wan_udp_pkt->wan_ip_dst;
	wan_udp_pkt->wan_ip_dst = ip_temp;

		 
	/* fill in IP checksum */
	wan_udp_pkt->wan_ip_sum = 0;
	wan_udp_pkt->wan_ip_sum = 
		calc_checksum(&data[/*sizeof(fr_encap_hdr_t)*/0],
		      	      sizeof(struct iphdr));

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
static void switch_net_numbers(unsigned char *sendpacket, 
		               unsigned long network_number, 
			       unsigned char incoming)
{
	unsigned long pnetwork_number;

	pnetwork_number = (unsigned long)((sendpacket[6] << 24) + 
			  (sendpacket[7] << 16) + (sendpacket[8] << 8) + 
			  sendpacket[9]);

	if (!incoming) {
		/* If the destination network number is ours, make it 0 */
		if( pnetwork_number == network_number) {
			sendpacket[6] = sendpacket[7] = sendpacket[8] = 
					 sendpacket[9] = 0x00;
		}
	} else {
		/* If the incoming network is 0, make it ours */
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
		/* If the source network is ours, make it 0 */
		if( pnetwork_number == network_number) {
			sendpacket[18] = sendpacket[19] = sendpacket[20] = 
					 sendpacket[21] = 0x00;
		}
	} else {
		/* If the source network is 0, make it ours */
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
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats *if_stats(netdevice_t *dev)
{
	fr_channel_t* chan = wan_netif_priv(dev);
	
	if(chan == NULL)
		return NULL;

	return &chan->ifstats;
}

/****** Interrupt Handlers **************************************************/

/*============================================================================
 * fr_isr:	S508 frame relay interrupt service routine.
 *
 * Description:
 *	Frame relay main interrupt service route. This
 *      function check the interrupt type and takes
 *      the appropriate action.
 */
static WAN_IRQ_RETVAL fr_isr (sdla_t* card)
{
	int i,err;
	u8		intr_type;
	fr508_flags_t	flags;
	wan_mbox_t*	mb = &card->wan_mbox;

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	/* This flag prevents nesting of interrupts.  See sdla_isr() routine
         * in sdlamain.c.  */
	set_bit(0,&card->in_isr);
	
	++card->statistics.isr_entry;

	/* All peripheral (configuraiton, re-configuration) events
	 * take presidence over the ISR.  Thus, retrigger */
	if (test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		++card->statistics.isr_already_critical;
		goto fr_isr_exit;
	}
	
        if(card->type != SDLA_S514) {
		if (test_bit(SEND_CRIT, (void*)&card->wandev.critical)) {
                        printk(KERN_INFO "%s: Critical while in ISR: If Send Running!\n",
                                card->devname);
			++card->statistics.isr_already_critical;
			goto fr_isr_exit;
		}
	}

	switch (flags.iflag) {

                case FR_INTR_RXRDY:  /* receive interrupt */
	    		++card->statistics.isr_rx;
          		rx_intr(card);
            		break;


                case FR_INTR_TXRDY:  /* transmit interrupt */
	    		++ card->statistics.isr_tx; 
			tx_intr(card); 
            		break;

                case FR_INTR_READY:  	
	    		card->timer_int_enabled++;
			++card->statistics.isr_intr_test;
	    		break;	

                case FR_INTR_DLC: /* Event interrupt occured */
			mb->wan_command = FR_READ_STATUS;
			mb->wan_data_len = 0;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err)
				fr_event(card, err, mb);
			break;

                case FR_INTR_TIMER:  /* Timer interrupt */
			timer_intr(card);
			break;

		case FR_INTR_MODEM:
			mb->wan_command = FR_READ_STATUS;
			mb->wan_data_len = 0;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err)
				fr_event(card, err, mb);
			break;
	
		default:
	    		++card->statistics.isr_spurious;
            		spur_intr(card);
			card->hw_iface.peek(card->hw, card->intr_type_off, &intr_type, 1);
	    		printk(KERN_INFO "%s: Interrupt Type 0x%02X!\n", 
				card->devname, intr_type); 
	    
			printk(KERN_INFO "%s: ID Bytes = ",card->devname);
			{
				unsigned char str[8];
				card->hw_iface.peek(card->hw, card->intr_type_off+0x28, str, 8);
 	    			for(i = 0; i < 8; i ++){
					printk("%02X ", str[i]); 
				}
			}
	   	 	printk("\n");	
            
			break;
    	}

fr_isr_exit:
	
	clear_bit(0,&card->in_isr);
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);

	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}



/*===========================================================
 * rx_intr	Receive interrupt handler.
 *
 * Description
 * 	Upon receiveing an interrupt: 
 *	1. Check that the firmware is in sync with 
 *     	   the driver. 
 *      2. Find an appropriate network interface
 *         based on the received dlci number.
 *	3. Check that the netowrk interface exists
 *         and that it's setup properly.
 *	4. Copy the data into an skb buffer.
 *	5. Check the packet type and take
 *         appropriate acton: UPD, API, ARP or Data.
 */

static void rx_intr (sdla_t* card)
{
	fr_rx_buf_ctl_t	frbuf;
	fr_channel_t* chan;
	struct sk_buff* skb;
	netdevice_t* dev;
	void* buf;
	unsigned dlci, len, offs, len_incl_hdr;
	int i, udp_type;	

	card->hw_iface.peek(card->hw, card->rxmb_off, &frbuf, sizeof(frbuf));
	/* Check that firmware buffers are in sync */
	if (frbuf.flag != 0x01) {
		unsigned char	str[8];
		printk(KERN_INFO 
			"%s: corrupted Rx buffer @ 0x%X, flag = 0x%02X!\n", 
			card->devname, (unsigned)card->rxmb_off, frbuf.flag);
		card->hw_iface.peek(card->hw, card->intr_type_off+0x28, str, 8);
		printk(KERN_INFO "%s: ID Bytes = ",card->devname);
		for(i = 0; i < 8; i ++)
			printk("0x%02X ", str[i]);

		printk("\n");
	
		++card->statistics.rx_intr_corrupt_rx_bfr;

		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it means that driver 
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash. 
                 */
		printk(KERN_INFO "%s: Critical router failure ...!!!\n", card->devname);
		printk(KERN_INFO "Please contact Sangoma Technologies !\n");
		fr_set_intr_mode(card, 0, 0, 0);	
		return;
	}

	len  = frbuf.length;
	dlci = frbuf.dlci;
	offs = frbuf.offset;
	/* Find the network interface for this packet */
	dev = find_channel(card, dlci);
   

	/* Check that the network interface is active and
         * properly setup */
	if (dev == NULL) {
   		if( net_ratelimit()) { 
			printk(KERN_INFO "%s: rx_isr: received data on unconfigured DLCI %d!\n",
                                                card->devname, dlci);
		}
		++card->statistics.rx_intr_on_orphaned_DLCI; 
		++card->wandev.stats.rx_dropped;
		goto rx_done;
	}

	if ((chan = wan_netif_priv(dev)) == NULL){
		if( net_ratelimit()) { 
			printk(KERN_INFO "%s: rx_isr: received data on unconfigured DLCI %d!\n",
                                                card->devname, dlci);
		}
		++card->statistics.rx_intr_on_orphaned_DLCI; 
		++card->wandev.stats.rx_dropped;
		goto rx_done;
	}

	if (!is_dev_running(dev)){

		++chan->ifstats.rx_dropped;
	
		if (net_ratelimit()) { 
			printk(KERN_INFO 
				"%s: rx_isr: no dev running!\n", 
					card->devname);
		}

		chan->drvstats_rx_intr.
			rx_intr_dev_not_started ++;

		goto rx_done;
	}

	skb = wan_skb_alloc(len+2); 
	if (skb == NULL){
		++chan->ifstats.rx_dropped;
		if (net_ratelimit()) { 
			printk(KERN_INFO 
				"%s: rx_isr: no socket buffers available!\n", 
					card->devname);
		}
		chan->drvstats_rx_intr.rx_intr_no_socket ++;
		goto rx_done;
	}

	
	/* Copy data from the board into the socket buffer */
	if ((offs + len) > card->u.f.rx_top_off + 1) {
		unsigned tmp = card->u.f.rx_top_off - offs + 1;

		buf = skb_put(skb, tmp);
		card->hw_iface.peek(card->hw, offs, buf, tmp);
		offs = card->u.f.rx_base_off;
		len -= tmp;
	}

	buf = skb_put(skb, len);
	card->hw_iface.peek(card->hw, offs, buf, len);

	/* We got the packet from the bard. 
         * Check the packet type and take appropriate action */
	udp_type = udp_pkt_type( skb, card, UDP_PKT_FRM_NETWORK);

	if(udp_type != UDP_INVALID_TYPE) {

		/* UDP Debug packet received, store the
		 * packet and handle it in timer interrupt */
		skb_pull(skb, 1); 
		if (wanrouter_type_trans(skb, dev)){ 
			if(store_udp_mgmt_pkt(udp_type,UDP_PKT_FRM_NETWORK,card,skb,dlci)){

				card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TIMER); 

				if (udp_type == UDP_FPIPE_TYPE){
					++chan->drvstats_rx_intr.rx_intr_PIPE_request;
				}
			}
		}

	}else if (chan->common.usedby == API) {

		/* We are in API mode. 
                 * Add an API header to the RAW packet
                 * and queue it into a circular buffer.
                 * Then kick the fr_bh() bottom half handler */

		api_rx_hdr_t* api_rx_hdr;
		
		skb_push(skb, sizeof(api_rx_hdr_t));
		api_rx_hdr = (api_rx_hdr_t*)&skb->data[0x00];
		api_rx_hdr->wp_api_rx_hdr_fr_attr = frbuf.attr;
		api_rx_hdr->wp_api_rx_hdr_fr_time_stamp = frbuf.tmstamp;

		skb->protocol = htons(WP_PVC_PROT);
		wan_skb_reset_mac_header(skb);
		skb->dev      = dev;
		skb->pkt_type = WAN_PACKET_DATA;

		if (wan_api_enqueue_skb(chan,skb) < 0){
			wan_skb_free(skb);
			++card->wandev.stats.rx_dropped;
			goto rx_done;
		}

		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));

	}else if (handle_IPXWAN(skb->data,chan->name,chan->enable_IPX, chan->network_number)){

		if (chan->enable_IPX) {

			if (chan->tx_ipx){
				if (net_ratelimit()){
					printk(KERN_INFO 
						"%s: Dropping IPX data, tx busy : %s!\n",
						card->devname,dev->name);
				}
				wan_skb_free(skb);
			}else{
				/* Queue the ipx packet into a tx buffer
				 * which will be handeld by the tx interrupt */
				chan->tx_ipx=skb;	
			}

			atomic_inc(&card->u.f.tx_interrupts_pending);
			card->hw_iface.set_bit(card->hw, chan->dlci_int_off, FR_INTR_TXRDY);
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TXRDY);
			
		}else{
			wan_skb_free(skb);
		}

	} else if (is_arp(skb->data)) {

		/* ARP support enabled Mar 16 2000 
		 * Process incoming ARP reply/request, setup
		 * dynamic routes. */ 

		if (chan->inarp_rx == WANOPT_YES){
			int err;

			err=process_ARP((arphdr_1490_t *)skb->data, card, dev);

			switch (err){

			case 1:
				/* Got request we must send replay. The skb
				 * packet contains the replay thus, queue it
				 * into a tx_arp queue, which will be tx on
				 * next tx interrupt */

				if (chan->tx_arp){
					if (net_ratelimit()){
						printk(KERN_INFO "Warning: Dropping ARP Req, tx busy!\n");
					}
					wan_skb_free(skb);
				}else{
					/* Queue the arp packet into a tx buffer
					 * which will be handeld by the tx interrupt */
					chan->tx_arp=skb;	
				}
				atomic_inc(&card->u.f.tx_interrupts_pending);
				card->hw_iface.set_bit(card->hw, chan->dlci_int_off, FR_INTR_TXRDY);
				card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TXRDY);
				break;	

			default:

				if (err == -1 && net_ratelimit()){  
					printk (KERN_INFO 
			   		"%s: Error processing ARP Packet.\n", 
						card->devname);
				}
				wan_skb_free(skb);
				break;
			}
		}else{
			wan_skb_free(skb);
		}

	} else if (skb->data[0] != chan->fr_encap_0) {

		switch (skb->data[0]){

			case CISCO_UI:
				printk(KERN_INFO "%s-%s: Changing data encapsulation to CISCO\n",
						card->devname,dev->name);
				chan->fr_encap_0 = CISCO_UI;
				chan->fr_encap_1 = CISCO_IP;
				goto rx_intr_receive_ok;
				
			case Q922_UI:
				printk(KERN_INFO "%s-%s: Changing data encapsulation to IETF\n",
						card->devname,dev->name);
				chan->fr_encap_0 = Q922_UI;
				chan->fr_encap_1 = NLPID_IP;
				goto rx_intr_receive_ok;

			default:
	
				if (skb->data[0] != CISCO_INV && net_ratelimit()) { 
					printk(KERN_INFO 
					"%s: Rx non IETF/CISCO pkt : 0x%02X 0x%02X, dropping !\n",
					card->devname,skb->data[0],skb->data[1]);
				}
				++chan->drvstats_rx_intr.rx_intr_bfr_not_passed_to_stack;
				++chan->ifstats.rx_errors;
				++card->wandev.stats.rx_errors;
				wan_skb_free(skb);
				break;
		}

	}else{

rx_intr_receive_ok:
		
		len_incl_hdr = skb->len;
		/* Decapsulate packet and pass it up the
		   protocol stack */
		skb->dev = dev;
		
		if (chan->common.usedby == BRIDGE || chan->common.usedby == BRIDGE_NODE){
		
			/* Make sure it's an Ethernet frame, otherwise drop it */
			if (!memcmp(skb->data, "\x03\x00\x80\x00\x80\xC2\x00\x07", 8)) {
				skb_pull(skb, 8);
				skb->protocol=eth_type_trans(skb,dev);
			}else{
				++chan->drvstats_rx_intr.rx_intr_bfr_not_passed_to_stack;
				++chan->ifstats.rx_errors;
				++card->wandev.stats.rx_errors;

				/* NC: Jan 02 2002 
				 * Memory leak bug fix, deallocate packet
				 * if this is not an ethernet frame */
				wan_skb_free(skb);

				goto rx_done;
			}
		}else{
		
			/* remove hardware header */
			buf = skb_pull(skb, 1); 
	
			skb->protocol=htons(ETH_P_IP);

			if (chan->fr_encap_0 == CISCO_UI && skb->data[0] == CISCO_IP){
				skb_pull(skb,1);

			}else if (!wanrouter_type_trans(skb, dev)) {
				
				/* can't decapsulate packet */
				wan_skb_free(skb);

				++chan->drvstats_rx_intr.rx_intr_bfr_not_passed_to_stack;
				++chan->ifstats.rx_errors;
				++card->wandev.stats.rx_errors;
				goto rx_done;	
			}

			wan_skb_reset_mac_header(skb);
		} 
		

		/* Send a packed up the IP stack */
		DEBUG_SUB_MEM(skb->truesize);

#if defined(LINUX_2_4) || defined(LINUX_2_6)
		if (netif_rx(skb) == NET_RX_DROP){
			++chan->ifstats.rx_dropped;
		}else{
			++chan->drvstats_rx_intr.rx_intr_bfr_passed_to_stack;
			++chan->ifstats.rx_packets;
			++card->wandev.stats.rx_packets;
			chan->ifstats.rx_bytes += len_incl_hdr;
			card->wandev.stats.rx_bytes += len_incl_hdr;
		}
#else
		netif_rx(skb);
		++chan->drvstats_rx_intr.rx_intr_bfr_passed_to_stack;
		++chan->ifstats.rx_packets;
		++card->wandev.stats.rx_packets;	
		chan->ifstats.rx_bytes += len_incl_hdr;
		card->wandev.stats.rx_bytes += len_incl_hdr;
#endif
	}

rx_done:

       	/* Release buffer element and calculate a pointer to the next one */ 
       	frbuf.flag = 0;
	card->hw_iface.poke(card->hw, card->rxmb_off, &frbuf, sizeof(frbuf));
	card->rxmb_off += sizeof(frbuf);
	if (card->rxmb_off > card->u.f.rxmb_last_off){
		card->rxmb_off = card->u.f.rxmb_base_off;
	}

}

/*==================================================================
 * tx_intr:	Transmit interrupt handler.
 *
 * Rationale:
 *      If the board is busy transmitting, if_send() will
 *      buffers a single packet and turn on
 *      the tx interrupt. Tx interrupt will be called
 *      by the board, once the firmware can send more
 *      data. Thus, no polling is required.	 
 *
 * Description:
 *	Tx interrupt is called for each 
 *      configured dlci channel. Thus: 
 * 	1. Obtain the netowrk interface based on the
 *         dlci number.
 *      2. Check that network interface is up and
 *         properly setup.
 * 	3. Check for a buffered packed.
 *      4. Transmit the packed.
 *	5. If we are in WANPIPE mode, mark the 
 *         NET_BH handler. 
 *      6. If we are in API mode, kick
 *         the AF_WANPIPE socket for more data. 
 *	   
 */
static void tx_intr(sdla_t *card)
{
        fr508_flags_t	flags;
	unsigned long	bctl_off;
        fr_tx_buf_ctl_t	bctl;
        netdevice_t* dev;
        fr_channel_t* chan;

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));

	/* Alex Apr 8 2004 Sangoma ISA card */
	bctl_off = flags.tse_offs;
	card->hw_iface.peek(card->hw, bctl_off, &bctl, sizeof(bctl));

        /* Find the structure and make it unbusy */
        dev = find_channel(card, flags.dlci);
	if (dev == NULL){
		printk(KERN_INFO "%s: No Dev found in TX Interrupt.\n",
				card->devname);	
		goto end_of_tx_intr;
	}

        if ((chan = wan_netif_priv(dev)) == NULL){
		printk(KERN_INFO "%s: No DLCI priv area found in TX Interrupt\n",
				card->devname);	
		goto end_of_tx_intr;
	}

        if((!chan->transmit_length || !chan->delay_skb) && 
	    !chan->tx_ipx && !chan->tx_arp) {
                printk(KERN_INFO "%s: tx int error - transmit length zero\n",
				card->wandev.name);

		chan->transmit_length=0;
		if (chan->delay_skb){
			wan_skb_free(chan->delay_skb);
			chan->delay_skb=NULL;	
		}
		if (chan->tx_arp){
			wan_skb_free(chan->tx_arp);
			chan->tx_arp=NULL;
		}
		if (chan->tx_ipx){
			wan_skb_free(chan->tx_ipx);
			chan->tx_ipx=NULL;
		}
                goto tx_wakeup_net_dev;
        }

	if (!(dev->flags&IFF_UP) || 
	    chan->common.state != WAN_CONNECTED || 
	    card->wandev.state != WAN_CONNECTED){
		
		chan->transmit_length=0;
		if (chan->delay_skb){
			wan_skb_free(chan->delay_skb);
			chan->delay_skb=NULL;	
		}
		if (chan->tx_arp){
			wan_skb_free(chan->tx_arp);
			chan->tx_arp=NULL;
		}
		if (chan->tx_ipx){
			wan_skb_free(chan->tx_ipx);
			chan->tx_ipx=NULL;
		}
		bctl.flag = 0xA0;
		card->hw_iface.poke(card->hw, bctl_off, &bctl, sizeof(bctl));
		goto tx_wakeup_net_dev;
	}

	/* If the 'if_send()' procedure is currently checking the 'tbusy'
	   status, then we cannot transmit. Instead, we configure the microcode
	   so as to re-issue this transmit interrupt at a later stage. 
	*/
	if (test_bit(SEND_TXIRQ_CRIT, (void*)&card->wandev.critical)) {

		bctl.flag = 0xA0;
		card->hw_iface.poke(card->hw, bctl_off, &bctl, sizeof(bctl));
		card->hw_iface.set_bit(card->hw, chan->dlci_int_off, FR_INTR_TXRDY);
		DEBUG_EVENT("TX INTER RETRY \n");
		return;

	}else if (chan->tx_arp){
		bctl.dlci = flags.dlci;
		bctl.length = chan->tx_arp->len;
		card->hw_iface.poke(card->hw, 
		          bctl.offset, 
			  chan->tx_arp->data,
 	              	  chan->tx_arp->len);
		bctl.flag = 0xC0;
		card->hw_iface.poke(card->hw, bctl_off, &bctl, sizeof(bctl));
		++chan->ifstats.tx_packets;
		++card->wandev.stats.tx_packets;
		chan->ifstats.tx_bytes += chan->tx_arp->len;
		card->wandev.stats.tx_bytes += chan->tx_arp->len;

		wan_skb_free(chan->tx_arp);
		chan->tx_arp=NULL;
	
		chan->route_flag = ADD_ROUTE;
		trigger_fr_poll (dev);

		/* If there is not other data to be transmitted by
		 * the tx interrupt then, wakup the network
		 * interface. Otherwise, let tx interrupt retrigger.
		 */

		if (!chan->transmit_length && !chan->tx_ipx){
			goto tx_wakeup_net_dev;
		}else{
			card->hw_iface.set_bit(card->hw, chan->dlci_int_off, FR_INTR_TXRDY);
		}
		
		
 	}else if (chan->tx_ipx){
		bctl.dlci = flags.dlci;
		bctl.length = chan->tx_ipx->len+chan->fr_header_len;
		card->hw_iface.poke(card->hw, 
		          bctl.offset, 
			  chan->tx_ipx->data,
 	              	  chan->tx_ipx->len);
		bctl.flag = 0xC0;
		card->hw_iface.poke(card->hw, bctl_off, &bctl, sizeof(bctl));
		++chan->ifstats.tx_packets;
		++card->wandev.stats.tx_packets;
		chan->ifstats.tx_bytes += chan->tx_ipx->len;
		card->wandev.stats.tx_bytes += chan->tx_ipx->len;

		wan_skb_free(chan->tx_ipx);
		chan->tx_ipx=NULL;
		
		/* If there is not other data to be transmitted by
		 * the tx interrupt then, wakup the network
		 * interface. Otherwise, let tx interrupt retrigger.
		 */
		if (!chan->transmit_length){
			goto tx_wakeup_net_dev;
		}else{
			card->hw_iface.set_bit(card->hw, chan->dlci_int_off, FR_INTR_TXRDY);
			//bctl->flag = 0xA0;
		}

	}else{
	       	bctl.dlci = flags.dlci;
	        bctl.length = chan->transmit_length+chan->fr_header_len;
        	card->hw_iface.poke(card->hw, 
		          fr_send_hdr(card,bctl.dlci,bctl.offset), 
			  chan->delay_skb->data,
 	              	  chan->delay_skb->len);
	        bctl.flag = 0xC0;
		card->hw_iface.poke(card->hw, bctl_off, &bctl, sizeof(bctl));
		++chan->ifstats.tx_packets;
		++card->wandev.stats.tx_packets;
		chan->ifstats.tx_bytes += chan->transmit_length;
		card->wandev.stats.tx_bytes += chan->transmit_length;

		/* We must free an sk buffer, which we used
		 * for delayed transmission; Otherwise, the sock
		 * will run out of memory */
                wan_skb_free(chan->delay_skb);

		chan->delay_skb = NULL;				
        	chan->transmit_length = 0;

#if defined(LINUX_2_4) || defined(LINUX_2_6)
		dev->trans_start = jiffies;
#endif

tx_wakeup_net_dev:
	
		if (is_queue_stopped(dev)){
			/* If using API, than wakeup socket BH handler */
			if (chan->common.usedby == API){
				start_net_queue(dev);
				wan_wakeup_api(chan);
			}else{
				wake_net_dev(dev);
			}
		}
	}

	if (!chan->transmit_length){
		if (is_queue_stopped(dev)){
			/* If using API, than wakeup socket BH handler */
			if (chan->common.usedby == API){
				start_net_queue(dev);
				wan_wakeup_api(chan);
			}else{
				wake_net_dev(dev);
			}
		}
	}

end_of_tx_intr:

 	/* if any other interfaces have transmit interrupts pending, 
	 * do not disable the global transmit interrupt */
	atomic_dec(&card->u.f.tx_interrupts_pending);
	if(atomic_read(&card->u.f.tx_interrupts_pending)<=0){
		atomic_set(&card->u.f.tx_interrupts_pending,0);
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, FR_INTR_TXRDY);
	}
}


/*============================================================================
 * timer_intr:	Timer interrupt handler.
 *
 * Rationale:
 *	All commans must be executed within the timer
 *      interrupt since no two commands should execute
 *      at the same time.
 *
 * Description:
 *	The timer interrupt is used to:
 *    	1. Processing udp calls from 'fpipemon'.
 *    	2. Processing update calls from /proc file system
 *   	3. Reading board-level statistics for 
 *         updating the proc file system.
 *    	4. Sending inverse ARP request packets.
 *	5. Configure a dlci/channel.
 *	6. Unconfigure a dlci/channel. (Node only)
 */

static void timer_intr(sdla_t *card)
{

	/* UDP Debuging: fpipemon call */
        if (card->u.f.timer_int_enabled & TMR_INT_ENABLED_UDP) {
		if(card->u.f.udp_type == UDP_FPIPE_TYPE) {
                    	process_udp_mgmt_pkt(card,NULL);
		         card->u.f.timer_int_enabled &=
					~TMR_INT_ENABLED_UDP;
		}
        }

	/* /proc update call : triggered from update() */
	if (card->u.f.timer_int_enabled & TMR_INT_ENABLED_UPDATE) {
		fr_get_err_stats(card);
		fr_get_stats(card, NULL);
		if (IS_TE1_CARD(card)) {
			/* TE1 Update T1/E1 alarms */
			card->wandev.fe_iface.read_alarm(&card->fe, 0); 
			/* TE1 Update T1/E1 perfomance counters */
			card->wandev.fe_iface.read_pmon(&card->fe, 0); 
		}else if (IS_56K_CARD(card)) {
			/* 56K Update CSU/DSU alarms */
			card->wandev.fe_iface.read_alarm(&card->fe, 1); 
		}	
		card->update_comms_stats = 0;
		card->u.f.timer_int_enabled &= ~TMR_INT_ENABLED_UPDATE;
	}

	/* TE timer interrupt */
	if (card->u.f.timer_int_enabled & TMR_INT_ENABLED_TE){
		card->wandev.fe_iface.polling(&card->fe);
		card->u.f.timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}

	/* /proc update call : triggered from update() */
	if (card->u.f.timer_int_enabled & TMR_INT_ENABLED_UPDATE_DLCI){
		fr_get_stats(card, NULL);
		fr_get_stats(card, card->u.f.update_dlci);
		card->update_comms_stats = 0;
		card->u.f.update_dlci = NULL;
		card->u.f.timer_int_enabled &= ~TMR_INT_ENABLED_UPDATE_DLCI;
	}

	/* Update the channel state call.  This is call is
         * triggered by if_send() function */
	if (card->u.f.timer_int_enabled & TMR_INT_ENABLED_UPDATE_STATE){
		netdevice_t *dev;
		if (card->wandev.state == WAN_CONNECTED){
			struct wan_dev_le	*devle;
			WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
				fr_channel_t *chan;	

				dev = WAN_DEVLE2DEV(devle);
				
				if (!dev || !wan_netif_priv(dev)) continue;
				chan = wan_netif_priv(dev);
				if (chan->common.state != WAN_CONNECTED){
					update_chan_state(dev);
				}
			}
		}
		card->u.f.timer_int_enabled &= ~TMR_INT_ENABLED_UPDATE_STATE;
	}


	/* Transmit ARP packets */
	if (card->u.f.timer_int_enabled & TMR_INT_ENABLED_ARP){
		int i=0;
		netdevice_t *dev;

		if (card->u.f.arp_dev == NULL){
			card->u.f.arp_dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		}

		dev = card->u.f.arp_dev;

		for (;;){ 
			fr_channel_t *chan = wan_netif_priv(dev);

			/* If the interface is brought down cancel sending In-ARPs */
			if (!(dev->flags&IFF_UP)){
				clear_bit(0,&chan->inarp_ready);	
			}

			if (test_bit(0,&chan->inarp_ready)){
				int arp_err;

				if (check_tx_status(card,dev)){
					set_bit(ARP_CRIT,&card->wandev.critical);
					break;
				}

				arp_err = send_inarp_request(card,dev);

				if (arp_err == 0){
					/* Arp Send successfully */
					clear_bit(0,&chan->inarp_ready);
					trigger_fr_arp(dev);
					
				}else if(arp_err ==  -EINVAL){
					/* Arp send not supported */
					clear_bit(0,&chan->inarp_ready);
					chan->inarp = INARP_NONE;
					printk(KERN_INFO 
						"%s: Error in sending InARP. InARP not supported on %s!\n",
							card->devname,dev->name);

				}else{
					/* Arp Send failed
					 * Retrigger the arp transmission at the later
					 * time and keep the ARP_CRIT flag critical */
					break;
				}
			
				/* No matter what happend to the arp send on that
				 * particular interface, we must go throught and 
				 * check all other interfaces, on the next time */
				dev = move_dev_to_next(card,dev);
				break;
			}else{
				dev = move_dev_to_next(card,dev);
			}

			/* If we came to the last interface and all arps have
			 * been sent, then stop the timer */
			if (++i == atomic_read(&card->wandev.if_cnt)){
				card->u.f.timer_int_enabled &= ~TMR_INT_ENABLED_ARP;
				clear_bit(ARP_CRIT,&card->wandev.critical);
				break;
			}
		}
		card->u.f.arp_dev = dev;
	}

        if (!card->u.f.timer_int_enabled){
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, FR_INTR_TIMER);
	}
}


/*============================================================================
 * spur_intr:	Spurious interrupt handler.
 * 
 * Description:
 *  	We don't know this interrupt.
 *      Print a warning.
 */

static void spur_intr (sdla_t* card)
{
	if (net_ratelimit()){ 
		printk(KERN_INFO "%s: spurious interrupt!\n", card->devname);
	}
}


//FIXME: Fix the IPX in next version
/*===========================================================================
 *  Return 0 for non-IPXWAN packet
 *         1 for IPXWAN packet or IPX is not enabled!
 *  FIXME: Use a IPX structure here not offsets
 */
static int handle_IPXWAN(unsigned char *sendpacket, 
			 char *devname, unsigned char enable_IPX, 
			 unsigned long network_number)
{
	int i;

	if( sendpacket[1] == 0x00 && sendpacket[2] == 0x80 &&
	    sendpacket[6] == 0x81 && sendpacket[7] == 0x37) { 

		/* It's an IPX packet */
		if (!enable_IPX){
			/* Return 1 so we don't pass it up the stack. */
			//FIXME: Take this out when IPX is fixed
			if (net_ratelimit()){ 
				printk (KERN_INFO 
				"%s: WARNING: Unsupported IPX packet received and dropped\n",
					devname);
			}
			return 1;
		}
	} else {
		/* It's not IPX so return and pass it up the stack. */
		return 0;
	}

	if( sendpacket[24] == 0x90 && sendpacket[25] == 0x04){
		/* It's IPXWAN */

		if( sendpacket[10] == 0x02 && sendpacket[42] == 0x00){

			/* It's a timer request packet */
			printk(KERN_INFO "%s: Received IPXWAN Timer Request packet\n",
					devname);

			/* Go through the routing options and answer no to every
			 * option except Unnumbered RIP/SAP
			 */
			for(i = 49; sendpacket[i] == 0x00; i += 5){
				/* 0x02 is the option for Unnumbered RIP/SAP */
				if( sendpacket[i + 4] != 0x02){
					sendpacket[i + 1] = 0;
				}
			}

			/* Skip over the extended Node ID option */
			if( sendpacket[i] == 0x04 ){
				i += 8;
			}

			/* We also want to turn off all header compression opt.
			 */
			for(; sendpacket[i] == 0x80 ;){
				sendpacket[i + 1] = 0;
				i += (sendpacket[i + 2] << 8) + (sendpacket[i + 3]) + 4;
			}

			/* Set the packet type to timer response */
			sendpacket[42] = 0x01;

			printk(KERN_INFO "%s: Sending IPXWAN Timer Response\n",
					devname);

		} else if( sendpacket[42] == 0x02 ){

			/* This is an information request packet */
			printk(KERN_INFO 
				"%s: Received IPXWAN Information Request packet\n",
						devname);

			/* Set the packet type to information response */
			sendpacket[42] = 0x03;

			/* Set the router name */
			sendpacket[59] = 'F';
			sendpacket[60] = 'P';
			sendpacket[61] = 'I';
			sendpacket[62] = 'P';
			sendpacket[63] = 'E';
			sendpacket[64] = '-';
			sendpacket[65] = CVHexToAscii(network_number >> 28);
			sendpacket[66] = CVHexToAscii((network_number & 0x0F000000)>> 24);
			sendpacket[67] = CVHexToAscii((network_number & 0x00F00000)>> 20);
			sendpacket[68] = CVHexToAscii((network_number & 0x000F0000)>> 16);
			sendpacket[69] = CVHexToAscii((network_number & 0x0000F000)>> 12);
			sendpacket[70] = CVHexToAscii((network_number & 0x00000F00)>> 8);
			sendpacket[71] = CVHexToAscii((network_number & 0x000000F0)>> 4);
			sendpacket[72] = CVHexToAscii(network_number & 0x0000000F);
			for(i = 73; i < 107; i+= 1)
			{
				sendpacket[i] = 0;
			}

			printk(KERN_INFO "%s: Sending IPXWAN Information Response packet\n",
					devname);
		} else {

			printk(KERN_INFO "%s: Unknown IPXWAN packet!\n",devname);
			return 0;
		}

		/* Set the WNodeID to our network address */
		sendpacket[43] = (unsigned char)(network_number >> 24);
		sendpacket[44] = (unsigned char)((network_number & 0x00FF0000) >> 16);
		sendpacket[45] = (unsigned char)((network_number & 0x0000FF00) >> 8);
		sendpacket[46] = (unsigned char)(network_number & 0x000000FF);

		return 1;
	}

	/* If we get here, its an IPX-data packet so it'll get passed up the 
	 * stack.
	 * switch the network numbers 
	 */
	switch_net_numbers(&sendpacket[8], network_number ,1);
	return 0;
}
/*============================================================================
 * process_route
 * 
 * Rationale:
 *	If the interface goes down, or we receive an ARP request,
 *      we have to change the network interface ip addresses.
 * 	This cannot be done within the interrupt.
 *
 * Description:
 *
 * 	This routine is called as a polling routine to dynamically 
 *	add/delete routes negotiated by inverse ARP.  It is in this 
 *    	"task" because we don't want routes to be added while in 
 *      interrupt context.
 *
 * Usage:
 *	This function is called by fr_poll() polling funtion.
 */

static void process_route (netdevice_t *dev)
{
	fr_channel_t *chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;

	struct ifreq if_info;
	struct sockaddr_in *if_data;
	mm_segment_t fs = get_fs();
	u32 ip_tmp;
	int err;


	switch(chan->route_flag){

	case ADD_ROUTE:
				
		/* Set remote addresses */
		memset(&if_info, 0, sizeof(if_info));
		strcpy(if_info.ifr_name, dev->name);

		set_fs(get_ds());     /* get user space block */ 
		
		if_data = (struct sockaddr_in *)&if_info.ifr_dstaddr;
		if_data->sin_addr.s_addr = chan->ip_remote;
		if_data->sin_family = AF_INET;
		err = wp_devinet_ioctl( SIOCSIFDSTADDR, &if_info );

		set_fs(fs);           /* restore old block */

		if (err) {

			printk(KERN_INFO 
				"%s: Route Add failed.  Error: %d\n", 
					card->devname,err);
			printk(KERN_INFO "%s: Address: %u.%u.%u.%u\n",
				chan->name, NIPQUAD(chan->ip_remote));

		}else {
			printk(KERN_INFO "%s: Route Added Successfully: %u.%u.%u.%u\n",
				card->devname,NIPQUAD(chan->ip_remote));
			chan->route_flag = ROUTE_ADDED;
		}
		break;

	case REMOVE_ROUTE:

		/* Set remote addresses */
		memset(&if_info, 0, sizeof(if_info));
		strcpy(if_info.ifr_name, dev->name);

		ip_tmp = get_ip_address(dev,WAN_POINTOPOINT_IP);	

		set_fs(get_ds());     /* get user space block */ 
		
		if_data = (struct sockaddr_in *)&if_info.ifr_dstaddr;
		if_data->sin_addr.s_addr = 0;
		if_data->sin_family = AF_INET;
		err = wp_devinet_ioctl( SIOCSIFDSTADDR, &if_info );

		set_fs(fs);    
		
		if (err) {

			printk(KERN_INFO 
				"%s: Deleting of route failed.  Error: 0x%x", 
					card->devname,err);
			printk(KERN_INFO "%s: Address: %u.%u.%u.%u\n",
				dev->name,NIPQUAD(chan->ip_remote) );

		} else {

			printk(KERN_INFO "%s: Route Removed Sucessfuly: %u.%u.%u.%u\n", 
				card->devname,NIPQUAD(ip_tmp));
			chan->route_flag = NO_ROUTE;
		}
		break;

	} /* Case Statement */

}



/****** Frame Relay Firmware-Specific Functions *****************************/

/*============================================================================
 * Read firmware code version.
 * o fill string str with firmware version info. 
 */
static int fr_read_version (sdla_t* card, char* str)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		mbox->wan_command = FR_READ_CODE_VERSION;
		mbox->wan_data_len = 0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && retry-- && fr_event(card, err, mbox));
	
	if (!err && str) {
		int len = mbox->wan_data_len;
		memcpy(str, mbox->wan_data, len);
	        str[len] = '\0';
	}
	return err;
}

/*============================================================================
 * Set global configuration.
 */
static int fr_configure (sdla_t* card, fr_conf_t *conf)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int dlci_num = card->u.f.dlci_num;
	int err, i;

	WAN_MBOX_INIT(mb);
	do
	{
		memcpy(mb->wan_data, conf, sizeof(fr_conf_t));

		if (dlci_num){
			for (i = 0; i < dlci_num; ++i){
				((fr_conf_t*)mb->wan_data)->dlci[i] = 
						card->u.f.node_dlci[i]; 
			}
		}
		mb->wan_command = FR_SET_CONFIG;
		mb->wan_data_len =
			sizeof(fr_conf_t) + dlci_num * sizeof(short);
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	} while (err && retry-- && fr_event(card, err, mb));

	/*NC Oct 12 2000 */
	if (err != CMD_OK){
		printk(KERN_INFO "%s: Frame Relay Configuration Failed: rc=0x%x\n",
				card->devname,err);
	}
	
	return err;
}

/*============================================================================
 * Set DLCI configuration.
 */
static int fr_dlci_configure (sdla_t* card, fr_dlc_conf_t *conf, unsigned dlci)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	unsigned char *buf=mb->wan_data;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		memcpy(buf, conf, sizeof(fr_dlc_conf_t));
		mb->wan_fr_dlci = (unsigned short) dlci; 
		mb->wan_command = FR_SET_CONFIG;
		mb->wan_data_len = sizeof(fr_dlc_conf_t);
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry--);
	
	return err;
}
/*============================================================================
 * Set interrupt mode.
 */
static int fr_set_intr_mode (sdla_t* card, unsigned mode, unsigned mtu,
	unsigned short timeout)
{
	wan_mbox_t* mb = &card->wan_mbox;
	fr508_intr_ctl_t* ictl = (void*)mb->wan_data;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		memset(ictl, 0, sizeof(fr508_intr_ctl_t));
		ictl->mode   = mode;
		ictl->tx_len = mtu;
		ictl->irq    = card->wandev.irq; // ALEX_TODAY card->hw.irq;

		/* indicate timeout on timer */
		if (mode & 0x20) ictl->timeout = timeout; 

		mb->wan_data_len = sizeof(fr508_intr_ctl_t);
		mb->wan_command = FR_SET_INTR_MODE;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	} while (err && retry-- && fr_event(card, err, mb));
	
	return err;
}

/*============================================================================
 * Enable communications.
 */
static int fr_comm_enable (sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_command = FR_COMM_ENABLE;
		mb->wan_data_len = 0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry-- && fr_event(card, err, mb));
	
	return err;
}

/*============================================================================
 * fr_comm_disable 
 *
 * Warning: This functin is called by the shutdown() procedure. It is void
 *          since wan_netif_priv(dev) are has already been deallocated and no
 *          error checking is possible using fr_event() function.
 */
static void fr_comm_disable (sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	fr_set_intr_mode(card, 0, 0, 0);

	WAN_MBOX_INIT(mb);
	do {
		mb->wan_command = FR_SET_MODEM_STATUS;
		mb->wan_data_len = 1;
		mb->wan_data[0] = 0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry--);
	
	retry = MAX_CMD_RETRY;
	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_command = FR_COMM_DISABLE;
		mb->wan_data_len = 0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry--);

	return;
}



/*============================================================================
 * Get communications error statistics. 
 */
static int fr_get_err_stats (sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_command = FR_READ_ERROR_STATS;
		mb->wan_data_len = 0;
		mb->wan_fr_dlci = 0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry-- && fr_event(card, err, mb));

	if (!err) {
		fr_comm_stat_t* stats = (void*)mb->wan_data;
		card->wandev.stats.rx_over_errors    = stats->rx_overruns;
		card->wandev.stats.rx_crc_errors     = stats->rx_bad_crc;
		card->wandev.stats.rx_missed_errors  = stats->rx_aborts;
		card->wandev.stats.rx_length_errors  = stats->rx_too_long;
		card->wandev.stats.tx_aborted_errors = stats->tx_aborts;
	
	}

	return err;
}

/*============================================================================
 * Get statistics. 
 */
static int fr_get_stats (sdla_t* card, fr_channel_t* chan)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;
	int dlci = 0;

	if (chan && chan->common.state == WAN_CONNECTED){
		dlci = chan->dlci;
	}

	WAN_MBOX_INIT(mb);
	do {
		mb->wan_command = FR_READ_STATISTICS;
		mb->wan_data_len = 0;
		mb->wan_fr_dlci = dlci;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry-- && fr_event(card, err, mb));

	if (!err) {
		if (dlci == 0){
			fr_link_stat_t* stats = (void*)mb->wan_data;
			card->wandev.stats.rx_frame_errors = 
					stats->rx_bad_format;
			card->wandev.stats.rx_dropped =
					stats->rx_dropped + 
					stats->rx_dropped2;
		}else{
			fr_dlci_stat_t* stats = (void*)mb->wan_data;
			chan->rx_DE_set = stats->rx_DE_set;
		}
	}

	return err;
}

/*============================================================================
 * Add DLCI(s) (Access Node only!).
 * This routine will perform the ADD_DLCIs command for the specified DLCI.
 */
static int fr_add_dlci (sdla_t* card, int dlci)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		unsigned short* dlci_list = (void*)mb->wan_data;

		mb->wan_data_len  = sizeof(short);
		dlci_list[0] = dlci;
		mb->wan_command = FR_ADD_DLCI;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	} while (err && retry-- && fr_event(card, err, mb));
	
	return err;
}

/*============================================================================
 * Activate DLCI(s) (Access Node only!). 
 * This routine will perform the ACTIVATE_DLCIs command with a DLCI number. 
 */
static int fr_activate_dlci (sdla_t* card, int dlci)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		unsigned short* dlci_list = (void*)mb->wan_data;

		mb->wan_data_len  = sizeof(short);
		dlci_list[0] = dlci;
		mb->wan_command = FR_ACTIVATE_DLCI;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	} while (err && retry-- && fr_event(card, err, mb));
	
	return err;
}

/*============================================================================
 * Delete DLCI(s) (Access Node only!). 
 * This routine will perform the DELETE_DLCIs command with a DLCI number. 
 */
static int fr_delete_dlci (sdla_t* card, int dlci)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		unsigned short* dlci_list = (void*)mb->wan_data;

		mb->wan_data_len  = sizeof(short);
		dlci_list[0] = dlci;
		mb->wan_command = FR_DELETE_DLCI;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	} while (err && retry-- && fr_event(card, err, mb));
	
	return err;
}

/*============================================================================
 * Issue in-channel signalling frame. 
 */
static int fr_issue_isf (sdla_t* card, int isf)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_data[0] = isf;
		mb->wan_data_len  = 1;
		mb->wan_command = FR_ISSUE_IS_FRAME;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry-- && fr_event(card, err, mb));
	
	return err;
}


/*============================================================================
 * Issue in-channel signalling frame. 
 */

static unsigned int fr_send_hdr (sdla_t*card, int dlci, unsigned int offset)
{
	netdevice_t *dev = find_channel(card,dlci);	
	fr_channel_t *chan;

	if (!dev || !(chan=wan_netif_priv(dev)))
		return offset;
	
	if (chan->fr_header_len){
		card->hw_iface.poke(card->hw, offset, chan->fr_header, chan->fr_header_len);
	}
	
	return offset+chan->fr_header_len;
}

/*============================================================================
 * Send a frame on a selected DLCI.  
 */
static int fr_send_data_header (sdla_t* card, int dlci, unsigned char attr, int len,
	void *buf, unsigned char hdr_len,char lock)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	int retry = 2;//MAX_CMD_RETRY;
	int err=0,event_err=0;

	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_fr_dlci    = dlci;
		mb->wan_fr_attr    = attr;
		mb->wan_data_len  = len+hdr_len;
		mb->wan_command = FR_WRITE;
		err = card->hw_iface.cmd(card->hw, card->mbox_off + 0x800, mb);

		switch (err){

		case 0:
		case FRRES_DLCI_INACTIVE:
		case FRRES_CIR_OVERFLOW:
		case FRRES_BUFFER_OVERFLOW:
		case FRRES_TOO_LONG:
			event_err=0;
			break; 

		default:
			if (lock){
				unsigned long flags;
				s508_s514_send_event_lock(card,&flags);
				event_err=fr_event(card, err, mb);
				s508_s514_send_event_unlock(card,&flags);
			}else{
				event_err=fr_event(card, err, mb);
			}
			break;
		}
	} while (err && retry-- && event_err);
			
	if (!err) {
		fr_tx_buf_ctl_t frbuf;
		unsigned long 	frbuf_off = *(unsigned long*)mb->wan_data;
 
		card->hw_iface.peek(card->hw, frbuf_off, &frbuf, sizeof(frbuf));

		card->hw_iface.poke(card->hw, fr_send_hdr(card,dlci,frbuf.offset), buf, len);
		frbuf.flag = 0x01;
		card->hw_iface.poke(card->hw, frbuf_off, &frbuf, sizeof(frbuf));
	}

	return err;
}

static int fr_send (sdla_t* card, int dlci, unsigned char attr, int len,
	void *buf)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_fr_dlci    = dlci;
		mb->wan_fr_attr    = attr;
		mb->wan_data_len  = len;
		mb->wan_command = FR_WRITE;
		err = card->hw_iface.cmd(card->hw, card->mbox_off + 0x800, mb);
	} while (err && retry-- && fr_event(card, err, mb));

	if (!err) {
		fr_tx_buf_ctl_t frbuf;
		unsigned long 	frbuf_off = *(unsigned long*)mb->wan_data;
 
		card->hw_iface.peek(card->hw, frbuf_off, &frbuf, sizeof(frbuf));

		card->hw_iface.poke(card->hw, frbuf.offset, buf, len);
		frbuf.flag = 0x01;
		card->hw_iface.poke(card->hw, frbuf_off, &frbuf, sizeof(frbuf));

	}

	return err;
}

/*============================================================================
 * Enable timer interrupt  
 */
static void fr_enable_timer (void* card_id)
{
	sdla_t* 		card = (sdla_t*)card_id;

	card->u.f.timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TIMER); 
	return;
}

/****** Firmware Asynchronous Event Handlers ********************************/

/*============================================================================
 * Main asyncronous event/error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int fr_event (sdla_t *card, int event, wan_mbox_t* mb)
{
	fr508_flags_t	flags;
	u8		tmp;
	int i;

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));

	switch (event) {

		case FRRES_MODEM_FAILURE:
			return fr_modem_failure(card, mb);

		case FRRES_CHANNEL_DOWN:
			printk(KERN_INFO "%s: FR Event: Channel Down!\n",card->devname);
			{
				struct wan_dev_le	*devle;
				netdevice_t *dev;

			/* Remove all routes from associated DLCI's */
			WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
				fr_channel_t *chan;

				dev = WAN_DEVLE2DEV(devle);
				
				if (!dev || !wan_netif_priv(dev)){ 
					continue;
				}
				
				chan = wan_netif_priv(dev);
				if (chan->route_flag == ROUTE_ADDED) {
					chan->route_flag = REMOVE_ROUTE;
				}

				if (chan->inarp == INARP_CONFIGURED) {
					chan->inarp = INARP_REQUEST;
				}

				/* If the link becomes disconnected then,
                                 * all channels will be disconnected
                                 * as well.
                                 */
				set_chan_state(dev,WAN_DISCONNECTED);
			}
				
			wanpipe_set_state(card, WAN_DISCONNECTED);

			/* Debugging */
			WAN_DEBUG_START(card);
			return 1;
			}

		case FRRES_CHANNEL_UP:
			printk(KERN_INFO "%s: FR Event: Channel Up!\n",card->devname);
			wanpipe_set_state(card, WAN_CONNECTED);
	
			{
				struct wan_dev_le	*devle;
				netdevice_t *dev;
				fr_channel_t *chan;
				
				WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
					dev = WAN_DEVLE2DEV(devle);
					if (!dev || !wan_netif_priv(dev)) continue;
					chan = wan_netif_priv(dev);
					if (chan->dlci_stat & FR_DLCI_ACTIVE){
						if (card->wandev.ignore_front_end_status == WANOPT_YES ||
					    	    card->fe.fe_status == FE_CONNECTED){
							set_chan_state(dev,WAN_CONNECTED);
						}
					}
				}
			}
				
			return 1;

		case FRRES_DLCI_CHANGE:
			return fr_dlci_change(card, mb, 0);

		case FRRES_DLCI_MISMATCH:
			printk(KERN_INFO "%s: FR Event: DLCI list mismatch!\n", 
				card->devname);
			return 1;

		case CMD_TIMEOUT:
			printk(KERN_INFO "%s: command 0x%02X timed out!\n",
				card->devname, mb->wan_command);
			printk(KERN_INFO "%s: ID Bytes = ",card->devname);
 	    		for(i = 0; i < 8; i ++){
				card->hw_iface.peek(card->hw, card->intr_type_off+0x18+i, &tmp, 1);
				printk("0x%02X ", tmp);
			}
	   	 	printk("\n");	
            
			break;

		case FRRES_DLCI_INACTIVE:
			break;
 
		case FRRES_CIR_OVERFLOW:
			break;
			
		case FRRES_BUFFER_OVERFLOW:
			break; 
			
		default:
			printk(KERN_INFO "%s: command 0x%02X returned 0x%02X!\n"
				, card->devname, mb->wan_command, event);
			break;
	}

	return 0;
}

/*============================================================================
 * Handle modem error.
 *
 * Return zero if previous command has to be cancelled.
 */
static int fr_modem_failure (sdla_t *card, wan_mbox_t* mb)
{

	if (IS_56K_CARD(card)) {
		FRONT_END_STATUS_STRUCT	FE_status;
		unsigned long		FE_status_off = 0xF020;
		
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
		fr_handle_front_end_state (card);

	}else if (IS_TE1_CARD(card)) {
	
		FRONT_END_STATUS_STRUCT	FE_status;
		unsigned long		FE_status_off = 0xF020;


		card->hw_iface.peek(card->hw, FE_status_off, &FE_status, sizeof(FE_status));
		/* TE_INTR */
		card->wandev.fe_iface.isr(&card->fe);
		FE_status.opp_flag = 0x01;
		card->hw_iface.poke(card->hw, FE_status_off, &FE_status, sizeof(FE_status));
	
		fr_handle_front_end_state (card);

	}else{
		printk(KERN_INFO "%s: physical link down! (modem error 0x%02X)\n",
			card->devname, mb->wan_data[0]);

		if (mb->wan_data[0] == 0x28){
			card->fe.fe_status = FE_CONNECTED;	
		}else{
			card->fe.fe_status = FE_DISCONNECTED;
		}
		fr_handle_front_end_state (card);
	}
	
	
	switch (mb->wan_command){
		case FR_WRITE:
	
		case FR_READ:
			return 0;
	}
	
	return 1;
}

/*============================================================================
 * Handle DLCI status change.
 *
 * Return zero if previous command has to be cancelled.
 */
static int fr_dlci_change (sdla_t *card, wan_mbox_t* mb, int mbox_offset)
{
	struct wan_dev_le	*devle;
	dlci_status_t* status = (void*)((unsigned char*)mb->wan_data+mbox_offset);
	int cnt = mb->wan_data_len / sizeof(dlci_status_t);
	fr_channel_t *chan;
	netdevice_t* dev2;

	if (cnt > 1 && card->u.f.auto_dlci_cfg){
		netdevice_t* dev;
		printk(KERN_INFO "\n");
		printk(KERN_INFO "%s: Warning: CPE set for auto DLCI config but found %i DLCIs\n",
				card->devname,cnt);

		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			fr_channel_t *chan = wan_netif_priv(dev);
			if (chan->dlci == 0xFFFF){
				printk(KERN_INFO "%s: Error, CPE cannot select between multiple DLCIs!\n",
						card->devname);
				printk(KERN_INFO "%s: Error, Aboring auto config!\n",
						card->devname);
				printk(KERN_INFO "%s: Please turn off Auto DLCI option in %s.conf\n",
						card->devname,card->devname);

				set_chan_state(dev, WAN_DISCONNECTED);
				/* Debuggin */
				WAN_DEBUG_START(card);
				return 1;
			}
		}
	}

	
	for (; cnt; --cnt, ++status) {

		unsigned short dlci= status->dlci;
		netdevice_t* dev = find_channel(card, dlci);
		
		if (dev == NULL){
			printk(KERN_INFO 
				"%s: Warning: Unconfigured DLCI %u reported as %s, %s!\n", 
				card->devname, dlci,
				(status->state & FR_DLCI_DELETED) ? "Deleted" : "Included",
				(status->state & FR_DLCI_ACTIVE) ? "Active" : "Inactive");

		}else{
			fr_channel_t *chan = wan_netif_priv(dev);
			if (!chan){
				continue;
			}
		
			if (card->u.f.auto_dlci_cfg && chan->dlci == 0xFFFF){
				chan->dlci=dlci;
				chan->common.lcn=dlci;
			}
			chan->dlci_stat = status->state;

			if ((status->state & ~FR_DLCI_NEW) == FR_DLCI_INOPER) {
				printk(KERN_INFO
					"%s: DLCI %u reported as Included, Inactive%s!\n",
					card->devname, dlci, 
					(status->state&FR_DLCI_NEW)?", New":"");
				printk(KERN_INFO
					"%s: Remote Frame end Down!\n",
					card->devname);

				if (chan->common.state != WAN_DISCONNECTED){
					if (chan->route_flag == ROUTE_ADDED) {
						chan->route_flag = REMOVE_ROUTE;
						/* The state change will trigger
						 * the fr polling routine */
					}

					if (chan->inarp == INARP_CONFIGURED) {
						chan->inarp = INARP_REQUEST;
					}

					set_chan_state(dev, WAN_DISCONNECTED);
					/* Debugging */
					WAN_DEBUG_START(card);
				}
			}
	
			if (status->state & FR_DLCI_DELETED) {

				printk(KERN_INFO
					"%s: DLCI %u reported as Deleted!\n",
					card->devname, dlci);
				printk(KERN_INFO
					"%s: Contact Frame provider!\n",
					card->devname);
	
				
				if (chan->common.state != WAN_DISCONNECTED){
					if (chan->route_flag == ROUTE_ADDED) {
						chan->route_flag = REMOVE_ROUTE;
						/* The state change will trigger
						 * the fr polling routine */
					}

					if (chan->inarp == INARP_CONFIGURED) {
						chan->inarp = INARP_REQUEST;
					}

					set_chan_state(dev, WAN_DISCONNECTED);
					/* Debugging */
					WAN_DEBUG_START(card);
				}


			} else if (status->state & FR_DLCI_ACTIVE) {

				chan = wan_netif_priv(dev);
		
				printk(KERN_INFO
					"%s: DLCI %u reported as Included, Active%s!\n",
					card->devname, dlci,
					(status->state&FR_DLCI_NEW)?", New":"");

				/* This flag is used for configuring specific 
				   DLCI(s) when they become active.
			 	*/ 
				chan->dlci_configured = DLCI_CONFIG_PENDING;
				if (card->wandev.state == WAN_CONNECTED){
					if (card->wandev.ignore_front_end_status == WANOPT_YES ||	
				    	    card->fe.fe_status == FE_CONNECTED){
						set_chan_state(dev, WAN_CONNECTED);
					}
				}
			}
		}
	}
	
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev2 = WAN_DEVLE2DEV(devle);
		if (!dev2 || !wan_netif_priv(dev2)) continue;
		chan = wan_netif_priv(dev2);
		if (chan->dlci_configured == DLCI_CONFIG_PENDING) {
			if (fr_init_dlci(card, chan)){
				return 1;
			}
		}
	}
	return 1;
}


static int fr_init_dlci (sdla_t *card, fr_channel_t *chan)
{
	fr_dlc_conf_t cfg;

	memset(&cfg, 0, sizeof(cfg));

	if ( chan->cir_status == CIR_DISABLED) {

		cfg.cir_fwd = cfg.cir_bwd  = 16;
		cfg.bc_fwd = cfg.bc_bwd = 16;
		cfg.conf_flags = 0x0001;	

	}else if (chan->cir_status == CIR_ENABLED) {
	
		cfg.cir_fwd = cfg.cir_bwd = chan->cir;
		cfg.bc_fwd  = cfg.bc_bwd  = chan->bc;
		cfg.be_fwd  = cfg.be_bwd  = chan->be;
		cfg.conf_flags = 0x0000;
	}
	
	if (fr_dlci_configure( card, &cfg , chan->dlci)){
		printk(KERN_INFO 
			"%s: DLCI Configure failed for %d\n",
				card->devname, chan->dlci);
		return 1;	
	}
	
	chan->dlci_configured = DLCI_CONFIGURED;

	/* Read the interface byte mapping into the channel 
	 * structure.
	 */
	read_DLCI_IB_mapping( card, chan );

	return 0;
}
/******* Miscellaneous ******************************************************/

/*============================================================================
 * Update channel state. 
 */
static int update_chan_state (netdevice_t* dev)
{
	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;
	int err;

	WAN_MBOX_INIT(mb);
	do
	{
		mb->wan_command = FR_LIST_ACTIVE_DLCI;
		mb->wan_data_len = 0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry-- && fr_event(card, err, mb));

	if (!err) {
		
		unsigned short* list = (void*)mb->wan_data;
		int cnt = mb->wan_data_len / sizeof(short);
		
		err=1;
		
		for (; cnt; --cnt, ++list) {
			if (*list == chan->dlci) {
				chan->dlci_stat=0x02;
				if (card->wandev.state == WAN_CONNECTED){
					if (card->wandev.ignore_front_end_status == WANOPT_YES ||
				    	    card->fe.fe_status == FE_CONNECTED){
 						set_chan_state(dev, WAN_CONNECTED);
					}
				}else{
					set_chan_state(dev, WAN_DISCONNECTED);
				}
				
				/* May 23 2000. NC
				 * When a dlci is added or restarted,
                                 * the dlci_int_interface pointer must
				 * be reinitialized.  */
				if (!chan->dlci_int_interface_off){
					err=fr_init_dlci (card,chan);
				}
				return err;
			}
		}
	}
	chan->dlci_stat=0x00;
	set_chan_state(dev, WAN_DISCONNECTED);
	
	return err;
}

/*============================================================================
 * Set channel state.
 */
static void set_chan_state (netdevice_t* dev, int state)
{
	fr_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;

	if (chan->common.state != state) {

		struct timeval tv;
		switch (state) {

			case WAN_CONNECTED:
				printk(KERN_INFO
					"%s: Interface %s: DLCI %d connected\n",
					card->devname, dev->name, 
					chan->dlci > HIGHEST_VALID_DLCI?0:chan->dlci);

				/* If the interface was previoulsy down,
                                 * bring it up, since the channel is active */

				trigger_fr_poll (dev);
				trigger_fr_arp  (dev);
				break;

			case WAN_CONNECTING:
				printk(KERN_INFO 
				      "%s: Interface %s: DLCI %d connecting\n",
					card->devname, dev->name, 
					chan->dlci > HIGHEST_VALID_DLCI?0:chan->dlci);
				break;

			case WAN_DISCONNECTED:
				printk (KERN_INFO 
				    "%s: Interface %s: DLCI %d disconnected!\n",
					card->devname, dev->name, 
					chan->dlci > HIGHEST_VALID_DLCI?0:chan->dlci);
			
				/* If the interface is up, bring it down,
                                 * since the channel is now disconnected */
				trigger_fr_poll (dev);
				break;
		}

		chan->common.state = state;
		do_gettimeofday(&tv);
		chan->router_last_change = tv.tv_sec;

		if (chan->common.usedby == API){
			wan_update_api_state(chan);
		}
	}

	chan->state_tick = jiffies;
}

/*============================================================================
 * Find network device by its channel number.
 *
 * We need this critical flag because we change
 * the dlci_to_dev_map outside the interrupt.
 *
 * NOTE: del_if() functions updates this array, it uses
 *       the spin locks to avoid corruption.
 */
static netdevice_t* find_channel (sdla_t* card, unsigned dlci)
{

	if (card->u.f.auto_dlci_cfg){
		netdevice_t	*dev;
		
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			fr_channel_t* chan = wan_netif_priv(dev);
			if (chan->dlci != 0xFFFF && chan->dlci != dlci){
				return NULL;
			}
			return dev;
		}
		return NULL;
	}
	
	if(dlci > HIGHEST_VALID_DLCI)
		return NULL;

	return(card->u.f.dlci_to_dev_map[dlci]);
}

/*============================================================================
 * Check to see if a frame can be sent. If no transmit buffers available,
 * enable transmit interrupts.
 *
 * Return:	1 - Tx buffer(s) available
 *		0 - no buffers available
 */
static int is_tx_ready (sdla_t* card, fr_channel_t* chan)
{
	unsigned char sb;

        if(card->type == SDLA_S514)
		return 1;

	card->hw_iface.isa_read_1(card->hw, 0x00, &sb);
	if (sb & 0x02) 
		return 1;

	return 0;
}

/*============================================================================
 * Convert decimal string to unsigned integer.
 * If len != 0 then only 'len' characters of the string are converted.
 */
static unsigned int dec_to_uint (unsigned char* str, int len)
{
	unsigned val;

	if (!len) 
		len = strlen(str);

	for (val = 0; len && is_digit(*str); ++str, --len)
		val = (val * 10) + (*str - (unsigned)'0');

	return val;
}



/*=============================================================================
 * Store a UDP management packet for later processing.
 */

static int store_udp_mgmt_pkt(int udp_type, char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, int dlci)
{
        int udp_pkt_stored = 0;
	
	netdevice_t *dev=find_channel(card,dlci);
	fr_channel_t *chan;
	
	if (!dev || !(chan=wan_netif_priv(dev)))
		return 1;
	
        if(!atomic_read(&card->u.f.udp_pkt_len) && (skb->len <= MAX_LGTH_UDP_MGNT_PKT)){
		atomic_set(&card->u.f.udp_pkt_len,skb->len + chan->fr_header_len);
                card->u.f.udp_type = udp_type;
                card->u.f.udp_pkt_src = udp_pkt_src;
                card->u.f.udp_dlci = dlci;
                memcpy((u8*)&card->u.f.udp_pkt_data, skb->data, skb->len);
                card->u.f.timer_int_enabled |= TMR_INT_ENABLED_UDP;
                udp_pkt_stored = 1;

        }else{
                printk(KERN_INFO "ERROR: UDP packet not stored for DLCI %d\n", 
							dlci);
	}

        if(udp_pkt_src == UDP_PKT_FRM_STACK){
                wan_skb_free(skb);
	}else{
                wan_skb_free(skb);
	}
		
        return(udp_pkt_stored);
}


/*==============================================================================
 * Process UDP call of type FPIPE8ND
 */
static int process_udp_mgmt_pkt(sdla_t* card, void *local_dev)
{
	int c_retry = MAX_CMD_RETRY;
	unsigned char *buf;
	unsigned char frames;
	unsigned int len;
	unsigned short buffer_length;
	struct sk_buff *new_skb;
	wan_mbox_t* mb = &card->wan_mbox;
	int err;
	struct timeval tv;
	int udp_mgmt_req_valid = 1;
        netdevice_t* dev;
        fr_channel_t* chan;
        wan_udp_pkt_t *wan_udp_pkt;
	unsigned short num_trc_els;
	fr_trc_el_t* ptr_trc_el;
	fr_trc_el_t trc_el;
	fpipemon_trc_t* fpipemon_trc;

	char udp_pkt_src = card->u.f.udp_pkt_src; 
	int dlci = card->u.f.udp_dlci;

	if (local_dev == NULL){
	
		/* Find network interface for this packet */
		dev = find_channel(card, dlci);
		if (!dev){
			atomic_set(&card->u.f.udp_pkt_len,0);
			return -ENODEV;
		}
		if ((chan = wan_netif_priv(dev)) == NULL){
			atomic_set(&card->u.f.udp_pkt_len,0);
			return -ENODEV;
		}

		/* If the UDP packet is from the network, we are going to have to 
		   transmit a response. Before doing so, we must check to see that
		   we are not currently transmitting a frame (in 'if_send()') and
		   that we are not already in a 'delayed transmit' state.
		*/
		if(udp_pkt_src == UDP_PKT_FRM_NETWORK) {
			if (check_tx_status(card,dev)){
				atomic_set(&card->u.f.udp_pkt_len,0);
				return -EBUSY;
			}
		}

		wan_udp_pkt = (wan_udp_pkt_t *)&card->u.f.udp_pkt_data;

		
		if(udp_pkt_src == UDP_PKT_FRM_NETWORK) {
		
			switch(wan_udp_pkt->wan_udp_command) {

				case FR_READ_MODEM_STATUS:
				case FR_READ_STATUS:
				case FPIPE_ROUTER_UP_TIME:
				case FR_READ_ERROR_STATS:
				case FPIPE_DRIVER_STAT_GEN:
				case FR_READ_STATISTICS:
				case FR_READ_ADD_DLC_STATS:
				case FR_READ_CONFIG:
				case FR_READ_CODE_VERSION:
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
	}else{
		dev = (netdevice_t *) local_dev;
		if ((chan = wan_netif_priv(dev)) == NULL){
			return -ENODEV;
		}
		dlci=chan->dlci;	
		
		if (atomic_read(&card->u.f.udp_pkt_len) == 0){
			return -ENODEV;
		}
	
		wan_udp_pkt = (wan_udp_pkt_t *)&card->u.f.udp_pkt_data;

		udp_mgmt_req_valid=1;
	}
	

	wan_udp_pkt->wan_udp_opp_flag = 0x0; 
	if(!udp_mgmt_req_valid) {
		/* set length to 0 */
		wan_udp_pkt->wan_udp_data_len = 0;
		/* set return code */
		wan_udp_pkt->wan_udp_return_code = 0xCD; 
		
		chan->drvstats_gen.UDP_PIPE_mgmt_direction_err ++;

		if (net_ratelimit()){	
			printk(KERN_INFO 
			"%s: Warning, Illegal UDP command attempted from network: %x\n",
			card->devname,wan_udp_pkt->wan_udp_command);
		}
		
	} else {   
      
		switch(wan_udp_pkt->wan_udp_command) {

		case FPIPE_ENABLE_TRACING:
			if(!card->TracingEnabled) {
				WAN_MBOX_INIT(mb);
				do {
                       			mb->wan_command = FR_SET_TRACE_CONFIG;
                       			mb->wan_data_len = 1;
                     			mb->wan_fr_dlci = 0x00;
                   			mb->wan_data[0] = wan_udp_pkt->wan_udp_data[0] | 
						RESET_TRC;
                    			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
                       		} while (err && c_retry-- && fr_event(card, err, mb));

                        	if(err) {
					card->TracingEnabled = 0;
					/* set the return code */
					wan_udp_pkt->wan_udp_return_code =
  						mb->wan_return_code;
					mb->wan_data_len = 0;
					break;
				}

				card->hw_iface.peek(card->hw, NO_TRC_ELEMENTS_OFF,
						&num_trc_els, 2);
				card->hw_iface.peek(card->hw, BASE_TRC_ELEMENTS_OFF,
						&card->u.f.trc_el_base, 4);
				card->u.f.curr_trc_el = card->u.f.trc_el_base;
             			card->u.f.trc_el_last = card->u.f.curr_trc_el +
							((num_trc_els - 1) * 
							sizeof(fr_trc_el_t));
   
				/* Calculate the maximum trace data area in */
				/* the UDP packet */
				card->u.f.trc_bfr_space=
					MAX_LGTH_UDP_MGNT_PKT-
					sizeof(struct iphdr)-
					sizeof(struct udphdr) - 
					sizeof(wan_mgmt_t)-
					sizeof(wan_cmd_t);

				/* set return code */
				wan_udp_pkt->wan_udp_return_code = 0;
			
			} else {
                        	/* set return code to line trace already 
				   enabled */
				wan_udp_pkt->wan_udp_return_code = 1;
                    	}

			mb->wan_data_len = 0;
			card->TracingEnabled = 1;
			break;


                case FPIPE_DISABLE_TRACING:
			if(card->TracingEnabled) {
			
				WAN_MBOX_INIT(mb);
				do {
					mb->wan_command = FR_SET_TRACE_CONFIG;
					mb->wan_data_len = 1;
					mb->wan_fr_dlci = 0x00;
					mb->wan_data[0] = ~ACTIVATE_TRC;
					err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
				} while (err && c_retry-- && fr_event(card, err, mb));
                    	}

                    	/* set return code */
			wan_udp_pkt->wan_udp_return_code = 0;
			mb->wan_data_len = 0;
			card->TracingEnabled = 0;
			break;

                case FPIPE_GET_TRACE_INFO:

		        /* Line trace cannot be performed on the 502 */
                        if(!card->TracingEnabled) {
                                /* set return code */
                                wan_udp_pkt->wan_udp_return_code = 1;
                                mb->wan_data_len = 0;
                                break;
                        }

			ptr_trc_el = (fr_trc_el_t*) card->u.f.curr_trc_el;

                        buffer_length = 0;
			wan_udp_pkt->wan_udp_data[0x00] = 0x00;

                        for(frames = 0; frames < MAX_FRMS_TRACED; frames ++) {

                                card->hw_iface.peek(card->hw, (unsigned long)ptr_trc_el,
					  (void *)&trc_el.flag,
					  sizeof(fr_trc_el_t));

                                if(trc_el.flag == 0x00) {
                                        break;
				}
                                if((card->u.f.trc_bfr_space - buffer_length)
                                        < sizeof(fpipemon_trc_hdr_t)+trc_el.length) { 
                                        wan_udp_pkt->wan_udp_data[0x00] |= MORE_TRC_DATA;
                                        break;
                                }

				fpipemon_trc = 
					(fpipemon_trc_t *)&wan_udp_pkt->wan_udp_data[buffer_length]; 
				fpipemon_trc->fpipemon_trc_hdr.status =
					trc_el.attr;
                            	fpipemon_trc->fpipemon_trc_hdr.tmstamp =
					trc_el.tmstamp;
                            	fpipemon_trc->fpipemon_trc_hdr.length = 
					trc_el.length;

                                if(!trc_el.offset || !trc_el.length) {

                                     	fpipemon_trc->fpipemon_trc_hdr.data_passed = 0x00;

 				}else if((trc_el.length + sizeof(fpipemon_trc_hdr_t) + 1) >
					(card->u.f.trc_bfr_space - buffer_length)){

                                        fpipemon_trc->fpipemon_trc_hdr.data_passed = 0x00;
                                    	wan_udp_pkt->wan_udp_data[0x00] |= MORE_TRC_DATA;
 
                                }else {
                                        fpipemon_trc->fpipemon_trc_hdr.data_passed = 0x01;
                                        card->hw_iface.peek(card->hw, trc_el.offset,
                           			  fpipemon_trc->data,
						  trc_el.length);
				}			

                                trc_el.flag = 0x00;
                                card->hw_iface.poke_byte(card->hw, (unsigned long)ptr_trc_el, 0x00);
                               
				ptr_trc_el ++;
				if((void *)ptr_trc_el > card->u.f.trc_el_last)
					ptr_trc_el = (fr_trc_el_t*)card->u.f.trc_el_base;

				buffer_length += sizeof(fpipemon_trc_hdr_t);
                               	if(fpipemon_trc->fpipemon_trc_hdr.data_passed) {
                               		buffer_length += trc_el.length;
                               	}

				if(wan_udp_pkt->wan_udp_data[0x00] & MORE_TRC_DATA) {
					break;
				}
                        }
                      
			if(frames == MAX_FRMS_TRACED) {
                        	wan_udp_pkt->wan_udp_data[0x00] |= MORE_TRC_DATA;
			}
             
			card->u.f.curr_trc_el = (void *)ptr_trc_el;

                        /* set the total number of frames passed */
			wan_udp_pkt->wan_udp_data[0x00] |=
				((frames << 1) & (MAX_FRMS_TRACED << 1));

                        /* set the data length and return code */
			wan_udp_pkt->wan_udp_data_len = mb->wan_data_len = buffer_length;
                        wan_udp_pkt->wan_udp_return_code = 0;
                        break;

                case FPIPE_FT1_READ_STATUS:
			card->hw_iface.peek(card->hw, 0xF020,
				&wan_udp_pkt->wan_udp_data[0x00] , 2);
			wan_udp_pkt->wan_udp_data_len = mb->wan_data_len = 2;
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		case FPIPE_FLUSH_DRIVER_STATS:
			init_chan_statistics(chan);
			init_global_statistics(card);
			mb->wan_data_len = 0;
			break;
		
		case FPIPE_ROUTER_UP_TIME:
			do_gettimeofday(&tv);
			chan->router_up_time = tv.tv_sec - 
						chan->router_start_time;
    	                *(unsigned long *)&wan_udp_pkt->wan_udp_data =
    				chan->router_up_time;	
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 4;
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		case FPIPE_DRIVER_STAT_IFSEND:
			memcpy(wan_udp_pkt->wan_udp_data,
				&chan->drvstats_if_send.if_send_entry,
				sizeof(if_send_stat_t));
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len =sizeof(if_send_stat_t);	
			wan_udp_pkt->wan_udp_return_code = 0;
			break;
	
		case FPIPE_DRIVER_STAT_INTR:

			memcpy(wan_udp_pkt->wan_udp_data,
                                &card->statistics.isr_entry,
                                sizeof(global_stats_t));

                        memcpy(&wan_udp_pkt->wan_udp_data[sizeof(global_stats_t)],
                                &chan->drvstats_rx_intr.rx_intr_no_socket,
                                sizeof(rx_intr_stat_t));

			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 
					sizeof(global_stats_t) +
					sizeof(rx_intr_stat_t);
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		case FPIPE_DRIVER_STAT_GEN:
                        memcpy(wan_udp_pkt->wan_udp_data,
                                &chan->drvstats_gen.UDP_PIPE_mgmt_kmalloc_err,
                                sizeof(pipe_mgmt_stat_t));

                        memcpy(&wan_udp_pkt->wan_udp_data[sizeof(pipe_mgmt_stat_t)],
                               &card->statistics, sizeof(global_stats_t));

                        mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = sizeof(global_stats_t)+
                                                     sizeof(rx_intr_stat_t);
			wan_udp_pkt->wan_udp_return_code = 0;
                        break;


		case FR_FT1_STATUS_CTRL:
			if(wan_udp_pkt->wan_udp_data[0] == 1) {
				if(card->rCount++ != 0 ){
					wan_udp_pkt->wan_udp_return_code = 0;
					mb->wan_data_len = 1;
					break;
				} 
			}
           
			/* Disable FT1 MONITOR STATUS */
                        if(wan_udp_pkt->wan_udp_data[0] == 0) {
				if( --card->rCount != 0) {
                                        wan_udp_pkt->wan_udp_return_code = 0;
					mb->wan_data_len = 1;
					break;
				} 
			}  
			goto udp_mgmt_dflt;

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
#if 0
		case WAN_FE_LB_MODE:
		    /* Activate/Deactivate Line Loopback modes */
		    if (IS_TE1_CARD(card)){
			err = card->wandev.fe_iface.set_fe_lbmode(
						&card->fe, 
						wan_udp_pkt->wan_udp_data[0], 
						wan_udp_pkt->wan_udp_data[1]);
			wan_udp_pkt->wan_udp_return_code = 
					(!err) ? CMD_OK : WAN_UDP_FAILED_CMD;
		    }else{
			wan_udp_pkt->wan_udp_return_code = WAN_UDP_INVALID_CMD;
		    }
		    wan_udp_pkt->wan_udp_data_len = 0x00;
		    break;

		case WAN_GET_MEDIA_TYPE:
	 		wan_udp_pkt->wan_udp_data[0] = 
				(IS_T1_CARD(card) ? WAN_MEDIA_T1 :
				 IS_E1_CARD(card) ? WAN_MEDIA_E1 :
				 IS_56K_CARD(card) ? WAN_MEDIA_56K : 
				 				WAN_MEDIA_NONE);
		    	wan_udp_pkt->wan_udp_data_len = sizeof(unsigned char); 
			wan_udp_pkt->wan_udp_return_code = 0;
			mb->wan_data_len = sizeof(unsigned char);
			break;

		case WAN_FE_GET_STAT:
		    	if (IS_TE1_CARD(card)) {	
	 	    		/* TE1_56K Read T1/E1/56K alarms */
			  	*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					card->wandev.fe_iface.read_alarm(
							&card->fe, 0);
				/* TE1 Update T1/E1 perfomance counters */
    				card->wandev.fe_iface.read_pmon(&card->fe); 
	        		memcpy(&wan_udp_pkt->wan_udp_data[sizeof(unsigned long)],
					&card->wandev.te_pmon,
					sizeof(sdla_te_pmon_t));
		        	wan_udp_pkt->wan_udp_return_code = 0;
		    		wan_udp_pkt->wan_udp_data_len = 
					sizeof(unsigned long) + sizeof(sdla_te_pmon_t); 
				mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			}else if (IS_56K_CARD(card)){
				/* 56K Update CSU/DSU alarms */
				card->wandev.k56_alarm = sdla_56k_alarm(card, 1); 
			 	*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
			                        card->wandev.k56_alarm;
				wan_udp_pkt->wan_udp_return_code = 0;
	    			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
				mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			}
		    	break;

 		case WAN_FE_FLUSH_PMON:
 	    		/* TE1 Flush T1/E1 pmon counters */
	    		if (IS_TE1_CARD(card)){	
				card->wandev.fe_iface.flush_pmon(&card->fe);
	        		wan_udp_pkt->wan_udp_return_code = 0;
	    		}
	    		break;

		case WAN_FE_GET_CFG:
			/* Read T1/E1 configuration */
	    		if (IS_TE1_CARD(card)){	
        			memcpy(&wan_udp_pkt->wan_udp_data[0],
					&card->wandev.te_cfg,
					sizeof(sdla_te_cfg_t));
		        	wan_udp_pkt->wan_udp_return_code = 0;
	    			wan_udp_pkt->wan_udp_data_len = sizeof(sdla_te_cfg_t);
				mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			}
			break;
#endif


		case WAN_GET_PROTOCOL:
		   	wan_udp_pkt->wan_udp_data[0] = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len  = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

			
		default:
udp_mgmt_dflt:
			WAN_MBOX_INIT(mb);
 			do {
				memcpy(&mb->wan_command,
					&wan_udp_pkt->wan_udp_command,
					sizeof(wan_cmd_t));
				if(mb->wan_data_len) {
					memcpy(&mb->wan_data,
						(char *)wan_udp_pkt->wan_udp_data,
						mb->wan_data_len);
				}
 			
				err = card->hw_iface.cmd(card->hw, card->mbox_off, mb); 
				
			} while (err && c_retry-- && fr_event(card, err, mb));

			if(!err){
				chan->drvstats_gen.
					UDP_PIPE_mgmt_adptr_cmnd_OK ++;
			}else{
                                chan->drvstats_gen.
					UDP_PIPE_mgmt_adptr_cmnd_timeout ++;
			}
			
       	                /* copy the result back to our buffer */
			memcpy(&wan_udp_pkt->wan_udp_hdr.wan_cmd, mb, sizeof(wan_cmd_t)); 

                       	if(mb->wan_data_len) {
                               	memcpy(&wan_udp_pkt->wan_udp_data,
					&mb->wan_data, mb->wan_data_len);
			}
		} 
        }
  

        /* Fill UDP TTL */
        wan_udp_pkt->wan_ip_ttl = card->wandev.ttl;

	if (local_dev){
		wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
		return 1;
	}

       	len = reply_udp((u8*)&card->u.f.udp_pkt_data, mb->wan_data_len);

	if(udp_pkt_src == UDP_PKT_FRM_NETWORK) {

		chan->fr_header_len=2;
		chan->fr_header[0]=chan->fr_encap_0;
		chan->fr_header[1]=chan->fr_encap_1;
			
		err = fr_send_data_header(card, dlci, 0, len, 
			(u8*)&card->u.f.udp_pkt_data,chan->fr_header_len,0);
		if (err){ 
			chan->drvstats_gen.UDP_PIPE_mgmt_adptr_send_passed ++;
		}else{
			chan->drvstats_gen.UDP_PIPE_mgmt_adptr_send_failed ++;
		}
		
	}else{
		/* Allocate socket buffer */
		if((new_skb = wan_skb_alloc(len)) != NULL) {

			/* copy data into new_skb */
			buf = skb_put(new_skb, len);
			memcpy(buf, (u8*)&card->u.f.udp_pkt_data, len);
        
			chan->drvstats_gen.
				UDP_PIPE_mgmt_passed_to_stack ++;
			new_skb->dev = dev;
			new_skb->protocol = htons(ETH_P_IP);
			wan_skb_reset_mac_header(new_skb);

			DEBUG_SUB_MEM(new_skb->truesize);
			netif_rx(new_skb);
            	
		} else {
			chan->drvstats_gen.UDP_PIPE_mgmt_no_socket ++;
			printk(KERN_INFO 
			"%s: UDP mgmt cmnd, no socket buffers available!\n", 
			card->devname);
            	}
        }

	atomic_set(&card->u.f.udp_pkt_len,0);
	return len;
}

/*==============================================================================
 * Send Inverse ARP Request
 */

int send_inarp_request(sdla_t *card, netdevice_t *dev)
{
	int err=0;

	arphdr_1490_t *ArpPacket;
	arphdr_fr_t *arphdr;
	fr_channel_t *chan = wan_netif_priv(dev);
	struct in_device *in_dev;

	in_dev = dev->ip_ptr;

	if(in_dev != NULL ) {	

		ArpPacket = wan_malloc(sizeof(arphdr_1490_t) + sizeof(arphdr_fr_t));
		/* SNAP Header indicating ARP */
		ArpPacket->control	= 0x03;
		ArpPacket->pad		= 0x00;
		ArpPacket->NLPID	= 0x80;
		ArpPacket->OUI[0]	= 0;
		ArpPacket->OUI[1]	= 0;
		ArpPacket->OUI[2]	= 0;
		ArpPacket->PID		= 0x0608;

		arphdr = (arphdr_fr_t *)(ArpPacket + 1); // Go to ARP Packet

		/* InARP request */		
		arphdr->ar_hrd = 0x0F00;	/* Frame Relay HW type */
		arphdr->ar_pro = 0x0008;	/* IP Protocol	       */
		arphdr->ar_hln = 2;		/* HW addr length      */
		arphdr->ar_pln = 4;		/* IP addr length      */
		arphdr->ar_op = htons(0x08);	/* InARP Request       */
		arphdr->ar_sha = 0; 		/* src HW DLCI - Doesn't matter */
		if(in_dev->ifa_list != NULL)
			arphdr->ar_sip = in_dev->ifa_list->ifa_local;  /* Local Address       */else
			arphdr->ar_sip = 0;
		arphdr->ar_tha = 0; 		/* dst HW DLCI - Doesn't matter */
		arphdr->ar_tip = 0;		/* Remote Address -- what we want */

		err = fr_send(card, chan->dlci, 0, sizeof(arphdr_1490_t) + sizeof(arphdr_fr_t),
		   			(void *)ArpPacket);

		if (!err){
			printk(KERN_INFO "\n");
			printk(KERN_INFO "%s: Sending InARP request on DLCI %d.\n", 
				card->devname, chan->dlci);
		}
	
		DEBUG_SUB_MEM(sizeof(arphdr_1490_t) + sizeof(arphdr_fr_t));
		wan_free(ArpPacket);
	}else{
		printk(KERN_INFO "%s: INARP ERROR: %s doesn't have a local IP address!\n",
				card->devname,dev->name);
		return -EINVAL;
	}

	return err;
}
	

/*==============================================================================
 * Check packet for ARP Type
 */

int is_arp(void *buf)
{
	arphdr_1490_t *arphdr = (arphdr_1490_t *)buf;
	
	if (arphdr->pad   == 0x00  &&
	    arphdr->NLPID == 0x80  &&
	    arphdr->PID   == 0x0608) 
		return 1;
	else return 0;
}

/*==============================================================================
 * Process ARP Packet Type
 */

int process_ARP(arphdr_1490_t *ArpPacket, sdla_t *card, netdevice_t* dev)
{

	int err=0;
	arphdr_fr_t *arphdr = (arphdr_fr_t *)(ArpPacket + 1); /* Skip header */
	struct in_device *in_dev;
	fr_channel_t *chan = wan_netif_priv(dev);

	in_dev = dev->ip_ptr;

	/* Check that IP addresses exist for our network address */
	if (in_dev == NULL || in_dev->ifa_list == NULL){ 
		err = -1;
		goto process_arp_exit;
	}

	switch (ntohs(arphdr->ar_op)) {

	case 0x08:  // Inverse ARP request  -- Send Reply, add route.
			
		/* Check for valid Address */
		printk(KERN_INFO "%s: Recvd PtP addr -InArp Req: %u.%u.%u.%u\n", 
			card->devname, NIPQUAD(arphdr->ar_sip));

		/* Check that the network address is the same as ours, only
                 * if the netowrk mask is not 255.255.255.255. Otherwise
                 * this check would not make sense */

		if (in_dev->ifa_list->ifa_mask != 0xFFFFFFFF && 
		    (in_dev->ifa_list->ifa_mask & arphdr->ar_sip) != 
		    (in_dev->ifa_list->ifa_mask & in_dev->ifa_list->ifa_local)){

			printk(KERN_INFO 
				"%s: Error, Invalid PtP address. %u.%u.%u.%u  InARP ignored.\n", 
					card->devname,NIPQUAD(arphdr->ar_sip));

			printk(KERN_INFO "%s: mask %u.%u.%u.%u\n", 
				card->devname, NIPQUAD(in_dev->ifa_list->ifa_mask));
			printk(KERN_INFO "%s: local %u.%u.%u.%u\n", 
				card->devname,NIPQUAD(in_dev->ifa_list->ifa_local));

			err = -1;
			goto process_arp_exit;
		}

		if (in_dev->ifa_list->ifa_local == arphdr->ar_sip){
			printk(KERN_INFO 
				"%s: Local addr = PtP addr.  InARP ignored.\n", 
					card->devname);

			err = -1;
			goto process_arp_exit;
		}
	
		arphdr->ar_op = htons(0x09);	/* InARP Reply */

		/* Set addresses */
		arphdr->ar_tip = arphdr->ar_sip;
		arphdr->ar_sip = in_dev->ifa_list->ifa_local;

		chan->ip_local = in_dev->ifa_list->ifa_local;
		chan->ip_remote = arphdr->ar_sip;

		/* Signal the above function to queue up the 
		 * arp response and send it to a tx interrupt */
		err = 1;

		break;

	case 0x09:  // Inverse ARP reply

		/* Check for valid Address */
		printk(KERN_INFO "%s: Recvd PtP addr %u.%u.%u.%u -InArp Reply\n", 
				card->devname, NIPQUAD(arphdr->ar_sip));


		/* Compare network addresses, only if network mask
                 * is not 255.255.255.255  It would not make sense
                 * to perform this test if the mask was all 1's */

		if (in_dev->ifa_list->ifa_mask != 0xffffffff &&
		    (in_dev->ifa_list->ifa_mask & arphdr->ar_sip) != 
			(in_dev->ifa_list->ifa_mask & in_dev->ifa_list->ifa_local)) {

			printk(KERN_INFO "%s: Error, Invalid PtP address.  InARP ignored.\n", 
					card->devname);
			err = -1;
			goto process_arp_exit;
		}

		/* Make sure that the received IP address is not
                 * the same as our own local address */
		if (in_dev->ifa_list->ifa_local == arphdr->ar_sip) {
			printk(KERN_INFO "%s: Local addr = PtP addr.  InARP ignored.\n", 
				card->devname);
			err = -1;
			goto process_arp_exit;
		}			

		chan->ip_local  = in_dev->ifa_list->ifa_local;
		chan->ip_remote = arphdr->ar_sip;

		/* Add Route Flag */
		/* The route will be added in the polling routine so
		   that it is not interrupt context. */

		chan->route_flag = ADD_ROUTE;
		chan->inarp = INARP_CONFIGURED;
		trigger_fr_poll(dev);
		
		break;
	default:  // ARP's and RARP's -- Shouldn't happen.
		DEBUG_EVENT("%s: Rx invalid frame relay ARP/RARP packet! 0x%x\n",
				card->devname, ntohs(arphdr->ar_op));
		err=1;
		break;
	}

process_arp_exit:

	return err;
}


/*============================================================
 * trigger_fr_arp
 *
 * Description:
 * 	Add an fr_arp() task into a arp
 *      timer handler for a specific dlci/interface.  
 *      This will kick the fr_arp() routine 
 *      within the specified time interval. 
 *
 * Usage:
 * 	This timer is used to send ARP requests at
 *      certain time intervals. 
 * 	Called by an interrupt to request an action
 *      at a later date.
 */	

static void trigger_fr_arp (netdevice_t *dev)
{
	fr_channel_t* chan = wan_netif_priv(dev);

	del_timer(&chan->fr_arp_timer);
	chan->fr_arp_timer.expires = jiffies + (chan->inarp_interval * HZ);
	add_timer(&chan->fr_arp_timer);
	return;
}



/*==============================================================================
 * ARP Request Action
 *
 *	This funciton is called by timer interrupt to send an arp request
 *      to the remote end.
 */

static void fr_arp (unsigned long data)
{
	netdevice_t *dev = (netdevice_t *)data;
	fr_channel_t *chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;

	/* Send ARP packets for all devs' until
         * ARP state changes to CONFIGURED */

	if (chan->inarp == INARP_REQUEST &&
	    chan->common.state == WAN_CONNECTED && 
	    card->wandev.state == WAN_CONNECTED){
		set_bit(0,&chan->inarp_ready);
		card->u.f.timer_int_enabled |= TMR_INT_ENABLED_ARP;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, FR_INTR_TIMER);
	}
 
	return;
}
	

/*==============================================================================
 * Perform the Interrupt Test by running the READ_CODE_VERSION command MAX_INTR_
 * TEST_COUNTER times.
 */
static int intr_test( sdla_t* card )
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err,i;

        err = fr_set_intr_mode(card, FR_INTR_READY, card->wandev.mtu, 0 );
	
	if (err == CMD_OK) {

		WAN_MBOX_INIT(mb);	
		for ( i = 0; i < MAX_INTR_TEST_COUNTER; i++ ) {
 			/* Run command READ_CODE_VERSION */
			mb->wan_data_len  = 0;
			mb->wan_command = FR_READ_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != CMD_OK) 
				fr_event(card, err, mb);
		}
	
	} else {
		printk(KERN_INFO "Interrupt test failed err=0x%02X\n",
				err);
		return err;	
	}

	err = fr_set_intr_mode( card, 0, card->wandev.mtu, 0 );

	if( err != CMD_OK ) 
		return err;

	return 0;
}

/*==============================================================================
 * Determine what type of UDP call it is. FPIPE8ND ?
 */
static int udp_pkt_type( struct sk_buff *skb, sdla_t* card, int direction)
{
	wan_udp_pkt_t *wan_udp_pkt = (wan_udp_pkt_t *)skb->data;

	if (skb->len < sizeof(wan_udp_pkt_t)){
		return UDP_INVALID_TYPE;
	}

	/* Quick HACK */
	if (direction == UDP_PKT_FRM_NETWORK){
		wan_udp_pkt = (wan_udp_pkt_t *)&skb->data[2];
	}
	
        if((wan_udp_pkt->wan_ip_p == UDPMGMT_UDP_PROTOCOL) &&
		(wan_udp_pkt->wan_udp_dport == ntohs(card->wandev.udp_port)) &&
		(wan_udp_pkt->wan_udp_request_reply == UDPMGMT_REQUEST)) {
                        if(!strncmp(wan_udp_pkt->wan_udp_signature,
                                UDPMGMT_FPIPE_SIGNATURE, 8)){
                                return UDP_FPIPE_TYPE;
			}
			if(!strncmp(wan_udp_pkt->wan_udp_signature,
                                GLOBAL_UDP_SIGNATURE, 8)){
                                return UDP_FPIPE_TYPE;
			}
	}
        return UDP_INVALID_TYPE;
}


/*==============================================================================
 * Initializes the Statistics values in the fr_channel structure.
 */
void init_chan_statistics( fr_channel_t* chan)
{
        memset(&chan->drvstats_if_send.if_send_entry, 0,
		sizeof(if_send_stat_t));
        memset(&chan->drvstats_rx_intr.rx_intr_no_socket, 0,
                sizeof(rx_intr_stat_t));
        memset(&chan->drvstats_gen.UDP_PIPE_mgmt_kmalloc_err, 0,
                sizeof(pipe_mgmt_stat_t));
}
	
/*==============================================================================
 * Initializes the Statistics values in the Sdla_t structure.
 */
void init_global_statistics( sdla_t* card )
{
	/* Intialize global statistics for a card */
        memset(&card->statistics.isr_entry, 0, sizeof(global_stats_t));
}

static void read_DLCI_IB_mapping( sdla_t* card, fr_channel_t* chan )
{
	wan_mbox_t* mb = &card->wan_mbox;
	int retry = MAX_CMD_RETRY;	
	dlci_IB_mapping_t* result; 
	int err, counter, found;	

	WAN_MBOX_INIT(mb);	
	do {
		mb->wan_command = FR_READ_DLCI_IB_MAPPING;
		mb->wan_data_len = 0;	
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	} while (err && retry-- && fr_event(card, err, mb));

	if( mb->wan_return_code != 0){
		printk(KERN_INFO "%s: Read DLCI IB Mapping failed\n", 
			chan->name);
	}

	counter = mb->wan_data_len / sizeof(dlci_IB_mapping_t);
	result = (void *)mb->wan_data;
	
	found = 0;
	for (; counter; --counter, ++result) {
		if ( result->dlci == chan->dlci ) {
			chan->IB_addr = result->addr_value;
			/* Alex Apr 8 2004 Sangoma ISA card */
	             	chan->dlci_int_interface_off = chan->IB_addr;
	             	chan->dlci_int_off = 
				chan->IB_addr + offsetof(fr_dlci_interface_t, gen_interrupt);
			found = 1;
			break;	
		} 
	}
	if (!found)
		printk( KERN_INFO "%s: DLCI %d not found by IB MAPPING cmd\n", 
		card->devname, chan->dlci);
}



void s508_s514_lock(sdla_t *card, unsigned long *smp_flags)
{
	if (card->type != SDLA_S514){
		spin_lock_irqsave(&card->wandev.lock, *smp_flags);
	}else{
		spin_lock(&card->u.f.if_send_lock);
	}
	return;
}


void s508_s514_unlock(sdla_t *card, unsigned long *smp_flags)
{
	if (card->type != SDLA_S514){
		spin_unlock_irqrestore (&card->wandev.lock, *smp_flags);
	}else{
		spin_unlock(&card->u.f.if_send_lock);
	}
	return;
}


void s508_s514_send_event_lock(sdla_t *card, unsigned long *smp_flags)
{
	if (card->type == SDLA_S514){
		spin_lock_irqsave(&card->wandev.lock, *smp_flags);		
	}
	return;
}

void s508_s514_send_event_unlock(sdla_t *card, unsigned long *smp_flags)
{
	if (card->type == SDLA_S514){
		spin_unlock_irqrestore(&card->wandev.lock, *smp_flags);		
	}
	return;
}



/*----------------------------------------------------------------------
                  RECEIVE INTERRUPT: BOTTOM HALF HANDLERS 
 ----------------------------------------------------------------------*/

/*========================================================
 * fr_bh
 *
 * Description:
 *	Frame relay receive BH handler. 
 *	Dequeue data from the BH circular 
 *	buffer and pass it up the API sock.
 *       	
 * Rationale: 
 *	This fuction is used to offload the 
 *	rx_interrupt during API operation mode.  
 *	The fr_bh() function executes for each 
 *	dlci/interface.  
 * 
 *      Once receive interrupt copies data from the
 *      card into an skb buffer, the skb buffer
 *  	is appended to a circular BH buffer.
 *  	Then the interrupt kicks fr_bh() to finish the
 *      job at a later time (no within the interrupt).
 *       
 * Usage:
 * 	Interrupts use this to defer a taks to 
 *      a polling routine.
 *
 */	

static void fr_bh (unsigned long data)
{
	fr_channel_t* chan = (fr_channel_t*)data;
	sdla_t *card = chan->card;
	struct sk_buff *skb;
	int len;

	while ((skb=wan_api_dequeue_skb(chan)) != NULL){

		len=skb->len;
	
		if (wan_api_rx(chan,skb) == 0){
			card->wandev.stats.rx_packets++;
			card->wandev.stats.rx_bytes += len;
			chan->ifstats.rx_packets++;
			chan->ifstats.rx_bytes += len;
			chan->drvstats_rx_intr.rx_intr_bfr_passed_to_stack ++;
		}else{
			++card->wandev.stats.rx_dropped;
			++chan->ifstats.rx_dropped;
			wan_skb_free(skb);	
		}
		
	}	

	WAN_TASKLET_END((&chan->common.bh_task));

	return;
}

/*----------------------------------------------------------------------
               POLL BH HANDLERS AND KICK ROUTINES 
 ----------------------------------------------------------------------*/

/*============================================================
 * trigger_fr_poll
 *
 * Description:
 * 	Add a fr_poll() task into a tq_scheduler bh handler
 *      for a specific dlci/interface.  This will kick
 *      the fr_poll() routine at a later time. 
 *
 * Usage:
 * 	Interrupts use this to defer a taks to 
 *      a polling routine.
 *
 */	
static void trigger_fr_poll (netdevice_t *dev)
{
	fr_channel_t* chan = wan_netif_priv(dev);
	
	
	WAN_TASKQ_SCHEDULE((&chan->fr_poll_task));
	return;
}


/*============================================================
 * fr_poll
 *	
 * Rationale:
 * 	We cannot manipulate the routing tables, or
 *      ip addresses withing the interrupt. Therefore
 *      we must perform such actons outside an interrupt 
 *      at a later time. 
 *
 * Description:	
 *	Frame relay polling routine, responsible for 
 *     	shutting down interfaces upon disconnect
 *     	and adding/removing routes. 
 *      
 * Usage:        
 * 	This function is executed for each frame relay
 * 	dlci/interface through a tq_schedule bottom half.
 *      
 *      trigger_fr_poll() function is used to kick
 *      the fr_poll routine.  
 */


# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void fr_poll (void *dev_ptr)
#else
static void fr_poll (struct work_struct *work)
#endif
{
	sdla_t *card;
	u8 check_gateway=0; 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
	netdevice_t 	*dev;
        fr_channel_t	*chan = container_of(work, fr_channel_t, fr_poll_task);
	dev=chan->common.dev;
	if (!dev) {
         	return;
	}
#else
	fr_channel_t* chan;
	netdevice_t *dev=(netdevice_t*)dev_ptr;
	if (!dev || (chan = wan_netif_priv(dev)) == NULL) {
		return;
	}
#endif      

	card = chan->card;
	
	/* (Re)Configuraiton is in progress, stop what you are 
	 * doing and get out */
	if (test_bit(PERI_CRIT,&card->wandev.critical)){
		return;
	}

	switch (chan->common.state){

	case WAN_DISCONNECTED:

		if (test_bit(DYN_OPT_ON,&chan->interface_down) &&
		    !test_bit(DEV_DOWN, &chan->interface_down) &&
		    dev->flags&IFF_UP){

			printk(KERN_INFO "%s: Interface %s is Down.\n", 
				card->devname,dev->name);
			change_dev_flags(dev,dev->flags&~IFF_UP);
			set_bit(DEV_DOWN, &chan->interface_down);
			chan->route_flag = NO_ROUTE;
			
		}else{
			if (chan->inarp != INARP_NONE)
				process_route(dev);	
		}
		break;

	case WAN_CONNECTED:

		if (test_bit(DYN_OPT_ON,&chan->interface_down) &&
		    test_bit(DEV_DOWN, &chan->interface_down) &&
		    !(dev->flags&IFF_UP)){

			printk(KERN_INFO "%s: Interface %s is Up.\n", 
					card->devname,dev->name);

			change_dev_flags(dev,dev->flags|IFF_UP);
			clear_bit(DEV_DOWN, &chan->interface_down);
			check_gateway=1;
		}

		if (chan->inarp != INARP_NONE){
			process_route(dev);
			check_gateway=1;
		}

		if (chan->gateway && check_gateway)
			add_gateway(card,dev);

		break;

	}

	return;	
}

/*==============================================================
 * check_tx_status
 *
 * Rationale:
 *	We cannot transmit from an interrupt while
 *      the if_send is transmitting data.  Therefore,
 *      we must check whether the tx buffers are
 *      begin used, before we transmit from an
 *      interrupt.	
 * 
 * Description:	
 *	Checks whether it's safe to use the transmit 
 *      buffers. 
 *
 * Usage:
 * 	ARP and UDP handling routines use this function
 *      because, they need to transmit data during
 *      an interrupt.
 */

static int check_tx_status(sdla_t *card, netdevice_t *dev)
{

	if (card->type == SDLA_S514){
		if (test_bit(SEND_CRIT, (void*)&card->wandev.critical) ||
			test_bit(SEND_TXIRQ_CRIT, (void*)&card->wandev.critical)) {
			return 1;
		}
	}

	if (is_queue_stopped(dev) || (atomic_read(&card->u.f.tx_interrupts_pending)))
     		return 1; 

	return 0;
}

/*===============================================================
 * move_dev_to_next
 *  
 * Description:
 *	Move the dev pointer to the next location in the
 *      link list.  Check if we are at the end of the 
 *      list, if so start from the begining.
 *
 * Usage:
 * 	Timer interrupt uses this function to efficiently
 *      step through the devices that need to send ARP data.
 *
 */

netdevice_t * move_dev_to_next (sdla_t *card, netdevice_t *dev)
{
	struct wan_dev_le	*devle;
	if (atomic_read(&card->wandev.if_cnt) == 1){
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

static void config_fr_dev (sdla_t *card,netdevice_t *dev)
{
	fr_channel_t *chan=wan_netif_priv(dev);

	/* If signalling is set to NO, then setup 
       	 * DLCI addresses right away.  Don't have to wait for
	 * link to connect. 
	 */
	if (card->wandev.signalling == WANOPT_NO){
		printk(KERN_INFO "%s: Signalling set to NO: Mapping DLCI's\n",
				card->wandev.name);
		if (fr_init_dlci(card,chan)){
			printk(KERN_INFO "%s: ERROR: Failed to configure DLCI %i !\n",
				card->devname, chan->dlci);
			return;
		}
	}

	if (card->wandev.station == WANOPT_CPE) {

		update_chan_state(dev);	
	
		/* CPE: User enabled option to send out a Full
		 * status request as soon as all interfaces are
		 * configured */
		if (card->u.f.issue_fs_on_startup && 
		    (atomic_read(&card->wandev.if_cnt) == atomic_read(&card->wandev.if_up_cnt))){
			card->u.f.issue_fs_on_startup=0;
			fr_issue_isf(card, FR_ISF_FSE);
		}

	} else {	
		/* FR switch: activate DLCI(s) */
	
		/* For Switch emulation we have to ADD and ACTIVATE
		 * the DLCI(s) that were configured with the SET_DLCI_
		 * CONFIGURATION command. Add and Activate will fail if
		 * DLCI specified is not included in the list.
		 *
		 * Also If_open is called once for each interface. But
		 * it does not get in here for all the interface. So
		 * we have to pass the entire list of DLCI(s) to add 
		 * activate routines.  
		 */ 
		
		if (!check_dlci_config (card, chan)){
			fr_add_dlci(card, chan->dlci);
			fr_activate_dlci(card, chan->dlci);
		}
	}

	if (!card->u.f.auto_dlci_cfg){
		card->u.f.dlci_to_dev_map[chan->dlci] = dev;
	}
	return;
}


/*==============================================================
 * unconfig_fr_dev
 *
 * Rationale:
 *	All commands must be executed during an interrupt.
 * 
 * Description:	
 *	Remove the dlci from firmware.
 *	This funciton is used in NODE shutdown.
 */

static void unconfig_fr_dev (sdla_t *card,netdevice_t *dev)
{
	fr_channel_t *chan=wan_netif_priv(dev);

	if (card->wandev.station == WANOPT_NODE){
		printk(KERN_INFO "%s: Unconfiguring DLCI %i\n",
				card->devname,chan->dlci);
		fr_delete_dlci(card,chan->dlci);
	}

	if (!card->u.f.auto_dlci_cfg){
		card->u.f.dlci_to_dev_map[chan->dlci] = NULL;
	}
}


static int setup_fr_header(struct sk_buff ** skb_orig, netdevice_t* dev, char op_mode)
{
	struct sk_buff *skb = *skb_orig;
	fr_channel_t *chan=wan_netif_priv(dev);

	if (op_mode == WANPIPE){

		switch (htons(skb->protocol)){
			
		case ETH_P_IP:
			chan->fr_header[0]=chan->fr_encap_0;
			chan->fr_header[1]=chan->fr_encap_1;
			return 2;

		case ETH_P_IPX: 
			chan->fr_header[0]=Q922_UI;
			chan->fr_header[1]=0;
			chan->fr_header[2]=NLPID_SNAP;
			chan->fr_header[3]=0;
			chan->fr_header[4]=0;
			chan->fr_header[5]=0;
			*((unsigned short*)&chan->fr_header[6]) = skb->protocol;
			return 8;

		default:
			return -EINVAL;
		}
	}

	/* If we are in bridging mode, we must apply
	 * an Ethernet header */
	if (op_mode == BRIDGE || op_mode == BRIDGE_NODE){

		/* Encapsulate the packet as a bridged Ethernet frame. */
#ifdef DEBUG
		printk(KERN_INFO "%s: encapsulating skb for frame relay\n", 
			dev->name);
#endif
		
		chan->fr_header[0] = 0x03;
		chan->fr_header[1] = 0x00;
		chan->fr_header[2] = 0x80;
		chan->fr_header[3] = 0x00;
		chan->fr_header[4] = 0x80;
		chan->fr_header[5] = 0xC2;
		chan->fr_header[6] = 0x00;
		chan->fr_header[7] = 0x07;

		/* Yuck. */
		skb->protocol = ETH_P_802_3;
		return 8;
	}
		
	return 0;
}


static int check_dlci_config (sdla_t *card, fr_channel_t *chan)
{
	struct wan_dev_le	*devle;
	wan_mbox_t* mb = &card->wan_mbox;
	int err=0;
	fr_conf_t *conf=NULL;
	unsigned short dlci_num = chan->dlci;
	int dlci_offset=0;
	netdevice_t *dev=NULL;

	WAN_MBOX_INIT(mb);	
	mb->wan_command = FR_READ_CONFIG;
	mb->wan_data_len = 0;
	mb->wan_fr_dlci = dlci_num; 	
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	if (err == CMD_OK){
		return 0;
	}

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev) continue;
		set_chan_state(dev,WAN_DISCONNECTED);
	}
	
	printk(KERN_INFO "%s: FR NODE configuring unconfigured DLCI %i\n",
			card->devname,dlci_num);
	
	WAN_MBOX_INIT(mb);	
	mb->wan_command = FR_COMM_DISABLE;
	mb->wan_data_len = 0;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != CMD_OK){
		fr_event(card, err, mb);
		return 2;
	}

	printk(KERN_INFO "%s: FR NODE disabled communications \n",
			card->devname);
	
	WAN_MBOX_INIT(mb);	
	mb->wan_command = FR_READ_CONFIG;
	mb->wan_data_len = 0;
	mb->wan_fr_dlci = 0; 	
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	if (err != CMD_OK){
		fr_event(card, err, mb);
		return 2;
	}
	
	conf = (fr_conf_t *)mb->wan_data;

	dlci_offset=0;

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		fr_channel_t *chan_tmp; 

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)) continue;
		chan_tmp = wan_netif_priv(dev);
		conf->dlci[dlci_offset] = chan_tmp->dlci;		
		dlci_offset++;
	}
	
	WAN_MBOX_INIT(mb);	
	mb->wan_data_len = 0x20 + dlci_offset*2;

	mb->wan_command = FR_SET_CONFIG;
	mb->wan_fr_dlci = 0; 
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != CMD_OK){
		fr_event(card, err, mb);
		return 2;
	}

	initialize_rx_tx_buffers (card);
	
	printk(KERN_INFO "%s: FR Node add new DLCI %i\n",
			card->devname,dlci_num);

	if (fr_comm_enable (card)){
		return 2;
	}

	printk(KERN_INFO "%s: FR Node Enabling Communications \n",
			card->devname);

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		fr_channel_t *chan_tmp;

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)) continue;
		chan_tmp = wan_netif_priv(dev);
		fr_init_dlci(card,chan_tmp);
		fr_add_dlci(card, chan_tmp->dlci);
		fr_activate_dlci(card, chan_tmp->dlci);
	}

	return 1;
}

static void initialize_rx_tx_buffers (sdla_t *card)
{
	fr_buf_info_t	buf_info;
	unsigned long	buf_info_off;
	
	/* Alex Apr 8 2004 Sangoma ISA card */
	buf_info_off = FR_MB_VECTOR + FR508_RXBC_OFFS;
	card->hw_iface.peek(card->hw, buf_info_off, &buf_info, sizeof(buf_info));

	card->rxmb_off = buf_info.rse_next;
	card->u.f.rxmb_base_off = buf_info.rse_base; 
	card->u.f.rxmb_last_off =
			(buf_info.rse_num - 1) * sizeof(fr_rx_buf_ctl_t) +
                        buf_info.rse_base;

	card->u.f.rx_base_off = buf_info.buf_base;
	card->u.f.rx_top_off  = buf_info.buf_top;

	atomic_set(&card->u.f.tx_interrupts_pending,0);

	return;
}

static int set_adapter_config (sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	ADAPTER_CONFIGURATION_STRUCT* cfg = (ADAPTER_CONFIGURATION_STRUCT*)mb->wan_data;
	int err;

	WAN_MBOX_INIT(mb);	
	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &cfg->adapter_type); 
	cfg->adapter_config = 0x00; 
	cfg->operating_frequency = 00; 
	mb->wan_data_len = sizeof(ADAPTER_CONFIGURATION_STRUCT);
	mb->wan_command = SET_ADAPTER_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(err != 0) {
		fr_event(card,err,mb);
	}
	return (err);
}



/*
 * Proc FS function 
 */
#define PROC_CFG_FRM	"%-15s| %-12s| %-5u|\n"
#define PROC_STAT_FRM	"%-15s| %-12s| %-14s|\n"
static char fr_config_hdr[] =
	"Interface name | Device name | DLCI |\n";
static char fr_status_hdr[] =
	"Interface name | Device name | Status        |\n";

static int fr_get_config_info(void* priv, struct seq_file* m, int* stop_cnt) 
{
	fr_channel_t*	chan = (fr_channel_t*)priv;
	sdla_t*		card = chan->card;

	if (chan == NULL)
		return m->count;
	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m, "%s", fr_config_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_CFG_FRM, chan->name, card->devname, chan->dlci);
	return m->count;
}

static int fr_get_status_info(void* priv, struct seq_file* m, int* stop_cnt)
{
	fr_channel_t*	chan = (fr_channel_t*)priv;
	sdla_t*		card = chan->card;

	if (chan == NULL)
		return m->count;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", fr_status_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_STAT_FRM, chan->name, card->devname, STATE_DECODE(chan->common.state));
	return m->count;
}

#define PROC_DEV_FR_S_FRM	"%-20s| %-14s|\n"
#define PROC_DEV_FR_D_FRM	"%-20s| %-14d|\n"
#define PROC_DEV_SEPARATE	"=====================================\n"

static int fr_set_dev_config(struct file *file, 
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

static int fr_set_if_info(struct file *file, 
			  const char *buffer,
			  unsigned long count, 
			  void *data)
{
	netdevice_t*	dev = (void*)data;
	fr_channel_t* 	chan = (fr_channel_t*)wan_netif_priv(dev);
	sdla_t*		card = chan->card;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return count;
	chan = (fr_channel_t*)wan_netif_priv(dev);
	if (chan->card == NULL)
		return count;
	card = chan->card;

	printk(KERN_INFO "%s: New interface config (%s)\n",
			chan->name, buffer);
	/* Parse string */

	return count;
}


static void fr_handle_front_end_state (void* card_id)
{
	struct wan_dev_le	*devle;
	sdla_t*		card = (sdla_t*)card_id;
	netdevice_t*	dev;
	fr_channel_t*	chan;

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}
	
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)) continue;
		chan = wan_netif_priv(dev);
		if (!chan){
			continue;
		}

		if (card->fe.fe_status == FE_CONNECTED){ 
			if (chan->dlci_stat & FR_DLCI_ACTIVE &&
			    card->wandev.state == WAN_CONNECTED &&
			    chan->common.state != WAN_CONNECTED){
				set_chan_state(dev,WAN_CONNECTED);
			}
		}else{
			if (chan->common.state != WAN_DISCONNECTED){
				if (chan->route_flag == ROUTE_ADDED) {
					chan->route_flag = REMOVE_ROUTE;
					/* The state change will trigger
					 * the fr polling routine */
				}

				if (chan->inarp == INARP_CONFIGURED) {
					chan->inarp = INARP_REQUEST;
				}

				set_chan_state(dev,WAN_DISCONNECTED);
				/* Debugging */
				WAN_DEBUG_START(card);
			}
		}
	}
}


/* SNMP
 ******************************************************************************
 *			fr_snmp_data()	
 *
 * Description: Save snmp request and parameters in private structure, enable
 *		TIMER interrupt, put current process in sleep.
 * Arguments:   
 * Returns:
 ******************************************************************************
 */
#define   FRDLCMIIFINDEX        3
#define   FRDLCMISTATE          4
#define   FRDLCMIADDRESS        5
#define   FRDLCMIADDRESSLEN     6
#define   FRDLCMIPOLLINGINTERVAL  7
#define   FRDLCMIFULLENQUIRYINTERVAL  8
#define   FRDLCMIERRORTHRESHOLD  9
#define   FRDLCMIMONITOREDEVENTS  10
#define   FRDLCMIMAXSUPPORTEDVCS  11
#define   FRDLCMIMULTICAST      12
#define   FRDLCMISTATUS         13
#define   FRDLCMIROWSTATUS      14
#define   FRCIRCUITIFINDEX      17
#define   FRCIRCUITDLCI         18
#define   FRCIRCUITSTATE        19
#define   FRCIRCUITRECEIVEDFECNS  20
#define   FRCIRCUITRECEIVEDBECNS  21
#define   FRCIRCUITSENTFRAMES   22
#define   FRCIRCUITSENTOCTETS   23
#define   FRCIRCUITRECEIVEDFRAMES  24
#define   FRCIRCUITRECEIVEDOCTETS  25
#define   FRCIRCUITCREATIONTIME  26
#define   FRCIRCUITLASTTIMECHANGE  27
#define   FRCIRCUITCOMMITTEDBURST  28
#define   FRCIRCUITEXCESSBURST  29
#define   FRCIRCUITTHROUGHPUT   30
#define   FRCIRCUITMULTICAST    31
#define   FRCIRCUITTYPE         32
#define   FRCIRCUITDISCARDS     33
#define   FRCIRCUITRECEIVEDDES  34
#define   FRCIRCUITSENTDES      35
#define   FRCIRCUITLOGICALIFINDEX  36
#define   FRCIRCUITROWSTATUS    37
#define   FRERRIFINDEX          40
#define   FRERRTYPE             41
#define   FRERRDATA             42
#define   FRERRTIME             43
#define   FRERRFAULTS           44
#define   FRERRFAULTTIME        45
#define   FRTRAPSTATE           46
#define   FRTRAPMAXRATE         47

static int fr_snmp_data(sdla_t* card, netdevice_t *dev, void* data)
{
	fr_channel_t* 	chan = NULL;
	wanpipe_snmp_t*	snmp;
	struct timeval	tv;
	
	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return -EFAULT;
	chan = (fr_channel_t*)wan_netif_priv(dev);
	/* Update device statistics */
	if (card->wandev.update) {
		int rslt = 0;
		card->u.f.update_dlci = chan;
		rslt = card->wandev.update(&card->wandev);
		if(rslt) {
			return (rslt) ? (-EBUSY) : (-EINVAL);
		}
	}

	snmp = (wanpipe_snmp_t*)data;

	switch(snmp->snmp_magic){
	/***** Data Link Connection Management Interface ****/
	case FRDLCMIIFINDEX:
		break;

	case FRDLCMISTATE:
		 snmp->snmp_val = 
		 (card->wandev.signalling == WANOPT_FR_ANSI) ? SNMP_FR_ANSIT1617D :
		 (card->wandev.signalling == WANOPT_FR_Q933) ? SNMP_FR_ITUT933A :
		 (card->wandev.signalling == WANOPT_FR_LMI) ? SNMP_FR_LMIREV:
		       						SNMP_FR_NOLMICONF;
		break;

	case FRDLCMIADDRESS:
		snmp->snmp_val = SNMP_FR_Q922;
		break;

	case FRDLCMIADDRESSLEN:
		snmp->snmp_val = SNMP_FR_4BYTE_ADDR;
		break;

	case FRDLCMIPOLLINGINTERVAL:
		snmp->snmp_val = card->u.f.t391;
		break;

	case FRDLCMIFULLENQUIRYINTERVAL:
		snmp->snmp_val = card->u.f.n391;
		break;

	case FRDLCMIERRORTHRESHOLD:
		snmp->snmp_val = card->u.f.n392;
		break;

	case FRDLCMIMONITOREDEVENTS:
		snmp->snmp_val = card->u.f.n393;
		break;

	case FRDLCMIMAXSUPPORTEDVCS:
		snmp->snmp_val = HIGHEST_VALID_DLCI;
		break;

	case FRDLCMIMULTICAST:
		snmp->snmp_val = SNMP_FR_NONBROADCAST;
		break;

	case FRDLCMISTATUS: /* FIXME */
		snmp->snmp_val = SNMP_FR_RUNNING;
		break;

	case FRDLCMIROWSTATUS: /* FIXME */
		snmp->snmp_val = 0;
		break;

	/****************** Circuit Table *******************/
	case FRCIRCUITIFINDEX: 
		break;

	case FRCIRCUITDLCI:
		snmp->snmp_val = chan->dlci;
		break;

	case FRCIRCUITSTATE:
		snmp->snmp_val = (chan->common.state == WAN_CONNECTED) ? SNMP_FR_ACTIVE : SNMP_FR_INACTIVE;
		break;

	case FRCIRCUITRECEIVEDFECNS:
		snmp->snmp_val = chan->rx_FECN;
		break;

	case FRCIRCUITRECEIVEDBECNS:
		snmp->snmp_val = chan->rx_BECN;
		break;

	case FRCIRCUITSENTFRAMES:
		snmp->snmp_val = chan->ifstats.tx_packets;
		break;

	case FRCIRCUITSENTOCTETS:
		snmp->snmp_val = chan->ifstats.tx_bytes;
		break;

	case FRCIRCUITRECEIVEDFRAMES:
		snmp->snmp_val = chan->ifstats.rx_packets;
		break;

	case FRCIRCUITRECEIVEDOCTETS:
		snmp->snmp_val = chan->ifstats.rx_bytes;
		break;

	case FRCIRCUITCREATIONTIME:
		do_gettimeofday( &tv );
		snmp->snmp_val = tv.tv_sec - chan->router_start_time;
		break;

	case FRCIRCUITLASTTIMECHANGE:
		do_gettimeofday( &tv );
		snmp->snmp_val = tv.tv_sec - chan->router_last_change;
		break;

	case FRCIRCUITCOMMITTEDBURST:
		snmp->snmp_val = (unsigned long)chan->bc;
		break;

	case FRCIRCUITEXCESSBURST:
		snmp->snmp_val = (unsigned long)chan->be;
		break;

	case FRCIRCUITTHROUGHPUT: /* FIXME */
		snmp->snmp_val = (unsigned long)0;
		break;

	case FRCIRCUITMULTICAST:
		snmp->snmp_val = (chan->mc) ? SNMP_FR_ONEWAY : SNMP_FR_UNICAST;
		break;

	case FRCIRCUITTYPE:
		snmp->snmp_val = chan->dlci_type;
		break;

	case FRCIRCUITDISCARDS:
		snmp->snmp_val = chan->ifstats.rx_errors;
		break;

	case FRCIRCUITRECEIVEDDES:
		snmp->snmp_val = chan->rx_DE_set;
		break;

	case FRCIRCUITSENTDES:
		snmp->snmp_val = chan->tx_DE_set;
		break;

	case FRCIRCUITLOGICALIFINDEX: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case FRCIRCUITROWSTATUS: /* FIXME */
		snmp->snmp_val = 0;
		break;

	/****************** Errort Table  *******************/
	case FRERRIFINDEX: 
		break;

	case FRERRTYPE: /* FIXME */
		snmp->snmp_val = chan->err_type;
		break;

	case FRERRDATA: /* FIXME */
		strcpy((void*)snmp->snmp_data, chan->err_data);
		break;

	case FRERRTIME: /* FIXME */
		snmp->snmp_val = chan->err_time;
		break;

	case FRERRFAULTS: /* FIXME */
		snmp->snmp_val = chan->err_faults;
		break;

	case FRERRFAULTTIME: /* FIXME */
		snmp->snmp_val = (unsigned long)0;
		break;

	/************ Frame Relay Trap Control **************/
	case FRTRAPSTATE:
		snmp->snmp_val = chan->trap_state;
		break;

	case FRTRAPMAXRATE:
		snmp->snmp_val = chan->trap_max_rate;
		break;

	default:
            	return -EAFNOSUPPORT;
	}

	return 0;

}

/*
****************************************************************************
**
**
**
*/
static int fr_debugging(sdla_t* card)
{

	if (card->wan_debugging_state == WAN_DEBUGGING_CONT && 
	    card->wandev.station == WANOPT_CPE){
		card->wan_debugging_state = WAN_DEBUGGING_PROTOCOL;
		/*
		** If link still down after 100 seconds (only for CPE), then
		** report error. 
		*/
		return 100;
	}

	if (card->wandev.station == WANOPT_CPE){
		if (card->wan_debug_last_msg != WAN_DEBUG_FR_CPE_MSG){
			DEBUG_EVENT("%s: Frame Relay switch not replying.\n",
						card->devname);
			DEBUG_EVENT("%s: Check ANSI/LMI/Q.933 setting with Frame Relay provider.\n",
						card->devname);
		}
		card->wan_debug_last_msg = WAN_DEBUG_FR_CPE_MSG;
	}else{
		if (card->wan_debug_last_msg != WAN_DEBUG_FR_NODE_MSG){
			DEBUG_EVENT("%s: No Signalling frames received from Frame Relay CPE!\n",
						card->devname);
		}
		card->wan_debug_last_msg = WAN_DEBUG_FR_NODE_MSG;
	} 
	return 0;
}

static unsigned long fr_crc_frames(sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	unsigned long	smp_flags;
	unsigned long	rx_bad_crc = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!fr_get_err_stats(card)){
		fr_comm_stat_t* err_stats = (void*)mb->wan_data;
		rx_bad_crc = err_stats->rx_bad_crc;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return rx_bad_crc;
}

static unsigned long fr_abort_frames(sdla_t * card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	unsigned long	smp_flags;
	unsigned long	rx_aborts = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!fr_get_err_stats(card)){
		fr_comm_stat_t* err_stats = (void*)mb->wan_data;
		rx_aborts = err_stats->rx_aborts;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return rx_aborts;
}

static unsigned long fr_tx_underun_frames(sdla_t* card)
{
	wan_mbox_t*	mb = &card->wan_mbox;
	unsigned long	smp_flags;
	unsigned long	tx_missed_undr = 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (!fr_get_err_stats(card)){
		fr_comm_stat_t* err_stats = (void*)mb->wan_data;
		tx_missed_undr = err_stats->tx_missed_undr;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return tx_missed_undr;
}



/****** End *****************************************************************/
