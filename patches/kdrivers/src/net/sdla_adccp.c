/*****************************************************************************
* sdla_adccp.c	WANPIPE(tm) Multiprotocol WAN Link Driver.  X.25 module.
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
* Apr 30, 2003  Nenad Corbic	 o Inital Version based on X25
*****************************************************************************/

/*======================================================
 * 	Includes 
 *=====================================================*/


#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/wanproc.h>
#include <linux/wanpipe_snmp.h>
#include <linux/sdla_adccp.h>	/* HDLC API definitions */
#include <linux/if_wanpipe_common.h>
#include <linux/if_wanpipe.h>
#include <linux/wanpipe_x25_kernel.h>



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

#define DCD(x) (x & 0x03 ? "HIGH" : "LOW")
#define CTS(x) (x & 0x05 ? "HIGH" : "LOW")

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

	unsigned long  tq_working;

	sdla_t* card;			/* -> owner */
	netdevice_t *dev;		/* -> bound devce */

	int ch_idx;
	unsigned char enable_IPX;
	unsigned long network_number;
	struct net_device_stats ifstats;	/* interface statistics */
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

	unsigned long		disc_delay;
	x25api_t 		x25_api_event;
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


/*=================================================  
 * 	Network device interface 
 */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_send (struct sk_buff* skb, netdevice_t* dev);
static struct net_device_stats *if_stats (netdevice_t* dev);
static int if_ioctl (netdevice_t *dev, struct ifreq *ifr, int cmd);

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


/*=================================================  
 *	Background polling routines 
 */
static void wpx_poll (void*);
#if 0
static void poll_disconnected (sdla_t* card);
#endif
static void poll_connecting (sdla_t* card);
static void poll_active (sdla_t* card);
static void trigger_x25_poll(sdla_t *card);
static void x25_timer_routine(unsigned long data);



/*=================================================  
 *	X.25 firmware interface functions 
 */
static int x25_get_version (sdla_t* card, char* str);
static int hdlc_configure (sdla_t* card, TX25Config* conf);
static int x25_get_err_stats (sdla_t* card);
static int x25_get_stats (sdla_t* card);
static int x25_set_intr_mode (sdla_t* card, int mode);
static int x25_close_hdlc (sdla_t* card);
static int x25_open_hdlc (sdla_t* card);
static int x25_setup_hdlc (sdla_t* card);
static int x25_clear_call (sdla_t* card, int lcn, int cause, int diagn);
static int x25_send (sdla_t* card, int lcn, int qdm, int len, void* buf);
static int x25_fetch_events (sdla_t* card);
static int x25_error (sdla_t* card, int err, int cmd, int lcn);

/*=================================================  
 *	X.25 asynchronous event handlers 
 */
