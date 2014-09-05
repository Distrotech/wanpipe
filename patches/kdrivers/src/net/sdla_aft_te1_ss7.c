/*****************************************************************************
* sdla_aft_te1_ss7.c	
*
* 		WANPIPE(tm) AFT SS7 Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* May 20, 2004	Nenad Corbic	Initial version.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe.h"
#include "wanpipe_abstr.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */
#include "if_wanpipe.h"
#include "sdlapci.h"
#include "sdla_aft_te1_ss7.h"

//#define  XILINX_A010 	1	

/****** Defines & Macros ****************************************************/

/* Private critical flags */
enum {
	POLL_CRIT = PRIV_CRIT,
	TX_BUSY,
	RX_BUSY,
	TASK_POLL,
	CARD_DOWN
};

enum { 
	LINK_DOWN,
	DEVICE_DOWN
};

#define MAX_IP_ERRORS	10

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_TX_BUF	10
#define MAX_RX_BUF	10

#undef DEB_XILINX    

#if 1 
# define TRUE_FIFO_SIZE 1
#else
# undef  TRUE_FIFO_SIZE
# define HARD_FIFO_CODE 0x01
#endif

static int aft_rx_copyback=1000;

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following
 * structure will incorporate the card structure along with Protocol specific data
 */

typedef struct private_area
{
	wanpipe_common_t 	common;

	wan_skb_queue_t 	wp_tx_free_list;
	wan_skb_queue_t 	wp_tx_pending_list;
	wan_skb_queue_t 	wp_tx_complete_list;
	netskb_t 		*tx_dma_skb;
	u8			tx_dma_cnt;

	wan_skb_queue_t 	wp_rx_free_list;
	wan_skb_queue_t 	wp_rx_complete_list;
	netskb_t 		*rx_dma_skb;

	unsigned long 		time_slot_map;
	unsigned char 		num_of_time_slots;
	long          		logic_ch_num;

	unsigned char		hdlc_eng;
	unsigned char		dma_status;
	unsigned char		protocol;
	unsigned char 		ignore_modem;

	struct net_device_stats	if_stats;

#if 1
	int 		tracing_enabled;		/* For enabling Tracing */
	unsigned long 	router_start_time;
	unsigned long   trace_timeout;

	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;

	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned char 	udp_pkt_src;		/* udp packet processing */
	unsigned short 	timer_int_enabled;

	unsigned char 	interface_down;

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	struct tq_struct 	poll_task;
	struct timer_list 	poll_delay_timer;

	u8 		gateway;
	u8 		true_if_encoding;

	//FIXME: add driver stats as per frame relay!
#endif

	/* Entry in proc fs per each interface */
	struct proc_dir_entry	*dent;

	unsigned char 	udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t 	udp_pkt_len;

	char 		if_name[WAN_IFNAME_SZ+1];

	u8		idle_flag;
	u16		max_idle_size;
	u8		idle_start;

	u8		pkt_error;
	u8		rx_fifo_err_cnt;

	int		first_time_slot;
	
	netskb_t  *tx_idle_skb;
	unsigned long	tx_dma_addr;
	unsigned int	tx_dma_len;
	unsigned char	rx_dma;
	unsigned char   pci_retry;
	
	unsigned char	fifo_size_code;
	unsigned char	fifo_base_addr;
	unsigned char 	fifo_size;

	int		dma_mru;

	void *		prot_ch;
	int		prot_state;

	wan_trace_t	trace_info;

}private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03

#define WP_WAIT 	0
#define WP_NO_WAIT	1

/* variable for keeping track of enabling/disabling FT1 monitor status */
//static int rCount;

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

/**SECTOIN**************************************************
 *
 * Function Prototypes
 *
 ***********************************************************/

/* WAN link driver entry points. These are called by the WAN router module. */
static int 	update (wan_device_t* wandev);
static int 	new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf);
static int 	del_if(wan_device_t *wandev, netdevice_t *dev);

/* Network device interface */
static int 	if_init   (netdevice_t* dev);
static int 	wanpipe_xilinx_open   (netdevice_t* dev);
//static int 	if_open   (netdevice_t* dev);
static int 	wanpipe_xilinx_close  (netdevice_t* dev);
//static int 	if_close  (netdevice_t* dev);
//static int 	if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);
//static struct net_device_stats* if_stats (netdevice_t* dev);
static int 	wanpipe_xilinx_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);
static struct net_device_stats* wanpipe_xilinx_ifstats (netdevice_t* dev);

//static int 	if_send (netskb_t* skb, netdevice_t* dev);
static int 	wanpipe_xilinx_send (netskb_t* skb, netdevice_t* dev);

static void 	handle_front_end_state(void* card_id);
static void 	enable_timer(void* card_id);
static void 	wanpipe_xilinx_tx_timeout (netdevice_t* dev);
//static void 	if_tx_timeout (netdevice_t *dev);

/* Miscellaneous Functions */
static void 	port_set_state (sdla_t *card, int);

static void 	disable_comm (sdla_t *card);

/* Interrupt handlers */
static void 	wp_xilinx_isr (sdla_t* card);

/* Bottom half handlers */
static void 	wp_bh (unsigned long);

/* Miscellaneous functions */
#if 0
static int 	chk_bcast_mcast_addr(sdla_t* card, netdevice_t* dev,
				netskb_t *skb);
#endif

static int 	process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				private_area_t*,
				int local_dev);

static int xilinx_chip_configure(sdla_t *card);
static int xilinx_chip_unconfigure(sdla_t *card);
static int xilinx_dev_configure(sdla_t *card, private_area_t *chan);
static void xilinx_dev_unconfigure(sdla_t *card, private_area_t *chan);
static int xilinx_dma_rx(sdla_t *card, private_area_t *chan);
static void xilinx_dev_enable(sdla_t *card, private_area_t *chan);
static void xilinx_dev_close(sdla_t *card, private_area_t *chan);
static int xilinx_dma_tx (sdla_t *card, private_area_t *chan);
static void xilinx_dma_tx_complete (sdla_t *card, private_area_t *chan);
static void xilinx_dma_rx_complete (sdla_t *card, private_area_t *chan);
static void xilinx_dma_max_logic_ch(sdla_t *card);
static int xilinx_init_rx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char);
static void xilinx_init_tx_dma_descr(sdla_t *card, private_area_t *chan);
static int xilinx_init_tx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char);
static void xilinx_tx_post_complete (sdla_t *card, private_area_t *chan, netskb_t *skb);
static void xilinx_rx_post_complete (sdla_t *card, private_area_t *chan,
                                     netskb_t *skb,
                                     netskb_t **new_skb,
                                     unsigned char *pkt_error);


static char request_xilinx_logical_channel_num (sdla_t *card, private_area_t *chan, long *free_ch);
static void free_xilinx_logical_channel_num (sdla_t *card, int logic_ch);


static unsigned char read_cpld(sdla_t *card, unsigned short cpld_off);
static unsigned char write_cpld(sdla_t *card, unsigned short cpld_off,unsigned char cpld_data);

static int xilinx_write_bios(sdla_t *card,struct ifreq *ifr);
static int xilinx_write(sdla_t *card,struct ifreq *ifr);
static int xilinx_read(sdla_t *card,struct ifreq *ifr);

static void front_end_interrupt(sdla_t *card, unsigned long reg);
static void  enable_data_error_intr(sdla_t *card);
static void  disable_data_error_intr(sdla_t *card, unsigned char);

static void xilinx_tx_fifo_under_recover (sdla_t *card, private_area_t *chan);

static int xilinx_write_ctrl_hdlc(sdla_t *card, u32 timeslot, u8 reg_off, u32 data);

static int set_chan_state(sdla_t* card, netdevice_t* dev, int state);

static int request_fifo_baddr_and_size(sdla_t *card, private_area_t *chan);
static int map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr);
static int free_fifo_baddr_and_size (sdla_t *card, private_area_t *chan);

static int update_comms_stats(sdla_t* card);

static void aft_red_led_ctrl(sdla_t *card, int mode);
static void aft_led_timer(unsigned long data);

static int aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num);
static int aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb);
static int aft_reinit_pending_rx_bufs(private_area_t *chan);
static int aft_core_ready(sdla_t *card);

static int capture_trace_packet (sdla_t *card, private_area_t*chan, netskb_t *skb,char direction);

/* Procfs functions */
static int wan_aft_ss7_get_info(void* pcard, struct seq_file* m, int* stop_cnt); 

static void xilinx_delay(int sec)
{
#if 0
	unsigned long timeout=jiffies;
	while ((jiffies-timeout)<(sec*HZ)){
		schedule();
	}
#endif
}

/**SECTION*********************************************************
 *
 * Public Functions
 *
 ******************************************************************/



/*============================================================================
 * wp_xilinx_init - Cisco HDLC protocol initialization routine.
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

int wp_aft_te1_ss7_init (sdla_t* card, wandev_conf_t* conf)
{
	int err;

	/* Verify configuration ID */
	wan_clear_bit(CARD_DOWN,&card->wandev.critical);

	if (card->wandev.config_id != WANCONFIG_AFT_TE1_SS7) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
				  card->devname, card->wandev.config_id);
		return -EINVAL;
	}

	if (conf == NULL){
		DEBUG_EVENT("%s: Bad configuration structre!\n",
				card->devname);
		return -EINVAL;
	}

#if defined(WAN_DEBUG_MEM)
        DEBUG_EVENT("%s: Total Mem %d\n",__FUNCTION__,wan_atomic_read(&wan_debug_mem));
