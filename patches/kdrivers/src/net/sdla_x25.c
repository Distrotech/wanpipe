/*****************************************************************************
* sdla_x25.c	WANPIPE(tm) Multiprotocol WAN Link Driver.  X.25 module.
*
* Author:	Nenad Corbic	<ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jul 11, 2003  Nenad Corbic	 o Update the x25 driver to interface to the new
*                                  api layer.  
* Jul 10, 2003  Nenad Corbic     o Bug fix: tx frames greater than max packet size
*                                  A duplicate frame was generated due to the
*                                  bug in setup_for_delayed_transmit() function.
*                                  Solution: Tx frames > MTX via tx interrupt.
* Jul 19, 2002  Nenad Corbic	 o Tx offset initialized in setup_for_delayed_transmit()
* 				   Caused problems if previous frame faild to send.
* Jan 15, 2002  Nenad Corbic	 o Expanded OOB message reporting to all async
*  				   messages.
* Dec 05, 2001  Nenad Corbic 	 o Bug fix: channel_disconnect() function missed
* 				   an event and marked channel inactive before
* 				   clearing call.
* Oct 10, 2001  Nenad Corbic	 o Added the dev->do_ioctl function to replace
* 				   the poll_active() function during API operation.
* 				   The API commands are not polled any more, but
* 				   executed in priority through a tx interrupt.
* Sep 20, 2001  Nenad Corbic     o The min() function has changed for 2.4.9
*                                  kernel. Thus using the wp_min() defined in
*                                  wanpipe.h
* Sept 6, 2001  Alex Feldman     o Add SNMP support.
* Apr 26, 2001  David Rokhvarg   o Added pattern matching on call accept.
* Apr 16, 2001  David Rokhvarg	 o Fixed auto call accept: user_data filed is 
* 				   check as char.
* Apr 03, 2001  Nenad Corbic	 o Fixed the rx_skb=NULL bug in x25 in rx_intr().
* Dec 26, 2000  Nenad Corbic	 o Added a new polling routine, that uses
*                                  a kernel timer (more efficient).
* Dec 25, 2000  Nenad Corbic	 o Updated for 2.4.X kernel
* Jul 26, 2000  Nenad Corbic	 o Increased the local packet buffering
* 				   for API to 4096+header_size. 
* Jul 17, 2000  Nenad Corbic	 o Fixed the x25 startup bug. Enable 
* 				   communications only after all interfaces
* 				   come up.  HIGH SVC/PVC is used to calculate
* 				   the number of channels.
*                                  Enable protocol only after all interfaces
*                                  are enabled.
* Jul 10, 2000	Nenad Corbic	 o Fixed the M_BIT bug. 
* Apr 25, 2000  Nenad Corbic	 o Pass Modem messages to the API.
*                                  Disable idle timeout in X25 API.
* Apr 14, 2000  Nenad Corbic	 o Fixed: Large LCN number support.
*                                  Maximum LCN number is 4095.
*                                  Maximum number of X25 channels is 255.
* Apr 06, 2000  Nenad Corbic	 o Added SMP Support.
* Mar 29, 2000  Nenad Corbic	 o Added support for S514 PCI Card
* Mar 23, 2000  Nenad Corbic	 o Improved task queue, BH handling.
* Mar 14, 2000  Nenad Corbic  	 o Updated Protocol Violation handling
*                                  routines.  Bug Fix.
* Mar 10, 2000  Nenad Corbic	 o Bug Fix: corrupted box recovery.
* Mar 09, 2000  Nenad Corbic     o Fixed the auto HDLC bug.
* Mar 08, 2000	Nenad Corbic     o Fixed LAPB HDLC startup problems.
*                                  Application must bring the link up 
*                                  before tx/rx, and bring the 
*                                  link down on close().
* Mar 06, 2000	Nenad Corbic	 o Added an option for logging call setup 
*                                  information. 
* Feb 29, 2000  Nenad Corbic 	 o Added support for LAPB HDLC API
* Feb 25, 2000  Nenad Corbic     o Fixed the modem failure handling.
*                                  No Modem OOB message will be passed 
*                                  to the user.
* Feb 21, 2000  Nenad Corbic 	 o Added Xpipemon Debug Support
* Dec 30, 1999 	Nenad Corbic	 o Socket based X25API 
* Sep 17, 1998	Jaspreet Singh	 o Updates for 2.2.X  kernel
* Mar 15, 1998	Alan Cox	 o 2.1.x porting
* Dec 19, 1997	Jaspreet Singh	 o Added multi-channel IPX support
* Nov 27, 1997	Jaspreet Singh	 o Added protection against enabling of irqs
*				   when they are disabled.
* Nov 17, 1997  Farhan Thawar    o Added IPX support
*				 o Changed if_send() to now buffer packets when
*				   the board is busy
*				 o Removed queueing of packets via the polling
*				   routing
*				 o Changed if_send() critical flags to properly
*				   handle race conditions
* Nov 06, 1997  Farhan Thawar    o Added support for SVC timeouts
*				 o Changed PVC encapsulation to ETH_P_IP
* Jul 21, 1997  Jaspreet Singh	 o Fixed freeing up of buffers using kfree()
*				   when packets are received.
* Mar 11, 1997  Farhan Thawar   Version 3.1.1
*                                o added support for V35
*                                o changed if_send() to return 0 if
*                                  wandev.critical() is true
*                                o free socket buffer in if_send() if
*                                  returning 0
*                                o added support for single '@' address to
*                                  accept all incoming calls
*                                o fixed bug in set_chan_state() to disconnect
* Jan 15, 1997	Gene Kozin	Version 3.1.0
*				 o implemented exec() entry point
* Jan 07, 1997	Gene Kozin	Initial version.
*****************************************************************************/

/*======================================================
 * 	Includes 
 *=====================================================*/


#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/wanpipe_abstr.h>
#include <linux/wanproc.h>
#include <linux/wanpipe_snmp.h>
#include <linux/sdla_x25.h>	/* X.25 firmware API definitions */
#include <linux/if_wanpipe_common.h>
#include <linux/if_wanpipe.h>


/*======================================================
 * 	Defines & Macros 
 *=====================================================*/


#define	CMD_OK		0		/* normal firmware return code */
#define	CMD_TIMEOUT	0xFF		/* firmware command timed out */
#define	MAX_CMD_RETRY	10		/* max number of firmware retries */

#define	X25_CHAN_MTU	4096		/* unfragmented logical channel MTU */
#define	X25_HRDHDR_SZ	7		/* max encapsulation header size */
#define	X25_CONCT_TMOUT	(90*HZ)		/* link connection timeout */
#define	X25_RECON_TMOUT	(10*HZ)		/* link connection timeout */
#define	CONNECT_TIMEOUT	(90*HZ)		/* link connection timeout */
#define	HOLD_DOWN_TIME	(30*HZ)		/* link hold down time */
#define MAX_BH_BUFF	10

#undef PRINT_DEBUG 
#ifdef PRINT_DEBUG
#define DBG_PRINTK(format, a...) printk(format, ## a)
#else
#define DBG_PRINTK(format, a...)
#endif  

#define TMR_INT_ENABLED_POLL_ACTIVE      0x01
#define TMR_INT_ENABLED_POLL_CONNECT_ON  0x02
#define TMR_INT_ENABLED_POLL_CONNECT_OFF 0x04
#define TMR_INT_ENABLED_POLL_DISCONNECT  0x08
#define TMR_INT_ENABLED_CMD_EXEC	 0x10
#define TMR_INT_ENABLED_UPDATE		 0x20
#define TMR_INT_ENABLED_UDP_PKT		 0x40

#define MAX_X25_ADDR_SIZE	16
#define MAX_X25_DATA_SIZE 	129
#define MAX_X25_FACL_SIZE	110

#define TRY_CMD_AGAIN	2
#define DELAY_RESULT    1
#define RETURN_RESULT   0

#define DCD(x) (x & X25_DCD_MASK ? "Up" : "Down")
#define CTS(x) (x & X25_CTS_MASK ? "Up" : "Down")

#define MAX_X25_TRACE_QUEUE 100

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

/* Private critical flags */
enum { 
	POLL_CRIT = PRIV_CRIT 
};

/* Driver will not write log messages about 
 * modem status if defined.*/
#undef MODEM_NOT_LOG

/*==================================================== 
 * 	For IPXWAN 
 *===================================================*/

#define CVHexToAscii(b) (((unsigned char)(b) > (unsigned char)9) ? ((unsigned char)'A' + ((unsigned char)(b) - (unsigned char)10)) : ((unsigned char)'0' + (unsigned char)(b)))


/*====================================================
 *           MEMORY DEBUGGING FUNCTION
 *====================================================

#define KMEM_SAFETYZONE 8

static void * dbg_kmalloc(unsigned int size, int prio, int line) {
	int i = 0;
	void * v = kmalloc(size+sizeof(unsigned int)+2*KMEM_SAFETYZONE*8,prio);
	char * c1 = v;	
	c1 += sizeof(unsigned int);
	*((unsigned int *)v) = size;

	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		c1[0] = 'D'; c1[1] = 'E'; c1[2] = 'A'; c1[3] = 'D';
		c1[4] = 'B'; c1[5] = 'E'; c1[6] = 'E'; c1[7] = 'F';
		c1 += 8;
	}
	c1 += size;
	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		c1[0] = 'M'; c1[1] = 'U'; c1[2] = 'N'; c1[3] = 'G';
		c1[4] = 'W'; c1[5] = 'A'; c1[6] = 'L'; c1[7] = 'L';
		c1 += 8;
	}
	v = ((char *)v) + sizeof(unsigned int) + KMEM_SAFETYZONE*8;
	printk(KERN_INFO "line %d  kmalloc(%d,%d) = %p\n",line,size,prio,v);
	return v;
}
static void dbg_kfree(void * v, int line) {
	unsigned int * sp = (unsigned int *)(((char *)v) - (sizeof(unsigned int) + KMEM_SAFETYZONE*8));
	unsigned int size = *sp;
	char * c1 = ((char *)v) - KMEM_SAFETYZONE*8;
	int i = 0;
	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		if (   c1[0] != 'D' || c1[1] != 'E' || c1[2] != 'A' || c1[3] != 'D'
		    || c1[4] != 'B' || c1[5] != 'E' || c1[6] != 'E' || c1[7] != 'F') {
			printk(KERN_INFO "kmalloced block at %p has been corrupted (underrun)!\n",v);
			printk(KERN_INFO " %4x: %2x %2x %2x %2x %2x %2x %2x %2x\n", i*8,
			                c1[0],c1[1],c1[2],c1[3],c1[4],c1[5],c1[6],c1[7] );
		}
		c1 += 8;
	}
	c1 += size;
	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		if (   c1[0] != 'M' || c1[1] != 'U' || c1[2] != 'N' || c1[3] != 'G'
		    || c1[4] != 'W' || c1[5] != 'A' || c1[6] != 'L' || c1[7] != 'L'
		   ) {
			printk(KERN_INFO "kmalloced block at %p has been corrupted (overrun):\n",v);
			printk(KERN_INFO " %4x: %2x %2x %2x %2x %2x %2x %2x %2x\n", i*8,
			                c1[0],c1[1],c1[2],c1[3],c1[4],c1[5],c1[6],c1[7] );
		}
		c1 += 8;
	}
	printk(KERN_INFO "line %d  kfree(%p)\n",line,v);
	v = ((char *)v) - (sizeof(unsigned int) + KMEM_SAFETYZONE*8);
	kfree(v);
}

#define kmalloc(x,y) dbg_kmalloc(x,y,__LINE__)
#define kfree(x) dbg_kfree(x,__LINE__)

==============================================================*/



/*===============================================
 * 	Data Structures 
 *===============================================*/


/*========================================================
 * Name: 	x25_channel
 *
 * Purpose:	To hold private informaton for each  
 *              logical channel.
 *		
 * Rationale:  	Per-channel debugging is possible if each 
 *              channel has its own private area.
 *	
 * Assumptions:
 *
 * Description:	This is an extention of the 'netdevice_t' 
 *              we create for each network interface to keep 
 *              the rest of X.25 channel-specific data. 
 *
 * Construct:	Typedef
 */
typedef struct x25_channel
{
	wanpipe_common_t common;	/* common area for x25api and socket */
	char name[WAN_IFNAME_SZ+1];	/* interface name, ASCIIZ */
	char addr[WAN_ADDRESS_SZ+1];	/* media address, ASCIIZ */
	unsigned tx_pkt_size;
	unsigned short protocol;	/* ethertype, 0 - multiplexed */
	char drop_sequence;		/* mark sequence for dropping */
	unsigned long state_tick;	/* time of the last state change */
	unsigned idle_timeout;		/* sec, before disconnecting */
	unsigned long i_timeout_sofar;  /* # of sec's we've been idle */
	unsigned hold_timeout;		/* sec, before re-connecting */
	unsigned long tick_counter;	/* counter for transmit time out */
	char devtint;			/* Weather we should dev_tint() */
	struct sk_buff* rx_skb;		/* receive socket buffer */
	struct sk_buff* tx_skb;		/* transmit socket buffer */

	sdla_t* card;			/* -> owner */

	int ch_idx;
	unsigned char enable_IPX;
	unsigned long network_number;
	struct net_device_stats ifstats;	/* interface statistics */

	unsigned short transmit_length;
	unsigned short tx_offset;
	char transmit_buffer[X25_CHAN_MTU+sizeof(x25api_hdr_t)];

	if_send_stat_t   if_send_stat;
        rx_intr_stat_t   rx_intr_stat;
        pipe_mgmt_stat_t pipe_mgmt_stat;    

	unsigned long router_start_time; /* Router start time in seconds */
	unsigned long router_up_time;

	char x25_src_addr[WAN_ADDRESS_SZ+1];	   /* x25 media source address, 
						      ASCIIZ */
	char accept_dest_addr[WAN_ADDRESS_SZ+1];   /* pattern match string in -d<string>
						      for accepting calls, ASCIIZ */	
	char accept_src_addr[WAN_ADDRESS_SZ+1];    /* pattern match string in -s<string> 
						      for accepting calls, ASCIIZ */
	char accept_usr_data[WAN_ADDRESS_SZ+1];/* pattern match string in -u<string> 
						      for accepting calls, ASCIIZ */		

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;
	int			chan_direct;
	unsigned long 		chan_establ_time;
	unsigned long 		chan_clear_time;
	unsigned char 		chan_clear_cause;
	unsigned char 		chan_clear_diagn;
	char 			cleared_called_addr[MAX_X25_ADDR_SIZE];	
	char 			cleared_calling_addr[MAX_X25_ADDR_SIZE];
	char 			cleared_facil[MAX_X25_ADDR_SIZE];

	unsigned char		critical;
	
	unsigned char		call_string[X25_CALL_STR_SZ+1];
	x25api_t 		x25_api_event;
	x25api_hdr_t 		x25_api_cmd_hdr;
	x25api_hdr_t 		x25_api_tx_hdr;

	unsigned char label  	[WAN_IF_LABEL_SZ+1];
	unsigned char dlabel  	[WAN_IF_LABEL_SZ+1];

	unsigned char		used;
	pid_t			pid;
	unsigned long		api_state;
	spinlock_t		lock;
	atomic_t		cmd_rc;
	unsigned long		cmd_timeout;

} x25_channel_t;

/* FIXME Take this out */

#pragma pack(1)
#ifdef NEX_OLD_CALL_INFO
typedef struct x25_call_info
{
	char dest[17];			;/* ASCIIZ destination address */
	char src[17];			;/* ASCIIZ source address */
	char nuser;			;/* number of user data bytes */
	unsigned char user[127];	;/* user data */
	char nfacil;			;/* number of facilities */
	struct
	{
		unsigned char code;     ;
		unsigned char parm;     ;
	} facil[64];			        /* facilities */
} x25_call_info_t;
#else
typedef struct x25_call_info
{
	char dest[MAX_X25_ADDR_SIZE]		;/* ASCIIZ destination address */
	char src[MAX_X25_ADDR_SIZE]		;/* ASCIIZ source address */
	unsigned char nuser			;
	unsigned char user[MAX_X25_DATA_SIZE]	;/* user data */
	unsigned char nfacil			;
	unsigned char facil[MAX_X25_FACL_SIZE]	;
	unsigned short lcn             		;
} x25_call_info_t;
#endif
#pragma pack()


  
/*===============================================
 *	Private Function Prototypes
 *==============================================*/


/*================================================= 
 * WAN link driver entry points. These are 
 * called by the WAN router module.
 */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);
static int del_if (wan_device_t* wandev, netdevice_t* dev);
static void disable_comm (sdla_t* card);
static void disable_comm_shutdown(sdla_t *card);



/*================================================= 
 *	WANPIPE-specific entry points 
 */
static void x25api_bh (unsigned long data);
static void wakeup_sk_bh (netdevice_t *dev);


/*=================================================  
 * 	Network device interface 
 */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_send (struct sk_buff* skb, netdevice_t* dev);
static struct net_device_stats *if_stats (netdevice_t* dev);

static void if_tx_timeout (netdevice_t *dev);

/*=================================================  
 * 	Interrupt handlers 
 */
static WAN_IRQ_RETVAL wpx_isr	(sdla_t *);
static void rx_intr	(sdla_t *);
static void tx_intr	(sdla_t *);
static void status_intr	(sdla_t *);
static void event_intr	(sdla_t *);
static void spur_intr	(sdla_t *);
static void timer_intr  (sdla_t *);
static void trace_intr  (sdla_t *);


static int tx_intr_send(sdla_t *, netdevice_t *);
static netdevice_t * move_dev_to_next (sdla_t *, netdevice_t *);

/*=================================================  
 *	Background polling routines 
 */
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void wpx_poll (void *card_ptr);
#else
static void wpx_poll (struct work_struct *work);
#endif
static void poll_disconnected (sdla_t* card);
static void poll_connecting (sdla_t* card);
static void poll_active (sdla_t* card);
static void trigger_x25_poll(sdla_t *card);
static void x25_timer_routine(unsigned long data);



/*=================================================  
 *	X.25 firmware interface functions 
 */
static int x25_get_version (sdla_t* card, char* str);
static int x25_configure (sdla_t* card, TX25Config* conf);
static int hdlc_configure (sdla_t* card, TX25Config* conf);
static int set_hdlc_level (sdla_t* card);
static int x25_get_err_stats (sdla_t* card);
static int x25_get_stats (sdla_t* card);
static int x25_set_intr_mode (sdla_t* card, int mode);
static int x25_close_hdlc (sdla_t* card);
static int x25_open_hdlc (sdla_t* card);
static int x25_setup_hdlc (sdla_t* card);
static int x25_set_dtr (sdla_t* card, int dtr);
static int x25_get_chan_conf (sdla_t* card, x25_channel_t* chan);
static int x25_place_call (sdla_t* card, x25_channel_t* chan);
static int x25_accept_call (sdla_t* card, int lcn, int qdm, unsigned char *data, int len);
static int x25_interrupt (sdla_t* card, int lcn, int qdm, unsigned char *data, int len);
static int x25_clear_call (sdla_t* card, int lcn, int cause, int diagn, int qdm, unsigned char *data, int len);
static int x25_reset_call (sdla_t* card, int lcn, int cause, int diagn, int qdm);
static int x25_send (sdla_t* card, int lcn, int qdm, int len, void* buf);
static int x25_fetch_events (sdla_t* card);
static int x25_error (sdla_t* card, int err, int cmd, int lcn);
static int is_match_found(unsigned char info[], unsigned char chan[]);

/*=================================================  
 *	X.25 asynchronous event handlers 
 */