static int incoming_call (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int call_accepted (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int call_cleared (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int timeout_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);
static int restart_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb);


/*=================================================  
 *	Miscellaneous functions 
 */
static int connect (sdla_t* card);
static int disconnect (sdla_t* card);
static int chan_disc (netdevice_t* dev);
static void set_chan_state (netdevice_t* dev, int state);
static int chan_send (netdevice_t* , void* , unsigned, unsigned char);
static unsigned char bps_to_speed_code (unsigned long bps);
static void parse_call_info (unsigned char*, x25_call_info_t*);
static netdevice_t * find_channel(sdla_t *, unsigned);
static int wait_for_cmd_rc(sdla_t *card, x25_channel_t* chan);
static int test_chan_command_busy(sdla_t *card,x25_channel_t *chan);
static int delay_cmd(sdla_t *card, x25_channel_t *chan, int delay);


/*=================================================  
 *      X25 API Functions 
 */
static int wanpipe_pull_data_in_skb (sdla_t *, netdevice_t *, struct sk_buff **);
static void trigger_intr_exec(sdla_t *, unsigned char);
static int execute_delayed_cmd (sdla_t*, netdevice_t *, char);
static int alloc_and_init_skb_buf (sdla_t *,struct sk_buff **, int);
static int send_oob_msg (sdla_t *, netdevice_t *, wan_mbox_t *);
static int tx_intr_cmd_exec(sdla_t *card);
static void api_oob_event (sdla_t *card,wan_mbox_t *mbox);
static int hdlc_link_down (sdla_t*);

/*=================================================
 *     XPIPEMON Functions
 */
static int process_udp_mgmt_pkt(sdla_t *,netdevice_t*);
static int reply_udp( unsigned char *, unsigned int); 
static void init_x25_channel_struct( x25_channel_t *);
static void init_global_statistics( sdla_t *);
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

/*=================================================  
 * 	Global Variables 
 *=================================================*/
static TX25Stats	X25Stats;



/*================================================= 
 *	Public Functions 
 *=================================================*/




/*===================================================================
 * wp_adccp_init:  ADCCP Protocol Initialization routine.
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

int wp_adccp_init (sdla_t* card, wandev_conf_t* conf)
{
	union{
		char str[80];
		TX25Config cfg;
	} u;
	wan_x25_conf_t*	x25_adm_conf = &card->u.x.x25_adm_conf;
	wan_x25_conf_t*	x25_conf = &card->u.x.x25_conf;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_ADCCP){
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
			card->devname, conf->config_id)
		;
		return -EINVAL;
	}

	/* Initialize protocol-specific fields */
	/* ALEX Apr 8 2004 Sangoma ISA card */
	card->mbox_off  = X25_MB_VECTOR + X25_MBOX_OFFS;
	card->flags_off = X25_MB_VECTOR + X25_STATUS_OFFS;
	card->rxmb_off  = X25_MB_VECTOR + X25_RXMBOX_OFFS;

	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */
	if (x25_get_version(card, NULL) || x25_get_version(card, u.str))
		return -EIO;


	/* Run only in LAPB HDLC mode.
         * Check the user defined option and configure accordingly */
	printk(KERN_INFO "%s: running LAP_B HDLC firmware v%s\n",
		card->devname, u.str);
	card->u.x.LAPB_hdlc = 1;

	/* Copy Admin configuration to sdla_t structure */
	memcpy(x25_adm_conf, &conf->u.x25, sizeof(wan_x25_conf_t));

	/* Configure adapter. Here we set resonable defaults, then parse
	 * device configuration structure and set configuration options.
	 * Most configuration options are verified and corrected (if
	 * necessary) since we can't rely on the adapter to do so.
	 */
	memset(&u.cfg, 0, sizeof(u.cfg));
	u.cfg.t1		= conf->u.x25.t1;
	u.cfg.t2		= conf->u.x25.t2;
	u.cfg.t4		= conf->u.x25.t4;
	u.cfg.n2		= 10;
	u.cfg.autoHdlc		= 1;		/* automatic HDLC connection */
	u.cfg.hdlcWindow	= conf->u.x25.hdlc_window;
	u.cfg.station		= X25_STATION_DTE;		/* DTE */
	u.cfg.local_station_address = conf->u.x25.local_station_address;

	if (conf->clocking != WANOPT_EXTERNAL)
		u.cfg.baudRate = bps_to_speed_code(conf->bps);

	if (conf->u.x25.station != WANOPT_DTE){
		u.cfg.station = 0;		/* DCE mode */
	}

        if (conf->electrical_interface != WANOPT_RS232 ){
	        u.cfg.hdlcOptions |= 0x80;      /* V35 mode */
	} 

	DEBUG_EVENT("%s: Lapb Configuration: \n",
			card->devname);
	DEBUG_EVENT("%s:    T1  = %d\n",
			         card->devname,
				 u.cfg.t1);
	DEBUG_EVENT("%s:    T2  = %d\n",
			         card->devname,
				 u.cfg.t2);
	DEBUG_EVENT("%s:    T4  = %d\n",
			         card->devname,
				 u.cfg.t4);
	DEBUG_EVENT("%s:    N2  = %d\n",
			         card->devname,
				 u.cfg.n2);

	DEBUG_EVENT("%s:    Baud Rate        = %d\n",
			         card->devname,
				 u.cfg.baudRate);

	DEBUG_EVENT("%s:    Hdlc Window      = %d\n",
			         card->devname,
				 u.cfg.hdlcWindow);
	DEBUG_EVENT("%s:    Station          = %s\n",
			         card->devname,
				 u.cfg.station==X25_STATION_DTE?"DTE":"DCE");
	DEBUG_EVENT("%s:    Local State Addr = %i\n",
			         card->devname,
			         conf->u.x25.local_station_address);
	
	u.cfg.hdlcMTU = 1027;


	/* adjust MTU */
	if (!conf->mtu || (conf->mtu >= 1024))
		card->wandev.mtu = 1024;
	else if (conf->mtu >= 512)
		card->wandev.mtu = 512;
	else if (conf->mtu >= 256)
		card->wandev.mtu = 256;
	else if (conf->mtu >= 128)
		card->wandev.mtu = 128;
	else 
		card->wandev.mtu = 64;

	/* Copy the real configuration to card structure */
	x25_conf->hdlc_window 	= u.cfg.hdlcWindow;
	x25_conf->t1 		= u.cfg.t1;
	x25_conf->t2 		= u.cfg.t2;
	x25_conf->t4 		= u.cfg.t4;
	x25_conf->n2 		= u.cfg.n2;
	x25_conf->oob_on_modem 	= conf->u.x25.oob_on_modem;

	/* initialize adapter */
	if (hdlc_configure(card, &u.cfg) != CMD_OK)
		return -EIO;

	if ((x25_close_hdlc(card) != CMD_OK)){
		return -EIO;
	}
	
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

#if 0
	// Proc fs functions
	card->wandev.get_config_info 	= &x25_get_config_info;
	card->wandev.get_status_info 	= &x25_get_status_info;
	card->wandev.get_dev_config_info= &x25_get_dev_config_info;
	card->wandev.get_if_info     	= &x25_get_if_info;
	card->wandev.set_dev_config    	= &x25_set_dev_config;
	card->wandev.set_if_info     	= &x25_set_if_info;

	/* SNMP data */
	card->get_snmp_data     	= &x25_snmp_data;
#endif

	/* WARNING: This function cannot exit with an error
	 *          after the change of state */
	card->wandev.state	= WAN_DISCONNECTED;
	
	card->wandev.enable_tx_int = 0;
	card->irq_dis_if_send_count = 0;
        card->irq_dis_poll_count = 0;
	card->u.x.tx_dev = NULL;
	card->u.x.no_dev = 0;


	/* Configure for S514 PCI Card */
	/* ALEX Apr 8 2004 Sangoma ISA card */
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
	sdla_t* card;
	netdevice_t	*dev;
	unsigned long smp_flags;
	
	/* sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;

	if (wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	if (test_bit(SEND_CRIT, (void*)&wandev->critical))
		return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&wandev->dev_head));
	if (!dev)
		return -ENODEV;
	
	card = wandev->priv;

	DEBUG_EVENT("%s: UPDATE\n",card->devname);
	
	spin_lock_irqsave(&card->wandev.lock, smp_flags);

#if 0
	if (*card->u.x.hdlc_buf_status & 0x40){
		x25_get_err_stats(card);
		x25_get_stats(card);
	}
#else
	x25_get_err_stats(card);
	x25_get_stats(card);
#endif
	spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

	return 0;
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

	if(card->wandev.new_if_cnt++ > 0) {
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
	dev->priv = chan;
	
	strcpy(chan->name, conf->name);
	chan->card = card;
	chan->dev = dev;
	chan->tx_skb = chan->rx_skb = NULL;

	if(strcmp(conf->usedby, "WANPIPE") == 0){
                printk(KERN_INFO "%s: Running in WANPIPE mode %s\n",
			wandev->name, chan->name);
                chan->common.usedby = WANPIPE;
		chan->protocol = htons(ETH_P_IP);

        }else if(strcmp(conf->usedby, "API") == 0){
		chan->common.usedby = API;
                printk(KERN_INFO "%s: Running in API mode %s\n",
			wandev->name, chan->name);
		wan_reg_api(chan, dev, card->devname);
		chan->protocol = htons(WP_LAPB_PROT);
	}


	if (err){
		kfree(chan);
		dev->priv = NULL;
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
	dev->init = &if_init;

	init_x25_channel_struct(chan);

	chan->common.state = WAN_DISCONNECTED;
	set_chan_state(dev, WAN_DISCONNECTED);	

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
	sdla_t *card=wandev->priv;
	x25_channel_t* chan = dev->priv;

	set_chan_state(dev, WAN_DISCONNECTED);
	WAN_TASKLET_KILL(&chan->common.bh_task);
	wan_unreg_api(dev->priv, card->devname);
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
	disable_comm_shutdown(card);
	del_timer(&card->u.x.x25_timer);
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
	x25_channel_t* chan = dev->priv;
	sdla_t* card = chan->card;
	wan_device_t* wandev = &card->wandev;

	/* Initialize device driver entry points */
	dev->open		= &if_open;
	dev->stop		= &if_close;
	dev->hard_start_xmit	= &if_send;
	dev->get_stats		= &if_stats;
	dev->do_ioctl		= &if_ioctl;

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (chan->common.usedby != API){
		dev->tx_timeout		= &if_tx_timeout;
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
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); 
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end);

        /* Set transmit buffer queue length */
        dev->tx_queue_len = 100;

	/* FIXME Why are we doing this */
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
	x25_channel_t* chan = dev->priv;
	sdla_t* card = chan->card;
	struct timeval tv;
	unsigned long smp_flags;
	
	if (open_dev_check(dev))
		return -EBUSY;

	/* Increment the number of interfaces */
	++card->u.x.no_dev;
	
	wanpipe_open(card);

	/* LAPB protocol only uses one interface, thus
	 * start the protocol after it comes up. */
	if (card->open_cnt == 1){
		TX25Status	status;
		card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
		S508_S514_lock(card, &smp_flags);
		x25_set_intr_mode(card, INTR_ON_TIMER); 
		status.imask &= ~INTR_ON_TIMER;
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
		S508_S514_unlock(card, &smp_flags);
	}
	/* Device is not up untill the we are in connected state */
	do_gettimeofday( &tv );
	chan->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

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
	x25_channel_t* chan = dev->priv;
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
		S508_S514_unlock(card, &smp_flags);
	}

	wanpipe_close(card);

	/* If this is the last close, disconnect physical link */
	if (!card->open_cnt){
		S508_S514_lock(card, &smp_flags);
		disconnect(card);
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
    	x25_channel_t* chan = dev->priv;
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
	x25_channel_t* chan = dev->priv;
	sdla_t* card = chan->card;
	TX25Status	status;
	unsigned long smp_flags=0;
	int err=0;

	++chan->if_send_stat.if_send_entry;
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	netif_stop_queue(dev);
#endif

	/* No need to check frame length, since socket code
         * will perform the check for us */


#if defined(LINUX_2_1)
	if (dev->tbusy){
		if ((jiffies - chan->tick_counter) < (5*HZ)){
			return 1;
		}

		if_tx_timeout(dev);
	}
#endif

	chan->tick_counter = jiffies;
	
	/* Critical region starts here */
	S508_S514_lock(card, &smp_flags);
	
	if (test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)){
		printk(KERN_INFO "Hit critical in if_send()! %lx\n",card->wandev.critical);
		start_net_queue(dev);
		++chan->if_send_stat.if_send_critical_ISR;
		goto if_send_crit_exit;
	}

	if (card->wandev.state != WAN_CONNECTED){

		++chan->ifstats.tx_dropped;
		++card->wandev.stats.tx_dropped;
		++chan->if_send_stat.if_send_wan_disconnected;	
		start_net_queue(dev);

	}else if (chan->protocol && (chan->protocol != skb->protocol)){

		printk(KERN_INFO
			"%s: unsupported Ethertype 0x%04X on interface %s!\n",
			chan->name, htons(skb->protocol), dev->name);
		
		printk(KERN_INFO "PROTO %Xn", htons(chan->protocol));
		++chan->ifstats.tx_errors;
		++chan->ifstats.tx_dropped;
		++card->wandev.stats.tx_dropped;
		++chan->if_send_stat.if_send_protocol_error;
		start_net_queue(dev);
		
	}else switch (chan->common.state){

		case WAN_DISCONNECTED:

			/* Try to establish connection. If succeded, then start
			 * transmission, else drop a packet.
			 */
			++chan->ifstats.tx_dropped;
			++card->wandev.stats.tx_dropped;
			++chan->if_send_stat.if_send_wan_disconnected;	
			start_net_queue(dev);
			break;

		case WAN_CONNECTED:

			/* We never drop here, if cannot send than, copy
	                 * a packet into a transmit buffer 
                         */
			err=chan_send(dev, skb->data, skb->len, 0);
			if (err){
				err=-1;
				++chan->if_send_stat.if_send_bfr_not_passed_to_adptr;
				stop_net_queue(dev);
			}else{
				++chan->if_send_stat.if_send_bfr_passed_to_adptr;
				start_net_queue(dev);
			}
			break;

		default:
			++chan->ifstats.tx_dropped;	
			++card->wandev.stats.tx_dropped;
			++chan->if_send_stat.if_send_wan_disconnected;	
			start_net_queue(dev);
			break;
	}

if_send_crit_exit:

	if (err==0){
       		wan_dev_kfree_skb(skb, FREE_WRITE);
	}else{
		if (chan->common.state == WAN_CONNECTED){
	        	status.imask |= INTR_ON_TX_FRAME;
			card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
			++chan->if_send_stat.if_send_tx_int_enabled;
		}
	}

	clear_bit(SEND_CRIT,(void*)&card->wandev.critical);
	S508_S514_unlock(card, &smp_flags);
	return err;
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
	x25_channel_t *chan = dev->priv;

	if(chan == NULL)
		return NULL;

	return &chan->ifstats;
}