#endif


	/* Obtain hardware configuration parameters */
	card->wandev.clocking 			= conf->clocking;
	card->wandev.ignore_front_end_status 	= conf->ignore_front_end_status;
	card->wandev.ttl 			= conf->ttl;
	card->wandev.electrical_interface 			= conf->electrical_interface;
	card->wandev.comm_port 			= conf->comm_port;
	card->wandev.udp_port   		= conf->udp_port;
	card->wandev.new_if_cnt 		= 0;
	wan_atomic_set(&card->wandev.if_cnt,0);

	memcpy(&card->u.xilinx.cfg,&conf->u.xilinx,sizeof(wan_xilinx_conf_t));

	card->u.xilinx.cfg.dma_per_ch = 10;
		
	/* TE1 Make special hardware initialization for T1/E1 board */
	if (IS_TE1_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg		= card->hw_iface.fe_read;

		card->wandev.fe_enable_timer = enable_timer;
		card->wandev.te_link_state = handle_front_end_state;
		conf->electrical_interface =
			IS_T1_CARD(card) ? WANOPT_V35 : WANOPT_RS232;

		if (card->wandev.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else if (IS_56K_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->wandev.fe_iface);
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

	/* WARNING: After this point the init function
	 * must return with 0.  The following bind
	 * functions will cause problems if structures
	 * below are not initialized */
	
        card->wandev.update             = &update;
        card->wandev.new_if             = &new_if;
        card->wandev.del_if             = &del_if;

     	card->disable_comm              = &disable_comm;

#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
	/* Proc fs functions hooks */
	card->wandev.get_config_info 	= &get_config_info;
	card->wandev.get_status_info 	= &get_status_info;
	card->wandev.get_dev_config_info= &get_dev_config_info;
	card->wandev.get_if_info     	= &get_if_info;
	card->wandev.set_dev_config    	= &set_dev_config;
	card->wandev.set_if_info     	= &set_if_info;
#endif
	card->wandev.get_info		= &wan_aft_ss7_get_info;

	/* Setup Port Bps */
	if(card->wandev.clocking) {
		card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
  	}

        /* For Primary Port 0 */
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
        card->wandev.mtu = conf->mtu;
#else
        card->wandev.mtu =
                (conf->mtu >= MIN_WP_PRI_MTU) ?
                 wp_min(conf->mtu, MAX_WP_PRI_MTU) : DEFAULT_WP_PRI_MTU;
#endif


	if (!card->u.xilinx.cfg.mru){
		card->u.xilinx.cfg.mru = card->wandev.mtu;
	}


	write_cpld(card,LED_CONTROL_REG,0x0E);

	
	card->hw_iface.getcfg(card->hw, SDLA_BASEADDR, &card->u.xilinx.bar);

	xilinx_delay(1);
	err=xilinx_chip_configure(card);
        card->isr = &wp_xilinx_isr;

	xilinx_delay(1);

	/* Set protocol link state to disconnected,
	 * After seting the state to DISCONNECTED this
	 * function must return 0 i.e. success */
	port_set_state(card,WAN_CONNECTING);

	wan_init_timer(&card->u.xilinx.led_timer, 
		       aft_led_timer, 
		       (unsigned long)card);

	wan_add_timer(&card->u.xilinx.led_timer,HZ);

	DEBUG_EVENT("%s: Config:\n",card->devname);
	DEBUG_EVENT("%s:    MTU = %d\n", 
			card->devname, 
			card->wandev.mtu);
	DEBUG_EVENT("%s:    MRU = %d\n", 
			card->devname, 
			card->u.xilinx.cfg.mru);

	return 0;
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
 	netdevice_t* dev;
        volatile private_area_t* chan;
	unsigned long smp_flags;

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;

	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	if(wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical))
                return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if(dev == NULL)
		return -ENODEV;

	if((chan=dev->priv) == NULL)
		return -ENODEV;

       	if(card->update_comms_stats){
		return -EAGAIN;
	}

#if 0
	{
        	private_area_t* chan;
		DEBUG_EVENT("%s: Starting up Interfaces\n",card->devname);
		for (dev=card->wandev.dev;dev;dev=wan_next_dev(dev)){
			chan=dev->priv;
			if (WAN_NETIF_QUEUE_STOPPED(dev)){
				DEBUG_EVENT("%s: Waking up device! Q=%i\n",
						dev->name,
		                wan_skb_queue_len(&chan->wp_tx_pending_list));
				start_net_queue(dev);
			}
		}
	}
#endif

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	update_comms_stats(card);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	
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
static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	sdla_t* card = wandev->priv;
	private_area_t* chan;
	int err = 0;
	netskb_t *skb;

	DEBUG_EVENT( "%s: Configuring Interface: %s\n",
			card->devname, dev->name);

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)){
		DEBUG_EVENT( "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	/* allocate and initialize private data */
	chan = wan_malloc(sizeof(private_area_t));
	if(chan == NULL){
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}
	memset(chan, 0, sizeof(private_area_t));

	chan->first_time_slot=-1;

	strncpy(chan->if_name, dev->name, WAN_IFNAME_SZ);

	chan->common.card = card;

	WAN_IFQ_INIT(&chan->wp_tx_free_list, 0);
	WAN_IFQ_INIT(&chan->wp_tx_pending_list,0);
	WAN_IFQ_INIT(&chan->wp_tx_complete_list,0);
	
	WAN_IFQ_INIT(&chan->wp_rx_free_list,0);
	WAN_IFQ_INIT(&chan->wp_rx_complete_list,0);

	wan_trace_info_init(&chan->trace_info,MAX_TRACE_QUEUE);

	/* Initialize the socket binding information
	 * These hooks are used by the API sockets to
	 * bind into the network interface */

	WAN_TASKLET_INIT((&chan->common.bh_task), 0, wp_bh, chan);
	chan->common.dev = dev;
	chan->tracing_enabled = 0;
	chan->route_status = NO_ROUTE;
	chan->route_removed = 0;

	/* Setup interface as:
	 *    WANPIPE 	  = IP over Protocol (Firmware)
	 *    API     	  = Raw Socket access to Protocol (Firmware)
	 *    BRIDGE  	  = Ethernet over Protocol, no ip info
	 *    BRIDGE_NODE = Ethernet over Protocol, with ip info
	 */
	if(strcmp(conf->usedby, "WANPIPE") == 0) {

		DEBUG_EVENT( "%s: Running in WANPIPE mode\n",
			wandev->name);
		chan->common.usedby = WANPIPE;

		/* Option to bring down the interface when
        	 * the link goes down */
		if (conf->if_down){
			wan_set_bit(DYN_OPT_ON,&chan->interface_down);
			DEBUG_EVENT(
			 "%s:%s: Dynamic interface configuration enabled\n",
			   card->devname,chan->if_name);
		}

	} else if( strcmp(conf->usedby, "API") == 0) {
		chan->common.usedby = API;
		DEBUG_EVENT( "%s:%s: Running in API mode\n",
			wandev->name,chan->if_name);

		wan_reg_api(chan, dev, card->devname);

	}else if (strcmp(conf->usedby, "BRIDGE") == 0) {
		chan->common.usedby = BRIDGE;
		DEBUG_EVENT( "%s:%s: Running in WANPIPE (BRIDGE) mode.\n",
				card->devname,chan->if_name);

	}else if (strcmp(conf->usedby, "BRIDGE_N") == 0) {
		chan->common.usedby = BRIDGE_NODE;
		DEBUG_EVENT( "%s:%s: Running in WANPIPE (BRIDGE_NODE) mode.\n",
				card->devname,chan->if_name);

	}else if (strcmp(conf->usedby, "TDM_VOICE") == 0) {
		chan->common.usedby = TDM_VOICE;
		DEBUG_EVENT( "%s:%s: Running in TDM Voice mode.\n",
				card->devname,chan->if_name);

	}else if (strcmp(conf->usedby, "STACK") == 0) {
		chan->common.usedby = STACK;
		DEBUG_EVENT( "%s:%s: Running in Stack mode.\n",
				card->devname,chan->if_name);
		
	}else{
		DEBUG_ERROR( "%s:%s: Error: Invalid operation mode [WANPIPE|API|BRIDGE|BRIDGE_NODE]\n",
				card->devname,chan->if_name);
		err=-EINVAL;
		goto new_if_error;
	}

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
	if (chan->common.usedby == TDM_VOICE){
		int	err;
		//if (wp_tdmv_init(card, conf)){
		WAN_TDMV_CALL(init, (card, conf), err);
		if (err){
			err = -EINVAL;
			goto new_if_error;
		}
	}
#endif

	xilinx_delay(1);

	chan->hdlc_eng = conf->hdlc_streaming;

	if (!chan->hdlc_eng){
		if (card->u.xilinx.cfg.mru&0x03){
			DEBUG_ERROR("%s:%s: Error, Transparent MTU must be word aligned!\n",
					card->devname,chan->if_name);
			err = -EINVAL;
			goto new_if_error;
		}
	}
	
	chan->time_slot_map=conf->active_ch;

	err=xilinx_dev_configure(card,chan);
	if (err){
		goto new_if_error;
	}

	xilinx_delay(1);

	if (!chan->hdlc_eng){
		unsigned char *buf;

		if (!chan->max_idle_size){
			chan->max_idle_size=card->u.xilinx.cfg.mru;
		}
	
		DEBUG_EVENT("%s:%s: Config Transparent Mode: \n",
			card->devname,chan->if_name);

		DEBUG_EVENT("%s:%s:     Idle Flag = 0x%02X\n",
				card->devname,chan->if_name,
				chan->idle_flag);
		DEBUG_EVENT("%s:%s:     Idle Len  = %i\n",
				card->devname,chan->if_name,
				chan->max_idle_size);
		
		//NENAD FIXME !!!!!!!!!!!!!!!!
		//chan->idle_flag=0x7E;     
		chan->idle_flag=0;     

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		if (chan->common.usedby == TDM_VOICE){
			chan->idle_flag = WAN_TDMV_IDLE_FLAG;
		}
#endif

		chan->tx_idle_skb = wan_skb_alloc(chan->max_idle_size); 
		if (!chan->tx_idle_skb){
			err=-ENOMEM;
			goto new_if_error;
		}
		buf=skb_put(chan->tx_idle_skb,chan->max_idle_size);
		memset(buf,chan->idle_flag,chan->max_idle_size);
	}
	
	chan->dma_mru = card->u.xilinx.cfg.mru;
	
	chan->dma_mru = xilinx_valid_mtu(chan->dma_mru+100);
	if (!chan->dma_mru){
		DEBUG_ERROR("%s:%s: Error invalid MTU %i  mru %i\n",
			card->devname,
			chan->if_name,
			card->wandev.mtu,card->u.xilinx.cfg.mru);
		err= -EINVAL;
		goto new_if_error;
	}

	DEBUG_EVENT("%s:%s: Allocating %i dma skb len=%i\n",
			card->devname,chan->if_name,
			card->u.xilinx.cfg.dma_per_ch,
			chan->dma_mru);

	
	err=aft_alloc_rx_dma_buff(card, chan, card->u.xilinx.cfg.dma_per_ch);
	if (err){
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
	if ((chan->gateway = conf->gateway) == WANOPT_YES){
		DEBUG_EVENT( "%s: Interface %s is set as a gateway.\n",
			card->devname,chan->if_name);
	}

	/* Get Multicast Information from the user
	 * FIXME: This option is not clearly defined
	 */
	chan->mc = conf->mc;


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

#if 0
	/* Create interface file in proc fs.
	 * Once the proc file system is created, the new_if() function
	 * should exit successfuly.
	 *
	 * DO NOT place code under this function that can return
	 * anything else but 0.
	 */
	err = wanrouter_proc_add_interface(wandev,
					   &chan->dent,
					   chan->if_name,
					   dev);
	if (err){
		DEBUG_EVENT(
			"%s: can't create /proc/net/router/frmw/%s entry!\n",
			card->devname, chan->if_name);
		goto new_if_error;
	}
#endif
	/* Only setup the dev pointer once the new_if function has
	 * finished successfully.  DO NOT place any code below that
	 * can return an error */
	dev->init = &if_init;
	dev->priv = chan;

	/* Increment the number of network interfaces
	 * configured on this card.
	 */
	wan_atomic_inc(&card->wandev.if_cnt);

	chan->common.state = WAN_CONNECTING;

	DEBUG_EVENT( "\n");

	return 0;

new_if_error:

	while ((skb=wan_skb_dequeue(&chan->wp_tx_free_list)) != NULL){
		wan_skb_free(skb);
	}

	while ((skb=wan_skb_dequeue(&chan->wp_rx_free_list)) != NULL){
		wan_skb_free(skb);
	}

	WAN_TASKLET_KILL(&chan->common.bh_task);
	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

	if (chan->tx_idle_skb){
		wan_skb_free(chan->tx_idle_skb);
		chan->tx_idle_skb=NULL;
	}

	wan_free(chan);

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
 * NOTE: DO NOT deallocate dev->priv here! It will be
 *       done by the upper layer.
 *
 */
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	private_area_t* 	chan = dev->priv;
	sdla_t*			card = (sdla_t*)chan->common.card;
	netskb_t 		*skb;

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
	if (chan->common.usedby == TDM_VOICE){
		int err;
		//if ((err = wp_tdmv_remove(card)) != 0){
		WAN_TDMV_CALL(remove, (card), err);
		if (err){
			return err;
		}
	}
#endif
	xilinx_dev_unconfigure(card,chan);

	WAN_TASKLET_KILL(&chan->common.bh_task);
	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

	while ((skb=wan_skb_dequeue(&chan->wp_rx_free_list)) != NULL){
		wan_skb_free(skb);
	}

	while ((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
		wan_skb_free(skb);
	}

	while ((skb=wan_skb_dequeue(&chan->wp_tx_free_list)) != NULL){
		wan_skb_free(skb);
	}
	while ((skb=wan_skb_dequeue(&chan->wp_tx_pending_list)) != NULL){
		wan_skb_free(skb);
	}


        if (chan->tx_dma_addr && chan->tx_dma_len){
                pci_unmap_single(NULL,
                                 chan->tx_dma_addr,
                                 chan->tx_dma_len,
                                 PCI_DMA_TODEVICE);

                chan->tx_dma_addr=0;
                chan->tx_dma_len=0;
        }


	if (chan->tx_dma_skb){
		DEBUG_EVENT("freeing tx dma skb\n");
		wan_skb_free(chan->tx_dma_skb);
		chan->tx_dma_skb=NULL;
	}

	if (chan->tx_idle_skb){
		DEBUG_EVENT("freeing idle tx dma skb\n");
		wan_skb_free(chan->tx_idle_skb);
		chan->tx_idle_skb=NULL;
	}

	if (chan->rx_dma_skb){
		wp_rx_element_t *rx_el;
		netskb_t *skb=chan->rx_dma_skb;

		chan->rx_dma_skb=NULL;
		rx_el=(wp_rx_element_t *)&skb->cb[0];
	
		pci_unmap_single(NULL, 
			 rx_el->dma_addr,
			 chan->dma_mru,
			 PCI_DMA_FROMDEVICE);

                wan_skb_free(skb);
        }

	/* Delete interface name from proc fs. */
#if 0
	wanrouter_proc_delete_interface(wandev, chan->if_name);
#endif

	/* Decrement the number of network interfaces
	 * configured on this card.
	 */
	wan_atomic_dec(&card->wandev.if_cnt);

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
#if defined(__LINUX__)
static int if_init (netdevice_t* dev)
{
	private_area_t* chan = dev->priv;
	sdla_t*		card = (sdla_t*)chan->common.card;
	wan_device_t* 	wandev = &card->wandev;

	/* Initialize device driver entry points */
	dev->open		= &wanpipe_xilinx_open;
	dev->stop		= &wanpipe_xilinx_close;
	dev->hard_start_xmit	= &wanpipe_xilinx_send;
	dev->get_stats		= &wanpipe_xilinx_ifstats;
# if defined(LINUX_2_4)||defined(LINUX_2_6)
	dev->tx_timeout		= &wanpipe_xilinx_tx_timeout;
	dev->watchdog_timeo	= TX_TIMEOUT;
# endif
	dev->do_ioctl		= wanpipe_xilinx_ioctl;

	if (chan->common.usedby == BRIDGE ||
       	    chan->common.usedby == BRIDGE_NODE){

		/* Setup the interface for Bridging */
		int hw_addr=0;
		ether_setup(dev);

		/* Use a random number to generate the MAC address */
		memcpy(dev->dev_addr, "\xFE\xFC\x00\x00\x00\x00", 6);
		get_random_bytes(&hw_addr, sizeof(hw_addr));
		*(int *)(dev->dev_addr + 2) += hw_addr;

	}else{

		if (chan->protocol != WANCONFIG_PPP &&
		    chan->protocol != WANCONFIG_CHDLC){
			dev->flags     |= IFF_POINTOPOINT;
			dev->flags     |= IFF_NOARP;
			dev->type	= ARPHRD_PPP;
			dev->mtu		= card->wandev.mtu;
			dev->hard_header_len	= 0;
			dev->hard_header	= NULL; 
			dev->rebuild_header	= NULL;
		}

		/* Enable Mulitcasting if user selected */
		if (chan->mc == WANOPT_YES){
			dev->flags 	|= IFF_MULTICAST;
		}

		if (chan->true_if_encoding){
			dev->type	= ARPHRD_HDLC; /* This breaks the tcpdump */
		}else{
			dev->type	= ARPHRD_PPP;
		}
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
#endif

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
static int wanpipe_xilinx_open (netdevice_t* dev)
{
	private_area_t* chan = dev->priv;
	sdla_t* card = (sdla_t*)chan->common.card;
	struct timeval tv;
	int err = 0;

	/* Only one open per interface is allowed */
	if (open_dev_check(dev))
		return -EBUSY;

	/* Initialize the router start time.
	 * Used by wanpipemon debugger to indicate
	 * how long has the interface been up */
	do_gettimeofday(&tv);
	chan->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

        /* If FRONT End is down, it means that the DMA
         * is disabled.  In this case don't try to
         * reset fifo.  Let the enable_data_error_intr()
         * function do this, after front end has come up */

	if (card->wandev.state == WAN_CONNECTED){
	
		xilinx_init_rx_dev_fifo(card,chan,WP_WAIT);
		xilinx_init_tx_dev_fifo(card,chan,WP_WAIT);
		xilinx_init_tx_dma_descr(card,chan);

		err=xilinx_dma_rx(card,chan);
		if (err){
			return err;
		}
	}

	/* Check for transparent HDLC mode */
	if (!chan->hdlc_eng){

		u32 reg=0,reg1;

		/* The Transparent HDLC engine is
	         * enabled.  The Rx dma has already
        	 * been setup above.  Now setup
         	* TX DMA and enable the HDLC engine */

		xilinx_dma_tx(card,chan);

		DEBUG_CFG("%s: Transparent Tx Enabled!\n",
			dev->name);

		 /* Select an HDLC logic channel for configuration */
        	card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);

        	reg&=~HDLC_LOGIC_CH_BIT_MASK;
        	reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

        	card->hw_iface.bus_write_4(card->hw,
                        XILINX_TIMESLOT_HDLC_CHAN_REG,
                        (reg|(chan->logic_ch_num&HDLC_LOGIC_CH_BIT_MASK)));

		reg=0;

		/* Enable the transparend HDLC
                 * engine. */
                wan_set_bit(HDLC_RX_PROT_DISABLE_BIT,&reg);
                wan_set_bit(HDLC_TX_PROT_DISABLE_BIT,&reg);

	        wan_set_bit(HDLC_TX_CHAN_ENABLE_BIT,&reg);
        	wan_set_bit(HDLC_RX_ADDR_RECOGN_DIS_BIT,&reg);

		card->hw_iface.bus_read_4(card->hw,XILINX_TIMESLOT_HDLC_CHAN_REG,&reg1);

		DEBUG_CFG("%s: Writting to REG(0x64)=0x%X   Reg(0x60)=0x%X\n",
			dev->name,reg,reg1);

		xilinx_write_ctrl_hdlc(card,
                                       chan->first_time_slot,
                                       XILINX_HDLC_CONTROL_REG,
                                       reg);
		if (err){
 			DEBUG_EVENT("%s:%d wait for timeslot failed\n",__FUNCTION__,__LINE__);
			return err;
		}
	}

	xilinx_dev_enable(card,chan);

	chan->ignore_modem=0x0F;


	/* Increment the module usage count */
	wanpipe_open(card);

        if (card->wandev.state == WAN_CONNECTED){

                /* If Front End is connected already set interface
                 * state to Connected too */
                set_chan_state(card, dev, WAN_CONNECTED);
        }

	DEBUG_TEST("%s: Opened!\n",dev->name);
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
static int wanpipe_xilinx_close (netdevice_t* dev)
{
	private_area_t* chan = dev->priv;
	sdla_t* card = (sdla_t*)chan->common.card;

	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	chan->common.state = WAN_DISCONNECTED;
	
	xilinx_dev_close(card,chan);
	chan->ignore_modem=0x00;

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
	unsigned long flags;

	wan_set_bit(CARD_DOWN,&card->wandev.critical);

	wan_del_timer(&card->u.xilinx.led_timer);
	
	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
	}

	wan_spin_lock_irq(&card->wandev.lock,&flags);

	card->isr=NULL;

	/* Disable DMA ENGINE before we perform 
         * core reset.  Otherwise, we will receive
         * rx fifo errors on subsequent resetart. */
	disable_data_error_intr(card,DEVICE_DOWN);
	
	wan_spin_unlock_irq(&card->wandev.lock,&flags);

	udelay(10);

	xilinx_chip_unconfigure(card);

#if defined(WAN_DEBUG_MEM)
        DEBUG_EVENT("%s: Total Mem %d\n",__FUNCTION__,wan_atomic_read(&wan_debug_mem));
#endif

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
static void wanpipe_xilinx_tx_timeout (netdevice_t* dev)
{
    	private_area_t* chan = dev->priv;
	sdla_t *card = (sdla_t*)chan->common.card;

	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this
	 * is only used as a last resort.
	 */

	++chan->if_stats.collisions;

	DEBUG_EVENT( "%s: Transmit timed out on %s\n", card->devname,dev->name);
	DEBUG_EVENT("%s: TxStatus=0x%X  DMAADDR=0x%lX  DMALEN=%i \n",
			chan->if_name,
			chan->dma_status,
			chan->tx_dma_addr,
			chan->tx_dma_len);
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
static int wanpipe_xilinx_send (netskb_t* skb, netdevice_t* dev)
{

	private_area_t *chan = dev->priv;
	sdla_t *card = (sdla_t*)chan->common.card;

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
                ++chan->if_stats.collisions;
		if((jiffies - chan->tick_counter) < (5 * HZ)) {
			return 1;
		}

		if_tx_timeout(dev);
	}
#endif

	if (chan->common.state != WAN_CONNECTED){
		++chan->if_stats.tx_carrier_errors;
		wan_skb_free(skb);

	}else if (chan->protocol == WANCONFIG_FR && chan->prot_state != WAN_CONNECTED){
		++chan->if_stats.tx_carrier_errors;
		wan_skb_free(skb);

	}else {

		if (chan->common.usedby == API){
			api_tx_hdr_t *api_tx_hdr = (api_tx_hdr_t *)skb->data;
			if (sizeof(api_tx_hdr_t) >= skb->len){
				wan_skb_free(skb);
				++chan->if_stats.tx_dropped;
				goto if_send_exit_crit;
			}

			skb->csum=api_tx_hdr->ss7_prot_len;

			wan_skb_pull(skb,sizeof(api_tx_hdr_t));
		}

		if (wan_skb_queue_len(&chan->wp_tx_pending_list) > MAX_TX_BUF){
			stop_net_queue(dev);
			xilinx_dma_tx(card,chan);
			return 1;
		}else{
#if 0
			chan->if_stats.tx_errors++;
#endif
			skb_unlink(skb);

			wan_skb_queue_tail(&chan->wp_tx_pending_list,skb);
			xilinx_dma_tx(card,chan);
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		 	dev->trans_start = jiffies;
#endif
		}
	}

if_send_exit_crit:

	start_net_queue(dev);
	return 0;
}


#if 0
/*============================================================================
 * chk_bcast_mcast_addr - Check for source broadcast addresses
 *
 * Check to see if the packet to be transmitted contains a broadcast or
 * multicast source IP address.
 */

static int chk_bcast_mcast_addr(sdla_t *card, netdevice_t* dev,
				netskb_t *skb)
{
	u32 src_ip_addr;
        u32 broadcast_ip_addr = 0;
	private_area_t *chan=dev->priv;
        struct in_device *in_dev;
        /* read the IP source address from the outgoing packet */
        src_ip_addr = *(u32 *)(skb->data + 12);

	if (chan->common.usedby != WANPIPE){
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

#endif

/*============================================================================
 * if_stats
 *
 * Used by /proc/net/dev and ifconfig to obtain interface
 * statistics.
 *
 * Return a pointer to struct net_device_stats.
 */
static struct net_device_stats* wanpipe_xilinx_ifstats (netdevice_t* dev)
{
	private_area_t* chan;

	if ((chan=dev->priv) == NULL)
		return NULL;

#if 0
	 DEBUG_EVENT("%s: RxCompList=%i RxFreeList=%i  TxList=%i\n",
                        dev->name,
                        wan_skb_queue_len(&chan->wp_rx_complete_list),
                        wan_skb_queue_len(&chan->wp_rx_free_list),
                        wan_skb_queue_len(&chan->wp_tx_pending_list));
#endif


	return &chan->if_stats;
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
static int 
wanpipe_xilinx_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	private_area_t* chan= (private_area_t*)dev->priv;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	unsigned long smp_flags;
	int err=0;

	if (!chan){
		return -ENODEV;
	}
	card=(sdla_t*)chan->common.card;

	NET_ADMIN_CHECK();

	switch(cmd)
	{

		case SIOC_WANPIPE_BIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}
			
			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

			//NENAD FIXME: TAKE IT OUT
			chan->if_stats.tx_errors=0;

			break;

		case SIOC_WANPIPE_UNBIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}

			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
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


		case SDLA_HDLC_READ_REG:
			err=xilinx_read(card,ifr);
			break;

		case SDLA_HDLC_WRITE_REG:
			err=xilinx_write(card,ifr);
			break;

		
		case SDLA_HDLC_SET_PCI_BIOS:
			err=xilinx_write_bios(card,ifr);
			break;
		
		case SIOC_WANPIPE_PIPEMON:

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
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (wan_test_bit(0,&card->in_isr)) {
				DEBUG_EVENT( "%s:%s Pipemon command failed, Driver busy: try again.\n",
						card->devname,dev->name);
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EBUSY;
			}

			process_udp_mgmt_pkt(card,dev,chan,1);

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
			DEBUG_EVENT("%s: Command %x not supported!\n",
				card->devname,cmd);
			return -EOPNOTSUPP;
	}

	return err;
}


/**SECTION**********************************************************
 *
 * 	FIRMWARE Specific Interface Functions
 *
 *******************************************************************/


/*============================================================================
 * xilinx_chip_configure
 *
 *
 */

static int xilinx_chip_configure(sdla_t *card)
{
	u32 reg,tmp;
	int err=0;
	u16 adapter_type,adptr_security;

    	DEBUG_CFG("Xilinx Chip Configuration. -- \n");

	xilinx_delay(1);

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
	
	/* Configure for T1 or E1 front end */
	if (IS_T1_CARD(card)){
		card->u.xilinx.num_of_time_slots=NUM_OF_T1_CHANNELS;
		wan_clear_bit(INTERFACE_TYPE_T1_E1_BIT,&reg);
		wan_set_bit(FRONT_END_FRAME_FLAG_ENABLE_BIT,&reg);
	}else if (IS_E1_CARD(card)){
		card->u.xilinx.num_of_time_slots=NUM_OF_E1_CHANNELS;
		wan_set_bit(INTERFACE_TYPE_T1_E1_BIT,&reg);
		wan_set_bit(FRONT_END_FRAME_FLAG_ENABLE_BIT,&reg);
	}else{
		DEBUG_ERROR("%s: Error: Xilinx doesn't support non T1/E1 interface!\n",
				card->devname);
		return -EINVAL;
	}

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	udelay(10000);

        /* Reset PMC */
        card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
        wan_clear_bit(FRONT_END_RESET_BIT,&reg);
        card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
        udelay(1000);

        wan_set_bit(FRONT_END_RESET_BIT,&reg);
        card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
        udelay(100);


	DEBUG_CFG("--- Chip Reset. -- \n");

	/* Reset Chip Core */
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);
	wan_set_bit(CHIP_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	udelay(100);

	/* Disable the chip/hdlc reset condition */
	wan_clear_bit(CHIP_RESET_BIT,&reg);

	/* Disable ALL chip interrupts */
	wan_clear_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(FRONT_END_INTR_ENABLE_BIT,&reg);

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	xilinx_delay(1);

#if 1
	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &adapter_type);
	udelay(100);

	
	/* Sanity check, where we make sure that A101
         * adapter gracefully fails on non existing
         * secondary port */
	if (adapter_type == A101_1TE1_SUBSYS_VENDOR &&
            card->wandev.S514_cpu_no[0] == SDLA_CPU_B){
		DEBUG_EVENT("%s: Hardware Config Mismatch: A103 Adapter not found!\n",
			card->devname);
		//return -ENODEV;
	}

	DEBUG_EVENT("%s: Hardware Adapter Type 0x%X\n",
			card->devname,adapter_type);

	adptr_security = read_cpld(card,SECURITY_CPLD_REG);
	adptr_security = adptr_security >> SECURITY_CPLD_SHIFT;
	adptr_security = adptr_security & SECURITY_CPLD_MASK;

	switch(adptr_security){

	case SECURITY_1LINE_UNCH:
		DEBUG_EVENT("%s: Security 1 Line UnCh\n",
			card->devname);
		break;
	case SECURITY_1LINE_CH:
		DEBUG_EVENT("%s: Security 1 Line Ch\n",
                        card->devname);
		break;
	case SECURITY_2LINE_UNCH:
		DEBUG_EVENT("%s: Security 2 Line UnCh\n",
                        card->devname);
		break;
	case SECURITY_2LINE_CH:
		DEBUG_EVENT("%s: Security 2 Line Ch\n",
                        card->devname);
		break;
	default:
		DEBUG_ERROR("%s: Error Invalid Security ID=0x%X\n",
                        card->devname,adptr_security);
		//return -EINVAL;
	}

#endif

	/* Turn off Onboard RED LED */
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);		
	wan_set_bit(XILINX_RED_LED,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
	udelay(10);

	err=aft_core_ready(card);
	if (err != 0){
		DEBUG_EVENT("%s: WARNING: HDLC Core Not Ready: B4 TE CFG!\n",
                                        card->devname);

	}
	
	DEBUG_EVENT("%s: Configuring A101 PMC T1/E1/J1 Front End\n",
                	        card->devname);
	err = -EINVAL;
	if (card->wandev.fe_iface.config){
		err = card->wandev.fe_iface.config(&card->fe);
	}
	if (err){
       		DEBUG_EVENT("%s: Failed %s configuratoin!\n",
                                	card->devname,
                                	(IS_T1_CARD(card))?"T1":"E1");
               	return -EINVAL;
       	}

	xilinx_delay(1);

	err=aft_core_ready(card);
	if (err != 0){
		DEBUG_ERROR("%s: Error: HDLC Core Not Ready!\n",
					card->devname);

		card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

		/* Disable the chip/hdlc reset condition */
		wan_set_bit(CHIP_RESET_BIT,&reg);

		card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
		return err;
    	} else{
		DEBUG_CFG("%s: HDLC Core Ready 0x%08X\n",
                                        card->devname,reg);
    	}

	xilinx_delay(1);

	/* Setup global DMA parameters */
	reg=0;
	reg|=(XILINX_DMA_SIZE    << DMA_SIZE_BIT_SHIFT);
	reg|=(XILINX_DMA_FIFO_UP << DMA_FIFO_HI_MARK_BIT_SHIFT);
	reg|=(XILINX_DMA_FIFO_LO << DMA_FIFO_LO_MARK_BIT_SHIFT);

	/* Enable global DMA engine and set to default
	 * number of active channels. Note: this value will
	 * change in dev configuration */
	reg|=(XILINX_DEFLT_ACTIVE_CH << DMA_ACTIVE_CHANNEL_BIT_SHIFT);
	wan_set_bit(DMA_ENGINE_ENABLE_BIT,&reg);

    	DEBUG_CFG("--- Setup DMA control Reg. -- \n");

	card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
	DEBUG_CFG("--- Tx/Rx global enable. -- \n");

	xilinx_delay(1);

	reg=0;
	card->hw_iface.bus_write_4(card->hw,XILINX_TIMESLOT_HDLC_CHAN_REG,reg);

    	/* Clear interrupt pending registers befor first interrupt enable */
	card->hw_iface.bus_read_4(card->hw, XILINX_DMA_RX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw, XILINX_DMA_TX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, (u32*)&reg);
	if (wan_test_bit(DMA_INTR_FLAG,&reg)){
        	DEBUG_ERROR("%s: Error: Active DMA Interrupt Pending. !\n",
					card->devname);

	        reg = 0;
		/* Disable the chip/hdlc reset condition */
		wan_set_bit(CHIP_RESET_BIT,&reg);
		card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
        	return err;
	}
	if (wan_test_bit(ERROR_INTR_FLAG,&reg)){
        	DEBUG_ERROR("%s: Error: Active Error Interrupt Pending. !\n",
					card->devname);

		reg = 0;
		/* Disable the chip/hdlc reset condition */
		wan_set_bit(CHIP_RESET_BIT,&reg);
		card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
		return err;
	}


	/* Alawys disable global data and error
         * interrupts */
    	wan_clear_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg); 

	/* Always enable the front end interrupt */

	wan_set_bit(FRONT_END_INTR_ENABLE_BIT,&reg);

    	DEBUG_CFG("--- Set Global Interrupts (0x%X)-- \n",reg);

	xilinx_delay(1);


	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	DEBUG_CFG("--- Set Global Interrupt enabled. -- \n");

	return err;
}

/*============================================================================
 * xilinx_chip_configure
 *
 *
 */

static int xilinx_chip_unconfigure(sdla_t *card)
{
	u32	reg = 0;
 
        card->hw_iface.bus_write_4(card->hw,XILINX_TIMESLOT_HDLC_CHAN_REG,reg);
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);
	/* Enable the chip/hdlc reset condition */
	reg=0;
	wan_set_bit(CHIP_RESET_BIT,&reg);

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
	return 0;
}


/*============================================================================
 * xilinx_dev_configure
 *
 *
 */

static int xilinx_dev_configure(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	long free_logic_ch,i;

    	DEBUG_TEST("-- Configure Xilinx. --\n");

	chan->logic_ch_num=-1;

	if (!IS_TE1_CARD(card)){
		return -EINVAL;
	}

	if (IS_E1_CARD(card)){
		DEBUG_CFG("%s: Time Slot Orig 0x%lX  Shifted 0x%lX\n",
			chan->if_name,
			chan->time_slot_map,
			chan->time_slot_map<<1);
		chan->time_slot_map=chan->time_slot_map<<1;
		wan_clear_bit(0,&chan->time_slot_map);
	}

	/* Channel definition section. If not channels defined
	 * return error */
	if (chan->time_slot_map == 0){
		printk(KERN_INFO "%s: Invalid Channel Selection 0x%lX\n",
				card->devname,chan->time_slot_map);
		return -EINVAL;
	}

	DEBUG_EVENT("%s:%s: Active channels = 0x%lX\n",
		card->devname,chan->if_name,chan->time_slot_map);

	xilinx_delay(1);

	/* Check that the time slot is not being used. If it is
	 * stop the interface setup.  Notice, though we proceed
	 * to check for all timeslots before we start binding
	 * the channels in.  This way, we don't have to go back
	 * and clean the time_slot_map */
	for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
		if (wan_test_bit(i,&chan->time_slot_map)){

			if (chan->first_time_slot == -1){
				DEBUG_EVENT("%s:%s: Setting first time slot to %ld\n",
						card->devname,chan->if_name,i);
				chan->first_time_slot=i;
			}

			DEBUG_CFG("%s: Confiuring %s for timeslot %li\n",
					card->devname, chan->if_name, 
				        IS_E1_CARD(card)?i:i+1);

			if (wan_test_bit(i,&card->u.xilinx.time_slot_map)){
				printk(KERN_INFO "%s: Channel/Time Slot resource conflict!\n",
						card->devname);
				printk(KERN_INFO "%s: %s: Channel/Time Slot %li, aready in use!\n",
						card->devname,chan->if_name,(i+1));

				return -EEXIST;
			}

			/* Calculate the number of timeslots for this
                         * interface */
			++chan->num_of_time_slots;
		}
	}

	xilinx_delay(1);

	chan->logic_ch_num=request_xilinx_logical_channel_num(card, chan, &free_logic_ch);
	if (chan->logic_ch_num == -1){
		return -EBUSY;
	}

	xilinx_delay(1);

	DEBUG_TEST("%s:%d: GOT Logic ch %li  Free Logic ch %li\n",
		__FUNCTION__,__LINE__,chan->logic_ch_num,free_logic_ch);

	xilinx_delay(1);

	for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
		if (wan_test_bit(i,&chan->time_slot_map)){

			wan_set_bit(i,&card->u.xilinx.time_slot_map);

			card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);
			reg&=~TIMESLOT_BIT_MASK;

			//FIXME do not hardcode !
            		reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */


			/* Select a Timeslot for configuration */
			card->hw_iface.bus_write_4(card->hw,
				       XILINX_TIMESLOT_HDLC_CHAN_REG,
		                       (reg|(i<<TIMESLOT_BIT_SHIFT)));
		

			reg=chan->logic_ch_num&CONTROL_RAM_DATA_MASK;

#ifdef TRUE_FIFO_SIZE
			reg|=(chan->fifo_size_code&HDLC_FIFO_SIZE_MASK)<<HDLC_FIFO_SIZE_SHIFT;
#else
	
			reg|=(HARD_FIFO_CODE&HDLC_FIFO_SIZE_MASK)<<HDLC_FIFO_SIZE_SHIFT;
#endif

			reg|=(chan->fifo_base_addr&HDLC_FIFO_BASE_ADDR_MASK)<<
						HDLC_FIFO_BASE_ADDR_SHIFT;

			if (!chan->hdlc_eng){
				wan_set_bit(TRANSPARENT_MODE_BIT,&reg);
			}

			DEBUG_TEST("Setting Timeslot %li to logic ch %li Reg=0x%X\n",
				        i, chan->logic_ch_num,reg);

			xilinx_write_ctrl_hdlc(card, 
                                               i,
                                               XILINX_CONTROL_RAM_ACCESS_BUF,
					       reg);

		}
	}

	if (free_logic_ch != -1){

		char free_ch_used=0;
#if 0
		if (wan_atomic_read(&card->wandev.if_cnt)==3){
			free_logic_ch=4;
		}
#endif
		for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
			if (!wan_test_bit(i,&card->u.xilinx.time_slot_map)){

				card->hw_iface.bus_read_4(card->hw, 
                                                          XILINX_TIMESLOT_HDLC_CHAN_REG, 
							  &reg);

        		        reg&=~TIMESLOT_BIT_MASK;
		                reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

				/* Select a Timeslot for configuration */
				card->hw_iface.bus_write_4(card->hw,
					       XILINX_TIMESLOT_HDLC_CHAN_REG,
					       (reg|(i<<TIMESLOT_BIT_SHIFT)));


				reg=free_logic_ch&CONTROL_RAM_DATA_MASK;

				/* For the rest of the unused logic channels
                                 * bind them to timeslot 31 and set the fifo
                                 * size to 32 byte = Code=0x00 */

	                        reg|=(FIFO_32B&HDLC_FIFO_SIZE_MASK)<<HDLC_FIFO_SIZE_SHIFT;

                        	reg|=(free_logic_ch&HDLC_FIFO_BASE_ADDR_MASK)<<
                                                HDLC_FIFO_BASE_ADDR_SHIFT;


				DEBUG_TEST("Setting Timeslot %li to free logic ch %li Reg=0x%X\n",
                                        i, free_logic_ch,reg);

				xilinx_write_ctrl_hdlc(card, 
                                                       i,
                                                       XILINX_CONTROL_RAM_ACCESS_BUF,
						       reg);

				free_ch_used=1;
			}
		}


		/* We must check if the free logic has been bound
                 * to any timeslots */
		if (free_ch_used){

			DEBUG_CFG("%s: Setting Free CH %ld to idle\n",
					chan->if_name,free_logic_ch);

			xilinx_delay(1);

			/* Setup the free logic channel as IDLE */

			card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);

			reg&=~HDLC_LOGIC_CH_BIT_MASK;
	        	reg&=HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

			card->hw_iface.bus_write_4(card->hw,
                        	XILINX_TIMESLOT_HDLC_CHAN_REG,
                        	(reg|(free_logic_ch&HDLC_LOGIC_CH_BIT_MASK)));

			reg=0;
			wan_clear_bit(HDLC_RX_PROT_DISABLE_BIT,&reg);
                	wan_clear_bit(HDLC_TX_PROT_DISABLE_BIT,&reg);

	        	wan_set_bit(HDLC_RX_ADDR_RECOGN_DIS_BIT,&reg);

			xilinx_write_ctrl_hdlc(card,
                                               chan->first_time_slot,
                                               XILINX_HDLC_CONTROL_REG,
                                               reg);
		}
	}

	/* Select an HDLC logic channel for configuration */
	card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);

	reg&=~HDLC_LOGIC_CH_BIT_MASK;
    	reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

	card->hw_iface.bus_write_4(card->hw,
			XILINX_TIMESLOT_HDLC_CHAN_REG,
			(reg|(chan->logic_ch_num&HDLC_LOGIC_CH_BIT_MASK)));

	reg=0;

	if (chan->hdlc_eng){
		/* HDLC engine is enabled on the above logical channels */
		wan_clear_bit(HDLC_RX_PROT_DISABLE_BIT,&reg);
		wan_clear_bit(HDLC_TX_PROT_DISABLE_BIT,&reg);
		DEBUG_EVENT("%s:%s: Config for HDLC mode\n",
                        card->devname,chan->if_name);
	}else{

		/* Transprent Mode */

		/* Do not start HDLC Core here, because
                 * we have to setup Tx/Rx DMA buffers first
	         * The transparent mode, will start
                 * comms as soon as the HDLC is enabled */


		xilinx_write_ctrl_hdlc(card,
                                       chan->first_time_slot,
                                       XILINX_HDLC_CONTROL_REG,
                                       0);
		return 0;
	}

	wan_set_bit(HDLC_TX_CHAN_ENABLE_BIT,&reg);
	wan_set_bit(HDLC_RX_ADDR_RECOGN_DIS_BIT,&reg);

	xilinx_write_ctrl_hdlc(card,
                               chan->first_time_slot,
                               XILINX_HDLC_CONTROL_REG,
                               reg);

	return 0;
}


