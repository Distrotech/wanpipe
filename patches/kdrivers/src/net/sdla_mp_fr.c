/*****************************************************************************
* sdla_mp_fr.c  Multi-Port Frame Relay driver module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Apr 09,2002   Nenad Corbic	Initial version
* 				Based on raw-hdlc fimrware
* 				Some of the frame realy protocol was
* 				taken from linux/drivers/net/wan/hdlc.c
* 				and from Sangoma's own frame relay firmware.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/sdlapci.h>
#include <linux/wanproc.h>	
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
#include <linux/if_wanpipe.h>		
#include <linux/sdla_mp_fr.h>
#include <linux/wanpipe_snmp.h>

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
#include <linux/wanpipe_lapb_kernel.h>
#endif

/****** Defines & Macros ****************************************************/

#ifdef	_DEBUG_
#define	STATIC
#else
#define	STATIC		static
#endif

/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   	0x01
#define TMR_INT_ENABLED_UPDATE	0x02
#define TMR_INT_ENABLED_TE	0x03
 
#define	CHDLC_DFLT_DATA_LEN	1500		/* default MTU */
#define CHDLC_HDR_LEN		1

#define IFF_POINTTOPOINT 0x10

#define CHDLC_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_BH_BUFF	10

#define CRC_LENGTH 	2 
#define PPP_HEADER_LEN 	4
#define MAX_TRACE_LEN   25 
#define MAX_TRACE_ASCII_LEN MAX_TRACE_LEN*3+5

#define MAX_FR_RX_BUF 80
#define FR_RX_BUF_LIMIT 1001

#define FR_PROT_AREA(a) ((fr_prot_t*)a->u.c.prot)

#define MODE_FR_CCITT 0
#define MAX_FR_HEADER_SZ 16

#ifndef IS_TE1_CARD
#define IS_TE1_CARD(card)	IS_TE1(card->wandev.te_cfg)
#endif

#ifndef IS_56K_CARD
#define IS_56K_CARD(card)	IS_56K(card->wandev.te_cfg)
#endif


/* Enable TE1_56K Card support */
#define TE1_56_CARD_SUPPORT 1

#undef _DBG_ANNEXG_

/* Private critical flags */
enum { 
	SEND_TXIRQ_CRIT = PRIV_CRIT,
	ARP_CRIT
};
	
	
/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with CHDLC specific data
 */

typedef struct fr_private_area
{
	wanpipe_common_t common;
	sdla_t		*card;

	char		if_name[WAN_IFNAME_SZ+1];
	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
        
	u32             IP_address;		/* IP addressing */
        u32             IP_netmask;
	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned short 	udp_pkt_lgth;		/* udp packet processing */
	char 		udp_pkt_src;

	unsigned char 	trace_state;
	unsigned char 	trace_buf [MAX_TRACE_ASCII_LEN];

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;
	
	unsigned long 	router_last_change;
	unsigned int	dlci_type;
	unsigned long	rx_DE_set;
	unsigned long	tx_DE_set;
	unsigned long	rx_FECN;
	unsigned long	rx_BECN;
	unsigned 	err_type;
	char		err_data[SNMP_FR_ERRDATA_LEN];
	unsigned long 	err_time;
	unsigned long 	err_faults;
	unsigned	trap_state;
	unsigned long	trap_max_rate;
	unsigned cir;			/* committed information rate */
	unsigned bc;			/* committed burst size */
	unsigned be;			/* excess burst size */
	
	unsigned char 	ignore_modem;

	unsigned short 	dlci;
	unsigned char	dlci_state;
	unsigned char   inarp;
	unsigned char   route_flag;
	unsigned short  newstate;
	
	struct net_device_stats stats;
	unsigned char	hdr_len;
	unsigned char  	header[MAX_FR_HEADER_SZ];

	unsigned char udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t udp_pkt_len;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	netdevice_t *annexg_dev;
	unsigned char label[WAN_IF_LABEL_SZ+1];
#endif

	if_send_stat_t drvstats_if_send;
        rx_intr_stat_t drvstats_rx_intr;
        pipe_mgmt_stat_t drvstats_gen;

	unsigned long  tracing_enabled;
	struct sk_buff_head trace_queue;
	unsigned long  trace_timeout;

	unsigned int   max_trace_queue;

} fr_channel_t, fr_private_area_t;

#if 0
int fr_to_hdlc_cmd_map[]
{

}
#endif


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

#undef TX_PKT_DEBUG
#undef RX_PKT_DEBUG


/* variable for keeping track of enabling/disabling FT1 monitor status */
static int rCount = 0;

/* variable for tracking how many interfaces to open for WANPIPE on the
   two ports */

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);
extern unsigned short wan_calc_checksum (char *, int);
//extern void debug_print_udp_pkt(unsigned char *data,int len,char trc_enabled, char direction);


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);
static int del_if (wan_device_t* wandev, netdevice_t* dev);

static int fr_snmp_data(sdla_t* card, netdevice_t *dev, void* data);

static void disable_comm (sdla_t *card);
static unsigned int dec_to_uint (unsigned char* str, int len);
static netdevice_t* find_channel (sdla_t* card, unsigned dlci);
static void fr_timer(unsigned long arg);
static void fr_lmi_send(sdla_t *card, int fullrep);
static int fr_hard_header(sdla_t *card, netdevice_t *dev, u16 type);



/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
static int if_send (struct sk_buff* skb, netdevice_t* dev);
static struct net_device_stats* if_stats (netdevice_t* dev);

static void if_tx_timeout (netdevice_t *dev);

/* CHDLC Firmware interface functions */
static int hdlc_configure 	(sdla_t* card, void* data);
static int hdlc_comm_enable 	(sdla_t* card);
static int hdlc_comm_disable 	(sdla_t* card);
static int hdlc_read_version 	(sdla_t* card, char* str);
static int hdlc_set_intr_mode 	(sdla_t* card, unsigned mode);
static int hdlc_send (sdla_t* card, void* data, unsigned len);
static int hdlc_send_hdr_data (sdla_t* card, netdevice_t *dev, void* data, unsigned len);

static int hdlc_read_comm_err_stats (sdla_t* card);
static int hdlc_read_op_stats (sdla_t* card);
static int config_hdlc (sdla_t *card);
#ifdef TE1_56_CARD_SUPPORT
static int set_adapter_config (sdla_t* card);
#endif

/* Miscellaneous CHDLC Functions */
static int set_hdlc_config (sdla_t* card);
static void init_hdlc_tx_rx_buff( sdla_t* card);
static int hdlc_error (sdla_t *card, int err, wan_mbox_t *mb);
static int process_hdlc_exception(sdla_t *card);
static int process_global_exception(sdla_t *card);
static int update_comms_stats(sdla_t* card);
static void chan_set_state (netdevice_t *dev, int);

/* Interrupt handlers */
static WAN_IRQ_RETVAL wp_hdlc_fr_isr (sdla_t* card);
static void rx_intr (sdla_t* card);
static void tx_intr (sdla_t *card);
static void timer_intr(sdla_t *);

static void fr_bh (unsigned long card_data);

/* Miscellaneous functions */
static int reply_udp( unsigned char *data, unsigned int mbox_len );
static int intr_test( sdla_t* card);
//static int udp_pkt_type( struct sk_buff *skb, sdla_t* card, int direction);
//static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
//                              struct sk_buff *skb, netdevice_t* dev,
//                                fr_private_area_t* chan);
static int process_udp_mgmt_pkt(sdla_t* card, void* local_dev);

static void s508_s514_lock (sdla_t *card, unsigned long *smp_flags);
static void s508_s514_unlock (sdla_t *card, unsigned long *smp_flags);

static int fr_get_config_info(void* priv, struct seq_file* m, int*);
static int fr_get_status_info(void* priv, struct seq_file* m, int*);

static int fr_set_dev_config(struct file*, const char*, unsigned long, void *);
static int fr_set_if_info(struct file*, const char*, unsigned long, void *);

static int capture_trace_packet (sdla_t *card,fr_private_area_t*chan,struct sk_buff *skb,char dir);

/* TE1 */
#ifdef TE1_56_CARD_SUPPORT
static void hdlc_enable_timer (void* card_id);
static void fr_handle_front_end_state (void* card_id);
#endif


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev);
static netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t* annexg_dev_name);
static int get_map(wan_device_t *wandev, netdevice_t *dev, struct seq_file* m, int *);
static void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats);
#endif

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
int wp_hdlc_fr_init (sdla_t* card, wandev_conf_t* conf)
{
	unsigned char port_num;
	int err;
	unsigned long max_permitted_baud = 0;
	fr_prot_t *fr_prot;

	int i;
	struct timeval tv;

	union{
		char str[80];
	}u;
	volatile wan_mbox_t* mb;
	wan_mbox_t* mb1;
	unsigned long timeout, smp_lock;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_MFR) {
		printk(KERN_INFO "%s: invalid configuration ID %u MFR=%u!\n",
				  card->devname, conf->config_id, WANCONFIG_MFR);
		return -EINVAL;
	}

	/* Find out which Port to use. HDLC protocol can use both primary
	 * and secondayr pory */
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

	/* Allocate and initialize Frame relay protocol area */
	if (!card->u.c.prot){
		card->u.c.prot=kmalloc(sizeof(fr_prot_t), GFP_KERNEL);
		if (!card->u.c.prot){
			return -ENOMEM;
		}
		memset(card->u.c.prot,0,sizeof(fr_prot_t));
	}
	
	fr_prot = FR_PROT_AREA(card);

	fr_prot->station = conf->u.fr.station;
	memcpy(&fr_prot->cfg, &conf->u.fr, sizeof(wan_fr_conf_t));

	
	/* Setup a shared memory pointers to protocol mailbox 
	 * (set a pointer to the actual mailbox in the allocated 
	 * virtual memory area) */
	/* ALEX Apr 8 2004 Sangoma ISA card */
	if (card->u.c.comm_port == WANOPT_PRI){
		card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;
	}else{
		card->mbox_off = SEC_BASE_ADDR_MB_STRUCT;
	}

	mb = &card->wan_mbox;
	mb1 = &card->wan_mbox;

	/* Check that firmware is up and running by checking for I in
	 * mail box */
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

	card->wandev.ignore_front_end_status=WANOPT_NO;

	//err=check_conf_hw_mismatch(card,conf->te_cfg.media);
	err = (card->hw_iface.check_mismatch) ? 
			card->hw_iface.check_mismatch(card->hw,conf->fe_cfg.media) : -EINVAL;
	if (err){
		return err;
	}
	
	/* TE1 Make special hardware initialization for T1/E1 board */
        if (IS_TE1_MEDIA(&conf->fe_cfg)) {

#ifdef TE1_56_CARD_SUPPORT
		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
		card->wandev.fe_enable_timer = hdlc_enable_timer;
		card->wandev.te_link_state = fr_handle_front_end_state;
		conf->electrical_interface = 
			(IS_T1_CARD(card)) ? WANOPT_V35 : WANOPT_RS232;

		if (card->u.c.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}
#else
		return -EINVAL;
#endif
		
        }else if (IS_56K_MEDIA(&conf->fe_cfg)) {

#ifdef TE1_56_CARD_SUPPORT
		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe,&card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
		if (card->u.c.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}
#else
		return -EINVAL;
#endif
	}else{
		card->fe.fe_status = FE_CONNECTED;
	}


	if (card->wandev.ignore_front_end_status == WANOPT_NO){
		printk(KERN_INFO 
		  "%s: Enabling front end state monitor\n",
				card->devname);
	}else{
		printk(KERN_INFO 
		"%s: Disabling front end state monitor\n",
				card->devname);
	}

	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (hdlc_read_version(card, u.str))
		return -EIO;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	printk(KERN_INFO "%s: Running Frame Relay AnnexG over Raw-HDLC firmware v%s\n",
			card->devname,u.str); 
#else
	printk(KERN_INFO "%s: Running Frame Relay over Raw-HDLC firmware v%s\n",
			card->devname,u.str);
#endif

	
#ifdef TE1_56_CARD_SUPPORT
	if (set_adapter_config(card)) {
		return -EIO;
	}
#endif
	
	/* Initialize standard function pointer that are used
	 * to control, configure and interigate currently 
	 * running protocol */
	card->isr			= &wp_hdlc_fr_isr;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->wandev.udp_port   	= conf->udp_port;
	card->disable_comm		= &disable_comm;

	card->wandev.new_if_cnt = 0;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	card->wandev.bind_annexg	= &bind_annexg;
	card->wandev.un_bind_annexg	= &un_bind_annexg;
	card->wandev.get_map		= &get_map;
	card->wandev.get_active_inactive= &get_active_inactive;