static int if_ioctl (netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	x25_channel_t *chan; 	
	sdla_t *card;
	unsigned long smp_flags;
	wan_udp_pkt_t *wan_udp_pkt;
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

	switch(cmd)
	{
		
		case SIOC_WANPIPE_BIND_SK:

			if (!ifr){
				DEBUG_EVENT("%s: BIND SK Invalid no dev \n",__FUNCTION__);
				return EINVAL;
			}
			
			DEBUG_TEST("%s: SIOC_WANPIPE_BIND_SK \n",__FUNCTION__);

			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

			return err;

		case SIOC_WANPIPE_UNBIND_SK:

			if (!ifr){
				DEBUG_EVENT("%s: UN BIND SK Invalid no dev \n",__FUNCTION__);
				err= -EINVAL;
				break;
			}

			DEBUG_TEST("%s: SIOC_WANPIPE_UNBIND_SK \n",__FUNCTION__);
			
			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);


			if (chan->common.state != WAN_DISCONNECTED){
				S508_S514_lock(card, &smp_flags);
				err=hdlc_link_down(card);
				S508_S514_unlock(card, &smp_flags);

				if (err==0){
					delay_cmd(card,chan,HZ/10);		
				}
			}

			S508_S514_lock(card, &smp_flags);
			disconnect(card);
			set_chan_state(dev,WAN_DISCONNECTED);
			S508_S514_unlock(card, &smp_flags);

			err=0;

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
	
		case SIOC_X25_PLACE_CALL:

			S508_S514_lock(card, &smp_flags);
			connect(card);
			set_chan_state(dev,WAN_CONNECTING);
			S508_S514_unlock(card, &smp_flags);
		
			if (atomic_read(&card->u.x.command_busy)){
				atomic_set(&card->u.x.command_busy,0);
			}
			
			atomic_set(&chan->common.disconnect,0);
			atomic_set(&chan->common.command,0);

			del_timer(&card->u.x.x25_timer);
			card->u.x.x25_timer.expires=jiffies+HZ;
			add_timer(&card->u.x.x25_timer);

#if 0
			err=test_chan_command_busy(card,chan);
			if (err){
				break;
			}

			atomic_set(&chan->common.command,X25_PLACE_CALL);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			del_timer(&card->u.x.x25_timer);
			card->u.x.x25_timer.expires=jiffies+1;
			add_timer(&card->u.x.x25_timer);

			err=wait_for_cmd_rc(card,chan);
			
			DEBUG_EVENT("%s:%s PLACE CALL RC %x\n",card->devname,dev->name,err);
#endif
			break;

		case SIOC_X25_CLEAR_CALL:

			err=0;
			break;

			err=test_chan_command_busy(card,chan);
			if (err){
				break;
			}

			atomic_set(&chan->common.command,X25_CLEAR_CALL);
			trigger_intr_exec(card, TMR_INT_ENABLED_CMD_EXEC);

			err=wait_for_cmd_rc(card,chan);

			DEBUG_EVENT("%s:%s CLEAR CALL RC %i\n",card->devname,dev->name,err);
			break;


		case SIOC_WANPIPE_SNMP:
		case SIOC_WANPIPE_SNMP_IFSPEED:
			return wan_snmp_data(card, dev, cmd, ifr);

		case SIOC_WANPIPE_PIPEMON:

			if (atomic_read(&card->u.x.udp_pkt_len) != 0){
				return -EBUSY;
			}
	
			atomic_set(&card->u.x.udp_pkt_len,sizeof(wan_udp_hdr_t));
		
			wan_udp_pkt=(wan_udp_pkt_t*)&card->u.x.udp_pkt_data;
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				atomic_set(&card->u.x.udp_pkt_len,0);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)){
				printk(KERN_INFO "%s:%s pipemon command busy: try again!\n",
						card->devname,dev->name);
				atomic_set(&card->u.x.udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EBUSY;
			}
			
			if (process_udp_mgmt_pkt(card,dev) <= 0 ){
				atomic_set(&card->u.x.udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EINVAL;
			}

			
			spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

			/* This area will still be critical to other
			 * PIPEMON commands due to udp_pkt_len
			 * thus we can release the irq */
			
			if (atomic_read(&card->u.x.udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				printk(KERN_INFO "(Debug): PiPemon Buf too bit on the way up! %i\n",
						atomic_read(&card->u.x.udp_pkt_len));
				atomic_set(&card->u.x.udp_pkt_len,0);
				return -EINVAL;
			}

			if (copy_to_user(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				atomic_set(&card->u.x.udp_pkt_len,0);
				return -EFAULT;
			}
		
			atomic_set(&card->u.x.udp_pkt_len,0);
			break;
			
		default:
			printk(KERN_INFO "%s: Unsupported Ioctl Call 0x%x \n",
					chan->common.dev->name,cmd);
			return -EOPNOTSUPP;
#if 0	
			DBG_PRINTK(KERN_INFO "Ioctl: Cmd 0x%x\n",atomic_read(&chan->common.command));
			
			if (chan->common.usedby == API && 
			    atomic_read(&chan->common.command)){

				if (card->u.x.LAPB_hdlc && 
				    atomic_read(&chan->common.command) == X25_PLACE_CALL){

					unsigned long smp_flags;
					
					S508_S514_lock(card, &smp_flags);
					connect(card);
					set_chan_state(dev,WAN_CONNECTING);
					S508_S514_unlock(card, &smp_flags);
				
					if (atomic_read(&card->u.x.command_busy)){
						atomic_set(&card->u.x.command_busy,0);
					}
					
					atomic_set(&chan->common.disconnect,0);
					atomic_set(&chan->common.command,0);
					del_timer(&card->u.x.x25_timer);
					card->u.x.x25_timer.expires=jiffies+HZ;
					add_timer(&card->u.x.x25_timer);
				}else{
					trigger_intr_exec(card,TMR_INT_ENABLED_CMD_EXEC);
				}
			}
			
			if ((chan->common.usedby == API) && 
			     atomic_read(&chan->common.disconnect)){
				
				unsigned long smp_flags;

				set_bit(0,&chan->common.disconnect);
			
				if (chan->common.state == WAN_CONNECTED){
					trigger_intr_exec(card,TMR_INT_ENABLED_CMD_EXEC);
				}else{
					S508_S514_lock(card, &smp_flags);
					hdlc_link_down(card);
					disconnect(card);
					set_chan_state(dev,WAN_DISCONNECTED);
					S508_S514_unlock(card, &smp_flags);
				}
			}

			break;
#endif
	}
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
	TX25Status	status;
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));

	/* Sanity check, should never happen */
	if (test_bit(0,&card->in_isr)){
		printk(KERN_INFO "%s: Critical in WPX_ISR\n",card->devname);
		status.iflags = 0;
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}
	
	card->in_isr = 1;
	++card->statistics.isr_entry;

	if (test_bit(PERI_CRIT,(void*)&card->wandev.critical)){
		printk(KERN_INFO "%s: Critical in PERI\n",card->devname);
		card->in_isr=0;
		status.iflags = 0;
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}
	
	if (test_bit(SEND_CRIT, (void*)&card->wandev.critical)){

 		printk(KERN_INFO "%s: wpx_isr: wandev.critical set to 0x%02lx, int type = 0x%02x\n", 
			card->devname, card->wandev.critical, status.iflags);
		card->in_isr = 0;
		status.iflags = 0;
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}


	/* For all interrupts set the critical flag to CRITICAL_RX_INTR.
         * If the if_send routine is called with this flag set it will set
         * the enable transmit flag to 1. (for a delayed interrupt)
         */
	switch (status.iflags){

		case RX_INTR_PENDING:		/* receive interrupt */
			++card->statistics.isr_rx;
			rx_intr(card);
			break;

		case TX_INTR_PENDING:		/* transmit interrupt */
			++card->statistics.isr_tx;
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

		default:		/* unwanted interrupt */
			++card->statistics.isr_spurious;
			spur_intr(card);
	}

	card->in_isr = 0;
	status.iflags = 0;
	card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
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
	wan_mbox_t	rxmb;
	unsigned	lcn;
	netdevice_t*	dev = NULL;
	x25_channel_t*	chan;
	struct sk_buff* skb=NULL;

	card->hw_iface.peek(card->hw, card->rxmb_off, &rxmb, sizeof(rxmb));
	lcn = rxmb.wan_x25_lcn;
	dev = find_channel(card,lcn);
	if (dev == NULL){
		/* Invalid channel, discard packet */
		DEBUG_EVENT("%s: receiving on orphaned LCN %d!\n",
				card->devname, lcn);
		return;
	}

	chan = dev->priv;
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


	/* ------------ API ----------------*/

	if (chan->common.usedby == API){
		
		if (wan_api_enqueue_skb(chan,skb) < 0){
			++chan->ifstats.rx_dropped;
			++card->wandev.stats.rx_dropped;
			++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;
			wan_dev_kfree_skb(skb, FREE_READ);
			return;
		}		

		chan->rx_skb = NULL;

		WAN_TASKLET_SCHEDULE(&chan->common.bh_task);

		return;
	}


	/* ------------- WANPIPE -------------------*/
	
	/* set rx_skb to NULL so we won't access it later when kernel already owns it */
	chan->rx_skb=NULL;
	
	/* Decapsulate packet, if necessary */
	if (!skb->protocol && !wanrouter_type_trans(skb, dev)){
		/* can't decapsulate packet */
                wan_dev_kfree_skb(skb, FREE_READ);
		++chan->ifstats.rx_errors;
		++chan->ifstats.rx_dropped;
		++card->wandev.stats.rx_dropped;
		++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;

	}else{
		if( handle_IPXWAN(skb->data, chan->name, 
				  chan->enable_IPX, chan->network_number, 
				  skb->protocol)){

			if( chan->enable_IPX ){
				if(chan_send(dev, skb->data, skb->len,0)){
					chan->tx_skb = skb;
				}else{
                                        wan_dev_kfree_skb(skb, FREE_WRITE);
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
	void*		bufptr;
	wan_mbox_t	rxmb;
	unsigned	len;
	x25_channel_t*	chan = dev->priv;
	struct sk_buff*	new_skb = *skb;

	card->hw_iface.peek(card->hw, card->rxmb_off, &rxmb, sizeof(rxmb));
	len = rxmb.wan_data_len;
	/* Add on the API header to the received
         * data 
	 */
	len += sizeof(x25api_hdr_t);

	/* Allocate new socket buffer */
	new_skb = dev_alloc_skb(len + 15);
	if (new_skb == NULL){
		printk(KERN_INFO "%s: no socket buffers available!\n",
			card->devname);
		chan->drop_sequence = 1;	/* set flag */
		++chan->ifstats.rx_dropped;
		return 1;
	}

	bufptr = skb_put(new_skb,len);

	if (chan->common.usedby == API){
		/* Fill in the x25api header 
		 */
		x25api_t * api_data = (x25api_t*)bufptr;
		api_data->hdr.qdm = rxmb.wan_x25_qdm;
		api_data->hdr.cause = rxmb.wan_x25_cause;
		api_data->hdr.diagn = rxmb.wan_x25_diagn;
		api_data->hdr.length = rxmb.wan_data_len;
		api_data->hdr.lcn  = rxmb.wan_x25_lcn;
		memcpy(api_data->data, rxmb.wan_data, rxmb.wan_data_len);
	}else{
		memcpy(bufptr, rxmb.wan_data, len);
	}

	new_skb->dev = dev;

	if (chan->common.usedby == API){
		wan_skb_reset_mac_header(new_skb);
		new_skb->protocol = htons(X25_PROT);
		new_skb->pkt_type = WAN_PACKET_DATA;
	}else{
		new_skb->protocol = chan->protocol;
	}

	*skb = new_skb; 

	return 0;
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
	TX25Status	status;
	x25_channel_t *chan=NULL;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	

#if 1
	/* Try sending an async packet first */
	if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_CMD_EXEC){
		if (tx_intr_cmd_exec(card) == 0){
			card->u.x.timer_int_enabled &=
					~TMR_INT_ENABLED_CMD_EXEC;
		}else{
			/* There are more commands pending
			 * re-trigger tx interrupt */
			DEBUG_TX("Re Triggerint Cmd\n");
			++card->statistics.isr_enable_tx_int;
			return;
		}
	}
#endif

	chan = dev->priv;


	/* Device was set to transmit, check if the TX
	 * buffers are available 
	 */		
		
	if (is_queue_stopped(dev)){
		if (chan->common.usedby == API){
			start_net_queue(dev);
			wan_wakeup_api(chan);
		}else{
			wake_net_dev(dev);
		}
	}

	/* if any other interfaces have transmit interrupts pending, */
	/* do not disable the global transmit interrupt */
	status.imask &= ~INTR_ON_TX_FRAME;
	card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
	return;
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
	TX25Status	status;
	
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	if(card->u.x.timer_int_enabled & TMR_INT_ENABLED_UDP_PKT) {

		if (card->u.x.udp_type == UDP_XPIPE_TYPE){
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
		chan = dev->priv;

		printk(KERN_INFO 
			"%s: Closing down Idle link %s on LCN %d\n",
					card->devname,chan->name,chan->common.lcn); 
		chan->i_timeout_sofar = jiffies;
		chan_disc(dev);	
         	card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_ACTIVE;
		card->u.x.poll_device=NULL;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_CONNECT_ON) {

		netdevice_t *dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		wanpipe_set_state(card, WAN_CONNECTED);
		set_chan_state(dev,WAN_CONNECTED);

		/* 0x8F enable all interrupts */
		x25_set_intr_mode(card, INTR_ON_RX_FRAME|	
					INTR_ON_TX_FRAME|
					INTR_ON_MODEM_STATUS_CHANGE|
					X25_ASY_TRANS_INTR_PENDING |
					INTR_ON_TIMER |
					DIRECT_RX_INTR_USAGE
				); 

		status.imask &= ~INTR_ON_TX_FRAME;	/* mask Tx interrupts */
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_CONNECT_ON;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_CONNECT_OFF) {
		DEBUG_TX("%s: Poll connect, Turning OFF\n",card->devname);
		disconnect(card);
		if (card->u.x.LAPB_hdlc){
			netdevice_t *dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			set_chan_state(dev,WAN_DISCONNECTED);
			send_oob_msg(card,dev,&card->wan_mbox);
		}
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_CONNECT_OFF;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_POLL_DISCONNECT) {

		DBG_PRINTK(KERN_INFO "POll disconnect, trying to connect\n");
		connect(card);
		if (card->u.x.LAPB_hdlc){
			netdevice_t *dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
			set_chan_state(dev,WAN_CONNECTED);
		}
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_POLL_DISCONNECT;

	}else if (card->u.x.timer_int_enabled & TMR_INT_ENABLED_UPDATE){

#if 0
		if (*card->u.x.hdlc_buf_status & 0x40){
			x25_get_err_stats(card);
			x25_get_stats(card);
			card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_UPDATE;
		}
#else
		x25_get_err_stats(card);
		x25_get_stats(card);
		card->u.x.timer_int_enabled &= ~TMR_INT_ENABLED_UPDATE;
#endif
	}

	if(!(card->u.x.timer_int_enabled & ~TMR_INT_ENABLED_CMD_EXEC)){
		//printk(KERN_INFO "Turning Timer Off \n");
		status.imask &= ~INTR_ON_TIMER;
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
	}
}

/*====================================================================
 * 	Modem status interrupt handler.
 *===================================================================*/
static void status_intr (sdla_t* card)
{

	/* Added to avoid Modem status message flooding */
	static TX25ModemStatus	last_stat;
	wan_mbox_t*		mbox = &card->wan_mbox;
	TX25ModemStatus*	modem_status;
	netdevice_t*		dev;
	x25_channel_t*		chan;
	int			err;

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
				card->devname,DCD(modem_status->status),CTS(modem_status->status));

			last_stat.status = modem_status->status;
		
			if (card->u.x.oob_on_modem){
				struct wan_dev_le	*devle;

				DBG_PRINTK(KERN_INFO "Modem status oob msg!\n");

				mbox->wan_x25_pktType = mbox->wan_command;
				mbox->wan_return_code = 0x08;

				/* Send a OOB to all connected sockets */
				WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
					dev = WAN_DEVLE2DEV(devle);
					if (!dev || !wan_netif_priv(dev))
						continue;
					chan = wan_netif_priv(dev);
					if (chan->common.usedby == API){
						printk(KERN_INFO "(Debug): Send oob msg 5\n");

						send_oob_msg(card,dev,mbox);				
					}
				}

				/* The modem OOB message will probably kill the
				 * the link. If we don't clear the flag here,
				 * a deadlock could occur */ 
				if (atomic_read(&card->u.x.command_busy)){
					atomic_set(&card->u.x.command_busy,0);
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

static void wpx_poll (void *card_ptr)
{
	netdevice_t	*dev;
	sdla_t *card=card_ptr;
	++card->statistics.poll_processed;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){
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
#if 0
			poll_disconnected(card);
#endif
			break;
	}

wpx_poll_exit:
	clear_bit(POLL_CRIT,&card->wandev.critical);
	return;
}

static void trigger_x25_poll(sdla_t *card)
{
	wan_schedule_task(&card->u.x.x25_poll_task);
}

/*====================================================================
 * 	Handle physical link establishment phase.
 * 	o if connection timed out, disconnect the link.
 *===================================================================*/

static void poll_connecting (sdla_t* card)
{
	TX25Status	status;
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));

	if (status.ghdlc_status & X25_HDLC_ABM){

		trigger_intr_exec (card, TMR_INT_ENABLED_POLL_CONNECT_ON);

	}else if ((jiffies - card->state_tick) > CONNECT_TIMEOUT){

		trigger_intr_exec (card, TMR_INT_ENABLED_POLL_CONNECT_OFF);

	}
}

#if 0
/*====================================================================
 * 	Handle physical link disconnected phase.
 * 	o if hold-down timeout has expired and there are open interfaces, 
 *	connect link.
 *===================================================================*/

static void poll_disconnected (sdla_t* card)
{
	netdevice_t *dev; 
	x25_channel_t *chan;

	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL)
		return;

	if ((chan=dev->priv) == NULL)
		return;

	if (card->open_cnt && (jiffies - card->state_tick) > HOLD_DOWN_TIME){
		trigger_intr_exec(card, TMR_INT_ENABLED_POLL_DISCONNECT);
	}
}
#endif

/*====================================================================
 * 	Handle active link phase.
 * 	o fetch X.25 asynchronous events.
 * 	o kick off transmission on all interfaces.
 *===================================================================*/

static void poll_active (sdla_t* card)
{
	TX25Status	status;
	netdevice_t *dev;
	x25_channel_t *chan;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	chan = wan_netif_priv(dev);
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	if (!(status.ghdlc_status & X25_HDLC_ABM)){

		if (jiffies-chan->disc_delay < HZ){
			return;
		}
		
		trigger_intr_exec (card, TMR_INT_ENABLED_POLL_CONNECT_OFF);
	}
}

static void trigger_intr_exec(sdla_t *card, unsigned char TYPE)
{
	TX25Status	status;
	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	
	card->u.x.timer_int_enabled |= TYPE;
	if (TYPE == TMR_INT_ENABLED_CMD_EXEC){
		status.imask |= INTR_ON_TX_FRAME;
		card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));
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
		mbox->wan_data[1] = card->wandev.irq;
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
#if 0
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

	return err;
}
#endif
/*====================================================================
 * 	Accept X.25 call.
 *====================================================================*/