static int incoming_call (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int call_accepted (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int call_cleared (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int timeout_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int restart_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int x25_down_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb, unsigned char);
 



/*=================================================  
 *	Miscellaneous functions 
 */
static int connect (sdla_t* card);
static int disconnect (sdla_t* card);
static netdevice_t* get_dev_by_lcn(wan_device_t* wandev, unsigned lcn);
static int chan_connect (netdevice_t* dev);
static int chan_disc (netdevice_t* dev);
static void set_chan_state (netdevice_t* dev, int state);
static int chan_send (netdevice_t* , void* , unsigned, unsigned char);
static unsigned char bps_to_speed_code (unsigned long bps);
static unsigned int dec_to_uint (unsigned char* str, int len);
static unsigned int hex_to_uint (unsigned char*, int);
static void parse_call_info (unsigned char*, x25_call_info_t*);
static netdevice_t * find_channel(sdla_t *, unsigned);
static void bind_lcn_to_dev (sdla_t *, netdevice_t *,unsigned);
static void setup_for_delayed_transmit (netdevice_t*, void*, unsigned);
static int baud_rate_check (int baud);
static int lapb_connect(sdla_t *card, x25_channel_t *chan);

/*=================================================  
 *      X25 API Functions 
 */
static int wanpipe_pull_data_in_skb (sdla_t *, netdevice_t *, struct sk_buff **);
static void trigger_intr_exec(sdla_t *, unsigned char);
static int execute_delayed_cmd (sdla_t*, netdevice_t *, char);
static int api_incoming_call (sdla_t*, wan_mbox_t *, int);
static int alloc_and_init_skb_buf (sdla_t *,struct sk_buff **, int);
static int clear_confirm_event (sdla_t *, wan_mbox_t*);
static int send_oob_msg (sdla_t *, netdevice_t *, wan_mbox_t *);
static int tx_intr_cmd_exec(sdla_t *card);
static void api_oob_event (sdla_t *card,wan_mbox_t *mbox);
static int check_bad_command (sdla_t *, netdevice_t *);
#if 0
static void hdlc_link_down (sdla_t*);
#endif

/*=================================================
 *     XPIPEMON Functions
 */
static int process_udp_mgmt_pkt(sdla_t *,netdevice_t*);
static int udp_pkt_type( struct sk_buff *, sdla_t*);
static int reply_udp( unsigned char *, unsigned int); 
static void init_x25_channel_struct( x25_channel_t *);
static void init_global_statistics( sdla_t *);
static int store_udp_mgmt_pkt(int, char, sdla_t*, netdevice_t *, struct sk_buff *, int);
static unsigned short calc_checksum (char *, int);



/*================================================= 
 *	IPX functions 
 */
static void switch_net_numbers(unsigned char *, unsigned long, unsigned char);
static int handle_IPXWAN(unsigned char *, char *, unsigned char , 
			 unsigned long , unsigned short );

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

static void S508_S514_lock(sdla_t *, unsigned long *);
static void S508_S514_unlock(sdla_t *, unsigned long *);

/*================================================= 
 *	PROC fs functions 
 */
static void x25_clear_cmd_update(sdla_t* card, unsigned lcn, wan_mbox_t* mb);

static int x25_get_config_info(void* priv, struct seq_file* m, int* stop_cnt);
static int x25_get_status_info(void* priv, struct seq_file* m, int* stop_cnt);
static int x25_set_dev_config(struct file*, const char*, unsigned long, void *);
static int x25_set_if_info(struct file*, const char*, unsigned long, void *);

/* SNMP */
static int x25_snmp_data(sdla_t* card, netdevice_t *dev, void* data);

static int bind_api_to_svc(sdla_t *card, void *sk_id);
static int x25_ioctl (struct net_device *dev, struct ifreq *ifr, int cmd);
static int x25_api_cmd_setup(x25_channel_t *chan, struct ifreq *ifr);
static int x25_api_setup_cmd_hdr(x25_channel_t *chan, struct ifreq *ifr);
static int x25_api_setup_call_string(x25_channel_t *chan, struct ifreq *ifr);
static int x25_api_get_cmd_result(x25_channel_t *chan, struct ifreq *ifr);
static void x25_update_api_state(sdla_t *card,x25_channel_t *chan);
static netdevice_t *find_free_svc_dev(sdla_t *card);
static void release_svc_dev(sdla_t *card, netdevice_t *dev);
static int wait_for_cmd_rc(sdla_t *card, x25_channel_t* chan);
static int test_chan_command_busy(sdla_t *card,x25_channel_t *chan);



/*=================================================  
 * 	Global Variables 
 *=================================================*/
static TX25Stats	X25Stats;



/*================================================= 
 *	Public Functions 
 *=================================================*/




/*===================================================================
 * wpx_init:	X.25 Protocol Initialization routine.
 *
 * Purpose:	To initialize the protocol/firmware.
 * 
 * Rationale:	This function is called by setup() function, in
 *              sdlamain.c, to dynamically setup the x25 protocol.
 *		This is the first protocol specific function, which
 *              executes once on startup.
 *                
 * Description:	This procedure initializes the x25 firmware and
 *    		sets up the mailbox, transmit and receive buffer
 *              pointers. It also initializes all debugging structures
 *              and sets up the X25 environment.
 *
 *		Sets up hardware options defined by user in [wanpipe#] 
 *		section of wanpipe#.conf configuration file. 
 *
 * 		At this point adapter is completely initialized 
 *      	and X.25 firmware is running.
 *  		o read firmware version (to make sure it's alive)
 *  		o configure adapter
 *  		o initialize protocol-specific fields of the 
 *                adapter data space.
 *
 * Called by:	setup() function in sdlamain.c
 *
 * Assumptions:	None
 *
 * Warnings:	None
 *
 * Return: 	0	o.k.
 *	 	< 0	failure.
 */

int wpx_init (sdla_t* card, wandev_conf_t* conf)
{
	int err;
	union{
		char str[80];
		TX25Config cfg;
	} u;
	wan_x25_conf_t*	x25_adm_conf = &card->u.x.x25_adm_conf;
	wan_x25_conf_t*	x25_conf = &card->u.x.x25_conf;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_X25){
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
			card->devname, conf->config_id)
		;
		return -EINVAL;
	}

	/* Initialize protocol-specific fields */
	/* Alex Apr 8 2004 Sangoma ISA card */
	card->mbox_off  = X25_MB_VECTOR + X25_MBOX_OFFS;
	card->rxmb_off  = X25_MB_VECTOR + X25_RXMBOX_OFFS;
	card->flags_off = X25_MB_VECTOR + X25_STATUS_OFFS;

	card->intr_type_off = card->flags_off + offsetof(TX25Status, iflags);
	card->intr_perm_off = card->flags_off + offsetof(TX25Status, imask);
	
	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */
	if (x25_get_version(card, NULL) || x25_get_version(card, u.str))
		return -EIO;


	/* X25 firmware can run ether in X25 or LAPB HDLC mode.
         * Check the user defined option and configure accordingly */
	if (conf->u.x25.LAPB_hdlc_only == WANOPT_YES){
		if (set_hdlc_level(card) != CMD_OK){
			return -EIO;	
		}else{
			printk(KERN_INFO "%s: running LAP_B HDLC firmware v%s\n",
				card->devname, u.str);
		}
		card->u.x.LAPB_hdlc = 1;
	}else{
		printk(KERN_INFO "%s: running X.25 firmware v%s\n",
				card->devname, u.str);
		card->u.x.LAPB_hdlc = 0;
	}

	/* Copy Admin configuration to sdla_t structure */
	memcpy(x25_adm_conf, &conf->u.x25, sizeof(wan_x25_conf_t));

	if (!x25_adm_conf->cmd_retry_timeout){
		x25_adm_conf->cmd_retry_timeout=5;
	}

	DEBUG_EVENT("%s: X25 cmd retry timeout = %i sec\n",
			card->devname,
			x25_adm_conf->cmd_retry_timeout);

	card->wandev.ignore_front_end_status = conf->ignore_front_end_status;

	if (card->wandev.ignore_front_end_status == WANOPT_NO){
		printk(KERN_INFO 
		  "%s: Enabling front end link monitor\n",
				card->devname);
	}else{
		printk(KERN_INFO 
		"%s: Disabling front end link monitor\n",
				card->devname);
	}
	
	/* Configure adapter. Here we set resonable defaults, then parse
	 * device configuration structure and set configuration options.
	 * Most configuration options are verified and corrected (if
	 * necessary) since we can't rely on the adapter to do so.
	 */
	memset(&u.cfg, 0, sizeof(u.cfg));
	u.cfg.t1		= 3;
	u.cfg.n2		= 10;
	u.cfg.autoHdlc		= 1;		/* automatic HDLC connection */
	u.cfg.hdlcWindow	= 7;
	u.cfg.pktWindow		= X25_PACKET_WINDOW;
	u.cfg.station		= X25_STATION_DTE;		/* DTE */
	u.cfg.options		= 0x0090;	/* disable D-bit pragmatics */
	u.cfg.ccittCompat	= 1988;
	u.cfg.t10t20		= 30;
	u.cfg.t11t21		= 30;
	u.cfg.t12t22		= 30;
	u.cfg.t13t23		= 30;
	u.cfg.t16t26		= 30;
	u.cfg.t28		= 30;
	u.cfg.r10r20		= 5;
	u.cfg.r12r22		= 5;
	u.cfg.r13r23		= 5;
	u.cfg.responseOpt	= 1;		/* RR's after every packet */

	
	if (conf->u.x25.x25_conf_opt){
		u.cfg.options = conf->u.x25.x25_conf_opt;
	}

	if (conf->clocking != WANOPT_EXTERNAL)
		u.cfg.baudRate = bps_to_speed_code(conf->bps);

	if (conf->u.x25.station != WANOPT_DTE){
		u.cfg.station = 0;		/* DCE mode */
	}

        if (conf->electrical_interface != WANOPT_RS232 ){
	        u.cfg.hdlcOptions |= 0x80;      /* V35 mode */
	} 

	card->wandev.mtu = conf->mtu;

	if (!conf->u.x25.defPktSize){
		conf->u.x25.defPktSize = 1024;
		DEBUG_EVENT("%s: Defaulting X25 Def Pkt Size to 1024!\n",
				card->devname);
	}
	
	err=baud_rate_check(conf->u.x25.defPktSize);
	if (err!=0){
		DEBUG_EVENT("%s: Invalid Def Pkt Size %i\n",
				card->devname,conf->u.x25.defPktSize);
		return err;
	}

	if (!conf->u.x25.pktMTU){
		conf->u.x25.pktMTU = 1024;
		DEBUG_EVENT("%s: Defaulting X25 Max Pkt Size to 1024!\n",
				card->devname);
	}
	
	err=baud_rate_check(conf->u.x25.pktMTU);
	if (err!=0){
		DEBUG_EVENT("%s: Invalid Def Pkt Size %i\n",
				card->devname,conf->u.x25.defPktSize);
		return err;
	}


	if (conf->u.x25.pktMTU > card->wandev.mtu){
		DEBUG_EVENT("%s: Invalid IP MTU %i: Lower than X25 Max Pkt Size %i!\n",
				card->devname,card->wandev.mtu,conf->u.x25.pktMTU);
		return -EINVAL;
	}

	u.cfg.pktMTU = conf->u.x25.pktMTU;
	u.cfg.defPktSize = conf->u.x25.defPktSize;

	if (!card->u.x.LAPB_hdlc){
		DEBUG_EVENT("%s: Configuring: X25/LapB \n",card->devname);
		DEBUG_EVENT("%s:     X25  Station    = %s\n",
				card->devname,X25_STATION_DECODE(conf->u.x25.station)); 
		DEBUG_EVENT("%s:     X25  DefPktSize = %d\n",
				card->devname,conf->u.x25.defPktSize); 
		DEBUG_EVENT("%s:     X25  MaxPktSize = %d\n",
				card->devname,conf->u.x25.pktMTU);
		DEBUG_EVENT("%s:     X25  Window     = %d\n",
				card->devname,conf->u.x25.pkt_window);
		DEBUG_EVENT("%s:     LapB Window     = %d\n",
				card->devname,conf->u.x25.hdlc_window);
	}
	
	if (card->u.x.LAPB_hdlc){
		DEBUG_EVENT("%s: Configuring: LapB \n",card->devname);
		DEBUG_EVENT("%s:     LapB Station    = %s\n",
				card->devname,X25_STATION_DECODE(conf->u.x25.station)); 
		DEBUG_EVENT("%s:     LapB Pkt Size   = %d\n",
				card->devname,u.cfg.defPktSize);
		DEBUG_EVENT("%s:     LapB Window     = %d\n",
				card->devname,conf->u.x25.hdlc_window);
		u.cfg.hdlcMTU = u.cfg.defPktSize;
	}
	
	if (conf->u.x25.hi_pvc){
		card->u.x.x25_conf.hi_pvc = 
			wp_min(conf->u.x25.hi_pvc, MAX_LCN_NUM);
		card->u.x.x25_conf.lo_pvc = 
			wp_min(conf->u.x25.lo_pvc, card->u.x.x25_conf.hi_pvc);
	}

	if (conf->u.x25.hi_svc){
		card->u.x.x25_conf.hi_svc = 
			wp_min(conf->u.x25.hi_svc, MAX_LCN_NUM);
		card->u.x.x25_conf.lo_svc = 
			wp_min(conf->u.x25.lo_svc, card->u.x.x25_conf.hi_svc);
	}

	/* Figure out the total number of channels to configure */
	card->u.x.num_of_ch = 0;
	if (card->u.x.x25_conf.hi_svc != 0){
		card->u.x.num_of_ch = 
			(card->u.x.x25_conf.hi_svc - card->u.x.x25_conf.lo_svc) + 1;
	}
	if (card->u.x.x25_conf.hi_pvc != 0){
		card->u.x.num_of_ch += 
			(card->u.x.x25_conf.hi_pvc - card->u.x.x25_conf.lo_pvc) + 1;
	}

	if (card->u.x.num_of_ch == 0){
		printk(KERN_INFO "%s: ERROR, Minimum number of PVC/SVC channels is 1 !\n"
				 "%s: Please set the Lowest/Highest PVC/SVC values !\n",
				 card->devname,card->devname);
		return -ECHRNG;
	}
	
	u.cfg.loPVC = card->u.x.x25_conf.lo_pvc;
	u.cfg.hiPVC = card->u.x.x25_conf.hi_pvc;
	u.cfg.loTwoWaySVC = card->u.x.x25_conf.lo_svc;
	u.cfg.hiTwoWaySVC = card->u.x.x25_conf.hi_svc;

	if (conf->u.x25.hdlc_window)
		u.cfg.hdlcWindow = wp_min(conf->u.x25.hdlc_window, 7);
	if (conf->u.x25.pkt_window)
		u.cfg.pktWindow = wp_min(conf->u.x25.pkt_window, 7);

	if (conf->u.x25.t1)
		u.cfg.t1 = wp_min(conf->u.x25.t1, 30);
	if (conf->u.x25.t2)
		u.cfg.t2 = wp_min(conf->u.x25.t2, 29);
	if (conf->u.x25.t4)
		u.cfg.t4 = wp_min(conf->u.x25.t4, 240);
	if (conf->u.x25.n2)
		u.cfg.n2 = wp_min(conf->u.x25.n2, 30);

	if (conf->u.x25.t10_t20)
		u.cfg.t10t20 = wp_min(conf->u.x25.t10_t20,255);
	if (conf->u.x25.t11_t21)
		u.cfg.t11t21 = wp_min(conf->u.x25.t11_t21,255);
	if (conf->u.x25.t12_t22)
		u.cfg.t12t22 = wp_min(conf->u.x25.t12_t22,255);
	if (conf->u.x25.t13_t23)	
		u.cfg.t13t23 = wp_min(conf->u.x25.t13_t23,255);
	if (conf->u.x25.t16_t26)
		u.cfg.t16t26 = wp_min(conf->u.x25.t16_t26, 255);
	if (conf->u.x25.t28)
		u.cfg.t28 = wp_min(conf->u.x25.t28, 255);

	if (conf->u.x25.r10_r20)
		u.cfg.r10r20 = wp_min(conf->u.x25.r10_r20,250);
	if (conf->u.x25.r12_r22)
		u.cfg.r12r22 = wp_min(conf->u.x25.r12_r22,250);
	if (conf->u.x25.r13_r23)
		u.cfg.r13r23 = wp_min(conf->u.x25.r13_r23,250);


	if (conf->u.x25.ccitt_compat)
		u.cfg.ccittCompat = conf->u.x25.ccitt_compat;

	DEBUG_EVENT("%s:     Low  PVC        = %d\n",
				card->devname,u.cfg.loPVC);
	DEBUG_EVENT("%s:     High PVC        = %d\n",
				card->devname,u.cfg.hiPVC);
	DEBUG_EVENT("%s:     Low  SVC        = %d\n",
				card->devname,u.cfg.loTwoWaySVC);
	DEBUG_EVENT("%s:     High SVC        = %d\n",
				card->devname,u.cfg.hiTwoWaySVC);
	DEBUG_EVENT("%s:     Num of Channels = %d\n",
				card->devname,card->u.x.num_of_ch);
	
	/* Copy the real configuration to card structure */
	x25_conf->lo_pvc 	= u.cfg.loPVC;
	x25_conf->hi_pvc 	= u.cfg.hiPVC;
	x25_conf->lo_svc 	= u.cfg.loTwoWaySVC;
	x25_conf->hi_svc 	= u.cfg.hiTwoWaySVC;
	x25_conf->hdlc_window 	= u.cfg.hdlcWindow;
	x25_conf->pkt_window 	= u.cfg.pktWindow;
	x25_conf->t1 		= u.cfg.t1;
	x25_conf->t2 		= u.cfg.t2;
	x25_conf->t4 		= u.cfg.t4;
	x25_conf->n2 		= u.cfg.n2;
	x25_conf->t10_t20 	= u.cfg.t10t20;
	x25_conf->t11_t21 	= u.cfg.t11t21;
	x25_conf->t12_t22 	= u.cfg.t12t22;
	x25_conf->t13_t23 	= u.cfg.t13t23;
	x25_conf->t16_t26 	= u.cfg.t16t26;
	x25_conf->t28 		= u.cfg.t28;
	x25_conf->r10_r20 	= u.cfg.r10r20;
	x25_conf->r12_r22 	= u.cfg.r12r22;
	x25_conf->r13_r23	= u.cfg.r13r23;
	x25_conf->ccitt_compat 	= u.cfg.ccittCompat;
	x25_conf->x25_conf_opt 	= u.cfg.options;
	x25_conf->oob_on_modem 	= conf->u.x25.oob_on_modem;
	x25_conf->pktMTU	= u.cfg.pktMTU;
	x25_conf->defPktSize    = u.cfg.defPktSize;
	x25_conf->cmd_retry_timeout = x25_adm_conf->cmd_retry_timeout;
	

	/* initialize adapter */
	if (card->u.x.LAPB_hdlc){
		if (hdlc_configure(card, &u.cfg) != CMD_OK)
			return -EIO;
	}else{
		if (x25_configure(card, &u.cfg) != CMD_OK)
			return -EIO;
	}

	if ((x25_close_hdlc(card) != CMD_OK) ||		/* close HDLC link */
	    (x25_set_dtr(card, 0) != CMD_OK))		/* drop DTR */
		return -EIO;

	/* Initialize protocol-specific fields of adapter data space */
	card->wandev.bps	= conf->bps;
	card->wandev.electrical_interface	= conf->electrical_interface;
	card->wandev.clocking	= conf->clocking;
	card->wandev.station	= conf->u.x25.station;
	card->isr		= &wpx_isr;
	card->poll		= NULL;
	card->disable_comm	= &disable_comm;
	card->exec		= NULL;
	card->wandev.update	= &update;
	card->wandev.new_if	= &new_if;
	card->wandev.del_if	= &del_if;


	card->bind_api_to_svc	= &bind_api_to_svc;

	// Proc fs functions
	card->wandev.get_config_info 	= &x25_get_config_info;
	card->wandev.get_status_info 	= &x25_get_status_info;
	card->wandev.set_dev_config    	= &x25_set_dev_config;
	card->wandev.set_if_info     	= &x25_set_if_info;

	/* SNMP data */
	card->get_snmp_data     	= &x25_snmp_data;


	/* WARNING: This function cannot exit with an error
	 *          after the change of state */
	card->wandev.state	= WAN_DISCONNECTED;
	
	card->wandev.enable_tx_int = 0;
	card->irq_dis_if_send_count = 0;
        card->irq_dis_poll_count = 0;
	card->u.x.tx_dev = NULL;
	card->u.x.no_dev = 0;


	/* Configure for S514 PCI Card */
	/* Alex Apr 8 2004 Sangoma ISA card */
	card->u.x.hdlc_buf_status_off = 
		X25_MB_VECTOR + X25_MISC_HDLC_BITS;

	card->u.x.poll_device=NULL;
	card->wandev.udp_port = conf->udp_port;

	/* Enable or disable call setup logging */
	if (conf->u.x25.logging == WANOPT_YES){
		printk(KERN_INFO "%s: Enabling Call Logging.\n",
			card->devname);
		card->u.x.logging = 1;
	}else{	
		card->u.x.logging = 0;
	}

	/* Enable or disable modem status reporting */
	if (conf->u.x25.oob_on_modem == WANOPT_YES){
		printk(KERN_INFO "%s: Enabling OOB on Modem change.\n",
			card->devname);
		card->u.x.oob_on_modem = 1;
	}else{
		card->u.x.oob_on_modem = 0;
	}
	
	init_global_statistics(card);	


	WAN_TASKQ_INIT((&card->u.x.x25_poll_task),0,wpx_poll,card);

	init_timer(&card->u.x.x25_timer);
	card->u.x.x25_timer.data = (unsigned long)card;
	card->u.x.x25_timer.function = x25_timer_routine;

	skb_queue_head_init(&card->u.x.trace_queue);

	return 0;
}

/*=========================================================
 *	WAN Device Driver Entry Points 
 *========================================================*/

/*============================================================
 * Name:	update(),  Update device status & statistics.
 *
 * Purpose:	To provide debugging and statitical
 *              information to the /proc file system.
 *              /proc/net/wanrouter/wanpipe#
 *              	
 * Rationale:	The /proc file system is used to collect
 *              information about the kernel and drivers.
 *              Using the /proc file system the user
 *              can see exactly what the sangoma drivers are
 *              doing. And in what state they are in. 
 *                
 * Description: Collect all driver statistical information
 *              and pass it to the top laywer. 
 *		
 *		Since we have to execute a debugging command, 
 *              to obtain firmware statitics, we trigger a 
 *              UPDATE function within the timer interrtup.
 *              We wait until the timer update is complete.
 *              Once complete return the appropriate return
 *              code to indicate that the update was successful.
 *              
 * Called by:	device_stat() in wanmain.c
 *
 * Assumptions:	
 *
 * Warnings:	This function will degrade the performance
 *              of the router, since it uses the mailbox. 
 *
 * Return: 	0 	OK
 * 		<0	Failed (or busy).
 */

static int update (wan_device_t* wandev)
{
	netdevice_t	*dev;
	sdla_t* card;
	unsigned long smp_flags;
	u8	status;
	int err=0;
	
	/* sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;

	if (wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	if (test_bit(SEND_CRIT, (void*)&wandev->critical))
		return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&wandev->dev_head));
	if (dev == NULL)
		return -ENODEV;
	
	card = wandev->priv;
	
	spin_lock_irqsave(&card->wandev.lock, smp_flags);

	card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &status, 1);
	if (status & 0x40){
		x25_get_err_stats(card);
		x25_get_stats(card);
	}else{
		err=-EBUSY;
	}

	spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

	return err;
}


/*===================================================================
 * Name:	new_if
 *
 * Purpose:	To allocate and initialize resources for a 
 *              new logical channel.  
 * 
 * Rationale:	A new channel can be added dynamically via
 *              ioctl call.
 *                
 * Description:	Allocate a private channel structure, x25_channel_t.
 *		Parse the user interface options from wanpipe#.conf 
 *		configuration file. 
 *		Bind the private are into the network device private
 *              area pointer (dev->priv).
 *		Prepare the network device structure for registration.
 *
 * Called by:	ROUTER_IFNEW Ioctl call, from wanrouter_ioctl() 
 *              (wanmain.c)
 *
 * Assumptions: None
 *
 * Warnings:	None
 *
 * Return: 	0 	Ok
 *		<0 	Failed (channel will not be created)
 */
static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	sdla_t* card = wandev->priv;
	x25_channel_t* chan;
	int err = 0;

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)){
		printk(KERN_INFO "%s: invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	if(card->wandev.new_if_cnt++ > 0 && card->u.x.LAPB_hdlc) {
		printk(KERN_INFO "%s: Error: Running LAPB HDLC Mode !\n",
						card->devname);
		printk(KERN_INFO 
			"%s: Maximum number of network interfaces must be one !\n",
						card->devname);
		return -EEXIST;
	}

	/* allocate and initialize private data */
	chan = kmalloc(sizeof(x25_channel_t), GFP_ATOMIC);
	if (chan == NULL){
		printk(KERN_INFO "%s: Error: No memory !\n",
						card->devname);
		return -ENOMEM;
	}
	
	memset(chan, 0, sizeof(x25_channel_t));

	WAN_TASKLET_INIT((&chan->common.bh_task),0,x25api_bh,(unsigned long)chan);
	/* Bug Fix: Seg Err on PVC startup
	 * It must be here since bind_lcn_to_dev expects 
	 * it bellow */
	wan_netif_set_priv(dev,chan);
	
	strcpy(chan->name, conf->name);
	chan->card = card;
	chan->tx_skb = chan->rx_skb = NULL;

	/* verify media address */
	if (conf->addr[0] == '@'){		/* SVC */
		
		chan->common.svc = 1;
		strncpy(chan->addr, &conf->addr[1], WAN_ADDRESS_SZ);

		strncpy(chan->x25_src_addr, 
			&conf->x25_src_addr[0], WAN_ADDRESS_SZ);
		
		strncpy(chan->accept_dest_addr,
			&conf->accept_dest_addr[0], WAN_ADDRESS_SZ);

		strncpy(chan->accept_src_addr, 
			&conf->accept_src_addr[0], WAN_ADDRESS_SZ);

		strncpy(chan->accept_usr_data, 
			&conf->accept_usr_data[0], WAN_ADDRESS_SZ);
		
		/* Set channel timeouts (default if not specified) */
		chan->idle_timeout = (conf->idle_timeout) ? 
					conf->idle_timeout : 90;
		chan->hold_timeout = (conf->hold_timeout) ? 
					conf->hold_timeout : 10;

	}else if (is_digit(conf->addr[0])){	/* PVC */
		int lcn = dec_to_uint(conf->addr, 0);

		if ((lcn >= card->u.x.x25_conf.lo_pvc) && (lcn <= card->u.x.x25_conf.hi_pvc)){
			bind_lcn_to_dev (card, dev, lcn);
		}else{
			printk(KERN_INFO
				"%s: PVC %u is out of range on interface %s!\n",
				wandev->name, lcn, chan->name);
			err = -EINVAL;
		}
	}else{
		printk(KERN_INFO
			"%s: invalid media address on interface %s!\n",
			wandev->name, chan->name);
		err = -EINVAL;
	}

	if(strcmp(conf->usedby, "WANPIPE") == 0){
                printk(KERN_INFO "%s: Running in WANPIPE mode %s\n",
			wandev->name, chan->name);
                chan->common.usedby = WANPIPE;
		chan->protocol = htons(ETH_P_IP);

        }else if(strcmp(conf->usedby, "API") == 0){
		chan->common.usedby = API;
                printk(KERN_INFO "%s: Running in API mode %s\n",
			wandev->name, chan->name);
		chan->protocol = htons(X25_PROT);
		wan_reg_api(chan,dev,card->devname);
	}

	if (err){
		kfree(chan);
		wan_netif_set_priv(dev,NULL);
		return err;
	}
	
	chan->enable_IPX = conf->enable_IPX;
	
	if (chan->enable_IPX)
		chan->protocol = htons(ETH_P_IPX);
	
	if (conf->network_number)
		chan->network_number = conf->network_number;
	else
		chan->network_number = 0xDEADBEEF;

	/* prepare network device data space for registration */
	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&x25_ioctl);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);

	init_x25_channel_struct(chan);

	/*
	 * Create interface file in proc fs.
	 */
	err = wanrouter_proc_add_interface(wandev, &chan->dent, chan->name, dev);
	if (err){	
		printk(KERN_INFO
			"%s: can't create /proc/net/router/fr/%s entry!\n",
			card->devname, chan->name);
		return err;
	}

	spin_lock_init(&chan->lock);

	return 0;
}

/*===================================================================
 * Name:	del_if(),  Remove a logical channel.	 
 *
 * Purpose:	To dynamically remove a logical channel.
 * 
 * Rationale:	Each logical channel should be dynamically
 *              removable. This functin is called by an 
 *              IOCTL_IFDEL ioctl call or shutdown(). 
 *                
 * Description: Do nothing.
 *
 * Called by:	IOCTL_IFDEL : wanrouter_ioctl() from wanmain.c
 *              shutdown() from sdlamain.c
 *
 * Assumptions: 
 *
 * Warnings:
 *
 * Return: 	0 Ok. Void function.
 */


static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	unsigned long smp_flags;
	sdla_t *card=wandev->priv;
	x25_channel_t* chan = wan_netif_priv(dev);

	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, chan->name);

	WAN_TASKLET_KILL(&chan->common.bh_task);
	wan_unreg_api(chan, card->devname);
	
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	card->u.x.tx_dev=NULL;
	card->u.x.cmd_dev=NULL;
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return 0;
}


/*============================================================
 * Name:	disable_comm	
 *
 * Description:	Disable communications during shutdown.
 *              Dont check return code because there is 
 *              nothing we can do about it.  
 *
 * Warning:	Dev and private areas are gone at this point.
 *===========================================================*/

static void disable_comm(sdla_t* card)
{
	del_timer(&card->u.x.x25_timer);
	disable_comm_shutdown(card);
	return;
}


/*============================================================
 *	Network Device Interface 
 *===========================================================*/

/*===================================================================
 * Name:	if_init(),   Netowrk Interface Initialization 	 
 *
 * Purpose:	To initialize a network interface device structure.
 * 
 * Rationale:	During network interface startup, the if_init
 *              is called by the kernel to initialize the
 *              netowrk device structure.  Thus a driver
 *              can customze a network device. 
 *                
 * Description:	Initialize the netowrk device call back
 *              routines.  This is where we tell the kernel
 *              which function to use when it wants to send
 *              via our interface. 
 *		Furthermore, we initialize the device flags, 
 *              MTU and physical address of the board.
 *
 * Called by:	Kernel (/usr/src/linux/net/core/dev.c)
 * 		(dev->init())
 *
 * Assumptions: None
 *	
 * Warnings:	None
 *
 * Return: 	0 	Ok : Void function.
 */
static int if_init (netdevice_t* dev)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	wan_device_t* wandev = &card->wandev;

	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&x25_ioctl);

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (chan->common.usedby != API){
		WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
		dev->watchdog_timeo	= TX_TIMEOUT;
	}
#endif

	/* Initialize media-specific parameters */
	dev->type		= ARPHRD_PPP;		/* ARP h/w type */

	dev->flags		|= IFF_POINTOPOINT;
	dev->flags		|= IFF_NOARP;

	if (chan->common.usedby == API){
		dev->mtu	= X25_CHAN_MTU+sizeof(x25api_hdr_t);
	}else{
		dev->mtu	= card->wandev.mtu; 	
	}
	
	dev->hard_header_len	= 0;            /* media header length */
	dev->addr_len		= 2;		/* hardware address length */
	
	if (!chan->common.svc){
		*(unsigned short*)dev->dev_addr = htons(chan->common.lcn);
	}
	
	/* Initialize hardware parameters (just for reference) */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY (unsigned long)wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

        /* Set transmit buffer queue length */
        dev->tx_queue_len = 100;

	set_chan_state(dev, WAN_DISCONNECTED);
	return 0;
}


/*===================================================================
 * Name:	if_open(),   Open/Bring up the Netowrk Interface 
 *
 * Purpose:	To bring up a network interface.
 * 
 * Rationale:	
 *                
 * Description:	Open network interface.
 * 		o prevent module from unloading by incrementing use count
 * 		o if link is disconnected then initiate connection
 *
 * Called by:	Kernel (/usr/src/linux/net/core/dev.c)
 * 		(dev->open())
 *
 * Assumptions: None
 *	
 * Warnings:	None
 *
 * Return: 	0 	Ok
 * 		<0 	Failur: Interface will not come up.
 */

static int if_open (netdevice_t* dev)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	struct timeval tv;
	unsigned long smp_flags;
	
	if (open_dev_check(dev))
		return -EBUSY;


	/* Increment the number of interfaces */
	++card->u.x.no_dev;
	
	wanpipe_open(card);

	if (!card->u.x.LAPB_hdlc){
		/* X25 can have multiple interfaces thus, start the 
		 * protocol once all interfaces are up */

		//FIXME: There is a bug here. If interface is
		//brought down and up, it will try to enable comm.
		if (card->open_cnt == card->u.x.num_of_ch){

			S508_S514_lock(card, &smp_flags);
			connect(card);
			S508_S514_unlock(card, &smp_flags);

			del_timer(&card->u.x.x25_timer);
			card->u.x.x25_timer.expires=jiffies+HZ;
			add_timer(&card->u.x.x25_timer);
		}
	}
	/* Device is not up untill the we are in connected state */
	do_gettimeofday( &tv );
	chan->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

	/* PVC in WANPIPE mode should start connected */
	if (!chan->common.svc && chan->common.usedby == WANPIPE){
		unsigned long flags;
		int err;
		spin_lock_irqsave(&card->wandev.lock,flags);
		err=x25_get_chan_conf(card, chan);
		if (err == CMD_OK){
			set_chan_state(dev, WAN_CONNECTED);
		}else{
			DEBUG_EVENT("%s:%s: PVC conf error, channel not ready! 0x%X\n",
					card->devname,chan->name,err);
			set_chan_state(dev, WAN_DISCONNECTED);	
		}
		spin_unlock_irqrestore(&card->wandev.lock,flags);
	}
	
	return 0;
}

/*===================================================================
 * Name:	if_close(),   Close/Bring down the Netowrk Interface 
 *
 * Purpose:	To bring down a network interface.
 * 
 * Rationale:	
 *                
 * Description:	Close network interface.
 * 		o decrement use module use count
 *
 * Called by:	Kernel (/usr/src/linux/net/core/dev.c)
 * 		(dev->close())
 *		ifconfig <name> down: will trigger the kernel
 *              which will call this function.
 *
 * Assumptions: None
 *	
 * Warnings:	None
 *
 * Return: 	0 	Ok
 * 		<0 	Failure: Interface will not exit properly.
 */
static int if_close (netdevice_t* dev)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	unsigned long smp_flags;
	
	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	
	if ((chan->common.state == WAN_CONNECTED) || 
	    (chan->common.state == WAN_CONNECTING)){
		S508_S514_lock(card, &smp_flags);
		chan_disc(dev);
		set_chan_state(dev, WAN_DISCONNECTED);
		S508_S514_unlock(card, &smp_flags);
	}

	wanpipe_close(card);

	/* If this is the last close, disconnect physical link */
	if (!card->open_cnt){
		S508_S514_lock(card, &smp_flags);
		disconnect(card);
		if (card->sk){
			protocol_disconnected(card->sk);
			sock_put(card->sk);
			card->sk=NULL;
		}
		x25_set_intr_mode(card, 0);
		S508_S514_unlock(card, &smp_flags);
	}
	
	/* Decrement the number of interfaces */
	--card->u.x.no_dev;
	return 0;
}


/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	++chan->if_send_stat.if_send_tbusy_timeout;
	printk (KERN_INFO "%s: Transmit timed out on %s\n", 
			card->devname, dev->name);
	netif_wake_queue (dev);
}


/*=========================================================================
 * 	Send a packet on a network interface.
 * 	o set tbusy flag (marks start of the transmission).
 * 	o check link state. If link is not up, then drop the packet.
 * 	o check channel status. If it's down then initiate a call.
 * 	o pass a packet to corresponding WAN device.
 * 	o free socket buffer
 *
 * 	Return:	0	complete (socket buffer must be freed)
 *		non-0	packet may be re-transmitted (tbusy must be set)
 *
 * 	Notes:
 * 	1. This routine is called either by the protocol stack or by the "net
 *    	bottom half" (with interrupts enabled).
 * 	2. Setting tbusy flag will inhibit further transmit requests from the
 *    	protocol stack and can be used for flow control with protocol layer.
 *
 *========================================================================*/

static int if_send (struct sk_buff* skb, netdevice_t* dev)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	int udp_type;
	unsigned long smp_flags=0;

	++chan->if_send_stat.if_send_entry;

	/* No need to check frame length, since socket code
         * will perform the check for us */

#if defined(LINUX_2_1)
	if (dev->tbusy){
		netdevice_t *dev2;
		
		++chan->if_send_stat.if_send_tbusy;
		if ((jiffies - chan->tick_counter) < (5*HZ)){
			return 1;
		}
		
		if_tx_timeout(dev);
	}
#else
	netif_stop_queue(dev);
#endif

	chan->tick_counter = jiffies;
	
	/* Critical region starts here */
	S508_S514_lock(card, &smp_flags);
	
	if (test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)){
		printk(KERN_INFO "Hit critical in if_send()! %lx\n",card->wandev.critical);
		goto if_send_crit_exit;
	}
	
	udp_type = udp_pkt_type(skb, card);

        if(udp_type != UDP_INVALID_TYPE) {

                if(store_udp_mgmt_pkt(udp_type, UDP_PKT_FRM_STACK, card, dev, skb,
                        chan->common.lcn)) {
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, INTR_ON_TIMER);
                        if (udp_type == UDP_XPIPE_TYPE){
                                chan->if_send_stat.if_send_PIPE_request++;
			}
               	}
		start_net_queue(dev);
		clear_bit(SEND_CRIT,(void*)&card->wandev.critical);
		S508_S514_unlock(card, &smp_flags);
		return 0;
	}

	if (chan->transmit_length){
		//FIXME: This check doesn't make sense any more
		if (chan->common.state != WAN_CONNECTED){
			chan->transmit_length=0;
		}else{
			stop_net_queue(dev);

			atomic_inc(&card->u.x.tx_interrupts_pending);
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, INTR_ON_TX_FRAME);
			clear_bit(SEND_CRIT,(void*)&card->wandev.critical);
			S508_S514_unlock(card, &smp_flags);
			return 1;
		}
	}

	if (card->wandev.state != WAN_CONNECTED ||
	    chan->common.state != WAN_CONNECTED){
		++chan->ifstats.tx_dropped;
		++card->wandev.stats.tx_dropped;
		++chan->if_send_stat.if_send_wan_disconnected;	
		
	}else if (chan->common.usedby == API &&
	          skb->protocol!=htons(WP_X25_PROT) && 
		  skb->protocol!=htons(ETH_P_X25)){

		/* For backward compatibility we must check
		 * for WP_X25_PROT as well as ETH_P_X25 prot */

		printk(KERN_INFO
			"%s: Unsupported skb prot=0x%04X not equal to 0x%04X!\n",
			chan->name, htons(skb->protocol), htons(chan->protocol));
		
		++chan->ifstats.tx_errors;
		++chan->ifstats.tx_dropped;
		++card->wandev.stats.tx_dropped;
		++chan->if_send_stat.if_send_protocol_error;
		
	}else switch (chan->common.state){

		case WAN_DISCONNECTED:
			/* Try to establish connection. If succeded, then start
			 * transmission, else drop a packet.
			 */
			if (chan->common.usedby == API){
				++chan->ifstats.tx_dropped;
				++card->wandev.stats.tx_dropped;
				break;
			}else{
				if (chan_connect(dev) != 0){
					++chan->ifstats.tx_dropped;
					++card->wandev.stats.tx_dropped;
					break;
				}
			}
			/* fall through */

		case WAN_CONNECTED:
			if( skb->protocol == htons(ETH_P_IPX)) {
				if(chan->enable_IPX) {
					switch_net_numbers( skb->data, 
						chan->network_number, 0);
				} else {
					++card->wandev.stats.tx_dropped;
					++chan->ifstats.tx_dropped;
					++chan->if_send_stat.if_send_protocol_error;
					goto if_send_crit_exit;
				}
			}
			/* We never drop here, if cannot send than, copy
	                 * a packet into a transmit buffer 
                         */
			chan_send(dev, skb->data, skb->len, 0);
			break;

		default:
			++chan->ifstats.tx_dropped;	
			++card->wandev.stats.tx_dropped;
			break;
	}