#endif
	
	/* Initialize Proc fs functions */
	card->wandev.get_config_info 	= &fr_get_config_info;
	card->wandev.get_status_info 	= &fr_get_status_info;
	card->wandev.set_dev_config    	= &fr_set_dev_config;
	card->wandev.set_if_info     	= &fr_set_if_info;

	/* SNMP data */
	card->get_snmp_data	= &fr_snmp_data;

	/* reset the number of times the 'update()' proc has been called */
	card->u.c.update_call_count = 0;
	
	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 


	/* The secondary port on S508 card can only have RS232 interface */
	if ((card->u.c.comm_port == WANOPT_SEC && conf->electrical_interface == WANOPT_V35)&&
	    card->type != SDLA_S514){
		printk(KERN_INFO "%s: ERROR - V35 Interface not supported on S508 %s port \n",
			card->devname, PORT(card->u.c.comm_port));
		return -EIO;
	}


	card->wandev.clocking = conf->clocking;

	port_num = card->u.c.comm_port;

	
	/* Setup Port Baud Rate Bps */
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
				printk(KERN_INFO "%s: Baud too high!\n",
					card->wandev.name);
 				printk(KERN_INFO "%s: Baud rate set to %lu bps\n", 
					card->wandev.name, max_permitted_baud);
			}
                             
			card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
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

	
	/* Set up the interrupt status area */
	/* Read the CHDLC Configuration and obtain: 
	 *	Ptr to shared memory infor struct
         * Use this pointer to calculate the value of card->u.c.flags !
 	 */
	
	mb1->wan_data_len = 0;
	mb1->wan_command = READ_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb1);
	if(err != COMMAND_OK) {
		clear_bit(1, (void*)&card->wandev.critical);

                if(card->type != SDLA_S514)
                	enable_irq(card->wandev.irq/*ALEX_TODAY card->hw.irq*/);

		hdlc_error(card, err, mb1);
		return -EIO;
	}

	/* ALEX Apr 8 2004 Sangoma ISA card */
	card->flags_off =
			((CHDLC_CONFIGURATION_STRUCT *)mb1->wan_data)->
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
	
	skb_queue_head_init(&fr_prot->rx_free);
	skb_queue_head_init(&fr_prot->rx_used);
	skb_queue_head_init(&fr_prot->lmi_queue);
	skb_queue_head_init(&fr_prot->trace_queue);

	fr_prot->max_rx_queue = MAX_FR_RX_BUF;

	if (conf->max_rx_queue && conf->max_rx_queue<FR_RX_BUF_LIMIT){
		fr_prot->max_rx_queue = conf->max_rx_queue;
	}
	printk(KERN_INFO "%s: Setting Frame Relay global rx queue len to %i\n",
			card->devname,fr_prot->max_rx_queue);

	for (i=0;i<fr_prot->max_rx_queue;i++){
		struct sk_buff *skb;
		skb=dev_alloc_skb(card->wandev.mtu+10);
		if (!skb){
			printk(KERN_INFO "%s: Failed to allocate rx queues! No Memory!\n",card->devname);
			skb_queue_purge(&fr_prot->rx_free);
			return -ENOMEM;
		}
		skb_queue_tail(&fr_prot->rx_free,skb);
	}
		
	/* This is for the ports link state */
	card->wandev.state = WAN_DUALPORT;
	card->u.c.state = WAN_DISCONNECTED;

	tasklet_init(&fr_prot->wanpipe_task,fr_bh,(u32)card);
	
	fr_prot->max_trace_queue = MAX_TRACE_QUEUE;
	if (conf->max_trace_queue && conf->max_trace_queue<TRACE_QUEUE_LIMIT){
		fr_prot->max_trace_queue=conf->max_trace_queue;
	}
	
	printk(KERN_INFO "%s: Setting Max LMI trace queue to %i packets\n",
				card->devname,fr_prot->max_trace_queue);
	
	if (!card->wandev.piggyback){
		err = intr_test(card);

		if(err || (card->timer_int_enabled < MAX_INTR_TEST_COUNTER)) { 
			printk(KERN_INFO "%s: Interrupt test failed (%i)\n",
					card->devname, card->timer_int_enabled);
			printk(KERN_INFO "%s: Please choose another interrupt\n",
					card->devname);
			return  -EIO;
		}
			
		printk(KERN_INFO "%s: Interrupt test passed (%i)\n", 
				card->devname, card->timer_int_enabled);
	}

	wan_spin_lock_irq(&card->wandev.lock,&smp_lock);
	if (config_hdlc(card) < 0){
		wan_spin_unlock_irq(&card->wandev.lock,&smp_lock);
		return -EINVAL;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_lock);

	spin_lock_init(&card->u.c.if_send_lock);

	printk(KERN_INFO "%s: Frame Station=%s, Signalling=%s\n",
			card->devname,
			fr_prot->station == WANOPT_NODE ? "NODE" : "CPE",
			fr_prot->cfg.signalling == WANOPT_FR_ANSI ? "ANSI":"LMI");

	do_gettimeofday(&tv);
	card->u.c.router_start_time = tv.tv_sec;

	fr_prot->state = LINK_STATE_CHANGED;
	fr_prot->txseq = fr_prot->rxseq = 0;
	fr_prot->last_errors = 0xFFFFFFFF;
	fr_prot->n391cnt = 0;
	
	init_timer(&fr_prot->timer);
	fr_prot->timer.function = fr_timer;
	fr_prot->timer.data = (unsigned long)card;
	fr_prot->timer.expires = (jiffies+HZ);
	add_timer(&fr_prot->timer);

	if (fr_prot->station == WANOPT_CPE){
		printk(KERN_INFO "%s: Waiting for DLCI report from frame relay switch...\n\n",	
				card->devname);
	}

	fr_prot->state |= LINK_STATE_RELIABLE;
	wanpipe_set_state(card,WAN_CONNECTED);
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
#if 0
        if(!card->u.c.flags)
                return -ENODEV;
      	flags = card->u.c.flags;
#endif 
       
	if(test_and_set_bit(0,&card->update_comms_stats)){
		return -EAGAIN;
	}

			
	/* we will need 2 timer interrupts to complete the */
	/* reading of the statistics */
#if 0
#ifdef TE1_56_CARD_SUPPORT
	card->update_comms_stats = 
		(IS_TE1_CARD(card) || IS_56K_CARD(card)) ? 3 : 2;
#else
	card->update_comms_stats = 2;
#endif

       	flags->interrupt_info_struct.interrupt_permission |= APP_INT_ON_TIMER;
	card->u.c.timer_int_enabled = TMR_INT_ENABLED_UPDATE;
 
	/* wait a maximum of 1 second for the statistics to be updated */ 
        timeout = jiffies;
        for(;;) {
		if(card->update_comms_stats == 0)
			break;
                if ((jiffies - timeout) > (1 * HZ)){
    			card->update_comms_stats = 0;
 			card->u.c.timer_int_enabled &=
				~TMR_INT_ENABLED_UPDATE; 
 			return -EAGAIN;
		}
		schedule();
        }
#else
	spin_lock_irqsave(&card->wandev.lock, smp_flags);
	update_comms_stats(card);	
	card->update_comms_stats=0;
	spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

#endif
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
	fr_private_area_t* chan;
	int err = 0;
	unsigned short dlci=0;
	fr_prot_t *fr_prot;
	unsigned long smp_lock;

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		printk(KERN_INFO "%s: Error invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}
	
	/* allocate and initialize private data */
	chan = kmalloc(sizeof(fr_private_area_t), GFP_KERNEL);
	
	if(chan == NULL) 
		return -ENOMEM;

	memset(chan, 0, sizeof(fr_private_area_t));
	
	strncpy(chan->if_name,conf->name,WAN_IFNAME_SZ);
	
	chan->card = card; 
	fr_prot = FR_PROT_AREA(card);
	
	/* verify media address */
	if (is_digit(conf->addr[0])) {

		dlci = dec_to_uint(conf->addr, 0);

		if (dlci && (dlci <= HIGHEST_VALID_DLCI)) {
		
			chan->dlci = dlci;
			chan->common.lcn=dlci;
		
		} else {
		
			printk(KERN_INFO
				"%s: Invalid DLCI %u on interface %s Max=%i!\n",
				wandev->name, dlci, chan->if_name,HIGHEST_VALID_DLCI);
			err = -EINVAL;
		}

	} else {
		printk(KERN_INFO
			"%s: Invalid media address on interface %s, i.e. no DLCI specified!\n",
			wandev->name, chan->if_name);
		err = -EINVAL;
	}


	if (err != 0){
		kfree(chan);
		return err;
	}

	printk(KERN_INFO "\n");
	printk(KERN_INFO "%s: Configuring interface %s with DLCI=%i\n",
			card->devname,conf->name,chan->dlci);
	
         /* Disallow duplicate dlci configurations. */
	if (fr_prot->dlci_to_dev_map[chan->dlci] != NULL){
		kfree(chan);
		printk(KERN_INFO "%s: %s: DLCI %i already configured!\n",
				card->devname,chan->if_name,chan->dlci);
		return -EEXIST;
	}

	chan->route_status = NO_ROUTE;
	chan->route_removed = 0;

	/* Setup wanpipe as a router (WANPIPE) or as an API */
	if( strcmp(conf->usedby, "WANPIPE") == 0) {
		printk(KERN_INFO "%s: %s: Interface running in WANPIPE mode!\n",
			wandev->name,chan->if_name);
		card->u.c.usedby = WANPIPE;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	}else if (strcmp(conf->usedby, "ANNEXG") == 0){

		chan->common.usedby = ANNEXG;
		printk(KERN_INFO "%s: %s: Interface Running in ANNEXG mode.\n", 
				card->devname,chan->if_name);
	

#endif
	}else{
		printk(KERN_INFO 
			"%s: %s: API mode is not supported by Wanpipe Frame Realy!\n",
			wandev->name,chan->if_name);
		kfree(chan);
		return -EINVAL;
	}

	/* Get Multicast Information */
	chan->mc = conf->mc;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (strlen(conf->label)){
		strncpy(chan->label,conf->label,WAN_IF_LABEL_SZ);
	}
#endif

	/*
	 * Create interface file in proc fs.
	 */
	err = wanrouter_proc_add_interface(wandev, 
					   &chan->dent, 
					   conf->name, 
					   dev);
	if (err){	
		printk(KERN_INFO
			"%s: Failed to create /proc/net/router/fr/%s entry!\n",
			card->devname, conf->name);
		kfree(chan);	
		dev->priv=NULL;
		return err;
	}

	dev->init = NULL;
	dev->priv = chan;

	chan->dlci_state=0;
	chan->newstate=0;

	chan->max_trace_queue = MAX_TRACE_QUEUE;
	if (conf->max_trace_queue){
		chan->max_trace_queue = conf->max_trace_queue;
	}
	printk(KERN_INFO "%s: %s: Setting DLCI=%i trace queue to %i packets\n",
			card->devname,chan->if_name,chan->dlci,chan->max_trace_queue);

	skb_queue_head_init(&chan->trace_queue);

	card->wandev.new_if_cnt++;
	chan->common.state = WAN_DISCONNECTED;

	if_init(dev);

	wan_spin_lock_irq(&card->wandev.lock,&smp_lock);
	fr_prot->dlci_to_dev_map[chan->dlci] = dev;
	wan_spin_unlock_irq(&card->wandev.lock,&smp_lock);

	printk(KERN_INFO "\n");
	return 0;
}


/*============================================================================
 * Delete logical channel.
 */
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	fr_private_area_t *chan = dev->priv;
	sdla_t *card = wandev->priv;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	unsigned long smp_flags;
	
	if (!chan)
		return 0;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (fr_prot->tx_dev==dev){
		fr_prot->tx_dev=NULL;
	}
	if (chan->dlci && fr_prot->dlci_to_dev_map[chan->dlci]){
		fr_prot->dlci_to_dev_map[chan->dlci] = NULL;
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		struct net_device *tmp_dev;
		int err;
		
		printk(KERN_INFO "%s: Unregistering Lapb Protocol\n",wandev->name);
		
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

	/* Delete interface name from proc fs. */
	wanrouter_proc_delete_interface(wandev, chan->if_name);
	
	skb_queue_purge(&chan->trace_queue);
	chan_set_state(dev, WAN_DISCONNECTED);
	card->wandev.new_if_cnt--;
	
	return 0;
}

static void disable_comm (sdla_t *card)
{
	fr_prot_t* fr_prot = FR_PROT_AREA(card);
	unsigned long smp_lock;

	tasklet_kill(&fr_prot->wanpipe_task);
	
	wan_spin_lock_irq(&card->wandev.lock,&smp_lock);
	hdlc_set_intr_mode(card, 0);
	if (card->u.c.comm_enabled){
		hdlc_comm_disable(card);
	}
	wan_spin_unlock_irq(&card->wandev.lock,&smp_lock);

	if (fr_prot){
		del_timer(&fr_prot->timer);
		wan_spin_lock_irq(&card->wandev.lock,&smp_lock);
		skb_queue_purge(&fr_prot->rx_free);	
		skb_queue_purge(&fr_prot->rx_used);
		skb_queue_purge(&fr_prot->lmi_queue);
		skb_queue_purge(&fr_prot->trace_queue);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_lock);
		kfree(fr_prot);
		card->u.c.prot=NULL;
	}

#ifdef TE1_56_CARD_SUPPORT
	/* TE1 - Unconfiging */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
	}
#endif
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
	fr_private_area_t* chan = dev->priv;
	sdla_t* card = chan->card;
	fr_prot_t *fr_prot=FR_PROT_AREA(card);
	wan_device_t* wandev = &card->wandev;
	
	/* NOTE: Most of the dev initialization was
         *       done in sppp_attach(), called by new_if() 
         *       function. All we have to do here is
         *       to link four major routines below. 
         */

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
		
	dev->hard_header_len = 0;
	dev->addr_len = 2;
	*(u16*)dev->dev_addr = htons(chan->dlci);
	dlci_to_q922(dev->broadcast, fr_prot->lmi_dlci);
	
	dev->flags		|= IFF_POINTOPOINT;
	dev->flags		|= IFF_NOARP;
	
	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

	dev->type	= ARPHRD_DLCI;
	dev->mtu	= card->wandev.mtu;

	/* Set transmit buffer queue length 
         * If we over fill this queue the packets will
         * be droped by the kernel.
         * sppp_attach() sets this to 10, but
         * 100 will give us more room at low speeds.
	 */
        dev->tx_queue_len = 100;
	   
	/* Initialize socket buffers */
	//dev_init_buffers(dev);

	return 0;
}


/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	fr_private_area_t* chan = dev->priv;
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
 * Open network interface.
 * o enable communications and interrupts.
 * o prevent module from unloading by incrementing use count
 *
 * Return 0 if O.k. or errno.
 */
static int if_open (netdevice_t* dev)
{
	fr_private_area_t* chan = dev->priv;
	sdla_t* card = chan->card;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);

	/* Only one open per interface is allowed */

	if (open_dev_check(dev))
		return -EBUSY;

	netif_start_queue(dev);
	wanpipe_open(card);

	if (fr_prot->station == WANOPT_NODE){
		chan->dlci_state = 0;
		fr_prot->state |= LINK_STATE_CHANGED;
	}else{
		if (chan->dlci_state & PVC_STATE_ACTIVE){
			chan_set_state(dev,WAN_CONNECTED);	
		}else{
			chan_set_state(dev,WAN_CONNECTING);
			chan->dlci_state=0;
		}
	}

	return 0;
}

/*============================================================================
 * Close network interface.
 * o if this is the last close, then disable communications and interrupts.
 * o reset flags.
 */