#if 0
static int x25_accept_call (sdla_t* card, int lcn, int qdm)
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
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	} while (err && x25_error(card, err, X25_ACCEPT_CALL, lcn) && retry--);
	
	return err;
}
#endif
/*====================================================================
=======
 * 	Clear X.25 call.
 *====================================================================*/

static int x25_clear_call (sdla_t* card, int lcn, int cause, int diagn)
{
	wan_mbox_t* mbox = &card->wan_mbox;
  	int retry = MAX_CMD_RETRY;
	int err;

	do
	{
		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_x25_lcn     = lcn;
		mbox->wan_x25_cause   = cause;
		mbox->wan_x25_diagn   = diagn;
		mbox->wan_command = X25_CLEAR_CALL;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	} while (err && x25_error(card, err, X25_CLEAR_CALL, lcn) && retry--);

	return err;
}

/*====================================================================
 * 	Send X.25 data packet.
 *====================================================================*/

static int x25_send (sdla_t* card, int lcn, int qdm, int len, void* buf)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int err;
	unsigned char cmd;
		
	cmd = X25_HDLC_WRITE;

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

	if (err){
		x25_error(card, err, cmd , lcn);
	}
	
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
	TX25Status status;
	wan_mbox_t* mbox = &card->wan_mbox;
	int err = 0;

	card->hw_iface.peek(card->hw, card->flags_off, &status, sizeof(status));
	if (status.hdlc_status & 0x20)
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
	netdevice_t	*dev;
	int retry = 1;
	/* FIXME: CRITICAL NO MAILBOX */
