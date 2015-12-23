/*****************************************************************************
* sdla_aft_te3.c	WANPIPE(tm) AFT Xilinx Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 07, 2003	Nenad Corbic	Initial version.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe.h"
#include "wanpipe_abstr.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */

#if defined(__LINUX__)
# include "if_wanpipe.h"
#endif

#include "sdlapci.h"
#include "sdla_aft_te3.h"
#include "wanpipe_iface.h"          

//#define  XILINX_A010 	1	

#define DBGSTATS if(0)DEBUG_EVENT

/****** Defines & Macros ****************************************************/

/* Private critical flags */
enum {
	POLL_CRIT = PRIV_CRIT,
	CARD_DOWN
};

enum { 
	LINK_DOWN,
	DEVICE_DOWN
};

enum {
	TX_DMA_BUSY,
	TX_HANDLER_BUSY,
	TX_INTR_PENDING,
	TX_DATA_READY,

	RX_HANDLER_BUSY,
	RX_DMA_BUSY,
	RX_INTR_PENDING,
	RX_DATA_READY,

	RX_WTD_SKIP,
};

enum {
	AFT_FE_CFG_ERR,
	AFT_FE_CFG,
	AFT_FE_INTR,
	AFT_FE_POLL,
	AFT_FE_LED
};

#define MAX_IP_ERRORS	10

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )

#undef DEB_XILINX    

#if 0 
# define AFT_XTEST_DEBUG 1
#else
# undef AFT_XTEST_DEBUG
#endif

#if 1 
# define TRUE_FIFO_SIZE 1
#else
# undef  TRUE_FIFO_SIZE
# define HARD_FIFO_CODE 0x01
#endif


#if 0
#define AFT_T3_SINGLE_DMA_CHAIN 1
#else
#undef AFT_T3_SINGLE_DMA_CHAIN 
#endif

#define MAX_AFT_DMA_CHAINS 	16
#define MAX_TX_BUF		((MAX_AFT_DMA_CHAINS)*2)+1
#define MAX_RX_BUF		((MAX_AFT_DMA_CHAINS)*8)+1
#define MAX_RX_SCHAIN_BUF	(MAX_RX_BUF)*2


#define AFT_MAX_CHIP_SECURITY_CNT	100
/* Remove HDLC Address 
 * 1=Remove Enabled
 * 0=Remove Disabled
 */

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)


static int aft_rx_copyback=1000;

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following
 * structure will incorporate the card structure along with Protocol specific data
 */

typedef struct aft_dma_chain
{
	unsigned long	init;
	u32 		dma_addr;
	u32		dma_len;
	netskb_t 	*skb;
	u32		index;

	u32		dma_descr;
	u32		len_align;
	u32		reg;

	u8		pkt_error;
}aft_dma_chain_t;

typedef struct private_area
{
	wanpipe_common_t 	common;
	sdla_t			*card;
	netdevice_t	*dev;

	wan_skb_queue_t 	wp_tx_free_list;
	wan_skb_queue_t 	wp_tx_pending_list;
	wan_skb_queue_t 	wp_tx_complete_list;
	netskb_t 		*tx_dma_skb;
	u8			tx_dma_cnt;

	wan_skb_queue_t 	wp_rx_free_list;
	wan_skb_queue_t 	wp_rx_complete_list;

	unsigned long 		time_slot_map;
	unsigned char 		num_of_time_slots;
	long          		logic_ch_num;

	unsigned char		hdlc_eng;
	unsigned long		dma_status;
	unsigned char		protocol;
	unsigned char 		ignore_modem;

	struct net_device_stats	if_stats;
	aft_op_stats_t 		opstats;
        aft_comm_err_stats_t    errstats;
#if 1
	int 		tracing_enabled;		/* For enabling Tracing */
	wan_time_t 	router_start_time;
	unsigned long   trace_timeout;

	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	wan_time_t 	router_up_time;

	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned char 	udp_pkt_src;		/* udp packet processing */
	unsigned short 	timer_int_enabled;

	unsigned char 	interface_down;

	/* Polling task queue. Each interface
         * has its own task queue, which is used
         * to defer events from the interrupt */
	wan_taskq_t 	poll_task;
	wan_timer_info_t 	poll_delay_timer;

	u8 		gateway;
	u8 		true_if_encoding;

	//FIXME: add driver stats as per frame relay!
#endif

#if defined(__LINUX__)
	/* Entry in proc fs per each interface */
	struct proc_dir_entry	*dent;
#endif

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
	unsigned char	rx_dma;
	unsigned char   pci_retry;
	
	unsigned char	fifo_size_code;
	unsigned char	fifo_base_addr;
	unsigned char 	fifo_size;

	int		dma_mtu;

	void *		prot_ch;
	int		prot_state;

	wan_trace_t	trace_info;

	/* TE3 Specific Dma Chains */
	unsigned char	tx_chain_indx,tx_pending_chain_indx;
	aft_dma_chain_t tx_dma_chain_table[MAX_AFT_DMA_CHAINS];

	unsigned char	rx_chain_indx, rx_pending_chain_indx;
	aft_dma_chain_t rx_dma_chain_table[MAX_AFT_DMA_CHAINS];
	int		rx_no_data_cnt;

	unsigned long	dma_chain_status;
	unsigned long 	up;

	unsigned char   *tx_realign_buf;
	unsigned int	single_dma_chain;
	unsigned int	dma_bufs;

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

int wp_aft_te3_default_devcfg(sdla_t* card, wandev_conf_t* conf);
int wp_aft_te3_default_ifcfg(sdla_t* card, wanif_conf_t* conf);

/* WAN link driver entry points. These are called by the WAN router module. */
static int 	update (wan_device_t* wandev);
static int 	new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf);
static int 	del_if(wan_device_t *wandev, netdevice_t *dev);

/* Network device interface */
#if defined(__LINUX__)
static int 	if_init   (netdevice_t* dev);
#endif
static int 	if_open   (netdevice_t* dev);
static int 	if_close  (netdevice_t* dev);
static int 	if_do_ioctl(netdevice_t*, struct ifreq*, wan_ioctl_cmd_t);

#if defined(__LINUX__)
static int 	if_send (netskb_t* skb, netdevice_t* dev);
static struct net_device_stats* if_stats (netdevice_t* dev);
static int 	if_change_mtu(netdevice_t *dev, int new_mtu);
#else
static int if_send(netdevice_t *dev, netskb_t *skb, struct sockaddr *dst,struct rtentry *rt);
#endif

static void 	callback_front_end_state(void *card_id);
static void 	handle_front_end_state(void* card_id);
static void 	enable_timer(void* card_id);
static void 	if_tx_timeout (netdevice_t *dev);

/* Miscellaneous Functions */
static void 	port_set_state (sdla_t *card, int);

static void 	disable_comm (sdla_t *card);

/* Interrupt handlers */
static WAN_IRQ_RETVAL 	wp_aft_te3_isr (sdla_t* card);

/* Bottom half handlers */
#if defined(__LINUX__)
static void 	wp_bh (unsigned long);
#else
static void 	wp_bh (void*, int);
#endif

/* Miscellaneous functions */
static int 	process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				private_area_t*,
				int local_dev);

static int 	xilinx_t3_exar_chip_configure(sdla_t *card);
static int 	xilinx_t3_exar_dev_configure(sdla_t *card, private_area_t *chan);
static void 	xilinx_t3_exar_dev_unconfigure(sdla_t *card, private_area_t *chan);
static void 	xilinx_t3_exar_chip_unconfigure(sdla_t *card);
static void  	xilinx_t3_exar_transparent_config(sdla_t *card,private_area_t *chan);

static int 	xilinx_dma_rx(sdla_t *card, private_area_t *chan, int gcur_ptr);
static void 	xilinx_dev_enable(sdla_t *card, private_area_t *chan);
static void 	xilinx_dev_close(sdla_t *card, private_area_t *chan);
static void 	xilinx_dma_tx_complete (sdla_t *card, private_area_t *chan,int wtd);
static void 	xilinx_dma_rx_complete (sdla_t *card, private_area_t *chan, int wtd);
static int 	xilinx_init_rx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char);
static void 	xilinx_init_tx_dma_descr(sdla_t *card, private_area_t *chan);
static int 	xilinx_init_tx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char);
static void 	xilinx_tx_post_complete (sdla_t *card, private_area_t *chan, netskb_t *skb);
static void 	xilinx_rx_post_complete (sdla_t *card, private_area_t *chan,
                                     netskb_t *skb,
                                     netskb_t **new_skb,
                                     unsigned char *pkt_error);




#if 0
//FIXME: Not used check with M.F. if still needed
static unsigned char read_cpld(sdla_t *card, unsigned short cpld_off);
#endif
static int	write_cpld(void *pcard, unsigned short cpld_off,unsigned char cpld_data);
static int 	write_fe_cpld(void *pcard, unsigned short off,unsigned char data);

static int 	aft_devel_ioctl(sdla_t *card,struct ifreq *ifr);
static int 	xilinx_write_bios(sdla_t *card, wan_cmd_api_t *api_cmd);
static int 	xilinx_write(sdla_t *card, wan_cmd_api_t *api_cmd);
static int 	xilinx_read(sdla_t *card, wan_cmd_api_t *api_cmd);

static void 	front_end_interrupt(sdla_t *card, unsigned long reg);
static void  	enable_data_error_intr(sdla_t *card);
static void  	disable_data_error_intr(sdla_t *card, unsigned char);

static void 	xilinx_tx_fifo_under_recover (sdla_t *card, private_area_t *chan);

static int 	xilinx_write_ctrl_hdlc(sdla_t *card, u32 timeslot, u8 reg_off, u32 data);

static int 	set_chan_state(sdla_t* card, netdevice_t* dev, int state);

static int 	update_comms_stats(sdla_t* card);

static int 	protocol_init (sdla_t*card,netdevice_t *dev,
		               private_area_t *chan, wanif_conf_t* conf);
static int 	protocol_stop (sdla_t *card, netdevice_t *dev);
static int 	protocol_start (sdla_t *card, netdevice_t *dev);
static int 	protocol_shutdown (sdla_t *card, netdevice_t *dev);
static void 	protocol_recv(sdla_t *card, private_area_t *chan, netskb_t *skb);

static int 	aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num);
static int 	aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb);

static int 	xilinx_dma_te3_tx (sdla_t *card,private_area_t *chan);
static void 	aft_tx_dma_chain_handler(unsigned long data);
static void 	aft_tx_dma_chain_init(private_area_t *chan, aft_dma_chain_t *);
static void 	aft_rx_dma_chain_init(private_area_t *chan, aft_dma_chain_t *);
static void 	aft_index_tx_rx_dma_chains(private_area_t *chan);
static void 	aft_rx_dma_chain_handler(private_area_t *chan, int wtd, int reset);
#if 0
//FIXME: Currently not used...
static void 	aft_dma_te3_set_intr(aft_dma_chain_t *dma_chain, private_area_t *chan);
#endif
static void 	aft_init_tx_rx_dma_descr(private_area_t *chan);
static void 	aft_free_rx_complete_list(private_area_t *chan);
static void 	aft_list_descriptors(private_area_t *chan);
static void 	aft_free_rx_descriptors(private_area_t *chan);
static void	aft_te3_led_ctrl(sdla_t *card, int color, int led_pos, int on);
static void 	aft_list_tx_descriptors(private_area_t *chan);
static void 	aft_free_tx_descriptors(private_area_t *chan);

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))     
static void aft_port_task (void * card_ptr);
# else
static void aft_port_task (struct work_struct *work);	
# endif
#else
static void aft_port_task (void * card_ptr, int arg);
#endif

#if 0
static void 	aft_fe_intr_ctrl(sdla_t *card, int status);
#endif
static void 	__aft_fe_intr_ctrl(sdla_t *card, int status);

static void 	aft_reset_rx_chain_cnt(private_area_t *chan); 
static void 	aft_reset_tx_chain_cnt(private_area_t *chan); 
static void 	aft_critical_shutdown (sdla_t *card);
static int      aft_fifo_intr_ctrl(sdla_t *card, int ctrl);
static int 		xilinx_t3_exar_dev_configure_post(sdla_t *card, private_area_t *chan);

/* Procfs functions */
static int wan_aft3_get_info(void* pcard, struct seq_file* m, int* stop_cnt); 

/* Function interface between WANPIPE layer and kernel */
extern wan_iface_t	wan_iface;

static void xilinx_delay(int sec)
{
#if 0
	unsigned long timeout=SYSTEM_TICKS;
	while ((SYSTEM_TICKS-timeout)<(sec*HZ)){
		schedule();
	}
#endif
}

/**SECTION*********************************************************
 *
 * Public Functions
 *
 ******************************************************************/

int wp_aft_te3_default_devcfg(sdla_t* card, wandev_conf_t* conf)
{
	conf->config_id			= WANCONFIG_AFT_TE3;
	conf->u.xilinx.dma_per_ch	= MAX_RX_BUF;
	conf->u.xilinx.mru	= 1500;
	return 0;
}

int wp_aft_te3_default_ifcfg(sdla_t* card, wanif_conf_t* conf)
{
	conf->protocol = WANCONFIG_HDLC;
	memcpy(conf->usedby, "WANPIPE", 7);
	conf->if_down = 0;
	conf->ignore_dcd = WANOPT_NO;
	conf->ignore_cts = WANOPT_NO;
	conf->hdlc_streaming = WANOPT_NO;
	conf->mc = 0;
	conf->gateway = 0;
	conf->active_ch = ENABLE_ALL_CHANNELS;

	return 0;
}

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

int wp_aft_te3_init (sdla_t* card, wandev_conf_t* conf)
{
	int err;

	/* Verify configuration ID */
	wan_clear_bit(CARD_DOWN,&card->wandev.critical);
	if (card->wandev.config_id != WANCONFIG_AFT_TE3) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
				  card->devname, card->wandev.config_id);
		return -EINVAL;
	}

	if (conf == NULL){
		DEBUG_EVENT("%s: Bad configuration structre!\n",
				card->devname);
		return -EINVAL;
	}


	if (card->adptr_subtype == AFT_SUBTYPE_SHARK) {
		DEBUG_EVENT("%s: Starting SHARK T3/E3 Adapter!\n",
				card->devname);
	}

	card->hw_iface.getcfg(card->hw, SDLA_COREREV, &card->u.aft.firm_ver);
	card->hw_iface.getcfg(card->hw, SDLA_COREID, &card->u.aft.firm_id);

	/* Obtain hardware configuration parameters */
	card->wandev.clocking 			= conf->clocking;
	card->wandev.ignore_front_end_status 	= conf->ignore_front_end_status;
	card->wandev.ttl 			= conf->ttl;
	card->wandev.electrical_interface 			= conf->electrical_interface;
	card->wandev.comm_port 			= conf->comm_port;
	card->wandev.udp_port   		= conf->udp_port;
	card->wandev.new_if_cnt 		= 0;
	wan_atomic_set(&card->wandev.if_cnt,0);
	card->u.aft.chip_security_cnt=0;

	memcpy(&card->u.xilinx.cfg,&conf->u.xilinx,sizeof(wan_xilinx_conf_t));

	card->u.xilinx.cfg.dma_per_ch = MAX_RX_BUF;
		
	/* TE1 Make special hardware initialization for T1/E1 board */
	if (IS_TE3(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te3_iface_init(&card->wandev.fe_iface);
		card->fe.name = card->devname;
		card->fe.card = card;
		card->fe.write_cpld	= write_cpld;
//		card->fe.read_cpld	= read_cpld;

		card->fe.write_fe_cpld	= write_fe_cpld;

		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg	= card->hw_iface.fe_read;

		card->wandev.fe_enable_timer = enable_timer;
		card->wandev.te_link_state = callback_front_end_state;
//ALEX		conf->electrical_interface =
//			IS_T1_CARD(card) ? WANOPT_V35 : WANOPT_RS232;

		if (card->wandev.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else{
		DEBUG_EVENT("%s: Invalid Front-End media type!!\n",
				card->devname);
		return -EINVAL;
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
	card->wandev.get_info 		= &wan_aft3_get_info;

	/* Setup Port Bps */
	if(card->wandev.clocking) {
		card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
  	}

        /* For Primary Port 0 */
        card->wandev.mtu =
                (conf->mtu >= MIN_WP_PRI_MTU) ?
                 wp_min(conf->mtu, MAX_WP_PRI_MTU) : DEFAULT_WP_PRI_MTU;


	if (!card->u.xilinx.cfg.mru){
		card->u.xilinx.cfg.mru = card->wandev.mtu;
	}


	DEBUG_TEST("%s: Set MTU size to %d!\n", 
			card->devname, card->wandev.mtu);

	card->hw_iface.getcfg(card->hw, SDLA_BASEADDR, &card->u.xilinx.bar);

	xilinx_delay(1);
	port_set_state(card,WAN_DISCONNECTED);
	aft_te3_led_ctrl(card, WAN_AFT_RED, 0,WAN_AFT_ON);
	aft_te3_led_ctrl(card, WAN_AFT_GREEN, 0, WAN_AFT_OFF);

	WAN_TASKQ_INIT((&card->u.aft.port_task),0,aft_port_task,card);
	
        card->isr = &wp_aft_te3_isr;

	err=xilinx_t3_exar_chip_configure(card);
	if (err){
		return err;
	}

	xilinx_delay(1);

	/* Set protocol link state to disconnected,
	 * After seting the state to DISCONNECTED this
	 * function must return 0 i.e. success */

	DEBUG_EVENT( "%s: Init Done. Firm=%02X\n",card->devname,card->u.aft.firm_ver);
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

	if((chan=wan_netif_priv(dev)) == NULL)
		return -ENODEV;

       	if(card->update_comms_stats){
		return -EAGAIN;
	}

	DEBUG_TEST("%s: Chain Dma Status=0x%lX, TxCur=%i, TxPend=%i RxCur=%i RxPend=%i\n",
			chan->if_name, 
			chan->dma_chain_status,
			chan->tx_chain_indx,
			chan->tx_pending_chain_indx,
			chan->rx_chain_indx,
			chan->rx_pending_chain_indx);


	update_comms_stats(card);

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
			card->devname, wan_netif_name(dev));

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
	chan->single_dma_chain=0;

	if (conf->single_tx_buf) {
		chan->single_dma_chain=1;
	}

#ifdef AFT_T3_SINGLE_DMA_CHAIN
	chan->single_dma_chain=1;
#endif

	strncpy(chan->if_name, wan_netif_name(dev), WAN_IFNAME_SZ);

	chan->card = card;

	wan_skb_queue_init(&chan->wp_tx_free_list);
	wan_skb_queue_init(&chan->wp_tx_pending_list);
	wan_skb_queue_init(&chan->wp_tx_complete_list);
	
	wan_skb_queue_init(&chan->wp_rx_free_list);
	wan_skb_queue_init(&chan->wp_rx_complete_list);

	wan_trace_info_init(&chan->trace_info,MAX_TRACE_QUEUE);

	/* Initiaize Tx/Rx DMA Chains */
	aft_index_tx_rx_dma_chains(chan);
	
	/* Initialize the socket binding information
	 * These hooks are used by the API sockets to
	 * bind into the network interface */

	WAN_TASKLET_INIT((&chan->common.bh_task),0,wp_bh,chan);
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

		if (conf->protocol != WANOPT_NO){
			wan_netif_set_priv(dev,chan);
			if ((err=protocol_init(card,dev,chan,conf)) != 0){
				wan_netif_set_priv(dev, chan);
				goto new_if_error;
			}

			if (conf->ignore_dcd == WANOPT_YES || conf->ignore_cts == WANOPT_YES){
				DEBUG_EVENT( "%s: Ignore modem changes DCD/CTS\n",card->devname);
				chan->ignore_modem=1;
			}else{
				DEBUG_EVENT( "%s: Restart protocol on modem changes DCD/CTS\n",
						card->devname);
			}
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

	}else if (strcmp(conf->usedby, "STACK") == 0) {
		chan->common.usedby = STACK;
		if (chan->hdlc_eng){
			card->wandev.mtu+=32;
		}
		DEBUG_EVENT( "%s:%s: Running in Stack mode.\n",
				card->devname,chan->if_name);

		
	}else{
		DEBUG_ERROR( "%s:%s: Error: Invalid operation mode [WANPIPE|API|BRIDGE|BRIDGE_NODE]\n",
				card->devname,chan->if_name);
		err=-EINVAL;
		goto new_if_error;
	}

	xilinx_delay(1);

	chan->hdlc_eng = conf->hdlc_streaming;

	if (!chan->hdlc_eng){

		/* Enable single dma chain on Transparent mode */
		chan->single_dma_chain=1;

		if (card->wandev.mtu&0x03){
			DEBUG_ERROR("%s:%s: Error, Transparent MTU must be word aligned!\n",
					card->devname,chan->if_name);
			err = -EINVAL;
			goto new_if_error;
		}
	}
	chan->time_slot_map=conf->active_ch;

	err=xilinx_t3_exar_dev_configure(card,chan);
	if (err){
		goto new_if_error;
	}

	xilinx_delay(1);


	if (!chan->hdlc_eng){
		unsigned char *buf;

		if (!chan->max_idle_size){
			chan->max_idle_size=card->wandev.mtu;
		}
	
		DEBUG_EVENT("%s:%s: Config for Transparent mode: Idle=%X Len=%u\n",
			card->devname,chan->if_name,
			chan->idle_flag,chan->max_idle_size);

		chan->idle_flag=0x7E;     

		chan->tx_idle_skb = wan_skb_alloc(chan->max_idle_size); 
		if (!chan->tx_idle_skb){
			err=-ENOMEM;
			goto new_if_error;
		}
		buf=wan_skb_put(chan->tx_idle_skb,chan->max_idle_size);
		memset(buf,chan->idle_flag,chan->max_idle_size);
	}
	
	chan->dma_mtu = card->wandev.mtu >= card->u.xilinx.cfg.mru? 
				card->wandev.mtu:card->u.xilinx.cfg.mru; 
	
	chan->dma_mtu = xilinx_valid_mtu(chan->dma_mtu);
	if (!chan->dma_mtu){
		DEBUG_ERROR("%s:%s: Error invalid MTU %i  mru %i\n",
			card->devname,
			chan->if_name,
			card->wandev.mtu,card->u.xilinx.cfg.mru);
		err= -EINVAL;
		goto new_if_error;
	}

	chan->dma_bufs=card->u.xilinx.cfg.dma_per_ch;
	if (chan->single_dma_chain){
		chan->dma_bufs=MAX_RX_SCHAIN_BUF;
	}

	DEBUG_EVENT("%s:%s: Allocating %i dma len=%i Chains=%s\n",
			card->devname,chan->if_name,
			chan->dma_bufs,
			chan->dma_mtu,
			chan->single_dma_chain ? "Off":"On");

	
	err=aft_alloc_rx_dma_buff(card, chan, chan->dma_bufs);
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
	wan_netif_set_priv(dev,chan);
#if defined(__LINUX__)
	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);	
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,if_do_ioctl);
	WAN_NETDEV_OPS_MTU(dev,wan_netdev_ops,if_change_mtu);
# ifdef WANPIPE_GENERIC
	if_init(dev);
# endif
#else
	chan->common.is_netdev = 1;
	chan->common.iface.open = &if_open;
	chan->common.iface.close = &if_close;
	chan->common.iface.output = &if_send;
	chan->common.iface.ioctl = &if_do_ioctl;
	chan->common.iface.get_stats = &if_stats;
	chan->common.iface.tx_timeout = &if_tx_timeout;

	if (wan_iface.attach){
		if (!ifunit(wan_netif_name(dev))){
			wan_iface.attach(dev, NULL, chan->common.is_netdev);
		}
	}else{
		DEBUG_EVENT("%s: Failed to attach network interface %s!\n",
				card->devname, wan_netif_name(dev));
		wan_netif_set_priv(dev, NULL);
		err = -EINVAL;
		goto new_if_error;
	}
	wan_netif_set_mtu(dev, card->wandev.mtu);
#endif

	/* Increment the number of network interfaces
	 * configured on this card.
	 */
	wan_atomic_inc(&card->wandev.if_cnt);

	chan->common.state = WAN_CONNECTING;

	wan_set_bit(AFT_FE_LED,&card->u.aft.port_task_cmd);
	WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));
	

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
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	private_area_t* 	chan = wan_netif_priv(dev);
	sdla_t*			card = chan->card;
	netskb_t 		*skb;
	wan_smp_flag_t		flags;

	xilinx_t3_exar_dev_unconfigure(card,chan);
	
	WAN_TASKLET_KILL(&chan->common.bh_task);

	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

	protocol_shutdown(card,dev);

	
	wan_spin_lock_irq(&card->wandev.lock,&flags);

	while ((skb=wan_skb_dequeue(&chan->wp_rx_free_list)) != NULL){
		wan_skb_free(skb);
	}

	while ((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
		wan_skb_free(skb);
	}

	while ((skb=wan_skb_dequeue(&chan->wp_tx_free_list)) != NULL){
		wan_skb_free(skb);
	}

	if (chan->tx_realign_buf){
		wan_free(chan->tx_realign_buf);
		chan->tx_realign_buf=NULL;
	}

	if (chan->tx_idle_skb){
         	wan_skb_free(chan->tx_idle_skb);
               chan->tx_idle_skb=NULL;
       	}


	wan_spin_unlock_irq(&card->wandev.lock,&flags);

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
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t*		card = chan->card;
	wan_device_t* 	wandev = &card->wandev;
#ifdef WANPIPE_GENERIC
	hdlc_device*	hdlc;
#endif

	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);