static int if_close (netdevice_t* dev)
{
	fr_private_area_t* chan = dev->priv;
	sdla_t* card = chan->card;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);

	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif

	if (fr_prot->station == WANOPT_NODE){
		chan->dlci_state &= ~(PVC_STATE_ACTIVE|PVC_STATE_NEW);
		fr_prot->state |= LINK_STATE_CHANGED;
	}
	wanpipe_close(card);

	chan_set_state(dev,WAN_DISCONNECTED);
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
static int if_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	unsigned long smp_flags;
	fr_private_area_t* chan = dev->priv;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	
	if (!chan)
		return -ENODEV;

	card = chan->card;
	
	if(!capable(CAP_NET_ADMIN))
		return -EPERM;

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
			
			wan_udp_pkt=(wan_udp_pkt_t*)&chan->udp_pkt_data;
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)){
				printk(KERN_INFO "%s:%s pipemon command busy: try again!\n",
						card->devname,dev->name);
				atomic_set(&chan->udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EBUSY;
			}

			
			if (process_udp_mgmt_pkt(card,dev) <= 0 ){
				atomic_set(&chan->udp_pkt_len,0);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
				return -EINVAL;
			}

			
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
	fr_private_area_t *chan = dev->priv;
	sdla_t *card = chan->card;
	//int udp_type = 0;
	unsigned long smp_flags;
	int err=0;

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	netif_stop_queue(dev);
	dev->trans_start = jiffies;
#endif
	
	if (skb == NULL){
		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		printk(KERN_INFO "%s: Received NULL skb buffer! interface %s got kicked!\n",
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
		if((jiffies - chan->tick_counter) < (5 * HZ)) {
			return 1;
		}
		if_tx_timeout(dev);
	}
#endif

	if (test_bit(SEND_TXIRQ_CRIT,&card->wandev.critical)){
		stop_net_queue(dev);
		chan->tick_counter = jiffies;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
		//printk(KERN_INFO "%s: (Debug): IF send busy: on TXIRQ CRIT \n",dev->name);
		return 1;
	}
	
	s508_s514_lock(card, &smp_flags);

    	if (test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)){
	
		printk(KERN_INFO "%s: Critical in if_send: %lx\n",
					card->wandev.name,card->wandev.critical);
                ++card->wandev.stats.tx_dropped;
		++chan->stats.tx_carrier_errors;
		start_net_queue(dev);
		goto if_send_crit_exit;
	}

	if (card->wandev.state != WAN_CONNECTED){
		++card->wandev.stats.tx_dropped;
		++chan->stats.tx_carrier_errors;
		start_net_queue(dev);
		goto if_send_crit_exit;
	}

	if (chan->common.state != WAN_CONNECTED){
		++card->wandev.stats.tx_dropped;
		++chan->stats.tx_carrier_errors;
		start_net_queue(dev);
		goto if_send_crit_exit;
	}

	if (fr_hard_header(card, dev, htons(skb->protocol)) <= 0){
		++card->wandev.stats.tx_errors;
		start_net_queue(dev);
		goto if_send_crit_exit;
	}

	err=hdlc_send_hdr_data(card, dev, skb->data, skb->len);
	
	if (err){
		err=-1;
		stop_net_queue(dev);
		//printk(KERN_INFO "%s: (Debug): IF send failed\n",dev->name);

	}else{

		if (capture_trace_packet(card,chan,skb,TRC_OUTGOING_FRM) < 0){
			chan->stats.tx_fifo_errors++;
		}

		++card->wandev.stats.tx_packets;
       		card->wandev.stats.tx_bytes += skb->len;
		++chan->stats.tx_packets;
		chan->stats.tx_bytes += skb->len;
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		dev->trans_start = jiffies;
#endif

#ifdef _DBG_ANNEXG_
		check_x25_pr_ps_cnt(card,skb);
#endif		
		start_net_queue(dev);
		
	}	

if_send_crit_exit:
	if (err==0){
                dev_kfree_skb_any(skb);
	}else{
		chan->tick_counter = jiffies;
		card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
	}

	clear_bit(SEND_CRIT, (void*)&card->wandev.critical);
	s508_s514_unlock(card, &smp_flags);

	return err;
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
		wan_calc_checksum(&data[UDP_OFFSET/*+sizeof(fr_encap_hdr_t)*/],
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
		wan_calc_checksum(&data[/*sizeof(fr_encap_hdr_t)*/0],
		      	      sizeof(struct iphdr));

	return len;
} /* reply_udp */


/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	fr_private_area_t* chan;

	if (!dev || !(dev->flags&IFF_UP))
		return NULL;
	
	/* Shutdown bug fix. In del_if() we kill
         * dev->priv pointer. This function, gets
         * called after del_if(), thus check
         * if pointer has been deleted */
	if ((chan=dev->priv) == NULL)
		return NULL;

	return &chan->stats;
}

/****** Cisco HDLC Firmware Interface Functions *******************************/

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str. 
 */
static int hdlc_read_version (sdla_t* card, char* str)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int len;
	char err;
	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		hdlc_error(card,err,mb);
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
static int hdlc_configure (sdla_t* card, void* data)
{
	int err;
	wan_mbox_t *mbox = &card->wan_mbox;
	int data_length = sizeof(CHDLC_CONFIGURATION_STRUCT);
	
	mbox->wan_data_len = data_length;  
	memcpy(mbox->wan_data, data, data_length);
	mbox->wan_command = SET_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	
	if (err != COMMAND_OK) hdlc_error (card, err, mbox);
                           
	return err;
}


/*============================================================================
 * Set interrupt mode -- HDLC Version.
 */

static int hdlc_set_intr_mode (sdla_t* card, unsigned mode)
{
	wan_mbox_t* mb = &card->wan_mbox;
	CHDLC_INT_TRIGGERS_STRUCT* int_data =
		 (CHDLC_INT_TRIGGERS_STRUCT *)mb->wan_data;
	int err;

	int_data->CHDLC_interrupt_triggers 	= mode;
	int_data->IRQ				= card->wandev.irq; //ALEX_TODAY card->hw.irq;
	int_data->interrupt_timer               = 1;
   
	mb->wan_data_len = sizeof(CHDLC_INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_CHDLC_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		hdlc_error (card, err, mb);
	return err;
}


/*============================================================================
 * Enable communications.
 */

static int hdlc_comm_enable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_CHDLC_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		hdlc_error(card, err, mb);
	else
		card->u.c.comm_enabled=1;

	return err;
}

/*============================================================================
 * Disable communications and Drop the Modem lines (DCD and RTS).
 */
static int hdlc_comm_disable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = DISABLE_CHDLC_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		hdlc_error(card,err,mb);

	return err;
}

/*============================================================================
 * Read communication error statistics.
 */
static int hdlc_read_comm_err_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_COMMS_ERROR_STATS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                hdlc_error(card,err,mb);
        return err;
}


/*============================================================================
 * Read CHDLC operational statistics.
 */
static int hdlc_read_op_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_CHDLC_OPERATIONAL_STATS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                hdlc_error(card,err,mb);
        return err;
}


/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card)
{
        wan_mbox_t* mb = &card->wan_mbox;
  	COMMS_ERROR_STATS_STRUCT* err_stats;
        CHDLC_OPERATIONAL_STATS_STRUCT *op_stats;

	/* on the first timer interrupt, read the comms error statistics */
#ifdef TE1_56_CARD_SUPPORT
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
		card->wandev.fe_iface.read_alarm(&card->fe, 1); 
	}
#endif	
	
	if(hdlc_read_comm_err_stats(card))
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

       	if(hdlc_read_op_stats(card))
               	return 1;
	op_stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)mb->wan_data;
	card->wandev.stats.rx_length_errors =
		(op_stats->Rx_Data_discard_short_count +
		op_stats->Rx_Data_discard_long_count);

	return 0;
}

static int hdlc_send_hdr_data (sdla_t* card, netdevice_t *dev, void* data, unsigned len)
{
	CHDLC_DATA_TX_STATUS_EL_STRUCT txbuf;
	fr_private_area_t *chan = dev->priv;
	
	card->hw_iface.peek(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));
	if (txbuf.opp_flag)
		return 1;
	
#if defined(TX_PKT_DEBUG)
	if (len <= 5 && !(((char *)data)[1]&0x01)){
		int x;

		if (len == 5 && (((char*)data)[4]&0x01))
			goto tx_skip_error;

		printk(KERN_INFO "ERROR !!! TX: Annexg Tx Frame error %i\n",
				len);

		printk(KERN_INFO "Bad Packet : ");
		for (x=0;x<len;x++){
			printk("%X ",((char*)data)[x]);
		}
		printk("\n");
		printk(KERN_INFO "\n");
tx_skip_error:
	}
#endif

	/* IMPORTANT BUG FIX: 
	 *
	 * The frame length must be set a few	 
	 * instructions before setting opp_flag.  On S508
	 * cards (ISA), since the frame_length and opp_flags
	 * phisical locations are next to one another, the 
	 * ISA bus can write them simultaneously to the
	 * card. This could lead to race conditions.
	 * 
	 * In some instances the firware doesn't pick up the
	 * new length and sends a cut off frame with good CRC */
	
	txbuf.frame_length = (len+chan->hdr_len);

	card->hw_iface.poke(card->hw,txbuf.ptr_data_bfr,chan->header,chan->hdr_len);
	card->hw_iface.poke(card->hw,(txbuf.ptr_data_bfr+chan->hdr_len),data,len);

	txbuf.opp_flag = 1;		/* start transmission */
	card->hw_iface.peek(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));

	/* Update transmit buffer control fields */
	card->u.c.txbuf_off += sizeof(txbuf);

	if (card->u.c.txbuf_off > card->u.c.txbuf_last_off)
		card->u.c.txbuf_off = card->u.c.txbuf_base_off;
	return 0;
}
/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int hdlc_send (sdla_t* card, void* data, unsigned len)
{
	CHDLC_DATA_TX_STATUS_EL_STRUCT txbuf;

	card->hw_iface.peek(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));
	
	if (txbuf.opp_flag)
		return 1;
	
	card->hw_iface.poke(card->hw, txbuf.ptr_data_bfr, data, len);

	txbuf.frame_length = len;
	txbuf.opp_flag = 1;		/* start transmission */
	card->hw_iface.poke(card->hw, card->u.c.txbuf_off, &txbuf, sizeof(txbuf));
	
	/* Update transmit buffer control fields */
	card->u.c.txbuf_off += sizeof(txbuf);

	if (card->u.c.txbuf_off > card->u.c.txbuf_last_off)
		card->u.c.txbuf_off = card->u.c.txbuf_base_off;

	return 0;
}

#ifdef TE1_56_CARD_SUPPORT

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
		hdlc_error(card,err,mb);
	}
	return (err);
}


/*============================================================================
 * Enable timer interrupt  
 */
static void hdlc_enable_timer (void* card_id)
{
	sdla_t* 		card = (sdla_t*)card_id;

	card->u.c.timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	return;
}

#endif
/****** Firmware Error Handler **********************************************/

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int hdlc_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

	case CMD_TIMEOUT:
		printk(KERN_INFO "%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		break;

	case S514_BOTH_PORTS_SAME_CLK_MODE:
		if(cmd == SET_CHDLC_CONFIGURATION) {
			printk(KERN_INFO
			 "%s: Configure both ports for the same clock source\n",
				card->devname);
			break;
		}

	default:
		printk(KERN_INFO "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

/****** Interrupt Handlers **************************************************/

/*============================================================================
 * Cisco HDLC interrupt service routine.
 */
STATIC WAN_IRQ_RETVAL wp_hdlc_fr_isr (sdla_t* card)
{
	SHARED_MEMORY_INFO_STRUCT flags;
	int i;

	
	/* Check for which port the interrupt has been generated
	 * Since Secondary Port is piggybacking on the Primary
         * the check must be done here. 
	 */

	/* Start card isr critical area */
	set_bit(0,&card->in_isr);

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));

	/* if critical due to peripheral operations
	 * ie. update() or getstats() then reset the interrupt and
	 * wait for the board to retrigger.
	 */
	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		printk(KERN_INFO "%s: ISR: Critical due to PERI\n",
				card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
		goto isr_done;
	}

	/* On a 508 Card, if critical due to if_send 
         * Major Error !!!
	 */
	if(card->type != SDLA_S514) {
		if(test_bit(0, (void*)&card->wandev.critical)) {
			printk(KERN_INFO "%s: ISR: Critical due to SEND %lx\n",
				card->devname, card->wandev.critical);
			goto isr_done;
		}
	}

	switch(flags.interrupt_info_struct.interrupt_type) {
		case RX_APP_INT_PEND:	/* 0x01: receive interrupt */
			rx_intr(card);
			break;

		case TX_APP_INT_PEND:	/* 0x02: transmit interrupt */
			tx_intr(card);
			break;

		case COMMAND_COMPLETE_APP_INT_PEND:/* 0x04: cmd cplt */
			++ card->timer_int_enabled;
			break;

		case CHDLC_EXCEP_COND_APP_INT_PEND:	/* 0x20 */
			process_hdlc_exception(card);
			break;

		case GLOBAL_EXCEP_COND_APP_INT_PEND:
			process_global_exception(card);

#ifdef TE1_56_CARD_SUPPORT
			/* Reset the 56k or T1/E1 front end exception condition */
			if(IS_56K_CARD(card) || IS_TE1_CARD(card)) {
				FRONT_END_STATUS_STRUCT FE_status;
				card->hw_iface.peek(card->hw, card->fe_status_off,
						&FE_status, sizeof(FE_status));
				FE_status.opp_flag = 0x01;	
				card->hw_iface.poke(card->hw, card->fe_status_off,
						&FE_status, sizeof(FE_status));
			}
#endif	
			break;

		case TIMER_APP_INT_PEND:
			timer_intr(card);
			break;

		default:

			if (card->next){
				set_bit(0,&card->spurious);
				break;
			}
			
			printk(KERN_INFO "%s: spurious interrupt 0x%02X!\n", 
				card->devname,
				flags.interrupt_info_struct.interrupt_type);
			printk(KERN_INFO "Code name: ");
			for(i = 0; i < 4; i ++)
				printk("%c",
					flags.global_info_struct.codename[i]); 
			printk("\n");
			printk(KERN_INFO "Code version: ");
			for(i = 0; i < 4; i ++)
				printk("%c", 
					flags.global_info_struct.codeversion[i]); 
			printk("\n");	
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
	SHARED_MEMORY_INFO_STRUCT flags;
	CHDLC_DATA_RX_STATUS_EL_STRUCT rxbuf;
	struct sk_buff *skb;
	unsigned len;
	unsigned addr;
	void *buf;
	int i;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);

	if (!fr_prot){
		printk(KERN_INFO "%s: Fr prot area not allocated!\n",
				card->devname);
		goto rx_exit;
	}

	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
	card->hw_iface.peek(card->hw, card->u.c.rxmb_off, &rxbuf, sizeof(rxbuf));
	addr = rxbuf.ptr_data_bfr;
	if (rxbuf.opp_flag != 0x01) {
		printk(KERN_INFO 
			"%s: corrupted Rx buffer @ 0x%lX, flag = 0x%02X!\n", 
			card->devname, card->u.c.rxmb_off, rxbuf.opp_flag);
                printk(KERN_INFO "Code name: ");
                for(i = 0; i < 4; i ++)
                        printk(KERN_INFO "%c",
                                flags.global_info_struct.codename[i]);
		printk(KERN_INFO "\n");
                printk(KERN_INFO "Code version: ");
                for(i = 0; i < 4; i ++)
                        printk(KERN_INFO "%c",
                                flags.global_info_struct.codeversion[i]);
                printk(KERN_INFO "\n");

		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it measn that driver 
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash. 
                 */
		printk(KERN_INFO "%s: Critical router failure ...!!!\n", 
				card->devname);
		printk(KERN_INFO "Please contact Sangoma Technologies !\n");
		hdlc_set_intr_mode(card,0);	
		return;
	}

	if (rxbuf.error_flag){	
		printk(KERN_INFO "%s: Rx bad frame: error_flag=%i : len=%i\n",
				card->devname,rxbuf.error_flag, rxbuf.frame_length);
		goto rx_exit_kick;
	}
	
	
	if ((rxbuf.frame_length < (4+CRC_LENGTH)) || 
	    (rxbuf.frame_length > (card->wandev.mtu+CRC_LENGTH))){
		printk(KERN_INFO "%s: Rx bad frame: invalid length : len=%i\n",
				card->devname,rxbuf.frame_length);
		goto rx_exit_kick;
	}	

	/* Take off two CRC bytes */
	len = rxbuf.frame_length - CRC_LENGTH;
	
	/* Allocate socket buffer */
	skb = skb_dequeue(&fr_prot->rx_free);
	if (skb == NULL) {
		if (net_ratelimit()){
			printk(KERN_INFO "%s: FR skb rx queue, Qlen=%i, is empty: no rx buffers available!\n",
						card->devname,MAX_FR_RX_BUF);
			
			printk(KERN_INFO "%s: The rx queue should be increased via config file!\n",
						card->devname);

			printk(KERN_INFO "%s: TQ Critical 0x%lX\n",
					card->devname,fr_prot->tq_working);
		}
		++card->wandev.stats.rx_dropped;
		goto rx_exit_kick;
	}

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
	wan_skb_reset_mac_header(skb);

#ifdef RX_PKT_DEBUG
	if (skb->len <= 7 && !(skb->data[3]&0x01)){
		int x;

		if (skb->len == 7 && (skb->data[6]&0x01))
			goto rx_skip_error;

		DEBUG_EVNET("ERROR !!! RX ISR: Annexg Rx Frame received %i : orig %i\n",
				skb->len,(rxbuf.frame_length-CRC_LENGTH));

		printk(KERN_INFO "Bad Packet : ");
		for (x=0;x<skb->len;x++){
			printk("%X ",skb->data[x]);
		}
		printk("\n");
		printk(KERN_INFO "\n");

		printk(KERN_INFO "Element=%x Curr Ptr %x  Len=%i  Top %x\n",
				card->u.c.rxmb_off,
				(u32)addr,rxbuf.frame_length,
				(u32)card->u.c.rx_top_off);
		
		printk(KERN_INFO "ERROR WE ARE DEAD !!!\n");
		
		hdlc_set_intr_mode(card,0);
		goto rx_exit;

rx_skip_error:
	}
#endif
	
	skb_queue_tail(&fr_prot->rx_used,skb);

rx_exit_kick:
	if (!test_and_set_bit(0,&fr_prot->tq_working)){
		tasklet_schedule(&fr_prot->wanpipe_task);
	}
	
rx_exit:
	/* Release buffer element and calculate a pointer to the next one */
	rxbuf.opp_flag = 0x00;

	card->hw_iface.poke_byte(card->hw, 
			card->u.c.rxmb_off+offsetof(CHDLC_DATA_RX_STATUS_EL_STRUCT, opp_flag),
		        rxbuf.opp_flag);

	card->u.c.rxmb_off += sizeof(rxbuf);
	if (card->u.c.rxmb_off > card->u.c.rxbuf_last_off){
		card->u.c.rxmb_off = card->u.c.rxbuf_base_off;
	}
}