//	unsigned dlen = ((wan_mbox_t*)card->mbox)->wan_data_len;
	unsigned dlen = ((wan_mbox_t*)&card->wan_mbox)->wan_data_len;
	wan_mbox_t* mb;

	mb = kmalloc(sizeof(wan_mbox_t) + dlen, GFP_ATOMIC);
	if (mb == NULL)
	{
		printk(KERN_INFO "%s: x25_error() out of memory!\n",
			card->devname);
		return 0;
	}
	memcpy(mb, &card->wan_mbox, sizeof(wan_mbox_t) + dlen);
	
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

		default:
			printk(KERN_INFO "%s: X.25 event 0x%02X on LCN %d! "
				"Cause:0x%02X Diagn:0x%02X\n",
				card->devname, mb->wan_x25_pktType,
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
			printk(KERN_INFO "%s: X.25 reset request on LCN %d! "
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
			mb->wan_x25_lcn, mb->wan_x25_pktType, mb->wan_x25_diagn);
		break;

	case 0x08:	/* modem failure */
#ifndef MODEM_NOT_LOG
		printk(KERN_INFO "%s: modem failure!\n", card->devname);
#endif
		api_oob_event(card,mb);
		break;

	case 0x09:	/* N2 retry limit */
		printk(KERN_INFO "%s: exceeded HDLC retry limit!\n",
			card->devname);
		api_oob_event(card,mb);
		break;

	case 0x06:	/* unnumbered frame was received while in ABM */
		printk(KERN_INFO "%s: received Unnumbered frame 0x%02X!\n",
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

	case X25RES_LINK_NOT_IN_ABM:
		retry = 0;	
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head)); 
		set_chan_state(dev, WAN_DISCONNECTED);
		break;

	case 0x01:
		if (card->u.x.LAPB_hdlc)
			break;

		if (mb->wan_command == 0x16)
			break;
		/* I use the goto statement here so if 
                 * somebody inserts code between the
                 * case and default, we will not have
                 * ghost problems */
		goto dflt_2;

	default:
dflt_2:
		printk(KERN_INFO "%s: command 0x%02X returned 0x%02X! Lcn %i\n",
			card->devname, cmd, err, mb->wan_x25_lcn)
		;
		retry = 0;	/* abort command */
	}
	kfree(mb);
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
	DEBUG_EVENT("%s: Unsupported incoming call!\n",card->devname);
	
	return 1;
}


/*====================================================================
 * 	Handle accepted call.
 *====================================================================*/

static int call_accepted (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	DEBUG_EVENT("%s: Unsupported call accepted!\n",card->devname);
	return 1;
}

/*====================================================================
 * 	Handle cleared call.
 *====================================================================*/

static int call_cleared (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	DEBUG_EVENT("%s: Unsupported call accepted!\n",card->devname);
	return 1;
}

/*====================================================================
 * 	Handle X.25 restart event.
 *====================================================================*/

static int restart_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	struct wan_dev_le	*devle;
	netdevice_t* dev;
	x25_channel_t *chan;
	unsigned char old_state;

	printk(KERN_INFO
		"%s: X.25 restart request! Cause:0x%02X Diagn:0x%02X\n",
		card->devname, mb->wan_x25_cause, mb->wan_x25_diagn);

	/* down all logical channels */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
		chan = wan_netif_priv(dev);
		old_state = chan->common.state;

		if (chan->common.usedby == API){
			send_oob_msg(card,dev,mb);				
		}

		set_chan_state(dev, WAN_DISCONNECTED);
	}

	if (card->sk){
		wanpipe_api_listen_rx(NULL,card->sk);
		sock_put(card->sk);
		card->sk=NULL;
	}
	
	return (cmd == X25_WRITE) ? 0 : 1;
}

/*====================================================================
 * Handle timeout event.
 *====================================================================*/