#ifdef WANPIPE_GENERIC
	hdlc		= dev_to_hdlc(dev);
	hdlc->xmit 	= if_send;
#else
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
#endif
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);

	dev->watchdog_timeo	= 2*HZ;
#endif
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,if_do_ioctl);
	WAN_NETDEV_OPS_MTU(dev,wan_netdev_ops,if_change_mtu);	

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
			dev->hard_header_len	= 16;
			dev->addr_len		= 0;
		}

		if (chan->common.usedby == API){
			dev->mtu = card->wandev.mtu+sizeof(wp_api_hdr_t);
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
static int if_open (netdevice_t* dev)
{
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	wan_smp_flag_t flags;

#if defined(__LINUX__)
	/* Only one open per interface is allowed */
	if (open_dev_check(dev)){
		DEBUG_EVENT("%s: Open dev check failed!\n",
				wan_netif_name(dev));
		return -EBUSY;
	}
#endif

	/* Initialize the router start time.
	 * Used by wanpipemon debugger to indicate
	 * how long has the interface been up */
	wan_getcurrenttime(&chan->router_start_time, NULL);

	WAN_NETIF_STOP_QUEUE(dev);
	WAN_NETIF_CARRIER_OFF(dev);


        /* If FRONT End is down, it means that the DMA
         * is disabled.  In this case don't try to
         * reset fifo.  Let the enable_data_error_intr()
         * function do this, after front end has come up */

	wan_spin_lock_irq(&card->wandev.lock,&flags);
	if (card->wandev.state == WAN_CONNECTED){
		DEBUG_TEST("%s: OPEN reseting fifo\n",
				wan_netif_name(dev));
		xilinx_init_rx_dev_fifo(card,chan,WP_WAIT);
		xilinx_init_tx_dev_fifo(card,chan,WP_WAIT);
		xilinx_init_tx_dma_descr(card,chan);

		xilinx_dma_rx(card,chan,-1);
	}

	/* Check for transparent HDLC mode */
	if (!chan->hdlc_eng){
		/* The Transparent HDLC engine is
	         * enabled.  The Rx dma has already
        	 * been setup above.  Now setup
         	* TX DMA and enable the HDLC engine */

		DEBUG_CFG("%s: Transparent Tx Enabled!\n",
			wan_netif_name(dev));

		xilinx_t3_exar_transparent_config(card,chan);
	}

	xilinx_dev_enable(card,chan);
	wan_set_bit(0,&chan->up);

	if (card->wandev.state == WAN_CONNECTED){
                /* If Front End is connected already set interface
                 * state to Connected too */
                set_chan_state(card, dev, WAN_CONNECTED);
		WAN_NETIF_WAKE_QUEUE(dev);
	        WAN_NETIF_CARRIER_ON(dev);
		if (chan->common.usedby == API){
			wan_wakeup_api(chan);
		}else if (chan->common.usedby == STACK){
			wanpipe_lip_kick(chan,0);
		}          
	}      

	wan_spin_unlock_irq(&card->wandev.lock,&flags);

	chan->ignore_modem=0x0F;

	/* Increment the module usage count */
	wanpipe_open(card);

	protocol_start(card,dev);

	/* Wait for the front end interrupt 
	 * before enabling the card */
	return 0;
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

static int if_close (netdevice_t* dev)
{
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = chan->card;
	wan_smp_flag_t smp_flags;
	
	wan_clear_bit(0,&chan->up);

	WAN_NETIF_STOP_QUEUE(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	protocol_stop(card,dev);

	chan->common.state = WAN_DISCONNECTED;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	xilinx_dev_close(card,chan);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

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
	wan_smp_flag_t flags;

	/* Unconfiging, only on shutdown */
	if (IS_TE3(&card->fe.fe_cfg)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		} 
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
	}

	wan_spin_lock_irq(&card->wandev.lock,&flags);

	/* Disable DMA ENGINE before we perform 
         * core reset.  Otherwise, we will receive
         * rx fifo errors on subsequent resetart. */
	disable_data_error_intr(card,DEVICE_DOWN);
	
	wan_set_bit(CARD_DOWN,&card->wandev.critical);

	wan_spin_unlock_irq(&card->wandev.lock,&flags);

	WP_DELAY(10);
	aft_te3_led_ctrl(card, WAN_AFT_RED, 0,WAN_AFT_ON);
	aft_te3_led_ctrl(card, WAN_AFT_GREEN, 0, WAN_AFT_ON);

	xilinx_t3_exar_chip_unconfigure(card);
		
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
static void if_tx_timeout (netdevice_t *dev)
{
    	private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	unsigned int cur_dma_ptr;
	u32 reg;
	wan_smp_flag_t smp_flags;
	
	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this
	 * is only used as a last resort.
	 */

	++chan->if_stats.collisions;

	DEBUG_EVENT( "%s: Transmit timed out on %s\n",
				card->devname,wan_netif_name(dev));

//	DEBUG_EVENT("%s: TxStatus=0x%X  DMAADDR=0x%lX  DMALEN=%i \n",
//			chan->if_name,
//			chan->dma_status,
//			chan->tx_dma_addr,
//			chan->tx_dma_len);

	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
	cur_dma_ptr=get_current_tx_dma_ptr(reg);

	DEBUG_EVENT("%s: Chain TxIntrPend=%i, TxBusy=%i TxCur=%i, TxPend=%i HwCur=%i TxErr=%li\n",
			chan->if_name, 
			wan_test_bit(TX_INTR_PENDING,&chan->dma_chain_status),
			wan_test_bit(TX_DMA_BUSY,&chan->dma_status),
			chan->tx_chain_indx,
			chan->tx_pending_chain_indx,
			cur_dma_ptr,
		        chan->if_stats.tx_fifo_errors);
	
	/* The Interrupt didn't trigger.
	 * Clear the interrupt pending flag and
	 * let watch dog, clean up the tx chain */

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	aft_list_tx_descriptors(chan);
	xilinx_tx_fifo_under_recover(card,chan);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	if (!chan->single_dma_chain){	
		aft_enable_tx_watchdog(card,AFT_TX_TIMEOUT);
	}
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
#if defined(__LINUX__)
static int if_send (netskb_t* skb, netdevice_t* dev)
#else
static int if_send(netdevice_t *dev, netskb_t *skb, struct sockaddr *dst,struct rtentry *rt)
#endif
{

	private_area_t *chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	wan_smp_flag_t smp_flags;

	/* Mark interface as busy. The kernel will not
	 * attempt to send any more packets until we clear
	 * this condition */

	if (skb == NULL){
		/* This should never happen. Just a sanity check.
		 */
		DEBUG_EVENT( "%s: interface %s got kicked!\n",
			card->devname, wan_netif_name(dev));

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
		if((SYSTEM_TICKS - chan->tick_counter) < (5 * HZ)) {
			return 1;
		}

		if_tx_timeout(dev);
	}
#endif

	if (chan->common.state != WAN_CONNECTED){
		WAN_NETIF_STOP_QUEUE(dev);
		if (WAN_NETIF_CARRIER_OK(dev)){
			DEBUG_ERROR("%s: Critical Error: Carrier on on tx dev down\n",
					chan->if_name);
		}
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		++chan->if_stats.tx_carrier_errors;


		return 1;

	} else if (!WAN_NETIF_UP(dev)) {
		++chan->if_stats.tx_carrier_errors;
               	WAN_NETIF_START_QUEUE(dev); 
		wan_skb_free(skb);
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		return 0;       

	}else {
		int err=0;
		
		if (chan->common.usedby == API){
			if (sizeof(wp_api_hdr_t) >= wan_skb_len(skb)){
				wan_skb_free(skb);
				++chan->if_stats.tx_errors;
				WAN_NETIF_START_QUEUE(dev);
				goto if_send_exit_crit;
			}
			wan_skb_pull(skb,sizeof(wp_api_hdr_t));
		}

		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

		if (wan_skb_queue_len(&chan->wp_tx_pending_list) > MAX_TX_BUF){
			WAN_NETIF_STOP_QUEUE(dev);
			xilinx_dma_te3_tx(card,chan);	
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
			return 1;
		}

		wan_skb_unlink(skb);

		wan_skb_queue_tail(&chan->wp_tx_pending_list,skb);

		err=xilinx_dma_te3_tx(card,chan);	
		
		switch (err){

			case -EBUSY:
			case 0:
				WAN_NETIF_START_QUEUE(dev);
				wan_netif_set_ticks(dev, SYSTEM_TICKS);
				err=0;
				break;
				
			default:

				/* The packet was dropped
				 * by the tx chain handler.
				 * The tx_dropped stat was updated,
				 * thus all is left for us is
				 * to start the interface again.
				 * This SHOULD NEVER happen */
				if (WAN_NET_RATELIMIT()){
					DEBUG_EVENT("%s: TE3 Failed to send: Should never happend!\n",
						chan->if_name);
				}
				WAN_NETIF_START_QUEUE(dev);
				wan_netif_set_ticks(dev, SYSTEM_TICKS);
				err=0;
				break;
		}
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

		return err;
	}

if_send_exit_crit:

	return 0;
}


#if defined(__LINUX__)
/*============================================================================
 * if_stats
 *
 * Used by /proc/net/dev and ifconfig to obtain interface
 * statistics.
 *
 * Return a pointer to struct net_device_stats.
 */
static struct net_device_stats gstats;
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	private_area_t* chan;

	if ((chan=wan_netif_priv(dev)) == NULL)
		return &gstats;

#if 0
{
	sdla_t *card;
	card=chan->card;

	
	u8 *base_addr=card->u.xilinx.rx_dma_ptr;
	u8 *base_addr_tx=card->u.xilinx.tx_dma_ptr;
	u8 *addr=(u8*)wan_dma_get_vaddr(card,base_addr);
	u8 *addrtx=(u8*)wan_dma_get_vaddr(card,base_addr_tx);
	u8 *addr1, *addr0;

	addr+=chan->logic_ch_num*card->u.xilinx.dma_mtu_off;

	addr0=addr+0*card->u.xilinx.dma_mtu;
	addr1=addr+1*card->u.xilinx.dma_mtu;

	DEBUG_EVENT("%s: Buf 0: 0x%02X   1: 0x%02X  Txbuf: 0x%02X RxCompList=%i RxFreeList=%i  TxList=%i\n",
			wan_netif_name(dev),addr0[0],addr1[0],addrtx[0],
			wan_skb_queue_len(&chan->wp_rx_complete_list),
			wan_skb_queue_len(&chan->wp_rx_free_list),
			wan_skb_queue_len(&chan->wp_tx_pending_list));
}
#endif


#if 0
	 DEBUG_EVENT("%s: RxCompList=%i RxFreeList=%i  TxList=%i\n",
                        wan_netif_name(dev),
                        wan_skb_queue_len(&chan->wp_rx_complete_list),
                        wan_skb_queue_len(&chan->wp_rx_free_list),
                        wan_skb_queue_len(&chan->wp_tx_pending_list));
#endif


	return &chan->if_stats;
}
#endif

#ifdef __LINUX__
static int if_change_mtu(netdevice_t *dev, int new_mtu)
{
	private_area_t* chan= (private_area_t*)wan_netif_priv(dev);

	if (!chan){
		return -ENODEV;
	}

	if (!chan->hdlc_eng) {
		return -EINVAL;
	}

	if (chan->common.usedby == API){
		new_mtu+=sizeof(wp_api_hdr_t);
	}else if (chan->common.usedby == STACK){
		new_mtu+=32;
	}

	if (new_mtu > chan->dma_mtu) {
		return -EINVAL;
	}

	dev->mtu = new_mtu;
	
	return 0;
}
#endif


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
if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t cmd)
{
	private_area_t* chan= (private_area_t*)wan_netif_priv(dev);
	sdla_t *card;
#if defined(__LINUX__)
	wan_smp_flag_t smp_flags;
#endif
	wan_udp_pkt_t *wan_udp_pkt;
	int err=0;

	if (!chan){
		return -ENODEV;
	}
	card=chan->card;

	NET_ADMIN_CHECK();

	switch(cmd)
	{
#if defined(__LINUX__)
		case SIOC_WANPIPE_BIND_SK:
			if (!ifr){
				err= -EINVAL;
				break;
			}
			
			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			chan->if_stats.rx_dropped=0;
			chan->if_stats.tx_carrier_errors=0;
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
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
#endif
		case SIOC_WAN_DEVEL_IOCTL:
			err = aft_devel_ioctl(card, ifr);
			break;

		case SIOC_AFT_CUSTOMER_ID:
			err=0;
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
			if (WAN_COPY_FROM_USER(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (wan_test_bit(0,&card->in_isr)) {
				DEBUG_EVENT( "%s:%s Pipemon command failed, Driver busy: try again.\n",
						card->devname,
						wan_netif_name(chan->common.dev));
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

			if (WAN_COPY_TO_USER(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
				wan_atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			wan_atomic_set(&chan->udp_pkt_len,0);
			return 0;

		default:
#ifndef WANPIPE_GENERIC
			DEBUG_IOCTL("%s: Command %x not supported!\n",
					card->devname,cmd);
			return -EOPNOTSUPP;
#else
			if (card->wandev.ioctl){
				err = card->wandev.hdlc_ioctl(card, dev, ifr, cmd);
			}
#endif
	}

	return err;
}


/**SECTION**********************************************************
 *
 * 	FIRMWARE Specific Interface Functions
 *
 *******************************************************************/


#define FIFO_RESET_TIMEOUT_CNT 1000
#define FIFO_RESET_TIMEOUT_US  10
static int xilinx_init_rx_dev_fifo(sdla_t *card, private_area_t *chan, unsigned char wait)
{

        u32 reg;
        u32 dma_descr;
	u8  timeout=1;
	u16 i;
	unsigned int cur_dma_ptr;

	/* Clean RX DMA fifo */
	aft_reset_rx_chain_cnt(chan);
 	
        card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
      	cur_dma_ptr=get_current_rx_dma_ptr(reg);        
	
        dma_descr=(u32)(cur_dma_ptr<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
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
				WP_DELAY(FIFO_RESET_TIMEOUT_US);
				continue;
			}
			timeout=0;
			break;
		} 

		if (timeout){
			DEBUG_ERROR("%s:%s: Error: Rx fifo reset timedout %u us\n",
				card->devname,chan->if_name,i*FIFO_RESET_TIMEOUT_US);
		}else{
			DEBUG_TEST("%s:%s: Rx Fifo Reset Successful\n",
				card->devname,chan->if_name); 
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
	unsigned int cur_dma_ptr;

	aft_reset_tx_chain_cnt(chan);

      	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
       	cur_dma_ptr=get_current_tx_dma_ptr(reg);   

        /* Clean TX DMA fifo */
        dma_descr=(u32)(cur_dma_ptr<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
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
                        	WP_DELAY(FIFO_RESET_TIMEOUT_US);
                        	continue;
                	}
               		timeout=0;
               		break;
        	}

        	if (timeout){
                	DEBUG_ERROR("%s:%s: Error: Tx fifo reset timedout %u us\n",
                                card->devname,chan->if_name,i*FIFO_RESET_TIMEOUT_US);
        	}else{
                	DEBUG_TEST("%s:%s: Tx Fifo Reset Successful\n",
                                card->devname,chan->if_name);
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



static void xilinx_dev_close(sdla_t *card, private_area_t *chan)
{
	u32 reg;

    	DEBUG_CFG("-- Close Xilinx device. --\n");

        /* Disable Logic Channel Interrupts for DMA and fifo */
        card->hw_iface.bus_read_4(card->hw,
                                  XILINX_GLOBAL_INTER_MASK, &reg);

        wan_clear_bit(chan->logic_ch_num,&reg);
	wan_clear_bit(chan->logic_ch_num,&card->u.xilinx.active_ch_map);

	/* We are masking the chan interrupt. 
         * Lock to make sure that the interrupt is
         * not running */
        card->hw_iface.bus_write_4(card->hw,
                                  XILINX_GLOBAL_INTER_MASK, reg);


	aft_reset_rx_watchdog(card);
	aft_reset_tx_watchdog(card);

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

	/* Initialize DMA descriptors and DMA Chains */
	aft_init_tx_rx_dma_descr(chan);

}

/**SECTION*************************************************************
 *
 * 	TX Handlers
 *
 **********************************************************************/


/*===============================================
 * xilinx_dma_tx_complete
 *
 */
static void xilinx_dma_tx_complete (sdla_t *card, private_area_t *chan, int wtd)
{
	DEBUG_TEST("%s: Tx interrupt wtd=%d\n",chan->if_name,wtd);

	aft_reset_tx_watchdog(card);

	if (!wtd){
		wan_clear_bit(TX_INTR_PENDING,&chan->dma_chain_status);
	}

	aft_tx_dma_chain_handler((unsigned long)chan);

	
	if (!wtd) {
       		xilinx_dma_te3_tx(card,chan);
       	}

	if (WAN_NETIF_QUEUE_STOPPED(chan->common.dev)){
		WAN_NETIF_WAKE_QUEUE(chan->common.dev);
#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC
		if (chan->common.usedby == API){
			wan_wakeup_api(chan);
		}else if (chan->common.usedby == STACK){
			wanpipe_lip_kick(chan,0);
		}
#endif
	}

	if (!wtd) {
               wan_set_bit(0,&chan->idle_start);
        }

	if (!chan->single_dma_chain){	
		aft_enable_tx_watchdog(card,AFT_TX_TIMEOUT);
	}

	return;
}

/*===============================================
 * xilinx_tx_post_complete
 *
 */
static void xilinx_tx_post_complete (sdla_t *card, private_area_t *chan, netskb_t *skb)
{
	unsigned long reg =  wan_skb_csum(skb);

	if ((wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)) ||
	    (reg & TxDMA_HI_DMA_DATA_LENGTH_MASK) ||
	    (reg&TxDMA_HI_DMA_PCI_ERROR_MASK)){

		/* We can get tx latency timeout thus dont print in that case */
		if (!(reg & TxDMA_HI_DMA_PCI_ERROR_DS_TOUT)){
			DEBUG_EVENT("%s:%s: Tx DMA Descriptor=0x%lX\n",
				card->devname,chan->if_name,reg);
		}

		/* Checking Tx DMA Go bit. Has to be '0' */
		if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
        		DEBUG_TEST("%s:%s: Error: TxDMA Intr: GO bit set on Tx intr\n",
                   		card->devname,chan->if_name);
			chan->errstats.Tx_dma_errors++;
		}

		if (reg & TxDMA_HI_DMA_DATA_LENGTH_MASK){
               		DEBUG_TEST("%s:%s: Error: TxDMA Length not equal 0 \n",
                   		card->devname,chan->if_name);
			chan->errstats.Tx_dma_len_nonzero++;
	        }   
 
    		/* Checking Tx DMA PCI error status. Has to be '0's */
		if (reg&TxDMA_HI_DMA_PCI_ERROR_MASK){
        
			chan->errstats.Tx_pci_errors++;
        	     	
			if (reg & TxDMA_HI_DMA_PCI_ERROR_M_ABRT){
        			DEBUG_ERROR("%s:%s: Tx Error: Abort from Master: pci fatal error!\n",
                	     		card->devname,chan->if_name);
			}
			if (reg & TxDMA_HI_DMA_PCI_ERROR_T_ABRT){
        			DEBUG_ERROR("%s:%s: Tx Error: Abort from Target: pci fatal error!\n",
                	     		card->devname,chan->if_name);
			}
			if (reg & TxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
				if (WAN_NET_RATELIMIT()) {
        			DEBUG_WARNING("%s:%s: Tx Warning: PCI Latency Timeout!\n",
                	     		card->devname,chan->if_name);
				}
				chan->errstats.Tx_pci_latency++;
				goto tx_post_ok;
			}
			if (reg & TxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
        			DEBUG_ERROR("%s:%s: Tx Error: 'Retry' exceeds maximum (64k): pci fatal error!\n",
                	     		card->devname,chan->if_name);
			}
		}
		chan->if_stats.tx_errors++;
		goto tx_post_exit;
	}

tx_post_ok:
        chan->opstats.Data_frames_Tx_count++;
        chan->opstats.Data_bytes_Tx_count+=wan_skb_len(skb);
	chan->if_stats.tx_packets++;
	chan->if_stats.tx_bytes+=wan_skb_len(skb);

        /* Indicate that the first tx frame went
         * out on the transparent link */
        wan_set_bit(0,&chan->idle_start);

	wan_capture_trace_packet(card, &chan->trace_info, skb, TRC_OUTGOING_FRM);

tx_post_exit:

	return;
}



/**SECTION*************************************************************
 *
 * 	RX Handlers
 *
 **********************************************************************/

/*===============================================
 * xilinx_dma_rx_complete
 *
 */
static void xilinx_dma_rx_complete (sdla_t *card, private_area_t *chan, int wtd)
{
	aft_reset_rx_watchdog(card);
	aft_rx_dma_chain_handler(chan,wtd,0);
}


static int aft_check_pci_errors(sdla_t *card, private_area_t *chan, wp_rx_element_t *rx_el)
{
	int pci_err=0;
	 if (rx_el->reg&RxDMA_HI_DMA_PCI_ERROR_MASK){

                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_M_ABRT){
			if (WAN_NET_RATELIMIT()) {
                        DEBUG_ERROR("%s:%s: Rx Error: Abort from Master: pci fatal error!\n",
                                   card->devname,chan->if_name);
			}
			pci_err=1;
                }
                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_T_ABRT){
			if (WAN_NET_RATELIMIT()) {
                        DEBUG_ERROR("%s:%s: Rx Error: Abort from Target: pci fatal error!\n",
                                   card->devname,chan->if_name);
			}
			pci_err=1;
                }
                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
			if (WAN_NET_RATELIMIT()) {
                        DEBUG_ERROR("%s:%s: Rx Error: No 'DeviceSelect' from target: pci fatal error!\n",
                                    card->devname,chan->if_name);
			}
			pci_err=1;
                }
                if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
			if (WAN_NET_RATELIMIT()) {
                        DEBUG_ERROR("%s:%s: Rx Error: 'Retry' exceeds maximum (64k): pci fatal error!\n",
                                    card->devname,chan->if_name);
			}
			pci_err=1;
                }

		if (!pci_err) {
			if (WAN_NET_RATELIMIT()) {
                	DEBUG_ERROR("%s: RXDMA Unknown PCI ERROR = 0x%x\n",chan->if_name,rx_el->reg);
			}
		}


		return -1;
        }



	return 0;
}



/*===============================================
 * xilinx_rx_post_complete
 *
 */
static void xilinx_rx_post_complete (sdla_t *card, private_area_t *chan, 
				     netskb_t *skb, 
				     netskb_t **new_skb,
				     unsigned char *pkt_error)
{

    	unsigned int len,data_error = 0;
	unsigned char *buf;

#if 0
	wp_rx_element_t *rx_el=(wp_rx_element_t *)&skb->cb[0];
#else
	wp_rx_element_t *rx_el;
	rx_el=(wp_rx_element_t *)wan_skb_data(skb);
#endif
	DEBUG_RX("%s:%s: RX HI=0x%X  LO=0x%X\n DMA=0x%lX",
		__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);   
	
#if 0
	chan->if_stats.rx_errors++;
#endif
	
	rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;
	*pkt_error=0;
	*new_skb=NULL;

	
    	/* Checking Rx DMA Go bit. Has to be '0' */
	if (wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&rx_el->reg)){
		if (WAN_NET_RATELIMIT()){
        	DEBUG_ERROR("%s:%s: Error: RxDMA Intr: GO bit set on Rx intr\n",
				card->devname,chan->if_name);
		}
		chan->if_stats.rx_errors++;
		chan->errstats.Rx_dma_descr_err++;
		goto rx_comp_error;
	}
   
	if (aft_check_pci_errors(card,chan,rx_el) != 0) {
		chan->errstats.Rx_pci_errors++;
		chan->if_stats.rx_errors++;
		goto rx_comp_error;
	}
 
	if (chan->hdlc_eng){
 
		/* Checking Rx DMA Frame start bit. (information for api) */
		if (!wan_test_bit(RxDMA_HI_DMA_FRAME_START_BIT,&rx_el->reg)){
			DEBUG_TEST("%s:%s RxDMA Intr: Start flag missing: MTU Mismatch! Reg=0x%X\n",
					card->devname,chan->if_name,rx_el->reg);
			chan->if_stats.rx_frame_errors++;
     			chan->opstats.Rx_Data_discard_long_count++;
                        chan->errstats.Rx_hdlc_corrupiton++;
			goto rx_comp_error;
		}
    
		/* Checking Rx DMA Frame end bit. (information for api) */
		if (!wan_test_bit(RxDMA_HI_DMA_FRAME_END_BIT,&rx_el->reg)){
			DEBUG_TEST("%s:%s: RxDMA Intr: End flag missing: MTU Mismatch! Reg=0x%X\n",
					card->devname,chan->if_name,rx_el->reg);
			chan->if_stats.rx_frame_errors++;
     			chan->opstats.Rx_Data_discard_long_count++;
                        chan->errstats.Rx_hdlc_corrupiton++;
			goto rx_comp_error;
		
       	 	} else {  /* Check CRC error flag only if this is the end of Frame */
        	
			if (wan_test_bit(RxDMA_HI_DMA_CRC_ERROR_BIT,&rx_el->reg)){
                   		DEBUG_TEST("%s:%s: RxDMA Intr: CRC Error! Reg=0x%X Len=%i\n",
                                		card->devname,chan->if_name,rx_el->reg,
						(rx_el->reg&RxDMA_HI_DMA_DATA_LENGTH_MASK)>>2);
				chan->if_stats.rx_frame_errors++;
			 	chan->opstats.Rx_Data_discard_long_count++;
                        	chan->errstats.Rx_hdlc_corrupiton++;
				wan_set_bit(WP_CRC_ERROR_BIT,&rx_el->pkt_error);	
                   		data_error = 1;
               		}

			/* Check if this frame is an abort, if it is
                 	 * drop it and continue receiving */
			if (wan_test_bit(RxDMA_HI_DMA_FRAME_ABORT_BIT,&rx_el->reg)){
				DEBUG_TEST("%s:%s: RxDMA Intr: Abort! Reg=0x%X\n",
						card->devname,chan->if_name,rx_el->reg);
				chan->if_stats.rx_frame_errors++;
     				chan->opstats.Rx_Data_discard_long_count++;
                        	chan->errstats.Rx_hdlc_corrupiton++;
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
		len=((((chan->dma_mtu>>2)-1)-len)<<2) - (~(rx_el->align)&RxDMA_LO_ALIGNMENT_BIT_MASK);
	}else{
		/* In Transparent mode, our RX buffer will always be
		 * aligned to the 32bit (word) boundary, because
                 * the RX buffers are all of equal length  */
		len=(((card->wandev.mtu>>2)-len)<<2) - (~(0x03)&RxDMA_LO_ALIGNMENT_BIT_MASK);
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

	//ORG wan_skb_pull(skb, sizeof(wp_rx_element_t));
	
	if (len > aft_rx_copyback){
		/* The rx size is big enough, thus
		 * send this buffer up the stack
		 * and allocate another one */
		memset(wan_skb_data(skb), 0, sizeof(wp_rx_element_t));
#if defined(__FreeBSD__)
		wan_skb_trim(skb, sizeof(wp_rx_element_t));
#endif
		wan_skb_put(skb,len);	
		wan_skb_pull(skb, sizeof(wp_rx_element_t));
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
#if defined(__FreeBSD__)
		wan_skb_trim(skb, sizeof(wp_rx_element_t));
#endif
		memcpy(buf,wan_skb_tail(skb),len);

		aft_init_requeue_free_skb(chan, skb);
	}
	
	return;

rx_comp_error:

	aft_init_requeue_free_skb(chan, skb);
    	return;
}



/**SECTION**************************************************
 *
 * 	Logic Channel Registration Support and
 * 	Utility funcitons
 *
 **********************************************************/

static int aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb)
{
	wan_skb_init(skb,sizeof(wp_api_hdr_t));
	wan_skb_trim(skb,0);
#if 0
	memset(&skb->cb[0],0,sizeof(wp_rx_element_t));
#endif		
	wan_skb_queue_tail(&chan->wp_rx_free_list,skb);

	return 0;
}

static int aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num)
{
	int i;
	netskb_t *skb;
	
	for (i=0;i<num;i++){
		skb=wan_skb_alloc(chan->dma_mtu);
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
	sdla_t*	card = (sdla_t*)card_id;
#if !defined(WAN_IS_TASKQ_SCHEDULE)
	wan_smp_flag_t smp_flags, smp_flags1;
#endif

	DEBUG_TEST("%s: %s Sdla Polling!\n",__FUNCTION__,card->devname);

#if defined(WAN_IS_TASKQ_SCHEDULE)
	wan_set_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd);
	WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));
#else
	card->hw_iface.hw_lock(card->hw,&smp_flags1);
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	WAN_FECALL(&card->wandev, polling, (&card->fe));
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags1);
#endif
	return;
}