if_send_crit_exit:
	
       	wan_skb_free(skb);

	start_net_queue(dev);
	clear_bit(SEND_CRIT,(void*)&card->wandev.critical);
	S508_S514_unlock(card, &smp_flags);
	return 0;
}

/*============================================================================
 * Setup so that a frame can be transmitted on the occurence of a transmit
 * interrupt.
 *===========================================================================*/

static void setup_for_delayed_transmit (netdevice_t* dev, void* buf,
	unsigned len)
{
        x25_channel_t* chan = wan_netif_priv(dev);
        sdla_t* card = chan->card;

	++chan->if_send_stat.if_send_adptr_bfrs_full;

        if(chan->transmit_length) {
                printk(KERN_INFO "%s: Error, transmit length set in delayed transmit!\n",
				card->devname);
                return;
        }

	if (chan->common.usedby == API){
		if (len > X25_CHAN_MTU+sizeof(x25api_hdr_t)) {
			++chan->ifstats.tx_dropped;	
			++card->wandev.stats.tx_dropped;
			printk(KERN_INFO "%s: Length is too big for delayed transmit\n",
				card->devname);
			return;
		}
	}else{
		if (len > dev->mtu || len > X25_CHAN_MTU) {
			++chan->ifstats.tx_dropped;	
			++card->wandev.stats.tx_dropped;
			printk(KERN_INFO "%s: Length is too big for delayed transmit\n",
				card->devname);
			return;
		}
	}

	/* N.C.: Bug Fix: Fri Jul 19 12:50:02 EDT 2002
	 * The tx_offset must be re-initialized to zero,
	 * each time a new frame is queued for tx.
	 * Otherwise, tx_offset value might be left from
	 * previous failure to send !*/
	chan->tx_offset = 0;
	
        chan->transmit_length = len;
        memcpy(chan->transmit_buffer, buf, len);

	++chan->if_send_stat.if_send_tx_int_enabled;

	/* Enable Transmit Interrupt */
	atomic_inc(&card->u.x.tx_interrupts_pending);
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, INTR_ON_TX_FRAME);
}


/*===============================================================
 * net_device_stats
 *
 * 	Get ethernet-style interface statistics.
 * 	Return a pointer to struct enet_statistics.
 *
 *==============================================================*/
static struct net_device_stats *if_stats (netdevice_t* dev)
{
	x25_channel_t *chan = wan_netif_priv(dev);

	if(chan == NULL)
		return NULL;

	return &chan->ifstats;
}

/*==============================================================
 * x25_ioctl
 *
 * Description:
 *	Network device ioctl function used by the socket api 
 *	layer or higher layers, to execute X25 commands and 
 *	perform registration and unregistration tasts.
 *
 * Usedby:
 * 	Higher protocol layers such as socket api, dsp, etc.
 */

static int x25_ioctl (struct net_device *dev, struct ifreq *ifr, int cmd)
{
	x25_channel_t *chan;
	sdla_t *card;
	int err=0;
	unsigned long flags;
	wan_udp_pkt_t *wan_udp_pkt;


	if (!dev || !(dev->flags & IFF_UP)){
		return -ENODEV;
	}
	
	if ((chan=wan_netif_priv(dev)) == NULL){
		return -ENODEV;
	}

	card=chan->card;

	DEBUG_TEST("%s:%s:%s: Cmd(%X)=%s ifr=%p lcn %i\n",__FUNCTION__,
			card->devname,dev->name,cmd,DECODE_X25_CMD(cmd),ifr,chan->common.lcn);
	switch (cmd){
	
		case SIOC_X25_PLACE_CALL:

			if (!test_bit(0,&card->u.x.card_ready)){
				atomic_set(&chan->common.command,0);
				trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);
				err = -EBUSY;
				goto x25_ioctl_exit;
			}

						
			if (card->u.x.LAPB_hdlc){
				spin_lock_irqsave(&card->wandev.lock,flags);
				err=lapb_connect(card,chan);
				spin_unlock_irqrestore(&card->wandev.lock,flags);
				goto x25_ioctl_exit;

			}

			if (card->wandev.state != WAN_CONNECTED ||
			    chan->common.state != WAN_DISCONNECTED){
 				err = -EBUSY;
 				goto x25_ioctl_exit;
 			}

			err=test_chan_command_busy(card,chan);
			if (err){
				goto x25_ioctl_exit;
			}
			
			atomic_set(&chan->common.command,X25_PLACE_CALL);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			err=wait_for_cmd_rc(card,chan);
			
			DEBUG_TEST("%s:%s PLACE CALL RC %x\n",card->devname,dev->name,err);
			break;
			
		case SIOC_X25_SET_CALL_DATA:
			err=x25_api_cmd_setup(chan,ifr);
			break;
			
		case SIOC_X25_GET_CALL_DATA:
			err=x25_api_get_cmd_result(chan, ifr);
			break;
			
		case SIOC_X25_CLEAR_CALL:

			if (card->wandev.state != WAN_CONNECTED){
 				err = -EINVAL;
 				goto x25_ioctl_exit;
 			}


			if (chan->common.state == WAN_DISCONNECTED || 
			    chan->common.state == WAN_DISCONNECTING){
 				err = -EINVAL;
 				goto x25_ioctl_exit;
 			}

			err=test_chan_command_busy(card,chan);
			if (err){
				goto x25_ioctl_exit;
			}
		
			if (!chan->common.svc){
				err=-EINVAL;
				goto x25_ioctl_exit;
			}
			
			if (chan->common.state == WAN_DISCONNECTED){
				err = -ENETDOWN;
				goto x25_ioctl_exit;
			}

			err=x25_api_setup_cmd_hdr(chan,ifr);
			if (err)
				goto x25_ioctl_exit;
			
			err=x25_api_setup_call_string(chan,ifr);
			if (err){
				goto x25_ioctl_exit;
			}

			if (chan->x25_api_cmd_hdr.qdm & 0x80){
				if (chan->transmit_length){
					DEBUG_EVENT("%s: Clear call failed: data in tx buf!\n",
							chan->common.dev->name);
					err= -EBUSY;
					goto x25_ioctl_exit;
				}
			}

			atomic_set(&chan->common.command,X25_CLEAR_CALL);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			err=wait_for_cmd_rc(card,chan);
			if (err == -EFAULT){
				err=x25_clear_call(card,chan->common.lcn,0,0,0,
						   chan->call_string,strlen(chan->call_string));
				if (err){
					err=-EINVAL;
				}
			}

			if (err != 0){
				DEBUG_EVENT("%s:%s: Warning: Clear call failed, delaying disconnect!\n",
						card->devname,chan->common.dev->name);
			}

			break;
		
		case SIOC_X25_ACCEPT_CALL:

			if (chan->common.state != WAN_CONNECTING){
				err = -EBUSY;
				goto x25_ioctl_exit;
			}

			err=test_chan_command_busy(card,chan);
			if (err){
				goto x25_ioctl_exit;
			}
			
			err=x25_api_setup_cmd_hdr(chan,ifr);
			if (err)
				goto x25_ioctl_exit;

			err=x25_api_setup_call_string(chan,ifr);
			if (err){
				goto x25_ioctl_exit;
			}
		
			atomic_set(&chan->common.command,X25_ACCEPT_CALL);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			err=wait_for_cmd_rc(card,chan);

			DEBUG_TEST("%s:%s ACCEPT CALL RC %x\n",card->devname,dev->name,err);

			break;
		
		case SIOC_X25_RESET_CALL:

			err=x25_api_setup_cmd_hdr(chan,ifr);
			if (err)
				goto x25_ioctl_exit;

			err=test_chan_command_busy(card,chan);
			if (err){
				goto x25_ioctl_exit;
			}

			atomic_set(&chan->common.command,X25_RESET);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			err=wait_for_cmd_rc(card,chan);

			DEBUG_TEST("%s:%s RESET CALL RC %x\n",card->devname,dev->name,err);
				
			break;

		case SIOC_X25_INTERRUPT:

			err=test_chan_command_busy(card,chan);
			if (err){
				goto x25_ioctl_exit;
			}

			err=x25_api_setup_cmd_hdr(chan,ifr);
			if (err)
				goto x25_ioctl_exit;

			err=x25_api_setup_call_string(chan,ifr);
			if (err){
				goto x25_ioctl_exit;
			}
		
			atomic_set(&chan->common.command,X25_WP_INTERRUPT);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			err=wait_for_cmd_rc(card,chan);

			DEBUG_TEST("%s:%s INTERRUPT RC %x\n",card->devname,dev->name,err);
				
			break;

		case SIOC_X25_SET_LCN_PID:
			if (ifr && ifr->ifr_data){
				err = copy_from_user(&chan->pid,
				  	              ifr->ifr_data,
					              sizeof(pid_t));
			}
			break;

		case SIOC_X25_SET_LCN_LABEL:
			if (ifr && ifr->ifr_data){
				err = copy_from_user(&chan->dlabel[0],
				  	              ifr->ifr_data,
						      WAN_IF_LABEL_SZ);

				chan->dlabel[WAN_IF_LABEL_SZ]=0;
			}
			break;	
			
		case SIOC_WANPIPE_BIND_SK:
			if (!ifr){
				err= -EINVAL;
				goto x25_ioctl_exit;
			}

			if (card->u.x.LAPB_hdlc){
				set_bit(0,&card->u.x.card_ready);
			}
			
			if (!test_bit(0,&card->u.x.card_ready)){
				atomic_set(&chan->common.command,0);
				trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);
				err = -EBUSY;
				goto x25_ioctl_exit;
			}
				
			if (ifr->ifr_data && !chan->common.sk){
				if (!spin_is_locked(&card->wandev.lock)){
					spin_lock_irqsave(&card->wandev.lock,flags);
					chan->common.sk = (struct sock*)ifr->ifr_data;
					sock_hold(chan->common.sk);
					set_bit(LCN_SK_ID,&chan->common.used);
					spin_unlock_irqrestore(&card->wandev.lock,flags);
				}else{
					chan->common.sk = (struct sock*)ifr->ifr_data;
					sock_hold(chan->common.sk);
					set_bit(LCN_SK_ID,&chan->common.used);
				}
				break;
			}
		
			err= -EBUSY;
			break;

		case SIOC_WANPIPE_UNBIND_SK:
			if (!ifr){
				err= -EINVAL;
				goto x25_ioctl_exit;
			}

			spin_lock_irqsave(&card->wandev.lock,flags);
			err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,flags);

			if (atomic_read(&chan->common.receive_block)){
				TX25Status status;

				card->hw_iface.peek(card->hw,
						    card->intr_perm_off,
						    &status.imask,
						    sizeof(status.imask));
				atomic_set(&chan->common.receive_block,0);
		
				if (!(status.imask & INTR_ON_RX_FRAME)){
					DEBUG_EVENT("%s: Disabling x25 flow control!\n",
								card->devname);
					status.imask |= INTR_ON_RX_FRAME;
					card->hw_iface.poke(card->hw,
							    card->intr_perm_off,
							    &status.imask,
							    sizeof(status.imask));
				}
			}

			if (chan->common.svc){
				atomic_set(&chan->common.disconnect,1);
			}else{
				set_chan_state(dev,WAN_DISCONNECTED);
			}

			if (card->u.x.LAPB_hdlc){
				spin_lock_irqsave(&card->wandev.lock,flags);
				disconnect(card);
				set_chan_state(dev,WAN_DISCONNECTED);
				spin_unlock_irqrestore(&card->wandev.lock,flags);
			}

			break;

		case SIOC_WANPIPE_GET_SK:
			if (!ifr){
				err= -EINVAL;
				goto x25_ioctl_exit;
			}
			
			if (test_bit(LCN_SK_ID,&chan->common.used) && chan->common.sk){
				sock_hold(chan->common.sk);
				ifr->ifr_data = (void*)chan->common.sk;
			}else{
				ifr->ifr_data = NULL;
			}
			break;
		
		case SIOC_WANPIPE_CHECK_TX:
		case SIOC_ANNEXG_CHECK_TX:
			err=chan->transmit_length;
			break;

		case SIOC_ANNEXG_KICK:

			if (atomic_read(&chan->common.receive_block)){
				WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
			}
			
			err=0;
			break;

		case SIOC_WANPIPE_SNMP:
		case SIOC_WANPIPE_SNMP_IFSPEED:
			err = wan_snmp_data(card, dev, cmd, ifr);
			break;

		case SIOC_WANPIPE_DEV_STATE:
			err = chan->common.state;
			break;

		case SIOC_WANPIPE_PIPEMON:

			if (atomic_read(&card->u.x.udp_pkt_len) != 0){
				err = -EBUSY;
				goto x25_ioctl_exit;
			}
	
			atomic_set(&card->u.x.udp_pkt_len,sizeof(wan_udp_hdr_t));
		
			wan_udp_pkt=(wan_udp_pkt_t*)&card->u.x.udp_pkt_data;
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				atomic_set(&card->u.x.udp_pkt_len,0);
				err = -EINVAL;
				goto x25_ioctl_exit;
			}

			spin_lock_irqsave(&card->wandev.lock, flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)){
				printk(KERN_INFO "%s:%s pipemon command busy: try again!\n",
						card->devname,dev->name);
				atomic_set(&card->u.x.udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, flags);
				err = -EINVAL;
				goto x25_ioctl_exit;
			}
			
			if (process_udp_mgmt_pkt(card,dev) <= 0 ){
				atomic_set(&card->u.x.udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, flags);
				err = -EINVAL;
				goto x25_ioctl_exit;
			}

			
			spin_unlock_irqrestore(&card->wandev.lock, flags);

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */
			
			if (atomic_read(&card->u.x.udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				printk(KERN_INFO "(Debug): PiPemon Buf too bit on the way up! %i\n",
						atomic_read(&card->u.x.udp_pkt_len));
				atomic_set(&card->u.x.udp_pkt_len,0);
				err = -EINVAL;
				goto x25_ioctl_exit;
			}

			if (copy_to_user(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				atomic_set(&card->u.x.udp_pkt_len,0);
				err = -EFAULT;
				goto x25_ioctl_exit;
			}
		
			atomic_set(&card->u.x.udp_pkt_len,0);
			break;

			
		default:
			DEBUG_EVENT("%s: Unsupported Ioctl Call 0x%x \n",
					chan->common.dev->name,cmd);
			err= -EOPNOTSUPP;
			break;
	}
x25_ioctl_exit:

	return err;	
}


/*
 *	Interrupt Handlers 
 */

/*
 * X.25 Interrupt Service Routine.
 */

static WAN_IRQ_RETVAL wpx_isr (sdla_t* card)
{
	u8	intr_type;

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	/* Sanity check, should never happen */
	if (test_bit(0,&card->in_isr)){
		printk(KERN_INFO "%s: Critical in WPX_ISR\n",card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}
	
	card->in_isr = 1;
	++card->statistics.isr_entry;

	if (test_bit(PERI_CRIT,(void*)&card->wandev.critical)){
		card->in_isr=0;
		card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}
	
	if (test_bit(SEND_CRIT, (void*)&card->wandev.critical)){

		card->hw_iface.peek(card->hw, card->intr_type_off, &intr_type, 1);
 		printk(KERN_INFO "%s: wpx_isr: wandev.critical set to 0x%02lx, int type = 0x%02x\n", 
			card->devname, 
			card->wandev.critical, 
			intr_type);
		
		card->in_isr = 0;
		card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	/* For all interrupts set the critical flag to CRITICAL_RX_INTR.
         * If the if_send routine is called with this flag set it will set
         * the enable transmit flag to 1. (for a delayed interrupt)
         */
	card->hw_iface.peek(card->hw, card->intr_type_off, &intr_type, 1);
	switch (intr_type){

		case RX_INTR_PENDING:		/* receive interrupt */
			rx_intr(card);
			break;

		case TX_INTR_PENDING:		/* transmit interrupt */
			tx_intr(card);
			break;

		case MODEM_INTR_PENDING:	/* modem status interrupt */
			status_intr(card);
			break;

		case X25_ASY_TRANS_INTR_PENDING:	/* network event interrupt */
			event_intr(card);
			break;

		case TIMER_INTR_PENDING:
			timer_intr(card);
			break;

		case TRACE_INTR_PENDING:
			trace_intr(card);
			break;

		default:		/* unwanted interrupt */
			spur_intr(card);
	}

	card->in_isr = 0;
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}

/*
 * 	Receive interrupt handler.
 * 	This routine handles fragmented IP packets using M-bit according to the
 * 	RFC1356.
 * 	o map ligical channel number to network interface.
 * 	o allocate socket buffer or append received packet to the existing one.
 * 	o if M-bit is reset (i.e. it's the last packet in a sequence) then 
 *   	decapsulate packet and pass socket buffer to the protocol stack.
 *
 * 	Notes:
 * 	1. When allocating a socket buffer, if M-bit is set then more data is
 *    	coming and we have to allocate buffer for the maximum IP packet size
 *    	expected on this channel.
 * 	2. If something goes wrong and X.25 packet has to be dropped (e.g. no
 *    	socket buffers available) the whole packet sequence must be discarded.
 */

static void rx_intr (sdla_t* card)
{
	wan_cmd_t	rxmb_cmd;
	unsigned	lcn;
	netdevice_t*	dev = NULL;
	x25_channel_t*	chan;
	struct sk_buff* skb=NULL;
	TX25Status	status;

	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	card->hw_iface.peek(card->hw, card->rxmb_off, &rxmb_cmd, sizeof(rxmb_cmd));
	lcn = rxmb_cmd.wan_cmd_x25_lcn;
	dev = find_channel(card, lcn);

	if (dev == NULL){
		/* Invalid channel, discard packet */
		printk(KERN_INFO "%s: receiving on orphaned LCN %d!\n",
			card->devname, lcn);
		return;
	}

	chan = wan_netif_priv(dev);
	chan->i_timeout_sofar = jiffies;


	/* Copy the data from the board, into an
         * skb buffer 
	 */
	if (wanpipe_pull_data_in_skb(card,dev,&skb)){
		++chan->ifstats.rx_dropped;
		++card->wandev.stats.rx_dropped;
		++chan->rx_intr_stat.rx_intr_no_socket;
		++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;
		return;
	}

	dev->last_rx = jiffies;		/* timestamp */

	/* set rx_skb to NULL so we won't access it later when 
	 * kernel already owns it */
	chan->rx_skb=NULL;

	/* ------------ API ----------------*/

	if (chan->common.usedby == API){
	
		int err = wan_api_enqueue_skb(chan,skb);

		switch (err){

		case 0:
			/* Buffer enqueued ok, proceed to trigger
			 * API bh */
			WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
			break;

		case 1:

			/* The rx buffer is now full. If we receive
			 * another packet, we will might drop it.
			 * Thus, turn off the rx interrupt to implement
			 * x25 flow control, until the rx queue
			 * is free */
			
			DEBUG_EVENT("%s:%s: Error: Driver Rx buffer is full\n",
				card->devname,chan->common.dev->name);
			DEBUG_EVENT("%s: Enabling x25 flow control!\n",
				card->devname);
			status.imask &= ~INTR_ON_RX_FRAME;
			card->hw_iface.poke(card->hw,
					    card->intr_perm_off,
					    &status.imask,
					    sizeof(status.imask));

			atomic_set(&chan->common.receive_block,1);
			break;
			
		default:
		
			DEBUG_EVENT("%s: Critical Error: Rx intr dropping x25 packet!\n",
					card->devname);
			++chan->ifstats.rx_dropped;
			++card->wandev.stats.rx_dropped;
			++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;
			wan_skb_free(skb);
			break;
		}		

		return;
	}


	/* ------------- WANPIPE -------------------*/
	
	
	/* Decapsulate packet, if necessary */
	if (!skb->protocol && !wanrouter_type_trans(skb, dev)){
		/* can't decapsulate packet */
		wan_skb_free(skb);
		++chan->ifstats.rx_errors;
		++chan->ifstats.rx_dropped;
		++card->wandev.stats.rx_dropped;
		++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;

	}else{
		if(handle_IPXWAN(skb->data, chan->name, 
				  chan->enable_IPX, chan->network_number, 
				  skb->protocol)){

			if( chan->enable_IPX ){
				if(chan_send(dev, skb->data, skb->len,0)){
					chan->tx_skb = skb;
				}else{
					wan_skb_free(skb);
					++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;
				}
			}else{
				/* increment IPX packet dropped statistic */
				++chan->ifstats.rx_dropped;
				++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;
			}
		}else{
			wan_skb_reset_mac_header(skb);
			chan->ifstats.rx_bytes += skb->len;

			++chan->ifstats.rx_packets;
			++chan->rx_intr_stat.rx_intr_bfr_passed_to_stack;
			netif_rx(skb);
		}
	}
	
	return;
}


static int wanpipe_pull_data_in_skb (sdla_t *card, netdevice_t *dev, struct sk_buff **skb)
{
	void *bufptr;
	wan_cmd_t	rxmb_cmd;
	unsigned	len;		/* packet length */
	unsigned	qdm;		/* Q,D and M bits */
	x25_channel_t *chan = wan_netif_priv(dev);
	struct sk_buff *new_skb = *skb;

	card->hw_iface.peek(card->hw, card->rxmb_off, &rxmb_cmd, sizeof(rxmb_cmd));
	len = rxmb_cmd.wan_cmd_data_len;
	qdm = rxmb_cmd.wan_cmd_x25_qdm;
	
	if (chan->common.usedby == WANPIPE){
		if (chan->drop_sequence){
			if (!(qdm & 0x01)){ 
				chan->drop_sequence = 0;
			}
			return 1;
		}
		if (chan->rx_skb)
			new_skb = chan->rx_skb;
	}else{
		/* Add on the API header to the received
                 * data 
		 */
		len += sizeof(x25api_hdr_t);
	}

	if (new_skb == NULL){
		int bufsize;

		if (chan->common.usedby == WANPIPE){
			bufsize = (qdm & 0x01) ? dev->mtu : len;
		}else{
			bufsize = len;
		}

		/* Allocate new socket buffer */
		new_skb = dev_alloc_skb(bufsize + 15);
		if (new_skb == NULL){
			printk(KERN_INFO "%s: no socket buffers available!\n",
				card->devname);
			chan->drop_sequence = 1;	/* set flag */
			++chan->ifstats.rx_dropped;
			return 1;
		}
	}

	if (skb_tailroom(new_skb) < len){
		/* No room for the packet. Call off the whole thing! */
		wan_skb_free(new_skb);
		if (chan->common.usedby == WANPIPE){
			chan->rx_skb = NULL;
			if (qdm & 0x01){ 
				chan->drop_sequence = 1;
			}
		}

		if(net_ratelimit()){
			printk(KERN_INFO "%s: unexpectedly long packet sequence "
				"on interface %s!\n", card->devname, dev->name);
		}
		++chan->ifstats.rx_length_errors;
		return 1;
	}

	bufptr = skb_put(new_skb,len);


	if (chan->common.usedby == API){
		/* Fill in the x25api header 
		 */
		x25api_t * api_data = (x25api_t*)bufptr;
		api_data->hdr.qdm = rxmb_cmd.wan_cmd_x25_qdm;
		api_data->hdr.cause = rxmb_cmd.wan_cmd_x25_cause;
		api_data->hdr.diagn = rxmb_cmd.wan_cmd_x25_diagn;
		api_data->hdr.length = rxmb_cmd.wan_cmd_data_len;
		api_data->hdr.lcn  = rxmb_cmd.wan_cmd_x25_lcn;
		card->hw_iface.peek(card->hw, card->rxmb_off + offsetof(wan_mbox_t, wan_data),
			  api_data->data, rxmb_cmd.wan_cmd_data_len);
	}else{
		card->hw_iface.peek(card->hw, card->rxmb_off + offsetof(wan_mbox_t, wan_data),
			  bufptr, len);
	}

	new_skb->dev = dev;

	if (chan->common.usedby == API){
		wan_skb_reset_mac_header(new_skb);
		new_skb->protocol = htons(X25_PROT);
		new_skb->pkt_type = WAN_PACKET_DATA;
	}else{
		new_skb->protocol = chan->protocol;
	}

	/* If qdm bit is set, more data is coming 
         * thus, exit and wait for more data before
         * sending the packet up. (Used by router only) 
	 */
	if ((qdm & 0x01) && (chan->common.usedby == WANPIPE)){ 
		chan->rx_skb = new_skb;
		return 1;	
	}

	*skb = new_skb; 

	return 0;
}


/*===============================================================
 * trace_intr
 *	
 *	Interrupt will be generated for each 
 *	available trace frame.  Copy the new trace
 *	frame into the trace queue.
 *
 *===============================================================*/

static void trace_intr(sdla_t *card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	TX25Trace *trace;
	int err;
	struct sk_buff *skb;
	unsigned char *buf;
	struct timeval tv;


	if (jiffies-card->u.x.trace_timeout > 3*HZ){
		wan_skb_queue_purge(&card->u.x.trace_queue);
		DEBUG_EVENT("%s: Trace Timeout: disabling trace\n",card->devname);
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = XPIPE_DISABLE_TRACING;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
		if (err){ 
			x25_error(card, err, XPIPE_DISABLE_TRACING, 0);
		}
		return;
	}

	memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
	mbox->wan_command = X25_READ_TRACE_DATA;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if (err){ 
		x25_error(card, err, X25_READ_TRACE_DATA, 0);
		return;
	}

	trace = (TX25Trace*)mbox->wan_data;	

	DEBUG_TEST("%s: Trace Interrupt Len %i\n",
			card->devname,trace->length);

	card->u.x.trace_lost_cnt+=trace->lost_cnt;
	
	/* Trace length contains the lapb+x25hader+crc = 7 */
	if (trace->length > X25_MAX_DATA+7){
		DEBUG_EVENT("%s: Error: Trace data overflow len=%i > max=%i\n",
				card->devname,trace->length,X25_MAX_DATA);
		card->u.x.trace_lost_cnt++;
		return;
	}

	if (wan_skb_queue_len(&card->u.x.trace_queue) > MAX_X25_TRACE_QUEUE){
		DEBUG_EVENT("%s: Error: Trace queue overflow len=%i\n",
				card->devname,MAX_X25_TRACE_QUEUE);
		card->u.x.trace_lost_cnt++;
		return;
	}

	if (trace->lost_cnt){
		DEBUG_EVENT("%s: Error: Firmware trace queue overflow  &m=%i\n",
				card->devname,trace->lost_cnt);
	}
	
	if (alloc_and_init_skb_buf(card,&skb,sizeof(trace_data_t)+trace->length)){
		card->u.x.trace_lost_cnt++;
		return;
	}
	
	buf = skb_put(skb,sizeof(trace_data_t));
	if (!buf){
		wan_skb_free(skb);
		card->u.x.trace_lost_cnt++;
		return;
	}

	trace->lost_cnt=card->u.x.trace_lost_cnt;
	card->u.x.trace_lost_cnt=0;

	memcpy(buf,trace,sizeof(TX25Trace));	

	/* Add an additional kernel timestamp */
	{
		trace_data_t *trace_hdr=(trace_data_t *)buf;
		do_gettimeofday(&tv);
		trace_hdr->sec=tv.tv_sec;
		trace_hdr->usec=tv.tv_usec;
	}
	
	if (trace->length){
		buf = skb_put(skb,trace->length);
		memcpy(buf,trace->data,trace->length);	
	}

	wan_skb_queue_tail(&card->u.x.trace_queue,skb);

	return;
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
	unsigned char more_to_tx=0;
	TX25Status status;
	u8	tmp;
	x25_channel_t *chan=NULL;
	int i=0;	

	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
#if 1	
	/* Try sending an async packet first */
	if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_CMD_EXEC){
		if (tx_intr_cmd_exec(card) == 0){
			if (atomic_read(&card->u.x.tx_interrupt_cmd) <= 0){
				atomic_set(&card->u.x.tx_interrupt_cmd,0);
				card->u.x.timer_int_enabled &=
					~TMR_INT_ENABLED_CMD_EXEC;
			}else{
				return;
			}
		}else{
			/* There are more commands pending
			 * re-trigger tx interrupt */
			return;
		}
	}
#endif

	if (card->u.x.tx_dev == NULL){
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		card->u.x.tx_dev = dev;
	}

	dev = card->u.x.tx_dev;

	for (;;){

		chan = wan_netif_priv(dev);
		if (!chan || !(dev->flags&IFF_UP)){
			dev = move_dev_to_next(card,dev);
			goto tx_dev_skip;
		}

		if (chan->transmit_length){
			/* Device was set to transmit, check if the TX
                         * buffers are available 
			 */		
			if (chan->common.state != WAN_CONNECTED){
				chan->transmit_length = 0;
				chan->tx_offset=0;
	
				++card->wandev.stats.tx_dropped;
				++chan->ifstats.tx_dropped;

				if (is_queue_stopped(dev)){
					if (chan->common.usedby == API){
						start_net_queue(dev);
						wakeup_sk_bh(dev);
					}else{
						wake_net_dev(dev);
					}
				}
				dev = move_dev_to_next(card,dev);
				break;
			}				

			card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &tmp, 1);
			if ((status.cflags[chan->ch_idx] & 0x40 || card->u.x.LAPB_hdlc) && 
			     (tmp & 0x40) ){
				/* Tx buffer available, we can send */
				
				if (tx_intr_send(card, dev)){
					more_to_tx=1;
				}

				/* If more than one interface present, move the
                                 * device pointer to the next interface, so on the 
                                 * next TX interrupt we will try sending from it. 
                                 */
				dev = move_dev_to_next(card,dev);
				break;
			}else{
				/* Tx buffers not available, but device set
                                 * the TX interrupt.  Set more_to_tx and try  
                                 * to transmit for other devices.
				 */
				more_to_tx=1;
				dev = move_dev_to_next(card,dev);
			}

		}else{
			/* This device was not set to transmit,
                         * go to next 
			 */
			dev = move_dev_to_next(card,dev);
		}	
tx_dev_skip:
		if (++i >= card->u.x.no_dev){
			if (!more_to_tx){
				DBG_PRINTK(KERN_INFO "%s: Nothing to Send in TX INTR\n",
					card->devname);
			}
			break;
		}

	} //End of FOR

	card->u.x.tx_dev = dev;
	
	if (!more_to_tx){
		/* if any other interfaces have transmit interrupts pending, */
		/* do not disable the global transmit interrupt */
		atomic_dec(&card->u.x.tx_interrupts_pending);
		if (atomic_read(&card->u.x.tx_interrupts_pending) <= 0){
			atomic_set(&card->u.x.tx_interrupts_pending,0);
			card->hw_iface.clear_bit(card->hw, card->intr_perm_off, INTR_ON_TX_FRAME); 
		}
	}
	return;
}