static void xilinx_dev_unconfigure(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	int i;
	unsigned long smp_flags;


	DEBUG_CFG("\n-- Unconfigure Xilinx. --\n");

	/* Select an HDLC logic channel for configuration */
	if (chan->logic_ch_num != -1){

		card->hw_iface.bus_read_4(card->hw,XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);
		reg&=~HDLC_LOGIC_CH_BIT_MASK;
		reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

		card->hw_iface.bus_write_4(card->hw,
				XILINX_TIMESLOT_HDLC_CHAN_REG,
				(reg|(chan->logic_ch_num&HDLC_LOGIC_CH_BIT_MASK)));


		reg=0x00020000;
	        xilinx_write_ctrl_hdlc(card,
                               chan->first_time_slot,
                               XILINX_HDLC_CONTROL_REG,
                               reg);


	        for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
        	        if (wan_test_bit(i,&chan->time_slot_map)){

                        	card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);
                        	reg&=~TIMESLOT_BIT_MASK;
                        	reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

                        	/* Select a Timeslot for configuration */
                        	card->hw_iface.bus_write_4(card->hw,
                                       XILINX_TIMESLOT_HDLC_CHAN_REG,
                                       (reg|(i<<TIMESLOT_BIT_SHIFT)));


                        	reg=31&CONTROL_RAM_DATA_MASK;

                        	reg|=(FIFO_32B&HDLC_FIFO_SIZE_MASK)<<HDLC_FIFO_SIZE_SHIFT;
                        	reg|=(31&HDLC_FIFO_BASE_ADDR_MASK)<<
                                                HDLC_FIFO_BASE_ADDR_SHIFT;

                        	DEBUG_TEST("Setting Timeslot %i to logic ch %i Reg=0x%X\n",
                                	        i, 31 ,reg);

	                        xilinx_write_ctrl_hdlc(card,
        	                                       i,
                	                               XILINX_CONTROL_RAM_ACCESS_BUF,
                        	                       reg);
			}
		}

		/* Lock to protect the logic ch map to 
	         * chan device array */
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		free_xilinx_logical_channel_num(card,chan->logic_ch_num);
		free_fifo_baddr_and_size(card,chan);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

		chan->logic_ch_num=-1;

		for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
			if (wan_test_bit(i,&chan->time_slot_map)){
				wan_clear_bit(i,&card->u.xilinx.time_slot_map);
			}
		}
	}
}