/**SECTION**************************************************
 *
 * 	API Bottom Half Handlers
 *
 **********************************************************/

#if defined(__LINUX__)
static void wp_bh (unsigned long data)
#else
static void wp_bh (void* data, int dummy)
#endif
{
	private_area_t	*chan = (private_area_t *)data;
	netskb_t	*new_skb,*skb;
	unsigned char	pkt_error;
	wan_ticks_t	timeout=SYSTEM_TICKS;
	
	DEBUG_TEST("%s: ------------ BEGIN --------------: %lu\n",
			__FUNCTION__,SYSTEM_TICKS);

	if (!wan_test_bit(0,&chan->up)){
		DEBUG_EVENT("%s: wp_bh() chan not up!\n",
                                chan->if_name);
		WAN_TASKLET_END((&chan->common.bh_task));
		return;
	}

	
	while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){

#if 0
		chan->if_stats.rx_errors++;
#endif

		if (chan->common.usedby == API && chan->common.sk == NULL){
			DEBUG_TEST("%s: No sock bound to channel rx dropping!\n",
				chan->if_name);
			chan->if_stats.rx_dropped++;
			aft_init_requeue_free_skb(chan, skb);

			continue;
		}

		new_skb=NULL;
		pkt_error=0;

		/* The post function will take care
		 * of the skb and new_skb buffer.
		 * If new_skb buffer exists, driver
		 * must pass it up the stack, or free it */
		xilinx_rx_post_complete (chan->card, chan,
                                   	 skb,
                                     	 &new_skb,
                                     	 &pkt_error);
		if (new_skb){
	
			int len=wan_skb_len(new_skb);

			wan_capture_trace_packet(chan->card, &chan->trace_info,
					     new_skb,TRC_INCOMING_FRM);
		
			if (chan->common.usedby == API){
#if defined(__LINUX__)
# ifndef CONFIG_PRODUCT_WANPIPE_GENERIC

				/* Only for API, we insert packet status
				 * byte to indicate a packet error. Take
			         * this byte and put it in the api header */

				if (wan_skb_headroom(new_skb) >= sizeof(wp_api_hdr_t)){
					wp_api_hdr_t *rx_hdr=
						(wp_api_hdr_t*)skb_push(new_skb,sizeof(wp_api_hdr_t));	
					memset(rx_hdr,0,sizeof(wp_api_hdr_t));
					rx_hdr->wp_api_rx_hdr_error_flag=pkt_error;
				}else{
					int hroom=wan_skb_headroom(new_skb);
					int rx_sz=sizeof(wp_api_hdr_t);
					DEBUG_ERROR("%s: Error Rx pkt headroom %i < %i\n",
							chan->if_name,
							hroom,
							rx_sz);
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}

				new_skb->protocol = htons(PVC_PROT);
				wan_skb_reset_mac_header(new_skb);
				new_skb->dev      = chan->common.dev;
				new_skb->pkt_type = WAN_PACKET_DATA;	
#if 0	
				chan->if_stats.rx_frame_errors++;
#endif
				if (wan_api_rx(chan,new_skb) != 0){
					DEBUG_TEST("%s: Error: Rx Socket busy!\n",
						chan->if_name);
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}
# endif
#endif

			}else if (chan->common.usedby == STACK){

				if (wanpipe_lip_rx(chan,new_skb) != 0){
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}
				
			}else{
				protocol_recv(chan->card,chan,new_skb);
			}

     			chan->opstats.Data_frames_Rx_count++;
                        chan->opstats.Data_bytes_Rx_count+=len;
			chan->if_stats.rx_packets++;
			chan->if_stats.rx_bytes+=len;
		}

		if (SYSTEM_TICKS-timeout > 3){
			if (WAN_NET_RATELIMIT()){
				DEBUG_EVENT("%s: BH Squeeze! %ld\n", 
						chan->if_name,(unsigned long)(SYSTEM_TICKS-timeout));
			}
			break;
		}
	}

	while((skb=wan_skb_dequeue(&chan->wp_tx_complete_list)) != NULL){
		xilinx_tx_post_complete (chan->card,chan,skb);
		wan_skb_free(skb);
	}


	WAN_TASKLET_END((&chan->common.bh_task));