/*===============================================================
 * move_dev_to_next
 *  
 *
 *===============================================================*/


netdevice_t * move_dev_to_next (sdla_t *card, netdevice_t *dev)
{
	struct wan_dev_le	*devle;

	if (card->u.x.no_dev == 1){
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

/*===============================================================
 *  tx_intr_send
 *  
 *
 *===============================================================*/

static int tx_intr_send(sdla_t *card, netdevice_t *dev)
{
	x25_channel_t* chan = wan_netif_priv(dev); 

	if (chan_send (dev,chan->transmit_buffer,chan->transmit_length,1)){
		 
                /* Packet was split up due to its size, do not disable
                 * tx_intr 
                 */
		return 1;
	}

	chan->transmit_length=0;
	chan->tx_offset=0;

	/* If we are in API mode, wakeup the 
         * sock BH handler, not the NET_BH */
	if (is_queue_stopped(dev)){
		if (chan->common.usedby == API){
			start_net_queue(dev);
			wakeup_sk_bh(dev);
		}else{
			wake_net_dev(dev);
		}
	}
	return 0;
}


/*===============================================================
 * timer_intr
 *  
 * 	Timer interrupt handler.
 *	Check who called the timer interrupt and perform
 *      action accordingly.
 *
 *===============================================================*/

static void timer_intr (sdla_t *card)
{
	u8 tmp;

	DEBUG_TEST("%s: Timer inter 0x%X\n",
			card->devname,card->u.x.timer_int_enabled);
	
	if(card->u.x.timer_int_enabled & TMR_INT_ENABLED_UDP_PKT) {

		card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &tmp, 1); 
		if ((tmp & 0x40) && card->u.x.udp_type == UDP_XPIPE_TYPE){

                    	if(process_udp_mgmt_pkt(card,NULL)) {
		                card->u.x.timer_int_enabled &= 
					~TMR_INT_ENABLED_UDP_PKT;
			}
		}

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_ACTIVE) {

		netdevice_t *dev = card->u.x.poll_device;
		x25_channel_t *chan = NULL;

		if (!dev){
			card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_ACTIVE;
			return;
		}
		chan = wan_netif_priv(dev);

		printk(KERN_INFO 
			"%s: Closing down Idle link %s on LCN %d\n",
					card->devname,chan->name,chan->common.lcn); 
		chan->i_timeout_sofar = jiffies;
		chan_disc(dev);	
         	card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_ACTIVE;
		card->u.x.poll_device=NULL;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_CONNECT_ON) {

		wanpipe_set_state(card, WAN_CONNECTED);
		if (card->u.x.LAPB_hdlc){
			netdevice_t *dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			set_chan_state(dev,WAN_CONNECTED);
		}

		/* 0x8F enable all interrupts */
		x25_set_intr_mode(card, INTR_ON_RX_FRAME|	
					INTR_ON_TX_FRAME|
					INTR_ON_MODEM_STATUS_CHANGE|
					//INTR_ON_COMMAND_COMPLETE|
					X25_ASY_TRANS_INTR_PENDING |
					INTR_ON_TIMER |
					DIRECT_RX_INTR_USAGE |
					INTR_ON_TRACE_DATA
				); 

		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, INTR_ON_TX_FRAME);
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_CONNECT_ON;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_CONNECT_OFF) {

		DBG_PRINTK(KERN_INFO "Poll connect, Turning OFF\n");
		disconnect(card);
		if (card->u.x.LAPB_hdlc){
			netdevice_t *dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			/* FIXME: CRITICAL NO MAILBOX */
			send_oob_msg(card,dev,&card->wan_mbox);
			set_chan_state(dev,WAN_DISCONNECTED);
		}
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_CONNECT_OFF;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_DISCONNECT) {

		DBG_PRINTK(KERN_INFO "Poll disconnect, trying to connect\n");
		connect(card);
		if (card->u.x.LAPB_hdlc){
			netdevice_t *dev;
			dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			set_chan_state(dev,WAN_CONNECTED);
		}
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_DISCONNECT;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_UPDATE){

		card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &tmp, 1); 
		if (tmp & 0x40){
			x25_get_err_stats(card);
			x25_get_stats(card);
			card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_UPDATE;
		}
	}

	if(!(card->u.x.timer_int_enabled & ~TMR_INT_ENABLED_CMD_EXEC)){
		//printk(KERN_INFO "Turning Timer Off \n");
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, INTR_ON_TIMER);
	}
}

/*====================================================================
 * 	Modem status interrupt handler.
 *===================================================================*/
static void status_intr (sdla_t* card)
{

	/* Added to avoid Modem status message flooding */
	static TX25ModemStatus last_stat;

	wan_mbox_t* mbox = &card->wan_mbox;
	TX25ModemStatus *modem_status;
	netdevice_t *dev;
	x25_channel_t *chan;
	int err;

	memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
	mbox->wan_command = X25_READ_MODEM_STATUS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if (err){ 
		x25_error(card, err, X25_READ_MODEM_STATUS, 0);
	}else{
	
		modem_status = (TX25ModemStatus*)mbox->wan_data;	
	
           	/* Check if the last status was the same
           	 * if it was, do NOT print message again */
	
		if (last_stat.status != modem_status->status){

	     		printk(KERN_INFO "%s: Modem Status Change: DCD=%s, CTS=%s\n",
					card->devname,
					DCD(modem_status->status),
					CTS(modem_status->status));

			last_stat.status = modem_status->status;
		
			if (card->u.x.oob_on_modem){
				struct wan_dev_le	*devle;

				DBG_PRINTK(KERN_INFO "Modem status oob msg!\n");

				mbox->wan_x25_pktType = mbox->wan_command;
				mbox->wan_return_code = 0x08;

				/* Send a OOB to all connected sockets */
				WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
					dev = WAN_DEVLE2DEV(devle);
					chan=wan_netif_priv(dev);
					if (chan->common.usedby == API){
						send_oob_msg(card,dev,mbox);				
					}
				}
			}
		}
	}

	memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
	mbox->wan_command = X25_HDLC_LINK_STATUS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if (err){ 
		x25_error(card, err, X25_HDLC_LINK_STATUS, 0);
	}

}

/*====================================================================
 * 	Network event interrupt handler.
 *===================================================================*/
static void event_intr (sdla_t* card)
{
	x25_fetch_events(card);
}

/*====================================================================
 * 	Spurious interrupt handler.
 * 	o print a warning
 * 	o	 
 *====================================================================*/

static void spur_intr (sdla_t* card)
{
	printk(KERN_INFO "%s: spurious interrupt!\n", card->devname);
}


/*
 *	Background Polling Routines  
 */

/*====================================================================
 * 	Main polling routine.
 * 	This routine is repeatedly called by the WANPIPE 'thead' to allow for
 * 	time-dependent housekeeping work.
 *
 * 	Notes:
 * 	1. This routine may be called on interrupt context with all interrupts
 *    	enabled. Beware!
 *====================================================================*/

# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void wpx_poll (void *card_ptr)
#else
static void wpx_poll (struct work_struct *work) 
#endif
{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
        sdla_t 		*card = (sdla_t *)container_of(work, sdla_t, u.x.x25_poll_task);
#else
	sdla_t 		*card = (sdla_t *)card_ptr;
#endif    
	netdevice_t	*dev;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL)
		goto wpx_poll_exit;

	if (card->open_cnt != card->u.x.num_of_ch){
		goto wpx_poll_exit;
	}
	
	if (test_bit(PERI_CRIT,&card->wandev.critical)){
		goto wpx_poll_exit;
	}

	if (test_bit(SEND_CRIT,&card->wandev.critical)){
		goto wpx_poll_exit;
	}

	switch(card->wandev.state){
		case WAN_CONNECTED:
			poll_active(card);
			break;

		case WAN_CONNECTING:
			poll_connecting(card);
			break;

		case WAN_DISCONNECTED:
			poll_disconnected(card);
			break;
	}

wpx_poll_exit:
	clear_bit(POLL_CRIT,&card->wandev.critical);
	return;
}

static void trigger_x25_poll(sdla_t *card)
{
	WAN_TASKQ_SCHEDULE((&card->u.x.x25_poll_task));
}

/*====================================================================
 * 	Handle physical link establishment phase.
 * 	o if connection timed out, disconnect the link.
 *===================================================================*/

static void poll_connecting (sdla_t* card)
{
	unsigned char	gflags;

	card->hw_iface.peek(card->hw, card->flags_off + offsetof(TX25Status, gflags), &gflags, 1);

	DEBUG_TEST("%s: Poll Connecting 0x%X\n",card->devname,gflags);

	if (gflags & X25_HDLC_ABM){

		trigger_intr_exec (card, TMR_INT_ENABLED_POLL_CONNECT_ON);

	}else if ((jiffies - card->state_tick) > CONNECT_TIMEOUT){

		trigger_intr_exec (card, TMR_INT_ENABLED_POLL_CONNECT_OFF);

	}
}

/*====================================================================
 * 	Handle physical link disconnected phase.
 * 	o if hold-down timeout has expired and there are open interfaces, 
 *	connect link.
 *===================================================================*/

static void poll_disconnected (sdla_t* card)
{
	netdevice_t *dev; 
	x25_channel_t *chan;

	if (!card->u.x.LAPB_hdlc && card->open_cnt && 
	    ((jiffies - card->state_tick) > HOLD_DOWN_TIME)){
		trigger_intr_exec(card, TMR_INT_ENABLED_POLL_DISCONNECT);
	}

	if ((dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head))) == NULL)
		return;

	if ((chan=wan_netif_priv(dev)) == NULL)
		return;

}

/*====================================================================
 * 	Handle active link phase.
 * 	o fetch X.25 asynchronous events.
 * 	o kick off transmission on all interfaces.
 *===================================================================*/

static void poll_active (sdla_t* card)
{
	struct wan_dev_le	*devle;
	netdevice_t* dev;
	unsigned long flags;	
	wan_mbox_t* mbox = &card->wan_mbox;
	TX25Status status;
	unsigned char modem_status;
	unsigned char tx_timeout;

	card->hw_iface.peek(card->hw, X25_TX_TIMEOUT_OFFS, &tx_timeout, 1);
	if (tx_timeout){
		tx_timeout=0;
		card->hw_iface.poke(card->hw, X25_TX_TIMEOUT_OFFS, &tx_timeout, 1);
		DEBUG_EVENT("%s: Tx Timeout: no clock, disconnecting! \n",
					card->devname);

		spin_lock_irqsave(&card->wandev.lock, flags);
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_LINK_STATUS;
		card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
		spin_unlock_irqrestore(&card->wandev.lock, flags);
		
		x25_down_event (card, X25_HDLC_LINK_STATUS, 0, mbox,ASE_MODEM_DOWN);
		trigger_intr_exec(card, TMR_INT_ENABLED_POLL_DISCONNECT);
		return;
	}

	card->hw_iface.peek(card->hw, X25_MODEM_STATE_OFFS, &modem_status, 1);
	
	if (!(modem_status&X25_CTS_MASK) || !(modem_status&X25_DCD_MASK)){
		if (card->wandev.ignore_front_end_status == WANOPT_NO){

			DEBUG_EVENT("%s: Modem state down! DCD=%s CTS=%s\n",
					card->devname,
					DCD(modem_status),
					CTS(modem_status));

			spin_lock_irqsave(&card->wandev.lock, flags);
			memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
			mbox->wan_command = X25_HDLC_LINK_STATUS;
			card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
			spin_unlock_irqrestore(&card->wandev.lock, flags);
			
			x25_down_event (card, X25_HDLC_LINK_STATUS, 0, mbox,ASE_MODEM_DOWN);
			trigger_intr_exec(card, TMR_INT_ENABLED_POLL_DISCONNECT);
			return;
		}
	}
	
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	
	if (!(status.gflags & X25_HDLC_ABM)){

		DEBUG_EVENT("%s: Lapb hdlc link down!\n",card->devname);

		spin_lock_irqsave(&card->wandev.lock, flags);
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_LINK_STATUS;
		card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
		spin_unlock_irqrestore(&card->wandev.lock, flags);
		
		x25_down_event (card, X25_HDLC_LINK_STATUS, 0, mbox,ASE_LAPB_DOWN);
		trigger_intr_exec(card, TMR_INT_ENABLED_POLL_DISCONNECT);
		return;
	}


	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		x25_channel_t* chan;

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)) continue;
		chan = wan_netif_priv(dev);

		/* If SVC has been idle long enough, close virtual circuit */
		if ( chan->common.svc && 
		     chan->common.state == WAN_CONNECTED &&
		     chan->common.usedby == WANPIPE ){
		
			if( (jiffies - chan->i_timeout_sofar) / HZ > chan->idle_timeout ){
				/* Close svc */
				card->u.x.poll_device=dev;
				trigger_intr_exec (card, TMR_INT_ENABLED_POLL_ACTIVE);
			}
		}


		/* Check if the api aplication has gone
		 * away but failed to disconnect the svc */

		if (chan->common.state == WAN_CONNECTED &&
		    atomic_read(&chan->common.disconnect) &&
		    !chan->common.sk && chan->common.svc &&
		    chan->common.lcn){
		
			unsigned long flags;
			
			/* The Application tired to exit, but
			 * couldn't shut down the link via
			 * clear call.  Possibly due to flow
			 * control condition */

			DEBUG_EVENT("%s:%s: Poll Active trying to clear call on lcn %i\n",
					card->devname,chan->common.dev->name,chan->common.lcn);

	
			spin_lock_irqsave(&card->wandev.lock, flags);
			if (x25_clear_call(card,chan->common.lcn,
					   chan->x25_api_cmd_hdr.cause, 
				   	   chan->x25_api_cmd_hdr.diagn,0,NULL,0) == 0){
				atomic_set(&chan->common.disconnect,0);
			}
			spin_unlock_irqrestore(&card->wandev.lock, flags);

		}
	}
}

static void trigger_intr_exec(sdla_t *card, unsigned char TYPE)
{
	u8		intr_perm = 0x00;
	card->u.x.timer_int_enabled |= TYPE;

	if (TYPE == TMR_INT_ENABLED_CMD_EXEC){
		atomic_inc(&card->u.x.tx_interrupt_cmd);
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, INTR_ON_TX_FRAME);
	}else{
		card->hw_iface.peek(card->hw, card->intr_perm_off, &intr_perm, 1);
		if (!(intr_perm & INTR_ON_TIMER)){
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, INTR_ON_TIMER);
		}
	}
}

/*==================================================================== 
 * SDLA Firmware-Specific Functions 
 *
 *  Almost all X.25 commands can unexpetedly fail due to so called 'X.25
 *  asynchronous events' such as restart, interrupt, incoming call request,
 *  call clear request, etc.  They can't be ignored and have to be delt with
 *  immediately.  To tackle with this problem we execute each interface 
 *  command in a loop until good return code is received or maximum number 
 *  of retries is reached.  Each interface command returns non-zero return 
 *  code, an asynchronous event/error handler x25_error() is called.
 *====================================================================*/

/*====================================================================
 * 	Read X.25 firmware version.
 *		Put code version as ASCII string in str. 
 *===================================================================*/

static int x25_get_version (sdla_t* card, char* str)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_READ_CODE_VERSION;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, X25_READ_CODE_VERSION, 0) && retry--);

	if (!err && str)
	{
		int len = mbox->wan_data_len;

		memcpy(str, mbox->wan_data, len);
		str[len] = '\0';
	}
	return err;
}

/*====================================================================
 * 	Configure adapter.
 *===================================================================*/

static int x25_configure (sdla_t* card, TX25Config* conf)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		memcpy(mbox->wan_data, (void*)conf, sizeof(TX25Config));
		mbox->wan_data_len  = sizeof(TX25Config);
		mbox->wan_command = X25_SET_CONFIGURATION;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_SET_CONFIGURATION, 0) && retry-- );
	return err;
}

/*====================================================================
 * 	Configure adapter for HDLC only.
 *===================================================================*/

static int hdlc_configure (sdla_t* card, TX25Config* conf)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		memcpy(mbox->wan_data, (void*)conf, sizeof(TX25Config));
		mbox->wan_data_len  = sizeof(TX25Config);
		mbox->wan_command = X25_HDLC_SET_CONFIG;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, X25_SET_CONFIGURATION, 0) && retry--);

	return err;
}

static int set_hdlc_level (sdla_t* card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = SET_PROTOCOL_LEVEL;
		mbox->wan_data_len = 1;
		mbox->wan_data[0] = HDLC_LEVEL; //| DO_HDLC_LEVEL_ERROR_CHECKING; 	
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, SET_PROTOCOL_LEVEL, 0) && retry--);

	return err;
}



/*====================================================================
 * Get communications error statistics.
 *====================================================================*/

static int x25_get_err_stats (sdla_t* card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_READ_COMM_ERR;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, X25_HDLC_READ_COMM_ERR, 0) && retry--);
	
	if (!err)
	{
		THdlcCommErr* stats = (void*)mbox->wan_data;

		card->wandev.stats.rx_over_errors    = stats->rxOverrun;
		card->wandev.stats.rx_crc_errors     = stats->rxBadCrc;
		card->wandev.stats.rx_missed_errors  = stats->rxAborted;
		card->wandev.stats.tx_aborted_errors = stats->txAborted;
	}
	return err;
}

/*====================================================================
 * 	Get protocol statistics.
 *===================================================================*/

static int x25_get_stats (sdla_t* card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_READ_STATISTICS;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, X25_READ_STATISTICS, 0) && retry--) ;
	
	if (!err)
	{
		TX25Stats* stats = (void*)mbox->wan_data;

		card->wandev.stats.rx_packets = stats->rxData;
		card->wandev.stats.tx_packets = stats->txData;
		memcpy(&X25Stats, mbox->wan_data, mbox->wan_data_len);
	}
	return err;
}

/*====================================================================
 * 	Close HDLC link.
 *===================================================================*/

static int x25_close_hdlc (sdla_t* card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_LINK_CLOSE;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, X25_HDLC_LINK_CLOSE, 0) && retry--);
	
	return err;
}


/*====================================================================
 * 	Open HDLC link.
 *===================================================================*/

static int x25_open_hdlc (sdla_t* card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_LINK_OPEN;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_HDLC_LINK_OPEN, 0) && retry--);

	return err;
}

/*=====================================================================
 * Setup HDLC link.
 *====================================================================*/
static int x25_setup_hdlc (sdla_t* card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_LINK_SETUP;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_HDLC_LINK_SETUP, 0) && retry--);
	
	return err;
}

/*====================================================================
 * Set (raise/drop) DTR.
 *===================================================================*/

static int x25_set_dtr (sdla_t* card, int dtr)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_data[0] = 0;
		mbox->wan_data[2] = 0;
		mbox->wan_data[1] = dtr ? 0x02 : 0x01;
		mbox->wan_data_len  = 3;
		mbox->wan_command = X25_SET_GLOBAL_VARS;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_SET_GLOBAL_VARS, 0) && retry-- );
	
	return err;
}

/*====================================================================
 * 	Set interrupt mode.
 *===================================================================*/

static int x25_set_intr_mode (sdla_t* card, int mode)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_data[0] = mode;
		mbox->wan_data[1] = card->wandev.irq;	// ALEX_TODAY card->hw.irq;
		mbox->wan_data[2] = 2;
		mbox->wan_data_len = 3;
		mbox->wan_command = X25_SET_INTERRUPT_MODE;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_SET_INTERRUPT_MODE, 0) && retry--);
	
	return err;
}

/*====================================================================
 * 	Read X.25 channel configuration.
 *===================================================================*/

static int x25_get_chan_conf (sdla_t* card, x25_channel_t* chan)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int lcn = chan->common.lcn;
	int err;

	do{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_x25_lcn     = lcn;
		mbox->wan_command = X25_READ_CHANNEL_CONFIG;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_READ_CHANNEL_CONFIG, lcn) && retry--);

	if (!err){
		TX25Status status;

		card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
		/* calculate an offset into the array of status bytes */
		if (card->u.x.x25_conf.hi_svc <= X25_MAX_CHAN){ 

			chan->ch_idx = lcn - 1;

		}else{
			int offset;

			/* FIX: Apr 14 2000 : Nenad Corbic
			 * The data field was being compared to 0x1F using
                         * '&&' instead of '&'. 
			 * This caused X25API to fail for LCNs greater than 255.
			 */
			switch (mbox->wan_data[0] & 0x1F)
			{
				case 0x01: 
					offset = status.pvc_map; break;
				case 0x03: 
					offset = status.icc_map; break;
				case 0x07: 
					offset = status.twc_map; break;
				case 0x0B: 
					offset = status.ogc_map; break;
				default: 
					offset = 0;
			}
			chan->ch_idx = lcn - 1 - offset;
		}

		/* get actual transmit packet size on this channel */
		switch(mbox->wan_data[1] & 0x38)
		{
			case 0x00: 
				chan->tx_pkt_size = 16; 
				break;
			case 0x08: 
				chan->tx_pkt_size = 32; 
				break;
			case 0x10: 
				chan->tx_pkt_size = 64; 
				break;
			case 0x18: 
				chan->tx_pkt_size = 128; 
				break;
			case 0x20: 
				chan->tx_pkt_size = 256; 
				break;
			case 0x28: 
				chan->tx_pkt_size = 512; 
				break;
			case 0x30: 
				chan->tx_pkt_size = 1024; 
				break;
		}
		if (card->u.x.logging)
			printk(KERN_INFO "%s: X.25 packet size on LCN %d is %d.\n",
				card->devname, lcn, chan->tx_pkt_size);
	}
	return err;
}

/*====================================================================
 * 	Place X.25 call.
 *====================================================================*/

static int x25_place_call (sdla_t* card, x25_channel_t* chan)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	if (chan->common.usedby == WANPIPE){
		if (chan->protocol == htons(ETH_P_IP)){

			if(strcmp(&chan->x25_src_addr[0], "") == 0){
				sprintf(chan->call_string, "-d%s -uCC", chan->addr);
			}else{
				//the source address was supplied
				sprintf(chan->call_string, "-d%s -s%s -uCC", 
						chan->addr, 
						chan->x25_src_addr);
			}
				
			printk(KERN_INFO "%s: Call String : %s \n", card->devname, chan->call_string);

		}else if (chan->protocol == htons(ETH_P_IPX)){
			sprintf(chan->call_string, "-d%s -u800000008137", chan->addr);
		
		}
	
	}
	
	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		strcpy(mbox->wan_data, chan->call_string);
		mbox->wan_data_len  = strlen(chan->call_string);
		mbox->wan_command = X25_PLACE_CALL;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err  && x25_error(card, err, X25_PLACE_CALL, 0) && retry--);

	if (!err){
		bind_lcn_to_dev (card, chan->common.dev, mbox->wan_x25_lcn);
	}

	chan->x25_api_event.hdr.qdm = mbox->wan_x25_qdm;
	chan->x25_api_event.hdr.cause = mbox->wan_x25_cause;
	chan->x25_api_event.hdr.diagn = mbox->wan_x25_diagn;
	chan->x25_api_event.hdr.pktType = mbox->wan_x25_pktType&0x7F;
	chan->x25_api_event.hdr.length  = 0;
	chan->x25_api_event.hdr.result = 0;
	chan->x25_api_event.hdr.lcn = mbox->wan_x25_lcn;
	chan->x25_api_event.hdr.mtu = card->u.x.x25_conf.defPktSize;
	chan->x25_api_event.hdr.mru = card->u.x.x25_conf.defPktSize;
	
	
	return err;
}

/*====================================================================
 * 	Accept X.25 call.
 *====================================================================*/

static int x25_accept_call (sdla_t* card, int lcn, int qdm, unsigned char *data, int len)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_x25_lcn     = lcn;
		mbox->wan_x25_qdm     = qdm;
		mbox->wan_command = X25_ACCEPT_CALL;
		if (len){
			mbox->wan_data_len = len;
			strncpy(mbox->wan_data,data,len);
		}
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_ACCEPT_CALL, lcn) && retry--);
	
	return err;
}

static int x25_interrupt (sdla_t* card, int lcn, int qdm, unsigned char *data, int len)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_x25_lcn     = lcn;
		mbox->wan_x25_qdm     = qdm;
		mbox->wan_command = X25_WP_INTERRUPT;
		if (len){
			mbox->wan_data_len = len;
			strncpy(mbox->wan_data,data,len);
		}
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_WP_INTERRUPT, lcn) && retry--);
	
	return err;
}


/*====================================================================
 * 	Clear X.25 call.
 *====================================================================*/

static int x25_clear_call (sdla_t* card, int lcn, int cause, 
		           int diagn, int qdm, 
			   unsigned char *data, int len)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	DEBUG_TEST("%s:%s: LCN=%i QDM %x\n",__FUNCTION__,card->devname,lcn,qdm);
	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_x25_qdm     = qdm;
		mbox->wan_x25_lcn     = lcn;
		mbox->wan_x25_cause   = cause;
		mbox->wan_x25_diagn   = diagn;
		mbox->wan_command = X25_CLEAR_CALL;
		if (len){
			mbox->wan_data_len = len;
			strncpy(mbox->wan_data,data,len);
		}
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	} while (err && x25_error(card, err, X25_CLEAR_CALL, lcn) && retry--);

	return err;
}

static int x25_reset_call (sdla_t* card, int lcn, int cause, int diagn, int qdm)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	DEBUG_TEST("%s:%s: LCN=%i QDM %x\n",__FUNCTION__,card->devname,lcn,qdm);
	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_x25_qdm     = qdm;
		mbox->wan_x25_lcn     = lcn;
		mbox->wan_x25_cause   = cause;
		mbox->wan_x25_diagn   = diagn;
		mbox->wan_command = X25_RESET;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	} while (err && x25_error(card, err, X25_RESET, lcn) && retry--);

	return err;
}


/*====================================================================
 * 	Send X.25 data packet.
 *====================================================================*/

static int x25_send (sdla_t* card, int lcn, int qdm, int len, void* buf)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;
	unsigned char cmd;
		
	if (card->u.x.LAPB_hdlc)
		cmd = X25_HDLC_WRITE;
	else
		cmd = X25_WRITE;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		memcpy(mbox->wan_data, buf, len);
		mbox->wan_data_len  = (unsigned short)len;
		mbox->wan_x25_lcn     = (unsigned short)lcn;

		if (card->u.x.LAPB_hdlc){
			mbox->wan_x25_pf = (unsigned char)qdm;
		}else{			
			mbox->wan_x25_qdm = (unsigned char)qdm;
		}
		
		mbox->wan_command = cmd;
		
		/* The P6-PCI caching used to be a bug on old 
		 * S514 prototype boards.  This has been fixed,
		 * however the check below is a sanity check */
		if (mbox->wan_opp_flag){
			printk(KERN_INFO "\n");
			printk(KERN_INFO "%s: Critical Error: P6-PCI Cache bug!\n",
					card->devname);
			printk(KERN_INFO "%s: Please contact Sangoma Tech Support!\n\n",
					card->devname);
		}
		
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	} while (err && x25_error(card, err, cmd , lcn) && retry--);


	/* If buffers are busy the return code for LAPB HDLC is
         * 1. The above functions are looking for return code
         * of X25RES_NOT_READY if busy. */
	if (card->u.x.LAPB_hdlc && err == 1){
		err = X25RES_NOT_READY;
	}

	return err;
}