#define FIFO_RESET_TIMEOUT_CNT 1000
#define FIFO_RESET_TIMEOUT_US  10
static int xilinx_init_rx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char wait)
{

        u32 reg;
        u32 dma_descr;
	u8  timeout=1;
	u16 i;

        /* Clean RX DMA fifo */
        dma_descr=(unsigned long)(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
        reg=0;
        wan_set_bit(INIT_DMA_FIFO_CMD_BIT,&reg);

        DEBUG_TEST("%s: Clearing RX Fifo %s DmaDescr=(0x%X) Reg=(0x%X)\n",
                                __FUNCTION__,chan->if_name,
                                dma_descr,reg);

       	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	if (wait == WP_WAIT){
		for(i=0;i<FIFO_RESET_TIMEOUT_CNT;i++){
			card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);
			if (wan_test_bit(INIT_DMA_FIFO_CMD_BIT,&reg)){
				udelay(FIFO_RESET_TIMEOUT_US);
				continue;
			}
			timeout=0;
			break;
		} 

		if (timeout){
			DEBUG_ERROR("%s:%s: Error: Rx fifo reset timedout %u us\n",
				card->devname,chan->if_name,i*FIFO_RESET_TIMEOUT_US);
		}else{
			DEBUG_TEST("%s:%s: Rx Fifo reset successful %u us\n",
				card->devname,chan->if_name,i*FIFO_RESET_TIMEOUT_US); 
		}
	}else{
		timeout=0;
	}

	return timeout;
}

static int xilinx_init_tx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char wait)
{
	u32 reg;
        u32 dma_descr;
        u8  timeout=1;
	u16 i;

        /* Clean TX DMA fifo */
        dma_descr=(unsigned long)(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
        reg=0;
        wan_set_bit(INIT_DMA_FIFO_CMD_BIT,&reg);

        DEBUG_TEST("%s: Clearing TX Fifo %s DmaDescr=(0x%X) Reg=(0x%X)\n",
                                __FUNCTION__,chan->if_name,
                                dma_descr,reg);

        card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	if (wait == WP_WAIT){
        	for(i=0;i<FIFO_RESET_TIMEOUT_CNT;i++){
                	card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);
                	if (wan_test_bit(INIT_DMA_FIFO_CMD_BIT,&reg)){
                        	udelay(FIFO_RESET_TIMEOUT_US);
                        	continue;
                	}
               		timeout=0;
               		break;
        	}

        	if (timeout){
                	DEBUG_ERROR("%s:%s: Error: Tx fifo reset timedout %u us\n",
                                card->devname,chan->if_name,i*FIFO_RESET_TIMEOUT_US);
        	}else{
                	DEBUG_TEST("%s:%s: Tx Fifo reset successful %u us\n",
                                card->devname,chan->if_name,i*FIFO_RESET_TIMEOUT_US);
        	}
	}else{
		timeout=0;
	}

	return timeout;
}


static void xilinx_dev_enable(sdla_t *card, private_area_t *chan)
{
        u32 reg;

	DEBUG_TEST("%s: Enabling Global Inter Mask !\n",chan->if_name);

	/* Enable Logic Channel Interrupts for DMA and fifo */
	card->hw_iface.bus_read_4(card->hw,
                                  XILINX_GLOBAL_INTER_MASK, &reg);
	wan_set_bit(chan->logic_ch_num,&reg);

	card->hw_iface.bus_write_4(card->hw,
                                  XILINX_GLOBAL_INTER_MASK, reg);

	wan_set_bit(chan->logic_ch_num,&card->u.xilinx.active_ch_map);
}

static int xilinx_dma_rx(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	unsigned long dma_descr;
	unsigned long bus_addr;
	wp_rx_element_t *rx_el;


//    	DEBUG_RX("%s: Setup Rx DMA descriptor \n",chan->if_name);

	/* sanity check: make sure that DMA is in ready state */
#if 0
	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg);

	if (wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg)){
		DEBUG_ERROR("%s: Error: RxDMA GO Ready bit set on dma Rx\n",
				card->devname);
		return -EFAULT;
	}
#endif

	if (chan->rx_dma_skb){
		DEBUG_ERROR("%s: Critial Error: Rx Dma Buf busy!\n",chan->if_name);
		return -EINVAL;
	}

	chan->rx_dma_skb = wan_skb_dequeue(&chan->wp_rx_free_list);

	if (!chan->rx_dma_skb){
		DEBUG_ERROR("%s: Critical Error no rx dma buf Free=%i Comp=%i!\n",
				chan->if_name,wan_skb_queue_len(&chan->wp_rx_free_list),
				wan_skb_queue_len(&chan->wp_rx_complete_list));

		aft_reinit_pending_rx_bufs(chan);

		chan->rx_dma_skb = wan_skb_dequeue(&chan->wp_rx_free_list);
		if (!chan->rx_dma_skb){
			return -ENOMEM;
		}
#if 0
		aft_alloc_rx_dma_buff(card,chan,5);
		
		chan->rx_dma_skb = wan_skb_dequeue(&chan->wp_rx_free_list);
		if (!chan->rx_dma_skb){
			DEBUG_ERROR("%s: Critical Error no STILL NO MEM!\n",
					chan->if_name);
			return -ENOMEM;
		}
#endif
	}

	rx_el = (wp_rx_element_t *)&chan->rx_dma_skb->cb[0];
	memset(rx_el,0,sizeof(wp_rx_element_t));
	
	bus_addr = cpu_to_le32(pci_map_single(NULL,
		      	       		chan->rx_dma_skb->tail,
					chan->dma_mru,
		    	       		PCI_DMA_FROMDEVICE)); 	

	if (!bus_addr){
		DEBUG_ERROR("%s: %s Critical error pci_map_single() failed!\n",
				chan->if_name,__FUNCTION__);
		return -EINVAL;
	}
	
	rx_el->dma_addr=bus_addr;

	/* Write the pointer of the data packet to the
	 * DMA address register */
	reg=bus_addr;

	/* Set the 32bit alignment of the data length.
	 * Since we are setting up for rx, set this value
	 * to Zero */
	reg&=~(RxDMA_LO_ALIGNMENT_BIT_MASK);

    	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_LO;

//	DEBUG_RX("%s: RxDMA_LO = 0x%X, BusAddr=0x%X DmaDescr=0x%X\n",
//		__FUNCTION__,reg,bus_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	dma_descr=(unsigned long)(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;

    	reg =0;

	if (chan->hdlc_eng){
		reg|=(chan->dma_mru>>2)&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	}else{
		reg|=(card->u.xilinx.cfg.mru>>2)&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	}

#ifdef TRUE_FIFO_SIZE
	reg|=(chan->fifo_size_code&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
#else

	reg|=(HARD_FIFO_CODE&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
#endif
        reg|=(chan->fifo_base_addr&DMA_FIFO_BASE_ADDR_MASK)<<
                              DMA_FIFO_BASE_ADDR_SHIFT;	

	wan_set_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg);

	DEBUG_RX("%s: RXDMA_HI = 0x%X, BusAddr=0x%X DmaDescr=0x%X\n",
 	             __FUNCTION__,reg,bus_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	
	wan_set_bit(0,&chan->rx_dma);
	
	return 0;
}

static void xilinx_dev_close(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	unsigned long dma_descr;
	unsigned long smp_flags;

    	DEBUG_CFG("-- Close Xilinx device. --\n");

        /* Disable Logic Channel Interrupts for DMA and fifo */
        card->hw_iface.bus_read_4(card->hw,
                                  XILINX_GLOBAL_INTER_MASK, &reg);

        wan_clear_bit(chan->logic_ch_num,&reg);
	wan_clear_bit(chan->logic_ch_num,&card->u.xilinx.active_ch_map);

	/* We are masking the chan interrupt. 
         * Lock to make sure that the interrupt is
         * not running */
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
        card->hw_iface.bus_write_4(card->hw,
                                  XILINX_GLOBAL_INTER_MASK, reg);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	reg=0;

       /* Select an HDLC logic channel for configuration */
 	card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);
	
       	reg&=~HDLC_LOGIC_CH_BIT_MASK;
       	reg&= HDLC_LCH_TIMESLOT_MASK;         /* mask not valid bits */

       	card->hw_iface.bus_write_4(card->hw,
                       XILINX_TIMESLOT_HDLC_CHAN_REG,
                       (reg|(chan->logic_ch_num&HDLC_LOGIC_CH_BIT_MASK)));


	reg=0;
	xilinx_write_ctrl_hdlc(card,
                               chan->first_time_slot,
                               XILINX_HDLC_CONTROL_REG,
                               reg);

    	/* Clear descriptors */
	reg=0;
	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);
	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	/* FIXME: Cleanp up Tx and Rx buffers */
}