#if 1	
	{
	int len;
	if ((len=wan_skb_queue_len(&chan->wp_rx_complete_list))){
		DEBUG_TEST("%s: Triggering from bh rx=%i\n",chan->if_name,len); 
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));	
	}else if ((len=wan_skb_queue_len(&chan->wp_tx_complete_list))){
                DEBUG_TEST("%s: Triggering from bh tx=%i\n",chan->if_name,len); 
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));	
        }
	}
#endif

	DEBUG_TEST("%s: ------------ END -----------------: %lu\n",
                        __FUNCTION__,SYSTEM_TICKS);

	return;
}

/**SECTION**************************************************
 *
 * 	Interrupt Support Functions
 *
 **********************************************************/

static int fifo_error_interrupt(sdla_t *card, u32 reg, u32 tx_status, u32 rx_status)
{
	u32 err=0;
	u32 i;
	private_area_t *chan;
	int num_of_logic_ch;
	

	if (card->wandev.state != WAN_CONNECTED){
        	DEBUG_ERROR("%s: Warning: Ignoring Error Intr: link disc!\n",
                                  card->devname);
                return 0;
        }
	
	DEBUG_TEST("%s: RX FIFO=0x%08X TX FIFO=0x%08X\n",
			card->devname,rx_status,tx_status);

	if (IS_TE3(&card->fe.fe_cfg)){
		num_of_logic_ch=1;
	}else{
		num_of_logic_ch=card->u.xilinx.num_of_time_slots;
	}
	
        if (tx_status != 0){
		for (i=0;i<num_of_logic_ch;i++){
			if (wan_test_bit(i,&tx_status) && wan_test_bit(i,&card->u.xilinx.logic_ch_map)){
				
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					if (WAN_NET_RATELIMIT()) {
					DEBUG_ERROR("Warning: ignoring tx error intr: no dev!\n");
					}
					continue;
				}

				if (!wan_test_bit(0,&chan->up)){
					if (WAN_NET_RATELIMIT()) {
					DEBUG_ERROR("%s: Warning: ignoring tx error intr: dev down 0x%X  UP=0x%X!\n",
						wan_netif_name(chan->common.dev),chan->common.state,chan->ignore_modem);
					}
					continue;
				}

				if (chan->common.state != WAN_CONNECTED){
					if (WAN_NET_RATELIMIT()) {
					DEBUG_ERROR("%s: Warning: ignoring tx error intr: dev disc!\n",
                                                wan_netif_name(chan->common.dev));
					}
					continue;
				}

				if (!chan->hdlc_eng && !wan_test_bit(0,&chan->idle_start)){
					if (WAN_NET_RATELIMIT()) {
						DEBUG_ERROR("%s: Warning: ignoring tx error intr: dev init error!\n",
                                                	wan_netif_name(chan->common.dev));
					}
					if (chan->hdlc_eng){
						xilinx_tx_fifo_under_recover(card,chan);
					}
                                        continue;
				}
                		DEBUG_TEST("%s:%s: Warning TX Fifo Error on LogicCh=%li Slot=%i!\n",
                           		card->devname,chan->if_name,chan->logic_ch_num,i);

				xilinx_tx_fifo_under_recover(card,chan);
				++chan->if_stats.tx_fifo_errors;
				err=-EINVAL;
			}
		}
        }


        if (rx_status != 0){
		for (i=0;i<num_of_logic_ch;i++){
			if (wan_test_bit(i,&rx_status) && wan_test_bit(i,&card->u.xilinx.logic_ch_map)){
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					continue;
				}

				if (!wan_test_bit(0,&chan->up)){
					DEBUG_ERROR("%s: Warning: ignoring rx error intr: dev down 0x%X UP=0x%X!\n",
						wan_netif_name(chan->common.dev),chan->common.state,chan->ignore_modem);
					continue;
				}

				if (chan->common.state != WAN_CONNECTED){
					DEBUG_ERROR("%s: Warning: ignoring rx error intr: dev disc!\n",
                                                wan_netif_name(chan->common.dev));
                                        continue;
                                }

                		DEBUG_TEST("%s:%s: Warning RX Fifo Error on LCh=%li Slot=%i RxCL=%i RxFL=%i RxDMA=%i\n",
                           		card->devname,chan->if_name,chan->logic_ch_num,i,
					wan_skb_queue_len(&chan->wp_rx_complete_list),
					wan_skb_queue_len(&chan->wp_rx_free_list),
					chan->rx_dma);

				++chan->if_stats.rx_fifo_errors;
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
	DEBUG_TE3("%s: front_end_interrupt!\n",card->devname);

//	wp_debug_func_add(__FUNCTION__);

	if (IS_TE3(&card->fe.fe_cfg)){
		/* FIXME HANDLE T3 Interrupt */
		WAN_FECALL(&card->wandev, isr, (&card->fe));
	}else{
		DEBUG_ERROR("%s: Internal Error (Never should happened)!\n",
				card->devname);
	}
	
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
static WAN_IRQ_RETVAL wp_aft_te3_isr (sdla_t* card)
{
    	int i;
	u32 reg,cfg_reg;
	u32 dma_tx_reg,dma_rx_reg,rx_fifo_status=0,tx_fifo_status=0;
	private_area_t *chan;
	int skip_rx_wtd=0;
	WAN_IRQ_RETVAL_DECL(irq_ret);

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		WAN_IRQ_RETURN(irq_ret);
	}

    	wan_set_bit(0,&card->in_isr);

       /* -----------------2/6/2003 9:02AM------------------
     	* Disable all chip Interrupts  (offset 0x040)
     	*  -- "Transmit/Receive DMA Engine"  interrupt disable
     	*  -- "FiFo/Line Abort Error"        interrupt disable
     	* --------------------------------------------------*/
        card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);
	cfg_reg=reg;

	DEBUG_TEST("\n");
	DEBUG_TEST("%s:  ISR (0x%X) = 0x%08X \n",
			card->devname,XILINX_CHIP_CFG_REG,reg);

	if (wan_test_bit(ENABLE_TE3_FRACTIONAL,&reg)){
		unsigned int frc_crc;
		u32 freg;

		card->hw_iface.bus_read_4(card->hw,TE3_FRACT_ENCAPSULATION_REG, &freg);
		frc_crc=get_te3_rx_fract_crc_cnt(freg);	
		if (frc_crc){
			if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: TE3 Frac CRC Cnt = %i  0x%08X\n",
				card->devname, frc_crc,reg);	
			}
		}
	}
	
	if (wan_test_bit(SECURITY_STATUS_FLAG,&reg)){
		WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
		if (++card->u.aft.chip_security_cnt > 
				AFT_MAX_CHIP_SECURITY_CNT){

#if 1
			if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: Critical: Chip Security Compromised: Disabling Driver (%i)!\n",
				card->devname,card->u.aft.chip_security_cnt);
			}
			card->u.aft.chip_security_cnt=0;
			aft_critical_shutdown(card);
			goto isr_end; 
#else
			
			DEBUG_EVENT("%s: Critical: Chip Security Compromised: Disabling Driver (%i)!\n",
				card->devname,card->u.aft.chip_security_cnt);
			DEBUG_EVENT("%s: Please call Sangoma Tech Support (www.sangoma.com)!\n",
				card->devname);


			aft_critical_shutdown(card);
			goto isr_end;
#endif
		}
	}else{
		card->u.aft.chip_security_cnt=0;
	}

	/* Note: If interrupts are received without pending
         * flags, it usually indicates that the interrupt
         * is being shared.  (Check 'cat /proc/interrupts') 
	 */

        if (wan_test_bit(FRONT_END_INTR_ENABLE_BIT,&reg)){
		if (wan_test_bit(FRONT_END_INTR_FLAG,&reg)){
			WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
#if defined(__LINUX__) || defined(__FreeBSD__)
			wan_set_bit(AFT_FE_INTR,&card->u.aft.port_task_cmd);
			WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));	

			__aft_fe_intr_ctrl(card,0);
#else
#error "Front end not called from interrupt please resolve!"
#error "LED and Wanpipe Status will not be properly updated!"
              		front_end_interrupt(card,reg);
#endif
		}
        }

	/* Test Fifo Error Interrupt,
	 * If set shutdown all interfaces and
         * reconfigure */
	if (wan_test_bit(ERROR_INTR_ENABLE_BIT,&reg)){ 
        	if (wan_test_bit(ERROR_INTR_FLAG,&reg)){
			WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);

			card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG,&tx_fifo_status);
        		card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG,&rx_fifo_status);

			rx_fifo_status&=card->u.aft.active_ch_map;
			tx_fifo_status&=card->u.aft.active_ch_map;

			fifo_error_interrupt(card,reg,tx_fifo_status,rx_fifo_status);
		}
	}

       /* -----------------2/6/2003 9:37AM------------------
      	* Checking for Interrupt source:
      	* 1. Receive DMA Engine
      	* 2. Transmit DMA Engine
      	* 3. Error conditions.
      	* --------------------------------------------------*/
    	if (wan_test_bit(GLOBAL_INTR_ENABLE_BIT,&reg) &&
            (wan_test_bit(DMA_INTR_FLAG,&reg) || rx_fifo_status)){

		int num_of_logic_ch;
			
		WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);

		if (IS_TE3(&card->fe.fe_cfg)){
			num_of_logic_ch=1;
		}else{
			num_of_logic_ch=card->u.xilinx.num_of_time_slots;
		}

        	/* Receive DMA Engine */
		card->hw_iface.bus_read_4(card->hw,
                                XILINX_DMA_RX_INTR_PENDING_REG, 
                                &dma_rx_reg);

		DEBUG_TEST("%s: DMA_RX_INTR_REG(0x%X) = 0x%X  ActCH=0x%lX\n",
				card->devname,
				XILINX_DMA_RX_INTR_PENDING_REG,dma_rx_reg,
				card->u.xilinx.active_ch_map);

		dma_rx_reg&=card->u.xilinx.active_ch_map;
		
		if (dma_rx_reg == 0 && rx_fifo_status == 0){
			goto isr_skb_rx;
		}
		
		for (i=0; i<num_of_logic_ch;i++){
			if ((wan_test_bit(i,&dma_rx_reg)|| wan_test_bit(i,&rx_fifo_status)) && 
			     wan_test_bit(i,&card->u.xilinx.logic_ch_map)){

				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					DEBUG_ERROR("%s: Error: No Dev for Rx logical ch=%i\n",
							card->devname,i);
					continue;
				}

				if (!wan_test_bit(0,&chan->up)){
					DEBUG_ERROR("%s: Error: Dev not up for Rx logical ch=%i\n",
                                                        card->devname,i);
                                        continue;
				}	
				
#if 0	
				chan->if_stats.rx_frame_errors++;
#endif

                		DEBUG_ISR("%s: RX Interrupt pend. \n",
						card->devname);
				xilinx_dma_rx_complete(card,chan,0);
				skip_rx_wtd=1;
			}
		}
isr_skb_rx:

	        /* Transmit DMA Engine */

	        card->hw_iface.bus_read_4(card->hw,
					  XILINX_DMA_TX_INTR_PENDING_REG, 
					  &dma_tx_reg);

		dma_tx_reg&=card->u.xilinx.active_ch_map;

		DEBUG_TEST("%s: DMA_TX_INTR_REG(0x%X) = 0x%X, ChMap=0x%lX NumofCh=%i\n",
				card->devname,
				XILINX_DMA_TX_INTR_PENDING_REG,
				dma_tx_reg,
				card->u.xilinx.active_ch_map,
				num_of_logic_ch);

		if (dma_tx_reg == 0){
                        goto isr_skb_tx;
                }

		for (i=0; i<num_of_logic_ch ;i++){
			if (wan_test_bit(i,&dma_tx_reg) && wan_test_bit(i,&card->u.xilinx.logic_ch_map)){
				chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[i];
				if (!chan){
					DEBUG_ERROR("%s: Error: No Dev for Tx logical ch=%i\n",
							card->devname,i);
					continue;
				}

				if (!wan_test_bit(0,&chan->up)){
                                        DEBUG_ERROR("%s: Error: Dev not up for Tx logical ch=%i\n",
                                                        card->devname,i);
                                        continue;
                                }


             			DEBUG_TEST("---- TX Interrupt pend. --\n");
				xilinx_dma_tx_complete(card,chan,0);

			}else{
				DEBUG_EVENT("Failed Testing for Tx Timeslot %i TxReg=0x%X ChMap=0x%lX\n",i,
					dma_tx_reg,card->u.xilinx.logic_ch_map);
			}
        	}

		if (!wan_test_bit(ERROR_INTR_ENABLE_BIT,&cfg_reg)) { 
			DEBUG_EVENT("%s: Enabling FIFO Interrupt\n",card->devname);
			aft_fifo_intr_ctrl(card, 1);
		}
    	}

isr_skb_tx:

    	DEBUG_ISR("---- ISR SKB TX end.-------------------\n");
	
	if (wan_test_bit(AFT_TE3_TX_WDT_INTR_PND,&reg)){
		WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
		aft_reset_tx_watchdog(card);	
		chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[0];
		if (chan && wan_test_bit(0,&chan->up)){
#if 0
			++chan->if_stats.tx_dropped;
#endif
			xilinx_dma_tx_complete (card,chan,1);	
		}
		DEBUG_TEST("%s: Tx WatchDog Expired!\n",card->devname);
		aft_reset_tx_watchdog(card);	
	}

	if (wan_test_bit(AFT_TE3_RX_WDT_INTR_PND,&reg)){
		WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
		aft_reset_rx_watchdog(card);
		chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[0];
		if (!skip_rx_wtd && chan && wan_test_bit(0,&chan->up)){
#if 0
			chan->if_stats.rx_dropped++;
#endif
			xilinx_dma_rx_complete (card,chan,1);
		}else{
			if (chan){
				DEBUG_TEST("%s: Skipping Rx WTD Flags=%d\n",
					chan->if_name,wan_test_bit(0,&chan->up));
			}
			aft_reset_rx_watchdog(card);
			if (!chan->single_dma_chain){
				aft_enable_rx_watchdog(card,AFT_MAX_WTD_TIMEOUT);
			}
		}
		
		DEBUG_TEST("%s: Rx WatchDog Expired %p!\n",
				card->devname,chan);
	}


	/* -----------------2/6/2003 10:36AM-----------------
	 *    Finish of the interupt handler
	 * --------------------------------------------------*/
isr_end:
    	DEBUG_ISR("---- ISR end.-------------------\n");
    	wan_clear_bit(0,&card->in_isr);
	WAN_IRQ_RETURN(irq_ret);
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

		case WANPIPEMON_READ_CONFIGURATION:
		case WANPIPEMON_READ_CODE_VERSION:
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data_len=0;
			break;
	

		case WANPIPEMON_ENABLE_TRACING:
	
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			wan_udp_pkt->wan_udp_data_len = 0;
			
			if (!wan_test_bit(0,&trace_info->tracing_enabled)){
						
				trace_info->trace_timeout = SYSTEM_TICKS;
					
				wan_trace_purge(trace_info);
					
				if (wan_udp_pkt->wan_udp_data[0] == 0){
					wan_clear_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: TE3 trace enabled!\n",
						card->devname);
				}else if (wan_udp_pkt->wan_udp_data[0] == 1){
					wan_clear_bit(2,&trace_info->tracing_enabled);
					wan_set_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: TE3 trace enabled!\n",
							card->devname);
				}else{
					wan_clear_bit(1,&trace_info->tracing_enabled);
					wan_set_bit(2,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: TE3 trace enabled!\n",
							card->devname);
				}
				wan_set_bit (0,&trace_info->tracing_enabled);

			}else{
				DEBUG_ERROR("%s: Error: TE3 trace already running!\n",
						card->devname);
				wan_udp_pkt->wan_udp_return_code = 2;
			}
					
			break;

		case WANPIPEMON_DISABLE_TRACING:
			
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			
			if(wan_test_bit(0,&trace_info->tracing_enabled)) {
					
				wan_clear_bit(0,&trace_info->tracing_enabled);
				wan_clear_bit(1,&trace_info->tracing_enabled);
				wan_clear_bit(2,&trace_info->tracing_enabled);
				
				wan_trace_purge(trace_info);
				
				DEBUG_UDP("%s: Disabling TE3 trace\n",
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
				DEBUG_ERROR("%s: Error  TE3 trace not enabled\n",
						card->devname);
				/* set return code */
				wan_udp_pkt->wan_udp_return_code = 1;
				break;
			}

			buffer_length = 0;
			wan_udp_pkt->wan_udp_atm_num_frames = 0;	
			wan_udp_pkt->wan_udp_atm_ismoredata = 0;
					
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

		case WANPIPEMON_ROUTER_UP_TIME:
			wan_getcurrenttime( &chan->router_up_time, NULL );
			chan->router_up_time -= chan->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					chan->router_up_time;	
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = 0;
			break;
	
		case WANPIPEMON_READ_OPERATIONAL_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			memcpy(wan_udp_pkt->wan_udp_data,&chan->opstats,sizeof(aft_op_stats_t));
			wan_udp_pkt->wan_udp_data_len=sizeof(aft_op_stats_t);
			break;

		case WANPIPEMON_FLUSH_OPERATIONAL_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			memset(&chan->opstats,0,sizeof(aft_op_stats_t));
			wan_udp_pkt->wan_udp_data_len=0;
			break;
		
		case WANPIPEMON_READ_COMMS_ERROR_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			memcpy(wan_udp_pkt->wan_udp_data,&chan->errstats,sizeof(aft_comm_err_stats_t));
			wan_udp_pkt->wan_udp_data_len=sizeof(aft_comm_err_stats_t);
			break;
		
		case WANPIPEMON_FLUSH_COMMS_ERROR_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			memset(&chan->errstats,0,sizeof(aft_comm_err_stats_t));
			wan_udp_pkt->wan_udp_data_len=0;
			break;
	
		case WAN_GET_PROTOCOL:
		   	wan_udp_pkt->wan_udp_data[0] = card->wandev.config_id;
			wan_udp_pkt->wan_udp_return_code = CMD_OK;
			wan_udp_pkt->wan_udp_data_len = 1;
			break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_PLATFORM_ID;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_MASTER_DEV_NAME:
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
			break;			

#if 0
		case WAN_GET_MEDIA_TYPE:
			if (card->wandev.fe_iface.get_fe_media){
				wan_udp_pkt->wan_udp_data[0] = 
					card->wandev.fe_iface.get_fe_media(&card->fe);
				wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
				wan_udp_pkt->wan_udp_data_len = sizeof(unsigned char); 
			}else{
				wan_udp_pkt->wan_udp_return_code = WAN_UDP_INVALID_CMD;
			}
			break;
#endif
		default:
			if ((wan_udp_pkt->wan_udp_command == WAN_GET_MEDIA_TYPE) ||
			    ((wan_udp_pkt->wan_udp_command & 0xF0) == WAN_FE_UDP_CMD_START)){
				WAN_FECALL(&card->wandev, process_udp,
							(&card->fe, 
							&wan_udp_pkt->wan_udp_cmd,
							&wan_udp_pkt->wan_udp_data[0]));
				break;
			}
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
	
			if (WAN_NET_RATELIMIT()){
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
	struct wan_dev_le	*devle;
	netdevice_t *dev;

        if (card->wandev.state != state)
        {
#if 0
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
#endif
                card->wandev.state = state;
		WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev) continue;
			set_chan_state(card, dev, state);
		}
        }
}


/*============================================================
 * callback_front_end_state
 * 
 * Called by front end code to indicate that state has
 * changed. We will call the poll task to update the state.
 */