static netdevice_t * move_dev_to_next (sdla_t *card, netdevice_t *dev)
{
	struct wan_dev_le	*devle;
	
	if (!dev){
		return dev;
	}
	
	if (card->wandev.new_if_cnt == 1){
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

static void tx_intr (sdla_t *card)
{
	fr_prot_t *fr_prot=FR_PROT_AREA(card);
	struct sk_buff *skb;
	netdevice_t *dev;
	int i=0,no_dev_busy=0;
	
	if (test_bit(SEND_CRIT,&card->wandev.critical)){
		return;
	}

	set_bit(SEND_TXIRQ_CRIT,&card->wandev.critical);
	while ((skb=skb_dequeue(&fr_prot->lmi_queue))!=NULL){
		if (hdlc_send(card, skb->data, skb->len)){
			skb_queue_head(&fr_prot->lmi_queue,skb);

			/* We do not clear the critical flag because
			 * we want to stop the if_send() from
			 * attempting to tx. Since we have to
			 * tx the fr stuff first */
			return;
		}
		//card->wandev.stats.tx_aborted_errors++;
		dev_kfree_skb_any(skb);
	}
	clear_bit(SEND_TXIRQ_CRIT,&card->wandev.critical);

	if (fr_prot->tx_dev == NULL){
		fr_prot->tx_dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	}
	
	dev = fr_prot->tx_dev;
	if (!dev){
		printk(KERN_INFO "%s: No dev in tx intr\n",card->devname);
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
		return;
	}	
		
	for (;;){

		if (!dev || !(dev->flags & IFF_UP)){
			goto tx_skip;
		}

		if (is_queue_stopped(dev)){	
			wake_net_dev(dev);
		
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			{
			fr_private_area_t *chan=dev->priv;
			if (chan && chan->common.usedby == ANNEXG && chan->annexg_dev){
				if (IS_FUNC_CALL(lapb_protocol,lapb_mark_bh))
					lapb_protocol.lapb_mark_bh(chan->annexg_dev);
			}
			}
#endif			
			dev=move_dev_to_next(card,dev);
			break;
		}

tx_skip:
		dev=move_dev_to_next(card,dev);
		if (++i >= card->wandev.new_if_cnt){
			no_dev_busy=1;
			break;
		}
	}

	fr_prot->tx_dev = dev;
	
	if (no_dev_busy){
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
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
	
	if (test_bit(SEND_CRIT,&card->wandev.critical)){
		return;
	}

#ifdef TE1_56_CARD_SUPPORT
	/* TE timer interrupt */
	if (card->u.c.timer_int_enabled & TMR_INT_ENABLED_TE) {
		DEBUG_EVENT("%s: TE Polling\n",card->devname);
		card->wandev.fe_iface.polling(&card->fe);
		card->u.c.timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}
#endif

	/* only disable the timer interrupt if there are no udp or statistic */
	/* updates pending */
        if(!card->u.c.timer_int_enabled) {
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
        }
}

/*------------------------------------------------------------------------------
  Miscellaneous Functions
	- set_hdlc_config() used to set configuration options on the board
------------------------------------------------------------------------------*/

static int set_hdlc_config(sdla_t* card)
{

	CHDLC_CONFIGURATION_STRUCT cfg;

	memset(&cfg, 0, sizeof(CHDLC_CONFIGURATION_STRUCT));

	if(card->wandev.clocking)
		cfg.baud_rate = card->wandev.bps;

	cfg.line_config_options = (card->wandev.electrical_interface == WANOPT_RS232) ?
		INTERFACE_LEVEL_RS232 : INTERFACE_LEVEL_V35;

	cfg.modem_config_options	= 0;
	//API OPTIONS
	cfg.CHDLC_API_options		= DISCARD_RX_ERROR_FRAMES;
	cfg.modem_status_timer		= 100;
	cfg.CHDLC_protocol_options	= HDLC_STREAMING_MODE;
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
	
	return hdlc_configure(card, &cfg);
}

/*============================================================================
 * Process global exception condition
 */
static int process_global_exception(sdla_t *card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int err;

	mbox->wan_data_len = 0;
	mbox->wan_command = READ_GLOBAL_EXCEPTION_CONDITION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);

	if(err != CMD_TIMEOUT ){
	
		switch(mbox->wan_return_code) {
         
	      	case EXCEP_MODEM_STATUS_CHANGE:


#ifdef TE1_56_CARD_SUPPORT
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

				fr_handle_front_end_state(card);
				break;
			
			}
			
			if (IS_TE1_CARD(card)) {

				/* TE1 T1/E1 interrupt */
				card->wandev.fe_iface.isr(&card->fe);
				fr_handle_front_end_state(card);
				break;
			}	
#endif

			if (mbox->wan_data[0] & DCD_HIGH){
				card->fe.fe_status = FE_CONNECTED;
			}else{
				card->fe.fe_status = FE_DISCONNECTED;
			}
			
			printk(KERN_INFO "%s: Modem status change\n",
				card->devname);

			switch(mbox->wan_data[0] & (DCD_HIGH | CTS_HIGH)) {
				case (DCD_HIGH):
					printk(KERN_INFO "%s: DCD high, CTS low\n",card->devname);
					break;
				case (CTS_HIGH):
                                        printk(KERN_INFO "%s: DCD low, CTS high\n",card->devname);
                                        break;
                                case ((DCD_HIGH | CTS_HIGH)):
                                        printk(KERN_INFO "%s: DCD high, CTS high\n",card->devname);
                                        break;
				default:
                                        printk(KERN_INFO "%s: DCD low, CTS low\n",card->devname);
                                        break;
			}
			fr_handle_front_end_state(card);
			break;

                case EXCEP_TRC_DISABLED:
                        printk(KERN_INFO "%s: Line trace disabled\n",
				card->devname);
                        break;

		case EXCEP_IRQ_TIMEOUT:
			printk(KERN_INFO "%s: IRQ timeout occurred\n",
				card->devname); 
			break;

		case 0x16:
			break;

                default:
                        printk(KERN_INFO "%s: Global exception %x\n",
				card->devname, mbox->wan_return_code);
                        break;
                }
	}
	return 0;
}


/*============================================================================
 * Process fr exception condition
 */
static int process_hdlc_exception(sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_EXCEPTION_CONDITION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(err != CMD_TIMEOUT) {
	
		switch (err) {

		case EXCEP_LINK_ACTIVE:
			card->fe.fe_status = FE_CONNECTED;
			fr_handle_front_end_state (card);
			break;

		case EXCEP_LINK_INACTIVE_MODEM:
			card->fe.fe_status = FE_DISCONNECTED;
			fr_handle_front_end_state (card);
			break;

		case EXCEP_LOOPBACK_CONDITION:
			printk(KERN_INFO "%s: Loopback Condition Detected.\n",
						card->devname);
			break;

		case NO_CHDLC_EXCEP_COND_TO_REPORT:
			printk(KERN_INFO "%s: No exceptions reported.\n",
						card->devname);
			break;
		default:
			printk(KERN_INFO "%s: Exception Condition %x!\n",
					card->devname,err);
			break;
		}

	}
	return 0;
}


/*=============================================================================
 * Store a UDP management packet for later processing.
 */
#if 0
static int store_udp_mgmt_pkt(char udp_pkt_src, sdla_t* card,
                                struct sk_buff *skb, netdevice_t* dev,
                                fr_private_area_t* chan )
{
	int udp_pkt_stored = 0;

	if(!chan->udp_pkt_lgth &&
	  (skb->len <= MAX_LGTH_UDP_MGNT_PKT)) {
        	chan->udp_pkt_lgth = skb->len;
		chan->udp_pkt_src = udp_pkt_src;
       		memcpy(chan->udp_pkt_data, skb->data, skb->len);
		card->u.c.timer_int_enabled = TMR_INT_ENABLED_UDP;
		udp_pkt_stored = 1;
	}

	if(udp_pkt_src == UDP_PKT_FRM_STACK)
		dev_kfree_skb_any(skb);
	else
                dev_kfree_skb_any(skb);
	
	return(udp_pkt_stored);
}
#endif

static void fr_handle_front_end_state (void* card_id)
{
	struct wan_dev_le	*devle;
	sdla_t			*card = (sdla_t*)card_id;
	netdevice_t		*dev;
	fr_private_area_t	*chan;

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
		chan = wan_netif_priv(dev);
		if (card->fe.fe_status == FE_CONNECTED){ 
			if (chan->dlci_state & PVC_STATE_ACTIVE &&
			    card->wandev.state == WAN_CONNECTED &&
			    chan->common.state != WAN_CONNECTED){
				chan_set_state(dev,WAN_CONNECTED);
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

				chan_set_state(dev,WAN_DISCONNECTED);
			}
		}
	}
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



/*==============================================================================
 * Process UDP call of type FPIPE8ND
 */
static int process_udp_mgmt_pkt(sdla_t* card, void *local_dev)
{
	unsigned char frames;
	unsigned int len;
	unsigned short buffer_length;
	wan_mbox_t *mbox = &card->wan_mbox;
	int err;
	struct timeval tv;
	int udp_mgmt_req_valid = 1;
        netdevice_t* dev;
        fr_channel_t* chan;
        wan_udp_pkt_t *wan_udp_pkt;
	fpipemon_trc_t* fpipemon_trc;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	SHARED_MEMORY_INFO_STRUCT flags;
	int dlci = 0;
	int orig_cmd=0;

	if (local_dev == NULL){

		return 0;
#if 0
		/* Find network interface for this packet */
		dev = find_channel(card, dlci);
		if (!dev){
			card->u.f.udp_pkt_lgth = 0;
			return -ENODEV;
		}
		if ((chan = dev->priv) == NULL){
			card->u.f.udp_pkt_lgth = 0;
			return -ENODEV;
		}

		/* If the UDP packet is from the network, we are going to have to 
		   transmit a response. Before doing so, we must check to see that
		   we are not currently transmitting a frame (in 'if_send()') and
		   that we are not already in a 'delayed transmit' state.
		*/
		if(udp_pkt_src == UDP_PKT_FRM_NETWORK) {
			if (check_tx_status(card,dev)){
				card->u.f.udp_pkt_lgth = 0;
				return -EBUSY;
			}
		}

		wan_udp_pkt = (wan_udp_pkt_t *)card->u.f.udp_pkt_data;

		
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
				case WAN_FE_GET_STAT:
					udp_mgmt_req_valid = 1;
					break;
				default:
					udp_mgmt_req_valid = 0;
					break;
			}
		}
#endif
	}else{
		dev = (netdevice_t *) local_dev;
		if ((chan = dev->priv) == NULL){
			return -ENODEV;
		}
		dlci=chan->dlci;	
		
		if (atomic_read(&chan->udp_pkt_len) == 0){
			return -ENODEV;
		}
		
		wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;
		udp_mgmt_req_valid=1;
	}

	//debug_print_udp_pkt((u8*)wan_udp_pkt,sizeof(wan_udp_pkt_t),0,1);
	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
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

		struct sk_buff *skb;
		struct sk_buff_head *trace_queue;

		switch(wan_udp_pkt->wan_udp_command) {

		case FR_READ_CONFIG:
		case FR_READ_CODE_VERSION:
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len=0;
			break;
		
		case FR_LIST_ACTIVE_DLCI:
			{
			unsigned short *data_buf = (unsigned short*)wan_udp_pkt->wan_udp_data;
			int ch,data_len=0;
			
			for (ch=0;ch<MAX_FR_CHANNELS;ch++){
				
				if (!fr_prot->global_dlci_map[ch])
					continue;
				
				if (fr_prot->global_dlci_map[ch] & PVC_STATE_ACTIVE){
					*data_buf=ch;
					data_buf ++;
					data_len +=2;
				}
			}
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len = data_len;
			}
			break;
		
		case FR_READ_STATUS:
			{
			unsigned char *data_buf = (unsigned char*)wan_udp_pkt->wan_udp_data;
			int ch, data_len=0;

			*data_buf = (card->wandev.state == WAN_CONNECTED) ? 1:0;
			data_buf++;
			data_len+=1;

			for (ch=0;ch<MAX_FR_CHANNELS;ch++){
				
				if (!fr_prot->global_dlci_map[ch])
					continue;
				
				*(unsigned short*)data_buf=ch;
				data_buf +=2;
				*data_buf = (fr_prot->global_dlci_map[ch] | 0x40);
				data_buf++;
				data_len += 3;
			}
			
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len = data_len;
			}
			break;

		case FR_READ_STATISTICS:

			memcpy(wan_udp_pkt->wan_udp_data,&fr_prot->link_stats,sizeof(fr_link_stat_t));
			wan_udp_pkt->wan_udp_data_len = sizeof(fr_link_stat_t);
			wan_udp_pkt->wan_udp_return_code = 0;
			break;
			

		case FPIPE_ENABLE_TRACING:
		
			wan_udp_pkt->wan_udp_return_code = 0;
		
			if (wan_udp_pkt->wan_udp_data[0] & TRC_ACTIVE){
				
				if (!test_bit(0,&chan->tracing_enabled)){
					
					chan->stats.tx_fifo_errors=0;
					chan->stats.rx_over_errors=0;
					chan->trace_timeout=jiffies;
					
					skb_queue_purge(&chan->trace_queue);
					
					set_bit (0,&chan->tracing_enabled);
					printk(KERN_INFO "%s: %s: Frame Relay DLCI=%i trace enabled!\n",
							card->devname,chan->if_name,chan->dlci);
				}else{
					printk(KERN_INFO "%s: %s: Error: Frame Relay DLCI=%i trace running!\n",
							card->devname,chan->if_name,chan->dlci);
					wan_udp_pkt->wan_udp_return_code = 2;
				}
				
			}else if (wan_udp_pkt->wan_udp_data[0] & (TRC_SIGNALLING_FRMS|TRC_INFO_FRMS)){

				if (test_bit(0,&chan->tracing_enabled)){
					printk(KERN_INFO "\n");
					printk(KERN_INFO "%s: %s: Error: FR DLCI=%i trace running!\n",
							card->devname,chan->if_name,chan->dlci);
					wan_udp_pkt->wan_udp_return_code = 2;
				
				}else if (test_bit(1,&chan->tracing_enabled)){

					printk(KERN_INFO "\n");
					printk(KERN_INFO "%s: Error: Frame Relay LMI trace running!\n",
							card->devname);
					wan_udp_pkt->wan_udp_return_code = 1;
					
				}else if(!test_bit(0,&fr_prot->tracing_enabled)){

					skb_queue_purge(&fr_prot->trace_queue);
					fr_prot->trace_timeout=jiffies;
					
					set_bit (1,&chan->tracing_enabled);
					set_bit (0,&fr_prot->tracing_enabled);
					printk(KERN_INFO "%s: Frame Relay LMI trace enabled!\n",
							card->devname);
				}else{
					printk(KERN_INFO "\n");
					printk(KERN_INFO "%s: %s: Error: FR LMI trace running!\n",
							card->devname,chan->if_name);
					wan_udp_pkt->wan_udp_return_code = 2;
				}
				
			}else{
				printk(KERN_INFO "%s: %s: Invalid trace command options!\n",
						card->devname,chan->if_name);
				wan_udp_pkt->wan_udp_return_code = 1;
			}
		
			if (wan_udp_pkt->wan_udp_return_code == 0){
				chan->stats.rx_fifo_errors=0;
				chan->stats.tx_fifo_errors=0;
			}
		
			wan_udp_pkt->wan_udp_data_len=0;
			break;


                case FPIPE_DISABLE_TRACING:
			
			wan_udp_pkt->wan_udp_return_code = 0;
			
			if(test_bit(0,&chan->tracing_enabled)) {
				
				clear_bit(0,&chan->tracing_enabled);
                    		skb_queue_purge(&chan->trace_queue);
				printk(KERN_INFO "%s: %s: Disabling FR DLCI=%i trace\n",
						card->devname,chan->if_name, chan->dlci);
				
			}else if (test_bit(1,&chan->tracing_enabled)){
				
				clear_bit(1,&chan->tracing_enabled);
				clear_bit(0,&fr_prot->tracing_enabled);
				skb_queue_purge(&fr_prot->trace_queue);
				printk(KERN_INFO "%s: %s: Disabling FR LMI trace\n",
						card->devname,chan->if_name);
			
			}else{
				/* set return code to line trace already 
				   disabled */
				//printk(KERN_INFO "%s: %s: Frame Relay trace already disabled!\n",
				//			card->devname,chan->if_name);
				wan_udp_pkt->wan_udp_return_code = 1;
			}

                    	/* set return code */
			wan_udp_pkt->wan_udp_data_len=0;
			break;

                case FPIPE_GET_TRACE_INFO:

                        if(test_bit(0,&chan->tracing_enabled)){
				trace_queue=&chan->trace_queue;
				chan->trace_timeout=jiffies;
			}else if (test_bit(1,&chan->tracing_enabled)){
				trace_queue=&fr_prot->trace_queue;
				fr_prot->trace_timeout=jiffies;
			}else{
				printk(KERN_INFO "%s: %s: Error FR trace not enabled\n",
						card->devname,chan->if_name);
                                /* set return code */
                                wan_udp_pkt->wan_udp_return_code = 1;
				wan_udp_pkt->wan_udp_data_len=0;
                                break;
                        }

			frames=0;
                        buffer_length = 0;
			wan_udp_pkt->wan_udp_data[0x00] = 0x00;

			while ((skb=skb_dequeue(trace_queue)) != NULL){

				fr_trc_el_t *trc_el = (fr_trc_el_t*)skb->data;
	
				if (trc_el->length > skb->len){
					/* The frame is invalid or corrupted,
					 * drop it */
					dev_kfree_skb_any(skb);
					continue;
				}
				
				if((trc_el->length + sizeof(fpipemon_trc_hdr_t) + 1) >
					(MAX_TRACE_BUFFER - buffer_length)){
					wan_udp_pkt->wan_udp_data[0x00] |= MORE_TRC_DATA;
					skb_queue_head(trace_queue,skb);
					break;
				}
				
				fpipemon_trc = 
					(fpipemon_trc_t *)&wan_udp_pkt->wan_udp_data[buffer_length]; 
				fpipemon_trc->fpipemon_trc_hdr.status =
					trc_el->attr;
                            	fpipemon_trc->fpipemon_trc_hdr.tmstamp =
					trc_el->tmstamp;
                            	fpipemon_trc->fpipemon_trc_hdr.length = 
					trc_el->length;

                                if(!trc_el->length) {

                                     	fpipemon_trc->fpipemon_trc_hdr.data_passed = 0x00;
					
                                }else {
                                        fpipemon_trc->fpipemon_trc_hdr.data_passed = 0x01;
					memcpy(fpipemon_trc->data, (skb->data+sizeof(fr_trc_el_t)), trc_el->length);
				}			

				buffer_length += sizeof(fpipemon_trc_hdr_t);
                               	if(fpipemon_trc->fpipemon_trc_hdr.data_passed) {
                               		buffer_length += trc_el->length;
                               	}

				dev_kfree_skb_any(skb);

				if (++frames >= MAX_FRMS_TRACED){
					break;
				}

				if(wan_udp_pkt->wan_udp_data[0x00] & MORE_TRC_DATA) {
					break;
				}
                        }
                      
			if(frames == MAX_FRMS_TRACED) {
                        	wan_udp_pkt->wan_udp_data[0x00] |= MORE_TRC_DATA;
			}
             
                        /* set the total number of frames passed */
			wan_udp_pkt->wan_udp_data[0x00] |=
				((frames << 1) & (MAX_FRMS_TRACED << 1));

                        /* set the data length and return code */
			wan_udp_pkt->wan_udp_data_len = buffer_length;
                        wan_udp_pkt->wan_udp_return_code = 0;
                        break;

                case FPIPE_FT1_READ_STATUS:
			((unsigned char *)wan_udp_pkt->wan_udp_data )[0] =
				flags.FT1_info_struct.parallel_port_A_input;

			((unsigned char *)wan_udp_pkt->wan_udp_data )[1] =
				flags.FT1_info_struct.parallel_port_B_input;
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len = 2;
			break;


		case FPIPE_FLUSH_DRIVER_STATS:
			init_chan_statistics(chan);
			init_global_statistics(card);
			wan_udp_pkt->wan_udp_data_len=0;
			break;
	
		case FPIPE_ROUTER_UP_TIME:
			do_gettimeofday( &tv );
			card->u.c.router_up_time = tv.tv_sec - 
					card->u.c.router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					card->u.c.router_up_time;	
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			break;
	
		case FPIPE_DRIVER_STAT_IFSEND:
			memcpy(wan_udp_pkt->wan_udp_data,
				&chan->drvstats_if_send.if_send_entry,
				sizeof(if_send_stat_t));
			wan_udp_pkt->wan_udp_data_len =sizeof(if_send_stat_t);	
			wan_udp_pkt->wan_udp_return_code = 0;
			break;
	
		case FPIPE_DRIVER_STAT_INTR:

			memcpy(wan_udp_pkt->wan_udp_data,
                                &card->statistics.isr_entry,
                                sizeof(global_stats_t));

                        memcpy(&wan_udp_pkt->wan_udp_data[sizeof(global_stats_t)],
                                &chan->drvstats_rx_intr.rx_intr_no_socket,
                                sizeof(rx_intr_stat_t));

			wan_udp_pkt->wan_udp_data_len = 
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

                        wan_udp_pkt->wan_udp_data_len = sizeof(global_stats_t)+
                                                     sizeof(rx_intr_stat_t);
			wan_udp_pkt->wan_udp_return_code = 0;
                        break;


		case FR_FT1_STATUS_CTRL:
			/* Enable FT1 MONITOR STATUS */
			mbox->wan_command = 0x1C;
	        	if ((wan_udp_pkt->wan_udp_data[0] & ENABLE_READ_FT1_STATUS) ||  
				(wan_udp_pkt->wan_udp_data[0] & ENABLE_READ_FT1_OP_STATS)) {
			
			     	if( rCount++ != 0 ) {
					wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
					wan_udp_pkt->wan_udp_data_len=1;
					mbox->wan_data_len = 1;
		  			break;
		    	     	}
	      		}

	      		/* Disable FT1 MONITOR STATUS */
	      		if( wan_udp_pkt->wan_udp_data[0] == 0) {

	      	   	     	if( --rCount != 0) {
		  			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
					wan_udp_pkt->wan_udp_data_len=1;
					mbox->wan_data_len = 1;
		  			break;
	   	    	     	} 
	      		} 	
			goto dflt_1;

		case FR_SET_FT1_MODE:
			mbox->wan_command = 0x1E;
			goto dflt_1;
			

#ifdef TE1_56_CARD_SUPPORT
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
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			break;

		case WAN_FE_GET_STAT:
		    	if (IS_TE1_CARD(card)) {	
	 	    		/* TE1_56K Read T1/E1/56K alarms */
			  	*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					card->wandev.fe_iface.read_alarm(
							&card->fe, 0);
				/* TE1 Update T1/E1 perfomance counters */
    				sdla_te_pmon(card);
	        		memcpy(&wan_udp_pkt->wan_udp_data[sizeof(unsigned long)],
					&card->wandev.te_pmon,
					sizeof(sdla_te_pmon_t));
		        	wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
		    		wan_udp_pkt->wan_udp_data_len = 
					sizeof(unsigned long) + sizeof(sdla_te_pmon_t); 
			}else if (IS_56K_CARD(card)){
				/* 56K Update CSU/DSU alarms */
				card->wandev.k56_alarm = sdla_56k_alarm(card, 1); 
			 	*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
			                        card->wandev.k56_alarm;
				wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
	    			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			}
		    	break;

 		case WAN_FE_FLUSH_PMON:
 	    		/* TE1 Flush T1/E1 pmon counters */
	    		if (IS_TE1_CARD(card)){	
				card->wandev.fe_iface.flush_pmon(&card->fe);
	        		wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
	    		}
			wan_udp_pkt->wan_udp_data_len=0;
	    		break;

		case WAN_FE_GET_CFG:
			/* Read T1/E1 configuration */
	    		if (IS_TE1_CARD(card)){	
        			memcpy(&wan_udp_pkt->wan_udp_data[0],
					&card->wandev.te_cfg,
					sizeof(sdla_te_cfg_t));
		        	wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
	    			wan_udp_pkt->wan_udp_data_len = sizeof(sdla_te_cfg_t);
			}
			break;
#endif

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

#endif			


		default:
			orig_cmd=wan_udp_pkt->wan_udp_command;
			switch(wan_udp_pkt->wan_udp_command){

				case FR_READ_MODEM_STATUS:
					mbox->wan_command = READ_MODEM_STATUS;
					break;

				case FR_READ_ERROR_STATS:
					mbox->wan_command = READ_COMMS_ERROR_STATS;
					break;

				case FR_FLUSH_ERROR_STATS:
					mbox->wan_command = FLUSH_COMMS_ERROR_STATS;
					break;
				
				default:
					mbox->wan_return_code=1;
					goto udp_cmd_done;

			}
dflt_1:
		
			/* it's a board command */
			mbox->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			if (mbox->wan_data_len) {
				memcpy(&mbox->wan_data, (unsigned char *) wan_udp_pkt->
							wan_udp_data, mbox->wan_data_len);
	      		} 

			/* run the command on the board */
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
			if (err != COMMAND_OK) {
				hdlc_error(card,err,mbox);
				break;
			}

			/* copy the result back to our buffer */
			memcpy(&wan_udp_pkt->wan_udp_hdr.wan_cmd, mbox, sizeof(wan_cmd_t));
			
			if (mbox->wan_data_len) {
	         		memcpy(&wan_udp_pkt->wan_udp_data, &mbox->wan_data, 
								mbox->wan_data_len); 
	      		}

			wan_udp_pkt->wan_udp_data_len=mbox->wan_data_len;
			wan_udp_pkt->wan_udp_return_code=err;
			
			if(!err)
				chan->drvstats_gen.
					UDP_PIPE_mgmt_adptr_cmnd_OK ++;
			else
                                chan->drvstats_gen.
					UDP_PIPE_mgmt_adptr_cmnd_timeout ++;

			
			switch(orig_cmd){

				case FR_READ_ERROR_STATS:
					{
					COMMS_ERROR_STATS_STRUCT *comm_stats = 
						(COMMS_ERROR_STATS_STRUCT *)mbox->wan_data;

					wan_udp_pkt->wan_udp_data[8]=comm_stats->DCD_state_change_count;
					wan_udp_pkt->wan_udp_data[9]=comm_stats->CTS_state_change_count;
					wan_udp_pkt->wan_udp_data_len=mbox->wan_data_len=10;
					wan_udp_pkt->wan_udp_return_code=0;
					}
					break;

			}
		} 
        }
	