static int xilinx_dma_tx (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
	netskb_t *skb;
	unsigned long dma_descr;
	unsigned char len_align=0;
	int len=0;
	int ss7_len=0;
	unsigned char ss7_len_align=0;

    	DEBUG_TX("------ Setup Tx DMA descriptor. --\n");

	if (wan_test_and_set_bit(TX_BUSY,&chan->dma_status)){
		DEBUG_TX("%s:%d:  TX_BUSY set!\n",
			__FUNCTION__,__LINE__);
		return -EBUSY;
	}


	/* Free the previously skb dma mapping. 
         * In this case the tx interrupt didn't finish
         * and we must re-transmit.*/

        if (chan->tx_dma_addr && chan->tx_dma_len){
                 pci_unmap_single(NULL,
                                 chan->tx_dma_addr,
                                 chan->tx_dma_len,
                                 PCI_DMA_TODEVICE);

		DEBUG_EVENT("%s: Unmaping tx_dma_addr in %s\n",
                                chan->if_name,__FUNCTION__);

                chan->tx_dma_addr=0;
		chan->tx_dma_len=0;
        }

	/* Free the previously sent tx packet. To
         * minimize tx isr, the previously transmitted
         * packet is deallocated here */
	if (chan->tx_dma_skb){
		DEBUG_EVENT("%s: Deallocating tx_dma_skb in %s\n",
				chan->if_name,__FUNCTION__);
		wan_skb_free(chan->tx_dma_skb);
		chan->tx_dma_skb=NULL;
	}


	/* check queue pointers befor start transmittion */ 

	/* sanity check: make sure that DMA is in ready state */
	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;

	DEBUG_TX("%s:%d: chan logic ch=%li dma_descr=0x%lx set!\n",
                        __FUNCTION__,__LINE__,chan->logic_ch_num,dma_descr);

	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg);

	if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
		DEBUG_ERROR("%s: Error: TxDMA GO Ready bit set on dma Tx 0x%X\n",
				card->devname,reg);
		wan_clear_bit(TX_BUSY,&chan->dma_status);
		return -EFAULT;
	}

	if (chan->common.usedby == TDM_VOICE){
		skb = NULL;
	}else{
		skb=wan_skb_dequeue(&chan->wp_tx_pending_list);
	}


	if (!skb){
		if (chan->hdlc_eng){
			DEBUG_TEST("%s:%d: Tx pending list empty \n",
                       		 __FUNCTION__,__LINE__);
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			return -ENOBUFS;
		}
		
		/* Transparent HDLC Mode
                 * Transmit Idle Flag */
		len=chan->tx_idle_skb->len;

		chan->tx_dma_addr = pci_map_single(NULL,
					  chan->tx_idle_skb->data,
					  chan->tx_idle_skb->len,
					  PCI_DMA_TODEVICE);
		
		chan->tx_dma_len = chan->tx_idle_skb->len;

		//NENAD FIXME: TAKE IT OUT
		chan->if_stats.tx_errors++;

		
	}else{

		len=skb->len;
		if (skb->len > MAX_XILINX_TX_DMA_SIZE){
			/* FIXME: We need to split this frame into
			 *        multiple parts.  For now thought
			 *        just drop it :) */
			DEBUG_EVENT("%s:%d:  Tx pkt len > MAX XILINX TX DMA LEN!\n",
                	        __FUNCTION__,__LINE__);
			wan_skb_free(skb);
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			return -EINVAL;
		}

		chan->tx_dma_addr = pci_map_single(NULL,
					  skb->data,
					  skb->len,
					  PCI_DMA_TODEVICE); 	
		
		chan->tx_dma_len=skb->len;

		ss7_len=skb->csum;
		if ((chan->tx_dma_addr + (skb->len-ss7_len)) & 0x03){
			DEBUG_ERROR("%s: Error: Tx SS7 Prot Ptr not aligned to 32bit boudary!\n",
					card->devname);

			pci_unmap_single(NULL,
                        	         chan->tx_dma_addr,
                                	 chan->tx_dma_len,
                                 	PCI_DMA_TODEVICE);

			chan->tx_dma_addr=0;
			chan->tx_dma_len=0;
			
			if (skb){
				wan_skb_free(skb);
			}

			wan_clear_bit(TX_BUSY,&chan->dma_status);
			return -EINVAL;
		}

	}

	if (chan->tx_dma_addr & 0x03){
		DEBUG_ERROR("%s: Error: Tx Ptr not aligned to 32bit boudary!\n",
				card->devname);

		pci_unmap_single(NULL,
                                 chan->tx_dma_addr,
                                 chan->tx_dma_len,
                                 PCI_DMA_TODEVICE);


		chan->tx_dma_addr=0;
		chan->tx_dma_len=0;
		
		if (skb){
			wan_skb_free(skb);
		}

		wan_clear_bit(TX_BUSY,&chan->dma_status);
		return -EINVAL;
	}

	if (skb){
		chan->tx_dma_skb=skb;

		DEBUG_TX("TX SKB DATA 0x%08X 0x%08X 0x%08X 0x%08X \n",
			*(unsigned int*)&skb->data[0],
			*(unsigned int*)&skb->data[4],
			*(unsigned int*)&skb->data[8],
			*(unsigned int*)&skb->data[12]);

		DEBUG_TX("Tx dma skb bound %p List=%p Data=%p BusPtr=%lx\n",
			skb,skb->list,skb->data,chan->tx_dma_addr);
	}else{
		chan->tx_dma_skb=NULL;
	}


	/* WARNING: Do ont use the "skb" pointer from
         *          here on.  The skb pointer might not exist if
         *          we are in transparent mode */

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_LO;

	/* Write the pointer of the data packet to the
	 * DMA address register */
	reg=chan->tx_dma_addr;

	/* Set the 32bit alignment of the data length.
	 * Used to pad the tx packet to the 32 bit
	 * boundary */
	reg&=~(TxDMA_LO_ALIGNMENT_BIT_MASK);
	reg|=(len&0x03);

	if (len&0x03){
		len_align=1;
	}

	DEBUG_TX("%s: TXDMA_LO=0x%X PhyAddr=0x%lX DmaDescr=0x%lX\n",
			__FUNCTION__,reg,chan->tx_dma_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;

	/* Setup data length */
	reg=0;
	reg|=(((len>>2)+len_align)&TxDMA_HI_DMA_DATA_LENGTH_MASK);

	/* Setup SS7 protocol frame length */
	if (ss7_len){
		if (ss7_len&0x03){
			ss7_len_align=1;
		}
		reg&=TxDMA_HI_DMA_SS7_FRAME_LEN_MASK;
		reg|=(((ss7_len>>2)+ss7_len_align)&TxDMA_HI_DMA_SS7_FRAME_LEN_MASK)<<
				TxDMA_HI_DMA_SS7_FRAME_LEN_SHIFT;

		/* Set Frame Type */
		wan_set_bit(SS7_PROT_FRAME_TYPE,&reg);

		/* Enable SS7 Protocol in TxDMA */
		wan_set_bit(ENABLE_SS7_PORT_TX,&reg);
	}

	
#ifdef TRUE_FIFO_SIZE
        reg|=(chan->fifo_size_code&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
#else

        reg|=(HARD_FIFO_CODE&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
#endif
        reg|=(chan->fifo_base_addr&DMA_FIFO_BASE_ADDR_MASK)<<
                              DMA_FIFO_BASE_ADDR_SHIFT;

	if (chan->hdlc_eng){

		/* Only enable the Frame Start/Stop on
                 * non-transparent hdlc configuration */

		wan_set_bit(TxDMA_HI_DMA_FRAME_START_BIT,&reg);
		wan_set_bit(TxDMA_HI_DMA_FRAME_END_BIT,&reg);
	}



	

	
	wan_set_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg);

	DEBUG_TX("%s: TXDMA_HI=0x%X DmaDescr=0x%lX\n",
			__FUNCTION__,reg,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);


#if 0
	++chan->if_stats.tx_fifo_errors;
#endif

	return 0;

}

static void xilinx_dma_tx_complete (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
	unsigned long dma_descr;

#if 0
	++chan->if_stats.tx_carrier_errors;
#endif
	/* DEBUGTX */
//      card->hw_iface.bus_read_4(card->hw,0x78, &tmp1); 

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg);

    	if (!chan->tx_dma_skb){

		if (chan->hdlc_eng){
			DEBUG_ERROR("%s: Critical Error: Tx DMA intr: no tx skb !\n",
                    			card->devname);
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			return;
		}else{

			pci_unmap_single(NULL,
				 chan->tx_dma_addr,
				 chan->tx_dma_len,
		                 PCI_DMA_TODEVICE);

			chan->tx_dma_addr=0;
			chan->tx_dma_len=0;
			
			/* For Transparent mode, if no user 
                         * tx frames available, send an idle
                         * frame as soon as possible. 
			 */
        		wan_set_bit(0,&chan->idle_start);

			wan_clear_bit(TX_BUSY,&chan->dma_status);
			xilinx_dma_tx(card,chan);
			return;
		}

	}else{

		pci_unmap_single(NULL,
				 chan->tx_dma_addr,
				 chan->tx_dma_len,
		                 PCI_DMA_TODEVICE);

		chan->tx_dma_addr=0;
		chan->tx_dma_len=0;

	
		if (chan->hdlc_eng){

			/* Do not free the packet here, 
        	         * copy the packet dma info into csum
                	 * field and let the bh handler analyze
                 	 * the transmitted packet. 
  			 */

#if 0
			if (reg & TxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){

				DEBUG_ERROR("%s:%s: PCI Error: 'Retry' exceeds maximum (64k): Reg=0x%X!\n",
                                        card->devname,chan->if_name,reg);

				if (++chan->pci_retry < 3){
					wan_set_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg);

        				DEBUG_EVENT("%s: Retry: TXDMA_HI=0x%X DmaDescr=0x%lX\n",
                        			__FUNCTION__,reg,dma_descr);

        				card->hw_iface.bus_write_4(card->hw,dma_descr,reg);
					return;
				}				
			}
#endif
			chan->pci_retry=0;
			chan->tx_dma_skb->csum = reg;	
			wan_skb_queue_tail(&chan->wp_tx_complete_list,chan->tx_dma_skb);	
			chan->tx_dma_skb=NULL;

			wan_clear_bit(TX_BUSY,&chan->dma_status);

			WAN_TASKLET_SCHEDULE(&chan->common.bh_task);
		}else{
			netskb_t *skb = chan->tx_dma_skb;
			chan->tx_dma_skb=NULL;

			/* For transparend mode, handle the
                         * transmitted packet directly from
                         * interrupt, to avoid tx overrun.
                         * 
                         * We must clear TX_BUSY before post
                         * function, because the post function
                         * will restart tx dma via xilinx_dma_tx() 
                         */

			skb->csum = reg;
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			xilinx_tx_post_complete (card,chan,skb);
			wan_skb_free(skb);
		}
	}

	/* DEBUGTX */
//	card->hw_iface.bus_read_4(card->hw,0x78, &tmp1); 

}


static void xilinx_tx_post_complete (sdla_t *card, private_area_t *chan, netskb_t *skb)
{
	unsigned long reg = skb->csum;

	if ((wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)) ||
	    (reg & TxDMA_HI_DMA_DATA_LENGTH_MASK) ||
	    (reg&TxDMA_HI_DMA_PCI_ERROR_MASK)){

		DEBUG_EVENT("%s:%s: Tx DMA Descriptor=0x%lX\n",
			card->devname,chan->if_name,reg);

		/* Checking Tx DMA Go bit. Has to be '0' */
		if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
        		DEBUG_ERROR("%s:%s: Error: TxDMA Intr: GO bit set on Tx intr\n",
                   		card->devname,chan->if_name);
		}

		if (reg & TxDMA_HI_DMA_DATA_LENGTH_MASK){
               		DEBUG_ERROR("%s:%s: Error: TxDMA Length not equal 0 \n",
                   		card->devname,chan->if_name);
	        }   
#if 0 
    		/* Checking Tx DMA PCI error status. Has to be '0's */
		if (reg&TxDMA_HI_DMA_PCI_ERROR_MASK){
                	     	
			if (reg & TxDMA_HI_DMA_PCI_ERROR_M_ABRT){
        			DEBUG_ERROR("%s:%s: Tx Error: Abort from Master: pci fatal error!\n",
                	     		card->devname,chan->if_name);
			}
			if (reg & TxDMA_HI_DMA_PCI_ERROR_T_ABRT){
        			DEBUG_ERROR("%s:%s: Tx Error: Abort from Target: pci fatal error!\n",
                	     		card->devname,chan->if_name);
			}
			if (reg & TxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
        			DEBUG_WARNING("%s:%s: Tx Warning: PCI Latency Timeout!\n",
                	     		card->devname,chan->if_name);
				goto tx_post_ok;
			}
			if (reg & TxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
        			DEBUG_ERROR("%s:%s: Tx Error: 'Retry' exceeds maximum (64k): pci fatal error!\n",
                	     		card->devname,chan->if_name);
			}
		}
#endif
		chan->if_stats.tx_dropped++;
		goto tx_post_exit;
	}

	
	chan->if_stats.tx_packets++;
	chan->if_stats.tx_bytes+=skb->len;

        /* Indicate that the first tx frame went
         * out on the transparent link */
        wan_set_bit(0,&chan->idle_start);

	capture_trace_packet(card,chan,skb,TRC_OUTGOING_FRM);

tx_post_exit:

	if (WAN_NETIF_QUEUE_STOPPED(chan->common.dev)){
		WAN_NETIF_WAKE_QUEUE(chan->common.dev);
		if (chan->common.usedby == API){
			wan_wakeup_api(chan);
		}else if (chan->common.usedby == STACK){
			wanpipe_lip_kick(chan,0);
		}
	}

	xilinx_dma_tx(card,chan);	

	return;
}

static void xilinx_dma_rx_complete (sdla_t *card, private_area_t *chan)
{
	unsigned long dma_descr;
	netskb_t *skb;
	wp_rx_element_t *rx_el;

	wan_clear_bit(0,&chan->rx_dma);
	
	if (!chan->rx_dma_skb){
		DEBUG_ERROR("%s: Critical Error: rx_dma_skb\n",chan->if_name);
		return;
	}

	rx_el=(wp_rx_element_t *)&chan->rx_dma_skb->cb[0];

#if 0
	chan->if_stats.rx_frame_errors++;
#endif
	
//    	card->hw_iface.bus_read_4(card->hw,0x80, &rx_empty); 

    	/* Reading Rx DMA descriptor information */
	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_LO;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &rx_el->align);
	rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;

    	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &rx_el->reg);

	rx_el->pkt_error = chan->pkt_error;
	chan->pkt_error=0;

//	DEBUG_RX("%s: DmaDescrLo=0x%X  DmaHi=0x%X \n",
//		__FUNCTION__,rx_el.align,reg);


	pci_unmap_single(NULL, 
			 rx_el->dma_addr,
			 chan->dma_mru,
			 PCI_DMA_FROMDEVICE);

	DEBUG_TX("%s:%s: RX HI=0x%X  LO=0x%X DMA=0x%lX\n",
		__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);   
	
	skb=chan->rx_dma_skb;
	chan->rx_dma_skb=NULL;
	
	xilinx_dma_rx(card,chan);

	wan_skb_queue_tail(&chan->wp_rx_complete_list,skb);

	WAN_TASKLET_SCHEDULE(&chan->common.bh_task);

	WAN_TASKLET_SCHEDULE(&chan->common.bh_task);

//    	card->hw_iface.bus_read_4(card->hw,0x80, &rx_empty); 
}


static void xilinx_rx_post_complete (sdla_t *card, private_area_t *chan, 
				     netskb_t *skb, 
				     netskb_t **new_skb,
				     unsigned char *pkt_error)
{

    	unsigned int len,data_error = 0;
	unsigned char *buf;
	wp_rx_element_t *rx_el=(wp_rx_element_t *)&skb->cb[0];

	DEBUG_TX("%s:%s: RX HI=0x%X  LO=0x%X\n DMA=0x%lX",
		__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);   

#if 0
	chan->if_stats.rx_errors++;
#endif

	rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;
	*pkt_error=0;
	*new_skb=NULL;

	
    	/* Checking Rx DMA Go bit. Has to be '0' */
	if (wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&rx_el->reg)){
        	DEBUG_ERROR("%s:%s: Error: RxDMA Intr: GO bit set on Rx intr\n",
				card->devname,chan->if_name);
		chan->if_stats.rx_errors++;
		goto rx_comp_error;
	}
    
	/* Checking Rx DMA PCI error status. Has to be '0's */
	if (rx_el->reg&RxDMA_HI_DMA_PCI_ERROR_MASK){

		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_M_ABRT){
                	DEBUG_ERROR("%s:%s: Rx Error: Abort from Master: pci fatal error!\n",
                                   card->devname,chan->if_name);
                }
                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_T_ABRT){
                        DEBUG_ERROR("%s:%s: Rx Error: Abort from Target: pci fatal error!\n",
                                   card->devname,chan->if_name);
                }
                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
                        DEBUG_ERROR("%s:%s: Rx Error: No 'DeviceSelect' from target: pci fatal error!\n",
                                    card->devname,chan->if_name);
                }
                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
                        DEBUG_ERROR("%s:%s: Rx Error: 'Retry' exceeds maximum (64k): pci fatal error!\n",
                                    card->devname,chan->if_name);
                }

		DEBUG_ERROR("%s: RXDMA PCI ERROR = 0x%x\n",chan->if_name,rx_el->reg);
		chan->if_stats.rx_errors++;
		goto rx_comp_error;
	}

	if (chan->hdlc_eng){
 
		/* Checking Rx DMA Frame start bit. (information for api) */
		if (!wan_test_bit(RxDMA_HI_DMA_FRAME_START_BIT,&rx_el->reg)){
			DEBUG_EVENT("%s:%s RxDMA Intr: Start flag missing: MTU Mismatch! Reg=0x%X\n",
					card->devname,chan->if_name,rx_el->reg);
			chan->if_stats.rx_frame_errors++;
			goto rx_comp_error;
		}
    
		/* Checking Rx DMA Frame end bit. (information for api) */
		if (!wan_test_bit(RxDMA_HI_DMA_FRAME_END_BIT,&rx_el->reg)){
			DEBUG_EVENT("%s:%s: RxDMA Intr: End flag missing: MTU Mismatch! Reg=0x%X\n",
					card->devname,chan->if_name,rx_el->reg);
			chan->if_stats.rx_frame_errors++;
			goto rx_comp_error;
		
       	 	} else {  /* Check CRC error flag only if this is the end of Frame */
        	
			if (wan_test_bit(RxDMA_HI_DMA_CRC_ERROR_BIT,&rx_el->reg)){
				if (net_ratelimit()){
                   			DEBUG_ERROR("%s:%s: RxDMA Intr: CRC Error! Reg=0x%X\n",
                                		card->devname,chan->if_name,rx_el->reg);
				}
				chan->if_stats.rx_errors++;
				wan_set_bit(WP_CRC_ERROR_BIT,&rx_el->pkt_error);	
                   		data_error = 1;
               		}

			/* Check if this frame is an abort, if it is
                 	 * drop it and continue receiving */
			if (wan_test_bit(RxDMA_HI_DMA_FRAME_ABORT_BIT,&rx_el->reg)){
				if (net_ratelimit()){
					DEBUG_EVENT("%s:%s: RxDMA Intr: Abort! Reg=0x%X\n",
						card->devname,chan->if_name,rx_el->reg);
				}
				chan->if_stats.rx_frame_errors++;
				wan_set_bit(WP_ABORT_ERROR_BIT,&rx_el->pkt_error);
				data_error = 1;
			}

			if (chan->common.usedby != API && data_error){
				goto rx_comp_error;
			}	
    		}
	}

	len=rx_el->reg&RxDMA_HI_DMA_DATA_LENGTH_MASK;

	if (chan->hdlc_eng){
		/* In HDLC mode, calculate rx length based
                 * on alignment value, received from DMA */
		len=(((chan->dma_mru>>2)-len)<<2) - (~(rx_el->align)&RxDMA_LO_ALIGNMENT_BIT_MASK);
	}else{
		/* In Transparent mode, our RX buffer will always be
		 * aligned to the 32bit (word) boundary, because
                 * the RX buffers are all of equal length  */
		len=(((card->u.xilinx.cfg.mru>>2)-len)<<2) - (~(0x03)&RxDMA_LO_ALIGNMENT_BIT_MASK);
	}


	*pkt_error=rx_el->pkt_error;

	/* After a RX FIFO overflow, we must mark max 7 
         * subsequent frames since firmware, cannot 
         * guarantee the contents of the fifo */

	if (wan_test_bit(WP_FIFO_ERROR_BIT,&rx_el->pkt_error)){
		if (++chan->rx_fifo_err_cnt >= WP_MAX_FIFO_FRAMES){
			chan->rx_fifo_err_cnt=0;
		}
		wan_set_bit(WP_FIFO_ERROR_BIT,pkt_error);
	}else{
		if (chan->rx_fifo_err_cnt){
			if (++chan->rx_fifo_err_cnt >= WP_MAX_FIFO_FRAMES){
                        	chan->rx_fifo_err_cnt=0;
			}
			wan_set_bit(WP_FIFO_ERROR_BIT,pkt_error);
		}
	}

	
	if (len > aft_rx_copyback){
		/* The rx size is big enough, thus
		 * send this buffer up the stack
		 * and allocate another one */
		
		memset(&skb->cb[0],0,sizeof(wp_rx_element_t));
		skb_put(skb,len);	
		*new_skb=skb;

		aft_alloc_rx_dma_buff(card,chan,1);
	}else{

		/* The rx packet is very
		 * small thus, allocate a new 
		 * buffer and pass it up */
		*new_skb=wan_skb_alloc(len + 20);
		if (!*new_skb){
			DEBUG_EVENT("%s:%s: Failed to allocate rx skb pkt (len=%i)!\n",
				card->devname,chan->if_name,(len+20));
			chan->if_stats.rx_dropped++;
			goto rx_comp_error;
		}

		buf=wan_skb_put((*new_skb),len);
		memcpy(buf,skb->tail,len);

		aft_init_requeue_free_skb(chan, skb);
	}

	

	return;