static void callback_front_end_state(void *card_id)
{	
	sdla_t		*card = (sdla_t*)card_id;

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		return;
	}

	/* Call the poll task to update the state */
	wan_set_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd);
	WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));

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

static void handle_front_end_state(void *card_id)
{
	sdla_t		*card = (sdla_t*)card_id;
	unsigned char	status;
	
	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}
	
	WAN_FECALL(&card->wandev, get_fe_status, (&card->fe, &status,0));
	if (status == FE_CONNECTED){
		DEBUG_TEST("%s: Connected State %i\n",
				card->devname,card->fe.fe_param.te3.e3_lb_ctrl);
		if (IS_DS3(&card->fe.fe_cfg) ||
		    (card->fe.fe_param.te3.e3_lb_ctrl == 0 || card->fe.fe_param.te3.e3_lb_ctrl == 3)) {
			if (card->wandev.state != WAN_CONNECTED){
				enable_data_error_intr(card);
				port_set_state(card,WAN_CONNECTED);
				card->u.xilinx.state_change_exit_isr=1;
				wan_set_bit(AFT_FE_LED,&card->u.aft.port_task_cmd);
			}
		}
	} else {
		if (card->wandev.state != WAN_DISCONNECTED){
			port_set_state(card,WAN_DISCONNECTED);
			disable_data_error_intr(card,LINK_DOWN);
			card->u.xilinx.state_change_exit_isr=1;
			wan_set_bit(AFT_FE_LED,&card->u.aft.port_task_cmd);
		}
	}
}

#if 0
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
#endif

static int write_cpld(void *pcard, unsigned short off,unsigned char data)
{
	sdla_t	*card = (sdla_t*)pcard;
	u16             org_off;

        off &= ~BIT_DEV_ADDR_CLEAR;
        off |= BIT_DEV_ADDR_CPLD;

        /*ALEX: Save the current original address */
        card->hw_iface.bus_read_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                &org_off);

	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	WP_DELAY(5);

        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                off);
        
	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	WP_DELAY(5);

	card->hw_iface.bus_write_1(card->hw,
                                XILINX_MCPU_INTERFACE,
                                data);
        /*ALEX: Restore the original address */
        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                org_off);
        return 0;
}


static int write_fe_cpld(void *pcard, unsigned short off,unsigned char data)
{
	sdla_t	*card = (sdla_t*)pcard;
	u16             org_off;

        off &= ~AFT3_BIT_DEV_ADDR_EXAR_CLEAR;
        off |= AFT3_BIT_DEV_ADDR_EXAR_CPLD;

        /*ALEX: Save the current original address */
        card->hw_iface.bus_read_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                &org_off);

	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	WP_DELAY(5);

        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                off);
        
	/* This delay is required to avoid bridge optimization 
	 * (combining two writes together)*/
	WP_DELAY(5);

	card->hw_iface.bus_write_1(card->hw,
                                XILINX_MCPU_INTERFACE,
                                data);
        /*ALEX: Restore the original address */
        card->hw_iface.bus_write_2(card->hw,
                                XILINX_MCPU_INTERFACE_ADDR,
                                org_off);
        return 0;
}

static int xilinx_read(sdla_t *card, wan_cmd_api_t *api_cmd)
{

	if (api_cmd->offset <= 0x3C){
		 card->hw_iface.pci_read_config_dword(card->hw,
						api_cmd->offset,
						(u32*)&api_cmd->data[0]); 
		 api_cmd->len=4;

	}else{
		card->hw_iface.peek(card->hw, api_cmd->offset, &api_cmd->data[0], api_cmd->len);
	}

#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Reading Bar%i Offset=0x%X Len=%i\n",
			card->devname,api_cmd->bar,api_cmd->offset,api_cmd->len);
#endif

	return 0;
}

static int xilinx_write(sdla_t *card, wan_cmd_api_t *api_cmd)
{

#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Writting Bar%i Offset=0x%X Len=%i\n",
			card->devname,
			api_cmd->bar,api_cmd->offset,api_cmd->len);
#endif

#if 0
	card->hw_iface.poke(card->hw, api_cmd->offset, &api_cmd->data[0], api_cmd->len);
#endif
	if (api_cmd->len == 1){
		card->hw_iface.bus_write_1(
			card->hw,
			api_cmd->offset,
			(u8)api_cmd->data[0]);
	}else if (api_cmd->len == 2){
		card->hw_iface.bus_write_2(
			card->hw,
			api_cmd->offset,
			*(u16*)&api_cmd->data[0]);
	}else if (api_cmd->len == 4){
		card->hw_iface.bus_write_4(
			card->hw,
			api_cmd->offset,
			*(u32*)&api_cmd->data[0]);
	}else{
		card->hw_iface.poke(
			card->hw, 
			api_cmd->offset, 
			&api_cmd->data[0], 
			api_cmd->len);
	}

	return 0;
}

static int xilinx_write_bios(sdla_t *card, wan_cmd_api_t *api_cmd)
{
#ifdef DEB_XILINX
	DEBUG_EVENT("Setting PCI 0x%X=0x%08lX   0x3C=0x%08X\n",
			(card->wandev.S514_cpu_no[0] == SDLA_CPU_A) ? 0x10 : 0x14,
			card->u.xilinx.bar,card->wandev.irq);
#endif
	card->hw_iface.pci_write_config_dword(card->hw, 
			(card->wandev.S514_cpu_no[0] == SDLA_CPU_A) ? 0x10 : 0x14,
			card->u.xilinx.bar);
	card->hw_iface.pci_write_config_dword(card->hw, 0x3C, card->wandev.irq);
	card->hw_iface.pci_write_config_dword(card->hw, 0x0C, 0x0000ff00);

	return 0;
}

static int aft_devel_ioctl(sdla_t *card,struct ifreq *ifr)
{
	wan_cmd_api_t	api_cmd;	
	int		err;

	if (!ifr || !ifr->ifr_data){
		DEBUG_ERROR("%s: Error: No ifr or ifr_data\n",__FUNCTION__);
		return -EINVAL;
	}

	if (WAN_COPY_FROM_USER(&api_cmd,ifr->ifr_data,sizeof(wan_cmd_api_t))){
		return -EFAULT;
	}

	switch(api_cmd.cmd){
#if defined(__LINUX__)
	case SDLA_HDLC_READ_REG:
#endif
	case SIOC_WAN_READ_REG:
		err=xilinx_read(card, &api_cmd);
		break;

#if defined(__LINUX__)
	case SDLA_HDLC_WRITE_REG:
#endif
	case SIOC_WAN_WRITE_REG:
		err=xilinx_write(card, &api_cmd);
		break;
		
#if defined(__LINUX__)
	case SDLA_HDLC_SET_PCI_BIOS:
#endif
	case SIOC_WAN_SET_PCI_BIOS:
		err=xilinx_write_bios(card, &api_cmd);
		break;
	}

	if (WAN_COPY_TO_USER(ifr->ifr_data,&api_cmd,sizeof(wan_cmd_api_t))){
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
	struct wan_dev_le	*devle;
	u32 reg;
	netdevice_t *dev;

	DEBUG_TEST("%s: %s() !!!\n",
			card->devname,__FUNCTION__);
	
	
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
	
		if (!wan_test_bit(0,&chan->up)){
			continue;
		}

		xilinx_t3_exar_dev_configure_post(card,chan);

		aft_list_descriptors(chan);
	
		DEBUG_TEST("%s: 1) Free Used DMA CHAINS %s\n",
				card->devname,chan->if_name);
	
		aft_rx_dma_chain_handler(chan,0,1);
		aft_free_rx_complete_list(chan);
	
	
		aft_list_descriptors(chan);
	
		DEBUG_TEST("%s: 1) Free UNUSED DMA CHAINS %s\n",
				card->devname,chan->if_name);
	
		aft_free_rx_descriptors(chan);
	
		aft_free_tx_descriptors(chan);
	
	
		DEBUG_TEST("%s: 2) Init interface fifo no wait %s\n",
				card->devname,chan->if_name);
	
				xilinx_init_rx_dev_fifo(card, chan, WP_NO_WAIT);
				xilinx_init_tx_dev_fifo(card, chan, WP_NO_WAIT);
	
		aft_list_descriptors(chan);
	}
	
	if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
		card->hw_iface.bus_read_4(card->hw,TE3_FRACT_ENCAPSULATION_REG,&reg);
	
		DEBUG_EVENT("%s: Rx Fractional Frame Size = 0x%lX\n",
				card->devname,
				get_te3_rx_fract_frame_size(reg));
	
		/* FIXME: Setup bitrate and tx frame size */
	}
	
	
		/* Enable DMA controler, in order to start the
			* fifo cleaning */
	reg=0;
	reg|=(AFT_T3_DMA_FIFO_MARK << DMA_FIFO_T3_MARK_BIT_SHIFT);
	reg|=(MAX_AFT_DMA_CHAINS-1)&DMA_CHAIN_TE3_MASK;
	
		wan_set_bit(DMA_RX_ENGINE_ENABLE_BIT,&reg);
		wan_set_bit(DMA_TX_ENGINE_ENABLE_BIT,&reg);
		card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
	
	/* For all channels clean Tx/Rx fifos */
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		private_area_t *chan;
	
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !wan_netif_priv(dev))
			continue;
				chan = wan_netif_priv(dev);
	
		if (!wan_test_bit(0,&chan->up)){
			continue;
		}
	
		DEBUG_TEST("%s: 3) Init interface fifo %s\n",
				card->devname,chan->if_name);
	
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
	
		if (!wan_test_bit(0,&chan->up)){
			continue;
		}
	
		DEBUG_TEST("%s: 4) Init interface %s\n",
				card->devname,chan->if_name);
	
				xilinx_dma_rx(card,chan,-1);
	
		if (!chan->hdlc_eng) {
			aft_reset_tx_chain_cnt(chan);
			xilinx_dma_te3_tx(card,chan);
		}
	
		aft_list_descriptors(chan);
	
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
	
	/* Enable Fifo interrupt after first successful DMA */
	wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
	
	
	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
	
	aft_enable_rx_watchdog(card,AFT_RX_TIMEOUT);
	aft_enable_tx_watchdog(card,AFT_TX_TIMEOUT);
	
	DEBUG_TEST("%s: END !!! Dma=0x%08X\n",
			__FUNCTION__,reg);

}

static void disable_data_error_intr(sdla_t *card, unsigned char event)
{
	u32 reg;
#if 0
	private_area_t *chan;

		card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
		wan_clear_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
		wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg); 
		wan_clear_bit(FRONT_END_INTR_ENABLE_BIT,&reg);
		wan_clear_bit(ENABLE_TE3_FRACTIONAL,&reg);
		card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

		aft_reset_rx_watchdog(card);
		aft_reset_tx_watchdog(card);

		chan=(private_area_t*)card->u.xilinx.dev_to_ch_map[0];
		if (chan && wan_test_bit(0,&chan->up)){
			xilinx_dev_close(card,chan);
			WAN_NETIF_STOP_QUEUE(chan->common.dev);
			WAN_TASKLET_KILL(&chan->common.bh_task);
		}	
		
		xilinx_t3_exar_chip_unconfigure(card);
		
		WP_DELAY(10500);
		
		xilinx_t3_exar_chip_configure(card);
		if (chan && wan_test_bit(0,&chan->up)){
			WP_DELAY(500);
			xilinx_t3_exar_dev_configure(card,chan);
			WP_DELAY(500);
			xilinx_dev_enable(card, chan);
			WAN_TASKLET_INIT((&chan->common.bh_task),0,wp_bh,(unsigned long)chan);
		}
#endif
	

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
	wan_clear_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg); 
	if (event==DEVICE_DOWN){
		wan_clear_bit(FRONT_END_INTR_ENABLE_BIT,&reg);
		wan_clear_bit(ENABLE_TE3_FRACTIONAL,&reg);
	}
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
	wan_clear_bit(DMA_RX_ENGINE_ENABLE_BIT,&reg);
	wan_clear_bit(DMA_TX_ENGINE_ENABLE_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);

	aft_reset_rx_watchdog(card);
	aft_reset_tx_watchdog(card);

	if (event==DEVICE_DOWN){
		wan_set_bit(CARD_DOWN,&card->wandev.critical);
	}
	

	DEBUG_TEST("%s: Event = %s\n",__FUNCTION__,
			event==DEVICE_DOWN?"Device Down": "Link Down");
	
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
	if (IS_TE3(&card->fe.fe_cfg)) {
		WAN_FECALL(&card->wandev, read_alarm, (&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE));
		/* TE1 Update T1/E1 perfomance counters */
		WAN_FECALL(&card->wandev, read_pmon, (&card->fe, 0));
         }

        return 0;
}

static void xilinx_tx_fifo_under_recover (sdla_t *card, private_area_t *chan)
{

#if 1
	u32 reg;
	/* Enable DMA controler, in order to start the
         * fifo cleaning */
        card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
        wan_clear_bit(DMA_TX_ENGINE_ENABLE_BIT,&reg);
        card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
#endif
	
#if 0
	aft_list_tx_descriptors(chan);
#endif
	aft_free_tx_descriptors(chan);
	
#if 1 
	xilinx_init_tx_dev_fifo(card,chan,WP_NO_WAIT);
	
	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
        wan_set_bit(DMA_TX_ENGINE_ENABLE_BIT,&reg);
        card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);

	xilinx_init_tx_dev_fifo(card,chan,WP_WAIT);
#endif
	wan_clear_bit(0,&chan->idle_start);

	WAN_NETIF_WAKE_QUEUE(chan->common.dev);
#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC
	if (chan->common.usedby == API){
		wan_wakeup_api(chan);
	}else if (chan->common.usedby == STACK){
		wanpipe_lip_kick(chan,0);
	}
#endif
	if (!chan->single_dma_chain){
		aft_enable_tx_watchdog(card,AFT_TX_TIMEOUT);
	}
}

static int xilinx_write_ctrl_hdlc(sdla_t *card, u32 timeslot, u8 reg_off, u32 data)
{
	/* M.F. for Exar T3 card direct access
   	 * without analize current timeslot    */

	card->hw_iface.bus_write_4(card->hw,reg_off,data);

	return 0;
}

static int set_chan_state(sdla_t* card, netdevice_t* dev, int state)
{
       private_area_t *chan = wan_netif_priv(dev);
       if (!chan || !wan_test_bit(0,&chan->up)) {
        	return -ENODEV;
       }

       chan->common.state = state;
       if (state == WAN_CONNECTED){
               wan_clear_bit(0,&chan->idle_start);
	       WAN_NETIF_CARRIER_ON(dev);
	       WAN_NETIF_WAKE_QUEUE(dev);
	       chan->opstats.link_active_count++;
       }else{
	       WAN_NETIF_CARRIER_OFF(dev);
	       WAN_NETIF_STOP_QUEUE(dev);
    	       chan->opstats.link_inactive_modem_count++;
       }
	      
#if defined(__LINUX__)
# if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
       	if (chan->common.usedby == API){
               	wan_update_api_state(chan);
       	}
#endif
#endif

       	if (chan->common.usedby == STACK){
		if (state == WAN_CONNECTED){
			wanpipe_lip_connect(chan,0);
		}else{
			wanpipe_lip_disconnect(chan,0);
		}
	}
       return 0;
}


/**SECTION*************************************************************
 *
 * 	Protocol API Support Functions
 *
 **********************************************************************/


static int protocol_init (sdla_t *card, netdevice_t *dev,
		          private_area_t *chan,
			  wanif_conf_t* conf)
{

	chan->common.protocol = conf->protocol;

	DEBUG_TEST("%s: Protocol init 0x%X PPP=0x0%x FR=0x0%X\n",
			wan_netif_name(dev), chan->common.protocol,
			WANCONFIG_PPP,
			WANCONFIG_FR);

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC

	DEBUG_EVENT("%s: AFT Driver doesn't directly support any protocols!\n",
			chan->if_name);
	return -1;

#else
	if (chan->common.protocol == WANCONFIG_PPP || 
	    chan->common.protocol == WANCONFIG_CHDLC){

		struct ifreq		ifr;
		struct if_settings	ifsettings;
		
		wanpipe_generic_register(card, dev, wan_netif_name(dev));
		chan->common.prot_ptr = dev;

		if (chan->common.protocol == WANCONFIG_CHDLC){
			DEBUG_EVENT("%s: Starting Kernel CISCO HDLC protocol\n",
					chan->if_name);
			ifsettings.type = IF_PROTO_CISCO;
		}else{
			DEBUG_EVENT("%s: Starting Kernel Sync PPP protocol\n",
					chan->if_name);
			ifsettings.type = IF_PROTO_PPP;

		}
		ifr.ifr_data = (caddr_t)&ifsettings;
		if (wp_lite_set_proto(dev, &ifr)){
			wanpipe_generic_unregister(dev);
			return -EINVAL;
		}			
		
	}else if (chan->common.protocol == WANCONFIG_GENERIC){
		chan->common.prot_ptr = dev;
		
	}else{
		DEBUG_EVENT("%s:%s: Unsupported protocol %d\n",
				card->devname,chan->if_name,chan->common.protocol);
		return -EPROTONOSUPPORT;
	}
#endif
	
	return 0;
}


static int protocol_start (sdla_t *card, netdevice_t *dev)
{
	int err=0;
	
	private_area_t *chan=wan_netif_priv(dev);

	if (!chan)
		return 0;

	return err;
}

static int protocol_stop (sdla_t *card, netdevice_t *dev)
{
	private_area_t *chan=wan_netif_priv(dev);
	int err = 0;
	
	if (!chan)
		return 0;

	return err;
}

static int protocol_shutdown (sdla_t *card, netdevice_t *dev)
{
	private_area_t *chan=wan_netif_priv(dev);

	if (!chan)
		return 0;

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC	

	return 0;
#else
	
	if (chan->common.protocol == WANCONFIG_PPP || 
	    chan->common.protocol == WANCONFIG_CHDLC){

		chan->common.prot_ptr = NULL;
		wanpipe_generic_unregister(dev);

	}else if (chan->common.protocol == WANCONFIG_GENERIC){
		DEBUG_EVENT("%s:%s Protocol shutdown... \n",
				card->devname, chan->if_name);
	}
#endif
	return 0;
}

void protocol_recv(sdla_t *card, private_area_t *chan, netskb_t *skb)
{

#ifdef CONFIG_PRODUCT_WANPIPE_GENERIC
	if (chan->common.protocol == WANCONFIG_PPP || 
	    chan->common.protocol == WANCONFIG_CHDLC){
		wanpipe_generic_input(chan->common.dev, skb);
		return 0;
	}
#endif

#if defined(__LINUX__) && defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	if (chan->common.protocol == WANCONFIG_GENERIC){
		skb->protocol = htons(ETH_P_HDLC);
		skb->dev = chan->common.dev;
		wan_skb_reset_mac_header(skb);
		netif_rx(skb);
		return 0;
	}
#endif

#if defined(__LINUX__)
	skb->protocol = htons(ETH_P_IP);
	skb->dev = chan->common.dev;
	wan_skb_reset_mac_header(skb);
	netif_rx(skb);
#else
	DEBUG_EVENT("%s: Action not supported (IP)!\n",
				card->devname);
	wan_skb_free(skb);
#endif

	return;
}


/**SECTION*************************************************************
 *
 * 	T3 Exar Config Code
 *
 **********************************************************************/


/*=========================================================
 * xilinx_t3_exar_chip_configure
 *
 */