/*====================================================================
 * 	Fetch X.25 asynchronous events.
 *===================================================================*/

static int x25_fetch_events (sdla_t* card)
{
	unsigned char	gflags;
	wan_mbox_t* mbox = &card->wan_mbox;
	int err = 0;

	card->hw_iface.peek(card->hw, card->flags_off + offsetof(TX25Status, gflags), &gflags, 1);
	if (gflags & 0x20)
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_IS_DATA_AVAILABLE;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
 		if (err) x25_error(card, err, X25_IS_DATA_AVAILABLE, 0);
	}
	return err;
}

/*====================================================================
 * 	X.25 asynchronous event/error handler.
 *		This routine is called each time interface command returns 
 *		non-zero return code to handle X.25 asynchronous events and 
 *		common errors. Return non-zero to repeat command or zero to 
 *		cancel it.
 *
 * 	Notes:
 * 	1. This function may be called recursively, as handling some of the
 *    	asynchronous events (e.g. call request) requires execution of the
 *    	interface command(s) that, in turn, may also return asynchronous
 *    	events.  To avoid re-entrancy problems we copy mailbox to dynamically
 *    	allocated memory before processing events.
 *====================================================================*/

static int x25_error (sdla_t* card, int err, int cmd, int lcn)
{
	int retry = 1;
	wan_mbox_t* mb = &card->wan_mbox;
	unsigned dlen = mb->wan_data_len;

#if 0
	mb = kmalloc(sizeof(wan_mbox_t) + dlen, GFP_ATOMIC);
	if (mb == NULL)
	{
		printk(KERN_INFO "%s: x25_error() out of memory!\n",
			card->devname);
		return 0;
	}
	memcpy(mb, card->mbox, sizeof(wan_mbox_t) + dlen);
#endif	
	
	switch (err){

	case X25RES_ASYNC_PACKET:	/* X.25 asynchronous packet was received */

		mb->wan_data[dlen] = '\0';

		switch (mb->wan_x25_pktType & 0x7F){


		case ASE_CALL_RQST:		/* incoming call */
			retry = incoming_call(card, cmd, lcn, mb);
			break;

		case ASE_CALL_ACCEPTED:		/* connected */
			retry = call_accepted(card, cmd, lcn, mb);
			break;

		case ASE_CLEAR_RQST:		/* call clear request */
			retry = call_cleared(card, cmd, lcn, mb);
			x25_clear_cmd_update(card, lcn, mb);
			break;

		case ASE_RESET_RQST:		/* reset request */
			printk(KERN_INFO "%s: X.25 reset request on LCN %d! "
				"Cause:0x%02X Diagn:0x%02X\n",
				card->devname, mb->wan_x25_lcn, mb->wan_x25_cause,
				mb->wan_x25_diagn);
			api_oob_event (card,mb);
			break;

		case ASE_RESTART_RQST:		/* restart request */
			retry = restart_event(card, cmd, lcn, mb);
			break;

		case ASE_CLEAR_CONFRM:
			clear_confirm_event (card,mb);
			break;

		default:
			printk(KERN_INFO "%s: Rx X.25 event 0x%02X on LCN %d! "
				"Cause:0x%02X Diagn:0x%02X\n",
				card->devname, mb->wan_x25_pktType&0x7F,
				mb->wan_x25_lcn, mb->wan_x25_cause, mb->wan_x25_diagn);
			api_oob_event (card,mb);
		}
		break;

	case X25RES_PROTO_VIOLATION:	/* X.25 protocol violation indication */

		/* Bug Fix: Mar 14 2000
                 * The Protocol violation error conditions were  
                 * not handeled previously */

		switch (mb->wan_x25_pktType & 0x7F){

		case PVE_CLEAR_RQST:	/* Clear request */		
			retry = call_cleared(card, cmd, lcn, mb);
			break;	

		case PVE_RESET_RQST:	/* Reset request */
			printk(KERN_INFO "%s: Rx X.25 reset request on LCN %d! "
				"Cause:0x%02X Diagn:0x%02X\n",
				card->devname, mb->wan_x25_lcn, mb->wan_x25_cause,
				mb->wan_x25_diagn);
			api_oob_event (card,mb);
			break;

		case PVE_RESTART_RQST:	/* Restart request */
			retry = restart_event(card, cmd, lcn, mb);
			break;

		default :
			printk(KERN_INFO
				"%s: X.25 protocol violation on LCN %d! "
				"Packet:0x%02X Cause:0x%02X Diagn:0x%02X\n",
				card->devname, mb->wan_x25_lcn,
				mb->wan_x25_pktType & 0x7F, mb->wan_x25_cause, mb->wan_x25_diagn);
			api_oob_event(card,mb);
		}
		break;

	case 0x42:	/* X.25 timeout */
		retry = timeout_event(card, cmd, lcn, mb);
		break;

	case 0x43:	/* X.25 retry limit exceeded */
		printk(KERN_INFO
			"%s: exceeded X.25 retry limit on LCN %d! "
			"Packet:0x%02X Diagn:0x%02X\n", card->devname,
			mb->wan_x25_lcn, mb->wan_x25_pktType&0x7F, mb->wan_x25_diagn);
		break;

	case 0x08:	/* modem failure */
		api_oob_event(card,mb);
		break;

	case 0x09:	/* N2 retry limit */
		printk(KERN_INFO "%s: exceeded HDLC retry limit!\n",
			card->devname);
		x25_down_event(card,cmd,lcn,mb,ASE_LAPB_DOWN);
		break;

	case 0x06:	/* unnumbered frame was received while in ABM */
		printk(KERN_INFO "%s: Received Unnumbered frame 0x%02X!\n",
			card->devname, mb->wan_data[0]);
		api_oob_event(card,mb);
		break;

	case CMD_TIMEOUT:
		printk(KERN_INFO "%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		retry = 0;	/* abort command */
		break;

	case X25RES_NOT_READY:
		retry = 1;
		break;

	case 0x01:
		if (card->u.x.LAPB_hdlc)
			break;

		if (mb->wan_command == 0x16)
			break;

		if (mb->wan_command == 0x06)
			break;

		/* I use the goto statement here so if 
                 * somebody inserts code between the
                 * case and default, we will not have
                 * ghost problems */
		goto dflt_2;

	case X25RES_CHANNEL_IN_USE:

		if (cmd == X25_CLEAR_CALL){
			retry =0;
			break;
		}

	default:
dflt_2:
		printk(KERN_INFO "%s: command 0x%02X returned 0x%02X! Lcn %i\n",
			card->devname, cmd, err, mb->wan_x25_lcn)
		;
		retry = 0;	/* abort command */
	}

//	kfree(mb);

	return retry;
}

/*==================================================================== 
 *	X.25 Asynchronous Event Handlers
 * 	These functions are called by the x25_error() and should return 0, if
 * 	the command resulting in the asynchronous event must be aborted.
 *====================================================================*/



/*====================================================================
 *Handle X.25 incoming call request.
 *	RFC 1356 establishes the following rules:
 *	1. The first octet in the Call User Data (CUD) field of the call
 *     	   request packet contains NLPID identifying protocol encapsulation
 * 	2. Calls MUST NOT be accepted unless router supports requested
 *   	   protocol encapsulation.
 *	3. A diagnostic code 249 defined by ISO/IEC 8208 may be used 
 *	   when clearing a call because protocol encapsulation is not 
 *	   supported.
 *	4. If an incoming call is received while a call request is 
 *	   pending (i.e. call collision has occured), the incoming call 
 *	   shall be rejected and call request shall be retried.
 *====================================================================*/

static int incoming_call (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	struct wan_dev_le	*devle;
	wan_device_t* wandev = &card->wandev;
	int new_lcn = mb->wan_x25_lcn;
	netdevice_t* dev = get_dev_by_lcn(wandev, new_lcn);
	x25_channel_t* chan = NULL;
	int accept = 0;		/* set to '1' if o.k. to accept call */
	unsigned int user_data;
	x25_call_info_t* info;
	
	/* Make sure there is no call collision */
	if (dev != NULL)
	{
		printk(KERN_INFO
			"%s: X.25 incoming call collision on LCN %d!\n",
			card->devname, new_lcn);

		x25_clear_call(card, new_lcn, 0, 0, 0, NULL,0);
		return 1;
	}


	/* Parse call request data */
	info = kmalloc(sizeof(x25_call_info_t), GFP_ATOMIC);
	if (info == NULL)
	{
		printk(KERN_INFO
			"%s: not enough memory to parse X.25 incoming call "
			"on LCN %d!\n", card->devname, new_lcn);
		x25_clear_call(card, new_lcn, 0, 0, 0, NULL, 0);
		return 1;
	}
 
	parse_call_info(mb->wan_data, info);

	if (card->u.x.logging){
		printk(KERN_INFO "\n");
		printk(KERN_INFO "%s: X.25 incoming call on LCN %d!\n",
			card->devname, new_lcn);
	}
	/* Convert the first two ASCII characters into an
         * interger. Used to check the incoming protocol 
         */
	user_data = hex_to_uint(info->user,2);

	/* Find available channel */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)){
			continue;
		}
		
		chan = wan_netif_priv(dev);

		if (chan->common.usedby == API){
			dev=NULL;
			continue;
		}

		if (!chan->common.svc || (chan->common.state != WAN_DISCONNECTED)){
			dev=NULL;
			continue;
		}

		if (chan->protocol == htons(ETH_P_IP) && user_data != NLPID_IP){
			dev=NULL;
			continue;
		}

		if (chan->protocol == htons(ETH_P_IPX) && user_data != NLPID_SNAP){
			dev=NULL;
			continue;
		}
		
		if (user_data == NLPID_IP && chan->protocol != htons(ETH_P_IP)){
			printk(KERN_INFO "IP packet but configured for IPX : %x, %x\n",
				       htons(chan->protocol), info->user[0]);
			dev=NULL;
			continue;
		}
	
		if (user_data == NLPID_SNAP && chan->protocol != htons(ETH_P_IPX)){
			printk(KERN_INFO "IPX packet but configured for IP: %x\n",
				       htons(chan->protocol));
			dev=NULL;
			continue;
		}
		
		if (is_match_found(info->dest, chan->accept_dest_addr) &&
		    is_match_found(info->src, chan->accept_src_addr) &&
		    is_match_found(&info->user[2], chan->accept_usr_data)){
			break;
		}

		dev=NULL;
	}

	if (dev == NULL){

		/* If the call is not for any WANPIPE interfaces
                 * check to see if there is an API listening queue
                 * waiting for data. If there is send the packet
                 * up the stack.
                 */
		if (card->sk != NULL){
			if (api_incoming_call(card,mb,new_lcn)){
				DEBUG_EVENT("%s: Incoming call failed !\n", 
						card->devname);
				x25_clear_call(card, new_lcn, 0, 0x40, 0, NULL, 0);
			}
			accept = 0;
		}else{
			printk(KERN_INFO "%s: no channels available!\n",
				card->devname);
			
			x25_clear_call(card, new_lcn, 0, 0x40, 0, NULL, 0);
		}

	}else if (info->nuser == 0){

		printk(KERN_INFO
			"%s: no user data in incoming call on LCN %d!\n",
			card->devname, new_lcn)
		;
		x25_clear_call(card, new_lcn, 0, 0,0, NULL, 0);

	}else switch (user_data){

		case 0:		/* multiplexed */
			chan->protocol = htons(0);
			accept = 1;
			break;

		case NLPID_IP:	/* IP datagrams */
			accept = 1;
			break;

		case NLPID_SNAP: /* IPX datagrams */
			accept = 1;
			break;

		default:
			printk(KERN_INFO
				"%s: unsupported NLPID 0x%x in incoming call "
				"on LCN %d!\n", card->devname, user_data, new_lcn);
			x25_clear_call(card, new_lcn, 0, 249,0,NULL,0);
	}
	
	if (accept && (x25_accept_call(card, new_lcn, 0, NULL, 0) == CMD_OK)){

		bind_lcn_to_dev (card, chan->common.dev, new_lcn);
		
		if (x25_get_chan_conf(card, chan) == CMD_OK)
			set_chan_state(dev, WAN_CONNECTED);
		else 
			x25_clear_call(card, new_lcn, 0, 0, 0, NULL, 0);
	}
	kfree(info);
	return 1;
}

/*====================================================================
 * 	Pattern matching function in incoming calls.
 *====================================================================*/

static int is_match_found(unsigned char info[], unsigned char chan[])
{
	unsigned char temp[WAN_ADDRESS_SZ+1];

	/* accept all calls */
	if (strcmp(chan, "*") == 0)	return 1;
	
	/* case ?? - must exactly match */
	if (chan[strlen(chan) - 1] != '*' && chan[0] != '*' && strlen(chan)){
		if (strcmp(info, chan) == 0)	return 1;
	}
		
	/* case ??* must match at the begining */
	if ( chan[0] != '*' && chan[strlen(chan) - 1] == '*'){
		if (strncmp(info, &chan[0], strlen(chan) - 1) == 0)	return 1;
	}
		
	/* case *?? must match at the end */
	if ( chan[0] == '*' && chan[strlen(chan) - 1] != '*'){	
		if (strncmp(&info[strlen(info) - strlen(chan)+1], &chan[1], strlen(chan) - 1) == 0)
			return 1;
	}
		
	/* case *??* must match somewhere in the string */
	if ( chan[0] == '*' && chan[strlen(chan) - 1] == '*'){
			
		strncpy(temp, &chan[1], strlen(chan) - 2);
		/* null terminate for strstr() */
		temp[strlen(chan) - 2] = '\0';
			
		if (strstr(info, temp))	return 1;
	}

	/* if got here the string is empty - do not accept call */
	return 0;
}

/*====================================================================
 * 	Handle accepted call.
 *====================================================================*/

static int call_accepted (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	unsigned new_lcn = mb->wan_x25_lcn;
	netdevice_t* dev = find_channel(card, new_lcn);
	x25_channel_t* chan;

	if (dev == NULL){
		printk(KERN_INFO
			"%s: clearing orphaned connection on LCN %d!\n",
			card->devname, new_lcn);
		x25_clear_call(card, new_lcn, 0, 0, 0, NULL, 0);
		return 1;
	}

	if (card->u.x.logging)	
		printk(KERN_INFO "%s: Rx X.25 call accept on Dev %s and Lcn %d!\n",
			card->devname, dev->name, new_lcn);

	/* Get channel configuration and notify router */
	chan = wan_netif_priv(dev);
	if (x25_get_chan_conf(card, chan) != CMD_OK)
	{
		x25_clear_call(card, new_lcn, 0, 0, 0, NULL,0);
		return 1;
	}

	if (chan->common.usedby == API){

		chan->x25_api_event.hdr.qdm = mb->wan_x25_qdm;
		chan->x25_api_event.hdr.cause = mb->wan_x25_cause;
		chan->x25_api_event.hdr.diagn = mb->wan_x25_diagn;
		chan->x25_api_event.hdr.pktType = mb->wan_x25_pktType&0x7F;
		chan->x25_api_event.hdr.length  = 0;
		chan->x25_api_event.hdr.result = 0;
		chan->x25_api_event.hdr.lcn = mb->wan_x25_lcn;
		chan->x25_api_event.hdr.mtu = card->u.x.x25_conf.defPktSize;
		chan->x25_api_event.hdr.mru = card->u.x.x25_conf.defPktSize;
	}

	set_chan_state(dev, WAN_CONNECTED);

	return 1;
}

/*====================================================================
 * 	Handle cleared call.
 *====================================================================*/

static int call_cleared (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	unsigned new_lcn = mb->wan_x25_lcn;
	netdevice_t* dev = find_channel(card, new_lcn);
	x25_channel_t *chan;
	unsigned char old_state;

	if (card->u.x.logging){
		printk(KERN_INFO "%s: Rx X.25 clear request on Lcn %d! Cause:0x%02X "
		"Diagn:0x%02X\n",
		card->devname, new_lcn, mb->wan_x25_cause, mb->wan_x25_diagn);
	}

	if (dev == NULL){ 
		printk(KERN_INFO "%s: X.25 clear request : No device for clear\n",
				card->devname);
		return 1;
	}

	chan=wan_netif_priv(dev);

	old_state = chan->common.state;
	

	if (chan->common.usedby == API){
		send_oob_msg(card,dev,mb);				
	}

	set_chan_state(dev, WAN_DISCONNECTED);

	return ((cmd == X25_WRITE) && (lcn == new_lcn)) ? 0 : 1;
}

/*====================================================================
 * 	Handle X.25 restart event.
 *====================================================================*/

static int restart_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	struct wan_dev_le	*devle;
	netdevice_t* dev;
	x25_channel_t *chan;
	unsigned char old_state,tx_timeout=0;
	TX25Status status;

	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	printk(KERN_INFO
		"%s: Rx X.25 restart request! Cause:0x%02X Diagn:0x%02X\n",
		card->devname, mb->wan_x25_cause, mb->wan_x25_diagn);

	/* After a restart, force the tx timeout to zero
	 * so the polling code doesn't mistake this condition
	 * for loss of clock */
	card->hw_iface.poke(card->hw, X25_TX_TIMEOUT_OFFS, &tx_timeout, 1);

	/* down all logical channels */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)) continue;
		chan = wan_netif_priv(dev);
		old_state = chan->common.state;

		if (chan->common.usedby == API){
			send_oob_msg(card,dev,mb);				
		}

		if (chan->common.svc){
			set_chan_state(dev, WAN_DISCONNECTED);
		}else{
			/* PVC in WANPIPE mode always stays 
			 * connected.  The PVC in API mode
			 * should not have a state change, since
			 * the connection of the socket activates
			 * the interface */
			if (chan->common.usedby == WANPIPE){
				set_chan_state(dev, WAN_CONNECTED);
			}
		}

		atomic_set(&chan->common.command,0);
	}

	if (card->sk){
		wanpipe_api_listen_rx(NULL,card->sk);
		/* NC: Allow user listen socket to 
		 * stay connected to the driver, this way
		 * no incomint calls would be missed when
		 * the x25 layer comes back up */
		/*
		sock_put(card->sk);
		card->sk=NULL;
		*/
	}


	/* We use the rx interrupt as x25 flow
	 * control. If we ever get a restart request
	 * make sure the rx interrupt is 
	 * re-enabled */
	if (!(status.imask & INTR_ON_RX_FRAME)){
		status.imask |= INTR_ON_RX_FRAME;
		card->hw_iface.poke(card->hw, 
				    card->intr_perm_off,
				    &status.imask,
				    sizeof(status.imask));
	}

	return (cmd == X25_WRITE) ? 0 : 1;
}


static int x25_down_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb, unsigned char x25_pkt_type )
{
	struct wan_dev_le	*devle;	
	netdevice_t* dev;
	x25_channel_t *chan;
	unsigned char old_state;
	TX25Status status;

	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));

	printk(KERN_INFO
		"%s: Lapb layer disconnected!\n",card->devname);

	clear_bit(0,&card->u.x.card_ready);

	/* down all logical channels */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev)) continue;
		chan = wan_netif_priv(dev);
		old_state = chan->common.state;

		if (chan->common.usedby == API){
			mb->wan_x25_pktType=x25_pkt_type;
			send_oob_msg(card,dev,mb);				
		}

		set_chan_state(dev, WAN_DISCONNECTED);
	}

	if (card->sk){
		wanpipe_api_listen_rx(NULL,card->sk);
		/* NC: Allow user listen socket to 
		 * stay connected to the driver, this way
		 * no incomint calls would be missed when
		 * the x25 layer comes back up */
		/*
		sock_put(card->sk);
		card->sk=NULL;
		*/
	}


	/* We use the rx interrupt as x25 flow
	 * control. If we ever get a restart request
	 * make sure the rx interrupt is 
	 * re-enabled */
	if (!(status.imask & INTR_ON_RX_FRAME)){
		status.imask |= INTR_ON_RX_FRAME;
		card->hw_iface.poke(card->hw, 
				    card->intr_perm_off,
				    &status.imask,
				    sizeof(status.imask));
 	}
 
 	return (cmd == X25_WRITE) ? 0 : 1;
 }


/*====================================================================
 * Handle timeout event.
 *====================================================================*/

static int timeout_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	unsigned new_lcn = mb->wan_x25_lcn;

	if ((mb->wan_x25_pktType&0x7F) == 0x05)	/* call request time out */
	{
		netdevice_t* dev = find_channel(card,new_lcn);

		printk(KERN_INFO "%s: X.25 call timed timeout on LCN %d!\n",
			card->devname, new_lcn);

		if (dev){
			set_chan_state(dev, WAN_DISCONNECTED);
		}
	}else{ 
		printk(KERN_INFO "%s: X.25 packet 0x%02X timeout on LCN %d!\n",
		card->devname, mb->wan_x25_pktType&0x7F, new_lcn);
	}
	return 1;
}

/* 
 *	Miscellaneous 
 */

/*====================================================================
 * 	Establish physical connection.
 * 	o open HDLC and raise DTR
 *
 * 	Return:		0	connection established
 *			1	connection is in progress
 *			<0	error
 *===================================================================*/

static int connect (sdla_t* card)
{
	if (x25_open_hdlc(card) || x25_setup_hdlc(card)){
		DEBUG_EVENT("%s: Failed to open hdlc and setup hdlc\n",card->devname);
		return -EIO;
	}

	wanpipe_set_state(card, WAN_CONNECTING);

	x25_set_intr_mode(card, (INTR_ON_TIMER|INTR_ON_TRACE_DATA)); 
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, INTR_ON_TIMER);
	return 0;
}

/*
 * 	Tear down physical connection.
 * 	o close HDLC link
 * 	o drop DTR
 *
 * 	Return:		0
 *			<0	error
 */

static int disconnect (sdla_t* card)
{
	wanpipe_set_state(card, WAN_DISCONNECTED);
	
	/* disable all interrupt except timer & trace */
	x25_set_intr_mode(card, INTR_ON_TIMER|INTR_ON_TRACE_DATA);
	x25_close_hdlc(card);			/* close HDLC link */
	x25_set_dtr(card, 0);			/* drop DTR */

	return 0;
}

/*
 * Find network device by its channel number.
 */

static netdevice_t* get_dev_by_lcn (wan_device_t* wandev, unsigned lcn)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev = NULL;

	WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (((x25_channel_t*)wan_netif_priv(dev))->common.lcn == lcn){ 
			return dev;
		}
	}
	return NULL;
}

/*
 * 	Initiate connection on the logical channel.
 * 	o for PVC we just get channel configuration
 * 	o for SVCs place an X.25 call
 *
 * 	Return:		0	connected
 *			>0	connection in progress
 *			<0	failure
 */

static int chan_connect (netdevice_t* dev)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;

	if (chan->common.svc && chan->common.usedby == WANPIPE){
		if (!chan->addr[0]){
			printk(KERN_INFO "%s: No Destination Address\n",
					card->devname);
			return -EINVAL;	/* no destination address */
		}
		printk(KERN_INFO "%s: placing X.25 call to %s ...\n",
			card->devname, chan->addr);

		if (x25_place_call(card, chan) != CMD_OK)
			return -EIO;

		set_chan_state(dev, WAN_CONNECTING);
		return 1;
	}else{
		if (x25_get_chan_conf(card, chan) != CMD_OK)
			return -EIO;

		set_chan_state(dev, WAN_CONNECTED);
	}
	return 0;
}

/*
 * 	Disconnect logical channel.
 * 	o if SVC then clear X.25 call
 */

static int chan_disc (netdevice_t* dev)
{
	x25_channel_t* chan = wan_netif_priv(dev);

	if (chan->common.svc){ 
		x25_clear_call(chan->card, chan->common.lcn, 0, 0, 0,NULL,0);

		/* For API we disconnect on clear
                 * confirmation. 
                 */
		if (chan->common.usedby == API)
			return 0;
	}

	set_chan_state(dev, WAN_DISCONNECTED);
	
	return 0;
}

/*
 * 	Set logical channel state.
 */

static void set_chan_state (netdevice_t* dev, int state)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	
	if (chan->common.state != state)
	{
		switch (state)
		{
			case WAN_CONNECTED:
				if (card->u.x.logging){
					printk (KERN_INFO 
						"%s: interface %s connected, lcn %i !\n", 
						card->devname, dev->name,chan->common.lcn);
				}
				*(unsigned short*)dev->dev_addr = htons(chan->common.lcn);
				chan->i_timeout_sofar = jiffies;

				/* LAPB is PVC Based */
				if (card->u.x.LAPB_hdlc){
					chan->common.svc=0;
				}
				break;

			case WAN_CONNECTING:
				if (card->u.x.logging){
					printk (KERN_INFO 
						"%s: interface %s connecting, lcn %i ...\n", 
						card->devname, dev->name, chan->common.lcn);
				}
				break;

			case WAN_DISCONNECTED:
				if (card->u.x.logging){
					printk (KERN_INFO 
						"%s: interface %s disconnected, lcn %i !\n", 
						card->devname, dev->name,chan->common.lcn);
				}
				
				if (!card->u.x.LAPB_hdlc){
					atomic_set(&chan->common.disconnect,0);
				}

				if (chan->common.svc) {
					*(unsigned short*)dev->dev_addr = 0;
					card->u.x.svc_to_dev_map[(chan->common.lcn%X25_MAX_CHAN)]=NULL;
		                	chan->common.lcn = 0;
				}

				if (chan->transmit_length){
					chan->transmit_length=0;
					chan->tx_offset=0;
					if (is_queue_stopped(dev)){
						wake_net_dev(dev);
					}
				}

				/* We use the rx interrupt as x25 flow
				 * control. If we ever get a restart request
				 * make sure the rx interrupt is 
				 * re-enabled */

				if (atomic_read(&chan->common.receive_block)){
					TX25Status status;

					card->hw_iface.peek(card->hw,
						       	    card->intr_perm_off,
							    &status.imask,
							    sizeof(status.imask));
					atomic_set(&chan->common.receive_block,0);
		
					if (!(status.imask & INTR_ON_RX_FRAME)){
						DEBUG_EVENT("%s: Disabling x25 flow control!\n",
								card->devname);
						status.imask |= INTR_ON_RX_FRAME;
						card->hw_iface.poke(card->hw,
							       	    card->intr_perm_off,
								    &status.imask,
								    sizeof(status.imask));
					}
				}
			
				if (wan_skb_queue_len(&chan->common.rx_queue)){
					DEBUG_EVENT("%s: Purging svc queue!\n",
							chan->common.dev->name);
					wan_skb_queue_purge(&chan->common.rx_queue);	
				}
				atomic_set(&chan->common.command,0);
				atomic_set(&chan->cmd_rc,0);
				break;

			case WAN_DISCONNECTING:
				if (card->u.x.logging){
					printk(KERN_INFO "\n");
					printk (KERN_INFO 
					"%s: interface %s disconnecting, lcn %i ...\n", 
					card->devname, dev->name,chan->common.lcn);
				}

				atomic_set(&chan->common.disconnect,0);
				break;
		}
		chan->common.state = state;
		x25_update_api_state(card,chan);

	}
	chan->state_tick = jiffies;
}

/*
 * 	Send packet on a logical channel.
 *		When this function is called, tx_skb field of the channel data 
 *		space points to the transmit socket buffer.  When transmission 
 *		is complete, release socket buffer and reset 'tbusy' flag.
 *
 * 	Return:		0	- transmission complete
 *			1	- busy
 *
 * 	Notes:
 * 	1. If packet length is greater than MTU for this channel, we'll fragment
 *    	the packet into 'complete sequence' using M-bit.
 * 	2. When transmission is complete, an event notification should be issued
 *    	to the router.
 */