rx_comp_error:

	aft_init_requeue_free_skb(chan, skb);
    	return;
}


static char request_xilinx_logical_channel_num (sdla_t *card, private_area_t *chan, long *free_ch)
{
	char logic_ch=-1, free_logic_ch=-1;
	int i,err;

	*free_ch=-1;

	DEBUG_TEST("-- Request_Xilinx_logic_channel_num:--\n");

	DEBUG_TEST("%s:%d Global Num Timeslots=%i  Global Logic ch Map 0x%lX \n",
		__FUNCTION__,__LINE__,
                card->u.xilinx.num_of_time_slots,
                card->u.xilinx.logic_ch_map);


	err=request_fifo_baddr_and_size(card,chan);
	if (err){
		return -1;
	}


	for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
		if (!wan_test_and_set_bit(i,&card->u.xilinx.logic_ch_map)){
			logic_ch=i;
			break;
		}
	}

	if (logic_ch == -1){
		return logic_ch;
	}

	for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
		if (!wan_test_bit(i,&card->u.xilinx.logic_ch_map)){
			free_logic_ch=HDLC_FREE_LOGIC_CH;
			break;
		}
	}

	if (card->u.xilinx.dev_to_ch_map[(unsigned char)logic_ch]){
		DEBUG_ERROR("%s: Error, request logical ch=%i map busy\n",
				card->devname,logic_ch);
		return -1;
	}

	*free_ch=free_logic_ch;

	card->u.xilinx.dev_to_ch_map[(unsigned char)logic_ch]=(void*)chan;

	if (logic_ch > card->u.xilinx.top_logic_ch){
		card->u.xilinx.top_logic_ch=logic_ch;
		xilinx_dma_max_logic_ch(card);
	}


	DEBUG_CFG("Binding logic ch %d  Ptr=%p\n",logic_ch,chan);
	return logic_ch;
}

static void free_xilinx_logical_channel_num (sdla_t *card, int logic_ch)
{
	clear_bit (logic_ch,&card->u.xilinx.logic_ch_map);
	card->u.xilinx.dev_to_ch_map[logic_ch]=NULL;

	if (logic_ch >= card->u.xilinx.top_logic_ch){
		int i;

		card->u.xilinx.top_logic_ch=XILINX_DEFLT_ACTIVE_CH;

		for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
			if (card->u.xilinx.dev_to_ch_map[logic_ch]){
				card->u.xilinx.top_logic_ch=i;
			}
		}

		xilinx_dma_max_logic_ch(card);
	}

}

static void xilinx_dma_max_logic_ch(sdla_t *card)
{
	u32 reg;


	DEBUG_CFG("-- Xilinx_dma_max_logic_ch :--\n");

	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG, &reg);

        /* Set up the current highest active logic channel */

	reg&=DMA_ACTIVE_CHANNEL_BIT_MASK;
        reg|=(card->u.xilinx.top_logic_ch << DMA_ACTIVE_CHANNEL_BIT_SHIFT);

        card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
}

static int aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb)
{
	wan_skb_init(skb,sizeof(wp_api_hdr_t));
	wan_skb_trim(skb,0);
	memset(&skb->cb[0],0,sizeof(wp_rx_element_t));
		
	wan_skb_queue_tail(&chan->wp_rx_free_list,skb);

	return 0;
}

static int aft_reinit_pending_rx_bufs(private_area_t *chan)
{
	netskb_t *skb;

	while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
		chan->if_stats.rx_dropped++;
		aft_init_requeue_free_skb(chan,skb);
	}

	return 0;
}

static int aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num)
{
	int i;
	netskb_t *skb;
	
	for (i=0;i<num;i++){
		skb=wan_skb_alloc(chan->dma_mru);
		if (!skb){
			DEBUG_EVENT("%s: %s  no memory\n",
					chan->if_name,__FUNCTION__);
			return -ENOMEM;
		}
		wan_skb_queue_tail(&chan->wp_rx_free_list,skb);
	}

	return 0;
}




/*============================================================================
 * Enable timer interrupt
 */
static void enable_timer (void* card_id)
{
	sdla_t* 		card = (sdla_t*)card_id;
	unsigned long smp_flags;

	DEBUG_TEST("%s: %s Sdla Polling!\n",__FUNCTION__,card->devname);
	
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	card->wandev.fe_iface.polling(&card->fe);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	return;
}

/**SECTION**************************************************
 *
 * 	API Bottom Half Handlers
 *
 **********************************************************/

static void wp_bh (unsigned long data)
{
	private_area_t* chan = (private_area_t *)data;
	netskb_t *new_skb,*skb;
	unsigned char pkt_error;
	int len;

	if (!(chan->common.dev->flags&IFF_UP)){
		DEBUG_EVENT("%s: wp_bh() chan not up!\n",
                                chan->if_name);
		return;
	}

	while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){

#if 0
		chan->if_stats.rx_errors++;
#endif

		if (chan->common.usedby == API && chan->common.sk == NULL){
			DEBUG_TEST("%s: No sock bound to channel rx dropping!\n",
				chan->if_name);
			chan->if_stats.rx_fifo_errors++;

			aft_init_requeue_free_skb(chan, skb);

			continue;
		}

		new_skb=NULL;
		pkt_error=0;

		/* The post function will take care
		 * of the skb and new_skb buffer.
		 * If new_skb buffer exists, driver
		 * must pass it up the stack, or free it */
		xilinx_rx_post_complete (chan->common.card, chan,
                                   	 skb,
                                     	 &new_skb,
                                     	 &pkt_error);
		if (new_skb){
	
			int len=new_skb->len;

			capture_trace_packet(chan->common.card,chan,
					     new_skb,TRC_INCOMING_FRM);
		
			if (chan->common.usedby == API){

				/* Only for API, we insert packet status
				 * byte to indicate a packet error. Take
			         * this byte and put it in the api header */

				if (wan_skb_headroom(new_skb) >= sizeof(api_rx_hdr_t)){
					api_rx_hdr_t *rx_hdr=
						(api_rx_hdr_t*)skb_push(new_skb,sizeof(api_rx_hdr_t));	
					memset(rx_hdr,0,sizeof(api_rx_hdr_t));
					rx_hdr->error_flag=pkt_error;
				}else{
					DEBUG_ERROR("%s: Error Rx pkt headroom %i < %i\n",
							chan->if_name,
							wan_skb_headroom(new_skb),
							sizeof(api_rx_hdr_t));
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}

				new_skb->protocol = htons(PVC_PROT);
				new_skb->mac.raw  = new_skb->data;
				new_skb->dev      = chan->common.dev;
				new_skb->pkt_type = WAN_PACKET_DATA;	
#if 0	
				chan->if_stats.rx_frame_errors++;
#endif
				if (wan_api_rx(chan,new_skb) != 0){
					if (net_ratelimit()){
						DEBUG_ERROR("%s: Error: Rx Socket busy!\n",
							chan->if_name);
					}
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}
			}else if (chan->common.usedby == TDM_VOICE){

				++chan->if_stats.rx_dropped;
				wan_skb_free(new_skb);
				continue;

			}else if (chan->common.usedby == STACK){

				if (wanpipe_lip_rx(chan,new_skb) != 0){
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}

			}else{
				new_skb->protocol = htons(ETH_P_IP);
				new_skb->dev = chan->common.dev;
				new_skb->mac.raw  = new_skb->data;
				netif_rx(new_skb);
			}

			chan->if_stats.rx_packets++;
			chan->if_stats.rx_bytes+=len;
		}
	}



	while((skb=wan_skb_dequeue(&chan->wp_tx_complete_list)) != NULL){
		xilinx_tx_post_complete (chan->common.card,chan,skb);
		wan_skb_free(skb);
	}

	if ((len=wan_skb_queue_len(&chan->wp_rx_complete_list))){
		DEBUG_TEST("%s: Triggering from bh rx=%i\n",chan->if_name,len); 
		WAN_TASKLET_SCHEDULE(&chan->common.bh_task);
	}else if ((len=wan_skb_queue_len(&chan->wp_tx_complete_list))){
                DEBUG_TEST("%s: Triggering from bh tx=%i\n",chan->if_name,len); 
		WAN_TASKLET_SCHEDULE(&chan->common.bh_task);
        }

	return;
}

static int fifo_error_interrupt(sdla_t *card, unsigned long reg)
{
        u32 rx_status, tx_status;
	u32 err=0;
	u32 i;
	private_area_t *chan;

        /* Clear HDLC pending registers */
        card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG,&tx_status);
        card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG,&rx_status);
	
	if (card->wandev.state != WAN_CONNECTED){
        	DEBUG_ERROR("%s: Warning: Ignoring Error Intr: link disc!\n",
                                  card->devname);
                return 0;
        }

	tx_status&=card->u.xilinx.active_ch_map;
	rx_status&=card->u.xilinx.active_ch_map;

        if (tx_status != 0){
		for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
			if (wan_test_bit(i,&tx_status) && test_bit(i,&card->u.xilinx.logic_ch_map)){
				
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					DEBUG_ERROR("Warning: ignoring tx error intr: no dev!\n");
					continue;
				}

				if (!(chan->common.dev->flags&IFF_UP)){
					DEBUG_ERROR("%s: Warning: ignoring tx error intr: dev down 0x%X  UP=0x%X!\n",
						chan->common.dev->name,chan->common.state,chan->ignore_modem);
					continue;
				}

				if (chan->common.state != WAN_CONNECTED){
					DEBUG_ERROR("%s: Warning: ignoring tx error intr: dev disc!\n",
                                                chan->common.dev->name);
					continue;
				}

				if (!chan->hdlc_eng && !wan_test_bit(0,&chan->idle_start)){
					DEBUG_ERROR("%s: Warning: ignoring tx error intr: dev init error!\n",
                                                chan->common.dev->name);
					if (chan->hdlc_eng){
						xilinx_tx_fifo_under_recover(card,chan);
					}
                                        continue;
				}

                		DEBUG_ERROR("%s:%s: Warning TX Fifo Error on LogicCh=%li Slot=%i!\n",
                           		card->devname,chan->if_name,chan->logic_ch_num,i);

				xilinx_tx_fifo_under_recover(card,chan);
				err=-EINVAL;
			}
		}
        }


        if (rx_status != 0){
		for (i=0;i<card->u.xilinx.num_of_time_slots;i++){
			if (wan_test_bit(i,&rx_status) && test_bit(i,&card->u.xilinx.logic_ch_map)){
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					continue;
				}

				if (!(chan->common.dev->flags&IFF_UP)){
					DEBUG_ERROR("%s: Warning: ignoring rx error intr: dev down 0x%X UP=0x%X!\n",
						chan->common.dev->name,chan->common.state,chan->ignore_modem);
					continue;
				}

				if (chan->common.state != WAN_CONNECTED){
					DEBUG_ERROR("%s: Warning: ignoring rx error intr: dev disc!\n",
                                                chan->common.dev->name);
                                        continue;
                                }

                		DEBUG_ERROR("%s:%s: Warning RX Fifo Error on LCh=%li Slot=%i RxCL=%i RxFL=%i RxDMA=%i\n",
                           		card->devname,chan->if_name,chan->logic_ch_num,i,
					wan_skb_queue_len(&chan->wp_rx_complete_list),
					wan_skb_queue_len(&chan->wp_rx_free_list),
					chan->rx_dma);
#if 0
{
				unsigned long dma_descr;
				unsigned int reg;
			     	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
			        card->hw_iface.bus_read_4(card->hw, dma_descr, &reg);
				DEBUG_EVENT("%s: Hi Descriptor 0x%X\n",chan->if_name,reg);
}
#endif

				wan_set_bit(WP_FIFO_ERROR_BIT, &chan->pkt_error);

				err=-EINVAL;
			}
		}
        }

	return err;
}


static void front_end_interrupt(sdla_t *card, unsigned long reg)
{
	/* FIXME: To be filled by ALEX :) */
	DEBUG_TEST("%s: front_end_interrupt!\n",card->devname);

//	wp_debug_func_add(__FUNCTION__);

	card->wandev.fe_iface.isr(&card->fe);
	handle_front_end_state(card);
	return;
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
static void wp_xilinx_isr (sdla_t* card)
{
    	int i;
	u32 reg;
	u32 dma_tx_reg,dma_rx_reg;
	private_area_t *chan;

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		DEBUG_EVENT("%s: Card down, ignoring interrupt !!!!!!!!\n",
			card->devname);	
		return;
	}


    	wan_set_bit(0,&card->in_isr);

//	write_cpld(card,LED_CONTROL_REG,0x0F);

       /* -----------------2/6/2003 9:02AM------------------
     	* Disable all chip Interrupts  (offset 0x040)
     	*  -- "Transmit/Receive DMA Engine"  interrupt disable
     	*  -- "FiFo/Line Abort Error"        interrupt disable
     	* --------------------------------------------------*/
        card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

	DEBUG_ISR("\n");
	DEBUG_ISR("%s:  ISR (0x%X) = 0x%08X \n",
			card->devname,XILINX_CHIP_CFG_REG,reg);

	if (wan_test_bit(SECURITY_STATUS_FLAG,&reg)){
		DEBUG_EVENT("%s: Critical: Chip Security Compromised: Disabling Driver!\n",
			card->devname);
		DEBUG_EVENT("%s: Please call Sangoma Tech Support (www.sangoma.com)!\n",
			card->devname);

		port_set_state(card,WAN_DISCONNECTED);
		disable_data_error_intr(card,CARD_DOWN);
		goto isr_end;
	}

	/* Note: If interrupts are received without pending
         * flags, it usually indicates that the interrupt
         * is being shared.  (Check 'cat /proc/interrupts') 
	 */

        if (wan_test_bit(FRONT_END_INTR_ENABLE_BIT,&reg)){
		if (wan_test_bit(FRONT_END_INTR_FLAG,&reg)){
              		front_end_interrupt(card,reg);
			if (card->u.xilinx.state_change_exit_isr){
				card->u.xilinx.state_change_exit_isr=0;
				/* The state change occured, skip all
                         	 * other interrupts */
				goto isr_end;
			}
		}
        }

	/* Test Fifo Error Interrupt,
	 * If set shutdown all interfaces and
         * reconfigure */
	if (wan_test_bit(ERROR_INTR_ENABLE_BIT,&reg)){ 
        	if (wan_test_bit(ERROR_INTR_FLAG,&reg)){
			DEBUG_EVENT("%s: ERR INTR (0x%X)\n",card->devname,reg);
			fifo_error_interrupt(card,reg);
		}
	}

       /* -----------------2/6/2003 9:37AM------------------
      	* Checking for Interrupt source:
      	* 1. Receive DMA Engine
      	* 2. Transmit DMA Engine
      	* 3. Error conditions.
      	* --------------------------------------------------*/
    	if (wan_test_bit(GLOBAL_INTR_ENABLE_BIT,&reg) &&
            wan_test_bit(DMA_INTR_FLAG,&reg)){


        	/* Receive DMA Engine */
		card->hw_iface.bus_read_4(card->hw,
                                XILINX_DMA_RX_INTR_PENDING_REG, 
                                &dma_rx_reg);


		DEBUG_ISR("%s: DMA_RX_INTR_REG(0x%X) = 0x%X\n",
				card->devname,
				XILINX_DMA_RX_INTR_PENDING_REG,dma_rx_reg);

		dma_rx_reg&=card->u.xilinx.active_ch_map;

		if (dma_rx_reg == 0){
			goto isr_skb_rx;
		}

		for (i=0; i<card->u.xilinx.num_of_time_slots ;i++){
			if (wan_test_bit(i,&dma_rx_reg) && test_bit(i,&card->u.xilinx.logic_ch_map)){
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					DEBUG_ERROR("%s: Error: No Dev for Rx logical ch=%i\n",
							card->devname,i);
					continue;
				}

				if (!(chan->common.dev->flags&IFF_UP)){
					DEBUG_ERROR("%s: Error: Dev not up for Rx logical ch=%i\n",
                                                        card->devname,i);
                                        continue;
				}	
#if 0	
				chan->if_stats.rx_frame_errors++;
#endif

                		DEBUG_ISR("%s: RX Interrupt pend. \n",
						card->devname);
				xilinx_dma_rx_complete(card,chan);
			}
		}
isr_skb_rx:

	        /* Transmit DMA Engine */

	        card->hw_iface.bus_read_4(card->hw,
					  XILINX_DMA_TX_INTR_PENDING_REG, 
					  &dma_tx_reg);

		dma_tx_reg&=card->u.xilinx.active_ch_map;

		DEBUG_ISR("%s: DMA_TX_INTR_REG(0x%X) = 0x%X\n",
				card->devname,
				XILINX_DMA_RX_INTR_PENDING_REG,dma_tx_reg);

		if (dma_tx_reg == 0){
                        goto isr_skb_tx;
                }

		for (i=0; i<card->u.xilinx.num_of_time_slots ;i++){
			if (wan_test_bit(i,&dma_tx_reg) && test_bit(i,&card->u.xilinx.logic_ch_map)){
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					DEBUG_ERROR("%s: Error: No Dev for Tx logical ch=%i\n",
							card->devname,i);
					continue;
				}

				if (!(chan->common.dev->flags&IFF_UP)){
                                        DEBUG_ERROR("%s: Error: Dev not up for Tx logical ch=%i\n",
                                                        card->devname,i);
                                        continue;
                                }


             			DEBUG_ISR("---- TX Interrupt pend. --\n");
				xilinx_dma_tx_complete(card,chan);
			}
        	}
    	}