static int xilinx_t3_exar_chip_configure(sdla_t *card)
{
	u32 reg,tmp;
	volatile unsigned char cnt=0;
	int err=0;

    	DEBUG_CFG("T3 Exar Chip Configuration. -- \n");

	xilinx_delay(1);

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

	/* Enable the chip/hdlc reset condition */
	reg=0;
	wan_set_bit(CHIP_RESET_BIT,&reg);
	wan_set_bit(FRONT_END_RESET_BIT,&reg);

	/* Hardcode the AFT T3/E3 clock source
	 * to External/Normal */
	if (card->fe.fe_cfg.cfg.te3_cfg.clock == WAN_MASTER_CLK){
		wan_set_bit(AFT_T3_CLOCK_MODE,&reg);
	}else{
		wan_clear_bit(AFT_T3_CLOCK_MODE,&reg);
	}
		
	DEBUG_CFG("--- T3 Exar Chip Reset. -- \n");

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	WP_DELAY(100);

	/* Disable the chip/hdlc reset condition */
	wan_clear_bit(CHIP_RESET_BIT,&reg);
	wan_clear_bit(FRONT_END_RESET_BIT,&reg);

	/* Disable ALL chip interrupts */
	wan_clear_bit(GLOBAL_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg);
	wan_clear_bit(FRONT_END_INTR_ENABLE_BIT,&reg);

	/* Configure for T3 or E3 front end */
	
	if (IS_DS3(&card->fe.fe_cfg)){
		card->u.xilinx.num_of_time_slots=NUM_OF_T1_CHANNELS;
		wan_clear_bit(INTERFACE_TYPE_T3_E3_BIT,&reg);
		if (card->fe.fe_cfg.frame == WAN_FR_DS3_Cbit){
			wan_set_bit(INTERFACE_MODE_DS3_C_BIT,&reg);
		}else{
			wan_clear_bit(INTERFACE_MODE_DS3_C_BIT,&reg);
		}
	}else if (IS_E3(&card->fe.fe_cfg)){
		card->u.xilinx.num_of_time_slots=NUM_OF_E1_CHANNELS;
		wan_set_bit(INTERFACE_TYPE_T3_E3_BIT,&reg);
		if (card->fe.fe_cfg.frame == WAN_FR_E3_G832){
			wan_set_bit(INTERFACE_MODE_E3_G832,&reg);
		}else{
			wan_clear_bit(INTERFACE_MODE_E3_G832,&reg);
		}
	}else{
		DEBUG_ERROR("%s: Error: T3 Exar doesn't support non T3/E3 interface!\n",
				card->devname);
		return -EINVAL;
	}

	/* Hardcode the HDLC Conroller to
	 * HDLC mode. The transparent mode can be
	 * configured later in new_if() section 
	 *  
	 * HDLC		=clear bit
	 * TRANSPARENT	=set bit
	 * */
	wan_clear_bit(AFT_T3_HDLC_TRANS_MODE,&reg);

	
	/* Enable/Disable TX and RX Fractional 
	 * HDLC */

	/* FIXME: HAVE A CONFIG OPTION HERE */

	/* Enable Internal HDLC Encapsulation core */
	if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
		DEBUG_EVENT("%s: BEFORE TE3 FRACT = 0x%X\n",
			card->devname, reg);
	
		wan_set_bit(ENABLE_TE3_FRACTIONAL,&reg);

		//te3_enable_fractional(&reg, TE3_FRACT_VENDOR_KENTROX);
		DEBUG_EVENT("%s: AFTER TE3 FRACT = 0x%X\n",
                	        card->devname, reg);
	}else{
		wan_clear_bit(ENABLE_TE3_FRACTIONAL,&reg);
	}
	
	DEBUG_CFG("--- T3 Exar Chip enable/config. -- \n");

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	WP_DELAY(100);

	xilinx_delay(1);

	DEBUG_EVENT("%s: Config %s Front End: Clock=%s Type=0x%X\n",
			card->devname,
			FE_MEDIA_DECODE(&card->fe),
			(card->fe.fe_cfg.cfg.te3_cfg.clock == WAN_MASTER_CLK)?
			 "Master":"Normal",
			 card->adptr_subtype);

	if (card->wandev.fe_iface.config){
		err = card->wandev.fe_iface.config(&card->fe);
		if (err){
       			DEBUG_EVENT("%s: Failed %s configuratoin!\n",
                               	card->devname,
                             	FE_MEDIA_DECODE(&card->fe));
             		return -EINVAL;
		}

		/* Run rest of initialization not from lock */
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		} else {
			DEBUG_ERROR("%s: Internal Error (%s:%d)\n",
				card->devname,
				__FUNCTION__,__LINE__);
			return -EINVAL;
		}

       	}else{
		DEBUG_ERROR("%s: Internal Error (%s:%d)\n",
				card->devname,
				__FUNCTION__,__LINE__);
		return -EINVAL;
	}

	for (;;){
		card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

		if (!wan_test_bit(HDLC_CORE_READY_FLAG_BIT,&reg)){
			/* The HDLC Core is not ready! we have
			 * an error. */
			if (++cnt > 20){
				err = -EINVAL;
				break;
			}else{
				WP_DELAY(500);
				/* FIXME: we cannot do this while in
                                 * critical area */
			}
		}else{
			err=0;
			break;
		}
	}

	xilinx_delay(1);

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
	reg|=(AFT_T3_DMA_FIFO_MARK << DMA_FIFO_T3_MARK_BIT_SHIFT);
	reg|=(MAX_AFT_DMA_CHAINS-1)&DMA_CHAIN_TE3_MASK;

	/* Enable global DMA engine and set to default
	 * number of active channels. Note: this value will
	 * change in dev configuration */
	wan_set_bit(DMA_RX_ENGINE_ENABLE_BIT,&reg);
	wan_set_bit(DMA_TX_ENGINE_ENABLE_BIT,&reg);

    	DEBUG_CFG("--- Setup DMA control Reg. -- \n");

	card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
	DEBUG_CFG("--- Tx/Rx global enable. -- \n");

	xilinx_delay(1);

	reg=0;
	card->hw_iface.bus_write_4(card->hw,XILINX_TIMESLOT_HDLC_CHAN_REG,reg);

    	/* Clear interrupt pending registers befor first interrupt enable */
	card->hw_iface.bus_read_4(card->hw, XILINX_DMA_RX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw, XILINX_DMA_TX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw, XILINX_HDLC_RX_INTR_PENDING_REG, &tmp);
	card->hw_iface.bus_read_4(card->hw, XILINX_HDLC_TX_INTR_PENDING_REG, &tmp);
	
	card->hw_iface.bus_read_4(card->hw, XILINX_CHIP_CFG_REG, (u32*)&reg);

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

#ifndef AFT_XTEST_DEBUG
	/* Always enable the front end interrupt */
	wan_set_bit(FRONT_END_INTR_ENABLE_BIT,&reg);
#endif
    	DEBUG_CFG("--- Set Global Interrupts (0x%X)-- \n",reg);

	xilinx_delay(1);

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	DEBUG_CFG("--- Set Global Interrupt enabled. -- \n");

	return err;
}

static int xilinx_t3_exar_dev_configure_post(sdla_t *card, private_area_t *chan)
{
	u32 reg=0,reg1=0;

	if (IS_DS3(&card->fe.fe_cfg)){
		return 0;
	}

	DEBUG_TEST("%s: E3 Post Dev Config!\n",chan->if_name);

	if (chan->hdlc_eng){
		/* HDLC engine is enabled on the above logical channels */
		wan_clear_bit(HDLC_RX_PROT_DISABLE_BIT,&reg);
		wan_clear_bit(HDLC_TX_PROT_DISABLE_BIT,&reg);
		
		wan_set_bit(HDLC_TX_CHAN_ENABLE_BIT,&reg);

		if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
			DEBUG_EVENT("%s: Configuring for Fractional\n",card->devname);
			wan_set_bit(HDLC_RX_ADDR_FIELD_DISC_BIT,&reg);
			wan_set_bit(HDLC_TX_ADDR_INSERTION_BIT,&reg);
		}else{
			wan_set_bit(HDLC_RX_ADDR_RECOGN_DIS_BIT,&reg);
		}

		if (card->fe.fe_cfg.cfg.te3_cfg.fcs == 32){
			wan_set_bit(HDLC_TX_FCS_SIZE_BIT,&reg);
			wan_set_bit(HDLC_RX_FCS_SIZE_BIT,&reg);
		}
		
		DEBUG_TEST("%s:%s: Config for HDLC mode: FCS=%d\n",
                        card->devname,chan->if_name,
			card->fe.fe_cfg.cfg.te3_cfg.fcs);
	}else{

		/* Transprent Mode */

		/* Do not start HDLC Core here, because
                 * we have to setup Tx/Rx DMA buffers first
	         * The transparent mode, will start
                 * comms as soon as the HDLC is enabled */

	}

/* Select an HDLC Rx channel for configuration */
	reg1=1;
	card->hw_iface.bus_write_4(card->hw, AFT_T3_RXTX_ADDR_SELECT_REG, reg1);
	
	if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
		
			xilinx_write_ctrl_hdlc(card,
									chan->first_time_slot,
									XILINX_HDLC_ADDR_REG,
									0xA5A5A5A5);
	}
		
	xilinx_write_ctrl_hdlc(card,
							chan->first_time_slot,
							XILINX_HDLC_CONTROL_REG,
							reg);

	return 0;

}

static int xilinx_t3_exar_dev_configure(sdla_t *card, private_area_t *chan)
{

	u32 reg,reg1;

    	DEBUG_TEST("-- T3 Exar Dev Configure Xilinx. --\n");


	chan->logic_ch_num=0;
	chan->first_time_slot=0;
	card->u.xilinx.logic_ch_map=0x01;

	if (!card->u.xilinx.dev_to_ch_map[0]){
		card->u.xilinx.dev_to_ch_map[0]=(void*)chan;
	}

	  /* Setup global DMA parameters */
	if (chan->single_dma_chain){
        	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
        	reg&=~DMA_CHAIN_TE3_MASK;
        	reg|=1&DMA_CHAIN_TE3_MASK;
        	card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
	}

	reg=0;
	
	if (chan->hdlc_eng){
		/* HDLC engine is enabled on the above logical channels */
		wan_clear_bit(HDLC_RX_PROT_DISABLE_BIT,&reg);
		wan_clear_bit(HDLC_TX_PROT_DISABLE_BIT,&reg);
		
		wan_set_bit(HDLC_TX_CHAN_ENABLE_BIT,&reg);

		if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
			DEBUG_EVENT("%s: Configuring for Fractional\n",card->devname);
			wan_set_bit(HDLC_RX_ADDR_FIELD_DISC_BIT,&reg);
			wan_set_bit(HDLC_TX_ADDR_INSERTION_BIT,&reg);
		}else{
			wan_set_bit(HDLC_RX_ADDR_RECOGN_DIS_BIT,&reg);
		}

		if (card->fe.fe_cfg.cfg.te3_cfg.fcs == 32){
			wan_set_bit(HDLC_TX_FCS_SIZE_BIT,&reg);
			wan_set_bit(HDLC_RX_FCS_SIZE_BIT,&reg);
		}
		
		DEBUG_EVENT("%s:%s: Config for HDLC mode: FCS=%d\n",
                        card->devname,chan->if_name,
			card->fe.fe_cfg.cfg.te3_cfg.fcs);
	}else{

		/* Transprent Mode */

		/* Do not start HDLC Core here, because
                 * we have to setup Tx/Rx DMA buffers first
	         * The transparent mode, will start
                 * comms as soon as the HDLC is enabled */

	}

	/* Select an HDLC Tx channel for configuration */
	reg1=0;
	card->hw_iface.bus_write_4(card->hw, AFT_T3_RXTX_ADDR_SELECT_REG, reg1);

	if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
        	xilinx_write_ctrl_hdlc(card,
                               chan->first_time_slot,
                               XILINX_HDLC_ADDR_REG,
                               0xA5A5A5A5);
	}

	xilinx_write_ctrl_hdlc(card,
                               chan->first_time_slot,
                               XILINX_HDLC_CONTROL_REG,
                               reg);


	if (IS_DS3(&card->fe.fe_cfg)){
	
		/* Select an HDLC Rx channel for configuration */
		reg1=1;
		card->hw_iface.bus_write_4(card->hw, AFT_T3_RXTX_ADDR_SELECT_REG, reg1);

		if (card->fe.fe_cfg.cfg.te3_cfg.fractional){
	
			xilinx_write_ctrl_hdlc(card,
								chan->first_time_slot,
								XILINX_HDLC_ADDR_REG,
								0xA5A5A5A5);
		}
	
		xilinx_write_ctrl_hdlc(card,
								chan->first_time_slot,
								XILINX_HDLC_CONTROL_REG,
								reg);
	}
	return 0;

}

static void xilinx_t3_exar_dev_unconfigure(sdla_t *card, private_area_t *chan)
{
	/* Nothing to do for T3 Exar */
	wan_smp_flag_t flags;	

	wan_spin_lock_irq(&card->wandev.lock,&flags);
	card->u.xilinx.dev_to_ch_map[0]=NULL;
	aft_reset_rx_watchdog(card);
	aft_reset_tx_watchdog(card);
	wan_spin_unlock_irq(&card->wandev.lock,&flags);

}

static void xilinx_t3_exar_chip_unconfigure(sdla_t *card)
{

	u32 reg=0;

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

	/* Enable the chip/hdlc reset condition */
	wan_set_bit(CHIP_RESET_BIT,&reg);
	wan_set_bit(FRONT_END_RESET_BIT,&reg);

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

}


static void xilinx_t3_exar_transparent_config(sdla_t *card,private_area_t *chan)
{
	u32 reg;

	DEBUG_EVENT("%s: Configuring for Transparent mode!\n",
		card->devname);

	/* T3/E3 Exar Card */
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);
	/* Enable hdlc transparent mode */
	wan_set_bit(AFT_T3_HDLC_TRANS_MODE,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
}



/**SECTION*************************************************************
 *
 * 	TE3 Exar Tx Functions
 * 	DMA Chains
 *
 **********************************************************************/


/*===============================================
 * aft_tx_dma_chain_handler
 *
 */
static void aft_tx_dma_chain_handler(unsigned long data)
{
	private_area_t *chan = (private_area_t *)data;
	sdla_t *card = chan->card;
	int dma_free=0;
	u32 reg,dma_descr;
	aft_dma_chain_t *dma_chain;

	if (wan_test_and_set_bit(TX_HANDLER_BUSY,&chan->dma_status)){
		DEBUG_EVENT("%s: SMP Critical in %s\n",
				chan->if_name,__FUNCTION__);
		return;
	}

	dma_chain = &chan->tx_dma_chain_table[chan->tx_pending_chain_indx];
	
	for (;;){

		/* If the current DMA chain is in use,then
		 * all chains are busy */
		if (!wan_test_bit(0,&dma_chain->init)){
			break;
		}

		dma_descr=(chan->tx_pending_chain_indx<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

		/* If GO bit is set, then the current DMA chain
		 * is in process of being transmitted, thus
		 * all are busy */
		if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
			break;
		}
	
		DEBUG_TEST("%s: TX DMA Handler Chain %i\n",chan->if_name,dma_chain->index);

		if (dma_chain->skb){

			if (dma_chain->skb == chan->tx_idle_skb) {
				chan->if_stats.tx_carrier_errors++;
			} else {
				wan_skb_set_csum(dma_chain->skb, reg);
				wan_skb_queue_tail(&chan->wp_tx_complete_list,dma_chain->skb);
			}
			dma_chain->skb=NULL;

		}

		aft_tx_dma_chain_init(chan,dma_chain);

		if (chan->single_dma_chain){
			break;
		}

		if (++chan->tx_pending_chain_indx >= MAX_AFT_DMA_CHAINS){
			chan->tx_pending_chain_indx=0;
		}

		dma_chain = &chan->tx_dma_chain_table[chan->tx_pending_chain_indx];
		dma_free=1;
	}

	wan_clear_bit(TX_HANDLER_BUSY,&chan->dma_status);

	if (wan_skb_queue_len(&chan->wp_tx_complete_list)){
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));	
	}

	return;	
}

/*===============================================
 * aft_dma_chain_tx
 *
 */
static int aft_dma_chain_tx(aft_dma_chain_t *dma_chain,private_area_t *chan, int intr)
{

#define dma_descr   dma_chain->dma_descr
#define reg	    dma_chain->reg
#define len	    dma_chain->dma_len
#define dma_ch_indx dma_chain->index	
#define len_align   dma_chain->len_align	
#define card	    chan->card

	dma_descr=(dma_ch_indx<<4) + XILINX_TxDMA_DESCRIPTOR_HI;

	DEBUG_TX("%s:%d: chan logic ch=%i chain=%li dma_descr=0x%x set!\n",
                    __FUNCTION__,__LINE__,chan->logic_ch_num,dma_ch_indx,dma_descr);

	card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

	if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
		DEBUG_ERROR("%s: Error: TxDMA GO Ready bit set on dma (chain=%i) Tx 0x%X\n",
				card->devname,dma_ch_indx,reg);
		card->hw_iface.bus_write_4(card->hw,dma_descr,0);
		return -EBUSY;
	}
	
	dma_descr=(dma_ch_indx<<4) + XILINX_TxDMA_DESCRIPTOR_LO;

	/* Write the pointer of the data packet to the
	 * DMA address register */
	reg=dma_chain->dma_addr;

	/* Set the 32bit alignment of the data length.
	 * Used to pad the tx packet to the 32 bit
	 * boundary */
	reg&=~(TxDMA_LO_ALIGNMENT_BIT_MASK);
	reg|=(len&0x03);

	len_align=0;
	if (len&0x03){
		len_align=1;
	}

	DEBUG_TX("%s: TXDMA_LO=0x%X PhyAddr=0x%X DmaDescr=0x%X\n",
			__FUNCTION__,reg,dma_chain->dma_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	dma_descr=(dma_ch_indx<<4) + XILINX_TxDMA_DESCRIPTOR_HI;

	reg=0;

	reg|=(((len>>2)+len_align)&TxDMA_HI_DMA_DATA_LENGTH_MASK);

        reg|=(chan->fifo_size_code&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;

        reg|=(chan->fifo_base_addr&DMA_FIFO_BASE_ADDR_MASK)<<
                              DMA_FIFO_BASE_ADDR_SHIFT;


	if (chan->single_dma_chain){
		wan_clear_bit(DMA_HI_TE3_NOT_LAST_FRAME_BIT,&reg);
		wan_clear_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);
	}else{

		wan_set_bit(DMA_HI_TE3_NOT_LAST_FRAME_BIT,&reg);

		if (intr){
			DEBUG_TEST("%s: Setting Interrupt on index=%i\n",
					chan->if_name,dma_ch_indx);
			wan_clear_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);
		}else{
			wan_set_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);
		}
	}
	
	if (chan->hdlc_eng){
		/* Only enable the Frame Start/Stop on
                 * non-transparent hdlc configuration */

		wan_set_bit(TxDMA_HI_DMA_FRAME_START_BIT,&reg);
		wan_set_bit(TxDMA_HI_DMA_FRAME_END_BIT,&reg);
	}

	wan_set_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg);

	DEBUG_TEST("%s: TXDMA_HI=0x%X DmaDescr=0x%lX len=%i\n",
			__FUNCTION__,reg,dma_chain->dma_addr,len);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

#if 0
	++chan->if_stats.tx_fifo_errors;
#endif

	return 0;

#undef dma_descr  
#undef reg	 
#undef len	 
#undef dma_ch_indx
#undef len_align 
#undef card
}

/*===============================================
 * aft_dma_chain_init
 *
 */