static int chan_send (netdevice_t* dev, void* buff, unsigned data_len, unsigned char tx_intr)
{
	x25_channel_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	TX25Status status;
	u8	tmp;
	unsigned len=0, qdm=0, res=0, orig_len = 0;
	void *data;

	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	/* Check to see if channel is ready */
	card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &tmp, 1); 
	if ((!(status.cflags[chan->ch_idx] & 0x40) && !card->u.x.LAPB_hdlc)  || 
             !(tmp & 0x40)){ 
            
		if (!tx_intr){
			setup_for_delayed_transmit (dev, buff, data_len);
			return 0;
		}else{
			/* By returning 0 to tx_intr the packet will be dropped */
			++card->wandev.stats.tx_dropped;
			++chan->ifstats.tx_dropped;
			printk(KERN_INFO "%s: ERROR, Tx intr could not send, dropping %s:\n", 
				card->devname,dev->name);
			++chan->if_send_stat.if_send_bfr_not_passed_to_adptr;
			return 0;
		}
	}

	if (chan->common.usedby == API){
		/* Remove the API Header */
		x25api_hdr_t *api_data = (x25api_hdr_t *)buff;

		/* Set the qdm bits from the packet header 
                 * User has the option to set the qdm bits
                 */
		qdm = api_data->qdm;

		orig_len = len = data_len - sizeof(x25api_hdr_t);
		data = (unsigned char*)buff + sizeof(x25api_hdr_t);
	}else{
		data = buff;
		orig_len = len = data_len;
	}	

	if (tx_intr){
		/* We are in tx_intr, minus the tx_offset from 
                 * the total length. The tx_offset part of the
		 * data has already been sent. Also, move the 
		 * data pointer to proper offset location.
                 */
		len -= chan->tx_offset;
		data = (unsigned char*)data + chan->tx_offset;
	}
		
	/* Check if the packet length is greater than MTU
         * If YES: Cut the len to MTU and set the M bit 
         */

	if (len > chan->tx_pkt_size && !card->u.x.LAPB_hdlc){

		/* NC: Bug Fix Jul 8 2003
		 * Since setup_for_delayed_transmit() resets
		 * the chan->tx_offset value. We must start
		 * transmitting the "Long" packets
		 * from the interrupt, not from a process */
		if (!tx_intr){
			setup_for_delayed_transmit (dev, buff, data_len);
			return 0;
		}
		
		len = chan->tx_pkt_size;
		qdm |= M_BIT;		
	} 

	/* Pass only first three bits of the qdm byte to the send
         * routine. In case user sets any other bit which might
         * cause errors. 
         */

	switch(x25_send(card, chan->common.lcn, (qdm&0x07), len, data)){

		case 0x00:	/* success */
			chan->i_timeout_sofar = jiffies;

#if defined(LINUX_2_4)||defined(LINUX_2_6)
			dev->trans_start=jiffies;
#endif
			
			if ((qdm & M_BIT) && !card->u.x.LAPB_hdlc){

				chan->tx_offset += len;

				if (!tx_intr){
					DEBUG_EVENT("%s:%s: Critical Error: qdm set in non-interrupt mode!\n",
							__FUNCTION__,
							card->devname);
				}else{
					/* We are already in tx_inter, thus data is already
                                         * in the buffer. Update the offset and wait for
                                         * next tx_intr. We add on to the offset, since data can
                                         * be X number of times larger than max data size.
					 */
					++chan->ifstats.tx_packets;
					chan->ifstats.tx_bytes += len;
					
					++chan->if_send_stat.if_send_bfr_passed_to_adptr;

					/* The user can set the qdm bit as well.
                                         * If the entire packet was sent and qdm is still
                                         * set, than it's the user who has set the M bit. In that,
                                         * case indicate that the packet was send by returning 
					 * 0 and wait for a new packet. Otherwise, wait for next
                                         * tx interrupt to send the rest of the packet */

					if (chan->tx_offset < orig_len){
						res=1;
					}	
				}
			}else{
				++chan->ifstats.tx_packets;
				chan->ifstats.tx_bytes += len;

				++chan->if_send_stat.if_send_bfr_passed_to_adptr;
			}
			break;

		case 0x33:	/* Tx busy */
			if (tx_intr){
				printk(KERN_INFO "%s: Tx_intr: Big Error dropping packet %s\n",
						card->devname,dev->name);
				++chan->ifstats.tx_dropped;
				++card->wandev.stats.tx_dropped;
				++chan->if_send_stat.if_send_bfr_not_passed_to_adptr;
			}else{
				DBG_PRINTK(KERN_INFO 
					"%s: Send: Big Error should have tx: storring %s\n",
						card->devname,dev->name);
				setup_for_delayed_transmit (dev, buff, data_len);	
			}
			break;

		default:	/* failure */
			++chan->ifstats.tx_errors;
			if (tx_intr){
				printk(KERN_INFO "%s: Tx_intr: Failure to send, dropping %s\n",
					card->devname,dev->name);
				++chan->ifstats.tx_dropped;
				++card->wandev.stats.tx_dropped;
				++chan->if_send_stat.if_send_bfr_not_passed_to_adptr;
			}else{
				DBG_PRINTK(KERN_INFO "%s: Send: Failure to send !!!, storing %s\n",
					card->devname,dev->name);			
				setup_for_delayed_transmit (dev, buff, data_len);
			}
			break;	
	}

	return res;
}


/*
 * 	Parse X.25 call request data and fill x25_call_info_t structure.
 */

static void parse_call_info (unsigned char* str, x25_call_info_t* info)
{
	memset(info, 0, sizeof(x25_call_info_t));
	for (; *str; ++str)
	{
		int i;
		unsigned char ch;

		if (*str == '-') switch (str[1]) {

			/* Take minus 2 off the maximum size so that 
                         * last byte is 0. This way we can use string
                         * manipulaton functions on call information.
                         */

			case 'd':	/* destination address */
				for (i = 0; i < (MAX_X25_ADDR_SIZE-2); ++i){
					ch = str[2+i];
					if (isspace(ch)) break;
					info->dest[i] = ch;
				}
				break;

			case 's':	/* source address */
				for (i = 0; i < (MAX_X25_ADDR_SIZE-2); ++i){
					ch = str[2+i];
					if (isspace(ch)) break;
					info->src[i] = ch;
				}
				break;

			case 'u':	/* user data */
				for (i = 0; i < (MAX_X25_DATA_SIZE-2); ++i){
					ch = str[2+i];
					if (isspace(ch)) break;
					info->user[i] = ch; 
				}
				info->nuser = i;
				break;

			case 'f':	/* facilities */
				for (i = 0; i < (MAX_X25_FACL_SIZE-2); ++i){
					ch = str[2+i];
					if (isspace(ch)) break;
					info->facil[i] = ch;
				}
				info->nfacil = i;
				break;
		}
	}
}

/*
 * 	Convert line speed in bps to a number used by S502 code.
 */

static unsigned char bps_to_speed_code (unsigned long bps)
{
	unsigned char	number;

	if (bps <= 1200)        number = 0x01;
	else if (bps <= 2400)   number = 0x02;
	else if (bps <= 4800)   number = 0x03;
	else if (bps <= 9600)   number = 0x04;
	else if (bps <= 19200)  number = 0x05;
	else if (bps <= 38400)  number = 0x06;
	else if (bps <= 45000)  number = 0x07;
	else if (bps <= 56000)  number = 0x08;
	else if (bps <= 64000)  number = 0x09;
	else if (bps <= 74000)  number = 0x0A;
	else if (bps <= 112000) number = 0x0B;
	else if (bps <= 128000) number = 0x0C;
	else number = 0x0D;

	return number;
}

/*
 * 	Convert decimal string to unsigned integer.
 * 	If len != 0 then only 'len' characters of the string are converted.
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

/*
 * 	Convert hex string to unsigned integer.
 * 	If len != 0 then only 'len' characters of the string are conferted.
 */

static unsigned int hex_to_uint (unsigned char* str, int len)
{
	unsigned val, ch;

	if (!len) 
		len = strlen(str);

	for (val = 0; len; ++str, --len)
	{
		ch = *str;
		if (is_digit(ch))
			val = (val << 4) + (ch - (unsigned)'0');
		else if (is_hex_digit(ch))
			val = (val << 4) + ((ch & 0xDF) - (unsigned)'A' + 10);
		else break;
	}
	return val;
}


static int handle_IPXWAN(unsigned char *sendpacket, char *devname, unsigned char enable_IPX, unsigned long network_number, unsigned short proto)
{
	int i;

	if( proto == ETH_P_IPX) {
		/* It's an IPX packet */
		if(!enable_IPX) {
			/* Return 1 so we don't pass it up the stack. */
			return 1;
		}
	} else {
		/* It's not IPX so pass it up the stack.*/ 
		return 0;
	}

	if( sendpacket[16] == 0x90 &&
	    sendpacket[17] == 0x04)
	{
		/* It's IPXWAN  */

		if( sendpacket[2] == 0x02 &&
		    sendpacket[34] == 0x00)
		{
			/* It's a timer request packet */
			printk(KERN_INFO "%s: Received IPXWAN Timer Request packet\n",devname);

			/* Go through the routing options and answer no to every
			 * option except Unnumbered RIP/SAP
			 */
			for(i = 41; sendpacket[i] == 0x00; i += 5)
			{
				/* 0x02 is the option for Unnumbered RIP/SAP */
				if( sendpacket[i + 4] != 0x02)
				{
					sendpacket[i + 1] = 0;
				}
			}

			/* Skip over the extended Node ID option */
			if( sendpacket[i] == 0x04 )
			{
				i += 8;
			}

			/* We also want to turn off all header compression opt. 			 */ 
			for(; sendpacket[i] == 0x80 ;)
			{
				sendpacket[i + 1] = 0;
				i += (sendpacket[i + 2] << 8) + (sendpacket[i + 3]) + 4;
			}

			/* Set the packet type to timer response */
			sendpacket[34] = 0x01;

			printk(KERN_INFO "%s: Sending IPXWAN Timer Response\n",devname);
		}
		else if( sendpacket[34] == 0x02 )
		{
			/* This is an information request packet */
			printk(KERN_INFO "%s: Received IPXWAN Information Request packet\n",devname);

			/* Set the packet type to information response */
			sendpacket[34] = 0x03;

			/* Set the router name */
			sendpacket[51] = 'X';
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
			for(i = 66; i < 99; i+= 1)
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

		/* Set the WNodeID to our network address */
		sendpacket[35] = (unsigned char)(network_number >> 24);
		sendpacket[36] = (unsigned char)((network_number & 0x00FF0000) >> 16);
		sendpacket[37] = (unsigned char)((network_number & 0x0000FF00) >> 8);
		sendpacket[38] = (unsigned char)(network_number & 0x000000FF);

		return 1;
	} else {
		/*If we get here its an IPX-data packet, so it'll get passed up the stack.
		 */
		/* switch the network numbers */
		switch_net_numbers(sendpacket, network_number, 1);	
		return 0;
	}
}

/*
 *  	If incoming is 0 (outgoing)- if the net numbers is ours make it 0
 *  	if incoming is 1 - if the net number is 0 make it ours 
 */

static void switch_net_numbers(unsigned char *sendpacket, unsigned long network_number, unsigned char incoming)
{
	unsigned long pnetwork_number;

	pnetwork_number = (unsigned long)((sendpacket[6] << 24) + 
			  (sendpacket[7] << 16) + (sendpacket[8] << 8) + 
			  sendpacket[9]);
	

	if (!incoming) {
		/*If the destination network number is ours, make it 0 */
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




/********************* X25API SPECIFIC FUNCTIONS ****************/


/*===============================================================
 *  find_channel
 *
 *	Manages the lcn to device map. It increases performance
 *      because it eliminates the need to search through the link  
 *      list for a device which is bounded to a specific lcn.
 *
 *===============================================================*/


netdevice_t * find_channel(sdla_t *card, unsigned lcn)
{
	if (card->u.x.LAPB_hdlc){

		return WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));

	}else{
		/* We don't know whether the incoming lcn
                 * is a PVC or an SVC channel. But we do know that
                 * the lcn cannot be for both the PVC and the SVC
                 * channel.

		 * If the lcn number is greater or equal to 255, 
                 * take the modulo 255 of that number. We only have
                 * 255 locations, thus higher numbers must be mapped
                 * to a number between 0 and 245. 

		 * We must separate pvc's and svc's since two don't
                 * have to be contiguous.  Meaning pvc's can start
                 * from 1 to 10 and svc's can start from 256 to 266.
                 * But 256%255 is 1, i.e. CONFLICT.
		 */


		/* Highest LCN number must be less or equal to 4096 */
		if ((lcn <= MAX_LCN_NUM) && (lcn > 0)){

			if (lcn < X25_MAX_CHAN){
				if (card->u.x.svc_to_dev_map[lcn])
					return card->u.x.svc_to_dev_map[lcn];

				if (card->u.x.pvc_to_dev_map[lcn])
					return card->u.x.pvc_to_dev_map[lcn];
			
			}else{
				int new_lcn = lcn%X25_MAX_CHAN;
				if (card->u.x.svc_to_dev_map[new_lcn])
					return card->u.x.svc_to_dev_map[new_lcn];

				if (card->u.x.pvc_to_dev_map[new_lcn])
					return card->u.x.pvc_to_dev_map[new_lcn];
			}
		}
		return NULL;
	}
}

void bind_lcn_to_dev (sdla_t *card, netdevice_t *dev,unsigned lcn)
{
	x25_channel_t *chan = wan_netif_priv(dev);

	/* Modulo the lcn number by X25_MAX_CHAN (255)
	 * because the lcn number can be greater than 255 
         *
	 * We need to split svc and pvc since they don't have
         * to be contigous. 
	 */

	if (chan->common.svc){
		card->u.x.svc_to_dev_map[(lcn % X25_MAX_CHAN)] = dev;
	}else{
		card->u.x.pvc_to_dev_map[(lcn % X25_MAX_CHAN)] = dev;
	}
	chan->common.lcn = lcn;
}



/*===============================================================
 * x25api_bh 
 *
 *
 *==============================================================*/

static void x25api_bh (unsigned long data)
{
	x25_channel_t* chan = (x25_channel_t*)data;
	sdla_t* card = chan->card;
	struct sk_buff *skb;
	int len, err=0,full=0;

	while ((skb=wan_api_dequeue_skb(chan))){

		len=skb->len;
		if (chan->common.usedby == API){
			err=wan_api_rx(chan,skb);
			
			if (err == 0){
				++chan->rx_intr_stat.rx_intr_bfr_passed_to_stack;
				++chan->ifstats.rx_packets;
				chan->ifstats.rx_bytes += len;
			}else if (err == -ENOMEM){
				DEBUG_EVENT("%s:%s: Warning: API socket full!\n",card->devname,chan->common.dev->name);
				wan_skb_queue_head(&chan->common.rx_queue,skb);
				full=1;
				break;
			}else{
				++chan->ifstats.rx_dropped;
				wan_skb_free(skb);
			}
		}
	}	


	if (!full){
		if (atomic_read(&chan->common.receive_block)){
			TX25Status	status;

			card->hw_iface.peek(card->hw,
				       	    card->intr_perm_off,
					    &status.imask,
					    sizeof(status.imask));
			atomic_set(&chan->common.receive_block,0);
		
			if (!(status.imask & INTR_ON_RX_FRAME)){
				DEBUG_EVENT("%s: Disabling x25 flow control!\n",card->devname);
				status.imask |= INTR_ON_RX_FRAME;
				card->hw_iface.poke(card->hw,
					       	    card->intr_perm_off,
						    &status.imask,
						    sizeof(status.imask));
			}
		}
	}

	WAN_TASKLET_END((&chan->common.bh_task));

	return;
}

static void wakeup_sk_bh (netdevice_t *dev)
{

	x25_channel_t *chan = wan_netif_priv(dev);

	if (!chan->common.sk){
		return;
	}

	wanpipe_api_poll_wake(chan->common.sk);
	return;
}

/*===============================================================
 * tx_intr_cmd_exec
 *  
 *	Called by TX interrupt to execute a command
 *===============================================================*/

static int tx_intr_cmd_exec (sdla_t* card)
{
	netdevice_t *dev;
	unsigned char more_to_exec=0;
	volatile x25_channel_t *chan=NULL;
	volatile wan_mbox_t* mbox = &card->wan_mbox;

	int i=0,bad_cmd=0,err=0;	

	/* This is the very first time we come into
	 * the tx intr cmd exec routine.  Thus,
	 * the card is ready to receive commands */
	if (!test_and_set_bit(0,&card->u.x.card_ready)){
		DEBUG_EVENT("%s: X25 Driver Connected and Ready\n",
				card->devname);
		card->u.x.cmd_dev=NULL;
	}

	if (card->u.x.cmd_dev == NULL){
		card->u.x.cmd_dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	}

	dev = card->u.x.cmd_dev;

	for (;;){

		chan = wan_netif_priv(dev);
		if (!chan || !(dev->flags&IFF_UP)){
			dev = move_dev_to_next(card,dev);
			goto cmd_dev_skip;
		}
	
		if (atomic_read(&chan->common.command)){ 

			bad_cmd = check_bad_command(card,dev);
			
			err = execute_delayed_cmd(card, dev,bad_cmd);
			
			chan->x25_api_event.hdr.qdm = mbox->wan_x25_qdm;
			chan->x25_api_event.hdr.cause = mbox->wan_x25_cause;
			chan->x25_api_event.hdr.diagn = mbox->wan_x25_diagn;
			chan->x25_api_event.hdr.pktType = mbox->wan_x25_pktType&0x7F;
			chan->x25_api_event.hdr.length  = 0;
			chan->x25_api_event.hdr.result = 0;
			chan->x25_api_event.hdr.lcn = mbox->wan_x25_lcn;
			chan->x25_api_event.hdr.mtu = card->u.x.x25_conf.defPktSize;
			chan->x25_api_event.hdr.mru = card->u.x.x25_conf.defPktSize;

			atomic_set(&chan->cmd_rc,err);
			
			switch (err){

			default:

				/* Return the result to the socket without
                                 * delay. NO_WAIT Command */	
				atomic_set(&chan->common.command,0);
				more_to_exec=0;
				break;

			case -EAGAIN:

#if 0
				/* If command could not be executed for
                                 * some reason (i.e return code 0x33 busy)
                                 * set the more_to_exec bit which will
                                 * indicate that this command must be exectued
                                 * again during next timer interrupt 
				 */
				chan->cmd_timeout=jiffies;
#endif				
				more_to_exec=1;
				break;
			}

			bad_cmd=0;

			/* If flags is set, there are no hdlc buffers,
                         * thus, wait for the next pass and try the
                         * same command again. Otherwise, start searching 
                         * from next device on the next pass. 
			 */
			if (!more_to_exec){
				dev = move_dev_to_next(card,dev);
			}
			break;
		}else{
			/* This device has nothing to execute,
                         * go to next. 
			 */
			dev = move_dev_to_next(card,dev);
		}	
cmd_dev_skip:
		if (++i == card->u.x.no_dev){
			if (!more_to_exec){
				DEBUG_TX("%s: Nothing to execute in Timer\n",
					card->devname);
			}
			break;
		}

	} //End of FOR

	card->u.x.cmd_dev = dev;
	
	if (more_to_exec){
		/* If more commands are pending, do not turn off timer 
                 * interrupt */
		return 1;
	}else{
		/* No more commands, turn off timer interrupt */
		atomic_dec(&card->u.x.tx_interrupt_cmd);
		return 0;
	}	
}

/*===============================================================
 * execute_delayed_cmd 
 *
 *	Execute an API command which was passed down from the
 *      sock.  Sock is very limited in which commands it can
 *      execute.  Wait and No Wait commands are supported.  
 *      Place Call, Clear Call and Reset wait commands, where
 *      Accept Call is a no_wait command.
 *
 *===============================================================*/
static int execute_delayed_cmd (sdla_t* card, netdevice_t *dev, char bad_cmd)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	u8	status;
	int err=-EINVAL;
	x25_channel_t *chan = wan_netif_priv(dev);

	card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &status, 1);
	if (!(status & 0x40) && !bad_cmd){
		printk(KERN_INFO "%s: Execute Command Failed, No HDLC Buffers!\n",
				card->devname);
		return -EAGAIN;
	}

	/* If channel is pvc, instead of place call
         * run x25_channel configuration. If running LAPB HDLC
         * enable communications. 
         */
	if ((!chan->common.svc) && (atomic_read(&chan->common.command) == X25_PLACE_CALL)){

		if (card->u.x.LAPB_hdlc){
			DBG_PRINTK(KERN_INFO "LAPB: Connecting\n");
			err=connect(card);
			if (err==CMD_OK){
				set_chan_state(dev,WAN_CONNECTING);
				return 0;
			}else{
				set_chan_state(dev,WAN_DISCONNECTED);
				return -ENETDOWN;
			}
		}else{
			DBG_PRINTK(KERN_INFO "%s: PVC is CONNECTING\n",card->devname);
			if (x25_get_chan_conf(card, chan) == CMD_OK){
				set_chan_state(dev, WAN_CONNECTED);
				return 0;
			}else{ 
				set_chan_state(dev, WAN_DISCONNECTED);
				return -ENETDOWN;
			}
		}
	}

	/* Check if command is bad. We need to copy the cmd into
         * the buffer regardless since we return the, mbox to
         * the user */
	if (bad_cmd){
		return -EINVAL;
	}

	DEBUG_TEST("%s:%s:%s: Executing Command %d Lcn %d\n",
			__FUNCTION__,card->devname,dev->name,
			atomic_read(&chan->common.command),
			chan->common.lcn);

	switch (atomic_read(&chan->common.command)){

	case X25_PLACE_CALL:
	
		err=x25_place_call(card,chan);
		
		switch (err){

		case CMD_OK:

			DEBUG_TEST("%s: PLACE CALL Binding dev %s to lcn %i\n",
					card->devname,dev->name, mbox->wan_x25_lcn);
		
			bind_lcn_to_dev (card, dev, mbox->wan_x25_lcn);
			set_chan_state(dev, WAN_CONNECTING);
			break;

		default:
			set_chan_state(dev, WAN_DISCONNECTED);
			err=-ENETDOWN;
		}
		break;

	case X25_ACCEPT_CALL: 
	
		err=x25_accept_call(card,
				    chan->common.lcn,
				    0,
				    chan->call_string,
				    strlen(chan->call_string));
		
		switch (err){

		case CMD_OK:

			DEBUG_TEST("%s: ACCEPT Binding dev %s to lcn %i\n",
				card->devname,dev->name,mbox->wan_x25_lcn);

			bind_lcn_to_dev (card, dev, mbox->wan_x25_lcn);

			if (x25_get_chan_conf(card, chan) == CMD_OK){

				set_chan_state(dev, WAN_CONNECTED);
			}else{ 
				if (x25_clear_call(card, chan->common.lcn, 0, 0, 0, NULL,0) == CMD_OK){
					set_chan_state(dev, WAN_DISCONNECTING);
					err=-ENETDOWN;
				}else{
					/* Do not change the state here. If we fail 
					 * the accept the return code is send up 
					 *the stack, which will ether retry
                               	  	 * or clear the call 
					 */
					DEBUG_EVENT("%s: ACCEPT: STATE MAY BE CURRUPTED 2 !!!!!\n",
						card->devname);
					err=-EINVAL;
				}
			}
			break;


		case X25RES_ASYNC_PACKET:
		case X25RES_NOT_READY:
			err = -EAGAIN;
			break;

		default: 
			DBG_PRINTK(KERN_INFO "%s: ACCEPT FAILED\n",card->devname);
			if (x25_clear_call(card, chan->common.lcn, 0, 0, 0, NULL,0) == CMD_OK){
				set_chan_state(dev, WAN_DISCONNECTING);
				err = -ENETDOWN;
			}else{
				/* Do not change the state here. If we fail the accept. The
                                 * return code is send up the stack, which will ether retry
                                 * or clear the call */
				DEBUG_EVENT("%s: ACCEPT: STATE MAY BE CORRUPTED 1 !!!!!\n",
						card->devname);
				err = -EINVAL;
			}
		}
		break;

	case X25_CLEAR_CALL:

		err=x25_clear_call(card,chan->common.lcn,
				   chan->x25_api_cmd_hdr.cause, 
				   chan->x25_api_cmd_hdr.diagn,
				   0x80,
				   chan->call_string,
				   strlen(chan->call_string));
		
		switch (err){

		case CMD_OK:
			DEBUG_TEST("%s: CALL CLEAR OK: Dev %s  Lcn %i  Chan Lcn %i\n",
				    card->devname,dev->name,
				    mbox->wan_x25_lcn,chan->common.lcn);
			set_chan_state(dev, WAN_DISCONNECTING);
			break;

		case X25RES_CHANNEL_IN_USE:
		case X25RES_ASYNC_PACKET:
		case X25RES_NOT_READY:
			err=-EAGAIN;
			break;
			
		case X25RES_LINK_NOT_IN_ABM:
		case X25RES_INVAL_LCN:
		case X25RES_INVAL_STATE:

			set_chan_state(dev, WAN_DISCONNECTED);
			err = -EINVAL;
			break;
		default:
			/* If command did not execute because of user
                         * fault, do not change the state. This will
                         * signal the socket that clear command failed.
                         * User can retry or close the socket.
                         * When socket gets killed, it will set the 
                         * chan->disconnect which will signal
                         * driver to clear the call */
			DEBUG_EVENT("%s: Clear Command Failed, Rc %x\n",
				card->devname,err); 
			return -EINVAL;
		}
		break;


	case X25_RESET:
	
		err=x25_reset_call(card,chan->common.lcn,
				   chan->x25_api_cmd_hdr.cause, 
				   chan->x25_api_cmd_hdr.diagn,
				   0);

		switch(err){
			
		case CMD_OK:
			DEBUG_TEST("%s: RESET CLEAR OK: Dev %s  Lcn %i  Chan Lcn %i\n",
				    card->devname,dev->name,
				    mbox->wan_x25_lcn,chan->common.lcn);
			break;

		case X25RES_CHANNEL_IN_USE:
		case X25RES_ASYNC_PACKET:
		case X25RES_NOT_READY:
			err=-EAGAIN;
			break;


		case X25RES_LINK_NOT_IN_ABM:
		case X25RES_INVAL_LCN:
		case X25RES_INVAL_STATE:

			set_chan_state(dev, WAN_DISCONNECTED);
			err = -EINVAL;
			break;

		default:
			/* If command did not execute because of user
                         * fault, do not change the state. This will
                         * signal the socket that clear command failed.
                         * User can retry or close the socket.
                         * When socket gets killed, it will set the 
                         * chan->disconnect which will signal
                         * driver to clear the call */
			DEBUG_EVENT("%s: Clear Command Failed, Rc %x\n",
				card->devname,err); 
			return -EINVAL;
		}
		break;

	case X25_WP_INTERRUPT:

		err=x25_interrupt(card,
				  chan->common.lcn,
				  0,
				  chan->call_string,
				  strlen(chan->call_string));

		switch(err){
				
		case CMD_OK:
			DEBUG_TEST("%s: INTERRUPT OK: Dev %s  Lcn %i  Chan Lcn %i\n",
				    card->devname,dev->name,
				    mbox->wan_x25_lcn,chan->common.lcn);
			break;

		case X25RES_CHANNEL_IN_USE:
		case X25RES_ASYNC_PACKET:
		case X25RES_NOT_READY:
			err=-EAGAIN;
			break;


		case X25RES_LINK_NOT_IN_ABM:
		case X25RES_INVAL_LCN:
		case X25RES_INVAL_STATE:

			set_chan_state(dev, WAN_DISCONNECTED);
			err = -EINVAL;
			break;
	
		default:
			/* If command did not execute because of user
                         * fault, do not change the state. This will
                         * signal the socket that clear command failed.
                         * User can retry or close the socket.
                         * When socket gets killed, it will set the 
                         * chan->disconnect which will signal
                         * driver to clear the call */
			DEBUG_EVENT("%s: Clear Command Failed, Rc %x\n",
				card->devname,err); 
			return -EINVAL;
		}
		break;
	}	

	return err;
}

/*===============================================================
 * api_incoming_call 
 *
 *	Pass an incoming call request up the listening
 *      sock.  If the API sock is not listening reject the
 *      call.
 *
 *===============================================================*/

static int api_incoming_call (sdla_t* card, wan_mbox_t *mbox, int lcn)
{
	unsigned char *buf;
	x25api_hdr_t *x25_api_hdr;
	netdevice_t *dev;
	x25_channel_t *chan;
	int len = sizeof(x25api_hdr_t)+mbox->wan_data_len;
	struct sk_buff *skb;
	int err=0;
	
	if (!card->sk){
		DEBUG_EVENT("%s: X25 Send API call request, no Sk ID\n",
				card->devname);
		return -ENODEV;
	}

	dev=find_free_svc_dev(card);
	if (dev == NULL){
		DEBUG_EVENT("%s: Failed incoming call req: no free svc !\n",
				card->devname);
		return -ENODEV;
	}

	chan=wan_netif_priv(dev);

	if (alloc_and_init_skb_buf(card, &skb, len)){
		printk(KERN_INFO "%s: API incoming call, no memory\n",card->devname);
		release_svc_dev(card,dev);
		return -ENOMEM;
	}

	buf = skb_push(skb,sizeof(x25api_hdr_t));
	if (!buf){
		release_svc_dev(card,dev);
		wan_skb_free(skb);
		return -ENOMEM;
	}

	x25_api_hdr = (x25api_hdr_t *)buf;

	x25_api_hdr->qdm = mbox->wan_x25_qdm;
	x25_api_hdr->cause = mbox->wan_x25_cause;
	x25_api_hdr->diagn = mbox->wan_x25_diagn;
	x25_api_hdr->pktType = mbox->wan_x25_pktType&0x7F;
	x25_api_hdr->length  = mbox->wan_data_len;
	x25_api_hdr->result = 0;
	x25_api_hdr->lcn = lcn;
	x25_api_hdr->mtu = card->u.x.x25_conf.defPktSize;
	x25_api_hdr->mru = card->u.x.x25_conf.defPktSize;

	if (mbox->wan_data_len > 0 && mbox->wan_data_len <= X25_CALL_STR_SZ){
		buf = skb_put(skb,mbox->wan_data_len);
		if (!buf){
			release_svc_dev(card,dev);
			wan_skb_free(skb);
			return -ENOMEM;
		}
		memcpy(buf,mbox->wan_data,mbox->wan_data_len);
	}

	memcpy(&chan->x25_api_event,skb->data,skb->len);
	
	skb->pkt_type = WAN_PACKET_ERR;
	dev_hold(dev);
	skb->dev=dev;
	skb->destructor=wan_skb_destructor;
	skb->protocol=htons(ETH_P_X25);

	bind_lcn_to_dev(card, dev, lcn);
	set_chan_state(dev, WAN_CONNECTING);
		
	err=wanpipe_api_listen_rx(skb,card->sk);

	if (err!=0){
		release_svc_dev(card,dev);	
		wan_skb_free(skb);
		set_chan_state(dev, WAN_DISCONNECTED);
	}
	
	return err;
}