udp_cmd_done:
        
	/* Fill UDP TTL */
        wan_udp_pkt->wan_ip_ttl = card->wandev.ttl;

	len=0;
	if (local_dev){
		len = reply_udp(chan->udp_pkt_data, wan_udp_pkt->wan_udp_data_len);
	}else{
        	//len = reply_udp(card->u.f.udp_pkt_data, mbox->wan_data_len);
	}

	//debug_print_udp_pkt((u8*)wan_udp_pkt,sizeof(wan_udp_pkt_t),0,0);

	if (local_dev){
		atomic_set(&chan->udp_pkt_len,len);
		return len;
	
	}
#if 0	
	else if(udp_pkt_src == UDP_PKT_FRM_NETWORK) {

		chan->fr_header_len=2;
		chan->fr_header[0]=chan->fr_encap_0;
		chan->fr_header[1]=chan->fr_encap_1;
			
		err = fr_send_data_header(card, dlci, 0, len, 
			card->u.f.udp_pkt_data,chan->fr_header_len);
		if (err){ 
			chan->drvstats_gen.UDP_PIPE_mgmt_adptr_send_passed ++;
		}else{
			chan->drvstats_gen.UDP_PIPE_mgmt_adptr_send_failed ++;
		}
		
	}else{
		/* Allocate socket buffer */
		if((new_skb = dev_alloc_skb(len)) != NULL) {

			/* copy data into new_skb */
			buf = skb_put(new_skb, len);
			memcpy(buf, card->u.f.udp_pkt_data, len);
        
			chan->drvstats_gen.
				UDP_PIPE_mgmt_passed_to_stack ++;
			new_skb->dev = dev;
			new_skb->protocol = htons(ETH_P_IP);
			wan_skb_reset_mac_header(new_skb);
			netif_rx(new_skb);
            	
		} else {
			chan->drvstats_gen.UDP_PIPE_mgmt_no_socket ++;
			printk(KERN_INFO 
			"%s: UDP mgmt cmnd, no socket buffers available!\n", 
			card->devname);
            	}
        }

	
	card->u.f.udp_pkt_lgth = 0;