static void aft_tx_dma_chain_init(private_area_t *chan, aft_dma_chain_t *dma_chain)
{

	if (dma_chain->dma_addr){
		chan->card->hw_iface.pci_unmap_dma(chan->card->hw,
			 dma_chain->dma_addr,
	 		 dma_chain->dma_len,
	 		 PCI_DMA_TODEVICE);
	}
	

	if (dma_chain->skb){
		if (dma_chain->skb != chan->tx_idle_skb) {
			wan_skb_free(dma_chain->skb);
		}
		dma_chain->skb=NULL;
	}
	
	dma_chain->dma_addr=0;
	dma_chain->dma_len=0;
			
	wan_clear_bit(0,&dma_chain->init);
}

static void aft_rx_dma_chain_init(private_area_t *chan, aft_dma_chain_t *dma_chain)
{

	if (dma_chain->dma_addr){
		chan->card->hw_iface.pci_unmap_dma(chan->card->hw,
			 dma_chain->dma_addr,
	 		 dma_chain->dma_len,
	 		 PCI_DMA_FROMDEVICE);
	}
	

	if (dma_chain->skb){
		aft_init_requeue_free_skb(chan,dma_chain->skb);
		dma_chain->skb=NULL;
	}
	
	dma_chain->dma_addr=0;
	dma_chain->dma_len=0;
			
	wan_clear_bit(0,&dma_chain->init);
}

static int aft_realign_skb_pkt(private_area_t *chan, netskb_t *skb)
{
	unsigned char *data=wan_skb_data(skb);
	int len = wan_skb_len(skb);
	
	if (len > chan->dma_mtu){
		DEBUG_ERROR("%s: Critical error: Tx unalign pkt(%d) > MTU buf(%d)!\n",
				chan->if_name,len,chan->dma_mtu);
		return -ENOMEM;
	}

	if (!chan->tx_realign_buf){
		chan->tx_realign_buf=wan_malloc(chan->dma_mtu);
		if (!chan->tx_realign_buf){
			DEBUG_ERROR("%s: Error: Failed to allocate tx memory buf\n",
						chan->if_name);
			return -ENOMEM;
		}else{
			DEBUG_EVENT("%s: AFT Realign buffer allocated Len=%d\n",
						chan->if_name,chan->dma_mtu);

		}
	}

	memcpy(chan->tx_realign_buf,data,len);

	wan_skb_init(skb,0);
	wan_skb_trim(skb,0);

	if (wan_skb_tailroom(skb) < len){
		DEBUG_ERROR("%s: Critical error: Tx unalign pkt tail room(%i) < unalign len(%i)!\n",
				chan->if_name,wan_skb_tailroom(skb),len);
		
		return -ENOMEM;
	}

	data=wan_skb_put(skb,len);

	if ((unsigned long)data & 0x03){
		/* At this point pkt should be realigned. If not
		 * there is something really wrong! */
		return -EINVAL;
	}
	
	memcpy(data,chan->tx_realign_buf,len);

	return 0;	
}



/*===============================================
 * xilinx_dma_te3_tx
 *
 */

static int xilinx_dma_te3_tx (sdla_t *card,private_area_t *chan)
{
	int err=0, intr=0, cnt;
	aft_dma_chain_t *dma_chain;
	netskb_t *skb=NULL;

	if (wan_test_and_set_bit(TX_DMA_BUSY,&chan->dma_status)){
		DEBUG_EVENT("%s: SMP Critical in %s\n",
				chan->if_name,__FUNCTION__);
		
		return -EBUSY;
	}

	if (chan->tx_chain_indx >= MAX_AFT_DMA_CHAINS){
		DEBUG_ERROR("%s: MAJOR ERROR: TE3 Tx: Dma tx chain = %i\n",
				chan->if_name,chan->tx_chain_indx);
		if (!chan->single_dma_chain){
			aft_enable_tx_watchdog(card,AFT_TX_TIMEOUT);
		}
		wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);
		return -EBUSY;
	}

	aft_reset_tx_watchdog(card);

	for (cnt=0;cnt<MAX_AFT_DMA_CHAINS;cnt++) {

		dma_chain = &chan->tx_dma_chain_table[chan->tx_chain_indx];	
	
		/* If the current DMA chain is in use,then
		* all chains are busy */
		if (wan_test_and_set_bit(0,&dma_chain->init)){
			wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);
			return -EBUSY;
		}
	
		skb=wan_skb_dequeue(&chan->wp_tx_pending_list);
		if (!skb) {
			if (!chan->hdlc_eng) {
				skb=chan->tx_idle_skb;
				if (!skb) {
					if (WAN_NET_RATELIMIT()){
						DEBUG_ERROR("%s: Error: Tx Idle not allocated\n",
							card->devname);
					}
					wan_clear_bit(0,&dma_chain->init);
					wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);
					return 0;
				}
			} else {
				wan_clear_bit(0,&dma_chain->init);
				wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);
				return 0;
			}
		}
	
		
		
		if ((unsigned long)wan_skb_data(skb) & 0x03){
			err=aft_realign_skb_pkt(chan,skb);
			if (err){
				if (WAN_NET_RATELIMIT()){
				DEBUG_ERROR("%s: Tx Error: Non Aligned packet %p: dropping...\n",
						chan->if_name,wan_skb_data(skb));
				}
				wan_skb_free(skb);
				wan_clear_bit(0,&dma_chain->init);
				chan->if_stats.tx_errors++;
				wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);
				return -EINVAL;
			}
		}
	
	#if 0
		if (0){
			netskb_t *nskb=__dev_alloc_skb(wan_skb_len(skb),GFP_DMA|GFP_ATOMIC);
			if (!nskb) {
				wan_skb_free(skb);
							wan_clear_bit(0,&dma_chain->init);
							chan->if_stats.tx_errors++;
							return -EINVAL;
			} else {
				unsigned char *buf = wan_skb_put(nskb,wan_skb_len(skb));
				memcpy(buf,wan_skb_data(skb),wan_skb_len(skb));
				wan_skb_free(skb);
				skb=nskb;
			}
		}
	#endif
	
	
		dma_chain->skb=skb;
			
		dma_chain->dma_addr = 
					card->hw_iface.pci_map_dma(card->hw,
							wan_skb_data(dma_chain->skb),
							wan_skb_len(dma_chain->skb),
							PCI_DMA_TODEVICE);
			
		dma_chain->dma_len = wan_skb_len(dma_chain->skb);
	
		DEBUG_TX("%s: DMA Chain %i:  Cur=%i Pend=%i Len=%i\n",
				chan->if_name,dma_chain->index,
				chan->tx_chain_indx,chan->tx_pending_chain_indx,
				wan_skb_len(dma_chain->skb));
	
	
		intr=0;
		if (!chan->single_dma_chain && 
				!wan_test_bit(TX_INTR_PENDING,&chan->dma_chain_status)){
			int pending_indx=chan->tx_pending_chain_indx;
			if (chan->tx_chain_indx >= pending_indx){
				intr = ((MAX_AFT_DMA_CHAINS-(chan->tx_chain_indx - 
								pending_indx))<=2);
			}else{
				intr = ((pending_indx - chan->tx_chain_indx)<=2);
			}
	
			if (intr){
				DEBUG_TEST("%s: Setting tx interrupt on chain=%i\n",
						chan->if_name,dma_chain->index);
				wan_set_bit(TX_INTR_PENDING,&chan->dma_chain_status);
			}
		}
			
		err=aft_dma_chain_tx(dma_chain,chan,intr);
		if (err){
			DEBUG_ERROR("%s: Tx dma chain %i overrun error: should never happen!\n",
					chan->if_name,dma_chain->index);
			aft_tx_dma_chain_init(chan,dma_chain);
			chan->if_stats.tx_errors++;
			
			wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);
			return -EINVAL;
		}
	
	
		/* We are sure that the packet will
		* be bound and transmitted, thus unhook
		* it from the protocol stack */
		wan_skb_unlink(skb);
	
		if (!chan->single_dma_chain){
			if (++chan->tx_chain_indx >= MAX_AFT_DMA_CHAINS){
				chan->tx_chain_indx=0;
			}
	
			if (!wan_test_bit(TX_INTR_PENDING,&chan->dma_chain_status)){
				aft_enable_tx_watchdog(card,AFT_TX_TIMEOUT);
			}
		} else {
			/* Single DMA Mode */
			break;
		}
	}
	
	wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);

	return 0;
}



/**SECTION*************************************************************
 *
 * 	TE3 Exar Rx Functions
 * 	DMA Chains
 *
 **********************************************************************/

static int aft_dma_chain_rx(aft_dma_chain_t *dma_chain, private_area_t *chan, int intr)
{
#define dma_descr   dma_chain->dma_descr
#define reg	    dma_chain->reg
#define len	    dma_chain->dma_len
#define dma_ch_indx dma_chain->index	
#define len_align   dma_chain->len_align	
#define card	    chan->card

	/* Write the pointer of the data packet to the
	 * DMA address register */
	reg=dma_chain->dma_addr;

	/* Set the 32bit alignment of the data length.
	 * Since we are setting up for rx, set this value
	 * to Zero */
	reg&=~(RxDMA_LO_ALIGNMENT_BIT_MASK);

    	dma_descr=(dma_ch_indx<<4) + XILINX_RxDMA_DESCRIPTOR_LO;

//	DEBUG_RX("%s: RxDMA_LO = 0x%X, BusAddr=0x%X DmaDescr=0x%X\n",
//		__FUNCTION__,reg,bus_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	dma_descr=(u32)(dma_ch_indx<<4) + XILINX_RxDMA_DESCRIPTOR_HI;

    	reg =0;

	if (chan->single_dma_chain){
		wan_clear_bit(DMA_HI_TE3_NOT_LAST_FRAME_BIT,&reg);
		wan_clear_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);
	}else{

#ifdef AFT_TE3_IFT_FEATURE_DISABLE
#warning "AFT_TE3_IFT_FEATURE_DISABLE is defined!"
#else
		if (card->u.aft.firm_ver >= AFT_IFT_FIMR_VER) {
			wan_set_bit(DMA_HI_TE3_IFT_INTR_ENB_BIT,&reg);
		}
#endif
		wan_set_bit(DMA_HI_TE3_NOT_LAST_FRAME_BIT,&reg);

		if (intr){
			DEBUG_TEST("%s: Setting Rx Interrupt on index=%i\n",
					chan->if_name,dma_ch_indx);
			wan_clear_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);
		}else{
			wan_set_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);
		}

	}	

	if (chan->hdlc_eng){
		reg|=((chan->dma_mtu>>2)-1)&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	}else{
		reg|=(card->wandev.mtu>>2)&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	}

	reg|=(chan->fifo_size_code&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
        reg|=(chan->fifo_base_addr&DMA_FIFO_BASE_ADDR_MASK)<<
                              DMA_FIFO_BASE_ADDR_SHIFT;	

	wan_set_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg);

	DEBUG_TEST("%s: RXDMA_HI = 0x%X, BusAddr=0x%X DmaDescr=0x%X\n",
 	             __FUNCTION__,reg,dma_chain->dma_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	return 0;

#undef dma_descr  
#undef reg	 
#undef len	 
#undef dma_ch_indx
#undef len_align 
#undef card
}

static int xilinx_dma_rx(sdla_t *card, private_area_t *chan, int gcur_ptr)
{
	int err=0, intr=0;
	aft_dma_chain_t *dma_chain;
	int cur_dma_ptr, i,max_dma_cnt,free_queue_len;
	u32 reg;

	if (wan_test_and_set_bit(RX_DMA_BUSY,&chan->dma_status)){
		DEBUG_EVENT("%s: SMP Critical in %s\n",
				chan->if_name,__FUNCTION__);
		return -EBUSY;
	}

	aft_reset_rx_watchdog(card);

	if (chan->single_dma_chain) {
		max_dma_cnt=MAX_AFT_DMA_CHAINS;
		cur_dma_ptr=0;
		free_queue_len=MAX_AFT_DMA_CHAINS;
	}else{

		free_queue_len=wan_skb_queue_len(&chan->wp_rx_free_list);
		if (free_queue_len == 0){
			aft_free_rx_complete_list(chan);
			free_queue_len=wan_skb_queue_len(&chan->wp_rx_free_list);
			if (free_queue_len == 0){
				DEBUG_ERROR("%s: %s() CRITICAL ERROR: No free rx buffers\n",
						card->devname,__FUNCTION__);
				goto te3_rx_skip;
			}
		}

		card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
	       	cur_dma_ptr=get_current_rx_dma_ptr(reg);   

		if (chan->rx_chain_indx >= cur_dma_ptr){
			max_dma_cnt = MAX_AFT_DMA_CHAINS - (chan->rx_chain_indx-cur_dma_ptr);
		}else{
			max_dma_cnt = cur_dma_ptr - chan->rx_chain_indx;
		}      
		
		if (free_queue_len < max_dma_cnt){
	#if 0
			if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: Free List(%d) lower than max dma %d\n",
				card->devname,
				free_queue_len,
				max_dma_cnt);
			}
	#endif
			max_dma_cnt = free_queue_len;
		}

	}

	DEBUG_TEST("%s: DMA RX: CBoardPtr=%i  Driver=%i MaxDMA=%i\n",
			card->devname,cur_dma_ptr,chan->rx_chain_indx,max_dma_cnt);
	
	for (i=0;i<max_dma_cnt;i++){

		dma_chain = &chan->rx_dma_chain_table[chan->rx_chain_indx];

		/* If the current DMA chain is in use,then
		 * all chains are busy */
		if (wan_test_and_set_bit(0,&dma_chain->init)){
			DEBUG_TEST("%s: Warning: %s():%d dma chain busy %i!\n",
					card->devname, __FUNCTION__, __LINE__,
					dma_chain->index);
		
			err=-EBUSY;
			break;
		}
		
		dma_chain->skb=wan_skb_dequeue(&chan->wp_rx_free_list);
		if (!dma_chain->skb){
			DEBUG_TEST("%s: Warning Rx chain = %i: no free rx bufs (RxComp=%i RxFree=%i)\n",
					chan->if_name,dma_chain->index,
					wan_skb_queue_len(&chan->wp_rx_complete_list),
					wan_skb_queue_len(&chan->wp_rx_free_list));
			wan_clear_bit(0,&dma_chain->init);
			err=-EINVAL;
			break;
		}
		
		dma_chain->dma_addr = 
				      card->hw_iface.pci_map_dma(card->hw,
			      	       		wan_skb_tail(dma_chain->skb),
						chan->dma_mtu,
		    		       		PCI_DMA_FROMDEVICE); 	
		
		dma_chain->dma_len  = chan->dma_mtu;

		intr=0;
		if (!wan_test_bit(RX_INTR_PENDING,&chan->dma_chain_status)){
			
			free_queue_len--;

			if (free_queue_len <= 2){
				DEBUG_TEST("%s: DBG: Setting intr queue size low\n",
					card->devname);
				intr=1;
			}else{
				if (chan->rx_chain_indx >= cur_dma_ptr){
					intr = ((MAX_AFT_DMA_CHAINS - 
						(chan->rx_chain_indx-cur_dma_ptr)) <=4);
				}else{
					intr = ((cur_dma_ptr - chan->rx_chain_indx)<=4);
				}
			}

			if (intr){
				DEBUG_TEST("%s: Setting Rx interrupt on chain=%i\n",
					chan->if_name,dma_chain->index);
				wan_set_bit(RX_INTR_PENDING,&chan->dma_chain_status);
			}
		}

		DEBUG_TEST("%s: Setting Buffer on Rx Chain = %i Intr=%i\n",
					chan->if_name,dma_chain->index, intr);

		err=aft_dma_chain_rx(dma_chain,chan,intr);
		if (err){
			DEBUG_ERROR("%s: Rx dma chain %i overrun error: should never happen!\n",
					chan->if_name,dma_chain->index);
			aft_rx_dma_chain_init(chan,dma_chain);
			chan->if_stats.rx_dropped++;
			break;
		}

		if (chan->single_dma_chain){
			break;
		}

		if (++chan->rx_chain_indx >= MAX_AFT_DMA_CHAINS){
			chan->rx_chain_indx=0;
		}
	}

te3_rx_skip:

	if (!chan->single_dma_chain){
		aft_enable_rx_watchdog(card,AFT_RX_TIMEOUT);
	}
	
	wan_clear_bit(RX_DMA_BUSY,&chan->dma_status);
	
	return err;
}

/*===============================================
 * aft_rx_dma_chain_handler
 *
 */