static int timeout_event (sdla_t* card, int cmd, int lcn, wan_mbox_t* mb)
{
	unsigned new_lcn = mb->wan_x25_lcn;

	if (mb->wan_x25_pktType == 0x05)	/* call request time out */
	{
		netdevice_t* dev = find_channel(card,new_lcn);

		printk(KERN_INFO "%s: X.25 call timed timeout on LCN %d!\n",
			card->devname, new_lcn);

		if (dev){
			set_chan_state(dev, WAN_DISCONNECTED);
		}
	}else{ 
		printk(KERN_INFO "%s: X.25 packet 0x%02X timeout on LCN %d!\n",
		card->devname, mb->wan_x25_pktType, new_lcn);
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
	TX25Status status;

	if (x25_open_hdlc(card) || x25_setup_hdlc(card)){
		DEBUG_EVENT("Connect: FAILED\n");
		return -EIO;
	}

	wanpipe_set_state(card, WAN_CONNECTING);

	x25_set_intr_mode(card, INTR_ON_TIMER); 
	status.imask &= ~INTR_ON_TIMER;
	card->hw_iface.poke(card->hw, card->flags_off, &status, sizeof(status));

	return 1;
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
	x25_set_intr_mode(card, INTR_ON_TIMER);	/* disable all interrupt except timer */
	x25_close_hdlc(card);			/* close HDLC link */
	//x25_set_dtr(card, 0);			/* drop DTR */
	return 0;
}

/*
 * 	Disconnect logical channel.
 * 	o if SVC then clear X.25 call
 */

static int chan_disc (netdevice_t* dev)
{
	x25_channel_t* chan = dev->priv;

	if (chan->common.svc){ 
		x25_clear_call(chan->card, chan->common.lcn, 0, 0);

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
	x25_channel_t* chan = dev->priv;
	sdla_t* card = chan->card;
	
	if (chan->common.state != state)
	{
		switch (state)
		{
			case WAN_CONNECTED:
				if (card->u.x.logging){
					printk (KERN_INFO 
						"%s: interface %s connected !\n", 
						card->devname, dev->name);
				}
				*(unsigned short*)dev->dev_addr = htons(chan->common.lcn);
				chan->i_timeout_sofar = jiffies;

				/* LAPB is PVC Based */
				if (card->u.x.LAPB_hdlc){
					chan->common.svc=0;
				}

				if (is_queue_stopped(dev)){
					if (chan->common.usedby == API){
						start_net_queue(dev);
						wan_wakeup_api(chan);
					}else{
						wake_net_dev(dev);
					}
				}
				
				break;

			case WAN_CONNECTING:
				if (card->u.x.logging){
					printk (KERN_INFO 
						"%s: interface %s connecting ...\n", 
						card->devname, dev->name);
				}
				break;

			case WAN_DISCONNECTED:
				if (card->u.x.logging){
					printk (KERN_INFO 
						"%s: interface %s disconnected!\n", 
						card->devname, dev->name);
				}
				
				atomic_set(&chan->common.disconnect,0);

				atomic_set(&chan->common.command,0);
				break;

			case WAN_DISCONNECTING:
				if (card->u.x.logging){
					printk(KERN_INFO "\n");
					printk (KERN_INFO 
					"%s: interface %s disconnecting ...\n", 
					card->devname, dev->name);
				}
				atomic_set(&chan->common.disconnect,0);
				break;
		}
		chan->common.state = state;

		if (chan->common.usedby == API){
			wan_update_api_state(chan);
		}

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
	x25_channel_t* chan = dev->priv;
	sdla_t* card = chan->card;
	unsigned len=0, qdm=0, res=0, orig_len = 0;
	void *data;
	int err;
	u8	status;

	/* Check to see if channel is ready */
#if 1
	card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &status, 1); 
	if (!(status & 0x80)){ 
		return 1;
	}
#endif

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

	/* Pass only first three bits of the qdm byte to the send
         * routine. In case user sets any other bit which might
         * cause errors. 
         */

	err=x25_send(card, chan->common.lcn, (qdm&0x07), len, data);
	switch(err){
		case 0x00:	/* success */
			chan->i_timeout_sofar = jiffies;

#if defined(LINUX_2_4)||defined(LINUX_2_6)
			dev->trans_start=jiffies;
#endif
			
			++chan->ifstats.tx_packets;
			chan->ifstats.tx_bytes += len;
			res=0;
			break;

		case 0x33:	/* Tx busy */
			DEBUG_TX("%s: Send: Big Error should have tx: storring %s\n",
					card->devname,dev->name);
			res=1;
			break;

		default:	/* failure */
			++chan->ifstats.tx_errors;
			DEBUG_EVENT("%s: Send: Failure to send 0x%X, retry later...\n",
				card->devname,err);			
			res=1;
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
	return WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
}


/*===============================================================
 * x25api_bh 
 *
 *
 *==============================================================*/

static void x25api_bh (unsigned long data)
{
	x25_channel_t* chan = (x25_channel_t*)data;
	struct sk_buff *skb;
	int len;

	while ((skb=wan_api_dequeue_skb(chan))){

		len=skb->len;
		if (chan->common.usedby == API){
			if (wan_api_rx(chan,skb) == 0){
				++chan->rx_intr_stat.rx_intr_bfr_passed_to_stack;
				++chan->ifstats.rx_packets;
				chan->ifstats.rx_bytes += len;
			}else{
				++chan->ifstats.rx_dropped;
				++chan->rx_intr_stat.rx_intr_bfr_not_passed_to_stack;
				wan_skb_free(skb);
			}
		}
	}	

	WAN_TASKLET_END((&chan->common.bh_task));

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
	volatile x25_channel_t *chan=NULL;
	u8	status;
	int bad_cmd=0,err=0;	
	// FIXME: CRITICAL NO MAILBOX
	wan_mbox_t *mbox=&card->wan_mbox;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){
		return 0;
	}	

	chan = dev->priv;
	if (!chan){
		return 0;
	}

	card->hw_iface.peek(card->hw, card->u.x.hdlc_buf_status_off, &status, 1); 
	if (!(status & 0x80)){ 
		DEBUG_EVENT("%s: Tx intr hdlc buf status 0x%x: BUSY\n",
					card->devname, status);
		return 1;
	}

	DEBUG_EVENT("TX INTR Cmd %x\n",atomic_read(&chan->common.command));

	if (atomic_read(&chan->common.command)){

		err = execute_delayed_cmd(card, dev,
					 bad_cmd);

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
				break;

			case -EAGAIN:

				/* If command could not be executed for
                                 * some reason (i.e return code 0x33 busy)
                                 * set the more_to_exec bit which will
                                 * indicate that this command must be exectued
                                 * again during next timer interrupt 
				 */
				chan->cmd_timeout=jiffies;
				return 1;
		}
	}

	return 0;
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
	int err;
	x25_channel_t *chan=dev->priv;

	/* If channel is pvc, instead of place call
         * run x25_channel configuration. If running LAPB HDLC
         * enable communications. 
         */
	
	if (atomic_read(&chan->common.command) == X25_PLACE_CALL){
		connect(card);
		set_chan_state(dev,WAN_CONNECTING);
		return 0;
	}else{
		TX25LinkStatus *status;
		TX25Status flags;

		memset(&mbox->wan_command, 0, sizeof(TX25Cmd));
		mbox->wan_command = X25_HDLC_LINK_STATUS;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
		if (err){ 
			x25_error(card, err, X25_HDLC_LINK_STATUS, 0);
		}
		
		status = (TX25LinkStatus *)mbox->wan_data;

		if (status->txQueued){
			DEBUG_TX("LAPB: Tx Queued busy! %d\n",
					status->txQueued);
			return -EAGAIN; 
		}

		card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
		if (flags.ghdlc_status & X25_HDLC_ABM){
			DEBUG_TX("LAPB: HDLC Down ! TxQ=%d HDLC_Status=%d\n",
					status->txQueued,
					flags->ghdlc_status);
			hdlc_link_down(card);
			chan->disc_delay = jiffies;
		}

		DEBUG_EVENT("%s: Link Disconnecting ...\n",card->devname);

		return 0;
	}
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
	x25_channel_t *chan=dev->priv;
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
	x25_api_hdr->length  = len;
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

	if (chan->common.sk){
		err=wanpipe_api_sock_rx(skb,chan->common.dev,chan->common.sk);
		if (err != 0 ){
			if (err == -EINVAL){
				printk(KERN_INFO "%s:Major Error in Socket Above: OOB!!!\n",
								chan->common.dev->name);
				__sock_put(chan->common.sk);
				clear_bit(LCN_SK_ID,&chan->common.used);
				chan->common.sk = NULL;
			}
			wan_skb_free(skb);
			
			err=0;
		}
	}else{
		printk(KERN_INFO "%s: Error: No dev for OOB\n",chan->common.dev->name);
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
                wan_dev_kfree_skb(new_skb, FREE_READ);
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

	chan=dev->priv;

	if (chan->common.usedby == API){
		send_oob_msg(card,dev,mbox);
	}
}





static int hdlc_link_down (sdla_t *card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int retry = 5;
	int err=0;

	do {
		memset(mbox,0,sizeof(wan_mbox_t));
		mbox->wan_command = X25_HDLC_LINK_DISC;
		mbox->wan_data_len = 1;
		mbox->wan_x25_pf = 1;
		mbox->wan_data[0]=0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	} while (err  && x25_error(card, err, X25_HDLC_LINK_DISC, 0) && retry--);

	if (err){
		DEBUG_EVENT("%s: Hdlc Link Down Failed 0x%x\n",
				card->devname,err);
	}

	return err;
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
	wan_mbox_t       *mbox = &card->wan_mbox;
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

	chan = dev->priv;
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
			++chan->pipe_mgmt_stat.UDP_PIPE_mgmt_adptr_type_err;
			udp_mgmt_req_valid = 0;
			break;
		default:
			break;
       	}

	if(!udp_mgmt_req_valid) {
           	/* set length to 0 */
		wan_udp_pkt->wan_udp_data_len = 0;
		/* set return code */
		wan_udp_pkt->wan_udp_return_code = 0xCD; 
		
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
		   	wan_udp_pkt->wan_udp_data[0] = WANCONFIG_X25;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mbox->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mbox->wan_data_len  = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;	
	
		default :

			switch (wan_udp_pkt->wan_udp_command){
			case X25_READ_CONFIGURATION:
				wan_udp_pkt->wan_udp_command=X25_HDLC_READ_CONFIG;
				break;
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
	netdevice_t	*dev;

	DEBUG_TEST("%s: X25 TIMER ROUTINE \n",card->devname);

	++card->statistics.poll_entry;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){
		printk(KERN_INFO "%s: Stopping the X25 Poll Timer: No Dev.\n",
				card->devname);
		return;
	}

	if (test_bit(PERI_CRIT,&card->wandev.critical)){
		printk(KERN_INFO "%s: Stopping the X25 Poll Timer: Shutting down.\n",
				card->devname);
		return;
	}

	if (!test_and_set_bit(POLL_CRIT,&card->wandev.critical)){
		trigger_x25_poll(card);
	}

	if ((card->u.x.timer_int_enabled & ~TMR_INT_ENABLED_CMD_EXEC)){
		unsigned long smp_flags;
		
		spin_lock_irqsave(&card->wandev.lock, smp_flags);
		timer_intr (card);
		spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

		if ((card->u.x.timer_int_enabled & ~TMR_INT_ENABLED_CMD_EXEC)){
			del_timer(&card->u.x.x25_timer);
			card->u.x.x25_timer.expires=jiffies+(HZ/50);
			add_timer(&card->u.x.x25_timer);
			return;
		}
	}

	del_timer(&card->u.x.x25_timer);
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
	mbox->wan_data[1] = card->wandev.irq;	//ALEX_TODAY card->hw.irq;
	mbox->wan_data[2] = 2;
	mbox->wan_data_len = 3;
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

	if (dev == NULL || dev->priv == NULL)
		return;
	chan=dev->priv;
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

#if 0
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

static int x25_get_config_info(void* priv, char* buf, int cnt, int len, int offs, int* stop_cnt) 
{
	x25_channel_t*	chan = (x25_channel_t*)priv;
	sdla_t*		card = chan->card;
	int 		size = 0;

	if (chan == NULL)
		return cnt;
	if ((offs == 0 && cnt == 0) || (offs && offs == *stop_cnt)){
		PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size, "%s", x25_config_hdr));
	}

	PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size,
			    PROC_CFG_FRM, chan->name, card->devname, chan->ch_idx,
			    chan->x25_src_addr, chan->accept_dest_addr, chan->accept_usr_data));
	return cnt;
}