isr_skb_tx:

	/* -----------------2/6/2003 10:36AM-----------------
	 *    Finish of the interupt handler
	 * --------------------------------------------------*/
isr_end:

//	write_cpld(card,LED_CONTROL_REG,0x0E);

    	DEBUG_ISR("---- ISR end.-------------------\n");
    	wan_clear_bit(0,&card->in_isr);
	return;
}



/**SECTION***********************************************************
 *
 * WANPIPE Debugging Interfaces
 *
 ********************************************************************/



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
#if 1
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				private_area_t* chan, int local_dev )
{
	unsigned short buffer_length;
	wan_udp_pkt_t *wan_udp_pkt;
	struct timeval tv;
	wan_trace_t *trace_info=NULL;

	wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;

	if (wan_atomic_read(&chan->udp_pkt_len) == 0){
		return -ENODEV;
	}

	trace_info=&chan->trace_info;
	wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;

   	{

		netskb_t *skb;

		wan_udp_pkt->wan_udp_opp_flag = 0;

		switch(wan_udp_pkt->wan_udp_command) {

		case READ_CONFIGURATION:
		case READ_CODE_VERSION:
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len=0;
			break;
	

		case ENABLE_TRACING:
	
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			wan_udp_pkt->wan_udp_data_len = 0;
			
			if (!wan_test_bit(0,&trace_info->tracing_enabled)){
						
				trace_info->trace_timeout = SYSTEM_TICKS;
					
				wan_trace_purge(trace_info);
					
				if (wan_udp_pkt->wan_udp_data[0] == 0){
					wan_clear_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L3 trace enabled!\n",
						card->devname);
				}else if (wan_udp_pkt->wan_udp_data[0] == 1){
					wan_clear_bit(2,&trace_info->tracing_enabled);
					wan_set_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L2 trace enabled!\n",
							card->devname);
				}else{
					wan_clear_bit(1,&trace_info->tracing_enabled);
					wan_set_bit(2,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L1 trace enabled!\n",
							card->devname);
				}
				set_bit (0,&trace_info->tracing_enabled);

			}else{
				DEBUG_ERROR("%s: Error: ATM trace running!\n",
						card->devname);
				wan_udp_pkt->wan_udp_return_code = 2;
			}
					
			break;

		case DISABLE_TRACING:
			
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			
			if(wan_test_bit(0,&trace_info->tracing_enabled)) {
					
				wan_clear_bit(0,&trace_info->tracing_enabled);
				wan_clear_bit(1,&trace_info->tracing_enabled);
				wan_clear_bit(2,&trace_info->tracing_enabled);
				
				wan_trace_purge(trace_info);
				
				DEBUG_UDP("%s: Disabling ADSL trace\n",
							card->devname);
					
			}else{
				/* set return code to line trace already 
				   disabled */
				wan_udp_pkt->wan_udp_return_code = 1;
			}

			break;

	        case GET_TRACE_INFO:

			if(wan_test_bit(0,&trace_info->tracing_enabled)){
				trace_info->trace_timeout = SYSTEM_TICKS;
			}else{
				DEBUG_ERROR("%s: Error ATM trace not enabled\n",
						card->devname);
				/* set return code */
				wan_udp_pkt->wan_udp_return_code = 1;
				break;
			}

			buffer_length = 0;
			wan_udp_pkt->wan_udp_atm_num_frames = 0;	
			wan_udp_pkt->wan_udp_atm_ismoredata = 0;
					
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
					wan_udp_pkt->wan_udp_atm_ismoredata = 0x01;
					break;
				}

				m_copydata(skb, 
					   0, 
					   skb->m_pkthdr.len, 
					   &wan_udp_pkt->wan_udp_data[buffer_length]);
				buffer_length += skb->m_pkthdr.len;
				WAN_IFQ_DEQUEUE(&trace_info->trace_queue, skb);
				if (skb){
					wan_skb_free(skb);
				}
				wan_udp_pkt->wan_udp_atm_num_frames++;
			}
#elif defined(__LINUX__)
			while ((skb=skb_dequeue(&trace_info->trace_queue)) != NULL){

				if((MAX_TRACE_BUFFER - buffer_length) < wan_skb_len(skb)){
					/* indicate there are more frames on board & exit */
					wan_udp_pkt->wan_udp_atm_ismoredata = 0x01;
					if (buffer_length != 0){
						wan_skb_queue_head(&trace_info->trace_queue, skb);
					}else{
						/* If rx buffer length is greater than the
						 * whole udp buffer copy only the trace
						 * header and drop the trace packet */

						memcpy(&wan_udp_pkt->wan_udp_atm_data[buffer_length], 
							wan_skb_data(skb),
							sizeof(wan_trace_pkt_t));

						buffer_length = sizeof(wan_trace_pkt_t);
						wan_udp_pkt->wan_udp_atm_num_frames++;
						wan_skb_free(skb);	
					}
					break;
				}

				memcpy(&wan_udp_pkt->wan_udp_atm_data[buffer_length], 
				       wan_skb_data(skb),
				       wan_skb_len(skb));
		     
				buffer_length += wan_skb_len(skb);
				wan_skb_free(skb);
				wan_udp_pkt->wan_udp_atm_num_frames++;
			}
#endif                      
			/* set the data length and return code */
			wan_udp_pkt->wan_udp_data_len = buffer_length;
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			break;

		case ROUTER_UP_TIME:
			do_gettimeofday( &tv );
			chan->router_up_time = tv.tv_sec - 
					chan->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					chan->router_up_time;	
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = 0;
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
			break;



		case WAN_GET_PROTOCOL:
		   	wan_udp_pkt->wan_udp_aft_num_frames = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		default:
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
	
			if (net_ratelimit()){
				DEBUG_EVENT(
				"%s: Warning, Illegal UDP command attempted from network: %x\n",
				card->devname,wan_udp_pkt->wan_udp_command);
			}
			break;
		} /* end of switch */
     	} /* end of else */

     	/* Fill UDP TTL */
	wan_udp_pkt->wan_ip_ttl= card->wandev.ttl;

	wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
	return 1;

}
#endif



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
	netdevice_t *dev;
	struct wan_dev_le	*devle;


        if (card->wandev.state != state)
        {
                switch (state)
                {
                case WAN_CONNECTED:
                        DEBUG_EVENT( "%s: Link connected!\n",
                                card->devname);
			aft_red_led_ctrl(card,AFT_LED_OFF);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_ON);
                      	break;

                case WAN_CONNECTING:
                        DEBUG_EVENT( "%s: Link connecting...\n",
                                card->devname);
			aft_red_led_ctrl(card,AFT_LED_ON);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
                        break;

                case WAN_DISCONNECTED:
                        DEBUG_EVENT( "%s: Link disconnected!\n",
                                card->devname);
			aft_red_led_ctrl(card,AFT_LED_ON);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
                        break;
                }

                card->wandev.state = state;
		WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (dev){
				set_chan_state(card, dev, state);
			}
		}
        }
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

static void handle_front_end_state(void *card_id)
{
	sdla_t*	card = (sdla_t*)card_id;
	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	if (card->fe.fe_status == FE_CONNECTED){
		if (card->wandev.state != WAN_CONNECTED){
			enable_data_error_intr(card);
			port_set_state(card,WAN_CONNECTED);
			card->u.xilinx.state_change_exit_isr=1;
		}else{
		}
	}else{
		port_set_state(card,WAN_DISCONNECTED);
		disable_data_error_intr(card,LINK_DOWN);
		card->u.xilinx.state_change_exit_isr=1;
	}
}

static unsigned char read_cpld(sdla_t *card, unsigned short cpld_off)
{

        u16     org_off;
        u8      tmp;

        cpld_off &= ~BIT_DEV_ADDR_CLEAR;
        cpld_off |= BIT_DEV_ADDR_CPLD;

        /*ALEX: Save the current address. */
        card->hw_iface.bus_read_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                &org_off);

        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                cpld_off);

        card->hw_iface.bus_read_1(card->hw,XILINX_MCPU_INTERFACE, &tmp);

        /*ALEX: Restore original address */
        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                org_off);
        return tmp;


}

static unsigned char write_cpld(sdla_t *card, unsigned short off,unsigned char data)
{
	u16             org_off;

        off &= ~BIT_DEV_ADDR_CLEAR;
        off |= BIT_DEV_ADDR_CPLD;

        /*ALEX: Save the current original address */
        card->hw_iface.bus_read_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                &org_off);
        
	card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                off);
	
	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	udelay(5);
        
	card->hw_iface.bus_write_1(card->hw,
                                XILINX_MCPU_INTERFACE,
                                data);
	
	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	udelay(5);
	
        /*ALEX: Restore the original address */
        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                org_off);
        return 0;
}

static int xilinx_read(sdla_t *card,struct ifreq *ifr)
{
	struct sdla_hdlc_api api_cmd;	

	if (!ifr || !ifr->ifr_data){
		DEBUG_ERROR("%s: Error: No ifr or ifr_data\n",__FUNCTION__);
		return -EFAULT;
	}

	if (copy_from_user(&api_cmd,ifr->ifr_data,sizeof(struct sdla_hdlc_api))){
		return -EFAULT;
	}

	if (api_cmd.offset <= 0x3C){
		 card->hw_iface.pci_read_config_dword(card->hw,
						api_cmd.offset,
						(u32*)&api_cmd.data[0]); 
		 api_cmd.len=4;

	}else{
		card->hw_iface.peek(card->hw, api_cmd.offset, &api_cmd.data[0], api_cmd.len);
	}

#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Reading Bar%i Offset=0x%X Len=%i\n",
			card->devname,api_cmd.bar,api_cmd.offset,api_cmd.len);
#endif

	if (copy_to_user(ifr->ifr_data,&api_cmd,sizeof(struct sdla_hdlc_api))){
		return -EFAULT;
	}

	return 0;
}

static int xilinx_write(sdla_t *card,struct ifreq *ifr)
{

	struct sdla_hdlc_api api_cmd;	

	if (!ifr || !ifr->ifr_data){
		DEBUG_ERROR("%s: Error: No ifr or ifr_data\n",__FUNCTION__);
		return -EFAULT;
	}

	if (copy_from_user(&api_cmd,ifr->ifr_data,sizeof(struct sdla_hdlc_api))){
		return -EFAULT;
	}

#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Writting Bar%i Offset=0x%X Len=%i\n",
			card->devname,
			api_cmd.bar,api_cmd.offset,api_cmd.len);
#endif

#if 0
	card->hw_iface.poke(card->hw, api_cmd.offset, &api_cmd.data[0], api_cmd.len);
#endif
	if (api_cmd.len == 1){
		card->hw_iface.bus_write_1(
			card->hw,
			api_cmd.offset,
			(u8)api_cmd.data[0]);
	}else if (api_cmd.len == 2){
		card->hw_iface.bus_write_2(
			card->hw,
			api_cmd.offset,
			*(u16*)&api_cmd.data[0]);
	}else if (api_cmd.len == 4){
		card->hw_iface.bus_write_4(
			card->hw,
			api_cmd.offset,
			*(u32*)&api_cmd.data[0]);
	}else{
		card->hw_iface.poke(
			card->hw, 
			api_cmd.offset, 
			&api_cmd.data[0], 
			api_cmd.len);
	}

	if (copy_to_user(ifr->ifr_data,&api_cmd,sizeof(struct sdla_hdlc_api))){
		return -EFAULT;
	}

	return 0;

}

static int xilinx_write_bios(sdla_t *card,struct ifreq *ifr)
{
	struct sdla_hdlc_api api_cmd;	

	if (!ifr || !ifr->ifr_data){
		DEBUG_ERROR("%s: Error: No ifr or ifr_data\n",__FUNCTION__);
		return -EINVAL;
	}

	if (copy_from_user(&api_cmd,ifr->ifr_data,sizeof(struct sdla_hdlc_api))){
		return -EFAULT;
	}
#ifdef DEB_XILINX
	DEBUG_EVENT("Setting PCI 0xX=0x%08lX   0x3C=0x%08X\n",
			(card->wandev.S514_cpu_no[0] == SDLA_CPU_A) ? 0x10 : 0x14,
			card->u.xilinx.bar,card->wandev.irq);
#endif
	card->hw_iface.pci_write_config_dword(card->hw, 
			(card->wandev.S514_cpu_no[0] == SDLA_CPU_A) ? 0x10 : 0x14,
			card->u.xilinx.bar);
	card->hw_iface.pci_write_config_dword(card->hw, 0x3C, card->wandev.irq);
	card->hw_iface.pci_write_config_dword(card->hw, 0x0C, 0x0000ff00);

	if (copy_to_user(ifr->ifr_data,&api_cmd,sizeof(struct sdla_hdlc_api))){
		return -EFAULT;
	}

	return 0;
}

/*=========================================
 * enable_data_error_intr
 *
 * Description:
 *	
 *    Run only after the front end comes
 *    up from down state.
 *
 *    Clean the DMA Tx/Rx pending interrupts.
 *       (Ignore since we will reconfigure
 *        all dma descriptors. DMA controler
 *        was already disabled on link down)
 *
 *    For all channels clean Tx/Rx Fifo
 *
 *    Enable DMA controler
 *        (This starts the fifo cleaning
 *         process)
 *
 *    For all channels reprogram Tx/Rx DMA
 *    descriptors.
 * 
 *    Clean the Tx/Rx Error pending interrupts.
 *        (Since dma fifo's are now empty)
 *   
 *    Enable global DMA and Error interrutps.    
 *
 */

static void enable_data_error_intr(sdla_t *card)
{
	u32 			reg;	
	netdevice_t 		*dev;
	struct wan_dev_le	*devle;


	DEBUG_TEST("%s: !!!\n",__FUNCTION__);

	/* Clean Tx/Rx DMA interrupts */

	card->hw_iface.bus_read_4(card->hw,
                                  XILINX_DMA_RX_INTR_PENDING_REG, &reg);
        card->hw_iface.bus_read_4(card->hw,
                                  XILINX_DMA_TX_INTR_PENDING_REG, &reg);


        /* For all channels clean Tx/Rx fifos */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){

		private_area_t *chan;

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;

                chan = wan_netif_priv(dev);
		if (!(wan_netif_flags(dev)&IFF_UP)){
			continue;
		}

		DEBUG_TEST("%s: Init interface fifo no wait %s\n",__FUNCTION__,chan->if_name);

                xilinx_init_rx_dev_fifo(card, chan, WP_NO_WAIT);
                xilinx_init_tx_dev_fifo(card, chan, WP_NO_WAIT);
        }


        /* Enable DMA controler, in order to start the
         * fifo cleaning */
        card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
        wan_set_bit(DMA_ENGINE_ENABLE_BIT,&reg);
        card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);


	/* For all channels clean Tx/Rx fifos */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
             	private_area_t *chan;

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
                chan = wan_netif_priv(dev);
		if (!(wan_netif_flags(dev)&IFF_UP)){
			continue;
		}

		DEBUG_TEST("%s: Init interface fifo %s\n",__FUNCTION__,chan->if_name);

		xilinx_init_rx_dev_fifo(card, chan, WP_WAIT);
		xilinx_init_tx_dev_fifo(card, chan, WP_WAIT);

		DEBUG_TEST("%s: Clearing Fifo and idle_flag %s\n",
				card->devname,chan->if_name);
		wan_clear_bit(0,&chan->idle_start);
	}

	/* For all channels, reprogram Tx/Rx DMA descriptors.
         * For Tx also make sure that the BUSY flag is clear
         * and previoulsy Tx packet is deallocated */

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
             	private_area_t *chan;

		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
                chan = wan_netif_priv(dev);
		if (!(wan_netif_flags(dev)&IFF_UP)){
			continue;
		}

		DEBUG_TEST("%s: Init interface %s\n",__FUNCTION__,chan->if_name);

		if (chan->rx_dma_skb){
			wp_rx_element_t *rx_el;
			netskb_t *skb=chan->rx_dma_skb;

			chan->rx_dma_skb=NULL;
			rx_el=(wp_rx_element_t *)&skb->cb[0];
		
			pci_unmap_single(NULL, 
				 rx_el->dma_addr,
				 chan->dma_mru,
				 PCI_DMA_FROMDEVICE);

			aft_init_requeue_free_skb(chan, skb);
		}


                xilinx_dma_rx(card,chan);

	        if (chan->tx_dma_addr && chan->tx_dma_len){
        	         pci_unmap_single(NULL,
                	                 chan->tx_dma_addr,
                        	         chan->tx_dma_len,
                                	 PCI_DMA_TODEVICE);

           	     	chan->tx_dma_addr=0;
                	chan->tx_dma_len=0;
        	}


		if (chan->tx_dma_skb){
			wan_skb_free(chan->tx_dma_skb);
			chan->tx_dma_skb=NULL;
		}

		wan_clear_bit(TX_BUSY,&chan->dma_status);
                wan_clear_bit(0,&chan->idle_start);

		xilinx_dma_tx(card,chan);

                DEBUG_TEST("%s: Clearing Fifo and idle_flag %s\n",
                                card->devname,chan->if_name);

        }


	/* Clean Tx/Rx Error interrupts, since fifos are now
         * empty, and Tx fifo may generate an underrun which
         * we want to ignore :) */

     	card->hw_iface.bus_read_4(card->hw,
                                  XILINX_HDLC_RX_INTR_PENDING_REG, &reg);
        card->hw_iface.bus_read_4(card->hw,
                                  XILINX_HDLC_TX_INTR_PENDING_REG, &reg);


	/* Enable Global DMA and Error Interrupts */

	reg=0;
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
    	wan_set_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
	wan_set_bit(ERROR_INTR_ENABLE_BIT,&reg); 
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	DEBUG_TEST("%s: END !!!\n",__FUNCTION__);

}