#endif
	
	return len;

}


/*============================================================================
 * Initialize Receive and Transmit Buffers.
 */

static void init_hdlc_tx_rx_buff( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	unsigned long tx_config_off;
	unsigned long rx_config_off;
	CHDLC_TX_STATUS_EL_CFG_STRUCT	tx_config;
	CHDLC_RX_STATUS_EL_CFG_STRUCT 	rx_config;
	char err;
	
	mb->wan_data_len = 0;
	mb->wan_command = READ_CHDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		hdlc_error(card,err,mb);
		return;
	}

	/* ALEX Apr 8 2004 Sangoma ISA card */
	tx_config_off =
                ((CHDLC_CONFIGURATION_STRUCT *)mb->wan_data)->
			ptr_CHDLC_Tx_stat_el_cfg_struct;
       	rx_config_off =
		((CHDLC_CONFIGURATION_STRUCT *)mb->wan_data)->
			ptr_CHDLC_Rx_stat_el_cfg_struct;

      		/* Setup Head and Tails for buffers */
	card->hw_iface.peek(card->hw, tx_config_off, &tx_config, sizeof(tx_config));
	card->hw_iface.peek(card->hw, rx_config_off, &rx_config, sizeof(rx_config));
       	card->u.c.txbuf_base_off =
                tx_config.base_addr_Tx_status_elements;
       	card->u.c.txbuf_last_off = 
               	card->u.c.txbuf_base_off +
		(tx_config.number_Tx_status_elements - 1) * 
			sizeof(CHDLC_DATA_TX_STATUS_EL_STRUCT);
	card->hw_iface.peek(card->hw, tx_config_off, &tx_config, sizeof(tx_config));
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
static int intr_test( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err,i;

	card->timer_int_enabled = 0;

	/* The critical flag is unset because during intialization (if_open) 
	 * we want the interrupts to be enabled so that when the wpc_isr is
	 * called it does not exit due to critical flag set.
	 */ 

	err = hdlc_set_intr_mode(card, APP_INT_ON_COMMAND_COMPLETE);

	if (err == CMD_OK) { 
		for (i = 0; i < MAX_INTR_TEST_COUNTER; i ++) {	
			mb->wan_data_len  = 0;
			mb->wan_command = READ_CHDLC_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
		}
	}else{
		return err;
	}

	err = hdlc_set_intr_mode(card, 0);

	if (err != CMD_OK)
		return err;

	return 0;
}

/*==============================================================================
 * Determine what type of UDP call it is. FPIPE8ND ?
 */
#if 0
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
	
        if((wan_udp_pkt->ip_pkt.protocol == UDPMGMT_UDP_PROTOCOL) &&
		(wan_udp_pkt->ip_pkt.ver_inet_hdr_length == 0x45) &&
		(wan_udp_pkt->udp_pkt.udp_dst_port == 
		ntohs(card->wandev.udp_port)) &&
		(wan_udp_pkt->wp_mgmt.request_reply == 
		UDPMGMT_REQUEST)) {
                        if(!strncmp(wan_udp_pkt->wp_mgmt.signature,
                                UDPMGMT_FPIPE_SIGNATURE, 8)){
                                return UDP_FPIPE_TYPE;
			}
	}
        return UDP_INVALID_TYPE;
}
#endif

/*============================================================================
 * Set PORT state.
 */
static void chan_set_state (netdevice_t *dev, int state)
{
	fr_private_area_t *chan;
	sdla_t *card;

	if (!dev || !dev->priv){
		return;
	}

	chan=dev->priv;
	card = chan->card;
	
        if (chan->common.state != state){
	
		chan->common.state = state;
		
                switch (state){
			
                case WAN_CONNECTED:
                        printk (KERN_INFO "%s: %s: DLCI %i: link connected!\n",
                                card->devname,chan->if_name,chan->dlci);
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			if (chan->common.usedby == ANNEXG && chan->annexg_dev){
				if (IS_FUNC_CALL(lapb_protocol,lapb_link_up))
					lapb_protocol.lapb_link_up(chan->annexg_dev);
			}
#endif	
                      break;

                case WAN_CONNECTING:
                        printk (KERN_INFO "%s: %s: DLCI %i: link connecting...\n",
                                card->devname,chan->if_name,chan->dlci);
                        break;

                case WAN_DISCONNECTED:
                        printk (KERN_INFO "%s: %s: DLCI %i: link disconnected!\n",
                                card->devname,chan->if_name,chan->dlci);

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			if (chan->common.usedby == ANNEXG && chan->annexg_dev){
				if (IS_FUNC_CALL(lapb_protocol,lapb_link_down))
					lapb_protocol.lapb_link_down(chan->annexg_dev);
			}
#endif
			break;
                }
        }
}

/*===========================================================================
 * config_hdlc
 *
 *	Configure the fr protocol and enable communications.		
 *
 *   	The if_open() function binds this function to the poll routine.
 *      Therefore, this function will run every time the fr interface
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

static int config_hdlc (sdla_t *card)
{
	
	if (card->u.c.comm_enabled){
		hdlc_comm_disable(card);
		wanpipe_set_state(card, WAN_DISCONNECTED);
	}

	if (set_hdlc_config(card)) {
		printk(KERN_INFO "%s: CHDLC Configuration Failed!\n",
				card->devname);
		return -EIO;
	}

#ifdef TE1_56_CARD_SUPPORT
	
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
		/* Run rest of initialization not from lock */
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
		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}
		
#endif
	
	/* Set interrupt mode and mask */
        if (hdlc_set_intr_mode(card, APP_INT_ON_RX_FRAME |
                		APP_INT_ON_GLOBAL_EXCEP_COND |
                		APP_INT_ON_TX_FRAME |
                		APP_INT_ON_CHDLC_EXCEP_COND | APP_INT_ON_TIMER)){
		printk (KERN_INFO "%s: Failed to set interrupt triggers!\n",
				card->devname);
		return -EIO;	
        }
	

	/* Mask All interrupts  */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, 
		(APP_INT_ON_RX_FRAME | APP_INT_ON_TX_FRAME | 
		 APP_INT_ON_TIMER    | APP_INT_ON_GLOBAL_EXCEP_COND | 
		 APP_INT_ON_CHDLC_EXCEP_COND));

	if (hdlc_comm_enable(card) != 0) {
		printk(KERN_INFO "%s: Failed to enable fr communications!\n",
				card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
		card->u.c.comm_enabled=0;
		hdlc_set_intr_mode(card,0);
		return -EIO;
	}

	init_hdlc_tx_rx_buff(card);

#ifdef TE1_56_CARD_SUPPORT	
	/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1);
	}
#endif

	/* Unmask all interrupts except the Transmit and Timer interrupts */
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, 
		(APP_INT_ON_RX_FRAME | APP_INT_ON_GLOBAL_EXCEP_COND | 
		APP_INT_ON_CHDLC_EXCEP_COND));
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);


	printk(KERN_INFO "%s: Enabling data link layer!\n",card->devname);
	return 0; 
}


/*
 * ******************************************************************
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
	fr_private_area_t*	chan = priv;
	sdla_t*			card = NULL;

	if (chan == NULL){
		return m->count;
	}else{
		card = chan->card;

		if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
			PROC_ADD_LINE(m,
				"%s", fr_config_hdr);
		}
		
		PROC_ADD_LINE(m,
			PROC_CFG_FRM, chan->if_name, card->devname, chan->dlci);
		
	}
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
		PROC_STAT_FRM, chan->if_name, card->devname, STATE_DECODE(chan->common.state));
	return m->count;
}


#define PROC_DEV_FR_S_FRM	"%-20s| %-14s|\n"
#define PROC_DEV_FR_D_FRM	"%-20s| %-14d|\n"
#define PROC_DEV_SEPARATE	"=====================================\n"

			/* DevName, DLCI, DlciState, DlciNewS, GDlciState, DevState */ 
#define PROC_DEV_FR_SDDD_TITLE_FRM    	"%-12s| %-5s | %-6s | %-7s | %-8s | %-13s |\n"
#define PROC_DEV_FR_SDDD_FRM    	"%-12s| %-5d | %-6d | %-7d | %-8d | %-13s |\n"

#define PROC_DEV_SEPARATE1	"===================================================================\n"

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
	netdevice_t*		dev = (void*)data;
	fr_private_area_t* 	chan = NULL;
	sdla_t*			card = NULL;

	if (dev == NULL || dev->priv == NULL)
		return count;
	chan = (fr_private_area_t*)dev->priv;
	if (chan->card == NULL)
		return count;
	card = chan->card;

	printk(KERN_INFO "%s: New interface config (%s)\n",
			chan->if_name, buffer);
	/* Parse string */

	return count;
}


static void s508_s514_lock(sdla_t *card, unsigned long *smp_flags)
{
	if (card->type != SDLA_S514){

		spin_lock_irqsave(&card->wandev.lock, *smp_flags);
	}else{
		spin_lock(&card->u.c.if_send_lock);
	}
	return;
}

static void s508_s514_unlock(sdla_t *card, unsigned long *smp_flags)
{
	if (card->type != SDLA_S514){

		spin_unlock_irqrestore (&card->wandev.lock, *smp_flags);
	}else{
		spin_unlock(&card->u.c.if_send_lock);
	}
	return;
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


/* =============== Frame Relay Protocol ============================ */


static int fr_hard_header(sdla_t *card, netdevice_t *dev, u16 type)
{
	fr_private_area_t *chan = dev->priv;
	
	if (!chan)
		return -ENODEV;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (chan->common.usedby == ANNEXG){
		dlci_to_q922(chan->header, chan->dlci);
		chan->hdr_len = 2;
		return chan->hdr_len;
	}
#endif

	switch(type) {
		
	case ETH_P_IP:
		chan->hdr_len = 4;
		chan->header[3]=NLPID_IP;
		break;

	case ETH_P_IPV6:
		chan->hdr_len = 4;
		chan->header[3]=NLPID_IPV6;
		break;

	default:
		chan->hdr_len = 10;
		chan->header[3] = FR_PAD;
		chan->header[4] = NLPID_SNAP;
		chan->header[5] = FR_PAD;
		chan->header[6] = FR_PAD;
		chan->header[7] = FR_PAD;
		chan->header[8] = type>>8;
		chan->header[9] = (u8)type;
	}

	dlci_to_q922(chan->header, chan->dlci);
	chan->header[2] = FR_UI;

	return chan->hdr_len;
}


static __inline__ void dlci_to_status(sdla_t *card, u16 dlci, u8 *status,
				      u8 state)
{
	status[0] = (dlci>>4) & 0x3F;
	status[1] = ((dlci<<3) & 0x78) | 0x80;
	status[2] = 0x80;

	if (state & PVC_STATE_NEW)
		status[2] |= 0x08;
	else if (state & PVC_STATE_ACTIVE)
		status[2] |= 0x02;
}


static inline void fr_log_dlci_active(fr_private_area_t *chan)
{
	printk(KERN_INFO "%s: %s: DLCI %i: %sactive%s\n", 
	       ((sdla_t*)chan->card)->devname,
	       chan->if_name,
	       chan->dlci,
	       chan->dlci_state & PVC_STATE_ACTIVE ? "" : "in",
	       chan->dlci_state & PVC_STATE_NEW ? " new" : "");
}

static void fr_timer(unsigned long arg)
{
	sdla_t *card = (sdla_t*)arg;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	int i, cnt = 0, reliable;
	netdevice_t *dev;
	fr_private_area_t *chan;
	u32 list;

	
//	printk(KERN_INFO "FR TIMER FR:State=0x%X LastErrors=0x%X\n",
//			fr_prot->state,fr_prot->last_errors);

	
	if (fr_prot->station == WANOPT_NODE){
		reliable = ((jiffies - fr_prot->last_rx_poll) < (fr_prot->cfg.t392*HZ));
		if (!reliable){
			fr_prot->link_stats.T392_timeouts++;
		}
	}else{
		fr_prot->last_errors <<= 1; /* Shift the list */
		
		if (fr_prot->state & LINK_STATE_REQUEST) {
			printk(KERN_INFO "%s: Frame Relay: No LMI status reply received\n",
			       card->devname);
			fr_prot->last_errors |= 1;
			fr_prot->link_stats.T391_timeouts++;
		}

		for (i = 0, list = fr_prot->last_errors; i < fr_prot->cfg.n393;
		     i++, list >>= 1){
			cnt += (list & 1);	/* errors count */
		}
		reliable = (cnt < fr_prot->cfg.n392);
	}

	if ((fr_prot->state & LINK_STATE_RELIABLE) != (reliable ? LINK_STATE_RELIABLE : 0)){ 
		struct wan_dev_le	*devle;

		WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev || !wan_netif_priv(dev))
				continue;
			if (!(dev->flags & IFF_UP))
				continue;
			chan = wan_netif_priv(dev);
			chan->dlci_state &= ~(PVC_STATE_NEW | PVC_STATE_ACTIVE);
			chan_set_state(dev,WAN_DISCONNECTED);
		}
		
		fr_prot->state ^= LINK_STATE_RELIABLE;
	
		if (reliable){
			wanpipe_set_state(card,WAN_CONNECTED);
		}else{
			printk(KERN_INFO "%s: Link unreliable !\n",
					card->devname);
			wanpipe_set_state(card,WAN_DISCONNECTED);
		}
		
		//printk(KERN_INFO "%s: Link %sreliable\n", card->devname,
		//     reliable ? "" : "un");

		if (reliable) {
			fr_prot->n391cnt = 0; /* Request full status */
			fr_prot->state |= LINK_STATE_CHANGED;
		}
	}

	if (fr_prot->station == WANOPT_NODE){
		fr_prot->timer.expires = jiffies + fr_prot->cfg.t392*HZ;
	}else{
		if (fr_prot->n391cnt)
			fr_prot->n391cnt--;

		fr_lmi_send(card, (fr_prot->n391cnt == 0));

		fr_prot->state |= LINK_STATE_REQUEST;
		fr_prot->timer.expires = jiffies + (fr_prot->cfg.t391*HZ);
	}

	add_timer(&fr_prot->timer);
}