static int x25_get_status_info(void* priv, char* buf, int cnt, int len, int offs, int* stop_cnt)
{
	x25_channel_t*	chan = (x25_channel_t*)priv;
	sdla_t*		card = chan->card;
	int 		size = 0;

	if (chan == NULL)
		return cnt;
	if ((offs == 0 && cnt == 0) || (offs && offs == *stop_cnt)){
		PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size, "%s", x25_status_hdr));
	}

	PROC_ADD_LINE(cnt, (buf, &cnt, len, offs, stop_cnt, &size,
			    PROC_STAT_FRM, chan->name, card->devname, 
			    chan->ch_idx, STATE_DECODE(chan->common.state)));
	return cnt;
}

#define PROC_DEV_FR_SS_FRM	"%-20s| %-12s|%-20s| %-12s|\n"
#define PROC_DEV_FR_SD_FRM	"%-20s| %-12s|%-20s| %-12d|\n"
#define PROC_DEV_FR_DD_FRM	"%-20s| %-12d|%-20s| %-12d|\n"
#define PROC_DEV_FR_XD_FRM	"%-20s| 0x%-10X|%-20s| %-12d|\n"
#define PROC_DEV_SEPARATE	"==================================="
#define PROC_DEV_TITLE		" Parameters         | Value       |"

#if defined(LINUX_2_4)||defined(LINUX_2_6)
static int x25_get_dev_config_info(char* buf, char** start, off_t offs, int len)
#else
static int x25_get_dev_config_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
{
	int 		cnt = 0;
	wan_device_t*	wandev = (void*)start;
	sdla_t*		card = NULL;
	wan_x25_conf_t*	x25_conf = NULL;
	int 		size = 0;
	PROC_ADD_DECL(stop_cnt);

	if (wandev == NULL || wandev->priv == NULL)
		return cnt;

	PROC_ADD_INIT(offs, stop_cnt);
	card = (sdla_t*)wandev->priv;
	x25_conf = &card->u.x.x25_conf;

	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE PROC_DEV_SEPARATE "\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, "Configuration for %s device\n", wandev->name));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE PROC_DEV_SEPARATE "\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_TITLE PROC_DEV_TITLE "\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE PROC_DEV_SEPARATE "\n"));

	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_SS_FRM,
		 "Interface", INT_DECODE(wandev->interface),
		 "CALL_SETUP_LOG", CFG_DECODE(x25_conf->logging)));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_SS_FRM,
		 "Clocking", CLK_DECODE(wandev->clocking),
		 "OOB_ON_MODEM", CFG_DECODE(x25_conf->oob_on_modem)));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "BaudRate",wandev->bps,
		 "T1", x25_conf->t1));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "MTU", wandev->mtu,
		 "T2", x25_conf->t2));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "TTL", wandev->ttl,
		 "T4", x25_conf->t4));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "UDP Port",  wandev->udp_port,
		 "N2", x25_conf->n2));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_SD_FRM,
		 "Station", X25_STATION_DECODE(wandev->station),
		 "T10_T20", x25_conf->t10_t20));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "LowestPVC", x25_conf->lo_pvc,
		 "T11_T21", x25_conf->t11_t21));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "HighestPVC", x25_conf->hi_pvc,
		 "T12_T22", x25_conf->t12_t22));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "LowestSVC", x25_conf->lo_svc,
		 "T13_T23", x25_conf->t13_t23));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "HighestSVC", x25_conf->hi_svc,
		 "T16_T26", x25_conf->t16_t26));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "HdlcWindow", x25_conf->hdlc_window,
		 "T28", x25_conf->t28));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "PacketWindow", x25_conf->pkt_window,
		 "R10_R20", x25_conf->r10_r20));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_DD_FRM,
		 "CCITTCompat", x25_conf->ccitt_compat,
		 "R12_R22", x25_conf->r12_r22));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_FR_XD_FRM,
		 "X25Config", x25_conf->x25_conf_opt,
		 "R13_R23", x25_conf->r13_r23));
	
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_DEV_SEPARATE PROC_DEV_SEPARATE "\n"));

	PROC_ADD_RET(cnt, offs, stop_cnt);	

}