//N1
static void aft_rx_dma_chain_handler(private_area_t *chan, int wtd, int reset)
{
	sdla_t *card = chan->card;
	u32 reg,dma_descr;
	wp_rx_element_t *rx_el;
	aft_dma_chain_t *dma_chain;
	int i,max_dma_cnt, cur_dma_ptr;

	if (wan_test_and_set_bit(RX_HANDLER_BUSY,&chan->dma_status)){
		DEBUG_EVENT("%s: SMP Critical in %s\n",
				chan->if_name,__FUNCTION__);
		return;
	}

	if (!wtd){
		/* Not watchdog, thus called from an interrupt.
		 * Clear the RX INTR Pending flag */
		wan_clear_bit(RX_INTR_PENDING,&chan->dma_chain_status);	
	}

	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
	cur_dma_ptr=get_current_rx_dma_ptr(reg);

	max_dma_cnt = MAX_AFT_DMA_CHAINS;

	DEBUG_TEST("%s: DMA RX %s: CBoardPtr=%i  Driver=%i MaxDMA=%i\n",
			card->devname,
			wtd?"WTD":"Intr",
			cur_dma_ptr,
			chan->rx_chain_indx,max_dma_cnt);

	for (i=0;i<max_dma_cnt;i++){

		dma_chain = &chan->rx_dma_chain_table[chan->rx_pending_chain_indx];
		if (!dma_chain){
			DEBUG_ERROR("%s:%d Assertion Error !!!!\n",
				__FUNCTION__,__LINE__);
			break;
		}

		/* If the current DMA chain is in use,then
		 * all chains are busy */
		if (!wan_test_bit(0,&dma_chain->init)){
			DEBUG_TEST("%s: Warning (%s) Pending chain %i empty!\n",
				chan->if_name,__FUNCTION__,dma_chain->index);

			break;
		}

		dma_descr=(dma_chain->index<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

		/* If GO bit is set, then the current DMA chain
		 * is in process of being transmitted, thus
		 * all are busy */
		if (wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg)){
			DEBUG_TEST("%s: Warning (%s) Pending chain %i Go bit still set!\n",
				chan->if_name,__FUNCTION__,dma_chain->index);
			break;
		}

		card->hw_iface.pci_unmap_dma(card->hw,
				 dma_chain->dma_addr,
				 chan->dma_mtu,
				 PCI_DMA_FROMDEVICE);

		rx_el=(wp_rx_element_t *)wan_skb_push(dma_chain->skb, sizeof(wp_rx_element_t));
		memset(rx_el,0,sizeof(wp_rx_element_t));
	
#if 0
		chan->if_stats.rx_frame_errors++;
#endif

    		/* Reading Rx DMA descriptor information */
		dma_descr=(dma_chain->index<<4) + XILINX_RxDMA_DESCRIPTOR_LO;
		card->hw_iface.bus_read_4(card->hw,dma_descr, &rx_el->align);
		rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;

    		dma_descr=(dma_chain->index<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_read_4(card->hw,dma_descr, &rx_el->reg);

		if (aft_check_pci_errors(card,chan,rx_el) != 0) {
                	chan->errstats.Rx_pci_errors++;
                	chan->if_stats.rx_errors++;
			wan_skb_pull(dma_chain->skb, sizeof(wp_rx_element_t));
			aft_init_requeue_free_skb(chan, dma_chain->skb);
			dma_chain->skb=NULL;
		} else {
			rx_el->pkt_error= dma_chain->pkt_error;
			rx_el->dma_addr = dma_chain->dma_addr;

			wan_skb_queue_tail(&chan->wp_rx_complete_list,dma_chain->skb);

			DEBUG_RX("%s: RxInr Pending chain %i Rxlist=%i LO:0x%X HI:0x%X Data=0x%X Len=%i!\n",
					chan->if_name,dma_chain->index,
					wan_skb_queue_len(&chan->wp_rx_complete_list),
					rx_el->align,rx_el->reg,
					(*(unsigned char*)wan_skb_data(dma_chain->skb)),
					wan_skb_len(dma_chain->skb));
		}

		dma_chain->skb=NULL;
		dma_chain->dma_addr=0;
		dma_chain->dma_len=0;
		dma_chain->pkt_error=0;
		wan_clear_bit(0,&dma_chain->init);


		if (chan->single_dma_chain){
			break;
		}

		if (++chan->rx_pending_chain_indx >= MAX_AFT_DMA_CHAINS){
			chan->rx_pending_chain_indx=0;
		}

	}

	if (reset){
		goto reset_skip_rx_setup;
	}

	/* Major Bug fix: Only reload dma desc on real interrupt
           not on every watchdog. This caused pci errors when sending
           mixed voip and data traffic */	
	if (!wtd){
		xilinx_dma_rx(card,chan,cur_dma_ptr);
	}
	
	if (wan_skb_queue_len(&chan->wp_rx_complete_list)){
		DEBUG_TEST("%s: Rx Queued list triggering\n",chan->if_name);
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
		chan->rx_no_data_cnt=0;
	}
	
	if (!chan->single_dma_chain){
		if ((chan->rx_no_data_cnt >= 0)  && (++chan->rx_no_data_cnt < 3)){
			aft_enable_rx_watchdog(card,AFT_RX_TIMEOUT);
		}else{
			/* Enable Rx Interrupt on pending rx descriptor */
			DEBUG_TEST("%s: Setting Max Rx Watchdog Timeout\n",
					chan->if_name);
			aft_enable_rx_watchdog(card,AFT_MAX_WTD_TIMEOUT);
			chan->rx_no_data_cnt=-1;
		}
	}

reset_skip_rx_setup:

	wan_clear_bit(RX_HANDLER_BUSY,&chan->dma_status);


	return;	
}

static void aft_index_tx_rx_dma_chains(private_area_t *chan)
{
	int i;

	for (i=0;i<MAX_AFT_DMA_CHAINS;i++){
		chan->tx_dma_chain_table[i].index=i;
		chan->rx_dma_chain_table[i].index=i;
	}
}


static void aft_init_tx_rx_dma_descr(private_area_t *chan)
{
	int i;
	u32 reg=0;
	sdla_t *card=chan->card;
	unsigned int tx_dma_descr,rx_dma_descr;
	unsigned int dma_cnt=MAX_AFT_DMA_CHAINS;

	if (chan->single_dma_chain){
		dma_cnt=1;
	}


	for (i=0;i<dma_cnt;i++){

		tx_dma_descr=(u32)(i<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
		rx_dma_descr=(u32)(i<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
        	card->hw_iface.bus_write_4(card->hw,tx_dma_descr,reg);
		card->hw_iface.bus_write_4(card->hw,rx_dma_descr,reg);

		aft_tx_dma_chain_init(chan,&chan->tx_dma_chain_table[i]);
		aft_tx_dma_chain_init(chan,&chan->rx_dma_chain_table[i]);
	}
}

#if 0
static void aft_dma_te3_set_intr(aft_dma_chain_t *dma_chain, private_area_t *chan)
{
#define dma_ch_indx dma_chain->index	
#define card	    chan->card

	u32 reg=0;
	u32 len;
	u32 dma_descr;
	
	/* If the current DMA chain is in use,then
	 * all chains are busy */
	if (!wan_test_bit(0,&dma_chain->init)){
		DEBUG_ERROR("%s: (%s) Error: Pending chain %i empty!\n",
			chan->if_name,__FUNCTION__,dma_chain->index);
		return;
	}

	dma_descr=(unsigned long)(dma_ch_indx<<4) + XILINX_RxDMA_DESCRIPTOR_HI;

	card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

	/* If GO bit is set, then the current DMA chain
	 * is in process of being Received, thus
	 * all are busy */
	if (!wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg)){
		DEBUG_ERROR("%s: (%s) Error: Pending chain %i Go bit is not set!\n",
			chan->if_name,__FUNCTION__,dma_chain->index);
		return;
	}

	len=reg&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	
	DEBUG_TEST("%s: Set Rx Intr on 1st Pending Chain: index=%i Len=%i (dmalen=%i)\n",
				chan->if_name,dma_ch_indx,len,dma_chain->dma_len);

	wan_clear_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

#undef dma_ch_indx
#undef card
}
#endif

/*
 * ******************************************************************
 * Proc FS function 
 */
static int wan_aft3_get_info(void* pcard, struct seq_file *m, int *stop_cnt)
{
	sdla_t	*card = (sdla_t*)pcard;

	m->count = 
		WAN_FECALL(&card->wandev, update_alarm_info, (&card->fe, m, stop_cnt)); 
	m->count = 
		WAN_FECALL(&card->wandev, update_pmon_info, (&card->fe, m, stop_cnt)); 

	return m->count;
}


static void aft_free_rx_complete_list(private_area_t *chan)
{
	void *skb;
	while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){

		chan->if_stats.rx_dropped++;
		aft_init_requeue_free_skb(chan, skb);
	}
}

static void aft_list_descriptors(private_area_t *chan)
{
#if 0
	u32 reg,cur_dma_ptr;
	sdla_t *card=chan->card;
	aft_dma_chain_t *dma_chain;
	u32 dma_descr;
	int i;

	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
	cur_dma_ptr=get_current_rx_dma_ptr(reg);
	
	DEBUG_EVENT("%s: List Descritpors:\n",chan->if_name);

	DEBUG_EVENT("%s: Chain DMA Status=0x%lX, TxCur=%i, TxPend=%i RxCur=%i RxPend=%i HwCur=%i RC=%i RFree=%i\n",
			chan->if_name, 
			chan->dma_chain_status,
			chan->tx_chain_indx,
			chan->tx_pending_chain_indx,
			chan->rx_chain_indx,
			chan->rx_pending_chain_indx,
			cur_dma_ptr,
			wan_skb_queue_len(&chan->wp_rx_complete_list),
                        wan_skb_queue_len(&chan->wp_rx_free_list));


	for (i=0;i<MAX_AFT_DMA_CHAINS;i++){

		dma_chain = &chan->rx_dma_chain_table[i];
		if (!dma_chain){
			DEBUG_ERROR("%s:%d Assertion Error !!!!\n",
				__FUNCTION__,__LINE__);
			break;
		}

		dma_descr=(dma_chain->index<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

		DEBUG_EVENT("%s: DMA=%i : Init=%li Go=%u Intr=%u Addr=%s Skb=%s\n",
				chan->if_name,
				dma_chain->index,
				dma_chain->init,
				wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg),
				wan_test_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg),
				dma_chain->dma_addr?"Yes":"No",
			        dma_chain->skb?"Yes":"No");
	}
#endif
}

static void aft_list_tx_descriptors(private_area_t *chan)
{
#if 1
	u32 reg,cur_dma_ptr,low,dma_ctrl;
	sdla_t *card=chan->card;
	aft_dma_chain_t *dma_chain;
	u32 dma_descr;
	int i;

	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);
	cur_dma_ptr=get_current_tx_dma_ptr(reg);

	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&dma_ctrl);

	DEBUG_EVENT("%s: List TX Descritpors:\n",chan->if_name);

	DEBUG_EVENT("%s: Chain DMA Status=0x%lX, TxCur=%i, TxPend=%i HwCur=%i RP=%i RC=%i RFree=%i Ctrl=0x%X\n",
			chan->if_name, 
			chan->dma_chain_status,
			chan->tx_chain_indx,
			chan->tx_pending_chain_indx,
			cur_dma_ptr,
			wan_skb_queue_len(&chan->wp_tx_pending_list),
			wan_skb_queue_len(&chan->wp_tx_complete_list),
			wan_skb_queue_len(&chan->wp_tx_free_list),
			dma_ctrl);

	
	for (i=0;i<MAX_AFT_DMA_CHAINS;i++){

		dma_chain = &chan->tx_dma_chain_table[i];
		if (!dma_chain){
			DEBUG_ERROR("%s:%d Assertion Error !!!!\n",
				__FUNCTION__,__LINE__);
			break;
		}

		dma_descr=(dma_chain->index<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

		dma_descr=(dma_chain->index<<4) + XILINX_TxDMA_DESCRIPTOR_LO;
		card->hw_iface.bus_read_4(card->hw,dma_descr,&low);

		DEBUG_EVENT("%s: DMA=%i : Init=%lu Go=%u Intr=%u Addr=%s Skb=%s HI=0x%X LO=0x%X\n",
				chan->if_name,
				dma_chain->index,
				dma_chain->init,
				wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg),
				wan_test_bit(DMA_HI_TE3_INTR_DISABLE_BIT,&reg),
				dma_chain->dma_addr?"Yes":"No",
			        dma_chain->skb?"Yes":"No",
				reg,low);
	}
#endif
}


static void aft_free_rx_descriptors(private_area_t *chan)
{
	u32 reg;
	sdla_t *card=chan->card;
	aft_dma_chain_t *dma_chain;
	u32 dma_descr;
	int i;

	for (i=0;i<MAX_AFT_DMA_CHAINS;i++){

		dma_chain = &chan->rx_dma_chain_table[i];
		if (!dma_chain){
			DEBUG_ERROR("%s:%d Assertion Error !!!!\n",
				__FUNCTION__,__LINE__);
			break;
		}

		dma_descr=(dma_chain->index<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_read_4(card->hw,dma_descr,&reg);

		/* If GO bit is set, then the current DMA chain
		 * is in process of being transmitted, thus
		 * all are busy */
		reg=0;
		card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

		aft_rx_dma_chain_init(chan,&chan->rx_dma_chain_table[i]);

		dma_chain->skb=NULL;
		dma_chain->dma_addr=0;
		dma_chain->dma_len=0;
		dma_chain->pkt_error=0;
		wan_clear_bit(0,&dma_chain->init);
	}

	aft_free_rx_complete_list(chan);

	DEBUG_TEST("%s: Free Rx Bufs (RxComp=%i RxFree=%i)\n",
			chan->if_name,
			wan_skb_queue_len(&chan->wp_rx_complete_list),
			wan_skb_queue_len(&chan->wp_rx_free_list));  	

	aft_reset_rx_chain_cnt(chan);     
}

static void aft_reset_rx_chain_cnt(private_area_t *chan)
{
	u32 reg,cur_dma_ptr;
	sdla_t *card=chan->card;
	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);

	if (chan->single_dma_chain){
		set_current_rx_dma_ptr(&reg,0);
	}
	
	cur_dma_ptr=get_current_rx_dma_ptr(reg);
       	chan->rx_pending_chain_indx = chan->rx_chain_indx = cur_dma_ptr;
	return;
}           

static void aft_reset_tx_chain_cnt(private_area_t *chan)
{
	u32 reg,cur_dma_ptr;
	sdla_t *card=chan->card;
	card->hw_iface.bus_read_4(card->hw,AFT_TE3_CRNT_DMA_DESC_ADDR_REG,&reg);

	if (chan->single_dma_chain){
		set_current_tx_dma_ptr(&reg,0);
		cur_dma_ptr=0;
	}

	cur_dma_ptr=get_current_tx_dma_ptr(reg);

	chan->tx_pending_chain_indx = chan->tx_chain_indx = cur_dma_ptr;
	return;
}

static void aft_free_tx_descriptors(private_area_t *chan)
{
	u32 reg;
	sdla_t *card=chan->card;
	aft_dma_chain_t *dma_chain;
	u32 dma_descr;
	int i;
	void *skb;
	unsigned int dma_cnt=MAX_AFT_DMA_CHAINS;

	DEBUG_TEST("%s:%s: Tx: Freeing Descripors\n",card->devname,chan->if_name);

	wan_clear_bit(TX_INTR_PENDING,&chan->dma_chain_status);
	wan_clear_bit(TX_DMA_BUSY,&chan->dma_status);

	for (i=0;i<dma_cnt;i++){

		dma_chain = &chan->tx_dma_chain_table[i];
		if (!dma_chain){
			DEBUG_ERROR("%s:%d Assertion Error !!!!\n",
				__FUNCTION__,__LINE__);
			break;
		}

		dma_descr=(dma_chain->index<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
		reg=0;
		card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

		aft_tx_dma_chain_init(chan, dma_chain);
	}

	aft_reset_tx_chain_cnt(chan);

	while((skb=wan_skb_dequeue(&chan->wp_tx_complete_list)) != NULL){
		wan_skb_free(skb);
	}
}



static void aft_te3_led_ctrl(sdla_t *card, int color, int led_pos, int on)
{
	u32 reg;
	if (card->adptr_subtype == AFT_SUBTYPE_SHARK) {
		
		switch (color){
		
		case WAN_AFT_RED:
			if (on){
				wan_clear_bit(0,&card->u.aft.led_ctrl);
				wan_clear_bit(2,&card->u.aft.led_ctrl);
			}else{
				wan_set_bit(0,&card->u.aft.led_ctrl);
				wan_set_bit(2,&card->u.aft.led_ctrl);
			}	
			break;
		
		case WAN_AFT_GREEN:
			if (on){
				wan_clear_bit(1,&card->u.aft.led_ctrl);
				wan_clear_bit(3,&card->u.aft.led_ctrl);
			}else{
				wan_set_bit(1,&card->u.aft.led_ctrl);
				wan_set_bit(3,&card->u.aft.led_ctrl);
			}	
			break;			
		}
		
		write_cpld(card,0x00,card->u.aft.led_ctrl);
	} else {
		card->hw_iface.bus_read_4(card->hw,TE3_LOCAL_CONTROL_STATUS_REG,&reg);
		aft_te3_set_led(color, led_pos, on, &reg);
		card->hw_iface.bus_write_4(card->hw,TE3_LOCAL_CONTROL_STATUS_REG,reg);
	}
}


static void __aft_fe_intr_ctrl(sdla_t *card, int status)
{
	u32 reg;

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
	if (status){
		wan_set_bit(FRONT_END_INTR_ENABLE_BIT,&reg);
	}else{
		wan_clear_bit(FRONT_END_INTR_ENABLE_BIT,&reg);
	}
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
}

#if 0
static void aft_fe_intr_ctrl(sdla_t *card, int status)
{
	wan_smp_flag_t	smp_flags;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	__aft_fe_intr_ctrl(card, status);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
}
#endif


#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))     
static void aft_port_task (void * card_ptr)
# else
static void aft_port_task (struct work_struct *work)	
# endif
#else
static void aft_port_task (void * card_ptr, int arg)
#endif
{
#if defined(__LINUX__)
# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
        sdla_t 		*card = (sdla_t *)container_of(work, sdla_t, u.aft.port_task);
# else
	sdla_t 		*card = (sdla_t *)card_ptr;
# endif
#else
	sdla_t 		*card = (sdla_t *)card_ptr;
#endif 
	wan_smp_flag_t	smp_flags,isr_flags;

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		return;
	}

	DEBUG_TEST("%s: AFT PORT TASK CMD=0x%X!\n",
			card->devname,card->u.aft.port_task_cmd);
		
	if (wan_test_bit(AFT_FE_INTR,&card->u.aft.port_task_cmd)){
		card->hw_iface.hw_lock(card->hw,&smp_flags);

		wan_spin_lock_irq(&card->wandev.lock,&isr_flags);
		__aft_fe_intr_ctrl(card, 0);
		front_end_interrupt(card,0);
		wan_clear_bit(AFT_FE_INTR,&card->u.aft.port_task_cmd);
		__aft_fe_intr_ctrl(card, 1);
		wan_spin_unlock_irq(&card->wandev.lock,&isr_flags);
		
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	}

	if (wan_test_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd)){
		card->hw_iface.hw_lock(card->hw,&smp_flags);
		
		wan_spin_lock_irq(&card->wandev.lock,&isr_flags);
		__aft_fe_intr_ctrl(card, 0);
		WAN_FECALL(&card->wandev, polling, (&card->fe));
		wan_clear_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd);
		__aft_fe_intr_ctrl(card, 1);

		handle_front_end_state(card);

		wan_spin_unlock_irq(&card->wandev.lock,&isr_flags);
		
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	}

	if (wan_test_bit(AFT_FE_LED,&card->u.aft.port_task_cmd)){
		card->hw_iface.hw_lock(card->hw,&smp_flags);
		wan_spin_lock_irq(&card->wandev.lock,&isr_flags);
		__aft_fe_intr_ctrl(card, 0);
		if (card->wandev.state == WAN_CONNECTED){
			aft_te3_led_ctrl(card, WAN_AFT_RED, 0, WAN_AFT_OFF);	
			aft_te3_led_ctrl(card, WAN_AFT_GREEN, 0,WAN_AFT_ON);
		}else{
			aft_te3_led_ctrl(card, WAN_AFT_RED, 0, WAN_AFT_ON);	
			aft_te3_led_ctrl(card, WAN_AFT_GREEN, 0,WAN_AFT_OFF);
		}
		wan_clear_bit(AFT_FE_LED,&card->u.aft.port_task_cmd);
		__aft_fe_intr_ctrl(card, 1);
		wan_spin_unlock_irq(&card->wandev.lock,&isr_flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	}


}

static void aft_critical_shutdown (sdla_t *card)
{

#ifdef __LINUX__
	printk(KERN_ERR "%s: Error: Card Critically Shutdown!\n",
			card->devname);
#else
	DEBUG_ERROR("%s: Error: Card Critically Shutdown!\n",
			card->devname);
#endif  	

	
	/* Unconfiging, only on shutdown */
	if (IS_TE3(&card->fe.fe_cfg)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
		if (card->wandev.fe_iface.post_unconfig){
			card->wandev.fe_iface.post_unconfig(&card->fe);
		}
	}     
	
       	port_set_state(card,WAN_DISCONNECTED);
	disable_data_error_intr(card,DEVICE_DOWN);
	wan_set_bit(CARD_DOWN,&card->wandev.critical);	  

	aft_te3_led_ctrl(card, WAN_AFT_RED, 1, WAN_AFT_ON);	
	aft_te3_led_ctrl(card, WAN_AFT_GREEN, 1,WAN_AFT_OFF);
	                                                    
}


static int aft_fifo_intr_ctrl(sdla_t *card, int ctrl)
{
        u32 reg;
	card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG,&reg);
      	card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG,&reg);

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
	if (ctrl) {
		wan_set_bit(ERROR_INTR_ENABLE_BIT,&reg); 
	} else {
		wan_clear_bit(ERROR_INTR_ENABLE_BIT,&reg); 
	}
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);



        if (!ctrl){
		card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG,&reg);
 	     	card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG,&reg);
        }

        return 0;
}


#if 0
static int aft_hdlc_core_ready(sdla_t *card)
{
	u32 reg;
	int i,err=1;

	for (i=0;i<5;i++){
		card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

		if (!wan_test_bit(HDLC_CORE_READY_FLAG_BIT,&reg)){
			WP_DELAY(500);
		}else{
			err=0;
			break;
		}
	}

	if (err){
	DEBUG_ERROR("%s: Critical Error: HDLC Core not ready!\n",
			card->devname);
	}
	return err;
}
#endif
/****** End ****************************************************************/