/*===============================================================
 * clear_confirm_event
 *
 * 	Pass the clear confirmation event up the sock. The
 *      API will disconnect only after the clear confirmation
 *      has been received. 
 *
 *      Depending on the state, clear confirmation could 
 *      be an OOB event, or a result of an API command.
 *===============================================================*/

static int clear_confirm_event (sdla_t *card, wan_mbox_t* mb)
{
	netdevice_t *dev;
	x25_channel_t *chan;
	unsigned char old_state;	

	dev = find_channel(card,mb->wan_x25_lcn);
	if (!dev){
		DEBUG_EVENT("%s: Rx X25 Clear confirm on Lcn=%i: No dev!\n",
			card->devname, mb->wan_x25_lcn);
		return 1;
	}

	chan=wan_netif_priv(dev);
	DEBUG_EVENT("%s:%s Rx X25 Clear confirm on Lcn=%i\n",
			card->devname, dev->name, mb->wan_x25_lcn);

	/* If not API fall through to default. 
	 * If API, send the result to a waiting
         * socket.
	 */
	
	old_state = chan->common.state;
	set_chan_state(dev, WAN_DISCONNECTED);
	api_oob_event (card,mb);

	return 1;
}

/*===============================================================
 * send_oob_msg
 *
 *    Construct an NEM Message and pass it up the connected
 *    sock. If the sock is not bounded discard the NEM.
 *
 *===============================================================*/

static int send_oob_msg (sdla_t *card, netdevice_t *dev, wan_mbox_t *mbox)
{
	unsigned char *buf;
	int len = sizeof(x25api_hdr_t)+mbox->wan_data_len;
	x25api_hdr_t *x25_api_hdr;
	x25_channel_t *chan=wan_netif_priv(dev);
	struct sk_buff *skb;
	int err=0;

	if (chan->common.usedby != API)
		return -ENODEV;
		
	if (!chan->common.sk){
		return -ENODEV;
	}

	if (len > sizeof(x25api_t)){
		DEBUG_EVENT("%s: %s: oob len > x25api_t!\n",
				card->devname,__FUNCTION__);
		return -EINVAL;
	}
	
	if (alloc_and_init_skb_buf(card,&skb,len)){
		return -ENOMEM;
	}
	
	buf = skb_put(skb,sizeof(x25api_hdr_t));
	if (!buf){
		wan_skb_free(skb);
		return -ENOMEM;
	}

	x25_api_hdr = (x25api_hdr_t *)buf;

	x25_api_hdr->qdm = mbox->wan_x25_qdm;
	x25_api_hdr->cause = mbox->wan_x25_cause;
	x25_api_hdr->diagn = mbox->wan_x25_diagn;
	x25_api_hdr->pktType = mbox->wan_x25_pktType&0x7F;
	x25_api_hdr->length = mbox->wan_data_len;
	x25_api_hdr->result = 0;
	x25_api_hdr->lcn = mbox->wan_x25_lcn;
	x25_api_hdr->mtu = card->u.x.x25_conf.defPktSize;
	x25_api_hdr->mru = card->u.x.x25_conf.defPktSize;

	if (mbox->wan_data_len > 0 && mbox->wan_data_len <= X25_CALL_STR_SZ){
		buf = skb_put(skb,mbox->wan_data_len);
		if (!buf){
			wan_skb_free(skb);
			return -ENOMEM;
		}
		memcpy(buf,mbox->wan_data,mbox->wan_data_len);
	}

	memcpy(&chan->x25_api_event,skb->data,skb->len);
	
	skb->pkt_type = WAN_PACKET_ERR;
	skb->protocol=htons(ETH_P_X25);

	if (wan_api_rx(chan,skb)!=0){
		printk(KERN_INFO "%s: Error: No dev for OOB\n",
				chan->common.dev->name);
		err=0;
		wan_skb_free(skb);
	}

	return err;
}


/*===============================================================
 *  alloc_and_init_skb_buf 
 *
 *	Allocate and initialize an skb buffer. 
 *
 *===============================================================*/

static int alloc_and_init_skb_buf (sdla_t *card, struct sk_buff **skb, int len)
{
	struct sk_buff *new_skb = *skb;

	new_skb = dev_alloc_skb(len + X25_HRDHDR_SZ);
	if (new_skb == NULL){
		printk(KERN_INFO "%s: no socket buffers available!\n",
			card->devname);
		return 1;
	}

	if (skb_tailroom(new_skb) < len){
		/* No room for the packet. Call off the whole thing! */
                wan_skb_free(new_skb);
		printk(KERN_INFO "%s: Listen: unexpectedly long packet sequence\n"
			,card->devname);
		*skb = NULL;
		return 1;
	}

	*skb = new_skb;
	return 0;

}

/*===============================================================
 *  api_oob_event 
 *
 *	Send an OOB event up to the sock 
 *
 *===============================================================*/

static void api_oob_event (sdla_t *card,wan_mbox_t *mbox)
{
	netdevice_t *dev = find_channel(card,mbox->wan_x25_lcn);
	x25_channel_t *chan;

	if (!dev)
		return;

	chan=wan_netif_priv(dev);

	if (chan->common.usedby == API){
		send_oob_msg(card,dev,mbox);
	}
}

#if 0
static void hdlc_link_down (sdla_t *card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int retry = 5;
	int err=0;

	do {
		memset(mbox,0,sizeof(wan_mbox_t));
		mbox->wan_command = X25_HDLC_LINK_DISC;
		mbox->wan_data_len = 1;
		mbox->wan_data[0]=0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	} while (err  && x25_error(card, err, X25_HDLC_LINK_DISC, 0) && retry--);

	if (err)
		printk(KERN_INFO "%s: Hdlc Link Down Failed %x\n",card->devname,err);

	disconnect (card);
	
}
#endif

static int check_bad_command (sdla_t* card, netdevice_t *dev)
{
	x25_channel_t *chan = wan_netif_priv(dev);
	int bad_cmd = 0;

	switch (atomic_read(&chan->common.command)&0x7F){

		case X25_PLACE_CALL:
			if (chan->common.state != WAN_DISCONNECTED)
				bad_cmd=1;
			break;
		case X25_CLEAR_CALL:
			if (chan->common.state == WAN_DISCONNECTED)
				bad_cmd=1;
			break;
		case X25_ACCEPT_CALL:
			if (chan->common.state != WAN_CONNECTING)
				bad_cmd=1;
			break;
		case X25_RESET:
			if (chan->common.state != WAN_CONNECTED)
				bad_cmd=1;
			break;
		case X25_WP_INTERRUPT:
			if (chan->common.state != WAN_CONNECTED)
				bad_cmd=1;
			break;
		default:
			bad_cmd=1;
			break;
	}

	if (bad_cmd){
		printk(KERN_INFO "%s: Invalid State, BAD Command %x, dev %s, lcn %i, st %i\n", 
			card->devname,atomic_read(&chan->common.command),dev->name, 
			chan->common.lcn, chan->common.state);
	}

	return bad_cmd;
}



/*************************** XPIPEMON FUNCTIONS **************************/

/*==============================================================================
 * Process UDP call of type XPIPE
 */

static int process_udp_mgmt_pkt(sdla_t *card,netdevice_t *local_dev)
{
	int            c_retry = MAX_CMD_RETRY;
	unsigned int   len;
	struct sk_buff *new_skb;
	wan_mbox_t*	mbox = &card->wan_mbox;
	int            err;
	int            udp_mgmt_req_valid = 1;
	netdevice_t  *dev;
        x25_channel_t  *chan;
	unsigned short lcn;
	struct timeval tv;
	wan_udp_pkt_t *wan_udp_pkt = (wan_udp_pkt_t *)&card->u.x.udp_pkt_data;

	if (local_dev){
		dev = local_dev;	
	}else{
		dev = card->u.x.udp_dev;
	}

	if (!dev){
		atomic_set(&card->u.x.udp_pkt_len,0);
		return 0;
	}

	chan = wan_netif_priv(dev);
	lcn = chan->common.lcn;

	switch(wan_udp_pkt->wan_udp_command) {
            
		/* XPIPE_ENABLE_TRACE */
		case XPIPE_ENABLE_TRACING:

		/* XPIPE_GET_TRACE_INFO */
		case XPIPE_GET_TRACE_INFO:
 
		/* SET FT1 MODE */
		case XPIPE_SET_FT1_MODE:
           
			if(card->u.x.udp_pkt_src == UDP_PKT_FRM_NETWORK) {
                    		++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_direction_err;
				udp_mgmt_req_valid = 0;
				break;
			}

		/* XPIPE_FT1_READ_STATUS */
		case XPIPE_FT1_READ_STATUS:

		/* FT1 MONITOR STATUS */
		case XPIPE_FT1_STATUS_CTRL:
			// ALEX_TODAY if(card->hw.fwid !=  SFID_X25_508) {
			if (card->type == SDLA_S508){
				++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_type_err;
				udp_mgmt_req_valid = 0;
				break;
			}
		default:
			break;
       	}

	if(!udp_mgmt_req_valid) {
           	/* set length to 0 */
		wan_udp_pkt->wan_udp_data_len = 0;
		/* set return code */
		//ALEX_TODAY wan_udp_pkt->wan_udp_return_code = (card->hw.fwid != SFID_X25_508) ? 0x1F : 0xCD;
		wan_udp_pkt->wan_udp_return_code = (card->type != SDLA_S508) ? 0x1F : 0xCD;
		
	} else {   
        
		switch (wan_udp_pkt->wan_udp_command) {
    
		case XPIPE_FLUSH_DRIVER_STATS:
			init_x25_channel_struct(chan);
			init_global_statistics(card);
			mbox->wan_data_len = 0;
			break;


		case XPIPE_DRIVER_STAT_IFSEND:
			memcpy(wan_udp_pkt->wan_udp_data, &chan->if_send_stat, sizeof(if_send_stat_t));
			mbox->wan_data_len = sizeof(if_send_stat_t);
			wan_udp_pkt->wan_udp_data_len =  mbox->wan_data_len;	
			break;
	
		case XPIPE_DRIVER_STAT_INTR:
			memcpy(&wan_udp_pkt->wan_udp_data[0], &card->statistics, sizeof(global_stats_t));
                        memcpy(&wan_udp_pkt->wan_udp_data[sizeof(global_stats_t)],
                                &chan->rx_intr_stat, sizeof(rx_intr_stat_t));
			
			mbox->wan_data_len = sizeof(global_stats_t) +
					sizeof(rx_intr_stat_t);
			wan_udp_pkt->wan_udp_data_len =  mbox->wan_data_len;
			break;

		case XPIPE_DRIVER_STAT_GEN:
                        memcpy(wan_udp_pkt->wan_udp_data,
                                &chan->pipe_mgmt_stat.UDP_PIPE_mgmt_kmalloc_err,
                                sizeof(pipe_mgmt_stat_t));

                        memcpy(&wan_udp_pkt->wan_udp_data[sizeof(pipe_mgmt_stat_t)],
                               &card->statistics, sizeof(global_stats_t));

                        wan_udp_pkt->wan_udp_return_code = 0;
                        wan_udp_pkt->wan_udp_data_len = sizeof(global_stats_t)+
                                                     sizeof(rx_intr_stat_t);
                        mbox->wan_data_len = wan_udp_pkt->wan_udp_data_len;
                        break;

		case XPIPE_ROUTER_UP_TIME:
			do_gettimeofday(&tv);
			chan->router_up_time = tv.tv_sec - chan->router_start_time;
    	                *(unsigned long *)&wan_udp_pkt->wan_udp_data = chan->router_up_time;	
			wan_udp_pkt->wan_udp_data_len = mbox->wan_data_len = 4;
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		case WAN_GET_PROTOCOL:
		   	wan_udp_pkt->wan_udp_data[0] = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mbox->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mbox->wan_data_len  = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;	

		case XPIPE_GET_TRACE_INFO:

			card->u.x.trace_timeout=jiffies;

			new_skb=wan_skb_dequeue(&card->u.x.trace_queue);
			if (!new_skb){
				wan_udp_pkt->wan_udp_return_code = 1;
		    		mbox->wan_data_len  = wan_udp_pkt->wan_udp_data_len = 0;
				break;
			}
			
			memcpy(wan_udp_pkt->wan_udp_data,new_skb->data,new_skb->len);
			
			wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mbox->wan_data_len  = wan_udp_pkt->wan_udp_data_len = new_skb->len;

			wan_skb_free(new_skb);
			new_skb=NULL;
			break;

		case XPIPE_DISABLE_TRACING:
			card->u.x.trace_timeout=jiffies;
			wan_skb_queue_purge(&card->u.x.trace_queue);
			goto x25_udp_cmd;	


		default :
x25_udp_cmd:
			if (card->u.x.LAPB_hdlc){
				switch (wan_udp_pkt->wan_udp_command){
				case X25_READ_CONFIGURATION:
					wan_udp_pkt->wan_udp_command=X25_HDLC_READ_CONFIG;
					break;
				}
			}
			
			do {
				memcpy(&mbox->wan_command, &wan_udp_pkt->wan_udp_command, sizeof(TX25Cmd));
				if(mbox->wan_data_len){ 
					memcpy(&mbox->wan_data, 
					       (char *)wan_udp_pkt->wan_udp_data, 
					       mbox->wan_data_len);
				}	
		
				err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
			} while (err  && x25_error(card, err, mbox->wan_command, 0) && c_retry--);


			if ( err == CMD_OK || 
			    (err == 1 && 
			     (mbox->wan_command == 0x06 || 
			      mbox->wan_command == 0x16)  ) ){

				++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_cmnd_OK;
			} else {
				++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_cmnd_timeout;
			}

			  /* copy the result back to our buffer */
			memcpy(&wan_udp_pkt->wan_udp_command, &mbox->wan_command, sizeof(TX25Cmd));

      	         	if(mbox->wan_data_len) {
        	               memcpy(&wan_udp_pkt->wan_udp_data, &mbox->wan_data, mbox->wan_data_len);
			}
			break;

		} //switch

        }
   
	if (local_dev){
		return 1;
	}

        /* Fill UDP TTL */

	wan_udp_pkt->wan_ip_ttl = card->wandev.ttl;
       
	len = reply_udp((u8*)&card->u.x.udp_pkt_data, mbox->wan_data_len);

        if(card->u.x.udp_pkt_src == UDP_PKT_FRM_NETWORK) {
		
		err = x25_send(card, lcn, 0, len, (u8*)&card->u.x.udp_pkt_data);
		if (!err) 
			++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_send_passed;
		else
			++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_send_failed;
	
	} else {

		/* Allocate socket buffer */
		if((new_skb = dev_alloc_skb(len)) != NULL) {
			void *buf;

			/* copy data into new_skb */
			buf = skb_put(new_skb, len);
			memcpy(buf, (u8*)&card->u.x.udp_pkt_data, len);
        
			/* Decapsulate packet and pass it up the protocol 
			   stack */
			new_skb->dev = dev;
	
			if (chan->common.usedby == API)
                        	new_skb->protocol = htons(X25_PROT);
			else 
				new_skb->protocol = htons(ETH_P_IP);
	
                        wan_skb_reset_mac_header(new_skb);

			netif_rx(new_skb);
			++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_passed_to_stack;
            	
		} else {
			++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_no_socket;
			printk(KERN_INFO 
			"%s: UDP mgmt cmnd, no socket buffers available!\n", 
			card->devname);
            	}
        }

	atomic_set(&card->u.x.udp_pkt_len,0);
	return 1;
}


/*==============================================================================
 * Determine what type of UDP call it is. DRVSTATS or XPIPE8ND ?
 */
static int udp_pkt_type( struct sk_buff *skb, sdla_t* card )
{
	wan_udp_pkt_t *wan_udp_pkt = (wan_udp_pkt_t *)skb->data;

	if (skb->len < sizeof(wan_udp_pkt_t)){
		return UDP_INVALID_TYPE;
	}

        if((wan_udp_pkt->wan_ip_p == UDPMGMT_UDP_PROTOCOL) &&
		(wan_udp_pkt->wan_udp_dport == ntohs(card->wandev.udp_port)) &&
		(wan_udp_pkt->wan_udp_request_reply == UDPMGMT_REQUEST)) {

                        if(!strncmp(wan_udp_pkt->wan_udp_signature,
                                UDPMGMT_XPIPE_SIGNATURE, 8)){
                                return UDP_XPIPE_TYPE;
			}
			
			if(!strncmp(wan_udp_pkt->wan_udp_signature,
                                GLOBAL_UDP_SIGNATURE, 8)){
                                return UDP_XPIPE_TYPE;
			}
			
			printk(KERN_INFO "%s: UDP Packet, Failed Signature !\n",
				card->devname);
	}

        return UDP_INVALID_TYPE;
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
		calc_checksum(&data[0],
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

/*=============================================================================
 * Store a UDP management packet for later processing.
 */

static int store_udp_mgmt_pkt(int udp_type, char udp_pkt_src, sdla_t* card,
                                netdevice_t *dev, struct sk_buff *skb, int lcn)
{
        int udp_pkt_stored = 0;

        if(!atomic_read(&card->u.x.udp_pkt_len) && (skb->len <= sizeof(card->u.x.udp_pkt_data))){
		atomic_set(&card->u.x.udp_pkt_len,skb->len);
                card->u.x.udp_type = udp_type;
                card->u.x.udp_pkt_src = udp_pkt_src;
                card->u.x.udp_lcn = lcn;
		card->u.x.udp_dev = dev;
                memcpy((u8*)&card->u.x.udp_pkt_data, skb->data, skb->len);
                card->u.x.timer_int_enabled |= TMR_INT_ENABLED_UDP_PKT;
                udp_pkt_stored = 1;

        }else{
                printk(KERN_INFO "%s: ERROR: UDP packet not stored for LCN %d\n", 
							card->devname,lcn);
	}

	wan_skb_free(skb);

        return(udp_pkt_stored);
}



/*=============================================================================
 * Initial the ppp_private_area structure.
 */
static void init_x25_channel_struct( x25_channel_t *chan )
{
	memset(&chan->if_send_stat.if_send_entry,0,sizeof(if_send_stat_t));
	memset(&chan->rx_intr_stat.rx_intr_no_socket,0,sizeof(rx_intr_stat_t));
	memset(&chan->pipe_mgmt_stat.UDP_PIPE_mgmt_kmalloc_err,0,sizeof(pipe_mgmt_stat_t));
}

/*============================================================================
 * Initialize Global Statistics
 */
static void init_global_statistics( sdla_t *card )
{
	memset(&card->statistics.isr_entry,0,sizeof(global_stats_t));
}


/*===============================================================
 * SMP Support
 * ==============================================================*/

static void S508_S514_lock(sdla_t *card, unsigned long *smp_flags)
{
	spin_lock_irqsave(&card->wandev.lock, *smp_flags);
}
static void S508_S514_unlock(sdla_t *card, unsigned long *smp_flags)
{
	spin_unlock_irqrestore(&card->wandev.lock, *smp_flags);
}

/*===============================================================
 * x25_timer_routine
 *
 * 	A more efficient polling routine.  Each half a second
 * 	queue a polling task. We want to do the polling in a 
 * 	task not timer, because timer runs in interrupt time.
 *
 * 	FIXME Polling should be rethinked.
 *==============================================================*/

static void x25_timer_routine(unsigned long data)
{
	sdla_t *card = (sdla_t*)data;

	if (test_bit(PERI_CRIT,&card->wandev.critical)){
		printk(KERN_INFO "%s: Stopping the X25 Poll Timer: Shutting down.\n",
				card->devname);
		return;
	}

	if (card->open_cnt != card->u.x.num_of_ch){
		printk(KERN_INFO "%s: Stopping the X25 Poll Timer: Interface down.\n",
				card->devname);
		return;
	}

	if (!WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head))){
		printk(KERN_INFO "%s: Stopping the X25 Poll Timer: No Dev.\n",
				card->devname);
		return;
	}
		
	if (!test_and_set_bit(POLL_CRIT,&card->wandev.critical)){
		trigger_x25_poll(card);
	}
	
	card->u.x.x25_timer.expires=jiffies+(HZ>>1);
	add_timer(&card->u.x.x25_timer);
	return;
}

void disable_comm_shutdown(sdla_t *card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int err;

	/* Turn of interrutps */
	mbox->wan_data[0] = 0;
	// ALEX_TODAY if (card->hw.fwid == SFID_X25_508){
	if (card->type == SDLA_S508){
		mbox->wan_data[1] = card->wandev.irq; // ALEX_TODAY card->hw.irq;
		mbox->wan_data[2] = 2;
		mbox->wan_data_len = 3;
	}else {
	 	mbox->wan_data_len  = 1;
	}
	mbox->wan_command = X25_SET_INTERRUPT_MODE;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if (err)
		printk(KERN_INFO "INTERRUPT OFF FAIED %x\n",err);

	/* Bring down HDLC */
	mbox->wan_command = X25_HDLC_LINK_CLOSE;
	mbox->wan_data_len  = 0;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if (err)
		printk(KERN_INFO "LINK CLOSED FAILED %x\n",err);


	/* Brind down DTR */
	mbox->wan_data[0] = 0;
	mbox->wan_data[2] = 0;
	mbox->wan_data[1] = 0x01;
	mbox->wan_data_len  = 3;
	mbox->wan_command = X25_SET_GLOBAL_VARS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if (err)
		printk(KERN_INFO "DTR DOWN FAILED %x\n",err);

}

static void x25_clear_cmd_update(sdla_t* card, unsigned lcn, wan_mbox_t* mb)
{
	netdevice_t* 	dev = find_channel(card, lcn);
	x25_channel_t*	chan = NULL;
	x25_call_info_t info;
	struct timeval 	tv;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return;

	chan=wan_netif_priv(dev);
	do_gettimeofday(&tv);
	chan->chan_clear_time = tv.tv_sec;
	chan->chan_clear_cause = mb->wan_x25_cause;
	chan->chan_clear_diagn = mb->wan_x25_diagn;

	if (mb->wan_data_len){
		parse_call_info(mb->wan_data, &info);
		memcpy(chan->cleared_called_addr, info.src, MAX_X25_ADDR_SIZE);
		memcpy(chan->cleared_calling_addr, info.dest, MAX_X25_ADDR_SIZE);
		memcpy(chan->cleared_facil, info.facil, MAX_X25_FACL_SIZE);
	}
	return;
}

/*
 ********************************************************************
 * Proc FS function 
 */
#define PROC_CFG_FRM	"%-15s| %-12s| %-5u| %-9s| %-13s| %-13s|\n"
#define PROC_STAT_FRM	"%-15s| %-12s| %-5u| %-14s|\n"
static char x25_config_hdr[] =
	"Interface name | Device name | LCN  | Src Addr | Accept DAddr | Accept UData |\n";
static char x25_status_hdr[] =
	"Interface name | Device name | LCN  | Status        |\n";

static int x25_get_config_info(void* priv, struct seq_file* m, int* stop_cnt) 
{
	x25_channel_t*	chan = (x25_channel_t*)priv;
	sdla_t*		card = chan->card;

	if (chan == NULL)
		return m->count;
	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", x25_config_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_CFG_FRM, chan->name, card->devname, chan->ch_idx,
		chan->x25_src_addr, chan->accept_dest_addr, chan->accept_usr_data);
	return m->count;
}

static int x25_get_status_info(void* priv, struct seq_file* m, int* stop_cnt)
{
	x25_channel_t*	chan = (x25_channel_t*)priv;
	sdla_t*		card = chan->card;

	if (chan == NULL)
		return m->count;
	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", x25_status_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_STAT_FRM, chan->name, card->devname, 
		chan->ch_idx, STATE_DECODE(chan->common.state));
	return m->count;
}

#define PROC_DEV_FR_SS_FRM	"%-20s| %-12s|%-20s| %-12s|\n"
#define PROC_DEV_FR_SD_FRM	"%-20s| %-12s|%-20s| %-12d|\n"
#define PROC_DEV_FR_DD_FRM	"%-20s| %-12d|%-20s| %-12d|\n"
#define PROC_DEV_FR_XD_FRM	"%-20s| 0x%-10X|%-20s| %-12d|\n"
#define PROC_DEV_SEPARATE	"==================================="
#define PROC_DEV_TITLE		" Parameters         | Value       |"


static int x25_set_dev_config(struct file *file, 
			     const char *buffer,
			     unsigned long count, 
			     void *data)
{
	wan_device_t*	wandev = (void*)data;
	sdla_t*		card = NULL;

	if (wandev == NULL)
		return count;

	card = (sdla_t*)wandev->priv;

	printk(KERN_INFO "%s: New device config (%s)\n",
			wandev->name, buffer);
	/* Parse string */

	return count;
}

/* SNMP
 ******************************************************************************
 *			x25_snmp_data()	
 *
 * Description: Save snmp request and parameters in private structure, enable
 *		TIMER interrupt, put current process in sleep.
 * Arguments:   
 * Returns:
 ******************************************************************************
 */
#define   X25ADMNINDEX          		3
#define   X25ADMNINTERFACEMODE  		4
#define   X25ADMNMAXACTIVECIRCUITS  		5
#define   X25ADMNPACKETSEQUENCING  		6
#define   X25ADMNRESTARTTIMER   		7
#define   X25ADMNCALLTIMER      		8
#define   X25ADMNRESETTIMER     		9
#define   X25ADMNCLEARTIMER     		10
#define   X25ADMNWINDOWTIMER    		11
#define   X25ADMNDATARXMTTIMER  		12
#define   X25ADMNINTERRUPTTIMER  		13
#define   X25ADMNREJECTTIMER    		14
#define   X25ADMNREGISTRATIONREQUESTTIMER  	15
#define   X25ADMNMINIMUMRECALLTIMER  		16
#define   X25ADMNRESTARTCOUNT   		17
#define   X25ADMNRESETCOUNT     		18
#define   X25ADMNCLEARCOUNT     		19
#define   X25ADMNDATARXMTCOUNT  		20
#define   X25ADMNREJECTCOUNT    		21
#define   X25ADMNREGISTRATIONREQUESTCOUNT  	22
#define   X25ADMNNUMBERPVCS     		23
#define   X25ADMNDEFCALLPARAMID  		24
#define   X25ADMNLOCALADDRESS   		25
#define   X25ADMNPROTOCOLVERSIONSUPPORTED  	26
#define   X25OPERINDEX          		29
#define   X25OPERINTERFACEMODE  		30
#define   X25OPERMAXACTIVECIRCUITS  		31
#define   X25OPERPACKETSEQUENCING  		32
#define   X25OPERRESTARTTIMER   		33
#define   X25OPERCALLTIMER      		34
#define   X25OPERRESETTIMER     		35
#define   X25OPERCLEARTIMER     		36
#define   X25OPERWINDOWTIMER    		37
#define   X25OPERDATARXMTTIMER  		38
#define   X25OPERINTERRUPTTIMER  		39
#define   X25OPERREJECTTIMER    		40	
#define   X25OPERREGISTRATIONREQUESTTIMER  	41
#define   X25OPERMINIMUMRECALLTIMER  		42
#define   X25OPERRESTARTCOUNT   		43
#define   X25OPERRESETCOUNT     		44
#define   X25OPERCLEARCOUNT     		45
#define   X25OPERDATARXMTCOUNT  		46
#define   X25OPERREJECTCOUNT    		47
#define   X25OPERREGISTRATIONREQUESTCOUNT  	48
#define   X25OPERNUMBERPVCS     		49
#define   X25OPERDEFCALLPARAMID  		50
#define   X25OPERLOCALADDRESS   		51
#define   X25OPERDATALINKID     		52
#define   X25OPERPROTOCOLVERSIONSUPPORTED  	53
#define   X25STATINDEX          		56
#define   X25STATINCALLS        		57
#define   X25STATINCALLREFUSALS  		58
#define   X25STATINPROVIDERINITIATEDCLEARS  	59
#define   X25STATINREMOTELYINITIATEDRESETS  	60
#define   X25STATINPROVIDERINITIATEDRESETS  	61
#define   X25STATINRESTARTS     		62
#define   X25STATINDATAPACKETS  		63
#define   X25STATINACCUSEDOFPROTOCOLERRORS  	64
#define   X25STATININTERRUPTS   		65
#define   X25STATOUTCALLATTEMPTS  		66
#define   X25STATOUTCALLFAILURES  		67
#define   X25STATOUTINTERRUPTS  		68
#define   X25STATOUTDATAPACKETS  		69
#define   X25STATOUTGOINGCIRCUITS  		70
#define   X25STATINCOMINGCIRCUITS  		71
#define   X25STATTWOWAYCIRCUITS  		72
#define   X25STATRESTARTTIMEOUTS  		73
#define   X25STATCALLTIMEOUTS   		74
#define   X25STATRESETTIMEOUTS  		75
#define   X25STATCLEARTIMEOUTS  		76
#define   X25STATDATARXMTTIMEOUTS  		77
#define   X25STATINTERRUPTTIMEOUTS  		78
#define   X25STATRETRYCOUNTEXCEEDEDS  		79
#define   X25STATCLEARCOUNTEXCEEDEDS  		80
#define   X25CHANNELINDEX       		83
#define   X25CHANNELLIC         		84
#define   X25CHANNELHIC         		85
#define   X25CHANNELLTC         		86
#define   X25CHANNELHTC         		87
#define   X25CHANNELLOC         		88
#define   X25CHANNELHOC         		89
#define   X25CIRCUITINDEX       		92
#define   X25CIRCUITCHANNEL     		93
#define   X25CIRCUITSTATUS      		94
#define   X25CIRCUITESTABLISHTIME  		95
#define   X25CIRCUITDIRECTION   		96
#define   X25CIRCUITINOCTETS    		97
#define   X25CIRCUITINPDUS      		98
#define   X25CIRCUITINREMOTELYINITIATEDRESETS  	99
#define   X25CIRCUITINPROVIDERINITIATEDRESETS  	100
#define   X25CIRCUITININTERRUPTS  		101
#define   X25CIRCUITOUTOCTETS   		102
#define   X25CIRCUITOUTPDUS     		103
#define   X25CIRCUITOUTINTERRUPTS  		104
#define   X25CIRCUITDATARETRANSMISSIONTIMEOUTS  105
#define   X25CIRCUITRESETTIMEOUTS  		106
#define   X25CIRCUITINTERRUPTTIMEOUTS  		107
#define   X25CIRCUITCALLPARAMID  		108
#define   X25CIRCUITCALLEDDTEADDRESS  		109
#define   X25CIRCUITCALLINGDTEADDRESS  		110
#define   X25CIRCUITORIGINALLYCALLEDADDRESS  	111
#define   X25CIRCUITDESCR       		112
#define   X25CLEAREDCIRCUITENTRIESREQUESTED  	113
#define   X25CLEAREDCIRCUITENTRIESGRANTED  	114
#define   X25CLEAREDCIRCUITINDEX  		117
#define   X25CLEAREDCIRCUITPLEINDEX  		118
#define   X25CLEAREDCIRCUITTIMEESTABLISHED  	119
#define   X25CLEAREDCIRCUITTIMECLEARED  	120
#define   X25CLEAREDCIRCUITCHANNEL  		121
#define   X25CLEAREDCIRCUITCLEARINGCAUSE  	122
#define   X25CLEAREDCIRCUITDIAGNOSTICCODE  	123
#define   X25CLEAREDCIRCUITINPDUS  		124
#define   X25CLEAREDCIRCUITOUTPDUS  		125
#define   X25CLEAREDCIRCUITCALLEDADDRESS  	126
#define   X25CLEAREDCIRCUITCALLINGADDRESS  	127
#define   X25CLEAREDCIRCUITCLEARFACILITIES  	128
#define   X25CALLPARMINDEX      		131
#define   X25CALLPARMSTATUS     		132
#define   X25CALLPARMREFCOUNT   		133
#define   X25CALLPARMINPACKETSIZE  		134
#define   X25CALLPARMOUTPACKETSIZE  		135
#define   X25CALLPARMINWINDOWSIZE  		136
#define   X25CALLPARMOUTWINDOWSIZE  		137
#define   X25CALLPARMACCEPTREVERSECHARGING  	138
#define   X25CALLPARMPROPOSEREVERSECHARGING  	139
#define   X25CALLPARMFASTSELECT  		140
#define   X25CALLPARMINTHRUPUTCLASSIZE  	141
#define   X25CALLPARMOUTTHRUPUTCLASSIZE  	142
#define   X25CALLPARMCUG        		143
#define   X25CALLPARMCUGOA      		144
#define   X25CALLPARMBCUG       		145
#define   X25CALLPARMNUI        		146
#define   X25CALLPARMCHARGINGINFO  		147
#define   X25CALLPARMRPOA       		148
#define   X25CALLPARMTRNSTDLY   		149
#define   X25CALLPARMCALLINGEXT  		150
#define   X25CALLPARMCALLEDEXT  		151
#define   X25CALLPARMINMINTHUPUTCLS  		152
#define   X25CALLPARMOUTMINTHUPUTCLS  		153
#define   X25CALLPARMENDTRNSDLY  		154
#define   X25CALLPARMPRIORITY   		155
#define   X25CALLPARMPROTECTION  		156
#define   X25CALLPARMEXPTDATA   		157
#define   X25CALLPARMUSERDATA   		158
#define   X25CALLPARMCALLINGNETWORKFACILITIES  	159
#define   X25CALLPARMCALLEDNETWORKFACILITIES  	160