static void fr_lmi_send(sdla_t *card, int fullrep)
{
	struct sk_buff *skb;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	netdevice_t *dev;
	fr_private_area_t *chan;
	int len = (fr_prot->cfg.signalling == WANOPT_FR_ANSI) ? LMI_ANSI_LENGTH : LMI_LENGTH;
	int stat_len = 3;
	u8 *data;
	int i = 0,ch;
	int dlci_cnt=0;

	if (fr_prot->station == WANOPT_NODE && fullrep) {
		dlci_cnt=card->wandev.new_if_cnt;
		len += dlci_cnt * (2 + stat_len);

		if (len > card->wandev.mtu) {
			printk(KERN_INFO "\n");
			printk(KERN_INFO "%s: Error: Full status report size %i exceeds MTU=%i\n", 
					card->devname,len,card->wandev.mtu);
			printk(KERN_INFO "%s: Error: Too many DLCI's configured or increase MTU\n",
					card->devname);
			card->wandev.stats.tx_errors++;
			return;
		}
	}

	skb = dev_alloc_skb(len);
	if (!skb) {
		printk(KERN_INFO "%s: Memory squeeze on fr_lmi_send()\n",
			       card->devname);
		card->wandev.stats.tx_errors++;
		return;
	}
	
	memset(skb->data, 0, len);
	
	skb_reserve(skb, 4);
	skb_push(skb, 4);
	skb->data[3] = LMI_PROTO;
	dlci_to_q922(skb->data, fr_prot->lmi_dlci);
	skb->data[2] = FR_UI;
	
	data = skb->tail;
	data[i++] = LMI_CALLREF;
	data[i++] = (fr_prot->station == WANOPT_NODE) ? LMI_STATUS : LMI_STATUS_ENQUIRY;
	
	if (fr_prot->cfg.signalling == WANOPT_FR_ANSI){
		data[i++] = LMI_ANSI_LOCKSHIFT;
	}
	
	data[i++] = MODE_FR_CCITT ? LMI_CCITT_REPTYPE : LMI_REPTYPE;
	
	data[i++] = LMI_REPT_LEN;
	data[i++] = fullrep ? LMI_FULLREP : LMI_INTEGRITY;

	data[i++] = MODE_FR_CCITT ? LMI_CCITT_ALIVE : LMI_ALIVE;
	data[i++] = LMI_INTEG_LEN;
	data[i++] = fr_prot->txseq = fr_lmi_nextseq(fr_prot->txseq);
	data[i++] = fr_prot->rxseq;

	
	if (fr_prot->station == WANOPT_NODE && fullrep) {

		for (ch=0;ch<MAX_FR_CHANNELS;ch++){
	
			if ((dev=fr_prot->dlci_to_dev_map[ch]) == NULL)
				continue;
		
			if (--dlci_cnt < 0)
				break;
		
			chan=dev->priv;
			if (!chan)
				continue;
	
			//printk(KERN_INFO "Interigating %s DLCI=%i\n",chan->if_name,chan->dlci);
		
			data[i++] = MODE_FR_CCITT ? LMI_CCITT_PVCSTAT:LMI_PVCSTAT;
			data[i++] = stat_len;

			if ((fr_prot->state & LINK_STATE_RELIABLE) &&
			    (dev->flags & IFF_UP) &&
			    !(chan->dlci_state & (PVC_STATE_ACTIVE|PVC_STATE_NEW))) {
				
				chan->dlci_state |= PVC_STATE_NEW;
				fr_log_dlci_active(chan);

				if (!(chan->dlci_state & PVC_STATE_ACTIVE)){
					chan_set_state(dev,WAN_DISCONNECTED);
				}
			}

			dlci_to_status(card, chan->dlci,
				       data+i, chan->dlci_state);
			i += stat_len;
		}
	}

	if (fr_prot->station == WANOPT_NODE){
		if (fullrep){
			fr_prot->link_stats.node_tx_FSR++;
		}else{
			fr_prot->link_stats.node_tx_LIV++;
		}
	}else{
		if (fullrep){
			fr_prot->link_stats.cpe_tx_FSE++;
		}else{
			fr_prot->link_stats.cpe_tx_LIV++;
		}
	}
	
	skb_put(skb, i);
	skb->priority = TC_PRIO_CONTROL;

	capture_trace_packet(card,NULL,skb,TRC_OUTGOING_FRM);

	skb_queue_tail(&fr_prot->lmi_queue,skb);
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_FRAME);
}

void init_global_dlci_state(sdla_t *card)
{
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	memset(fr_prot->global_dlci_map,0,sizeof(fr_prot->global_dlci_map));
	return;
}

void set_global_dlci_state(sdla_t *card, u16 dlci, u8 state)
{
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	if (dlci > HIGHEST_VALID_DLCI){
		return;
	}
	fr_prot->global_dlci_map[dlci] = state;
}


static int fr_lmi_recv(sdla_t* card, struct sk_buff *skb)
{
	struct wan_dev_le	*devle;	
	int stat_len;
	int reptype = -1, error;
	u8 rxseq, txseq;
	fr_prot_t *fr_prot=FR_PROT_AREA(card);
	netdevice_t *dev;
	fr_private_area_t *chan;
	int i;

//	printk(KERN_INFO "\n");
//	printk(KERN_INFO "FR LMI REC: Len=%i Min=%i\n",skb->len, LMI_ANSI_LENGTH);
	
	if (skb->len < ((fr_prot->cfg.signalling == WANOPT_FR_ANSI) ? LMI_ANSI_LENGTH : LMI_LENGTH)) {
		printk(KERN_INFO "%s: Short LMI frame\n", card->devname);
		fr_prot->link_stats.rx_dropped++;
		return 1;
	}

	if (skb->data[5] != ((fr_prot->station != WANOPT_NODE) ?
			     LMI_STATUS : LMI_STATUS_ENQUIRY)) {
		printk(KERN_INFO "%s: LMI msgtype=%x, Not LMI status %s\n",
		       card->devname, skb->data[2],
		       (fr_prot->station != WANOPT_NODE) ? "enquiry" : "reply");
		fr_prot->link_stats.rx_bad_format++;
		return 1;
	}
	
	i = (fr_prot->cfg.signalling == WANOPT_FR_ANSI) ? 7 : 6;

	if (skb->data[i] !=
	    (MODE_FR_CCITT ? LMI_CCITT_REPTYPE : LMI_REPTYPE)) {
		printk(KERN_INFO "%s: Not a report type=%x\n",
		       card->devname, skb->data[i]);
		fr_prot->link_stats.rx_bad_format++;
		return 1;
	}
	i++;
	i++;				/* Skip length field */

	reptype = skb->data[i++];
	//printk(KERN_INFO "(Debug) RepType = 0x%x, Inc=%i\n",reptype,i);
	
	if (skb->data[i] != ((MODE_FR_CCITT) ? LMI_CCITT_ALIVE : LMI_ALIVE)) {
		printk(KERN_INFO "%s: Unsupported status element=%x\n",
		       card->devname, skb->data[i]);
		fr_prot->link_stats.rx_bad_format++;
		return 1;
	}
	i++;

	i++;			/* Skip length field */

	fr_prot->rxseq = skb->data[i++]; /* TX sequence from peer */
	rxseq = skb->data[i++];	/* Should confirm our sequence */

	txseq = fr_prot->txseq;

	//printk(KERN_INFO "TxSeqOrig=%i  TxSeqFrame=%i RxSeqFrame=%i Inc=%i\n",
	//		txseq, fr_prot->rxseq, rxseq,i);
	
	if (fr_prot->station == WANOPT_NODE) {
		switch (reptype){
			
		case LMI_INTEGRITY:
			fr_prot->link_stats.node_rx_LIV++;
			//card->wandev.stats.rx_crc_errors++;
			break;
		case LMI_FULLREP:
			fr_prot->link_stats.node_rx_FSE++;
			//card->wandev.stats.rx_frame_errors++;
			break;

		default:
			printk(KERN_INFO "%s: Unsupported report type=%x\n",
			       card->devname, reptype);
			fr_prot->link_stats.rx_bad_format++;
			return 1;
		}
	}else{
		switch (reptype){
			
		case LMI_INTEGRITY:
			fr_prot->link_stats.cpe_rx_LIV++;
			break;
		case LMI_FULLREP:
			fr_prot->link_stats.cpe_rx_FSR++;
			break;
		}

	}

	error = 0;
	if (!(fr_prot->state & LINK_STATE_RELIABLE)){
		//printk(KERN_INFO "FR Link State ! Reliable\n");
		error = 1;
	}

	if (rxseq == 0 || rxseq != txseq) {
		//printk(KERN_INFO "Rxseq =0 or != TxSeq ask for full status again\n");
		fr_prot->n391cnt = 0; /* Ask for full report next time */
		error = 1;
	}

	if (fr_prot->station == WANOPT_NODE) {
	
		if ((fr_prot->state & LINK_STATE_FULLREP_SENT) && !error) {
			/* Stop sending full report - the last one has been confirmed by DTE */
			fr_prot->state &= ~LINK_STATE_FULLREP_SENT;
			
			WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
				dev = WAN_DEVLE2DEV(devle);
				if (!dev || !wan_netif_priv(dev))
					continue;
				chan = wan_netif_priv(dev);
			
				if (chan->dlci_state & PVC_STATE_NEW) {
					chan->dlci_state &= ~PVC_STATE_NEW;
					chan->dlci_state |= PVC_STATE_ACTIVE;
					fr_log_dlci_active(chan);

					if (card->fe.fe_status == FE_CONNECTED){
						chan_set_state(dev,WAN_CONNECTED);
					}

					/* Tell DTE that new PVC is now active */
					fr_prot->state |= LINK_STATE_CHANGED;
				}
			}
		}

		if (fr_prot->state & LINK_STATE_CHANGED) {
			reptype = LMI_FULLREP;
			fr_prot->state |= LINK_STATE_FULLREP_SENT;
			fr_prot->state &= ~LINK_STATE_CHANGED;
		}

		fr_lmi_send(card, (reptype == LMI_FULLREP ? 1 : 0));
		return 0;
	}

	/* DTE */
	if (reptype != LMI_FULLREP || error){
		//printk(KERN_INFO "RepType=%i != LMI_FULLREP(0) Error=%i\n",
		//		reptype,error);
		return 0;
	}

	stat_len = 3;

	//printk(KERN_INFO "JUST BEFORE DEV LIST Cnt=%i Cnt=%i\n",i,(i + 2 + stat_len));
	
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
		chan = wan_netif_priv(dev);
		chan->newstate=0;
	}

	//printk(KERN_INFO "Starting to go through dlcis\n");
	init_global_dlci_state(card);
	
	while (skb->len >= (i + 2 + stat_len)) {

		u16 dlci;
		u8 state = 0;

		if (skb->data[i] != ((MODE_FR_CCITT) ? LMI_CCITT_PVCSTAT : LMI_PVCSTAT)) {
			printk(KERN_INFO "%s: Invalid PVCSTAT ID: %x\n",
			       card->devname, skb->data[i]);
			fr_prot->link_stats.rx_bad_format++;
			return 1;
		}
		i++;

		if (skb->data[i] != stat_len) {
			printk(KERN_INFO "%s: Invalid PVCSTAT length: %x\n",
			       card->devname, skb->data[i]);
			fr_prot->link_stats.rx_bad_format++;
			return 1;
		}
		i++;

		//printk(KERN_INFO "DLCI TO STATUS starting at I=%i\n",i);

		dlci = status_to_dlci((skb->data+i), &state);

		//printk(KERN_INFO "DLCI IS %i State 0x%x\n",dlci,state);
		
		dev = find_channel(card,dlci);
		if (dev){
			chan=dev->priv;
			if (!chan){
				fr_prot->link_stats.rx_dropped++;
				return 1;
			}
			chan->newstate = state;
		
		}else if (state == PVC_STATE_NEW){
			printk(KERN_INFO "%s: New PVC available, DLCI=%u\n",
			       card->devname, dlci);
		}
		
		set_global_dlci_state(card,dlci,state);
		
		i += stat_len;
	}
	
	//printk(KERN_INFO "End of dlci report\n");

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
		chan = wan_netif_priv(dev);
		if (chan->newstate == PVC_STATE_NEW)
			chan->newstate = PVC_STATE_ACTIVE;

		chan->newstate |= (chan->dlci_state & ~(PVC_STATE_NEW|PVC_STATE_ACTIVE));
		if (chan->dlci_state != chan->newstate) {
			chan->dlci_state = chan->newstate;
			fr_log_dlci_active(chan);

			if ((chan->dlci_state & PVC_STATE_ACTIVE) &&
			    card->fe.fe_status == FE_CONNECTED){
				chan_set_state(dev,WAN_CONNECTED);
			}

			if (!(chan->dlci_state & PVC_STATE_ACTIVE)){
				chan_set_state(dev,WAN_DISCONNECTED);
			}
		}else{
			if (!(chan->dlci_state & PVC_STATE_ACTIVE) && 
			      chan->common.state != WAN_DISCONNECTED){
				chan_set_state(dev,WAN_DISCONNECTED);
			}
		}
		
	}

	/* Next full report after N391 polls */
	fr_prot->n391cnt = fr_prot->cfg.n391;
	return 0;
}