static void disable_data_error_intr(sdla_t *card, unsigned char event)
{
	u32 reg;
	
	DEBUG_TEST("%s: !!!!!!!\n",__FUNCTION__);

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
    	wan_clear_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg); 
	if (event==DEVICE_DOWN){
		wan_clear_bit(FRONT_END_INTR_ENABLE_BIT,&reg);
	}
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
	wan_clear_bit(DMA_ENGINE_ENABLE_BIT,&reg);
        card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);

}

static void xilinx_init_tx_dma_descr(sdla_t *card, private_area_t *chan)
{
	unsigned long dma_descr;
	unsigned long reg=0;

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_write_4(card->hw,dma_descr, reg);
}



/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card)
{
	/* 1. On the first timer interrupt, update T1/E1 alarms
         * and PMON counters (only for T1/E1 card) (TE1)
         */

        /* TE1 Update T1/E1 alarms */
         if (IS_TE1_CARD(card)) {
                card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE); 
		/* TE1 Update T1/E1 perfomance counters */
                card->wandev.fe_iface.read_pmon(&card->fe); 
         }

        return 0;
}


static void xilinx_tx_fifo_under_recover (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
        unsigned long dma_descr;

	DEBUG_TEST("%s:%s: Tx Fifo Recovery \n",card->devname,chan->if_name);

	if (chan->hdlc_eng){
        	/* Initialize Tx DMA descriptor: Stop DMA */
        	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
        	card->hw_iface.bus_write_4(card->hw,dma_descr, reg);

		/* Clean the TX FIFO */
		xilinx_init_tx_dev_fifo(card, chan, WP_WAIT);
	}


        if (chan->tx_dma_addr && chan->tx_dma_len){
                 pci_unmap_single(NULL,
                                 chan->tx_dma_addr,
                                 chan->tx_dma_len,
                                 PCI_DMA_TODEVICE);

                chan->tx_dma_addr=0;
                chan->tx_dma_len=0;
        }


	/* Requeue the current tx packet, for 
         * re-transmission */
	if (chan->tx_dma_skb){
		wan_skb_queue_head(&chan->wp_tx_pending_list, chan->tx_dma_skb);		
		chan->tx_dma_skb=NULL;
	}	


	/* Wake up the stack, because tx dma interrupt
         * failed */
	if (WAN_NETIF_QUEUE_STOPPED(chan->common.dev)){
                WAN_NETIF_WAKE_QUEUE(chan->common.dev);
                if (chan->common.usedby == API){
                        wan_wakeup_api(chan);
                }
        }

	if (!chan->hdlc_eng){
		if (wan_test_bit(0,&chan->idle_start)){
			++chan->if_stats.tx_fifo_errors;
		}
	}else{
		++chan->if_stats.tx_fifo_errors;
	}

	DEBUG_EVENT("%s:%s: Tx Fifo Recovery: Restarting Transmission \n",
                                card->devname,chan->if_name);

	/* Re-start transmission */
	wan_clear_bit(TX_BUSY,&chan->dma_status);
        xilinx_dma_tx(card,chan);
}

static int xilinx_write_ctrl_hdlc(sdla_t *card, u32 timeslot, u8 reg_off, u32 data)
{
	u32 reg;
	u32 ts_orig=timeslot;
	unsigned long timeout=jiffies;

	if (timeslot == 0){
		timeslot=card->u.xilinx.num_of_time_slots-2;
	}else if (timeslot == 1){
		timeslot=card->u.xilinx.num_of_time_slots-1;
	}else{
		timeslot-=2;
	}

	timeslot=timeslot<<XILINX_CURRENT_TIMESLOT_SHIFT;
	timeslot&=XILINX_CURRENT_TIMESLOT_MASK;

	for (;;){
		card->hw_iface.bus_read_4(card->hw,XILINX_TIMESLOT_HDLC_CHAN_REG,&reg);	
		reg&=XILINX_CURRENT_TIMESLOT_MASK;

		if (reg == timeslot){
			card->hw_iface.bus_write_4(card->hw,reg_off,data);			
			return 0;
		}

		if ((jiffies-timeout) > 1){
			DEBUG_ERROR("%s: Error: Access to timeslot %i timed out!\n",
				card->devname,ts_orig);
			return -EIO;
		}
	}

	return -EIO;
}

static int set_chan_state(sdla_t* card, netdevice_t* dev, int state)
{
	private_area_t *chan = dev->priv;

	chan->common.state = state;
	if (state == WAN_CONNECTED){
	       DEBUG_TEST("%s: Setting idle_start to 0\n",
		       chan->if_name);
	       wan_clear_bit(0,&chan->idle_start);
	}

	if (chan->common.usedby == API){
	       wan_update_api_state(chan);
	}

	if (chan->common.usedby == STACK){
		if (state == WAN_CONNECTED){
			wanpipe_lip_connect(chan,0);
		}else{
			wanpipe_lip_disconnect(chan,0);
		}
	}
	return 0;
}


static char fifo_size_vector[] = {1, 2, 4, 8, 16, 32};
static char fifo_code_vector[] = {0, 1, 3, 7,0xF,0x1F};

static int request_fifo_baddr_and_size(sdla_t *card, private_area_t *chan)
{
	unsigned char req_fifo_size,fifo_size;
	int i;

	/* Calculate the optimal fifo size based
         * on the number of time slots requested */

	if (IS_T1_CARD(card)){	

		if (chan->num_of_time_slots == NUM_OF_T1_CHANNELS){
			req_fifo_size=32;
		}else if (chan->num_of_time_slots == 1){
			req_fifo_size=1;
		}else if (chan->num_of_time_slots == 2 || chan->num_of_time_slots == 3){
			req_fifo_size=2;
		}else if (chan->num_of_time_slots >= 4 && chan->num_of_time_slots<= 7){
			req_fifo_size=4;
		}else if (chan->num_of_time_slots >= 8 && chan->num_of_time_slots<= 15){
			req_fifo_size=8;
		}else if (chan->num_of_time_slots >= 16 && chan->num_of_time_slots<= 23){
			req_fifo_size=16;
		}else{
			DEBUG_EVENT("%s:%s: Invalid number of timeslots %i\n",
					card->devname,chan->if_name,chan->num_of_time_slots);
			return -EINVAL;		
		}
	}else{
		if (chan->num_of_time_slots == (NUM_OF_E1_CHANNELS-1)){
			req_fifo_size=32;
                }else if (chan->num_of_time_slots == 1){
			req_fifo_size=1;
                }else if (chan->num_of_time_slots == 2 || chan->num_of_time_slots == 3){
			req_fifo_size=2;
                }else if (chan->num_of_time_slots >= 4 && chan->num_of_time_slots <= 7){
			req_fifo_size=4;
                }else if (chan->num_of_time_slots >= 8 && chan->num_of_time_slots <= 15){
			req_fifo_size=8;
                }else if (chan->num_of_time_slots >= 16 && chan->num_of_time_slots <= 31){
			req_fifo_size=16;
                }else{
                        DEBUG_EVENT("%s:%s: Invalid number of timeslots %i\n",
                                        card->devname,chan->if_name,chan->num_of_time_slots);
                        return -EINVAL;
                }
	}

	DEBUG_TEST("%s:%s: Optimal Fifo Size =%i  Timeslots=%i \n",
		card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots);

	fifo_size=map_fifo_baddr_and_size(card,req_fifo_size,&chan->fifo_base_addr);
	if (fifo_size == 0 || chan->fifo_base_addr == 31){
		DEBUG_ERROR("%s:%s: Error: Failed to obtain fifo size %i or addr %i \n",
				card->devname,chan->if_name,fifo_size,chan->fifo_base_addr);
                return -EINVAL;
        }

	DEBUG_TEST("%s:%s: Optimal Fifo Size =%i  Timeslots=%i New Fifo Size=%i \n",
                card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots,fifo_size);


	for (i=0;i<sizeof(fifo_size_vector);i++){
		if (fifo_size_vector[i] == fifo_size){
			chan->fifo_size_code=fifo_code_vector[i];
			break;
		}
	}

	if (fifo_size != req_fifo_size){
		DEBUG_WARNING("%s:%s: Warning: Failed to obtain the req fifo %i got %i\n",
			card->devname,chan->if_name,req_fifo_size,fifo_size);
	}	

	DEBUG_TEST("%s: %s:Fifo Size=%i  Timeslots=%i Fifo Code=%i Addr=%i\n",
                card->devname,chan->if_name,fifo_size,
		chan->num_of_time_slots,chan->fifo_size_code,
		chan->fifo_base_addr);

	chan->fifo_size = fifo_size;

	return 0;
}


static int map_fifo_baddr_and_size(sdla_t *card, unsigned char fifo_size, unsigned char *addr)
{
	u32 reg=0;
	int i;

	for (i=0;i<fifo_size;i++){
		wan_set_bit(i,&reg);
	} 

	DEBUG_TEST("%s: Trying to MAP 0x%X  to 0x%lX\n",
                        card->devname,reg,card->u.xilinx.fifo_addr_map);

	for (i=0;i<32;i+=fifo_size){
		if (card->u.xilinx.fifo_addr_map & (reg<<i)){
			continue;
		}
		card->u.xilinx.fifo_addr_map |= reg<<i;
		*addr=i;

		DEBUG_TEST("%s: Card fifo Map 0x%lX Addr =%i\n",
	                card->devname,card->u.xilinx.fifo_addr_map,i);

		return fifo_size;
	}

	if (fifo_size == 1){
		return 0; 
	}

	fifo_size = fifo_size >> 1;
	
	return map_fifo_baddr_and_size(card,fifo_size,addr);
}


static int free_fifo_baddr_and_size (sdla_t *card, private_area_t *chan)
{
	u32 reg=0;
	int i;

	for (i=0;i<chan->fifo_size;i++){
                wan_set_bit(i,&reg);
        }
	
	DEBUG_TEST("%s: Unmapping 0x%X from 0x%lX\n",
		card->devname,reg<<chan->fifo_base_addr, card->u.xilinx.fifo_addr_map);

	card->u.xilinx.fifo_addr_map &= ~(reg<<chan->fifo_base_addr);

	DEBUG_TEST("%s: New Map is 0x%lX\n",
                card->devname, card->u.xilinx.fifo_addr_map);


	chan->fifo_size=0;
	chan->fifo_base_addr=0;

	return 0;
}

static void aft_red_led_ctrl(sdla_t *card, int mode)
{
	unsigned int led;

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &led);
	
	if (mode == AFT_LED_ON){
		wan_clear_bit(XILINX_RED_LED,&led);
	}else if (mode == AFT_LED_OFF){
		wan_set_bit(XILINX_RED_LED,&led);
	}else{
		if (wan_test_bit(XILINX_RED_LED,&led)){
			wan_clear_bit(XILINX_RED_LED,&led);
		}else{
			wan_set_bit(XILINX_RED_LED,&led);
		}
	}

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG, led);
}

static void aft_led_timer(unsigned long data)
{
	sdla_t *card=(sdla_t*)data;
	unsigned int te_alarm;

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		return;
	}

	if (IS_TE1_CARD(card)) {
		unsigned long smp_flags;

		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

         	te_alarm = card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);

		te_alarm&=~(WAN_TE_BIT_OOSMF_ALARM|WAN_TE_BIT_OOCMF_ALARM);
		
		//if (te_alarm){
		//	DEBUG_TEST("%s: Dbg: Alarms=0x%X\n",
		//			card->devname,te_alarm);
		//}

		if (!te_alarm){
			
			if (card->wandev.state == WAN_CONNECTED){
				aft_red_led_ctrl(card, AFT_LED_OFF);
				card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_ON);
			}else{
				aft_red_led_ctrl(card, AFT_LED_OFF);
				card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_TOGGLE);
			}

		}else if (te_alarm & (WAN_TE_BIT_RED_ALARM|WAN_TE_BIT_LOS_ALARM)){
			/* Red or LOS Alarm solid RED */
			aft_red_led_ctrl(card, AFT_LED_ON);	
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
			
		}else if (te_alarm & WAN_TE_BIT_OOF_ALARM){
			/* OOF Alarm flashing RED */
			aft_red_led_ctrl(card, AFT_LED_TOGGLE);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
		
		}else if (te_alarm & WAN_TE_BIT_AIS_ALARM){
			/* AIS - Blue Alarm flasing RED and GREEN */
			aft_red_led_ctrl(card, AFT_LED_TOGGLE);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_TOGGLE);
		}else if (te_alarm & WAN_TE_BIT_YEL_ALARM){
			/* Yellow Alarm */
			aft_red_led_ctrl(card, AFT_LED_ON);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_ON);
		}else{

			/* Default case shouldn't happen */
			DEBUG_EVENT("%s: Unknown Alarm 0x%X\n",card->devname,te_alarm);
			aft_red_led_ctrl(card, AFT_LED_ON);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_ON);
		}

		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		
		wan_add_timer(&card->u.xilinx.led_timer,HZ);
         }
}


int aft_core_ready(sdla_t *card)
{
	u32 reg;
	volatile unsigned char cnt=0;

        for (;;){
                card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

                if (!wan_test_bit(HDLC_CORE_READY_FLAG_BIT,&reg)){
                        /* The HDLC Core is not ready! we have
                         * an error. */
                        if (++cnt > 5){
                                return  -EINVAL;
                        }else{
                                udelay(500);
                                /* WARNING: we cannot do this while in
                                 * critical area */
                        }
                }else{
			return 0;
                }
        }

	return -EINVAL;
}


static int 
capture_trace_packet(sdla_t *card,
		     private_area_t *chan,
		     netskb_t *skb,
		     char direction)
{
	int		len = 0;
	void*		new_skb = NULL;
	wan_trace_pkt_t trc_el;	
	int		flag = 0;
	wan_trace_t     *trace_info = &chan->trace_info;

	if ((flag = wan_tracing_enabled(trace_info)) >= 0){

		/* Allocate a header mbuf */
		len = 	wan_skb_len(skb) + sizeof(wan_trace_pkt_t);

		new_skb = wan_skb_alloc(len);
		if (new_skb == NULL) 
			return -1;

		wan_getcurrenttime(&trc_el.sec, &trc_el.usec);

		trc_el.status		= direction;
		trc_el.data_avail	= 1;
		trc_el.time_stamp	= (unsigned short)((trc_el.usec/1000) % 0xFFFF);
		trc_el.real_length	= wan_skb_len(skb);
		
		wan_skb_copyback(new_skb, 
				  wan_skb_len(new_skb), 
			   	  sizeof(wan_trace_pkt_t), 
				  (caddr_t)&trc_el);
	
		wan_skb_copyback(new_skb, 
				  wan_skb_len(new_skb), 
				  wan_skb_len(skb),
				  (caddr_t)wan_skb_data(skb));
		
		wan_trace_enqueue(trace_info, new_skb);
	}

	return 0;
}

#if 0
static unsigned int dec_to_uint (unsigned char* str, int len)
{
	unsigned val;

	if (!len) 
		len = strlen(str);

	for (val = 0; len && is_digit(*str); ++str, --len)
		val = (val * 10) + (*str - (unsigned)'0');
	
	return val;
}
#endif

/*
 * ******************************************************************
 * Proc FS function 
 */
static int wan_aft_ss7_get_info(void* pcard, struct seq_file *m, int *stop_cnt)
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