static int x25_snmp_data(sdla_t* card, netdevice_t *dev, void* data)
{
	x25_channel_t* 	chan = NULL;
	wanpipe_snmp_t*	snmp;
	
	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return -EFAULT;
	chan = (x25_channel_t*)wan_netif_priv(dev);
	/* Update device statistics */
	if (card->wandev.update) {
		int rslt = 0;
		rslt = card->wandev.update(&card->wandev);
		if(rslt) {
			return (rslt) ? (-EBUSY) : (-EINVAL);
		}
	}

	snmp = (wanpipe_snmp_t*)data;

	switch(snmp->snmp_magic){
	/********** X.25 Administration Table *********/
	case X25ADMNINDEX:
		break;

	case X25ADMNINTERFACEMODE:
		snmp->snmp_val = 
			(card->wandev.station == WANOPT_DTE) ? SNMP_X25_DTE:
			(card->wandev.station == WANOPT_DCE) ? SNMP_X25_DCE:SNMP_X25_DXE;
		break;

	case X25ADMNMAXACTIVECIRCUITS:
	        snmp->snmp_val = card->u.x.num_of_ch;
		break;

	case X25ADMNPACKETSEQUENCING:
        	snmp->snmp_val = SNMP_X25_MODULO8;
		break;

	case X25ADMNRESTARTTIMER:
		snmp->snmp_val = card->u.x.x25_adm_conf.t10_t20;
		break;

	case X25ADMNCALLTIMER:
		snmp->snmp_val = card->u.x.x25_adm_conf.t11_t21;
		break;

	case X25ADMNRESETTIMER:
		snmp->snmp_val = card->u.x.x25_adm_conf.t12_t22;
		break;

	case X25ADMNCLEARTIMER:
		snmp->snmp_val = card->u.x.x25_adm_conf.t13_t23;
		break;

	case X25ADMNWINDOWTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNDATARXMTTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNINTERRUPTTIMER:
		snmp->snmp_val = card->u.x.x25_adm_conf.t16_t26;
		break;

	case X25ADMNREJECTTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNREGISTRATIONREQUESTTIMER:
		snmp->snmp_val = card->u.x.x25_adm_conf.t28;
		break;

	case X25ADMNMINIMUMRECALLTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNRESTARTCOUNT:
		snmp->snmp_val = card->u.x.x25_adm_conf.r10_r20;
		break;

	case X25ADMNRESETCOUNT:
		snmp->snmp_val = card->u.x.x25_adm_conf.r12_r22;
		break;

	case X25ADMNCLEARCOUNT:    	
		snmp->snmp_val = card->u.x.x25_adm_conf.r13_r23;
		break;

	case X25ADMNDATARXMTCOUNT: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNREJECTCOUNT: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNREGISTRATIONREQUESTCOUNT: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNNUMBERPVCS:     		
		snmp->snmp_val = card->u.x.x25_adm_conf.hi_pvc;
		break;

	case X25ADMNDEFCALLPARAMID: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNLOCALADDRESS: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25ADMNPROTOCOLVERSIONSUPPORTED: /* FIXME */
		snmp->snmp_val = 0;
		break;

	/********** X.25 Operational Table *********/
	case X25OPERINDEX:
		break;

	case X25OPERINTERFACEMODE:	
		snmp->snmp_val = (card->wandev.station == WANOPT_DTE) ? SNMP_X25_DTE:
		 	   (card->wandev.station == WANOPT_DCE) ? SNMP_X25_DCE:SNMP_X25_DXE;
		break;

	case X25OPERMAXACTIVECIRCUITS:  	
		snmp->snmp_val = card->u.x.num_of_ch;
		break;

	case X25OPERPACKETSEQUENCING:  
		snmp->snmp_val = SNMP_X25_MODULO8;
		break;

	case X25OPERRESTARTTIMER:   	
		snmp->snmp_val = card->u.x.x25_conf.t10_t20;
		break;

	case X25OPERCALLTIMER:      	
		snmp->snmp_val = card->u.x.x25_conf.t11_t21;
		break;

	case X25OPERRESETTIMER:     
		snmp->snmp_val = card->u.x.x25_conf.t12_t22;
		break;

	case X25OPERCLEARTIMER:    
		snmp->snmp_val = card->u.x.x25_conf.t13_t23;
		break;

	case X25OPERWINDOWTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25OPERDATARXMTTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25OPERINTERRUPTTIMER: 
		snmp->snmp_val = card->u.x.x25_conf.t16_t26;
		break;

	case X25OPERREJECTTIMER: /* FIXME */
		snmp->snmp_val = 0;
		break;

	case X25OPERREGISTRATIONREQUESTTIMER: 
                snmp->snmp_val = card->u.x.x25_conf.t28;
		break;

	case X25OPERMINIMUMRECALLTIMER: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25OPERRESTARTCOUNT:  
                snmp->snmp_val = card->u.x.x25_conf.r10_r20;
		break;

	case X25OPERRESETCOUNT:    
                snmp->snmp_val = card->u.x.x25_conf.r12_r22;
		break;

	case X25OPERCLEARCOUNT:    
                snmp->snmp_val = card->u.x.x25_conf.r13_r23;
		break;

	case X25OPERDATARXMTCOUNT: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25OPERREJECTCOUNT: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25OPERREGISTRATIONREQUESTCOUNT: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25OPERNUMBERPVCS:     
                snmp->snmp_val = card->u.x.x25_conf.hi_pvc;
		break;

	case X25OPERDEFCALLPARAMID: /* FIXME */
                snmp->snmp_val = 0;
		break;
	
	case X25OPERLOCALADDRESS: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25OPERDATALINKID: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25OPERPROTOCOLVERSIONSUPPORTED: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;


		 
	/********** X.25 Statistics Table *********/
	case X25STATINDEX:
		break;

	case X25STATINCALLS:       
        	snmp->snmp_val = X25Stats.rxCallRequest;
		break;

	case X25STATINCALLREFUSALS: 
                snmp->snmp_val = X25Stats.txClearRqst;
		break;

	case X25STATINPROVIDERINITIATEDCLEARS: 
                snmp->snmp_val = (card->wandev.station == WANOPT_DTE) ? 
					X25Stats.rxClearRqst:
					X25Stats.txClearRqst;
		break;

	case X25STATINREMOTELYINITIATEDRESETS:  
                snmp->snmp_val = (card->wandev.station == WANOPT_DTE) ? 
					X25Stats.txResetRqst:
					X25Stats.rxResetRqst;
		break;

	case X25STATINPROVIDERINITIATEDRESETS:  
                snmp->snmp_val = (card->wandev.station == WANOPT_DTE) ? 
					X25Stats.rxResetRqst:
					X25Stats.txResetRqst;
		break;

	case X25STATINRESTARTS:     
                snmp->snmp_val = X25Stats.rxRestartRqst;
		break;

	case X25STATINDATAPACKETS:  
                snmp->snmp_val = X25Stats.rxData;
		break;

	case X25STATINACCUSEDOFPROTOCOLERRORS:  
                snmp->snmp_val = X25Stats.rxClearRqst+X25Stats.rxResetRqst+
				X25Stats.rxRestartRqst+X25Stats.rxDiagnostic;
		break;

	case X25STATININTERRUPTS:   
                snmp->snmp_val = X25Stats.rxInterrupt;
		break;

	case X25STATOUTCALLATTEMPTS:  
                snmp->snmp_val = X25Stats.txCallRequest;
		break;

	case X25STATOUTCALLFAILURES:  
                snmp->snmp_val = X25Stats.rxClearRqst;
		break;

	case X25STATOUTINTERRUPTS: 
                snmp->snmp_val = X25Stats.txInterrupt;
		break;

	case X25STATOUTDATAPACKETS: 
                snmp->snmp_val = X25Stats.txData;
		break;

	case X25STATOUTGOINGCIRCUITS: 
                snmp->snmp_val = X25Stats.txCallRequest-X25Stats.rxClearRqst;
		break;

	case X25STATINCOMINGCIRCUITS:  
                snmp->snmp_val = X25Stats.rxCallRequest-X25Stats.txClearRqst;
		break;

	case X25STATTWOWAYCIRCUITS:  
                snmp->snmp_val = X25Stats.txCallRequest-X25Stats.rxClearRqst;
		break;

	case X25STATRESTARTTIMEOUTS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATCALLTIMEOUTS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATRESETTIMEOUTS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATCLEARTIMEOUTS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATDATARXMTTIMEOUTS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATINTERRUPTTIMEOUTS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATRETRYCOUNTEXCEEDEDS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25STATCLEARCOUNTEXCEEDEDS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	/********** X.25 Channel Table *********/
	case X25CHANNELINDEX: 
		break;

	case X25CHANNELLIC:        
		snmp->snmp_val = card->u.x.x25_conf.lo_svc;
		break;

	case X25CHANNELHIC:         
                snmp->snmp_val = card->u.x.x25_conf.hi_svc;
		break;

	case X25CHANNELLTC:        
                snmp->snmp_val = card->u.x.x25_conf.lo_svc;
		break;

	case X25CHANNELHTC:        
                snmp->snmp_val = card->u.x.x25_conf.hi_svc;
		break;

	case X25CHANNELLOC:        
                snmp->snmp_val = card->u.x.x25_conf.lo_svc;
		break;

	case X25CHANNELHOC:         
                snmp->snmp_val = card->u.x.x25_conf.hi_svc;
		break;

	/********** X.25 Per Circuits Information Table *********/
	case X25CIRCUITINDEX:
		break;

	case X25CIRCUITCHANNEL: /* FIXME */
               	snmp->snmp_val = 0;
		break;

	case X25CIRCUITSTATUS: /* FIXME */ 
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITESTABLISHTIME: 
                snmp->snmp_val = chan->chan_establ_time;
		break;

	case X25CIRCUITDIRECTION:   
                snmp->snmp_val = chan->chan_direct;
		break;

	case X25CIRCUITINOCTETS:    
                snmp->snmp_val = chan->ifstats.rx_bytes;
		break;

	case X25CIRCUITINPDUS:      
                snmp->snmp_val = chan->ifstats.rx_packets;
		break;

	case X25CIRCUITINREMOTELYINITIATEDRESETS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITINPROVIDERINITIATEDRESETS: /* FIXME */        
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITININTERRUPTS:  
                snmp->snmp_val = X25Stats.rxInterrupt;
		break;

	case X25CIRCUITOUTOCTETS:   
                snmp->snmp_val = chan->ifstats.tx_bytes;
		break;

	case X25CIRCUITOUTPDUS:     
                snmp->snmp_val = chan->ifstats.tx_packets;
		break;

	case X25CIRCUITOUTINTERRUPTS:  
                snmp->snmp_val = X25Stats.txInterrupt;
		break;

	case X25CIRCUITDATARETRANSMISSIONTIMEOUTS: /* FIXME */	       
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITRESETTIMEOUTS: /* FIXME */	       
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITINTERRUPTTIMEOUTS: /* FIXME */	       
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITCALLPARAMID: /* FIXME */	       
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITCALLEDDTEADDRESS: /* FIXME */       
                strcpy((void*)snmp->snmp_data,
		 	(chan->chan_direct == SNMP_X25_INCOMING) ? chan->accept_src_addr : "N/A");
		break;

	case X25CIRCUITCALLINGDTEADDRESS: /* FIXME */	       
                strcpy((void*)snmp->snmp_data,
		        (chan->chan_direct == SNMP_X25_INCOMING) ? chan->accept_dest_addr : "N/A");
		break;

	case X25CIRCUITORIGINALLYCALLEDADDRESS: /* FIXME */       
                snmp->snmp_val = 0;
		break;

	case X25CIRCUITDESCR: /* FIXME */      
                strcpy((void*)snmp->snmp_data, "N/A");
		break;


	case X25CLEAREDCIRCUITENTRIESREQUESTED: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CLEAREDCIRCUITENTRIESGRANTED: /* FIXME */
                snmp->snmp_val = 0;
		break;

	/********** X.25 The Cleared Circuit Table *********/
	case X25CLEAREDCIRCUITINDEX: /* FIXME */
        	snmp->snmp_val = 0;
		break;

	case X25CLEAREDCIRCUITPLEINDEX: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CLEAREDCIRCUITTIMEESTABLISHED: 
                snmp->snmp_val = chan->chan_establ_time;
		break;

	case X25CLEAREDCIRCUITTIMECLEARED: 
                snmp->snmp_val = chan->chan_clear_time;
		break;

	case X25CLEAREDCIRCUITCHANNEL:  
                snmp->snmp_val = chan->ch_idx;
		break;

	case X25CLEAREDCIRCUITCLEARINGCAUSE:  
                snmp->snmp_val = chan->chan_clear_cause;
		break;

	case X25CLEAREDCIRCUITDIAGNOSTICCODE: 
                snmp->snmp_val = chan->chan_clear_diagn;
		break;

	case X25CLEAREDCIRCUITINPDUS: 
                snmp->snmp_val = chan->ifstats.rx_packets;
		break;

	case X25CLEAREDCIRCUITOUTPDUS:  
                snmp->snmp_val = chan->ifstats.tx_packets;
		break;

	case X25CLEAREDCIRCUITCALLEDADDRESS:  
                strcpy((void*)snmp->snmp_data, 
		 	(chan->cleared_called_addr[0] != '\0') ? 
		 			chan->cleared_called_addr : "N/A");
		break;

	case X25CLEAREDCIRCUITCALLINGADDRESS:  
                strcpy((void*)snmp->snmp_data,
		 	(chan->cleared_calling_addr[0] != '\0') ? 
		 			chan->cleared_calling_addr : "N/A");
		break;

	case X25CLEAREDCIRCUITCLEARFACILITIES:  
                strcpy((void*)snmp->snmp_data,
		 	(chan->cleared_facil[0] != '\0') ? chan->cleared_facil : "N/A");
		break;



	/********** X.25 The Call Parameter Table *********/
	case X25CALLPARMINDEX: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CALLPARMSTATUS: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CALLPARMREFCOUNT: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CALLPARMINPACKETSIZE:  
                snmp->snmp_val = 0;	// default size
		break;

	case X25CALLPARMOUTPACKETSIZE: 
                snmp->snmp_val = 0;	// default size
		break;

	case X25CALLPARMINWINDOWSIZE:
                snmp->snmp_val = (card->u.x.x25_conf.pkt_window != X25_PACKET_WINDOW) ? 
				card->u.x.x25_conf.pkt_window : 0;
		break;

	case X25CALLPARMOUTWINDOWSIZE: 
                snmp->snmp_val = (card->u.x.x25_conf.pkt_window != X25_PACKET_WINDOW) ? 
				card->u.x.x25_conf.pkt_window : 0;
		break;

	case X25CALLPARMACCEPTREVERSECHARGING: /* FIXME */
                snmp->snmp_val = SNMP_X25_ARC_DEFAULT;
		break;

	case X25CALLPARMPROPOSEREVERSECHARGING: /* FIXME */
                snmp->snmp_val = SNMP_X25_PRC_DEFAULT;
		break;

	case X25CALLPARMFASTSELECT: /* FIXME */
                snmp->snmp_val = SNMP_X25_FS_DEFAULT;
		break;

	case X25CALLPARMINTHRUPUTCLASSIZE: /* FIXME */
                snmp->snmp_val = SNMP_X25_THRUCLASS_TCDEF;
		break;

	case X25CALLPARMOUTTHRUPUTCLASSIZE: /* FIXME */
                snmp->snmp_val = SNMP_X25_THRUCLASS_TCDEF;
		break;

	case X25CALLPARMCUG: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMCUGOA: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMBCUG: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMNUI: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMCHARGINGINFO: /* FIXME */
                snmp->snmp_val = SNMP_X25_CHARGINGINFO_DEF;
		break;

	case X25CALLPARMRPOA: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMTRNSTDLY: /* FIXME */
                snmp->snmp_val = 0;
		break;

	case X25CALLPARMCALLINGEXT: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMCALLEDEXT: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMINMINTHUPUTCLS: /* FIXME */
                snmp->snmp_val = SNMP_X25_THRUCLASS_TCDEF;
		break;

	case X25CALLPARMOUTMINTHUPUTCLS: /* FIXME */
                snmp->snmp_val = SNMP_X25_THRUCLASS_TCDEF;
		break;

	case X25CALLPARMENDTRNSDLY: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMPRIORITY: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMPROTECTION: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMEXPTDATA: /* FIXME */
                snmp->snmp_val = SNMP_X25_EXPTDATA_DEFULT;
		break;

	case X25CALLPARMUSERDATA:   
                strcpy((void*)snmp->snmp_data, chan->accept_usr_data);
		break;

	case X25CALLPARMCALLINGNETWORKFACILITIES: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	case X25CALLPARMCALLEDNETWORKFACILITIES: /* FIXME */
                strcpy((void*)snmp->snmp_data, "N/A");
		break;

	default:
            	return -EAFNOSUPPORT;
	}

	return 0;

}

#define PROC_IF_X25_S_FRM	"%-40s %-14s\n"
#define PROC_IF_X25_D_FRM	"%-40s %-14d\n"
#define PROC_IF_X25_L_FRM	"%-40s %-14ld\n"
#define PROC_IF_SEPARATE	"====================================================\n"
	
static int x25_set_if_info(struct file *file, 
			  const char *buffer,
			  unsigned long count, 
			  void *data)
{
	netdevice_t*	dev = (void*)data;
	x25_channel_t* 	chan = NULL;
	sdla_t*		card = NULL;

	if (dev == NULL || wan_netif_priv(dev) == NULL)
		return count;
	chan = (x25_channel_t*)wan_netif_priv(dev);
	if (chan->card == NULL)
		return count;
	card = chan->card;

	printk(KERN_INFO "%s: New interface config (%s)\n",
			chan->name, buffer);
	/* Parse string */

	return count;
}

static int baud_rate_check (int baud)
{	
	switch (baud){
	
	case 1024:
	case 512:
	case 256:
	case 128:
	case 64:
	case 16:
		return 0;

	default:
		return -EINVAL;
	}

	return -EINVAL;
}


static void release_svc_dev(sdla_t *card, netdevice_t *dev)
{
	x25_channel_t *chan = wan_netif_priv(dev);
	clear_bit(0,(void *)&chan->common.rw_bind);
}

static netdevice_t *find_free_svc_dev(sdla_t *card)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev;
	x25_channel_t 		*chan;

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev) || !(dev->flags&IFF_UP)){
			continue;
		}

		chan = wan_netif_priv(dev);
		if (chan->common.usedby == API && chan->common.svc){
			if (!test_and_set_bit(0,(void *)&chan->common.rw_bind)){
				
				if (chan->common.state != WANSOCK_DISCONNECTED){
					clear_bit(0,(void *)&chan->common.rw_bind);
				}

				return dev;
			}
		}
	}

	return NULL;
}



/*==============================================================
 * bind_api_to_svc
 *
 * Description:
 *	The api socket layer uses this function to bind
 *	the SVC data socket to the x25_lcn link.
 *
 *	All data received by the svc will be passed up 
 *	to the socket via bounded sk_id.
 *
 * Used by: 
 * 	api socket layer.
 * 
 */

static int bind_api_to_svc(sdla_t *card, void *sk_id)
{
	netdevice_t *dev;
	x25_channel_t *chan;

	WAN_ASSERT2((sk_id==NULL),-ENODEV);

	dev=find_free_svc_dev(card);
	if (!dev){
		DEBUG_EVENT("%s NO DEV\n",__FUNCTION__);
		return -ENODEV;
	}	

	chan = wan_netif_priv(dev);

	sock_hold(sk_id);
	chan->common.sk=sk_id;
	set_bit(LCN_SK_ID,&chan->common.used);
	
	return dev->ifindex;
}

static int x25_api_cmd_setup(x25_channel_t *chan, struct ifreq *ifr)
{
	x25api_t *x25api;
	int err;
	
	memset(&chan->x25_api_cmd_hdr,0,sizeof(x25api_hdr_t));
	memset(chan->call_string,0,sizeof(chan->call_string));
		
	if (!ifr){
		DEBUG_EVENT("%s: No ifr ptr!\n",__FUNCTION__);
		return -EINVAL;
	}
		
	x25api = (x25api_t*)ifr->ifr_data;
	if (!x25api){
		DEBUG_EVENT("%s: No ifr->ifr_data ptr!\n",__FUNCTION__);
		return -EINVAL;
	}
		
	err = copy_from_user(&chan->x25_api_cmd_hdr,x25api,sizeof(x25api_hdr_t));
	if (err){
		return err;
	}

	if (x25api->hdr.length > 0 && x25api->hdr.length <= X25_CALL_STR_SZ){
		err = copy_from_user(chan->call_string,x25api->data,x25api->hdr.length);
	}else{
		printk(KERN_INFO "%s: Invalid Place Call string: %s!\n",
				chan->common.dev->name,
				x25api->hdr.length ? "Call string too big" : 
						     "Call string size is zero");
		err = -EINVAL;
	}
	return err;
}

static int x25_api_get_cmd_result(x25_channel_t *chan, struct ifreq *ifr)
{
	x25api_t *x25api;
	int err=0;

	if (!ifr){
		DEBUG_EVENT("%s: No ifr ptr!\n",__FUNCTION__);
		return -EINVAL;
	}
	
	x25api = (x25api_t*)ifr->ifr_data;

	if (!x25api){
		DEBUG_EVENT("%s: No ifr->ifr_data ptr!\n",__FUNCTION__);
		return -EINVAL;
	}

	err=copy_to_user(x25api,&chan->x25_api_event,sizeof(x25api_t));
	
	return err;
}

static int x25_api_setup_cmd_hdr(x25_channel_t *chan, struct ifreq *ifr)
{
	x25api_t *x25api;
	int err;

	memset(&chan->x25_api_cmd_hdr,0,sizeof(x25api_hdr_t));

	if (ifr && ifr->ifr_data){
		x25api = (x25api_t*)ifr->ifr_data;
		err=copy_from_user(&chan->x25_api_cmd_hdr,
				   x25api,
				   sizeof(x25api_hdr_t));
		if (err) 
			return err;
	}

	return 0;
}

static int x25_api_setup_call_string(x25_channel_t *chan, struct ifreq *ifr)
{
	x25api_t *x25api;
	int err;

	memset(chan->call_string,0,sizeof(chan->call_string));

	if (ifr && ifr->ifr_data){
		x25api = (x25api_t*)ifr->ifr_data;
		if (x25api->hdr.length > 0 && x25api->hdr.length <= X25_CALL_STR_SZ){
			err = copy_from_user(chan->call_string,
					     x25api->data,
					     x25api->hdr.length);
			if (err) 
				return err;
		}
	}

	return 0;
}

static void x25_update_api_state(sdla_t *card,x25_channel_t *chan)
{
	int err=0;

	/* If the LCN state changes from Connected to Disconnected, and
	 * we are in the API mode, then notify the socket that the 
	 * connection has been lost */

	if (chan->common.usedby == API && chan->common.sk){

		if (chan->common.state != WAN_CONNECTED && test_bit(0,&chan->api_state)){
			clear_bit(0,&chan->api_state);
			protocol_disconnected (chan->common.sk);
			wan_unbind_api_from_svc(chan,chan->common.sk);
			return;
		}

			
		if (chan->common.state == WAN_CONNECTED && !test_bit(0,&chan->api_state)){

			set_bit(0,&chan->api_state);
			err=protocol_connected (chan->common.dev,chan->common.sk);
			if (err == -EINVAL){
				int lcn=chan->common.lcn;
				printk(KERN_INFO "%s:Major Error in Socket Above: CONN!!!\n",
						chan->common.dev->name);
			
				wan_unbind_api_from_svc(chan,chan->common.sk);
				x25_clear_call(card,lcn,0,0,0,NULL,0);
			}
			return;
		}

		if (chan->common.state == WAN_DISCONNECTED){
			err = protocol_disconnected (chan->common.sk);
			wan_unbind_api_from_svc(chan,chan->common.sk);
		}
	}

	if (chan->common.state != WAN_CONNECTED){
		clear_bit(0,&chan->api_state);
	}
}


static int test_chan_command_busy(sdla_t *card,x25_channel_t *chan)
{

	if (card->wandev.state != WAN_CONNECTED){
		return -ENOTCONN;
	}
	
	if (atomic_read(&chan->common.command)){
		DEBUG_EVENT("%s: Chan Command busy %x\n",
				card->devname,
				atomic_read(&chan->common.command));
		return -EBUSY;
	}

	return 0;
}


static int wait_for_cmd_rc(sdla_t *card, x25_channel_t *chan)
{
	chan->cmd_timeout=jiffies;
		
	while((jiffies-chan->cmd_timeout) < (card->u.x.x25_conf.cmd_retry_timeout*HZ)){
		if (!atomic_read(&chan->common.command)){
			return atomic_read(&chan->cmd_rc);	
		}
		schedule();
	}

	DEBUG_EVENT("%s: Warning: Delayed Command %x Timed out !\n",
			card->devname,atomic_read(&chan->common.command));
	
	atomic_set(&chan->common.command,0);
	return -EFAULT;
}


static int lapb_connect(sdla_t *card, x25_channel_t *chan )
{
	int err;
	netdevice_t *dev=chan->common.dev;

	err=connect(card);
	if (err==0){
		wanpipe_set_state(card, WAN_CONNECTING);
		set_chan_state(dev,WAN_CONNECTING);

		del_timer(&card->u.x.x25_timer);
		card->u.x.x25_timer.expires=jiffies+HZ;
		add_timer(&card->u.x.x25_timer);

		err=0;
	}else{
		wanpipe_set_state(card, WAN_DISCONNECTED);
		set_chan_state(dev,WAN_DISCONNECTED);
		err=-ENETDOWN;
	}

	return err;
}
/****** End *****************************************************************/