static void fr_netif(sdla_t *card, struct sk_buff *skb)
{
	fr_hdr *fh = (fr_hdr*)skb->data;
	u8 *data = skb->data;
	netdevice_t *dev;
	u16 dlci;
	fr_private_area_t *chan=NULL;
	fr_prot_t *fr_prot = FR_PROT_AREA(card);

	if (skb->len<4 || fh->ea1){ 
		fr_prot->link_stats.rx_bad_format++;
		goto fr_rx_error;
	}

	dlci = q922_to_dlci(skb->data);

	if (dlci == LMI_ANSI_DLCI || dlci == LMI_LMI_DLCI) {

		capture_trace_packet(card,NULL,skb,TRC_INCOMING_FRM);
		
		if (data[2] != FR_UI){
			goto fr_rx_error;
		}
		
		if (data[3] == LMI_PROTO) {
			
			if (fr_lmi_recv(card, skb))
				goto fr_rx_error;
			else {
				/* No request pending */
				fr_prot->state &= ~LINK_STATE_REQUEST;
				fr_prot->last_rx_poll = jiffies;
				return;
			}
		}

		printk(KERN_INFO "%s: Received non-LMI frame with LMI DLCI\n",card->devname);
		fr_prot->link_stats.rx_bad_format++;
		goto fr_rx_error;
	}

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));	
	if (!dev){
		printk(KERN_INFO "%s: FR Protocol no dev!\n",card->devname);
		return;
	}
	
	dev = find_channel(card, dlci);
	if (!dev) {
		fr_prot->link_stats.rx_bad_dlci++;
		goto fr_rx_error;
	}

	if ((dev->flags & IFF_UP) == 0) {
		printk(KERN_INFO "%s: Interface for receive DLCI %d is down\n",
		       card->devname, dlci);
		fr_prot->link_stats.rx_bad_dlci++;
		goto fr_rx_error;
	}

	chan=(fr_private_area_t*)dev->priv;
	if (!chan){
		printk(KERN_INFO "%s: PVC for received frame's DLCI %d is down\n",
		       card->devname, dlci);
		fr_prot->link_stats.rx_bad_dlci++;
		goto fr_rx_error;
	}

	if ((chan->dlci_state & PVC_STATE_FECN) != (fh->fecn ? PVC_STATE_FECN : 0)) {
		chan->rx_FECN++;
		printk(KERN_INFO "%s: FECN O%s\n", dev->name,
		       fh->fecn ? "N" : "FF");
		chan->dlci_state ^= PVC_STATE_FECN;
	}

	if ((chan->dlci_state & PVC_STATE_BECN) != (fh->becn ? PVC_STATE_BECN : 0)) {
		chan->rx_BECN++;
		printk(KERN_INFO "%s: BECN O%s\n", dev->name,
		       fh->becn ? "N" : "FF");
		chan->dlci_state ^= PVC_STATE_BECN;
	}

	if (chan->dlci_state & PVC_STATE_BECN){
		chan->stats.rx_compressed++;
	}

	if( capture_trace_packet(card,chan,skb,TRC_INCOMING_FRM) < 0){
		chan->stats.rx_fifo_errors++;
	}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (chan->common.usedby == ANNEXG){
		if (chan->annexg_dev){
			if (IS_FUNC_CALL(lapb_protocol,lapb_rx)){
				struct sk_buff *new_skb;
				
				skb_pull(skb, 2);
				new_skb=dev_alloc_skb(skb->len);
				if (!new_skb){
					chan->stats.rx_errors++;
					card->wandev.stats.rx_dropped++;
					fr_prot->link_stats.rx_dropped++;
					printk(KERN_INFO "%s: Annexg: Failed to allocate memory\n",
							card->devname);
					return;
				}

				new_skb->protocol = htons(ETH_P_X25);
				new_skb->dev = chan->annexg_dev;
				
				memcpy(skb_put(new_skb,skb->len),skb->data,skb->len);

#if 0
				if (new_skb->len <= 5 && !(new_skb->data[1]&0x01)){
					int x;

					if (new_skb->len == 5 && (new_skb->data[4]&0x01))
						goto skip_error;

					printk(KERN_INFO "%s: ERROR: Rx bad annexg frame len=%i\n",
							card->devname, new_skb->len);

					printk(KERN_INFO "Bad Packet : ");
					for (x=0;x<new_skb->len;x++){
						printk("%X ",new_skb->data[x]);
					}
					printk("\n");
					printk(KERN_INFO "\n");
					chan->stats.rx_dropped++;
					card->wandev.stats.rx_dropped++;
					dev_kfree_skb_any(new_skb);
					return;
skip_error:
				}
#endif

				chan->stats.rx_packets++; 
				chan->stats.rx_bytes += new_skb->len;
				card->wandev.stats.rx_packets++;
       				card->wandev.stats.rx_bytes += new_skb->len;
				
				lapb_protocol.lapb_rx(chan->annexg_dev,new_skb);
			}else{
				chan->stats.rx_errors++;
				card->wandev.stats.rx_dropped++;
				fr_prot->link_stats.rx_dropped++;
			}
		}else{
			chan->stats.rx_errors++;
			fr_prot->link_stats.rx_dropped++;
			card->wandev.stats.rx_dropped++;
		}
		return;
	}
#endif

	if (data[2] != FR_UI){
		fr_prot->link_stats.rx_bad_format++;
		chan->stats.rx_errors++;
		goto fr_rx_error;
	}

	if (data[3] == NLPID_IP) {
		struct sk_buff *new_skb;
		skb_pull(skb, 4); /* Remove 4-byte header (hdr, UI, NLPID) */
		skb->protocol = htons(ETH_P_IP);
		skb->dev = dev;
		
		new_skb=skb_clone(skb,GFP_ATOMIC);
		if (!new_skb){
			chan->stats.rx_errors++;
			fr_prot->link_stats.rx_dropped++;
			return;
		}
		
		++card->wandev.stats.rx_packets;
       		card->wandev.stats.rx_bytes += skb->len;

		chan->stats.rx_packets++; /* PVC traffic */
		chan->stats.rx_bytes += skb->len;

		netif_rx(new_skb);
		return;
	}

	if (data[3] == NLPID_IPV6) {

		struct sk_buff *new_skb;
		skb_pull(skb, 4); /* Remove 4-byte header (hdr, UI, NLPID) */
		skb->protocol = htons(ETH_P_IPV6);
		skb->dev = dev;
		
		new_skb=skb_clone(skb,GFP_ATOMIC);
		if (!new_skb){
			chan->stats.rx_errors++;
			fr_prot->link_stats.rx_dropped++;
			return;
		}
	
		++card->wandev.stats.rx_packets;
       		card->wandev.stats.rx_bytes += skb->len;

		chan->stats.rx_packets++; /* PVC traffic */
		chan->stats.rx_bytes += skb->len;

		netif_rx(new_skb);
		return;
	}

	if (data[3] == FR_PAD && data[4] == NLPID_SNAP && data[5] == FR_PAD &&
	    data[6] == FR_PAD && data[7] == FR_PAD &&
	    ((data[8]<<8) | data[9]) == ETH_P_ARP) {
		
		struct sk_buff *new_skb;

		skb_pull(skb, 10);
		skb->protocol = htons(ETH_P_ARP);
		skb->dev = dev;
		
		new_skb=skb_clone(skb,GFP_ATOMIC);
		if (!new_skb){
			chan->stats.rx_errors++;
			fr_prot->link_stats.rx_dropped++;
			return;
		}

		++card->wandev.stats.rx_packets;
       		card->wandev.stats.rx_bytes += skb->len;

		chan->stats.rx_packets++; /* PVC traffic */
		chan->stats.rx_bytes += skb->len;
		
		netif_rx(new_skb);
		return;
	}

	printk(KERN_INFO "%s: Unusupported protocol %x\n",
	       card->devname, data[3]);
	
	fr_prot->link_stats.rx_bad_format++;
	card->wandev.stats.rx_dropped++;
	chan->stats.rx_dropped++;
	return;

fr_rx_error:
	card->wandev.stats.rx_dropped++;
	return;
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
	fr_prot_t *fr_prot=FR_PROT_AREA(card);
	
	if(dlci > HIGHEST_VALID_DLCI)
		return NULL;

	return(fr_prot->dlci_to_dev_map[dlci]);
}


static void fr_bh (unsigned long card_data)
{
#define card ((sdla_t*)card_data)
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	struct sk_buff *skb;
	unsigned long timeout=jiffies;	

	if (!skb_queue_len(&fr_prot->rx_used)){
		clear_bit(0, &fr_prot->tq_working);
		return;
	}

	while ((skb=skb_dequeue(&fr_prot->rx_used)) != NULL){
	
		fr_netif(card,skb);
	
		skb->data = skb->head+16;
		skb_trim(skb,0);
		skb_queue_tail(&fr_prot->rx_free,skb);

		if ((jiffies-timeout) >= 4){
			printk(KERN_INFO "%s: FR BH Kicking out, bh timeout 40ms\n",
					card->devname);
			break;
		}
	}
	
	clear_bit(0, &fr_prot->tq_working);
	return;
#undef card
}


static int capture_trace_packet (sdla_t *card,fr_private_area_t*chan,struct sk_buff *skb,char direction)
{
	fr_prot_t *fr_prot = FR_PROT_AREA(card);
	
	if (!chan){
		goto trace_protocol;
	}

	
	if (test_bit(0,&chan->tracing_enabled)){
	
		if ((jiffies-chan->trace_timeout) > MAX_TRACE_TIMEOUT){
			printk(KERN_INFO "%s: %s: Disabling DLCI=%i trace, timeout!\n",
					card->devname,chan->if_name,chan->dlci);
			clear_bit(0,&chan->tracing_enabled);
			return 0;
		}

	 
		if (skb_queue_len(&chan->trace_queue) < chan->max_trace_queue){		
				
			struct sk_buff *new_skb;

			new_skb=dev_alloc_skb(skb->len+sizeof(fr_trc_el_t)+chan->hdr_len);
			if (new_skb){
				fr_trc_el_t *trc_el = (fr_trc_el_t*)skb_put(new_skb,sizeof(fr_trc_el_t));	
				trc_el->attr = direction;
				trc_el->tmstamp = (unsigned short)(jiffies%0xFFFF);
				trc_el->length = skb->len;
				
				if (direction == TRC_OUTGOING_FRM){
					trc_el->length += chan->hdr_len;
					memcpy(skb_put(new_skb,chan->hdr_len),chan->header,chan->hdr_len);
				}
				
				memcpy(skb_put(new_skb,skb->len),skb->data,skb->len);
				skb_queue_tail(&chan->trace_queue,new_skb);
			}else{
				//ALEX
				//printk(KERN_INFO "%s: %s: %i: Failed allocate memory for new packets in trace queue!\n",
				//		card->devname, chan->if_name, chan->dlci);
				return -ENOMEM;
			}
		}else{
			//ALEX
			//printk(KERN_INFO "%s: %s: %i: Too many packets in trace queue (%i)!\n",
			//		card->devname, chan->if_name, chan->dlci, 
			//		skb_queue_len(&chan->trace_queue));
			return -ENOBUFS;
		}
	}

	return 0;
	
trace_protocol:

	if (test_bit(0,&fr_prot->tracing_enabled)){

		if ((jiffies-fr_prot->trace_timeout) > MAX_TRACE_TIMEOUT){
			printk(KERN_INFO "%s: Disabling LMI trace, timeout!\n",
					card->devname);
			clear_bit(0,&fr_prot->tracing_enabled);
			return 0;
		}

		if (skb_queue_len(&fr_prot->trace_queue) < fr_prot->max_trace_queue){		
					
			struct sk_buff *new_skb;

			new_skb=dev_alloc_skb(skb->len+sizeof(fr_trc_el_t));
			if (new_skb){
				fr_trc_el_t *trc_el = (fr_trc_el_t*)skb_put(new_skb,sizeof(fr_trc_el_t));	
				trc_el->attr = direction;
				trc_el->tmstamp = (unsigned short)(jiffies%0xFFFF);
				trc_el->length = skb->len;
				memcpy(skb_put(new_skb,skb->len),skb->data,skb->len);
				skb_queue_tail(&fr_prot->trace_queue,new_skb);
			}else{
				//ALEX
				printk(KERN_INFO "%s: Failed to allocate memory for new packets in trace queue!\n",
						card->devname);
				return -ENOMEM;
			}
		}else{
			//ALEX
			printk(KERN_INFO "%s: Too many packets in trace queue (%i)!\n",
					card->devname, skb_queue_len(&fr_prot->trace_queue));
			return -ENOBUFS;
		}
	}
	return 0;
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
	wanpipe_snmp_t*	snmp = NULL;
	struct timeval	tv;
	fr_prot_t	*fr_prot;
	
	if (dev == NULL || dev->priv == NULL)
		return -EFAULT;
	chan = (fr_channel_t*)dev->priv;
	fr_prot=FR_PROT_AREA(card);
	/* Update device statistics */
	if (card->wandev.update) {
		int rslt = 0;
		fr_prot->update_dlci = chan;
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
		snmp->snmp_val = fr_prot->cfg.t391;
		break;

	case FRDLCMIFULLENQUIRYINTERVAL:
		snmp->snmp_val = fr_prot->cfg.n391;
		break;

	case FRDLCMIERRORTHRESHOLD:
		snmp->snmp_val = fr_prot->cfg.n392;
		break;

	case FRDLCMIMONITOREDEVENTS:
		snmp->snmp_val = fr_prot->cfg.n393;
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
		snmp->snmp_val = chan->stats.tx_packets;
		break;

	case FRCIRCUITSENTOCTETS:
		snmp->snmp_val = chan->stats.tx_bytes;
		break;

	case FRCIRCUITRECEIVEDFRAMES:
		snmp->snmp_val = chan->stats.rx_packets;
		break;

	case FRCIRCUITRECEIVEDOCTETS:
		snmp->snmp_val = chan->stats.rx_bytes;
		break;

	case FRCIRCUITCREATIONTIME:
		do_gettimeofday( &tv );
		snmp->snmp_val = tv.tv_sec - card->u.c.router_start_time;
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
		snmp->snmp_val = chan->stats.rx_errors;
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

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG

static int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev)
{
	unsigned long smp_flags=0;
	fr_channel_t* chan = dev->priv;
	sdla_t *card = chan->card;
	if (!chan)
		return -EINVAL;

	if (chan->common.usedby != ANNEXG)
		return -EPROTONOSUPPORT;
	
	if (chan->annexg_dev)
		return -EBUSY;

	spin_lock_irqsave(&card->wandev.lock,smp_flags);
	chan->annexg_dev = annexg_dev;
	spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
	return 0;
}

static netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t *annexg_dev)
{	
	struct wan_dev_le	*devle;
	netdevice_t		*dev;
	unsigned long		smp_flags=0;
	sdla_t			*card = wandev->priv;

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		fr_channel_t	*chan;

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
		chan = wan_netif_priv(dev);

		if (!chan->annexg_dev || chan->common.usedby != ANNEXG)
			continue;

		if (chan->annexg_dev == annexg_dev){
			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			chan->annexg_dev = NULL;
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
			return dev;
		}
	}
	return NULL;
}

static void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats_ptr)
{
	fr_channel_t* 	chan = dev->priv;
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
get_map(wan_device_t *wandev, netdevice_t *dev, struct seq_file* m, int *stop_cnt)
{
	fr_channel_t* 	chan = dev->priv;

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




#ifdef _DBG_ANNEXG_

#undef DBG_LAPB
#define DBG_X25
static int g_ps_cnt=0;
static unsigned int g_skb_ptr=0;
void check_x25_pr_ps_cnt(sdla_t *card,struct sk_buff *skb)
{
	int ps_cnt; 
	int lcn;

#ifdef DBG_LAPB
	if (skb->len >= 2 && !test_bit(0,&skb->data[1])){

		ps_cnt = (skb->data[1]&0x0F)>>1;

		if (ps_cnt == g_ps_cnt){
			printk(KERN_INFO "%s: Sending Len=%i Ps=%i Old=%i : New ptr=%u  Prev=%u\n",
				card->devname,skb->len,ps_cnt,g_ps_cnt,
				(u32)skb,g_skb_ptr);
		}
		g_ps_cnt=ps_cnt;
		g_skb_ptr=(u32)skb;
	}
#endif
	
#ifdef DBG_X25
	if (skb->len >= 5 && !test_bit(0,&skb->data[1])){
		if (!test_bit(0,&skb->data[4])){

			lcn=((skb->data[2]&0x0F)<<8) | skb->data[3];
			if (lcn!=2)
				return;

			
			ps_cnt = (skb->data[4]&0x0F)>>1;
			
			if (ps_cnt == g_ps_cnt){
			printk(KERN_INFO "%s: Sending Lcn=%i Len=%i Ps=%i Old=%i : New ptr=%u  Prev=%u\n",
				card->devname,lcn,skb->len,ps_cnt,g_ps_cnt,
				(u32)skb,g_skb_ptr);
			}

			g_ps_cnt=ps_cnt;
			g_skb_ptr=(u32)skb;
		}
	}
#endif
	return;
}
#endif


/****** End ****************************************************************/