static int x25_set_dev_config(struct file *file, 
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
	
	if (dev == NULL || dev->priv == NULL)
		return -EFAULT;
	chan = (x25_channel_t*)dev->priv;
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

#if defined(LINUX_2_4)||defined(LINUX_2_6)
static int x25_get_if_info(char* buf, char** start, off_t offs, int len)
#else
static int x25_get_if_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
{
	int cnt = 0;
	netdevice_t*	dev = (void*)start;
	x25_channel_t* 	chan = (x25_channel_t*)dev->priv;
	sdla_t*		card = chan->card;
	int 		size = 0;
	PROC_ADD_DECL(stop_cnt);

	PROC_ADD_INIT(offs, stop_cnt);
	/* Update device statistics (only at the first call) */
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
	
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, 
		 "********** X.25 Administration Table *********\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
   		 "x25AdmnInterfaceMode",
		 (card->wandev.station == WANOPT_DTE) ? SNMP_X25_DTE:
		 (card->wandev.station == WANOPT_DCE) ? SNMP_X25_DCE:SNMP_X25_DXE));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
	         "x25AdmnMaxActiveCircuits", card->u.x.num_of_ch));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
        	 "x25AdmnPacketSequencing", SNMP_X25_MODULO8));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
   		 "x25AdmnRestartTimer", card->u.x.x25_adm_conf.t10_t20));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnCallTimer", card->u.x.x25_adm_conf.t11_t21));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnResetTimer", card->u.x.x25_adm_conf.t12_t22));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnClearTimer", card->u.x.x25_adm_conf.t13_t23));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
 		 "x25AdmnWindowTimer", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnDataRxmtTimer", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnInterruptTimer", card->u.x.x25_adm_conf.t16_t26));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnRejectTimer", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnRegistrationRequestTimer", card->u.x.x25_adm_conf.t28));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnMinimumRecallTimer", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnRestartCount", card->u.x.x25_adm_conf.r10_r20));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnResetCount", card->u.x.x25_adm_conf.r12_r22));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnClearCount", card->u.x.x25_adm_conf.r13_r23));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnDataRxmtCount", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnRejectCount", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnRegistrationRequestCount", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnNumberPVCs", card->u.x.x25_adm_conf.hi_pvc));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25AdmnDefCallParamId", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
		 "x25AdmnLocalAddress", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
		 "x25AdmnProtocolVersionSupported", "N/A"));
	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt,&size,  
		 "********** X.25 Operational Table *********\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperInterfaceMode",
		 (card->wandev.station == WANOPT_DTE) ? SNMP_X25_DTE:
		 (card->wandev.station == WANOPT_DCE) ? SNMP_X25_DCE:SNMP_X25_DXE));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperMaxActiveCircuits", card->u.x.num_of_ch));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperPacketSequencing", SNMP_X25_MODULO8));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperRestartTimer", card->u.x.x25_conf.t10_t20));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperCallTimer", card->u.x.x25_conf.t11_t21));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperResetTimer", card->u.x.x25_conf.t12_t22));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperClearTimer", card->u.x.x25_conf.t13_t23));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperWindowTimer", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperDataRxmtTimer", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperInterruptTimer", card->u.x.x25_conf.t16_t26));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25OperRejectTimer", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperRegistrationRequestTimer", card->u.x.x25_conf.t28));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperMinimumRecallTimer", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperRestartCount",	card->u.x.x25_conf.r10_r20));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperResetCount", card->u.x.x25_conf.r12_r22));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperClearCount", card->u.x.x25_conf.r13_r23));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperDataRxmtCount", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperRejectCount", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperRegistrationRequestCount", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperNumberPVCs", card->u.x.x25_conf.hi_pvc));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25OperDefCallParamId", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25OperLocalAddress", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25OperDataLinkId", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25OperProtocolVersionSupported", "N/A"));

	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, 
		 "********** X.25 Statistics Table *********\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
        	 "x25StatInCalls", X25Stats.rxCallRequest));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInCallRefusals", X25Stats.txClearRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInProviderInitiatedClears",
		(card->wandev.station == WANOPT_DTE) ? 
					X25Stats.rxClearRqst:
					X25Stats.txClearRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInRemotelyInitiatedResets",
		(card->wandev.station == WANOPT_DTE) ? 
					X25Stats.txResetRqst:
					X25Stats.rxResetRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInProviderInitiatedResets",
		(card->wandev.station == WANOPT_DTE) ? 
					X25Stats.rxResetRqst:
					X25Stats.txResetRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInRestarts", X25Stats.rxRestartRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInDataPackets", X25Stats.rxData));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInAccusedOfProtocolErrors",
		X25Stats.rxClearRqst+X25Stats.rxResetRqst+
		X25Stats.rxRestartRqst+X25Stats.rxDiagnostic));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInInterrupts", X25Stats.rxInterrupt));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatOutCallAttempts", X25Stats.txCallRequest));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatOutCallFailures", X25Stats.rxClearRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatOutInterrupts", X25Stats.txInterrupt));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatOutDataPackets", X25Stats.txData));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatOutgoingCircuits", 
		 X25Stats.txCallRequest-X25Stats.rxClearRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatIncomingCircuits",
		X25Stats.rxCallRequest-X25Stats.txClearRqst));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatTwowayCircuits",
		 X25Stats.txCallRequest-X25Stats.rxClearRqst));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatRestartTimeouts", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatCallTimeouts", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatResetTimeouts", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatClearTimeouts", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatDataRxmtTimeouts", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatInterruptTimeouts", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatRetryCountExceededs", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25StatClearCountExceededs", 0));
                  
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, 
		 "********** X.25 Channel Table *********\n"));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
		 "x25ChannelLIC", card->u.x.x25_conf.lo_svc));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ChannelHIC", card->u.x.x25_conf.hi_svc));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ChannelLTC", card->u.x.x25_conf.lo_svc));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ChannelHTC", card->u.x.x25_conf.hi_svc));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ChannelLOC", card->u.x.x25_conf.lo_svc));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ChannelHOC", card->u.x.x25_conf.hi_svc));

	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, 
		"********** X.25 Per Circuits Information Table *********\n"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
               	 "x25CircuitChannel", 0));
	// FIXME 
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitStatus", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25CircuitEstablishTime", chan->chan_establ_time));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitDirection", chan->chan_direct));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25CircuitInOctets", chan->ifstats.rx_bytes));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25CircuitInPdus", chan->ifstats.rx_packets));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitInRemotelyInitiatedResets", 0));
	// FIXME       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitInProviderInitiatedResets", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitInInterrupts", X25Stats.rxInterrupt));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25CircuitOutOctets", chan->ifstats.tx_bytes));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25CircuitOutPdus", chan->ifstats.tx_packets));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitOutInterrupts", X25Stats.txInterrupt));
	// FIXME	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitDataRetransmissionTimeouts", 0));
	// FIXME	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitResetTimeouts", 0));
	// FIXME	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitInterruptTimeouts", 0));
	// FIXME	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitCallParamId", 0));
	// FIXME       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CircuitCalledDteAddress",
		 (chan->chan_direct == SNMP_X25_INCOMING) ? chan->accept_src_addr : "N/A"));
	// FIXME	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CircuitCallingDteAddress",
		 (chan->chan_direct == SNMP_X25_INCOMING) ? chan->accept_dest_addr : "N/A"));
	// FIXME       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CircuitOriginallyCalledAddress", 0));
	// FIXME       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CircuitDescr", "N/A"));
	       
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size,
		"********** X.25 The Cleared Circuit Table *********\n"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ClearedCircuitEntriesRequested", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ClearedCircuitEntriesGranted", 0));
	// FIXME
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
        	 "x25ClearedCircuitIndex", 0));
	// FIXME
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ClearedCircuitPleIndex", 0));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25ClearedCircuitTimeEstablished", chan->chan_establ_time));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25ClearedCircuitTimeCleared", chan->chan_clear_time));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ClearedCircuitChannel", chan->ch_idx));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ClearedCircuitClearingCause", chan->chan_clear_cause));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25ClearedCircuitDiagnosticCode", chan->chan_clear_diagn));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25ClearedCircuitInPdus", chan->ifstats.rx_packets));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_L_FRM,
                 "x25ClearedCircuitOutPdus", chan->ifstats.tx_packets));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25ClearedCircuitCalledAddress", 
		 (chan->cleared_called_addr[0] != '\0') ? 
		 			chan->cleared_called_addr : "N/A"));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25ClearedCircuitCallingAddress",
		 (chan->cleared_calling_addr[0] != '\0') ? 
		 			chan->cleared_calling_addr : "N/A"));
 	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25ClearedCircuitClearFacilities",
		 (chan->cleared_facil[0] != '\0') ? chan->cleared_facil : "N/A"));

	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, 
		"********** X.25 The Call Parameter Table *********\n"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmIndex", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmStatus", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmRefCount", 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmInPacketSize", 0));	// default size
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmOutPacketSize", 0));	// default size
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmInWindowSize",
		 (card->u.x.x25_conf.pkt_window != X25_PACKET_WINDOW) ? 
					card->u.x.x25_conf.pkt_window : 0));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmOutWindowSize",
		 (card->u.x.x25_conf.pkt_window != X25_PACKET_WINDOW) ? 
					card->u.x.x25_conf.pkt_window : 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmAcceptReverseCharging", SNMP_X25_ARC_DEFAULT));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmProposeReverseCharging", SNMP_X25_PRC_DEFAULT));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmFastSelect", SNMP_X25_FS_DEFAULT));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmInThruPutClasSize", SNMP_X25_THRUCLASS_TCDEF));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmOutThruPutClasSize", SNMP_X25_THRUCLASS_TCDEF));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmCug", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmCugoa", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmBcug", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmNui", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmChargingInfo", SNMP_X25_CHARGINGINFO_DEF));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmRpoa", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmTrnstDly", 0));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmCallingExt", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmCalledExt", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmInMinThuPutCls", SNMP_X25_THRUCLASS_TCDEF));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmOutMinThuPutCls", SNMP_X25_THRUCLASS_TCDEF));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmEndTrnsDly", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmPriority", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmProtection", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_D_FRM,
                 "x25CallParmExptData", SNMP_X25_EXPTDATA_DEFULT));
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmUserData", chan->accept_usr_data));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmCallingNetworkFacilities", "N/A"));
	// FIXME
	PROC_ADD_LINE(cnt, 
		(buf, &cnt, len, offs, &stop_cnt, &size, PROC_IF_X25_S_FRM,
                 "x25CallParmCalledNetworkFacilities", "N/A"));

get_if_info_end:
	PROC_ADD_RET(cnt, offs, stop_cnt);

}

	
static int x25_set_if_info(struct file *file, 
			  const char *buffer,
			  unsigned long count, 
			  void *data)
{
	netdevice_t*	dev = (void*)data;
	x25_channel_t* 	chan = NULL;
	sdla_t*		card = NULL;

	if (dev == NULL || dev->priv == NULL)
		return count;
	chan = (x25_channel_t*)dev->priv;
	if (chan->card == NULL)
		return count;
	card = chan->card;

	printk(KERN_INFO "%s: New interface config (%s)\n",
			chan->name, buffer);
	/* Parse string */

	return count;
}

#endif

static int test_chan_command_busy(sdla_t *card,x25_channel_t *chan)
{
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

	while((jiffies-chan->cmd_timeout) < HZ){
		if (!atomic_read(&chan->common.command)){
			return atomic_read(&chan->cmd_rc);	
		}
		schedule();
	}
	DEBUG_EVENT("%s: !!!!!!!! CRITICAL ERROR !!!!!!! Command %x Timed out !\n",
			card->devname,atomic_read(&chan->common.command));

	atomic_set(&chan->common.command,0);
	return -EFAULT;
}

static int delay_cmd(sdla_t *card, x25_channel_t *chan, int delay)
{
	unsigned long timeout=jiffies;

	while((jiffies-timeout) < delay){
		schedule();
	}
	return 0;
}

/****** End *****************************************************************/
