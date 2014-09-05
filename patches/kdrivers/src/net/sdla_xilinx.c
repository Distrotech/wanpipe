/*****************************************************************************
* sdla_xilinx.c	WANPIPE(tm) AFT A101/2 Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2007 Sangoma Technologies Inc.
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
#include "wanpipe_snmp.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */
#include "sdlapci.h"
#include "wanpipe_iface.h"
#include "wanpipe_tdm_api.h"
#include "wanpipe_api_hdr.h"
#include "sdla_xilinx.h"
 
#ifdef __WINDOWS__
# define PCI_DMA_FROMDEVICE	0
# define PCI_DMA_TODEVICE	1

extern
int 
set_netdev_state(
	sdla_t* card,
	netdevice_t* sdla_net_dev,
	int state
	);

extern
void
wan_get_random_mac_address(
	OUT unsigned char *mac_address
	);

extern 
int wp_get_motherboard_enclosure_serial_number(
	OUT char *buf, 
	IN int buf_length
	);

extern
int
sdla_restore_pci_config_space(
	IN void *level1_dev_pdx
    ) ;

#endif

/* #define  XILINX_A010 	1	*/

/****** Defines & Macros ****************************************************/

/* Private critical flags */
enum {
	POLL_CRIT = PRIV_CRIT,
	TX_BUSY,
	RX_BUSY,
	TASK_POLL,
	CARD_DOWN
};

enum wp_device_down_states{
	LINK_DOWN,
	DEVICE_DOWN
};


enum {
	AFT_FE_CFG_ERR,
	AFT_FE_CFG,
	AFT_FE_INTR,
	AFT_FE_POLL,
	AFT_FE_TDM_RBS
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

#if defined(__LINUX__)
# define AFT_TDM_API_SUPPORT 1
#elif defined(__WINDOWS__)
# define AFT_TDM_API_SUPPORT 1
#else
# undef AFT_TDM_API_SUPPORT
#endif

#define AFT_MAX_CHIP_SECURITY_CNT 100

#define AFT_MIN_FRMW_VER 24

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)


static int aft_rx_copyback=1000;

/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
* The card structure used to occupy the private area but now the following
* structure will incorporate the card structure along with Protocol specific data
*/

typedef struct private_area
{
	wanpipe_common_t 	common;/* MUST be at the top */

	wan_skb_queue_t 	wp_tx_free_list;
	wan_skb_queue_t 	wp_tx_pending_list;
	wan_skb_queue_t 	wp_tx_complete_list;
	netskb_t 		*tx_dma_skb;
	u8			tx_dma_cnt;

	wan_skb_queue_t 	wp_rx_free_list;
	wan_skb_queue_t 	wp_rx_complete_list;
	netskb_t 		*rx_dma_skb;

	u32	 		time_slot_map;
	unsigned char 		num_of_time_slots;
	long          		logic_ch_num;

	unsigned char		hdlc_eng;
	wan_bitmap_t		dma_status;
	unsigned char 		ignore_modem;

	struct net_device_stats	if_stats;

	int 		tracing_enabled;		/* For enabling Tracing */
	wan_time_t		router_start_time;	/*unsigned long 	router_start_time;*/
	unsigned long   trace_timeout;

	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	wan_time_t		router_up_time;	/* unsigned long 	router_up_time; */

	unsigned char  	mc;			/* Mulitcast support on/off */
	unsigned char 	udp_pkt_src;		/* udp packet processing */
	unsigned short 	timer_int_enabled;

	wan_bitmap_t 	interface_down;

	/* Polling task queue. Each interface
	* has its own task queue, which is used
	* to defer events from the interrupt */
	wan_taskq_t 		poll_task;
	wan_timer_info_t 	poll_delay_timer;

	u8 		gateway;
	u8 		true_if_encoding;


	/* Entry in proc fs per each interface */
	struct proc_dir_entry	*dent;

	unsigned char 	udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t 	udp_pkt_len;

	char 		if_name[WAN_IFNAME_SZ+1];

	u8		idle_flag;
	u16		max_idle_size;
	wan_bitmap_t	idle_start;

	wan_bitmap_t		pkt_error;
	u8		rx_fifo_err_cnt;

	int		first_time_slot;

	netskb_t  	*tx_idle_skb;
	u32		tx_dma_addr;
	u32		tx_dma_len;
	wan_bitmap_t	rx_dma;
	unsigned char   pci_retry;

	unsigned char	fifo_size_code;
	unsigned char	fifo_base_addr;
	unsigned char 	fifo_size;

	int		dma_mru;
	int		mru;
	int		mtu;

	void *		prot_ch;
	int		prot_state;

	wan_trace_t	trace_info;

	int		tdmv_sync;
	unsigned char	lip_atm;

	unsigned long	up;
	unsigned char   *tx_realign_buf;

	aft_op_stats_t  	opstats;
	aft_comm_err_stats_t	errstats;

	wan_xilinx_conf_if_t 	cfg;

	unsigned int		channelized_cfg;

#ifdef AFT_TDM_API_SUPPORT
	wanpipe_tdm_api_dev_t	wp_tdm_api_dev_idx[32];
#endif
	unsigned int		tdmv_chan;
	unsigned char		tdm_api;
	unsigned int 		tdmv_zaptel_cfg;
	unsigned int		tdmapi_timeslots;
	unsigned int		max_tx_bufs;

	struct private_area	*next;

	wan_skb_queue_t 	wp_dealloc_list;
}private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03

#define WP_WAIT 	0
#define WP_NO_WAIT	1

/* Function interface between WANPIPE layer and kernel */
#ifdef __WINDOWS__
wan_iface_t wan_iface;
extern int aft_core_tdmapi_event_init(private_area_t *chan);
#else
extern wan_iface_t wan_iface;
#endif

/* variable for keeping track of enabling/disabling FT1 monitor status */
/* static int rCount; */

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

/**SECTOIN**************************************************
*
* Function Prototypes
*
***********************************************************/

int wp_xilinx_default_devcfg(sdla_t* card, wandev_conf_t* conf);
int wp_xilinx_default_ifcfg(sdla_t* card, wanif_conf_t* conf);

/* WAN link driver entry points. These are called by the WAN router module. */
static int 	update (wan_device_t* wandev);
static int 	new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf);
static int 	del_if(wan_device_t *wandev, netdevice_t *dev);

/* Network device interface */
static int 	if_init   (netdevice_t* dev);
static int 	wanpipe_xilinx_open   (netdevice_t* dev);
static int 	wanpipe_xilinx_close  (netdevice_t* dev);
static int 	wanpipe_xilinx_ioctl(netdevice_t*, struct ifreq*, wan_ioctl_cmd_t);

#if defined(__LINUX__) || defined(__WINDOWS__)
static int 	wanpipe_xilinx_send (netskb_t* skb, netdevice_t* dev);
static struct net_device_stats* wanpipe_xilinx_ifstats (netdevice_t* dev);
#else
static int	wan_aft_output(netdevice_t*, netskb_t*, struct sockaddr*,
struct rtentry*);
#endif

static void 	handle_front_end_state(void* card_id);
static void 	enable_timer(void* card_id);
static void 	wanpipe_xilinx_tx_timeout (netdevice_t* dev);

/* Miscellaneous Functions */
static void 	port_set_state (sdla_t *card, int);

static void 	disable_comm (sdla_t *card);

/* Interrupt handlers */
static WAN_IRQ_RETVAL 	wp_xilinx_isr (sdla_t* card);

/* Bottom half handlers */
#if defined(__LINUX__)
static void 	wp_bh (unsigned long);
#elif defined(__WINDOWS__)
static void wp_bh (IN PKDPC Dpc, IN PVOID data, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
#else
static void	 wp_bh(void *data, int pending);
#endif

/* Miscellaneous functions */
static int 	process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
								 private_area_t*,
								 int local_dev);

static int 	xilinx_chip_configure(sdla_t *card);
static int 	xilinx_chip_unconfigure(sdla_t *card);
static int	xilinx_dev_configure(sdla_t *card, private_area_t *chan);
static void 	xilinx_dev_unconfigure(sdla_t *card, private_area_t *chan);
static int	xilinx_dma_rx(sdla_t *card, private_area_t *chan);
static void 	xilinx_dev_enable(sdla_t *card, private_area_t *chan);
static void 	xilinx_dev_close(sdla_t *card, private_area_t *chan);
static int 	xilinx_dma_tx (sdla_t *card, private_area_t *chan);
static void 	xilinx_dma_tx_complete (sdla_t *card, private_area_t *chan);
static void 	xilinx_dma_rx_complete (sdla_t *card, private_area_t *chan);
static void 	xilinx_dma_max_logic_ch(sdla_t *card);
static int 	xilinx_init_rx_dev_fifo(sdla_t *card, private_area_t *chan,
									unsigned char);
static void 	xilinx_init_tx_dma_descr(sdla_t *card, private_area_t *chan);
static int 	xilinx_init_tx_dev_fifo(sdla_t *card,
									private_area_t *chan, unsigned char);
static void 	xilinx_tx_post_complete (sdla_t *card,
										 private_area_t *chan, netskb_t *skb);
static void 	xilinx_rx_post_complete (sdla_t *card, private_area_t *chan,
										 netskb_t *skb,
										 netskb_t **new_skb,
										 wan_bitmap_t *pkt_error);


static char 	request_xilinx_logical_channel_num (sdla_t *card,
													private_area_t *chan, long *free_ch);
static void 	free_xilinx_logical_channel_num (sdla_t *card, int logic_ch);


static unsigned char read_cpld(sdla_t *card, unsigned short cpld_off);
static unsigned char write_cpld(sdla_t *card, unsigned short cpld_off,
								unsigned char cpld_data);

static int 	aft_devel_ioctl(sdla_t *card,struct ifreq *ifr);
static int 	xilinx_write_bios(sdla_t *card,wan_cmd_api_t *api_cmd);
static int 	xilinx_write(sdla_t *card,wan_cmd_api_t *api_cmd);
static int 	xilinx_api_fe_write(sdla_t *card,wan_cmd_api_t*);
static int 	xilinx_read(sdla_t *card,wan_cmd_api_t *api_cmd);
static int 	xilinx_api_fe_read(sdla_t *card,wan_cmd_api_t*);

static void 	front_end_interrupt(sdla_t *card, unsigned long reg, int lock);
static void  	enable_data_error_intr(sdla_t *card);
static void  	disable_data_error_intr(sdla_t *card, enum wp_device_down_states state);

static void 	xilinx_tx_fifo_under_recover (sdla_t *card, private_area_t *chan);

static int 	xilinx_write_ctrl_hdlc(sdla_t *card, u32 timeslot, u8 reg_off, u32 data);

static int 	set_chan_state(sdla_t* card, netdevice_t* dev, int state);

static int 	fifo_error_interrupt(sdla_t *card, u32 reg, u32 tx_err, u32 rx_err);
static int 	request_fifo_baddr_and_size(sdla_t *card, private_area_t *chan);
static int 	map_fifo_baddr_and_size(sdla_t *card,
									unsigned char fifo_size,
									unsigned char *addr);
static int 	free_fifo_baddr_and_size (sdla_t *card, private_area_t *chan);

static int 	update_comms_stats(sdla_t* card);

static void 	aft_red_led_ctrl(sdla_t *card, int mode);

static int 	protocol_init (sdla_t*card,netdevice_t *dev,
						   private_area_t *chan,
						   wanif_conf_t* conf);

static int 	protocol_stop (sdla_t *card, netdevice_t *dev);
static int 	protocol_start (sdla_t *card, netdevice_t *dev);
static int 	protocol_shutdown (sdla_t *card, netdevice_t *dev);
static void 	protocol_recv(sdla_t *card, private_area_t *chan, netskb_t *skb);

static int 	aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num, int irq);
static int 	aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb);
#if 0
static int 	aft_reinit_pending_rx_bufs(private_area_t *chan);
#endif
static int 	aft_core_ready(sdla_t *card);
static void 	aft_unmap_tx_dma(sdla_t *card, private_area_t *chan);

static int 	channel_timeslot_sync_ctrl(sdla_t *card, private_area_t * chan, int enable);
static int 	rx_chan_timeslot_sync_ctrl(sdla_t *card,int start);
static void 	aft_report_rbsbits(void* pcard, int channel, unsigned char status);

static int 	aft_realign_skb_pkt(private_area_t *chan, netskb_t *skb);

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static void xilinx_aft_port_task (void * card_ptr);
# else
static void xilinx_aft_port_task (struct work_struct *work);
# endif
#elif defined(__WINDOWS__)
KDEFERRED_ROUTINE xilinx_aft_port_task;
#else
static void xilinx_aft_port_task (void * card_ptr, int arg);
#endif

static void 	aft_fe_intr_ctrl(sdla_t *card, int status);
static void 	__aft_fe_intr_ctrl(sdla_t *card, int status);

static int  aft_dev_open(sdla_t *card, private_area_t *chan);
static void aft_dev_close(sdla_t *card, private_area_t *chan);

static int aft_dma_rx_tdmv(sdla_t *card, private_area_t *chan, netskb_t *skb);

/* Procfs functions */
static int	wan_aft_get_info(void* pcard, struct seq_file* m, int* stop_cnt);

static int 	aft_tdmv_init(sdla_t *card, wandev_conf_t *conf);
static int 	aft_tdmv_free(sdla_t *card);
static int	aft_tdmv_if_init(sdla_t *card, private_area_t *chan, wanif_conf_t *conf);
static int 	aft_tdmv_if_free(sdla_t *card, private_area_t *chan);
static void 	aft_critical_shutdown (sdla_t *card);

#ifdef AFT_TDM_API_SUPPORT
static int aft_read_rbs_bits(void *chan_ptr, u32 ch, u8 *rbs_bits);
static int aft_write_rbs_bits(void *chan_ptr, u32 ch, u8 rbs_bits);
static int aft_write_hdlc_frame(void *chan_ptr,  netskb_t *skb, wp_api_hdr_t *hdr);
static int aft_tdm_api_rx_tx_channelized(sdla_t *card, private_area_t *chan, netskb_t *skb);
static int aft_tdm_api_update_state_channelized(sdla_t *card, private_area_t *chan, int state);
static int aft_tdm_api_free_channelized(sdla_t *card, private_area_t *chan);
#endif

static void wanpipe_wake_stack(private_area_t* chan);

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

int wp_xilinx_default_devcfg(sdla_t* card, wandev_conf_t* conf)
{
	conf->config_id			= WANCONFIG_AFT;
	conf->u.aft.dma_per_ch	= 10;
	conf->u.aft.mru	= 1500;
	return 0;
}

int wp_xilinx_default_ifcfg(sdla_t* card, wanif_conf_t* conf)
{
	memcpy(conf->usedby, "WANPIPE", 7);
	conf->if_down		= 0;
	conf->ignore_dcd	= WANOPT_NO;
	conf->ignore_cts	= WANOPT_NO;
	conf->hdlc_streaming	= WANOPT_YES;
	conf->mc		= 0;
	conf->gateway		= 0;
	conf->active_ch		= ENABLE_ALL_CHANNELS;

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

int wp_xilinx_init (sdla_t* card, wandev_conf_t* conf)
{
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	int err;
#endif

	/* Verify configuration ID */
	wan_clear_bit(CARD_DOWN,&card->wandev.critical);
	if (card->wandev.config_id != WANCONFIG_AFT) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
			card->devname, card->wandev.config_id);
		return -EINVAL;
	}

	if (conf == NULL){
		DEBUG_EVENT("%s: Bad configuration structre!\n",
			card->devname);
		return -EINVAL;
	}


	/* Obtain hardware configuration parameters */
	card->wandev.clocking 			= conf->clocking;
	card->wandev.ignore_front_end_status 	= conf->ignore_front_end_status;
	card->wandev.ttl 			= conf->ttl;
	card->wandev.electrical_interface      	= conf->electrical_interface;
	card->wandev.comm_port 			= conf->comm_port;
	card->wandev.udp_port   		= conf->udp_port;
	card->wandev.new_if_cnt 		= 0;
	wan_atomic_set(&card->wandev.if_cnt,0);
	card->u.aft.chip_security_cnt=0;

	card->hw_iface.getcfg(card->hw, SDLA_COREREV, &card->u.aft.firm_ver);
	if (card->u.aft.firm_ver < AFT_MIN_FRMW_VER){
		DEBUG_EVENT( "%s: Invalid/Obselete AFT A101/2 firmware ver %i (not >= %d)!\n",
			card->devname, card->u.aft.firm_ver,AFT_MIN_FRMW_VER);
		DEBUG_EVENT( "%s  Refer to /usr/share/doc/wanpipe/README.aft_firm_update\n",
			card->devname);
		DEBUG_EVENT( "%s: Please contact Sangoma Technologies for more info.\n",
			card->devname);
		return -EINVAL;
	}

	memcpy(&card->u.aft.cfg,&conf->u.aft,sizeof(wan_xilinx_conf_t));
	memcpy(&card->tdmv_conf,&conf->tdmv_conf,sizeof(wan_tdmv_conf_t));

	card->u.aft.cfg.dma_per_ch = 10;
	if (conf->u.aft.dma_per_ch){
		card->u.aft.cfg.dma_per_ch=conf->u.aft.dma_per_ch;
		if (card->u.aft.cfg.dma_per_ch > MAX_DMA_PER_CH ||
			card->u.aft.cfg.dma_per_ch < MIN_DMA_PER_CH){
				DEBUG_EVENT("%s: Error invalid DMA Per Ch %d (Min=%d Max=%d)\n",
					card->devname,card->u.aft.cfg.dma_per_ch,
					MIN_DMA_PER_CH,MAX_DMA_PER_CH);
				return -EINVAL;
		}
	}

	/* TE1 Make special hardware initialization for T1/E1 board */
	if (IS_TE1_MEDIA(&conf->fe_cfg)){

		if (conf->fe_cfg.cfg.te_cfg.active_ch == 0){
			conf->fe_cfg.cfg.te_cfg.active_ch = -1;
		}

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg	= card->hw_iface.fe_write;
		card->fe.read_fe_reg	= card->hw_iface.fe_read;

		card->wandev.te_report_rbsbits = aft_report_rbsbits;
		card->wandev.fe_enable_timer = enable_timer;
		card->wandev.te_link_state = handle_front_end_state;

		if (card->fe.fe_cfg.cfg.te_cfg.te_clock == WAN_NORMAL_CLK){
			/* If using normal clocking disable
			* reference clock configuration */
			card->fe.fe_cfg.cfg.te_cfg.te_ref_clock = WAN_TE1_REFCLK_OSC;
		}

		conf->electrical_interface =
			IS_T1_CARD(card) ? WANOPT_V35 : WANOPT_RS232;

		if (card->wandev.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

	}else{
		DEBUG_EVENT("%s: Error: Unknown Front end cfg 0x%X (T1/E1)\n",
			card->devname,conf->fe_cfg.media);
		return -EINVAL;
	}

	card->u.aft.tdmv_dchan = 0;
	if (IS_E1_CARD(card)) {
		card->tdmv_conf.dchan = card->tdmv_conf.dchan << 1;
		wan_clear_bit(0,&card->tdmv_conf.dchan);
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

	card->disable_comm              = NULL;

#ifdef WANPIPE_ENABLE_PROC_FILE_HOOKS
	/* Proc fs functions hooks */
	card->wandev.get_config_info 	= &get_config_info;
	card->wandev.get_status_info 	= &get_status_info;
	card->wandev.get_dev_config_info= &get_dev_config_info;
	card->wandev.get_if_info     	= &get_if_info;
	card->wandev.set_dev_config    	= &set_dev_config;
	card->wandev.set_if_info     	= &set_if_info;
#endif
	card->wandev.get_info		= &wan_aft_get_info;

	/* Setup Port Bps */
	if(card->wandev.clocking) {
		card->wandev.bps = conf->bps;
	}else{
		card->wandev.bps = 0;
	}

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
	card->wandev.mtu = conf->mtu;
	card->wan_tdmv.sc = NULL;
#else

	card->wandev.mtu=conf->mtu;
	if (card->wandev.mtu > MAX_WP_PRI_MTU ||
		card->wandev.mtu < MIN_WP_PRI_MTU){
			DEBUG_EVENT("%s: Error Invalid Global MTU %d (Min=%d, Max=%d)\n",
				card->devname,card->wandev.mtu,
				MIN_WP_PRI_MTU,MAX_WP_PRI_MTU);

			return -EINVAL;
	}
#endif

	card->u.aft.cfg.mru=conf->u.aft.mru;
	if (!card->u.aft.cfg.mru){
		card->u.aft.cfg.mru = card->wandev.mtu;
	}

	if (card->u.aft.cfg.mru > MAX_WP_PRI_MTU ||
		card->u.aft.cfg.mru < MIN_WP_PRI_MTU){
			DEBUG_EVENT("%s: Error Invalid Global MRU %d (Min=%d, Max=%d)\n",
				card->devname,card->u.aft.cfg.mru,
				MIN_WP_PRI_MTU,MAX_WP_PRI_MTU);

			return -EINVAL;
	}

	write_cpld(card,LED_CONTROL_REG,0x0E);


	card->hw_iface.getcfg(card->hw, SDLA_BASEADDR, &card->u.aft.bar);

	WAN_TASKQ_INIT((&card->u.aft.port_task),0,xilinx_aft_port_task,card);

	/* Set protocol link state to disconnected,
	* After seting the state to DISCONNECTED this
	* function must return 0 i.e. success */
	port_set_state(card,WAN_CONNECTING);

	xilinx_delay(1);
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	card->isr = &wp_xilinx_isr;
	err=xilinx_chip_configure(card);
	if (err){
		xilinx_chip_unconfigure(card);
		return err;
	}
#endif

	xilinx_delay(1);

	wan_set_bit(AFT_CHIP_CONFIGURED,&card->u.aft.chip_cfg_status);

	if (wan_test_bit(AFT_FRONT_END_UP,&card->u.aft.chip_cfg_status)){
		wan_smp_flag_t smp_flags;
		DEBUG_TEST("%s: Front end up, retrying enable front end!\n",
			card->devname);
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		handle_front_end_state(card);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

		wan_clear_bit(AFT_FRONT_END_UP,&card->u.aft.chip_cfg_status);
	}

	DEBUG_EVENT("%s: Configuring Device   :%s FrmVr=%02X\n",
		card->devname,card->devname,card->u.aft.firm_ver);
	DEBUG_EVENT("%s:    Global MTU   = %d\n",
		card->devname,
		card->wandev.mtu);
	DEBUG_EVENT("%s:    Global MRU   = %d\n",
		card->devname,
		card->u.aft.cfg.mru);
	DEBUG_EVENT("%s:    RBS Signal   = %s\n",
		card->devname,
		card->u.aft.cfg.rbs?"On":"Off");
	DEBUG_EVENT("%s:    FE Ref Clock = %s\n",
		card->devname,
		WAN_TE1_REFCLK(&card->fe) == WAN_TE1_REFCLK_OSC?"Osc":"Line");


	DEBUG_EVENT("\n");

	err=aft_tdmv_init(card,conf);
	if (err){
		disable_comm(card);
		return err;
	}

	card->disable_comm              = &disable_comm;

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
	sdla_t			*card = wandev->priv;
	netdevice_t		*dev;
	volatile private_area_t	*chan;
	wan_smp_flag_t		smp_flags;

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;

	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	if(wan_test_bit(PERI_CRIT, (void*)&card->wandev.critical))
		return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return -ENODEV;

	chan=wan_netif_priv(dev);

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
				DEBUG_EVENT("%s: Waking up device! Q=%d\n",
					wan_netif_name(dev),
					wan_skb_queue_len(&chan->wp_tx_pending_list));
				WAN_NETIF_START_QUEUE(dev);	/*start_net_queue(dev);*/
			}
		}
	}
#endif

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	update_comms_stats(card);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);


	return 0;
}

/* can not include aft_core_private.h because private_area_t is defined there */
static __inline void wan_aft_skb_defered_dealloc(private_area_t *chan, netskb_t *skb)
{
	wan_skb_queue_tail(&chan->wp_dealloc_list,skb);
	WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
}

#if defined(__LINUX__) || defined(__WINDOWS__)

int xilinx_wan_user_process_udp_mgmt_pkt(void* card_ptr, void* chan_ptr, void *udata)
{
	sdla_t *card = (sdla_t *)card_ptr;
	private_area_t *chan = (private_area_t*)chan_ptr;
	wan_udp_pkt_t *wan_udp_pkt;
	
	DEBUG_UDP("%s(): line: %d\n", __FUNCTION__, __LINE__);

	if (wan_atomic_read(&chan->udp_pkt_len) != 0){
		return -EBUSY;
	}

	wan_atomic_set(&chan->udp_pkt_len, MAX_LGTH_UDP_MGNT_PKT);

	wan_udp_pkt=(wan_udp_pkt_t*)chan->udp_pkt_data;

#if defined (__WINDOWS__)
	/* udata IS a pointer to wan_udp_hdr_t. copy data from user's buffer */
	wpabs_memcpy(&wan_udp_pkt->wan_udp_hdr, udata, sizeof(wan_udp_hdr_t));
#else
	if (WAN_COPY_FROM_USER(
			&wan_udp_pkt->wan_udp_hdr,
			udata,
			sizeof(wan_udp_hdr_t))){
		wan_atomic_set(&chan->udp_pkt_len,0);
		return -EFAULT;
	}
#endif

	process_udp_mgmt_pkt(card,chan->common.dev,chan,1);

	/* This area will still be critical to other
		* PIPEMON commands due to udp_pkt_len
		* thus we can release the irq */

	if (wan_atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
		DEBUG_EVENT( "%s: Error: Pipemon buf too bit on the way up! %d\n",
				card->devname,wan_atomic_read(&chan->udp_pkt_len));
		wan_atomic_set(&chan->udp_pkt_len,0);
		return -EINVAL;
	}

#if defined (__WINDOWS__)
	/* udata IS a pointer to wan_udp_hdr_t. copy data into user's buffer */
	wpabs_memcpy(udata, &wan_udp_pkt->wan_udp_hdr, sizeof(wan_udp_hdr_t));
#else
	if (WAN_COPY_TO_USER(
			udata,
			&wan_udp_pkt->wan_udp_hdr,
			sizeof(wan_udp_hdr_t))){
		wan_atomic_set(&chan->udp_pkt_len,0);
		return -EFAULT;
	}
#endif

	wan_atomic_set(&chan->udp_pkt_len,0);

	return 0;
}

static int xilinx_driver_ctrl(void *chan_ptr, int cmd, wanpipe_api_cmd_t *api_cmd)
{
	private_area_t 	*chan = (private_area_t *)chan_ptr;
	int		err = 0;

	if (!chan) {
		return -ENODEV;
	}

	api_cmd->result=0;

	switch (cmd)
	{

	case WP_API_CMD_SET_TX_Q_SIZE:
		if (api_cmd->tx_queue_sz) { 
			chan->max_tx_bufs = (u16)api_cmd->tx_queue_sz;
		} else {
			err=-EINVAL;
		}
		break;
	case WP_API_CMD_GET_TX_Q_SIZE:
		 api_cmd->tx_queue_sz = chan->max_tx_bufs;
		break;

	case WP_API_CMD_GET_STATS:
		{
		wanpipe_chan_stats_t *wanpipe_chan_stats = (wanpipe_chan_stats_t*)&api_cmd->stats;
/*		chan->chan_stats.max_tx_queue_length = (u8)chan->max_tx_bufs;
		chan->chan_stats.max_rx_queue_length = (u8)chan->dma_per_ch;*/

		memset(wanpipe_chan_stats, 0x00, sizeof(wanpipe_chan_stats_t));

		wanpipe_chan_stats->rx_bytes = chan->opstats.Data_bytes_Rx_count;
		wanpipe_chan_stats->rx_packets = chan->opstats.Data_frames_Rx_count;

		wanpipe_chan_stats->tx_bytes = chan->opstats.Data_bytes_Tx_count;;
		wanpipe_chan_stats->tx_packets = chan->opstats.Data_frames_Tx_count;
		}
		break;

	case WP_API_CMD_RESET_STATS:
		memset(&chan->opstats,0,sizeof(chan->opstats));
		memset(&chan->common.if_stats,0,sizeof(struct net_device_stats));
		break;

	case WP_API_CMD_DRIVER_VERSION:
		{
		wan_driver_version_t *drv_ver=(wan_driver_version_t*)&api_cmd->data[0];
		drv_ver->major=WANPIPE_VERSION_MAJOR;
		drv_ver->minor=WANPIPE_VERSION_MINOR;
		drv_ver->minor1=WANPIPE_VERSION_MINOR1;
		drv_ver->minor2=WANPIPE_VERSION_MINOR2;
		api_cmd->data_len=sizeof(wan_driver_version_t);
		}
		break;

	case WP_API_CMD_FIRMWARE_VERSION:
/*		api_cmd->data[0] = card->u.aft.firm_ver;*/
		api_cmd->data_len=1;
		break;

	case WP_API_CMD_CPLD_VERSION:
		api_cmd->data[0] = 0;
		api_cmd->data_len=1;
		break;
	
	case WP_API_CMD_GEN_FIFO_ERR_RX:
	case WP_API_CMD_GEN_FIFO_ERR_TX:
/*		card->wp_debug_gen_fifo_err=1; */
        break;

/*	case WP_API_CMD_START_CHAN_SEQ_DEBUG:
		DEBUG_EVENT("%s: Span %i channel sequence deugging enabled !\n",
				card->devname, card->tdmv_conf.span_no);
		card->wp_debug_gen_fifo_err=1;*/
        break;

/*	case WP_API_CMD_STOP_CHAN_SEQ_DEBUG:
		DEBUG_EVENT("%s: Span %i channel sequence deugging disabled !\n",
				card->devname, card->tdmv_conf.span_no);
		card->wp_debug_gen_fifo_err=0;*/
        break;

	case WP_API_CMD_SET_IDLE_FLAG:
		if(chan->tx_idle_skb){
					
			chan->idle_flag = (unsigned char)api_cmd->idle_flag;

			memset(wan_skb_data(chan->tx_idle_skb), chan->idle_flag, wan_skb_len(chan->tx_idle_skb));

		}else{
			DEBUG_EVENT("%s: Error: WP_API_CMD_SET_IDLE_FLAG: tx_idle_skb is NULL!\n",
				chan->if_name);
		}
		break;

	default:
		api_cmd->result=SANG_STATUS_OPTION_NOT_SUPPORTED;
		err=-EOPNOTSUPP;
		break;
	}

	return err;
}

static int aft_event_ctrl(void *chan_ptr, wan_event_ctrl_t *event_ctrl)
{
	/* There are no events for A101/2 cards */
	return -EINVAL;
}

int xilinx_tdmapi_event_init(wanpipe_tdm_api_dev_t *wp_tdm_api_dev)
{
#if defined(AFT_TDM_API_SUPPORT)
	wp_tdm_api_dev->read_rbs_bits		= aft_read_rbs_bits;
	wp_tdm_api_dev->write_rbs_bits		= aft_write_rbs_bits;
	wp_tdm_api_dev->event_ctrl			= aft_event_ctrl;
	wp_tdm_api_dev->write_hdlc_frame	= aft_write_hdlc_frame;
	wp_tdm_api_dev->pipemon				= xilinx_wan_user_process_udp_mgmt_pkt;
	wp_tdm_api_dev->driver_ctrl			= xilinx_driver_ctrl;
#endif
	return 0;
}

static int aft_tdm_api_init(sdla_t *card, private_area_t *chan, int logic_ch, wanif_conf_t *conf)
{
#ifdef AFT_TDM_API_SUPPORT
	int err=0;
	wanpipe_tdm_api_dev_t *wp_tdm_api_dev = &chan->wp_tdm_api_dev_idx[logic_ch];

	DEBUG_TEST("%s(): logic_ch: %d\n", __FUNCTION__, logic_ch);

# ifdef __WINDOWS__
	chan->tdmv_chan = logic_ch;
# endif

	if (chan->common.usedby != TDM_VOICE_API &&
		chan->common.usedby != TDM_VOICE_DCHAN 
# ifdef __WINDOWS__		
		&& chan->common.usedby != API
# endif
		) {
		DEBUG_TEST("%s()\n", __FUNCTION__);
		return 0;
	}

	if (chan->tdmv_zaptel_cfg) {
		DEBUG_TEST("%s()\n", __FUNCTION__);
		return 0;
	}

	/* Initilaize TDM API Parameters */
	wp_tdm_api_dev->chan = chan;
	wp_tdm_api_dev->card = card;
	wan_spin_lock_init(&wp_tdm_api_dev->lock, "wan_tdmapi_lock");
	strncpy(wp_tdm_api_dev->name,chan->if_name,WAN_IFNAME_SZ);

	if (conf->hdlc_streaming) {
		wp_tdm_api_dev->hdlc_framing=1;
	}

	wp_tdm_api_dev->event_ctrl	= aft_event_ctrl;
	wp_tdm_api_dev->read_rbs_bits = aft_read_rbs_bits;
	wp_tdm_api_dev->write_rbs_bits = aft_write_rbs_bits;
	wp_tdm_api_dev->write_hdlc_frame = aft_write_hdlc_frame;

	xilinx_tdmapi_event_init(wp_tdm_api_dev);

	wp_tdm_api_dev->cfg.rx_disable = 0;
	wp_tdm_api_dev->cfg.tx_disable = 0;

	if (IS_T1_CARD(card)) {
		wp_tdm_api_dev->cfg.hw_tdm_coding=WP_MULAW;
	}else{
		wp_tdm_api_dev->cfg.hw_tdm_coding=WP_ALAW;
	}

	wp_tdm_api_dev->cfg.idle_flag = conf->u.aft.idle_flag;
	wp_tdm_api_dev->cfg.rbs_tx_bits = conf->u.aft.rbs_cas_idle;

	wp_tdm_api_dev->tdm_span = card->tdmv_conf.span_no;
	wp_tdm_api_dev->tdm_chan = logic_ch+1;

	if (IS_T1_CARD(card)){
		/* Convert active_ch bit map to user */
		wp_tdm_api_dev->active_ch = conf->active_ch << 1;
	}else{
		wp_tdm_api_dev->active_ch = conf->active_ch;
	}

	wp_tdm_api_dev->cfg.tx_queue_sz = 20;

	wp_tdm_api_dev->operation_mode	= WP_TDM_OPMODE_SPAN;
	wp_tdm_api_dev->cfg.hw_mtu_mru = chan->mtu;
	wp_tdm_api_dev->cfg.usr_period = 20; //chan->tdm_api_period;
	wp_tdm_api_dev->cfg.usr_mtu_mru = chan->mtu;

	/* Overwite the chan number based on group number */	
	/*wp_tdm_api_dev->tdm_chan = (u8)chan->if_cnt;*/

	err=wanpipe_tdm_api_reg(wp_tdm_api_dev);
	if (err){
		DEBUG_TEST("%s()\n", __FUNCTION__);
		return err;
	}

	wan_set_bit(0,&wp_tdm_api_dev->init);
	DEBUG_TEST("%s(): returning err: %d\n", __FUNCTION__, err);
	return err;
#else
	DEBUG_EVENT("%s: TDM API support not compiled in\n",
		card->devname);
	return -EINVAL;
#endif
}

static int aft_tdm_api_free(sdla_t *card, private_area_t *chan, int logic_ch)
{
#ifdef AFT_TDM_API_SUPPORT
	wanpipe_tdm_api_dev_t *wp_tdm_api_dev = &chan->wp_tdm_api_dev_idx[logic_ch];
	int err=0;

	if (wan_test_bit(0,&wp_tdm_api_dev->init)){
		wan_clear_bit(0,&wp_tdm_api_dev->init);
		err=wanpipe_tdm_api_unreg(wp_tdm_api_dev);
		if (err){
			wan_set_bit(0,&wp_tdm_api_dev->init);
			return err;
		}
	}
#endif
	return 0;
}


static int aft_tdm_api_init_channelized(sdla_t *card, private_area_t *chan, wanif_conf_t *conf)
{
#ifdef AFT_TDM_API_SUPPORT
	int i;
	int err=-EINVAL;
	u32 active_ch=conf->active_ch;

	if (IS_E1_CARD(card)){
		active_ch=active_ch>>1;
	}

	chan->tdmapi_timeslots=active_ch;

	for (i=0;i<card->u.aft.num_of_time_slots;i++) {
		if (wan_test_bit(i,&chan->tdmapi_timeslots)){
			err=aft_tdm_api_init(card,chan,i,conf);
			if (err){
				break;
			}
		}
	}
	return err;
#else
	DEBUG_EVENT("%s: TDM API support not compiled in\n",
		card->devname);
	return -EINVAL;
#endif
}

#ifdef AFT_TDM_API_SUPPORT
static int aft_tdm_api_free_channelized(sdla_t *card, private_area_t *chan)
{
	int i;
	int err=0;
	for (i=0;i<card->u.aft.num_of_time_slots;i++) {
		if (wan_test_bit(i,&chan->tdmapi_timeslots)){
			err=aft_tdm_api_free(card,chan,i);
			if (err){
				return err;
			}
		}
	}
	return 0;
}
#endif
#endif/* #if defined(__LINUX__) || defined(__WINDOWS__) */

static void wanpipe_wake_stack(private_area_t* chan)
{
	DEBUG_TEST("%s()\n", __FUNCTION__);

	WAN_NETIF_WAKE_QUEUE(chan->common.dev);

#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	if (chan->common.usedby == API){
# if defined(__LINUX__) || defined(__WINDOWS__)
		wan_wakeup_api(chan);
# endif
	}else if (chan->common.usedby == STACK){
		wanpipe_lip_kick(chan,0);
	}
#endif
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
static int new_if_private (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf, int channelized)
{
	sdla_t* card = wandev->priv;
	private_area_t* chan;
	int err = 0;

	DEBUG_EVENT( "%s: Configuring Interface: %s\n",
		card->devname, wan_netif_name(dev));

	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)){
		DEBUG_EVENT( "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	/* allocate and initialize private data */
	chan = wan_kmalloc(sizeof(private_area_t));
	if(chan == NULL){
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}
	memset(chan, 0, sizeof(private_area_t));

	DEBUG_UDP("%s(): dev->name: %s, chan ptr: 0x%p\n",
		__FUNCTION__, dev->name, chan);

	chan->first_time_slot=-1;

	strncpy(chan->if_name, wan_netif_name(dev), WAN_IFNAME_SZ);
	memcpy(&chan->cfg,&conf->u.aft,sizeof(chan->cfg));

	if (channelized){
		chan->channelized_cfg=1;
		if (wan_netif_priv(dev)){
#if 1
			private_area_t *cptr;
			for (cptr=wan_netif_priv(dev);cptr->next!=NULL;cptr=cptr->next);
			cptr->next=chan;
			chan->next=NULL;
#else
			chan->next = wan_netif_priv(dev);
			wan_netif_set_priv(dev, chan);
#endif
		}else{
			wan_netif_set_priv(dev, chan);
		}
	}else{
		chan->channelized_cfg=0;
		wan_netif_set_priv(dev, chan);
	}


	chan->common.card = card;
	chan->true_if_encoding=conf->true_if_encoding;

	WAN_IFQ_INIT(&chan->wp_tx_free_list, 0);
	WAN_IFQ_INIT(&chan->wp_tx_pending_list,0);
	WAN_IFQ_INIT(&chan->wp_tx_complete_list,0);

	WAN_IFQ_INIT(&chan->wp_rx_free_list,0);
	WAN_IFQ_INIT(&chan->wp_rx_complete_list,0);

	WAN_IFQ_INIT(&chan->wp_dealloc_list,0);

	wan_trace_info_init(&chan->trace_info,MAX_TRACE_QUEUE);

	/* Initialize the socket binding information
	* These hooks are used by the API sockets to
	* bind into the network interface */

	WAN_TASKLET_INIT((&chan->common.bh_task), 0, wp_bh, chan);
	chan->common.dev = dev;
	chan->tracing_enabled = 0;
	chan->route_removed = 0;



	/* Setup interface as:
	*    WANPIPE 	  = IP over Protocol (Firmware)
	*    API     	  = Raw Socket access to Protocol (Firmware)
	*    BRIDGE  	  = Ethernet over Protocol, no ip info
	*    BRIDGE_NODE = Ethernet over Protocol, with ip info
	*/

	chan->mtu = card->wandev.mtu;
	if (conf->u.aft.mtu){
		chan->mtu=conf->u.aft.mtu;
	}

	if (chan->mtu > MAX_WP_PRI_MTU ||
		chan->mtu < MIN_WP_PRI_MTU){
			DEBUG_EVENT("%s: Error Invalid %s MTU %d (Min=%d, Max=%d)\n",
				card->devname,chan->if_name,chan->mtu,
				MIN_WP_PRI_MTU,MAX_WP_PRI_MTU);
			err= -EINVAL;
			goto new_if_error;
	}

	chan->mru = card->u.aft.cfg.mru;
	if (conf->u.aft.mru){
		chan->mru = conf->u.aft.mru;
	}

	if (chan->mru > MAX_WP_PRI_MTU ||
		chan->mru < MIN_WP_PRI_MTU){
			DEBUG_EVENT("%s: Error Invalid %s MRU %d (Min=%d, Max=%d)\n",
				card->devname,chan->if_name,chan->mru,
				MIN_WP_PRI_MTU,MAX_WP_PRI_MTU);

			err= -EINVAL;
			goto new_if_error;
	}

	DEBUG_EVENT("%s:    UsedBy        :%s\n",
		card->devname,
		conf->usedby);


	if(strcmp(conf->usedby, "WANPIPE") == 0) {

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
			wan_netif_set_priv(dev, chan);
			if ((err=protocol_init(card,dev,chan,conf)) != 0){
				wan_netif_set_priv(dev, NULL);
				goto new_if_error;
			}

			if (conf->ignore_dcd == WANOPT_YES || conf->ignore_cts == WANOPT_YES){
				DEBUG_EVENT("%s: Ignore modem changes DCD/CTS\n",
					card->devname);
				chan->ignore_modem=1;
			}else{
				DEBUG_EVENT("%s: Restart protocol on modem changes DCD/CTS\n",
					card->devname);
			}
			DEBUG_EVENT("\n");
		}

#if defined(__LINUX__) || defined(__WINDOWS__)

	}else if( strcmp(conf->usedby, "API") == 0) {

		chan->common.usedby = API;
		DEBUG_EVENT( "%s:%s: Running in %s mode\n",	wandev->name,chan->if_name, conf->usedby);

# ifdef __WINDOWS__
		chan->tdmv_zaptel_cfg=0;

		err=aft_tdm_api_init_channelized(card,chan,conf);/* this will call CDEV code to create the file descriptor */
		if (err){
			goto new_if_error;
		}
# else
		wan_reg_api(chan, dev, card->devname);
# endif
#endif

#if defined(__LINUX__)
	}else if (strcmp(conf->usedby, "BRIDGE") == 0) {
		chan->common.usedby = BRIDGE;
#endif

#if defined(__LINUX__)
	}else if (strcmp(conf->usedby, "BRIDGE_N") == 0) {
		chan->common.usedby = BRIDGE_NODE;
#endif

#if defined(__LINUX__)
	}else if (strcmp(conf->usedby, "TDM_VOICE_DCHAN") == 0) {

# ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
#  ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN
		int dchan=card->u.aft.tdmv_dchan;

		/* DCHAN must be decremented for both
		* T1 and E1, since from tdmv driver's
		* perspective all timeslots start from ZERO*/
		dchan--;
		chan->tdmv_chan=dchan;

		chan->common.usedby = TDM_VOICE_DCHAN;
		conf->hdlc_streaming=1;
		chan->mru=chan->mtu=1500;
		chan->tdmv_zaptel_cfg=1;
		card->u.aft.tdmv_zaptel_cfg=1;
#  else
		DEBUG_EVENT("%s: Error: TDMV_DCHAN Option not compiled into the driver!\n",
			card->devname);
		err=-EINVAL;
		goto new_if_error;
#  endif
# else
		DEBUG_EVENT("\n");
		DEBUG_EVENT("%s:%s: Error: TDM VOICE/DCHAN prot not compiled\n",
			card->devname,chan->if_name);
		DEBUG_EVENT("%s:%s:        during installation process!\n",
			card->devname,chan->if_name);
		err=-EINVAL;
		goto new_if_error;
# endif
#endif

	}else if (strcmp(conf->usedby, "TDM_VOICE") == 0) {

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		chan->common.usedby = TDM_VOICE;
		chan->tdmv_zaptel_cfg=1;
		card->u.aft.tdmv_zaptel_cfg=1;
#else
		DEBUG_EVENT("\n");
		DEBUG_EVENT("%s:%s: Error: TDM VOICE prot not compiled\n",
			card->devname,chan->if_name);
		DEBUG_EVENT("%s:%s:        during installation process!\n",
			card->devname,chan->if_name);
		err=-EINVAL;
		goto new_if_error;
#endif

#ifdef AFT_TDM_API_SUPPORT
	}else if (strcmp(conf->usedby, "TDM_VOICE_API") == 0) {

		int dchan=card->u.aft.tdmv_dchan;

		/* DCHAN must be decremented for both
		* T1 and E1, since from tdmv driver's
		* perspective all timeslots start from ZERO*/
		dchan--;
		chan->tdmv_chan=dchan;

		chan->common.usedby = TDM_VOICE_API;
		chan->cfg.data_mux=1;
		conf->hdlc_streaming=0;
		chan->tdmv_zaptel_cfg=0;

		err=aft_tdm_api_init_channelized(card,chan,conf);
		if (err){
			goto new_if_error;
		}

#endif

#ifdef AFT_TDM_API_SUPPORT
	}else if (strcmp(conf->usedby, "TDM_VOICE_DCHAN_API") == 0) {
		int dchan=card->u.aft.tdmv_dchan;

		/* DCHAN must be decremented for both
		* T1 and E1, since from tdmv driver's
		* perspective all timeslots start from ZERO*/
		dchan--;
		chan->tdmv_chan=dchan;

		chan->common.usedby = TDM_VOICE_DCHAN;
		conf->hdlc_streaming=1;
		chan->mru=chan->mtu=1500;
		chan->tdmv_zaptel_cfg=0;

		err=aft_tdm_api_init_channelized(card,chan,conf);
		if (err){
			goto new_if_error;
		}
#endif
	}else if (strcmp(conf->usedby, "STACK") == 0) {
		chan->common.usedby = STACK;
		if (chan->hdlc_eng){
			chan->mtu+=32;
			chan->mru+=32;
		}

	}else{
		DEBUG_EVENT( "%s:%s: Error: Invalid operation mode [%s]\n",
			card->devname,chan->if_name, conf->usedby);
		err=-EINVAL;
		goto new_if_error;
	}


	chan->time_slot_map=conf->active_ch;

	err=aft_tdmv_if_init(card,chan,conf);
	if (err){
		err=-EINVAL;
		goto new_if_error;
	}


	DEBUG_EVENT("%s:    MRU           :%d\n",
		card->devname,
		chan->mru);

	DEBUG_EVENT("%s:    MTU           :%d\n",
		card->devname,
		chan->mtu);


	xilinx_delay(1);
	chan->hdlc_eng = conf->hdlc_streaming;

	DEBUG_EVENT("%s:    HDLC Eng      :%s\n",
		card->devname,
		chan->hdlc_eng?"On":"Off (Transparent)");

	if (!chan->hdlc_eng){

		if (!wan_test_bit(0,&card->u.aft.tdmv_sync)){
			DEBUG_EVENT("%s:    Slot Sync     :Enabled\n",
				card->devname);
			wan_set_bit(0,&card->u.aft.tdmv_sync);
			wan_set_bit(0,&chan->tdmv_sync);
		}else{
			DEBUG_EVENT("%s:    Slot Sync     :Disabled\n",
				card->devname);
			wan_clear_bit(0,&chan->tdmv_sync);
		}

		if(conf->protocol == WANCONFIG_LIP_ATM ||
			conf->protocol == WANCONFIG_LIP_KATM){
				/* if ATM NO sync needed!! */
				DEBUG_EVENT("%s: Disabling Time Slot Sync for ATM.\n", chan->if_name);
				card->u.aft.tdmv_sync = 0;
				chan->tdmv_sync = 0;
		}

		if (chan->mtu&0x03){
			DEBUG_EVENT("%s:%s: Error, Transparent MTU must be word aligned!\n",
				card->devname,chan->if_name);
			err = -EINVAL;
			goto new_if_error;
		}
	}

	DEBUG_EVENT("%s:    Timeslot Map  :0x%08X\n",
		card->devname,
		chan->time_slot_map);

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC
	err=xilinx_dev_configure(card,chan);
	if (err){
		goto new_if_error;
	}

	/*Set the actual logic ch number of this chan
	*as the dchan. Due to HDLC security issue, the
	*HDLC channels are mapped on first TWO logic channels */
	if (chan->common.usedby == TDM_VOICE_DCHAN){
		card->u.aft.tdmv_dchan=chan->logic_ch_num+1;
	}

	xilinx_delay(1);
#endif


	chan->dma_mru = xilinx_valid_mtu(chan->mru+100);
	if (!chan->dma_mru){
		DEBUG_EVENT("%s:%s: Error invalid MTU %d  MRU %d\n",
			card->devname,
			chan->if_name,
			chan->dma_mru,card->u.aft.cfg.mru);
		err= -EINVAL;
		goto new_if_error;
	}


	if (!chan->hdlc_eng){
		unsigned char *buf;

		chan->max_idle_size=chan->mru;
		chan->idle_flag = conf->u.aft.idle_flag;

		DEBUG_EVENT("%s:    Idle Flag     :0x%02X\n",
			card->devname,
			chan->idle_flag);
		DEBUG_EVENT("%s:    Idle Buf Len  :%d\n",
			card->devname,
			chan->max_idle_size);

		DEBUG_EVENT("%s:    Data Mux      :%s\n",
			card->devname,
			chan->cfg.data_mux?"On":"Off");

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		if (chan->common.usedby == TDM_VOICE){
			chan->idle_flag = WAN_TDMV_IDLE_FLAG;
		}
#endif

		chan->tx_idle_skb = wan_skb_alloc(chan->dma_mru);
		if (!chan->tx_idle_skb){
			err=-ENOMEM;
			goto new_if_error;
		}


		if(conf->protocol != WANCONFIG_LIP_ATM &&
			conf->protocol != WANCONFIG_LIP_KATM){
				buf=wan_skb_put(chan->tx_idle_skb,chan->dma_mru);
				memset(buf,chan->idle_flag,chan->dma_mru);
				wan_skb_trim(chan->tx_idle_skb,0);
				wan_skb_put(chan->tx_idle_skb,chan->max_idle_size);
		}else{
			buf=wan_skb_put(chan->tx_idle_skb,chan->max_idle_size);
			chan->lip_atm = 1;
			/* if running below LIP ATM, transmit idle cells */
			if(init_atm_idle_buffer((unsigned char*)buf,
				wan_skb_len(chan->tx_idle_skb),
				chan->if_name,
				chan->cfg.data_mux)){

					wan_skb_free(chan->tx_idle_skb);
					chan->tx_idle_skb = NULL;
					return -EINVAL;
			}

			wan_skb_reverse(chan->tx_idle_skb);
		}
	}


	DEBUG_EVENT("\n");
	DEBUG_EVENT("%s:    DMA MRU       :%d\n",
		card->devname,
		chan->dma_mru);

	if (wan_test_bit(0,&chan->tdmv_sync)){
		if (chan->dma_mru%4){
			DEBUG_EVENT("%s:%s: Error invalid TDM_VOICE MTU %d  MRU %d\n",
				card->devname,
				chan->if_name,
				chan->dma_mru,card->u.aft.cfg.mru);
			err= -EINVAL;
			goto new_if_error;
		}
	}

	DEBUG_EVENT("%s:    RX DMA Per Ch :%d\n",
		card->devname,
		card->u.aft.cfg.dma_per_ch);



	err=aft_alloc_rx_dma_buff(card, chan, card->u.aft.cfg.dma_per_ch,0);
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

	DEBUG_EVENT( "%s:    Net Gateway   :%s\n",
		card->devname,
		conf->gateway?"Yes":"No");

	chan->gateway = conf->gateway;

	/* Get Multicast Information from the user
	* FIXME: This option is not clearly defined
	*/
	chan->mc = conf->mc;
	chan->max_tx_bufs = MAX_TX_BUF;


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


#if defined(__LINUX__) || defined(__WINDOWS__)
	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);	
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&wanpipe_xilinx_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&wanpipe_xilinx_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&wanpipe_xilinx_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&wanpipe_xilinx_ifstats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&wanpipe_xilinx_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,wanpipe_xilinx_ioctl);


# ifdef CONFIG_PRODUCT_WANPIPE_GENERIC
	if_init(dev);
# endif
#else
	if_init(dev);
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	chan->common.is_netdev = 1;
	chan->common.iface.open      = &wanpipe_xilinx_open;
	chan->common.iface.close     = &wanpipe_xilinx_close;
	chan->common.iface.output    = &wan_aft_output;
	chan->common.iface.ioctl     = &wanpipe_xilinx_ioctl;
	chan->common.iface.tx_timeout= &wanpipe_xilinx_tx_timeout;
	if (wan_iface.attach){
		wan_iface.attach(dev, NULL, chan->common.is_netdev);
	}else{
		DEBUG_EVENT("%s: Failed to attach interface %s!\n",
			card->devname, wan_netif_name(dev));
		wan_netif_set_priv(dev, NULL);
		err = -EINVAL;
		goto new_if_error;
	}
	wan_netif_set_mtu(dev, chan->mtu);
#endif
	/* Increment the number of network interfaces
	** configured on this card. */
	wan_atomic_inc(&card->wandev.if_cnt);

	set_chan_state(card, dev, WAN_DISCONNECTED);	

	DEBUG_EVENT( "\n");

	return 0;

new_if_error:
	return err;
}


static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	int err=-EINVAL;
	sdla_t *card=wandev->priv;

	wan_netif_set_priv(dev, NULL);

	if (IS_E1_CARD(card)){
		DEBUG_TEST("%s: Time Slot Orig 0x%lX  Shifted 0x%lX DCHAN=0x%08X\n",
			card->devname,
			conf->active_ch,
			conf->active_ch<<1,
			card->tdmv_conf.dchan);
		conf->active_ch = conf->active_ch << 1;
		wan_clear_bit(0,&conf->active_ch);
	}

	if (strcmp(conf->usedby, "TDM_VOICE") == 0 ) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
		if (card->tdmv_conf.span_no){
			/* Initialize TDMV interface function */
			err = wp_tdmv_te1_init(&card->tdmv_iface);
			if (err){
				DEBUG_EVENT("%s: Error: Failed to initialize tdmv functions!\n",
					card->devname);
				return -EINVAL;
			}

			WAN_TDMV_CALL(create, (card, &card->tdmv_conf), err);
			if (err){
				DEBUG_EVENT("%s: Error: Failed to create tdmv span!\n",
					card->devname);
				return err;
			}
		}
#else
		DEBUG_EVENT("\n");
		DEBUG_EVENT("%s: Error: TDM VOICE prot not compiled\n",
			card->devname);
		DEBUG_EVENT("%s:        during installation process!\n",
			card->devname);
		return -EINVAL;
#endif
	}

	if (strcmp(conf->usedby, "TDM_VOICE") == 0 ||
		strcmp(conf->usedby, "TDM_VOICE_API") == 0){

			int dchan=0;
			int dchan_found=0;
			int i;

			for (i=card->u.aft.num_of_time_slots-1;i>=0;i--){
				if (wan_test_bit(i,&card->tdmv_conf.dchan)){
					dchan_found=1;
					card->u.aft.tdmv_dchan=i;
					dchan=i;
					break;
				}
			}

			if (dchan_found){
				if (IS_T1_CARD(card)) {
					card->u.aft.tdmv_dchan++;
				}
				wan_clear_bit(dchan,&conf->active_ch);
			}

			err=new_if_private(wandev,dev,conf,1);
			if (!err){
				if (card->tdmv_conf.dchan){

					conf->active_ch=0;
					if (strcmp(conf->usedby, "TDM_VOICE") == 0) {
						sprintf(conf->usedby,"TDM_VOICE_DCHAN");
					} else {
						sprintf(conf->usedby,"TDM_VOICE_DCHAN_API");
					}
					wan_set_bit(dchan,&conf->active_ch);

					err=new_if_private(wandev,dev,conf,1);
					if (err){
						return err;
					}
				}
			}

	}else{
		err=new_if_private(wandev,dev,conf,0);
	}

	if (err && wan_netif_priv(dev)){
		del_if(wandev,dev);
		if (wan_netif_priv(dev)){
			wan_free(wan_netif_priv(dev));
			wan_netif_set_priv(dev, NULL);
		}
	}

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
static int del_if_private (wan_device_t* wandev, netdevice_t* dev)
{
	private_area_t* 	chan = wan_netif_priv(dev);
	sdla_t*			card = (sdla_t*)chan->common.card;
	netskb_t 		*skb;
	wan_smp_flag_t 		flags;

	DEBUG_TEST("%s()\n", __FUNCTION__);

	if (wan_test_bit(0,&chan->tdmv_sync)){
		wan_clear_bit(0,&card->u.aft.tdmv_sync);
		wan_clear_bit(0,&chan->tdmv_sync);
	}


#ifdef AFT_TDM_API_SUPPORT
	if (aft_tdm_api_free_channelized(card,chan)){
		DEBUG_EVENT(
			"%s: Error: Failed to del iface: TDM API Device in use!\n",
			chan->if_name);
		return -EBUSY;
	}
#endif
	
	DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC
	xilinx_dev_unconfigure(card,chan);
#endif
	DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);

	WAN_TASKLET_KILL((&chan->common.bh_task));

	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

#ifdef AFT_TDM_API_SUPPORT
	aft_tdm_api_free_channelized(card,chan);
#endif

	aft_tdmv_if_free(card,chan);

	protocol_shutdown(card,dev);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	if (wan_iface.detach){
		wan_iface.detach(dev, chan->common.is_netdev);
	}
#endif

#ifndef __WINDOWS__
	/* The communications must be already disabled, before interfaces are deleted,
	* there is no need to use IRQ lock. */
	wan_spin_lock_irq(&card->wandev.lock,&flags);
#endif
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

	WAN_IFQ_PURGE(&chan->wp_dealloc_list);
	WAN_IFQ_DESTROY(&chan->wp_dealloc_list);

	if (chan->tx_dma_addr && chan->tx_dma_len){
		aft_unmap_tx_dma(card,chan);
	}

	if (chan->tx_dma_skb){
		DEBUG_TEST("freeing tx dma skb\n");
		wan_skb_free(chan->tx_dma_skb);
		chan->tx_dma_skb=NULL;
	}

	if (chan->tx_idle_skb){
		DEBUG_TEST("freeing idle tx dma skb\n");
		wan_skb_free(chan->tx_idle_skb);
		chan->tx_idle_skb=NULL;
	}

	if (chan->rx_dma_skb){
		wp_rx_element_t *rx_el;
		netskb_t *skb=chan->rx_dma_skb;

		chan->rx_dma_skb=NULL;
		rx_el=(wp_rx_element_t *)wan_skb_data(skb);

		card->hw_iface.pci_unmap_dma(card->hw,
			rx_el->dma_addr,
			chan->dma_mru,
			PCI_DMA_FROMDEVICE);

		wan_skb_free(skb);
	}

	if (chan->tx_realign_buf){
		wan_free(chan->tx_realign_buf);
		chan->tx_realign_buf=NULL;
	}

	chan->logic_ch_num=-1;
#ifndef __WINDOWS__
	wan_spin_unlock_irq(&card->wandev.lock,&flags);
#endif
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

static int del_if (wan_device_t* wandev,  netdevice_t* dev)
{
	private_area_t*	chan=wan_netif_priv(dev);
	int	err;

	DEBUG_TEST("%s()\n", __FUNCTION__);

	if (!chan){
		return 0;
	}

	if (chan->channelized_cfg){
		sdla_t *card=chan->common.card;

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		if (chan->tdmv_zaptel_cfg) {
			WAN_TDMV_CALL(running, (card), err);
			if (err){
				return -EBUSY;
			}
		}
#endif
		while(chan){
			err=del_if_private(wandev,dev);
			if (err){
				return err;
			}
			chan=chan->next;
			if (chan){
				wan_free(wan_netif_priv(dev));
				wan_netif_set_priv(dev, chan);
			}else{
				/* Leave the last chan dev
				* in dev->priv.  It will get
				* deallocated normally */
				break;
			}
		}

		aft_tdmv_free(card);

	}else{
		err = del_if_private(wandev,dev);
		if (err){
			return err;
		}
#if defined(__WINDOWS__)/* FIXME: make the same on Linux? */
		if (chan){
			wan_free(chan);
			wan_netif_set_priv(dev, NULL);
			chan = NULL;
		}
#endif
	}
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
static int if_init (netdevice_t* dev)
{
	private_area_t* chan = wan_netif_priv(dev);
#if defined(__LINUX__) || defined(__WINDOWS__)
	sdla_t*		card = (sdla_t*)chan->common.card;
	wan_device_t* 	wandev = &card->wandev;
#endif

	/* Initialize device driver entry points */
#if defined(__LINUX__) || defined(__WINDOWS__)
# ifndef CONFIG_PRODUCT_WANPIPE_GENERIC
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&wanpipe_xilinx_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&wanpipe_xilinx_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&wanpipe_xilinx_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&wanpipe_xilinx_ifstats);

#  if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (chan->common.usedby == TDM_VOICE ||
		chan->common.usedby == TDM_VOICE_DCHAN ||
		chan->common.usedby == TDM_VOICE_API) {
		WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,NULL);
	} else {
		WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&wanpipe_xilinx_tx_timeout);
	}
	dev->watchdog_timeo	= 2*HZ;
#  endif
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,wanpipe_xilinx_ioctl);
# endif

	if (chan->common.usedby == BRIDGE ||
		chan->common.usedby == BRIDGE_NODE){
#ifndef __WINDOWS__
			/* Setup the interface for Bridging */
			int hw_addr=0;
			ether_setup(dev);

			/* Use a random number to generate the MAC address */
			memcpy(dev->dev_addr, "\xFE\xFC\x00\x00\x00\x00", 6);
			get_random_bytes(&hw_addr, sizeof(hw_addr));
			*(int *)(dev->dev_addr + 2) += hw_addr;
#endif
	}else{

		if (chan->common.protocol != WANCONFIG_GENERIC){
#ifndef __WINDOWS__
			dev->flags     |= IFF_POINTOPOINT;
			dev->flags     |= IFF_NOARP;
			dev->type	= ARPHRD_PPP;
#endif
			dev->mtu		= chan->mtu;

			if (chan->common.usedby == API){
				dev->mtu+=sizeof(wp_api_hdr_t);
			}

			dev->hard_header_len	= 0;
#ifndef __WINDOWS__
			/* Enable Mulitcasting if user selected */
			if (chan->mc == WANOPT_YES){
				dev->flags 	|= IFF_MULTICAST;
			}

			if (chan->true_if_encoding){
				DEBUG_EVENT("%s: Setting IF Type to Broadcast\n",chan->if_name);
				dev->type	= ARPHRD_PPP; /* This breaks the tcpdump */
				dev->flags     &= ~IFF_POINTOPOINT;
				dev->flags     |= IFF_BROADCAST;
			}else{
				dev->type	= ARPHRD_PPP;
			}
#endif
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
#else
	dev->if_mtu	= chan->mtu;
#if 0
	DEBUG_EVENT("%s: Initialize network interface...\n",
		wan_netif_name(dev));
	dev->if_output	= NULL;
	dev->if_start	= NULL;	/*&wanpipe_xilinx_start;*/
	dev->if_ioctl	= NULL; /* &wplip_ioctl; */

	/* Initialize media-specific parameters */
	dev->if_flags	|= IFF_POINTOPOINT;
	dev->if_flags	|= IFF_NOARP;

	dev->if_mtu	= 1500;
	WAN_IFQ_SET_MAXLEN(&dev->if_snd, 100);
	dev->if_snd.ifq_len = 0;
	dev->if_type	= IFT_PPP;
#endif
#endif
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
static int wanpipe_xilinx_open (netdevice_t* dev)
{
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = (sdla_t*)chan->common.card;
	wan_smp_flag_t flags;
	int err = 0;

#ifdef CONFIG_PRODUCT_WANPIPE_GENERIC
	card->isr = &wp_xilinx_isr;
	err=xilinx_chip_configure(card);
	if (err){
		xilinx_chip_unconfigure(card);
		return -EINVAL;
	}

	err=xilinx_dev_configure(card, chan);
	if (err){
		xilinx_chip_unconfigure(card);
		return -EINVAL;
	}
	xilinx_delay(1);
#endif
	/* Only one open per interface is allowed */
#if defined(__LINUX__)
	if (open_dev_check(dev))
		return -EBUSY;
#endif

	WAN_NETIF_START_QUEUE(dev);
	WAN_NETIF_CARRIER_OFF(dev);

	wan_spin_lock_irq(&card->wandev.lock,&flags);

	err=aft_dev_open(card,chan);
	if (err) {
		wan_spin_unlock_irq(&card->wandev.lock,&flags);
		DEBUG_EVENT("%s: Error failed to configure interface!\n",
			chan->if_name);
		return err;
	}

	if (wan_test_bit(0,&chan->tdmv_sync) &&
		(card->wandev.state == WAN_CONNECTED ||
		card->tdmv_conf.span_no)){
			/* At this point we are out of sync. The
			* DMA was enabled while interface was down.
			* We must do a FULL recovery */
			DEBUG_EVENT("%s: Interface resynching!\n",
				chan->if_name);

			if (card->wandev.state == WAN_CONNECTED){
				disable_data_error_intr(card,LINK_DOWN);
				enable_data_error_intr(card);

			}else if (card->tdmv_conf.span_no) {
				/* The A101/2 Card must supply clock to
				* zaptel regardless of state. Thus fake
				* the front end connected state */
				disable_data_error_intr(card,LINK_DOWN);
				card->fe.fe_status = FE_CONNECTED;
				handle_front_end_state(card);

				/* This will set the LEDs to RED and
				* update the card state */
				card->fe.fe_status = FE_DISCONNECTED;
				handle_front_end_state(card);
			}
	}else if (!chan->hdlc_eng && chan->common.usedby == API
		&& card->wandev.state == WAN_CONNECTED){
			disable_data_error_intr(card,LINK_DOWN);
			enable_data_error_intr(card);
	}

	if (card->wandev.state == WAN_CONNECTED){
		/* If Front End is connected already set interface
		* state to Connected too */
		set_chan_state(card, dev, WAN_CONNECTED);
		WAN_NETIF_WAKE_QUEUE(dev);
		WAN_NETIF_CARRIER_ON(dev);
		wanpipe_wake_stack(chan);
	}

	wan_spin_unlock_irq(&card->wandev.lock,&flags);

	/* Increment the module usage count */
	wanpipe_open(card);

	protocol_start(card,dev);

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
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t* card = (sdla_t*)chan->common.card;


#ifdef CONFIG_PRODUCT_WANPIPE_GENERIC
	xilinx_dev_unconfigure(card, chan);
	xilinx_chip_unconfigure(card);
#endif
	WAN_NETIF_STOP_QUEUE(dev);	/* stop_net_queue(dev); */

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	protocol_stop(card,dev);

	aft_dev_close(card,chan);

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
	wan_smp_flag_t	flags;

	DEBUG_TEST("%s()\n", __FUNCTION__);

	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		wan_smp_flag_t smp_flags,smp_flags1;

		card->hw_iface.hw_lock(card->hw,&smp_flags1);
		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		card->hw_iface.hw_unlock(card->hw,&smp_flags1);
	}

	wan_spin_lock_irq(&card->wandev.lock,&flags);

	/* Disable DMA ENGINE before we perform
	* core reset.  Otherwise, we will receive
	* rx fifo errors on subsequent resetart. */
	disable_data_error_intr(card,DEVICE_DOWN);

	wan_set_bit(CARD_DOWN,&card->wandev.critical);

	wan_spin_unlock_irq(&card->wandev.lock,&flags);


	/* Release front end resources only after all interrupts and tasks have been shut down */
	if (card->wandev.fe_iface.post_unconfig) {
		card->wandev.fe_iface.post_unconfig(&card->fe);
	}     

	WP_DELAY(10);

	xilinx_chip_unconfigure(card);

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
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = (sdla_t*)chan->common.card;
	unsigned long dma_descr;
	u32 reg_lo, reg_hi;

	/* If our device stays busy for at least 5 seconds then we will
	* kick start the device by making dev->tbusy = 0.  We expect
	* that our device never stays busy more than 5 seconds. So this
	* is only used as a last resort.
	*/

	++chan->if_stats.collisions;

	DEBUG_EVENT( "%s: Transmit timed out on %s\n", card->devname,wan_netif_name(dev));
	DEBUG_EVENT("%s: TxStatus=0x%X  DMAADDR=0x%X  DMALEN=%d \n",
		chan->if_name,
		chan->dma_status,
		chan->tx_dma_addr,
		chan->tx_dma_len);

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_LO;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg_lo);

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg_hi);

	DEBUG_EVENT("%s:%s: TX Error: Lch=%li DmaLO: 0x%08X DmaHI: 0x%08X\n",
		card->devname,chan->if_name,chan->logic_ch_num,
		reg_lo, reg_hi);

	wan_clear_bit(TX_BUSY,&chan->dma_status);
	wan_netif_set_ticks(dev, SYSTEM_TICKS);

	xilinx_tx_fifo_under_recover(card,chan);

	wanpipe_wake_stack(chan);

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
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static int
wan_aft_output(netdevice_t *dev, netskb_t *skb, struct sockaddr *dst, struct rtentry *rt0)
#else
static int wanpipe_xilinx_send (netskb_t* skb, netdevice_t* dev)
#endif
{
	private_area_t *chan = wan_netif_priv(dev);
	sdla_t *card;

	WAN_ASSERT(chan == NULL);
	WAN_ASSERT(chan->common.card == NULL);
	card = (sdla_t*)chan->common.card;

#if defined(__WINDOWS__)
	DEBUG_TEST("%s(): IRQL: %s, card->fe.fe_status: %s\n", __FUNCTION__,
		decode_irql(KeGetCurrentIrql()), FE_STATUS_DECODE(card->fe.fe_status));
#endif

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
		DEBUG_TEST("%s(): line: %d: chan ptr: 0x%p, chan->common.state: %d\n", __FUNCTION__, __LINE__, chan, chan->common.state);
		++chan->if_stats.tx_carrier_errors;
		WAN_NETIF_STOP_QUEUE(dev);
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		return 1;
	} else if (!WAN_NETIF_UP(dev)) {
		DEBUG_TEST("%s(): line: %d\n", __FUNCTION__, __LINE__);
		++chan->if_stats.tx_carrier_errors;
		WAN_NETIF_START_QUEUE(dev);
		wan_skb_free(skb);
		wan_netif_set_ticks(dev, SYSTEM_TICKS);
		return 0;
	}else {

		if (chan->common.usedby == TDM_VOICE ||
			chan->common.usedby == TDM_VOICE_DCHAN){

				if (!card->u.aft.tdmv_dchan || card->u.aft.tdmv_dchan>32){
					wan_skb_free(skb);
					WAN_NETIF_START_QUEUE(dev);
					goto if_send_exit_crit;
				}

				chan=(private_area_t*)card->u.aft.dev_to_ch_map[card->u.aft.tdmv_dchan-1];
				if (!chan){
					wan_skb_free(skb);
					WAN_NETIF_START_QUEUE(dev);
					goto if_send_exit_crit;
				}

				if (!chan->hdlc_eng){
					wan_skb_free(skb);
					WAN_NETIF_START_QUEUE(dev);
					goto if_send_exit_crit;
				}


		}else if (chan->common.usedby == API){
			if (sizeof(wp_api_hdr_t) >= wan_skb_len(skb)){
				wan_skb_free(skb);
				++chan->if_stats.tx_dropped;
				goto if_send_exit_crit;
			}
			wan_skb_pull(skb,sizeof(wp_api_hdr_t));

		}

		if (chan->max_tx_bufs == 1) {
			wan_smp_flag_t smp_flags;
			DEBUG_TEST("%s(): line: %d\n", __FUNCTION__, __LINE__);

			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			if (wan_test_bit(TX_BUSY,&chan->dma_status)){
				WAN_NETIF_STOP_QUEUE(dev);
				wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
				return 1;
			}
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
			goto xilinx_tx_dma;

		} else if (wan_skb_queue_len(&chan->wp_tx_pending_list) > chan->max_tx_bufs){
			wan_smp_flag_t smp_flags;
			DEBUG_TEST("%s(): line: %d\n", __FUNCTION__, __LINE__);

			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			WAN_NETIF_STOP_QUEUE(dev);      /*stop_net_queue(dev);*/
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
			if (chan->hdlc_eng) {
				xilinx_dma_tx(card,chan);
			}
			return 1;
		}else{
xilinx_tx_dma:
			wan_skb_unlink(skb);

#if defined (__LINUX__)
#if 0
			if (is_tdm_api(chan,&chan->wp_tdm_api_cfg)){
				/* We must do this here, since it guarantees us that
				* the packet will be queued up. However, we should
				* disable the lock since this process could be computer
				* intensive */
				int err=wanpipe_tdm_api_tx(&chan->wp_tdm_api_cfg,&skb);
				if (err){
					++chan->if_stats.tx_errors;
					wan_skb_free(skb);
					WAN_NETIF_START_QUEUE(dev);
					err=0;
					goto if_send_exit_crit;
				}
			}
#endif
#endif
			if (!chan->hdlc_eng && chan->cfg.data_mux){
				wan_skb_reverse(skb);
			}

			wan_skb_queue_tail(&chan->wp_tx_pending_list,skb);
			if (chan->hdlc_eng) {
				xilinx_dma_tx(card,chan);
			}
			wan_netif_set_ticks(dev, SYSTEM_TICKS);
		}
	}

#ifdef __LINUX__
	if (dev->tx_queue_len < chan->max_tx_bufs &&
		dev->tx_queue_len > 0) {
			DEBUG_EVENT("%s: Resizing Tx Queue Len to %li\n",
				chan->if_name,dev->tx_queue_len);
			chan->max_tx_bufs = dev->tx_queue_len;
	}

	if (dev->tx_queue_len > chan->max_tx_bufs &&
		chan->max_tx_bufs != MAX_TX_BUF) {
			DEBUG_EVENT("%s: Resizing Tx Queue Len to %i\n",
				chan->if_name,MAX_TX_BUF);
			chan->max_tx_bufs = MAX_TX_BUF;
	}
#endif


if_send_exit_crit:

	WAN_NETIF_START_QUEUE(dev);
	return 0;
}


#if defined(__LINUX__) || defined(__WINDOWS__)
/*============================================================================
* if_stats
*
* Used by /proc/net/dev and ifconfig to obtain interface
* statistics.
*
* Return a pointer to struct net_device_stats.
*/
static struct net_device_stats gstats;
static struct net_device_stats* wanpipe_xilinx_ifstats (netdevice_t* dev)
{
	private_area_t* chan;

	if (!dev){
		return &gstats;
	}

	if ((chan=wan_netif_priv(dev)) == NULL)
		return &gstats;

	return &chan->if_stats;
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
wanpipe_xilinx_ioctl(netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t cmd)
{
	private_area_t* chan;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	wan_smp_flag_t		smp_flags;
#endif
	int err=0;

	DEBUG_UDP("%s(): line: %d\n", __FUNCTION__, __LINE__);

	if (!dev || !WAN_NETIF_UP(dev)){
		DEBUG_UDP("%s(): line: %d\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}

	if (!(chan=(private_area_t*)wan_netif_priv(dev))){
		DEBUG_UDP("%s(): line: %d\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	card=(sdla_t*)chan->common.card;

	DEBUG_UDP("%s(): line: %d\n", __FUNCTION__, __LINE__);

	switch(cmd)
	{
#if defined(__LINUX__)
case SIOC_WANPIPE_BIND_SK:
	if (!ifr){
		err= -EINVAL;
		break;
	}

#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	err=wan_bind_api_to_svc(chan,ifr->ifr_data);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
#endif

	chan->if_stats.tx_carrier_errors=0;

	break;

case SIOC_WANPIPE_UNBIND_SK:
	if (!ifr){
		err= -EINVAL;
		break;
	}

#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
#endif

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

#if defined (__LINUX__)
case SIOC_WANPIPE_GET_DEVICE_CONFIG_ID:
	err=card->wandev.config_id;
	break;

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

	/* We have to check here again because we don't know
	* what happened during spin_lock */
	if (wan_test_bit(0,&card->in_isr)) {
		DEBUG_TEST( "%s:%s Pipemon command failed, Driver busy: try again.\n",
			card->devname,wan_netif_name(dev));
		wan_atomic_set(&chan->udp_pkt_len,0);
		return -EBUSY;
	}

	process_udp_mgmt_pkt(card,dev,chan,1);

	/* This area will still be critical to other
	* PIPEMON commands due to udp_pkt_len
	* thus we can release the irq */

	if (wan_atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
		DEBUG_EVENT( "%s: Error: Pipemon buf too bit on the way up! %d\n",
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

case SIOC_AFT_CUSTOMER_ID:

	if (!ifr){
		return -EINVAL;
	}else{
		unsigned char cid;
		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
		cid=read_cpld(card,CUSTOMER_CPLD_ID_REG);
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		return WAN_COPY_TO_USER(ifr->ifr_data,&cid,sizeof(unsigned char));
	}
	break;

default:
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	if (card->wandev.wanpipe_ioctl){
		err = card->wandev.wanpipe_ioctl(dev, ifr, cmd);
	}
#else
# if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
	return 1;
# else
	DEBUG_TEST("%s: Command %x not supported!\n",
		card->devname,cmd);
	return -EOPNOTSUPP;
# endif
#endif
	break;
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
	wan_smp_flag_t		smp_flags;

	DEBUG_CFG("Xilinx Chip Configuration. -- \n");

	xilinx_delay(1);

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);

	/* Configure for T1 or E1 front end */
	if (IS_T1_CARD(card)){
		card->u.aft.num_of_time_slots=NUM_OF_T1_CHANNELS;
		wan_clear_bit(INTERFACE_TYPE_T1_E1_BIT,&reg);
		wan_set_bit(FRONT_END_FRAME_FLAG_ENABLE_BIT,&reg);
	}else if (IS_E1_CARD(card)){
		card->u.aft.num_of_time_slots=NUM_OF_E1_CHANNELS;
		wan_set_bit(INTERFACE_TYPE_T1_E1_BIT,&reg);
		wan_set_bit(FRONT_END_FRAME_FLAG_ENABLE_BIT,&reg);
	}else{
		DEBUG_EVENT("%s: Error: Xilinx doesn't support non T1/E1 interface!\n",
			card->devname);
		return -EINVAL;
	}

	/* Front end reference clock configuration.
	* TE1 front end can use either Oscillator to
	* generate clock, or use the clock from the
	* other line. Supported in Ver:24 */
	if (WAN_TE1_REFCLK(&card->fe) == WAN_TE1_REFCLK_OSC){
		wan_clear_bit(AFT_TE1_FE_REF_CLOCK_BIT,&reg);
	}else{
		wan_set_bit(AFT_TE1_FE_REF_CLOCK_BIT,&reg);
	}

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	WP_DELAY(10000);

	/* Reset PMC */
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
	wan_clear_bit(FRONT_END_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
	WP_DELAY(1000);

	wan_set_bit(FRONT_END_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
	WP_DELAY(100);


	DEBUG_CFG("--- Chip Reset. -- \n");

	/* Reset Chip Core */
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);
	wan_set_bit(CHIP_RESET_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);

	WP_DELAY(100);

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
	WP_DELAY(100);


	/* Sanity check, where we make sure that A101
	* adapter gracefully fails on non existing
	* secondary port */
	if (adapter_type == A101_1TE1_SUBSYS_VENDOR &&
		card->wandev.S514_cpu_no[0] == SDLA_CPU_B){
			DEBUG_EVENT("%s: Hardware Config Mismatch: A103 Adapter not found!\n",
				card->devname);
			/*return -ENODEV;*/
	}


	adptr_security = read_cpld(card,SECURITY_CPLD_REG);
	adptr_security = adptr_security >> SECURITY_CPLD_SHIFT;
	adptr_security = adptr_security & SECURITY_CPLD_MASK;

	DEBUG_EVENT("%s: Hardware Adapter Type 0x%X Scurity 0x%02X\n",
		card->devname,adapter_type, adptr_security);

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
	DEBUG_EVENT("%s: Error Invalid Security ID=0x%X\n",
		card->devname,adptr_security);
	return -EINVAL;
	}

#endif

	/* Turn off Onboard RED LED */
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG,&reg);
	wan_set_bit(XILINX_RED_LED,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
	WP_DELAY(10);

	err=aft_core_ready(card);
	if (err != 0){
		DEBUG_EVENT("%s: WARNING: HDLC Core Not Ready: B4 TE CFG!\n",
			card->devname);

	}

	card->hw_iface.hw_lock(card->hw,&smp_flags);

	err = -EINVAL;
	if (card->wandev.fe_iface.config){
		err=card->wandev.fe_iface.config(&card->fe);
	}
	aft_red_led_ctrl(card, AFT_LED_ON);
	card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);

	card->hw_iface.hw_unlock(card->hw,&smp_flags);

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

	xilinx_delay(1);

	err=aft_core_ready(card);
	if (err != 0){
		DEBUG_EVENT("%s: Error: HDLC Core Not Ready!\n",
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
		DEBUG_EVENT("%s: Error: Active DMA Interrupt Pending. !\n",
			card->devname);

		reg = 0;
		/* Disable the chip/hdlc reset condition */
		wan_set_bit(CHIP_RESET_BIT,&reg);
		card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);
		return err;
	}
	if (wan_test_bit(ERROR_INTR_FLAG,&reg)){
		DEBUG_EVENT("%s: Error: Active Error Interrupt Pending. !\n",
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

	DEBUG_CFG("%s: Enable Front End Interrupts\n",
		card->devname);

	xilinx_delay(1);

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG,reg);


	return err;
}

/*============================================================================
* xilinx_chip_unconfigure
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

	/* Channel definition section. If not channels defined
	* return error */
	if (chan->time_slot_map == 0){
		DEBUG_EVENT("%s: Invalid Channel Selection 0x%X\n",
			card->devname,chan->time_slot_map);
		return -EINVAL;
	}

	DEBUG_TEST("%s:%s: Active channels = 0x%X\n",
		card->devname,chan->if_name,chan->time_slot_map);

	xilinx_delay(1);

	/* Check that the time slot is not being used. If it is
	* stop the interface setup.  Notice, though we proceed
	* to check for all timeslots before we start binding
	* the channels in.  This way, we don't have to go back
	* and clean the time_slot_map */
	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&chan->time_slot_map)){

			if (chan->first_time_slot == -1){
				DEBUG_TEST("%s:%s: Setting first time slot to %ld\n",
					card->devname,chan->if_name,i);
				chan->first_time_slot=i;
			}

			DEBUG_CFG("%s: Configuring %s for timeslot %ld\n",
				card->devname, chan->if_name,
				IS_E1_CARD(card)?i:i+1);

			if (wan_test_bit(i,&card->u.aft.time_slot_map)){
				DEBUG_EVENT("%s: Channel/Time Slot resource conflict!\n",
					card->devname);
				DEBUG_EVENT("%s: %s: Channel/Time Slot %ld, aready in use!\n",
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

	DEBUG_TEST("%s:%d: GOT Logic ch %ld  Free Logic ch %ld\n",
		__FUNCTION__,__LINE__,chan->logic_ch_num,free_logic_ch);

	xilinx_delay(1);

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&chan->time_slot_map)){

			wan_set_bit(i,&card->u.aft.time_slot_map);

			card->hw_iface.bus_read_4(card->hw, XILINX_TIMESLOT_HDLC_CHAN_REG, &reg);
			reg&=~TIMESLOT_BIT_MASK;

			/*FIXME do not hardcode !*/
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

			DEBUG_TEST("Setting Timeslot %ld to logic ch %ld Reg=0x%X\n",
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
		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (!wan_test_bit(i,&card->u.aft.time_slot_map)){

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


				/* Force the channel into HDLC mode by default */
				wan_clear_bit(TRANSPARENT_MODE_BIT,&reg);

				DEBUG_TEST("Setting Timeslot %ld to free logic ch %ld Reg=0x%X\n",
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
		DEBUG_TEST("%s:%s: Config for HDLC mode\n",
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
	wan_smp_flag_t		smp_flags;


	DEBUG_CFG("\n-- Unconfigure Xilinx. --\n");

	if (wan_test_bit(0,&chan->tdmv_sync)){
		channel_timeslot_sync_ctrl(card,chan,0);
		rx_chan_timeslot_sync_ctrl(card,0);
	}

	/* Select an HDLC logic channel for configuration */
	if (chan->logic_ch_num != -1){
		DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);
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


		for (i=0;i<card->u.aft.num_of_time_slots;i++){
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

				DEBUG_TEST("Setting Timeslot %d to logic ch %d Reg=0x%X\n",
					i, 31 ,reg);


				/* Force the channel into HDLC mode by default */
				wan_clear_bit(TRANSPARENT_MODE_BIT,&reg);

				xilinx_write_ctrl_hdlc(card,
					i,
					XILINX_CONTROL_RAM_ACCESS_BUF,
					reg);
			}
		}
	
		DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);
		/* Lock to protect the logic ch map to
		* chan device array */
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		free_xilinx_logical_channel_num(card,chan->logic_ch_num);
		DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);
		free_fifo_baddr_and_size(card,chan);
		DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

		DEBUG_CFG("%s(): line: %d\n", __FUNCTION__, __LINE__);

		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (wan_test_bit(i,&chan->time_slot_map)){
				wan_clear_bit(i,&card->u.aft.time_slot_map);
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
				WP_DELAY(FIFO_RESET_TIMEOUT_US);
				continue;
			}
			timeout=0;
			break;
		}

		if (timeout){
			DEBUG_EVENT("%s:%s: Error: Rx fifo reset timedout %u us\n",
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
				WP_DELAY(FIFO_RESET_TIMEOUT_US);
				continue;
			}
			timeout=0;
			break;
		}

		if (timeout){
			DEBUG_EVENT("%s:%s: Error: Tx fifo reset timedout %u us\n",
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

	wan_set_bit(chan->logic_ch_num,&card->u.aft.active_ch_map);
}

static int xilinx_dma_rx(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	unsigned long dma_descr;
	unsigned long bus_addr;
	wp_rx_element_t *rx_el;


	if (chan->rx_dma_skb){
		wp_rx_element_t *rx_el;
		netskb_t *skb=chan->rx_dma_skb;
		DEBUG_TEST("%s: Clearing RX DMA Pending buffer \n",
			__FUNCTION__);

		chan->rx_dma_skb=NULL;
		rx_el=(wp_rx_element_t *)wan_skb_data(skb);

		card->hw_iface.pci_unmap_dma(card->hw,
			rx_el->dma_addr,
			chan->dma_mru,
			PCI_DMA_FROMDEVICE);

		aft_init_requeue_free_skb(chan, skb);
	}

	chan->rx_dma_skb = wan_skb_dequeue(&chan->wp_rx_free_list);

	if (!chan->rx_dma_skb){
		netskb_t *skb;
		DEBUG_TEST("%s: Critical Error no rx dma buf Free=%d Comp=%d!\n",
			chan->if_name,wan_skb_queue_len(&chan->wp_rx_free_list),
			wan_skb_queue_len(&chan->wp_rx_complete_list));

		while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
			aft_init_requeue_free_skb(chan, skb);
			chan->if_stats.rx_errors++;
		}


		if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
			while((skb=wan_skb_dequeue(&chan->wp_tx_pending_list)) != NULL){
				aft_init_requeue_free_skb(chan, skb);
				chan->if_stats.rx_errors++;
			}
		}

		chan->rx_dma_skb = wan_skb_dequeue(&chan->wp_rx_free_list);
		if (!chan->rx_dma_skb){
			DEBUG_EVENT("%s:%ld:%d Critical Error no STILL NO MEM: RxFree=%i, RxComp=%i!\n",
				chan->if_name,chan->logic_ch_num,chan->hdlc_eng,
				wan_skb_queue_len(&chan->wp_rx_free_list),
				wan_skb_queue_len(&chan->wp_rx_complete_list));

			aft_critical_shutdown(card);
			return -ENOMEM;
		}
	}

	wan_skb_put(chan->rx_dma_skb, sizeof(wp_rx_element_t));
	rx_el = (wp_rx_element_t *)wan_skb_data(chan->rx_dma_skb);
	memset(rx_el,0,sizeof(wp_rx_element_t));

	bus_addr = card->hw_iface.pci_map_dma(card->hw,
		wan_skb_tail(chan->rx_dma_skb),
		chan->dma_mru,
		PCI_DMA_FROMDEVICE);

	if (!bus_addr){
		DEBUG_EVENT("%s: %s Critical error pci_map_dma() failed!\n",
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

#if 0
	DEBUG_RX("%s: RxDMA_LO = 0x%X, BusAddr=0x%X DmaDescr=0x%X\n",
		__FUNCTION__,reg,bus_addr,dma_descr);*/
#endif
		card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	dma_descr=(unsigned long)(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;

	reg =0;

	if (chan->hdlc_eng){
		reg|=(chan->dma_mru>>2)&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	}else{
		reg|=(chan->mru>>2)&RxDMA_HI_DMA_DATA_LENGTH_MASK;
	}

#ifdef TRUE_FIFO_SIZE
	reg|=(chan->fifo_size_code&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
#else

	reg|=(HARD_FIFO_CODE&DMA_FIFO_SIZE_MASK)<<DMA_FIFO_SIZE_SHIFT;
#endif
	reg|=(chan->fifo_base_addr&DMA_FIFO_BASE_ADDR_MASK)<<
		DMA_FIFO_BASE_ADDR_SHIFT;

	wan_set_bit(RxDMA_HI_DMA_GO_READY_BIT,&reg);

	DEBUG_RX("%s: RXDMA_HI = 0x%X, BusAddr=0x%lX DmaDescr=0x%lX\n",
		__FUNCTION__,reg,bus_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);


	wan_set_bit(0,&chan->rx_dma);

	return 0;
}

static void xilinx_dev_close(sdla_t *card, private_area_t *chan)
{
	u32 reg;
	unsigned long dma_descr;
	wan_smp_flag_t		smp_flags;

	DEBUG_CFG("-- Close Xilinx device. --\n");

	/* Disable Logic Channel Interrupts for DMA and fifo */
	card->hw_iface.bus_read_4(card->hw,
		XILINX_GLOBAL_INTER_MASK, &reg);

	wan_clear_bit(chan->logic_ch_num,&reg);
	wan_clear_bit(chan->logic_ch_num,&card->u.aft.active_ch_map);

	/* We are masking the chan interrupt.
	* Lock to make sure that the interrupt is
	* not running */
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	card->hw_iface.bus_write_4(card->hw,
		XILINX_GLOBAL_INTER_MASK, reg);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);


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


	DEBUG_TX(" ------ Setup Tx DMA descriptor. --\n");

	if (wan_test_and_set_bit(TX_BUSY,&chan->dma_status)){
		DEBUG_TX("%s:%d:  TX_BUSY set!\n",
			__FUNCTION__,__LINE__);
		return -EBUSY;
	}


	/* Free the previously skb dma mapping.
	* In this case the tx interrupt didn't finish
	* and we must re-transmit.*/

	if (chan->tx_dma_addr && chan->tx_dma_len){

		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: Unmaping tx_dma_addr in %s\n",
				chan->if_name,__FUNCTION__);
		}

		aft_unmap_tx_dma(card,chan);
	}

	/* Free the previously sent tx packet. To
	* minimize tx isr, the previously transmitted
	* packet is deallocated here */
	if (chan->tx_dma_skb){

		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: Deallocating tx_dma_skb in %s\n",
				chan->if_name,__FUNCTION__);
		}
		wan_aft_skb_defered_dealloc(chan, chan->tx_dma_skb); /* instead of wan_skb_free(chan->tx_dma_skb);*/
		chan->tx_dma_skb=NULL;
	}


	/* check queue pointers befor start transmittion */

	/* sanity check: make sure that DMA is in ready state */
	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;

	DEBUG_TX("%s:%d: chan logic ch=%ld dma_descr=0x%lx set!\n",
		__FUNCTION__,__LINE__,chan->logic_ch_num,dma_descr);

	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg);

	if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
		DEBUG_TEST("%s: Error: TxDMA GO Ready bit set on dma Tx 0x%X\n",
			card->devname,reg);
		wan_clear_bit(TX_BUSY,&chan->dma_status);
		return -EFAULT;
	}

	switch(chan->common.usedby){

	case TDM_VOICE:
	case TDM_VOICE_API:
		skb = wan_skb_dequeue(&chan->wp_tx_pending_list);
		break;

	default:
		if(!chan->lip_atm){
			skb=wan_skb_dequeue(&chan->wp_tx_pending_list);
		}else{
			skb=atm_tx_skb_dequeue(&chan->wp_tx_pending_list, chan->tx_idle_skb, chan->if_name);
		}
		break;
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
		len=wan_skb_len(chan->tx_idle_skb);

		chan->tx_dma_addr = card->hw_iface.pci_map_dma(card->hw,
			wan_skb_data(chan->tx_idle_skb),
			wan_skb_len(chan->tx_idle_skb),
			PCI_DMA_TODEVICE);

		chan->tx_dma_len = wan_skb_len(chan->tx_idle_skb);

		chan->if_stats.tx_carrier_errors++;
	}else{
		len = wan_skb_len(skb);
		if (wan_skb_len(skb) > MAX_XILINX_TX_DMA_SIZE){
			/* FIXME: We need to split this frame into
			*        multiple parts.  For now thought
			*        just drop it :) */
			DEBUG_EVENT("%s:%d:  Tx pkt len > Max Len (%d)!\n",
				__FUNCTION__,__LINE__,MAX_XILINX_TX_DMA_SIZE);

			if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
				aft_init_requeue_free_skb(chan, skb);
			}else{
				wan_aft_skb_defered_dealloc(chan, skb); /* instead of wan_skb_free(skb); */
			}
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			return -EINVAL;
		}

		if ((unsigned long)wan_skb_data(skb) & 0x03){
			int err=aft_realign_skb_pkt(chan, skb);
			if (err){
				DEBUG_EVENT("%s: Error: Failed to allocate memory in %s()\n",
					card->devname,__FUNCTION__);

				if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
					aft_init_requeue_free_skb(chan, skb);
				}else{
					wan_aft_skb_defered_dealloc(chan, skb); /* instead of wan_skb_free(skb); */
				}
				wan_clear_bit(TX_BUSY,&chan->dma_status);
				return -EINVAL;
			}
		}



		chan->tx_dma_addr =
			card->hw_iface.pci_map_dma(card->hw,
			wan_skb_data(skb),
			wan_skb_len(skb),
			PCI_DMA_TODEVICE);

		chan->tx_dma_len = wan_skb_len(skb);
	}

	if (chan->tx_dma_addr & 0x03){

		DEBUG_EVENT("%s: Critcal Error: Tx Ptr not aligned to 32bit boudary!\n",
			card->devname);

		if (skb){
			if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
				aft_init_requeue_free_skb(chan, skb);
			}else{
				wan_aft_skb_defered_dealloc(chan, skb); /* instead of wan_skb_free(skb); */
			}
		}

		aft_unmap_tx_dma(card,chan);

		wan_clear_bit(TX_BUSY,&chan->dma_status);
		return -EINVAL;
	}

	if (skb){
#if defined(WAN_DEBUG_TX)
		char*	data = wan_skb_data(skb);
#endif
		chan->tx_dma_skb=skb;

#if defined(WAN_DEBUG_TX)
		DEBUG_TX("TX SKB DATA 0x%08X 0x%08X 0x%08X 0x%08X \n",
			*(unsigned int*)&data[0],
			*(unsigned int*)&data[4],
			*(unsigned int*)&data[8],
			*(unsigned int*)&data[12]);
#endif

#if defined(__LINUX__)
		DEBUG_TX("Tx dma skb bound %p List=%p Data=%p BusPtr=%lx\n",
			skb,skb->list,wan_skb_data(skb),chan->tx_dma_addr);
#endif
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

	DEBUG_TX("%s: TXDMA_LO=0x%X PhyAddr=0x%X DmaDescr=0x%lX\n",
		__FUNCTION__,reg,chan->tx_dma_addr,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;

	reg=0;
	reg|=(((len>>2)+len_align)&TxDMA_HI_DMA_DATA_LENGTH_MASK);

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

	/* For TDM VOICE Timeslot Synchronization,
	* we must use FRAME START bit as a first
	* slot sync character */
	if (wan_test_bit(0,&chan->tdmv_sync)){
		wan_set_bit(TxDMA_HI_DMA_FRAME_START_BIT,&reg);
	}

	wan_set_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg);

	DEBUG_TEST("%s: TXDMA_HI=0x%X DmaDescr=0x%lX\n",
		__FUNCTION__,reg,dma_descr);

	card->hw_iface.bus_write_4(card->hw,dma_descr,reg);

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
	/*      card->hw_iface.bus_read_4(card->hw,0x78, &tmp1); */

	dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &reg);

	if (reg & TxDMA_HI_DMA_DATA_LENGTH_MASK){
		chan->errstats.Tx_dma_len_nonzero++;
	}

	if (!chan->tx_dma_skb){

		if (chan->hdlc_eng){
			DEBUG_EVENT("%s: Critical Error: Tx DMA intr: no tx skb !\n",
				card->devname);
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			return;
		}else{

			aft_unmap_tx_dma(card,chan);

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

		aft_unmap_tx_dma(card,chan);

		if (chan->hdlc_eng){

			/* Do not free the packet here,
			* copy the packet dma info into csum
			* field and let the bh handler analyze
			* the transmitted packet.
			*/

			if (reg & TxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){

				DEBUG_EVENT("%s:%s: PCI Error: 'Retry' exceeds maximum (64k): Reg=0x%X!\n",
					card->devname,chan->if_name,reg);

				if (++chan->pci_retry < 3){
					wan_set_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg);

					DEBUG_EVENT("%s: Retry: TXDMA_HI=0x%X DmaDescr=0x%lX\n",
						__FUNCTION__,reg,dma_descr);

					card->hw_iface.bus_write_4(card->hw,dma_descr,reg);
					return;
				}
			}

			chan->pci_retry=0;
			wan_skb_set_csum(chan->tx_dma_skb, reg);
			wan_skb_queue_tail(&chan->wp_tx_complete_list,chan->tx_dma_skb);
			chan->tx_dma_skb=NULL;

			wan_clear_bit(TX_BUSY,&chan->dma_status);

			WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
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

			wan_skb_set_csum(skb, reg);
			wan_clear_bit(TX_BUSY,&chan->dma_status);
			xilinx_tx_post_complete (card,chan,skb);

			if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
				/* Voice code uses the rx buffer to
				* transmit! So put the rx buffer back
				* into the rx queue */
				aft_init_requeue_free_skb(chan, skb);
			}else{
				wan_aft_skb_defered_dealloc(chan, skb); /* instead of wan_skb_free(skb); */
			}
		}
	}

	/* DEBUGTX */
	/*	card->hw_iface.bus_read_4(card->hw,0x78, &tmp1);  */

}


static void xilinx_tx_post_complete (sdla_t *card, private_area_t *chan, netskb_t *skb)
{
	unsigned long reg = wan_skb_csum(skb);
	private_area_t *top_chan;

	if (card->u.aft.tdmv_dchan){
		top_chan=wan_netif_priv(chan->common.dev);
	}else{
		top_chan=chan;
	}

	xilinx_dma_tx(card,chan);

	if ((wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)) ||
		(reg & TxDMA_HI_DMA_DATA_LENGTH_MASK) ||
		(reg & TxDMA_HI_DMA_PCI_ERROR_MASK)){


			/* Checking Tx DMA Go bit. Has to be '0' */
			if (wan_test_bit(TxDMA_HI_DMA_GO_READY_BIT,&reg)){
				DEBUG_TEST("%s:%s: Error: TxDMA Intr: GO bit set on Tx intr (0x%lX)\n",
					card->devname,chan->if_name,reg);
				chan->errstats.Tx_dma_errors++;
			}

			if (reg & TxDMA_HI_DMA_DATA_LENGTH_MASK){
				DEBUG_TEST("%s:%s: Error: TxDMA Length not equal 0 (0x%lX)\n",
					card->devname,chan->if_name,reg);
				chan->errstats.Tx_dma_errors++;
			}

			/* Checking Tx DMA PCI error status. Has to be '0's */
			if (reg&TxDMA_HI_DMA_PCI_ERROR_MASK){

				if (reg & TxDMA_HI_DMA_PCI_ERROR_M_ABRT){
					if (WAN_NET_RATELIMIT()){
						DEBUG_EVENT("%s:%s: Tx Error: Abort from Master: pci fatal error! (0x%lX)\n",
							card->devname,chan->if_name,reg);
					}
				}
				if (reg & TxDMA_HI_DMA_PCI_ERROR_T_ABRT){
					if (WAN_NET_RATELIMIT()){
						DEBUG_EVENT("%s:%s: Tx Error: Abort from Target: pci fatal error! (0x%lX)\n",
							card->devname,chan->if_name,reg);
					}
				}
				if (reg & TxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
					DEBUG_TEST("%s:%s: Tx Warning: PCI Latency Timeout! (0x%lX)\n",
						card->devname,chan->if_name,reg);
					chan->errstats.Tx_pci_latency++;
					goto tx_post_ok;
				}
				if (reg & TxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
					if (WAN_NET_RATELIMIT()){
						DEBUG_EVENT("%s:%s: Tx Error: 'Retry' exceeds maximum (64k): pci fatal error! (0x%lX)\n",
							card->devname,chan->if_name,reg);
					}
				}
				chan->errstats.Tx_pci_errors++;
			}
			chan->if_stats.tx_dropped++;
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

#if defined(__WINDOWS__)
	wan_capture_trace_packet(card, &top_chan->trace_info, skb, TRC_OUTGOING_FRM);
#else
	if (wan_tracing_enabled(&top_chan->trace_info) >= 1){
		if (card->u.aft.tdmv_dchan){
			if (chan->common.usedby == TDM_VOICE_DCHAN){
				wan_capture_trace_packet(card, &top_chan->trace_info, skb, TRC_OUTGOING_FRM);
			}
		}else{
			wan_capture_trace_packet_offset(card, &top_chan->trace_info, skb,
				IS_T1_CARD(card)?24:16, TRC_OUTGOING_FRM);
		}
	}else{
		wan_capture_trace_packet(card, &top_chan->trace_info, skb, TRC_OUTGOING_FRM);
	}
#endif

tx_post_exit:

	if (WAN_NETIF_QUEUE_STOPPED(chan->common.dev)){
		wanpipe_wake_stack(chan);
	}

#ifdef AFT_TDM_API_SUPPORT
	if (chan->common.usedby == TDM_VOICE_DCHAN){
		wanpipe_tdm_api_dev_t *wp_tdm_api_dev =
			&chan->wp_tdm_api_dev_idx[chan->tdmv_chan];

		if (is_tdm_api(chan,wp_tdm_api_dev)){
			wanpipe_tdm_api_kick(wp_tdm_api_dev);
		}
	}
#endif


	return;
}

static void xilinx_dma_rx_complete (sdla_t *card, private_area_t *chan)
{
	unsigned long dma_descr;
	netskb_t *skb;
	wp_rx_element_t *rx_el;
	int tdmv_sync_reset=0;

#if 0
	/*NCDEBUG: Used to debug the TDM rx queues */
	chan->if_stats.rx_errors=wan_skb_queue_len(&chan->wp_rx_free_list);
	chan->if_stats.tx_errors=wan_skb_queue_len(&chan->wp_tx_pending_list);
#endif

	wan_clear_bit(0,&chan->rx_dma);

	if (!chan->rx_dma_skb){
		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: Critical Error: rx_dma_skb\n",chan->if_name);
		}
		return;
	}

	rx_el=(wp_rx_element_t *)wan_skb_data(chan->rx_dma_skb);

#if 0
	chan->if_stats.rx_frame_errors++;
#endif

	/*    	card->hw_iface.bus_read_4(card->hw,0x80, &rx_empty);  */

	/* Reading Rx DMA descriptor information */
	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_LO;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &rx_el->align);
	rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;

	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_read_4(card->hw,dma_descr, &rx_el->reg);

	rx_el->pkt_error = chan->pkt_error;

	/*	DEBUG_RX("%s: DmaDescrLo=0x%X  DmaHi=0x%X \n",*/
	/*		__FUNCTION__,rx_el.align,reg);*/


	card->hw_iface.pci_unmap_dma(card->hw,
		rx_el->dma_addr,
		chan->dma_mru,
		PCI_DMA_FROMDEVICE);

	DEBUG_RX("%s:%s: RX HI=0x%X  LO=0x%X DMA=0x%lX\n",
		__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);

	skb=chan->rx_dma_skb;
	chan->rx_dma_skb=NULL;

	if (wan_test_bit(WP_FIFO_ERROR_BIT, &chan->pkt_error)){
		if (wan_test_bit(0,&chan->tdmv_sync)){
			rx_chan_timeslot_sync_ctrl(card,0);
			xilinx_init_rx_dev_fifo(card, chan, WP_WAIT);
			tdmv_sync_reset=1;
		}
	}

	xilinx_dma_rx(card,chan);

	if (tdmv_sync_reset){
		rx_chan_timeslot_sync_ctrl(card,1);
	}

	if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
		signed int err;
		if (!wan_test_bit(WP_FIFO_ERROR_BIT, &chan->pkt_error)){
			err=aft_dma_rx_tdmv(card,chan,skb);
			if (err == 0){
				skb=NULL;
			}
		}

		if (skb){
			aft_init_requeue_free_skb(chan, skb);
			chan->if_stats.rx_dropped++;
		}
	}else{
		wan_skb_queue_tail(&chan->wp_rx_complete_list,skb);
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
	}

	chan->pkt_error=0;

	/*    	card->hw_iface.bus_read_4(card->hw,0x80, &rx_empty); */
}


static void xilinx_rx_post_complete (sdla_t *card, private_area_t *chan,
									 netskb_t *skb,
									 netskb_t **new_skb,
									 wan_bitmap_t *pkt_error)
{

	unsigned int len,data_error = 0;
	unsigned char *buf;
	wp_rx_element_t *rx_el=(wp_rx_element_t *)wan_skb_data(skb);

	DEBUG_RX("%s:%s: RX HI=0x%X  LO=0x%X DMA=0x%lX\n",
		__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);

#if 0
	chan->if_stats.rx_errors++;
#endif

	rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;
	*pkt_error=0;
	*new_skb=NULL;


	/* Checking Rx DMA Go bit. Has to be '0' */
	if (wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&rx_el->reg)){
		DEBUG_TEST("%s:%s: Error: RxDMA Intr: GO bit set on Rx intr\n",
			card->devname,chan->if_name);
		chan->if_stats.rx_errors++;
		chan->errstats.Rx_dma_descr_err++;
		goto rx_comp_error;
	}

	/* Checking Rx DMA PCI error status. Has to be '0's */
	if (rx_el->reg&RxDMA_HI_DMA_PCI_ERROR_MASK){

		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_M_ABRT){
			DEBUG_EVENT("%s:%s: Rx Error: Abort from Master: pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}
		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_T_ABRT){
			DEBUG_EVENT("%s:%s: Rx Error: Abort from Target: pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}
		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
			DEBUG_EVENT("%s:%s: Rx Error: No 'DeviceSelect' from target: pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}
		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
			DEBUG_EVENT("%s:%s: Rx Error: 'Retry' exceeds maximum (64k): pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}

		chan->if_stats.rx_errors++;
		chan->errstats.Rx_pci_errors++;
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
				DEBUG_TEST("%s:%s: RxDMA Intr: CRC Error! Reg=0x%X\n",
					card->devname,chan->if_name,rx_el->reg);
				chan->if_stats.rx_errors++;
				chan->errstats.Rx_crc_err_count++;
				wan_set_bit(WP_CRC_ERROR_BIT,&rx_el->pkt_error);
				data_error = 1;
			}

			/* Check if this frame is an abort, if it is
			* drop it and continue receiving */
			if (wan_test_bit(RxDMA_HI_DMA_FRAME_ABORT_BIT,&rx_el->reg)){
				DEBUG_TEST("%s:%s: RxDMA Intr: Abort! Reg=0x%X\n",
					card->devname,chan->if_name,rx_el->reg);
				chan->if_stats.rx_frame_errors++;
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
		len=(((chan->dma_mru>>2)-len)<<2) - (~(rx_el->align)&RxDMA_LO_ALIGNMENT_BIT_MASK);

		if (len < 1 || len > chan->dma_mru){
			chan->if_stats.rx_frame_errors++;
			chan->errstats.Rx_hdlc_corrupiton++;
			goto rx_comp_error;
		}

	}else{
		/* In Transparent mode, our RX buffer will always be
		* aligned to the 32bit (word) boundary, because
		* the RX buffers are all of equal length  */
		len=(((chan->mru>>2)-len)<<2) - (~(0x03)&RxDMA_LO_ALIGNMENT_BIT_MASK);

		if (len < 1 || len > chan->mru){
			chan->if_stats.rx_frame_errors++;
			goto rx_comp_error;
		}
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

#if defined(__OpenBSD__)
		/* Try not to return our mbuf to the kernel.
		** Still possibility that firmware can use the hardware pointer
		** (not confirmed yet) */
		*new_skb=wan_skb_alloc(len + 20);
		if (!*new_skb){
			DEBUG_EVENT("%s:%s: Failed to allocate rx skb pkt (len=%d)!\n",
				card->devname,chan->if_name,(len+20));
			chan->if_stats.rx_dropped++;
			goto rx_comp_error;
		}

		buf=wan_skb_put((*new_skb),len);
		memcpy(buf, wan_skb_tail(skb),len);

		aft_init_requeue_free_skb(chan, skb);
#else
		/* The rx size is big enough, thus
		* send this buffer up the stack
		* and allocate another one */
		wan_skb_put(skb,len);
		wan_skb_pull(skb, sizeof(wp_rx_element_t));
		*new_skb=skb;

		aft_alloc_rx_dma_buff(card,chan,1,1);
#endif
	} else {

		/* The rx packet is very
		* small thus, allocate a new
		* buffer and pass it up
		*/
		*new_skb=wan_skb_alloc(len + 20);
		if (!*new_skb){
			DEBUG_EVENT("%s:%s: Failed to allocate rx skb pkt (len=%d)!\n",
				card->devname,chan->if_name,(len+20));
			chan->if_stats.rx_dropped++;
			goto rx_comp_error;
		}

		buf=wan_skb_put((*new_skb),len);
		memcpy(buf, wan_skb_tail(skb),len);

		aft_init_requeue_free_skb(chan, skb);
	}


	if (!chan->hdlc_eng && chan->cfg.data_mux){
		wan_skb_reverse(*new_skb);
	}

	return;

rx_comp_error:

	aft_init_requeue_free_skb(chan, skb);
	return;
}


static char request_xilinx_logical_channel_num (sdla_t *card, private_area_t *chan, long *free_ch)
{
	signed char logic_ch=-1, free_logic_ch=-1;
	int i,err;

	*free_ch=-1;

	DEBUG_TEST("-- Request_Xilinx_logic_channel_num:--\n");

	DEBUG_TEST("%s:%d Global Num Timeslots=%d  Global Logic ch Map 0x%lX \n",
		__FUNCTION__,__LINE__,
		card->u.aft.num_of_time_slots,
		card->u.aft.logic_ch_map);


	err=request_fifo_baddr_and_size(card,chan);
	if (err){
		return -1;
	}


	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (!wan_test_and_set_bit(i,&card->u.aft.logic_ch_map)){
			logic_ch=i;
			break;
		}
	}

	if (logic_ch == -1){
		return logic_ch;
	}

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (!wan_test_bit(i,&card->u.aft.logic_ch_map)){
			free_logic_ch=HDLC_FREE_LOGIC_CH;
			break;
		}
	}

	if (card->u.aft.dev_to_ch_map[(unsigned char)logic_ch]){
		DEBUG_EVENT("%s: Error, request logical ch=%d map busy\n",
			card->devname,logic_ch);
		return -1;
	}

	*free_ch=free_logic_ch;

	card->u.aft.dev_to_ch_map[(unsigned char)logic_ch]=(void*)chan;

	if (logic_ch > card->u.aft.top_logic_ch){
		card->u.aft.top_logic_ch=logic_ch;
		xilinx_dma_max_logic_ch(card);
	}


	DEBUG_CFG("Binding logic ch %d  Ptr=%p\n",logic_ch,chan);
	return logic_ch;
}

static void free_xilinx_logical_channel_num (sdla_t *card, int logic_ch)
{
	wan_clear_bit (logic_ch,&card->u.aft.logic_ch_map);
	card->u.aft.dev_to_ch_map[logic_ch]=NULL;

	if (logic_ch >= card->u.aft.top_logic_ch){
		int i;

		card->u.aft.top_logic_ch=XILINX_DEFLT_ACTIVE_CH;

		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			/*NC: Bug fix: Apr 28 2005
			*    Used logic_ch instead of i as the
			*    index into the dev_to_ch_map */
			if (card->u.aft.dev_to_ch_map[i]){
				card->u.aft.top_logic_ch=i;
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
	reg|=(card->u.aft.top_logic_ch << DMA_ACTIVE_CHANNEL_BIT_SHIFT);

	card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);
}

static int aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb)
{
	wan_skb_init(skb,sizeof(wp_api_hdr_t));
	wan_skb_trim(skb,0);
#if 0
	memset(wan_skb_data(skb),0,sizeof(wp_rx_element_t));
#endif
	wan_skb_queue_tail(&chan->wp_rx_free_list,skb);

	return 0;
}

#if 0
static int aft_reinit_pending_rx_bufs(private_area_t *chan)
{
	netskb_t *skb;

	while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
		chan->if_stats.rx_dropped++;
		aft_init_requeue_free_skb(chan,skb);
	}

	return 0;
}
#endif

static int aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num, int irq)
{
	int i;
	netskb_t *skb;

	for (i=0;i<num;i++){
		if (irq) {
			skb=wan_skb_alloc(chan->dma_mru);
		} else {
			skb=wan_skb_kalloc(chan->dma_mru);
		}
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
	sdla_t	*card = (sdla_t*)card_id;

#if defined(__LINUX__) || defined(__WINDOWS__)
	wan_set_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd);
	WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));
#else
	wan_smp_flag_t	smp_flags;
	int		delay = 0;
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	delay = card->wandev.fe_iface.polling(&card->fe);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	if (delay){
		card->wandev.fe_iface.add_timer(&card->fe, delay);
	}
#endif

	DEBUG_TEST("%s: %s Sdla Polling End!\n",
		__FUNCTION__,card->devname);
	return;
}

/**SECTION**************************************************
*
* 	API Bottom Half Handlers
*
**********************************************************/

#if defined(__LINUX__)
static void wp_bh (unsigned long data)
#elif defined(__WINDOWS__)
static void wp_bh (IN PKDPC Dpc, IN PVOID data, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
#else
static void wp_bh(void *data, int pending)
#endif
{
	private_area_t	*chan = (private_area_t *)data;
	sdla_t 		*card = chan->common.card;
	netskb_t 	*new_skb, *skb, *dealloc_skb;
	wan_bitmap_t	pkt_error;
	int		len;
	private_area_t *top_chan;
	wan_smp_flag_t smp_flags;

	DEBUG_TEST("%s()\n", __FUNCTION__);

	if (!wan_test_bit(0,&chan->up)){
		DEBUG_EVENT("%s: wp_bh() chan not up!\n",
			chan->if_name);
		WAN_TASKLET_END((&chan->common.bh_task));
		return;
	}

#if defined(__WINDOWS__)
	top_chan=chan;
#else
	if (card->u.aft.tdmv_dchan){
		top_chan=wan_netif_priv(chan->common.dev);
	}else{
		top_chan=chan;
	}
#endif

	while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
		int len;

#if 0
		chan->if_stats.rx_errors++;
#endif

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
		if (new_skb == NULL){
			continue;
		}

		len=wan_skb_len(new_skb);

		if (top_chan){
			wan_capture_trace_packet(chan->common.card, &top_chan->trace_info,
				new_skb,TRC_INCOMING_FRM);
		}

		if (chan->common.usedby == API){
#if defined(__WINDOWS__)
			int err;
			wanpipe_tdm_api_dev_t *wp_tdm_api_dev =
				&chan->wp_tdm_api_dev_idx[chan->tdmv_chan];

			if (is_tdm_api(chan,wp_tdm_api_dev)){

				DEBUG_TEST("%s()\n", __FUNCTION__);

				if (wan_skb_headroom(new_skb) >= sizeof(wp_api_hdr_t)){
					wp_api_hdr_t *rx_hdr =
						(wp_api_hdr_t*)skb_push(new_skb,sizeof(wp_api_hdr_t));
					memset(rx_hdr,0,sizeof(wp_api_hdr_t));
					DEBUG_TEST("%s()\n", __FUNCTION__);
					//rx_hdr->error_flag=pkt_error;
				} else {
					if (WAN_NET_RATELIMIT()){
						DEBUG_EVENT("%s: Error Rx pkt headroom %u < %u\n",
							chan->if_name,
							(u32)wan_skb_headroom(new_skb),
							(u32)sizeof(wp_api_hdr_t));
					}
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}

				//wan_skb_print(new_skb);//FIXME: comment it out

				err=wanpipe_tdm_api_span_rx(wp_tdm_api_dev,new_skb);
				if (err){
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}

			} else {
				DEBUG_EVENT("%s: DCHAN Rx Packet critical error op not supported ch=%i\n",card->devname,chan->tdmv_chan);
				++chan->if_stats.rx_dropped;
				wan_skb_free(new_skb);
				continue;
			}
#else
			if (chan->common.sk == NULL){
				DEBUG_TEST("%s: No sock bound to channel rx dropping!\n",
					chan->if_name);
				chan->if_stats.rx_dropped++;
				wan_skb_print(new_skb);
				wan_skb_free(new_skb);
				continue;
			}
#endif

#if defined(__LINUX__)
# ifndef CONFIG_PRODUCT_WANPIPE_GENERIC

			/* Only for API, we insert packet status
			* byte to indicate a packet error. Take
			* this byte and put it in the api header */
			if (wan_skb_headroom(new_skb) >= sizeof(wp_api_hdr_t)){
				wp_api_hdr_t *rx_hdr=
					(wp_api_hdr_t*)wan_skb_push(new_skb,sizeof(wp_api_hdr_t));
				memset(rx_hdr,0,sizeof(wp_api_hdr_t));
				rx_hdr->wp_api_rx_hdr_error_flag=pkt_error;
			}else{
				int hroom=wan_skb_headroom(new_skb);
				int rx_sz=sizeof(wp_api_hdr_t);
				DEBUG_EVENT("%s: Error Rx pkt headroom %d < %d\n",
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
				if (net_ratelimit()){
					DEBUG_EVENT("%s: Error: Rx Socket busy!\n",
						chan->if_name);
				}
				++chan->if_stats.rx_dropped;
				wan_skb_free(new_skb);
				continue;
			}
# endif
#endif

		}else if (chan->common.usedby == TDM_VOICE_DCHAN){

			if (chan->tdmv_zaptel_cfg) {

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE) && defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN)
				sdla_t *card=chan->common.card;
				int	err;

				WAN_TDMV_CALL(rx_dchan,
					(&card->wan_tdmv,
					chan->tdmv_chan,
					wan_skb_data(new_skb),
					wan_skb_len(new_skb)), err);

# else
				DEBUG_EVENT("%s: DCHAN Rx Packet critical error TDMV not compiled!\n",card->devname);
# endif

				DEBUG_TEST("%s:%ld TDM DCHAN VOICE Rx Pkt Len=%d Chan=%d\n",
					card->devname,chan->logic_ch_num,wan_skb_len(new_skb),
					chan->tdmv_chan);
				wan_skb_free(new_skb);

#ifdef AFT_TDM_API_SUPPORT
				/* TDM API SUPPORT COMPILED IN */
			} else {

				int err;
				wanpipe_tdm_api_dev_t *wp_tdm_api_dev =
					&chan->wp_tdm_api_dev_idx[chan->tdmv_chan];

				if (is_tdm_api(chan,wp_tdm_api_dev)){

					if (wan_skb_headroom(new_skb) >= sizeof(wp_api_hdr_t)){
						wp_api_hdr_t *rx_hdr =
							(wp_api_hdr_t*)skb_push(new_skb,sizeof(wp_api_hdr_t));
						memset(rx_hdr,0,sizeof(wp_api_hdr_t));
						//rx_hdr->error_flag=pkt_error;
					} else {
						if (WAN_NET_RATELIMIT()){
							DEBUG_EVENT("%s: Error Rx pkt headroom %u < %u\n",
								chan->if_name,
								(u32)wan_skb_headroom(new_skb),
								(u32)sizeof(wp_api_hdr_t));
						}
						++chan->if_stats.rx_dropped;
						wan_skb_free(new_skb);
						continue;
					}

					//wan_skb_print(new_skb);//FIXME: comment it out

					err=wanpipe_tdm_api_span_rx(wp_tdm_api_dev,new_skb);
					if (err){
						++chan->if_stats.rx_dropped;
						wan_skb_free(new_skb);
						continue;
					}

				} else {
					DEBUG_EVENT("%s: DCHAN Rx Packet critical error op not supported ch=%i\n",card->devname,chan->tdmv_chan);
					++chan->if_stats.rx_dropped;
					wan_skb_free(new_skb);
					continue;
				}
			}
#else
				/* TDM API SUPPORT NOT COMPILED IN */

			} else {

				DEBUG_EVENT("%s: TDM API support not compiled in\n",card->devname);
				++chan->if_stats.rx_dropped;
				wan_skb_free(new_skb);
				continue;
			}
#endif

		} else if (chan->common.usedby == TDM_VOICE){
			/* TDM VOICE doesn't operate here */
			if (WAN_NET_RATELIMIT()){
				DEBUG_EVENT("%s:%ld Critical Error: TDM VOICE Rx Pkt in BH\n",
					chan->if_name,
					chan->logic_ch_num);
			}
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
			DEBUG_TEST("%s(): line: %d\n", __FUNCTION__, __LINE__);
			protocol_recv(chan->common.card,chan,new_skb);
		}

		chan->opstats.Data_frames_Rx_count++;
		chan->opstats.Data_bytes_Rx_count+=len;
		chan->if_stats.rx_packets++;
		chan->if_stats.rx_bytes+=len;
	}/* while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL) */

	while(chan->hdlc_eng && (skb=wan_skb_dequeue(&chan->wp_tx_complete_list)) != NULL){
		xilinx_tx_post_complete (chan->common.card,chan,skb);
		wan_skb_free(skb);
	}

	do {
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		dealloc_skb = wan_skb_dequeue(&chan->wp_dealloc_list);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
		if (!dealloc_skb) {
			break;
		}
		DEBUG_TEST("%s(): from 'wp_dealloc_list' freeing: 0x%p\n", __FUNCTION__, dealloc_skb);
		wan_skb_free(dealloc_skb);
	}while(dealloc_skb);

	WAN_TASKLET_END((&chan->common.bh_task));

	if ((len=wan_skb_queue_len(&chan->wp_rx_complete_list))){
		DEBUG_TEST("%s: Triggering from bh rx=%d\n",chan->if_name,len);
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
	}else if ((len=wan_skb_queue_len(&chan->wp_tx_complete_list))){
		DEBUG_TEST("%s: Triggering from bh tx=%d\n",chan->if_name,len);
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
	}

	return;
}

static int fifo_error_interrupt(sdla_t *card, u32 reg,u32 tx_status,u32 rx_status)
{
	u32 err=0;
	u32 i;
	private_area_t *chan;

	/* Clear HDLC pending registers */
	if (card->wandev.state != WAN_CONNECTED){
		DEBUG_TEST("%s: Warning: Ignoring Error Intr: link disc!\n",
			card->devname);
		return 0;
	}

	if (tx_status != 0){
		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (wan_test_bit(i,&tx_status) && wan_test_bit(i,&card->u.aft.logic_ch_map)){

				chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
				if (!chan){
					DEBUG_TEST("Warning: ignoring tx error intr: no dev!\n");
					continue;
				}

				if (!wan_test_bit(0,&chan->up)){
					DEBUG_TEST("%s: Warning: ignoring tx error intr: dev down 0x%X  UP=0x%X!\n",
						wan_netif_name(chan->common.dev),
						chan->common.state,
						chan->ignore_modem);
					continue;
				}

				if (chan->common.state != WAN_CONNECTED){
					if (WAN_NET_RATELIMIT()){
						DEBUG_EVENT("%s: Warning: ignoring tx error intr: dev disc!\n",
							wan_netif_name(chan->common.dev));
					}
					continue;
				}

				if (!chan->hdlc_eng && !wan_test_bit(0,&chan->idle_start)){
					DEBUG_TEST("%s: Warning: ignoring tx error intr: dev init error!\n",
						wan_netif_name(chan->common.dev));
					if (chan->hdlc_eng){
						xilinx_tx_fifo_under_recover(card,chan);
					}
					continue;
				}
#if 0
				if (chan->hdlc_eng && WAN_NET_RATELIMIT()){
					u32 reg_lo, reg_hi,dma_descr;
					dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_LO;
					card->hw_iface.bus_read_4(card->hw,dma_descr, &reg_lo);

					dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
					card->hw_iface.bus_read_4(card->hw,dma_descr, &reg_hi);

					DEBUG_TEST("%s:%s: TX Fifo Error on LogicCh=%li TSlot=%d! CFG: 0x%08X DmaLO: 0x%08X DmaHI: 0x%08X\n",
						card->devname,
						chan->if_name,
						chan->logic_ch_num,
						i,
						reg,
						reg_lo,
						reg_hi);

				}
#endif
				xilinx_tx_fifo_under_recover(card,chan);
				err=-EINVAL;
			}
		}
	}


	if (rx_status != 0){
		for (i=0;i<card->u.aft.num_of_time_slots;i++){
			if (wan_test_bit(i,&rx_status) && wan_test_bit(i,&card->u.aft.logic_ch_map)){
				chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
				if (!chan){
					continue;
				}

				if (!wan_test_bit(0,&chan->up)){
					DEBUG_TEST("%s: Warning: ignoring rx error intr: dev down 0x%X UP=0x%X!\n",
						wan_netif_name(chan->common.dev),
						chan->common.state,
						chan->ignore_modem);
					continue;
				}

				if (chan->common.state != WAN_CONNECTED){
					DEBUG_TEST("%s: Warning: ignoring rx error intr: dev disc!\n",
						wan_netif_name(chan->common.dev));
					continue;
				}

				chan->if_stats.rx_fifo_errors++;
				chan->errstats.Rx_overrun_err_count++;

#if 0
				if (chan->hdlc_eng && WAN_NET_RATELIMIT()){
					u32 reg_lo, reg_hi,dma_descr;

					dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_LO;
					card->hw_iface.bus_read_4(card->hw,dma_descr, &reg_lo);

					dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
					card->hw_iface.bus_read_4(card->hw,dma_descr, &reg_hi);

					DEBUG_TEST("%s:%s: RX Fifo Error: LCh=%li TSlot=%d RxCompQ=%d RxFreeQ=%d RxDMA=%d CFG=0x%08X DmaLO: 0x%08X DmaHI: 0x%08X\n",
						card->devname,chan->if_name,chan->logic_ch_num,i,
						wan_skb_queue_len(&chan->wp_rx_complete_list),
						wan_skb_queue_len(&chan->wp_rx_free_list),
						chan->rx_dma,
						reg,reg_lo, reg_hi);
				}
#endif
				wan_set_bit(WP_FIFO_ERROR_BIT, &chan->pkt_error);

				err=-EINVAL;
			}
		}
	}

	return err;
}


static void front_end_interrupt(sdla_t *card, unsigned long reg,int lock)
{
	DEBUG_ISR("%s: front_end_interrupt!\n",card->devname);

	card->wandev.fe_iface.isr(&card->fe);
	if (lock){
		wan_smp_flag_t smp_flags;
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		handle_front_end_state(card);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	}else{
		handle_front_end_state(card);
	}
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
static WAN_IRQ_RETVAL wp_xilinx_isr (sdla_t* card)
{
	int i;
	u32 reg;
	u32 dma_tx_reg,dma_rx_reg,tx_fifo_status=0,rx_fifo_status=0;
	private_area_t *chan;
	WAN_IRQ_RETVAL_DECL(irq_ret);

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		WAN_IRQ_RETURN(irq_ret);
	}

	wan_set_bit(0,&card->in_isr);

	/*	write_cpld(card,LED_CONTROL_REG,0x0F);*/

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
		if (++card->u.aft.chip_security_cnt > AFT_MAX_CHIP_SECURITY_CNT){
			DEBUG_EVENT("%s: Critical: Chip Security Compromised: Disabling Driver!\n",
				card->devname);
			DEBUG_EVENT("%s: Please call Sangoma Tech Support (www.sangoma.com)!\n",
				card->devname);

			aft_critical_shutdown(card);
			WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
			goto isr_end;
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
#if defined(__LINUX__) || defined(__WINDOWS__)
			if (card->wandev.fe_iface.check_isr &&
				card->wandev.fe_iface.check_isr(&card->fe)){
					wan_set_bit(AFT_FE_INTR,&card->u.aft.port_task_cmd);
					WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));

					__aft_fe_intr_ctrl(card,0);
			}
#else
			front_end_interrupt(card,reg,0);
#endif
			WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
		}
	}

	/* Test Fifo Error Interrupt,
	* If set shutdown all interfaces and
	* reconfigure */
	if (wan_test_bit(ERROR_INTR_ENABLE_BIT,&reg)){
		if (wan_test_bit(ERROR_INTR_FLAG,&reg)){
#if 0
			DEBUG_EVENT("%s: ERR INTR (0x%X)\n",card->devname,reg);
#endif
			card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG,&tx_fifo_status);
			card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG,&rx_fifo_status);

			rx_fifo_status&=card->u.aft.active_ch_map;
			tx_fifo_status&=card->u.aft.active_ch_map;

			fifo_error_interrupt(card,reg,tx_fifo_status,rx_fifo_status);
			WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);
		}
	}

	/* -----------------2/6/2003 9:37AM------------------
	* Checking for Interrupt source:
	* 1. Receive DMA Engine
	* 2. Transmit DMA Engine
	* 3. Error conditions.
	* --------------------------------------------------*/
	if (wan_test_bit(GLOBAL_INTR_ENABLE_BIT,&reg) &&
		(wan_test_bit(DMA_INTR_FLAG,&reg) || rx_fifo_status) ){

			WAN_IRQ_RETVAL_SET(irq_ret, WAN_IRQ_HANDLED);

			/* Receive DMA Engine */
			card->hw_iface.bus_read_4(card->hw,
				XILINX_DMA_RX_INTR_PENDING_REG,
				&dma_rx_reg);

#if 0
			card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_TX_INTR_PENDING_REG,&tx_fifo_status);
			card->hw_iface.bus_read_4(card->hw,XILINX_HDLC_RX_INTR_PENDING_REG,&rx_fifo_status);

			rx_fifo_status&=card->u.aft.active_ch_map;
			tx_fifo_status&=card->u.aft.active_ch_map;
#endif

			DEBUG_ISR("%s: DMA_RX_INTR_REG(0x%X) = 0x%X\n",
				card->devname,
				XILINX_DMA_RX_INTR_PENDING_REG,dma_rx_reg);

			dma_rx_reg&=card->u.aft.active_ch_map;

			if (dma_rx_reg == 0 && rx_fifo_status == 0){
				goto isr_skb_rx;
			}

			for (i=0; i<card->u.aft.num_of_time_slots ;i++){
				if ((wan_test_bit(i,&dma_rx_reg) || wan_test_bit(i,&rx_fifo_status)) &&
					wan_test_bit(i,&card->u.aft.logic_ch_map)){

						chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
						if (!chan){
							DEBUG_EVENT("%s: Error: No Dev for Rx logical ch=%d\n",
								card->devname,i);
							continue;
						}

						if (!wan_test_bit(0,&chan->up)){
							DEBUG_EVENT("%s: Error: Dev not up for Rx logical ch=%d\n",
								card->devname,i);
							continue;
						}
#if 0
						chan->if_stats.rx_frame_errors++;
#endif

						if (wan_test_bit(i, &rx_fifo_status)){
							/* If fifo error occured, the rx descriptor
							* was already handled, thus the rx pending
							* should just be disregarded */
							chan->if_stats.rx_frame_errors++;
							wan_set_bit(WP_FIFO_ERROR_BIT, &chan->pkt_error);
						}

						DEBUG_RX("%s: RX Interrupt pend. \n",
							card->devname);

#if 0
						/* NCDEBUG */
						chan->if_stats.tx_fifo_errors=wan_skb_queue_len(&chan->wp_tx_pending_list);
						chan->if_stats.rx_fifo_errors=wan_skb_queue_len(&chan->wp_rx_free_list);
#endif

						xilinx_dma_rx_complete(card,chan);

						if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){

#if 0
							/* Never check interrupt here
							* since it breaks channelization */
							card->hw_iface.bus_read_4(card->hw,
								XILINX_DMA_TX_INTR_PENDING_REG,
								&dma_tx_reg);
#endif
							xilinx_dma_tx_complete(card,chan);
						}
				}
			}
isr_skb_rx:

			/* Transmit DMA Engine */

			card->hw_iface.bus_read_4(card->hw,
				XILINX_DMA_TX_INTR_PENDING_REG,
				&dma_tx_reg);

			dma_tx_reg&=card->u.aft.active_ch_map;

			DEBUG_ISR("%s: DMA_TX_INTR_REG(0x%X) = 0x%X\n",
				card->devname,
				XILINX_DMA_RX_INTR_PENDING_REG,dma_tx_reg);

			if (dma_tx_reg == 0){
				goto isr_skb_tx;
			}

			for (i=0; i<card->u.aft.num_of_time_slots ;i++){
				if (wan_test_bit(i,&dma_tx_reg) && wan_test_bit(i,&card->u.aft.logic_ch_map)){
					chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
					if (!chan){
						DEBUG_EVENT("%s: Error: No Dev for Tx logical ch=%d\n",
							card->devname,i);
						continue;
					}

					if (!wan_test_bit(0,&chan->up)){
						DEBUG_EVENT("%s: Error: Dev not up for Tx logical ch=%d\n",
							card->devname,i);
						continue;
					}

					if (chan->common.usedby == TDM_VOICE ||chan->common.usedby == TDM_VOICE_API) {
						continue;
					}

					DEBUG_TX(" ---- TX Interrupt pend. --\n");
					xilinx_dma_tx_complete(card,chan);
				}
			}
	}

isr_skb_tx:

	/* -----------------2/6/2003 10:36AM-----------------
	*    Finish of the interupt handler
	* --------------------------------------------------*/
isr_end:

	/*	write_cpld(card,LED_CONTROL_REG,0x0E); */

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
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
								private_area_t* chan, int local_dev )
{
	unsigned short buffer_length;
	wan_udp_pkt_t *wan_udp_pkt;
	wan_trace_t *trace_info=NULL;
	wp_tdm_chan_stats_t *wp_tdm_chan_stats=NULL;

	wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;

	if (wan_atomic_read(&chan->udp_pkt_len) == 0){
		return -ENODEV;
	}

	trace_info=&chan->trace_info;
	wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;
	wp_tdm_chan_stats = (wp_tdm_chan_stats_t*)wan_udp_pkt->wan_udp_data;

	{
		netskb_t *skb;

		wan_udp_pkt->wan_udp_opp_flag = 0;

		switch(wan_udp_pkt->wan_udp_command) {

		case WANPIPEMON_READ_CONFIGURATION:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			wan_udp_pkt->wan_udp_data_len=0;
			break;

		case WANPIPEMON_READ_CODE_VERSION:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			card->hw_iface.getcfg(card->hw, SDLA_COREREV, &wan_udp_pkt->wan_udp_data[0]);
			wan_udp_pkt->wan_udp_data_len=1;
			break;

		case WANPIPEMON_AFT_LINK_STATUS:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			wan_udp_pkt->wan_udp_data[0] = card->wandev.state;/* WAN_CONNECTED/WAN_DISCONNECTED */
			wan_udp_pkt->wan_udp_data_len = 1;
			break;

		case WANPIPEMON_DIGITAL_LOOPTEST:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			DEBUG_EVENT("Ready to send some data!!!\n");
			break;

		case WANPIPEMON_AFT_MODEM_STATUS:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			if (card->wandev.state == WAN_CONNECTED){
				wan_udp_pkt->wan_udp_data[0]=0x28;
			}else{
				wan_udp_pkt->wan_udp_data[0]=0;
			}
			wan_udp_pkt->wan_udp_data_len=1;
			break;

		case WANPIPEMON_READ_OPERATIONAL_STATS:
#if defined(__WINDOWS__)
			memset(wp_tdm_chan_stats, 0x00, sizeof(wp_tdm_chan_stats_t));

			wp_tdm_chan_stats->rx_bytes = chan->opstats.Data_bytes_Rx_count;
			wp_tdm_chan_stats->rx_packets = chan->opstats.Data_frames_Rx_count;

			wp_tdm_chan_stats->tx_bytes = chan->opstats.Data_bytes_Tx_count;;
			wp_tdm_chan_stats->tx_packets = chan->opstats.Data_frames_Tx_count;
			
			wan_udp_pkt->wan_udp_data_len=sizeof(wp_tdm_chan_stats_t);
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
#else
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			memcpy(wan_udp_pkt->wan_udp_data,&chan->opstats,sizeof(aft_op_stats_t));
			wan_udp_pkt->wan_udp_data_len=sizeof(aft_op_stats_t);
#endif
			break;

		case WANPIPEMON_FLUSH_OPERATIONAL_STATS:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			memset(&chan->opstats,0,sizeof(aft_op_stats_t));
			wan_udp_pkt->wan_udp_data_len=0;
			break;

		case WANPIPEMON_READ_COMMS_ERROR_STATS:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			memcpy(wan_udp_pkt->wan_udp_data,&chan->errstats,sizeof(aft_comm_err_stats_t));
			wan_udp_pkt->wan_udp_data_len=sizeof(aft_comm_err_stats_t);
			break;

		case WANPIPEMON_FLUSH_COMMS_ERROR_STATS:
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			memset(&chan->errstats,0,sizeof(aft_comm_err_stats_t));
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
				wan_set_bit (0,&trace_info->tracing_enabled);

			}else{
				DEBUG_EVENT("%s: Error: AFT trace running!\n",
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

				DEBUG_UDP("%s: Disabling AFT trace\n",
					card->devname);

			}else{
				/* set return code to line trace already
				disabled */
				wan_udp_pkt->wan_udp_return_code = 1;
			}
			break;

		case WANPIPEMON_GET_TRACE_INFO:

			if(wan_test_bit(0,&trace_info->tracing_enabled)){
				trace_info->trace_timeout = SYSTEM_TICKS;
			}else{
				DEBUG_EVENT("%s: Error AFT trace not enabled\n",
					card->devname);
				/* set return code */
				wan_udp_pkt->wan_udp_return_code = 1;
				break;
			}

			buffer_length = 0;
			wan_udp_pkt->wan_udp_atm_num_frames = 0;
			wan_udp_pkt->wan_udp_atm_ismoredata = 0;

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
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
#elif defined(__WINDOWS__)
			{
			wan_smp_flag_t	smp_flags;
			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			skb=skb_dequeue(&trace_info->trace_queue);
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

			if(skb){
				memcpy(&wan_udp_pkt->wan_udp_data[buffer_length], 
				       wan_skb_data(skb),
				       wan_skb_len(skb));
		     
				buffer_length += (unsigned short)wan_skb_len(skb);
				wan_skb_free(skb);
				wan_udp_pkt->wan_udp_aft_num_frames++;

				/* NOT locking check of queue length intentionally! */
				if(wan_skb_queue_len(&trace_info->trace_queue) > 0){

					if(wan_skb_queue_len(&trace_info->trace_queue)){
						/* indicate there are more frames on board & exit */
						wan_udp_pkt->wan_udp_aft_ismoredata = 0x01;
					}else{
						wan_udp_pkt->wan_udp_aft_ismoredata = 0x00;
					}
				}
			}/* if(skb) */
			}
#endif
			/* set the data length and return code */
			wan_udp_pkt->wan_udp_data_len = buffer_length;
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			break;

		case WANPIPEMON_ROUTER_UP_TIME:
			wan_getcurrenttime(&chan->router_up_time, NULL);
			chan->router_up_time -= chan->router_start_time;
			*(wan_time_t *)&wan_udp_pkt->wan_udp_data =
				chan->router_up_time;
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			break;

		case WAN_GET_MEDIA_TYPE:
		case WAN_FE_GET_STAT:
		case WAN_FE_LB_MODE:
		case WAN_FE_FLUSH_PMON:
		case WAN_FE_GET_CFG:
		case WAN_FE_SET_DEBUG_MODE:
		case WAN_FE_TX_MODE:
			if (IS_TE1_CARD(card)){
				wan_smp_flag_t smp_flags;
				card->hw_iface.hw_lock(card->hw,&smp_flags);
				card->wandev.fe_iface.process_udp(
					&card->fe,
					&wan_udp_pkt->wan_udp_cmd,
					&wan_udp_pkt->wan_udp_data[0]);
				card->hw_iface.hw_unlock(card->hw,&smp_flags);
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
			wan_udp_pkt->wan_udp_data[0] = WAN_PLATFORM_ID;
			wan_udp_pkt->wan_udp_return_code = CMD_OK;
			wan_udp_pkt->wan_udp_data_len = 1;
			break;

		case WAN_GET_MASTER_DEV_NAME:
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
			break;

#if defined(__WINDOWS__)
		case WAN_GET_HW_MAC_ADDR:
			/*	There is no MAC address on AFT hardware, simply create a random MAC.
			Only S518 ADSL provides built-in MAC addrr (sdla_adsl.c). */
			wan_get_random_mac_address(wan_udp_pkt->wan_udp_data);
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			wan_udp_pkt->wan_udp_data_len = ETHER_ADDR_LEN;
			break;

		case WANPIPEMON_GET_BIOS_ENCLOSURE3_SERIAL_NUMBER:
			wan_udp_pkt->wan_udp_return_code = 
				wp_get_motherboard_enclosure_serial_number(wan_udp_pkt->wan_udp_data, 
						sizeof(wan_udp_pkt->wan_udp_data));

			wan_udp_pkt->wan_udp_data_len = sizeof(wan_udp_pkt->wan_udp_data);
			break;
#endif
		default:
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = WAN_UDP_INVALID_NET_CMD;

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

	if (card->wandev.state == state){
		return;
	}

	switch (state)
	{
	case WAN_CONNECTED:
		DEBUG_TEST( "%s: Link connected!\n",
			card->devname);
		break;

	case WAN_CONNECTING:
		DEBUG_TEST( "%s: Link connecting...\n",
			card->devname);
		break;

	case WAN_DISCONNECTED:
		DEBUG_TEST( "%s: Link disconnected!\n",
			card->devname);
		break;
	}

	card->wandev.state = state;
#if defined(__WINDOWS__)
	{
		int i;
		for (i = 0; i < NUM_OF_E1_CHANNELS; i++){
			if(card->sdla_net_device[i] != NULL && wan_test_bit(0,&card->up[i]) ){
				set_chan_state(card, card->sdla_net_device[i], state);
			}
		}
	}
#else
	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (dev){
			set_chan_state(card, dev, state);
		}
	}
#endif
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

#if defined(__WINDOWS__)
	DBG_LIP_OOB("%s(): IRQL: %s, card->fe.fe_status: %s\n", __FUNCTION__,
		decode_irql(KeGetCurrentIrql()), FE_STATUS_DECODE(card->fe.fe_status));
#endif

	if (card->wandev.ignore_front_end_status == WANOPT_YES){
		return;
	}

	if (!wan_test_bit(AFT_CHIP_CONFIGURED,&card->u.aft.chip_cfg_status)&&
		card->fe.fe_status == FE_CONNECTED){
			DEBUG_TEST("%s: Skipping Front Front End State = %x\n",
				card->devname,card->fe.fe_status);

			wan_set_bit(AFT_FRONT_END_UP,&card->u.aft.chip_cfg_status);
			return;
	}

	if (card->fe.fe_status == FE_CONNECTED){

		if (!wan_test_bit(0,&card->u.aft.comm_enabled)){
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
			if (card->wan_tdmv.sc){
				int	err;
				WAN_TDMV_CALL(state, (card, WAN_CONNECTED), err);
			}
#endif
			enable_data_error_intr(card);
			port_set_state(card,WAN_CONNECTED);

			aft_red_led_ctrl(card, AFT_LED_OFF);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_ON);

			if (card->u.aft.cfg.rbs){
				if (card->wandev.fe_iface.set_fe_sigctrl){
					card->wandev.fe_iface.set_fe_sigctrl(
						&card->fe,
						WAN_TE_SIG_INTR,
						ENABLE_ALL_CHANNELS,
						WAN_ENABLE);
				}
			}

		} else if (card->wandev.state != WAN_CONNECTED &&
			wan_test_bit(0,&card->u.aft.comm_enabled)){

				disable_data_error_intr(card,LINK_DOWN);

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
				if (card->wan_tdmv.sc){
					int	err;
					WAN_TDMV_CALL(state, (card, WAN_CONNECTED), err);
				}
#endif
				enable_data_error_intr(card);
				card->wandev.state = WAN_CONNECTED;
				aft_red_led_ctrl(card, AFT_LED_OFF);
				card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_ON);

				DEBUG_EVENT("%s: AFT Front End Restart!\n",
					card->devname);
		}else{
			DEBUG_TEST("%s(): line: %d: unhandled case!\n", __FUNCTION__, __LINE__);
		}

	}else{

#ifndef __WINDOWS__
		if (card->tdmv_conf.span_no){

			/* If running in TDMV voice mode, just note
			* that state went down, but keep the
			* connection up */
			card->wandev.state = WAN_DISCONNECTED;
			aft_red_led_ctrl(card, AFT_LED_ON);
			card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
			if (card->wan_tdmv.sc){
				int	err;
				WAN_TDMV_CALL(state, (card, WAN_DISCONNECTED), err);
			}
#endif
			return;
		}
#endif
		if (wan_test_bit(0,&card->u.aft.comm_enabled) ||
			card->wandev.state != WAN_DISCONNECTED){
		
				port_set_state(card,WAN_DISCONNECTED);
				disable_data_error_intr(card,LINK_DOWN);
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
				if (card->wan_tdmv.sc){
					int	err;
					WAN_TDMV_CALL(state, (card, WAN_DISCONNECTED), err);
				}
#endif
				aft_red_led_ctrl(card, AFT_LED_ON);
				card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
		}else{
			DEBUG_TEST("%s(): line: %d: unhandled case!\n", __FUNCTION__, __LINE__);
		}
	}
	return;
}

static unsigned char read_cpld(sdla_t *card, unsigned short cpld_off)
{

	u16     org_off;
	u8      tmp;

	cpld_off &= ~BIT_DEV_ADDR_CLEAR;
	cpld_off |= BIT_DEV_ADDR_CPLD;

	/*Save the current address. */
	card->hw_iface.bus_read_2(card->hw,
		XILINX_MCPU_INTERFACE_ADDR,
		&org_off);

	card->hw_iface.bus_write_2(card->hw,
		XILINX_MCPU_INTERFACE_ADDR,
		cpld_off);

	card->hw_iface.bus_read_1(card->hw,XILINX_MCPU_INTERFACE, &tmp);

	/*Restore original address */
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

	/* Save the current original address */
	card->hw_iface.bus_read_2(card->hw,
		XILINX_MCPU_INTERFACE_ADDR,
		&org_off);

	card->hw_iface.bus_write_2(card->hw,
		XILINX_MCPU_INTERFACE_ADDR,
		off);

	/* This delay is required to avoid bridge optimization
	* (combining two writes together)*/
	WP_DELAY(5);

	card->hw_iface.bus_write_1(card->hw,
		XILINX_MCPU_INTERFACE,
		data);

	/* This delay is required to avoid bridge optimization
	* (combining two writes together)*/
	WP_DELAY(5);

	/* Restore the original address */
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
	DEBUG_EVENT("%s: Reading Bar%d Offset=0x%X Len=%d\n",
		card->devname,api_cmd->bar,api_cmd->offset,api_cmd->len);
#endif

	return 0;
}

static int xilinx_api_fe_read(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	wan_smp_flag_t	smp_flags;
	int			qaccess = card->wandev.state == WAN_CONNECTED ? 1 : 0;

	card->hw_iface.hw_lock(card->hw,&smp_flags);
	api_cmd->data[0] = card->fe.read_fe_reg(card->hw, qaccess, 1, (int)api_cmd->offset);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);

#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Reading Bar%d Offset=0x%X Len=%d\n",
		card->devname,api_cmd->bar,api_cmd->offset,api_cmd->len);
#endif
	return 0;
}


static int xilinx_write(sdla_t *card, wan_cmd_api_t *api_cmd)
{
#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Writting Bar%d Offset=0x%X Len=%d\n",
		card->devname,
		api_cmd->bar,api_cmd->offset,api_cmd->len);
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


static int xilinx_api_fe_write(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	wan_smp_flag_t	smp_flags;
	int			err, qaccess;

	qaccess = card->wandev.state == WAN_CONNECTED ? 1 : 0;
#ifdef DEB_XILINX
	DEBUG_EVENT("%s: Writting Bar%d Offset=0x%X Len=%d\n",
		card->devname,
		api_cmd->bar,api_cmd->offset,api_cmd->len);
#endif

	card->hw_iface.hw_lock(card->hw,&smp_flags);
	err = card->fe.write_fe_reg(card->hw, qaccess, 1, (int)api_cmd->offset, (int)api_cmd->data[0]);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);

	return err;
}


static int xilinx_write_bios(sdla_t *card, wan_cmd_api_t *api_cmd)
{
#ifdef DEB_XILINX
	DEBUG_EVENT("Setting PCI 0xX=0x%08lX   0x3C=0x%08X\n",
		(card->wandev.S514_cpu_no[0] == SDLA_CPU_A) ? 0x10 : 0x14,
		card->u.aft.bar,card->wandev.irq);
#endif
	card->hw_iface.pci_write_config_dword(card->hw,
		(card->wandev.S514_cpu_no[0] == SDLA_CPU_A) ? 0x10 : 0x14,
		card->u.aft.bar);
	card->hw_iface.pci_write_config_dword(card->hw, 0x3C, card->wandev.irq);
	card->hw_iface.pci_write_config_dword(card->hw, 0x0C, 0x0000ff00);

	return 0;
}
 
static int aft_devel_ioctl(sdla_t *card, struct ifreq *ifr)
{
	wan_cmd_api_t	*api_cmd;
	wan_cmd_api_t	api_cmd_struct;
	int		err = -EINVAL;
	void 	*ifr_data_ptr;

#if defined (__WINDOWS__)
	ifr_data_ptr = ifr;
#else
	if (!ifr || !ifr->ifr_data){
		DEBUG_EVENT("%s: Error: No ifr or ifr_data\n",__FUNCTION__);
		return -EINVAL;
	}
	ifr_data_ptr = ifr->ifr_data;
#endif

	api_cmd = &api_cmd_struct;

	memset(api_cmd, 0, sizeof(wan_cmd_api_t));	

	if (WAN_COPY_FROM_USER(api_cmd, ifr_data_ptr, sizeof(wan_cmd_api_t))){
		return -EFAULT;
	}

	switch(api_cmd->cmd){
	case SIOC_WAN_READ_REG:
		err=xilinx_read(card,api_cmd);
		break;
		
	case SIOC_WAN_WRITE_REG:
		err=xilinx_write(card,api_cmd);
		break;
		
	case SIOC_WAN_FE_READ_REG:
		err=xilinx_api_fe_read(card,api_cmd);
		break;
		
	case SIOC_WAN_FE_WRITE_REG:
		err=xilinx_api_fe_write(card,api_cmd);
		break;
		
	case SIOC_WAN_SET_PCI_BIOS:
#if defined(__WINDOWS__)
		/* Restore PCI config space to what it was
		 * before the firmware update, so access to card
		 * is possible again. */
		err=sdla_restore_pci_config_space(card);
#else
		err=xilinx_write_bios(card,api_cmd);
#endif
		break;

#if defined(__WINDOWS__)
	case SIOC_WAN_GET_CARD_TYPE:
		api_cmd->len = 0;

		err = card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE,	 (u16*)&api_cmd->data[0]);
		if(err){
			break;
		}

		err = card->hw_iface.getcfg(card->hw, SDLA_ADAPTERSUBTYPE, (u16*)&api_cmd->data[sizeof(u16)]);
		if(err){
			break;
		}

		api_cmd->len = 2*sizeof(u16);
		break;
#endif

	case SIOC_WAN_COREREV:
		api_cmd->len = 0;
		
		err = card->hw_iface.getcfg(card->hw, SDLA_COREREV, (u8*)&api_cmd->data[0]);
		if(err){
			break;
		}
		
		api_cmd->len = sizeof(u8);
		break;

	default:
		DEBUG_ERROR("%s(): Error: unsupported cmd: %d\n", __FUNCTION__, api_cmd->cmd);
		err = -EINVAL;
		break;

	}

	api_cmd->ret = err;

	if (WAN_COPY_TO_USER(ifr_data_ptr, api_cmd, sizeof(wan_cmd_api_t))){
		return -EFAULT;
	}

	return err;
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
	int i;

	DEBUG_CFG("%s: !!!\n",__FUNCTION__);

	/* Clean Tx/Rx DMA interrupts */

	card->hw_iface.bus_read_4(card->hw,
		XILINX_DMA_RX_INTR_PENDING_REG, &reg);
	card->hw_iface.bus_read_4(card->hw,
		XILINX_DMA_TX_INTR_PENDING_REG, &reg);

	for (i=0; i<card->u.aft.num_of_time_slots;i++){
		private_area_t *chan;

		if (!wan_test_bit(i,&card->u.aft.logic_ch_map)){
			continue;
		}

		chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
		if (!chan){
			continue;
		}

		if (!wan_test_bit(0,&chan->up)){
			continue;
		}

		DEBUG_TEST("%s: Init interface fifo no wait %s\n",
			__FUNCTION__,chan->if_name);

		xilinx_init_rx_dev_fifo(card, chan, WP_NO_WAIT);
		xilinx_init_tx_dev_fifo(card, chan, WP_NO_WAIT);


		if (wan_test_bit(0,&chan->tdmv_sync)){
			channel_timeslot_sync_ctrl(card,chan,1);
		}
	}

	/* Enable DMA controler, in order to start the
	* fifo cleaning */
	card->hw_iface.bus_read_4(card->hw,XILINX_DMA_CONTROL_REG,&reg);
	wan_set_bit(DMA_ENGINE_ENABLE_BIT,&reg);
	card->hw_iface.bus_write_4(card->hw,XILINX_DMA_CONTROL_REG,reg);


	/* For all channels clean Tx/Rx fifos */
	for (i=0; i<card->u.aft.num_of_time_slots;i++){
		private_area_t *chan;

		if (!wan_test_bit(i,&card->u.aft.logic_ch_map)){
			continue;
		}

		chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
		if (!chan){
			continue;
		}

		if (!wan_test_bit(0,&chan->up)){
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

	for (i=0; i<card->u.aft.num_of_time_slots;i++){
		private_area_t *chan;
		netskb_t *skb;

		if (!wan_test_bit(i,&card->u.aft.logic_ch_map)){
			continue;
		}

		chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
		if (!chan){
			continue;
		}

		if (!wan_test_bit(0,&chan->up)){
			continue;
		}

		DEBUG_TEST("%s: Init interface %s\n",__FUNCTION__,chan->if_name);

		if (chan->rx_dma_skb){
			wp_rx_element_t *rx_el;
			netskb_t *skb=chan->rx_dma_skb;
			DEBUG_TEST("%s: Clearing RX DMA Pending buffer \n",__FUNCTION__);

			chan->rx_dma_skb=NULL;
			rx_el=(wp_rx_element_t *)wan_skb_data(skb);

			card->hw_iface.pci_unmap_dma(card->hw,
				rx_el->dma_addr,
				chan->dma_mru,
				PCI_DMA_FROMDEVICE);

			aft_init_requeue_free_skb(chan, skb);
		}

		while((skb=wan_skb_dequeue(&chan->wp_rx_complete_list)) != NULL){
			aft_init_requeue_free_skb(chan, skb);
			chan->if_stats.rx_errors++;
		}

		if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
			while((skb=wan_skb_dequeue(&chan->wp_tx_pending_list)) != NULL){
				aft_init_requeue_free_skb(chan, skb);
				chan->if_stats.rx_errors++;
			}
		}

		xilinx_dma_rx(card,chan);

		if (chan->tx_dma_addr && chan->tx_dma_len){
			DEBUG_TEST("%s: Clearing TX DMA Pending buffer \n",__FUNCTION__);
			aft_unmap_tx_dma(card,chan);
		}


		if (chan->tx_dma_skb){
			if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
				aft_init_requeue_free_skb(chan, chan->tx_dma_skb);
			}else{
				wan_aft_skb_defered_dealloc(chan,chan->tx_dma_skb);/* instead of wan_skb_free(chan->tx_dma_skb); */
			}
			chan->tx_dma_skb=NULL;
		}

		wan_clear_bit(TX_BUSY,&chan->dma_status);
		wan_clear_bit(0,&chan->idle_start);

		if (!chan->hdlc_eng){
			int err=xilinx_dma_tx(card,chan);
			if (err){
				DEBUG_EVENT("%s: DMA Tx failed err=%i\n",
					chan->if_name,err);
			}
		}

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

	if (wan_test_bit(0,&card->u.aft.tdmv_sync)){
		rx_chan_timeslot_sync_ctrl(card,1);
	}

	wan_set_bit(0,&card->u.aft.comm_enabled);

	DEBUG_TEST("%s: END !!!\n",__FUNCTION__);

}

static void disable_data_error_intr(sdla_t *card, enum wp_device_down_states event)
{
	u32 reg;

	DEBUG_TEST("%s(): !!!!!!!\n",__FUNCTION__);

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

	if (event==DEVICE_DOWN){
		wan_set_bit(CARD_DOWN,&card->wandev.critical);
	}

	wan_clear_bit(0,&card->u.aft.comm_enabled);
}

static void xilinx_init_tx_dma_descr(sdla_t *card, private_area_t *chan)
{
	u32 dma_descr;
	u32 reg=0;

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
		wan_smp_flag_t smp_flags;
		card->hw_iface.hw_lock(card->hw,&smp_flags);
		card->wandev.fe_iface.read_alarm(&card->fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		/* TE1 Update T1/E1 perfomance counters */
		card->wandev.fe_iface.read_pmon(&card->fe, 0);
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
	}

	return 0;
}

#if 0
static void xilinx_rx_fifo_over_recover(sdla_t *card, private_area_t *chan)
{

	u32 reg=0;
	u32 dma_descr;
	netskb_t *skb;

	/* Initialize Tx DMA descriptor: Stop DMA */
	dma_descr=(chan->logic_ch_num<<4) + XILINX_RxDMA_DESCRIPTOR_HI;
	card->hw_iface.bus_write_4(card->hw,dma_descr, reg);

	/* Clean the RX FIFO */
	rx_chan_timeslot_sync_ctrl(card,0);
	xilinx_init_rx_dev_fifo(card, chan, WP_WAIT);

	if ((skb=chan->rx_dma_skb) != NULL){
		wp_rx_element_t *rx_el;
		rx_el=(wp_rx_element_t *)wan_skb_data(chan->rx_dma_skb);

		card->hw_iface.pci_unmap_dma(card->hw,
			rx_el->dma_addr,
			chan->dma_mru,
			PCI_DMA_FROMDEVICE);

		DEBUG_RX("%s:%s: RX HI=0x%X  LO=0x%X DMA=0x%lX\n",
			__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);

		chan->rx_dma_skb=NULL;
		aft_init_requeue_free_skb(chan, skb);
	}

	wan_clear_bit(0,&chan->rx_dma);
	aft_dma_rx(card,chan);
	rx_chan_timeslot_sync_ctrl(card,1);

}
#endif

static void xilinx_tx_fifo_under_recover (sdla_t *card, private_area_t *chan)
{
	DEBUG_TEST("%s:%s: Tx Fifo Recovery \n",card->devname,chan->if_name);

	if (chan->common.usedby == TDM_VOICE || chan->common.usedby == TDM_VOICE_API){
		return;
	}

#if 0
	/* FIFO Already empty no need to clean the fifo */
	if (chan->hdlc_eng){
		/* Initialize Tx DMA descriptor: Stop DMA */
		dma_descr=(chan->logic_ch_num<<4) + XILINX_TxDMA_DESCRIPTOR_HI;
		card->hw_iface.bus_write_4(card->hw,dma_descr, reg);

		/* Clean the TX FIFO */
		xilinx_init_tx_dev_fifo(card, chan, WP_WAIT);
	}
#endif

	if (chan->tx_dma_addr && chan->tx_dma_len){
		aft_unmap_tx_dma(card,chan);
	}


	/* Requeue the current tx packet, for
	* re-transmission */
	if (chan->tx_dma_skb){
		wan_skb_queue_head(&chan->wp_tx_pending_list, chan->tx_dma_skb);
		chan->tx_dma_skb=NULL;
		if (chan->lip_atm) {
			netskb_t *tmpskb = chan->tx_dma_skb;
			chan->tx_dma_skb=NULL;
			wan_aft_skb_defered_dealloc(chan, tmpskb); /* instead of wan_skb_free(tmpskb); */
		} else {
			wan_skb_queue_head(&chan->wp_tx_pending_list, chan->tx_dma_skb);
			chan->tx_dma_skb=NULL;
		}
	}

	/* Wake up the stack, because tx dma interrupt failed */
	wanpipe_wake_stack(chan);

	if (!chan->hdlc_eng){
		if (wan_test_bit(0,&chan->idle_start)){
			++chan->if_stats.tx_fifo_errors;
		}
	}else{
		++chan->if_stats.tx_fifo_errors;
	}

	DEBUG_TEST("%s:%s: Tx Fifo Recovery: Restarting Transmission \n",
		card->devname,chan->if_name);

	/* Re-start transmission */
	wan_clear_bit(TX_BUSY,&chan->dma_status);

	xilinx_dma_tx(card,chan);
}

static int xilinx_write_ctrl_hdlc(sdla_t *card, u32 timeslot, u8 reg_off, u32 data)
{
	u32		reg;
	u32		ts_orig=timeslot;
	wan_ticks_t	timeout=SYSTEM_TICKS;

	if (timeslot == 0){
		timeslot=card->u.aft.num_of_time_slots-2;
	}else if (timeslot == 1){
		timeslot=card->u.aft.num_of_time_slots-1;
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

		if ((SYSTEM_TICKS-timeout) > 1){
			DEBUG_EVENT("%s: Warning: Access to timeslot %d timed out!\n",
				card->devname,ts_orig);
			break;
		}
	}

	card->hw_iface.bus_write_4(card->hw,reg_off,data);

	return 0;
}

static int set_chan_state(sdla_t* card, netdevice_t* dev, int state)
{
	private_area_t *chan = wan_netif_priv(dev);
	private_area_t *ch_ptr;

	if (chan == NULL) {
		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT("%s: %s:%d No chan ptr!\n",
				card->devname,__FUNCTION__,__LINE__);
		}
		/* This is case can happened for WANPIPE (LITE) */
		return 0;
	}

#if defined(__WINDOWS__)
	set_netdev_state(card, dev, state);
#endif

	chan->common.state = state;

	for (ch_ptr=chan; ch_ptr != NULL; ch_ptr=ch_ptr->next){
		ch_ptr->common.state=state;
		if (ch_ptr->tdmv_zaptel_cfg) {
			continue;
		}
#ifdef AFT_TDM_API_SUPPORT
		if (ch_ptr->common.usedby == TDM_VOICE_API ||
			ch_ptr->common.usedby == TDM_VOICE_DCHAN) {
				aft_tdm_api_update_state_channelized(card, ch_ptr, state);
		}
#endif
	}

	if (state == WAN_CONNECTED){
		DEBUG_TEST("%s: Setting idle_start to 0\n",
			chan->if_name);
		wan_clear_bit(0,&chan->idle_start);
		chan->opstats.link_active_count++;

		WAN_NETIF_CARRIER_ON(dev);
		WAN_NETIF_WAKE_QUEUE(dev);
	}else{
		chan->opstats.link_inactive_modem_count++;
		WAN_NETIF_CARRIER_OFF(dev);
		WAN_NETIF_STOP_QUEUE(dev);
	}

# if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
#if defined(__LINUX__)
	if (chan->common.usedby == API){
		wan_update_api_state(chan);
	}
#endif
	if (chan->common.usedby == STACK){

		if (state == WAN_CONNECTED){
			wanpipe_lip_connect(chan,0);
		}else{
			wanpipe_lip_disconnect(chan,0);
		}
	}
#endif
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
			DEBUG_EVENT("%s:%s: Invalid number of timeslots %d\n",
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
			DEBUG_EVENT("%s:%s: Invalid number of timeslots %d\n",
				card->devname,chan->if_name,chan->num_of_time_slots);
			return -EINVAL;
		}
	}

	DEBUG_TEST("%s:%s: Optimal Fifo Size =%d  Timeslots=%d \n",
		card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots);

	fifo_size=map_fifo_baddr_and_size(card,req_fifo_size,&chan->fifo_base_addr);
	if (fifo_size == 0 || chan->fifo_base_addr == 31){
		DEBUG_EVENT("%s:%s: Error: Failed to obtain fifo size %d or addr %d \n",
			card->devname,chan->if_name,fifo_size,chan->fifo_base_addr);
		return -EINVAL;
	}

	DEBUG_TEST("%s:%s: Optimal Fifo Size =%d  Timeslots=%d New Fifo Size=%d \n",
		card->devname,chan->if_name,req_fifo_size,chan->num_of_time_slots,fifo_size);


	for (i=0;i<sizeof(fifo_size_vector);i++){
		if (fifo_size_vector[i] == fifo_size){
			chan->fifo_size_code=fifo_code_vector[i];
			break;
		}
	}

	if (fifo_size != req_fifo_size){
		DEBUG_EVENT("%s:%s: Warning: Failed to obtain the req fifo %d got %d\n",
			card->devname,chan->if_name,req_fifo_size,fifo_size);
	}

	DEBUG_TEST("%s: %s:Fifo Size=%d  Timeslots=%d Fifo Code=%d Addr=%d\n",
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
		card->devname,reg,card->u.aft.fifo_addr_map);

	for (i=0;i<32;i+=fifo_size){
		if (card->u.aft.fifo_addr_map & (reg<<i)){
			continue;
		}
		card->u.aft.fifo_addr_map |= reg<<i;
		*addr=i;

		DEBUG_TEST("%s: Card fifo Map 0x%lX Addr =%d\n",
			card->devname,card->u.aft.fifo_addr_map,i);

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
		card->devname,reg<<chan->fifo_base_addr, card->u.aft.fifo_addr_map);

	card->u.aft.fifo_addr_map &= ~(reg<<chan->fifo_base_addr);

	DEBUG_TEST("%s: New Map is 0x%lX\n",
		card->devname, card->u.aft.fifo_addr_map);


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
				WP_DELAY(500);
				/* WARNING: we cannot do this while in
				* critical area */
			}
		}else{
			return 0;
		}
	}

	return -EINVAL;
}


static void aft_unmap_tx_dma(sdla_t *card, private_area_t *chan)
{

	card->hw_iface.pci_unmap_dma(card->hw,
		chan->tx_dma_addr,
		chan->tx_dma_len,
		PCI_DMA_TODEVICE);

	chan->tx_dma_addr=0;
	chan->tx_dma_len=0;

}

static int protocol_init (sdla_t *card, netdevice_t *dev,
						  private_area_t *chan,
						  wanif_conf_t* conf)
{

	chan->common.protocol = conf->protocol;

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC

	DEBUG_EVENT("%s: AFT Driver doesn't directly support any protocols!\n",
		chan->if_name);
	return -EPROTONOSUPPORT;

#else
	if (chan->common.protocol == WANCONFIG_PPP ||
		chan->common.protocol == WANCONFIG_CHDLC){

			struct ifreq		ifr;
			struct if_settings	ifsettings;

			chan->common.is_netdev = 0;
			if (wan_iface.attach){
				wan_iface.attach(dev, wan_netif_name(dev), chan->common.is_netdev);
			}else{
				DEBUG_EVENT("%s: Failed to attach interface!\n",
					chan->if_name);
				return -EINVAL;
			}
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
			if (!wan_iface.set_proto || wan_iface.set_proto(dev, &ifr)){
				if (wan_iface.detach){
					wan_iface.detach(dev, chan->common.is_netdev);
				}
				if (wan_iface.free){
					wan_iface.free(dev);
				}
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

#ifdef CONFIG_PRODUCT_WANPIPE_GENERIC
	if (chan->common.protocol == WANCONFIG_PPP ||
		chan->common.protocol == WANCONFIG_CHDLC){

			chan->common.prot_ptr = NULL;
			wanpipe_generic_unregister(dev);
			if (wan_iface.detach){
				wan_iface.detach(dev, chan->common.is_netdev);
			}
			if (wan_iface.free){
				wan_iface.free(dev);
			}

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

	if (wan_iface.input){
		wan_iface.input(chan->common.dev, skb);
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
	if (wan_iface.input){
		wan_iface.input(chan->common.dev, skb);
	}
#endif

	return;
}


static int channel_timeslot_sync_ctrl(sdla_t *card, private_area_t * chan, int enable)
{
	u32 reg;

	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

	if (enable){
		set_channel_timeslot_sync(&reg,chan->first_time_slot);

		/* Make sure that Rx channel is disabled until
		* we setup an rx dma descriptor */
		wan_clear_bit(START_RX_CHANNEL_TSLOT_SYNC,&reg);

		/* Enable Global Tx Rx timeslot sync */
		wan_set_bit(ENABLE_CHANNEL_TSLOT_SYNC,&reg);

		DEBUG_TEST("%s:%s: Enabling Channel Timeslot Synch (Reg=0x%X)\n",
			card->devname,chan->if_name,reg);

	}else{
		wan_clear_bit(ENABLE_CHANNEL_TSLOT_SYNC,&reg);

		DEBUG_TEST("%s:%s: Disabling Channel Timeslot Synch (Reg=0x%X)\n",
			card->devname,chan->if_name,reg);
	}

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG, reg);

	return 0;
}

static int rx_chan_timeslot_sync_ctrl(sdla_t *card, int start)
{
	u32 reg;
	card->hw_iface.bus_read_4(card->hw,XILINX_CHIP_CFG_REG, &reg);

	if (start){
		DEBUG_TEST("%s: Enabling Rx Timeslot Synch (Reg=0x%X)\n",
			card->devname,reg);

		wan_set_bit(START_RX_CHANNEL_TSLOT_SYNC,&reg);
	}else{
		DEBUG_TEST("%s: Disabling Rx Timeslot Synch (Reg=0x%X)\n",
			card->devname,reg);
		wan_clear_bit(START_RX_CHANNEL_TSLOT_SYNC,&reg);
	}

	card->hw_iface.bus_write_4(card->hw,XILINX_CHIP_CFG_REG, reg);

	return 0;
}

static int send_rbs_oob_msg (sdla_t *card, private_area_t *chan, int channel, int status)
{
#if defined(__LINUX__)
	wp_api_hdr_t *api_rx_el;
	wp_api_event_t *api_el;
	struct sk_buff *skb;
	int err=0, len=5;

	if (chan->common.usedby != API){
		return -ENODEV;
	}

	if (!chan->common.sk){
		return -ENODEV;
	}

	skb=wan_skb_alloc(sizeof(wp_api_hdr_t)+sizeof(wp_api_event_t)+len);
	if (!skb){
		return -ENOMEM;
	}

	api_rx_el=(wp_api_hdr_t *)wan_skb_put(skb,sizeof(wp_api_hdr_t));
	memset(api_rx_el,0,sizeof(wp_api_hdr_t));

	api_el=(wp_api_event_t *)wan_skb_put(skb,sizeof(wp_api_event_t));
	memset(api_el,0,sizeof(wp_api_event_t));

	api_el->wp_api_event_channel=channel;
	api_el->wp_api_event_rbs_bits=status;

	skb->pkt_type = WAN_PACKET_ERR;
	skb->protocol=htons(PVC_PROT);
	skb->dev=chan->common.dev;

	DEBUG_TEST("%s: Sending OOB message len=%i\n",
		chan->if_name, wan_skb_len(skb));

	if (wan_api_rx(chan,skb)!=0){
		err=-ENODEV;
		wan_skb_free(skb);
	}
	return err;

#else
	DEBUG_EVENT("%s: OOB messages not supported!\n",
		chan->if_name);
	return -EINVAL;
#endif
}


static void aft_report_rbsbits(void* pcard, int channel, unsigned char status)
{
	sdla_t *card=(sdla_t *)pcard;
	int i;

	DEBUG_TEST("%s: Report Ch=%i Status=0x%X\n",
		card->devname,channel,status);

	if (!wan_test_bit(channel-1, &card->u.aft.time_slot_map)){
		return;
	}

	for (i=0; i<card->u.aft.num_of_time_slots;i++){
		private_area_t *chan;

		if (!wan_test_bit(i,&card->u.aft.logic_ch_map)){
			continue;
		}

		chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
		if (!chan){
			continue;
		}

		if (!wan_test_bit(0,&chan->up)){
			continue;
		}

		if (!wan_test_bit(channel-1, &chan->time_slot_map)){
			continue;
		}

		send_rbs_oob_msg (card, chan, channel, status);
		break;
	}


	return;
}


static int aft_rx_post_complete_voice (sdla_t *card, private_area_t *chan,
									   netskb_t *skb)
{

	unsigned int len = 0;
	wp_rx_element_t *rx_el=(wp_rx_element_t *)wan_skb_data(skb);

	DEBUG_RX("%s:%s: RX HI=0x%X  LO=0x%X DMA=0x%lX\n",
		__FUNCTION__,chan->if_name,rx_el->reg,rx_el->align,rx_el->dma_addr);


	rx_el->align&=RxDMA_LO_ALIGNMENT_BIT_MASK;

	/* Checking Rx DMA Go bit. Has to be '0' */
	if (wan_test_bit(RxDMA_HI_DMA_GO_READY_BIT,&rx_el->reg)){
		DEBUG_TEST("%s:%s: Error: RxDMA Intr: GO bit set on Rx intr\n",
			card->devname,chan->if_name);
		chan->if_stats.rx_errors++;
		chan->errstats.Rx_dma_descr_err++;
		goto rx_comp_error;
	}

	/* Checking Rx DMA PCI error status. Has to be '0's */
	if (rx_el->reg&RxDMA_HI_DMA_PCI_ERROR_MASK){

		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_M_ABRT){
			DEBUG_EVENT("%s:%s: Rx Error: Abort from Master: pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}
		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_T_ABRT){
			DEBUG_EVENT("%s:%s: Rx Error: Abort from Target: pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}
		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_DS_TOUT){
			DEBUG_EVENT("%s:%s: Rx Error: No 'DeviceSelect' from target: pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}
		if (rx_el->reg & RxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT){
			DEBUG_EVENT("%s:%s: Rx Error: 'Retry' exceeds maximum (64k): pci fatal error! (0x%08X)\n",
				card->devname,chan->if_name,rx_el->reg);
		}

		chan->if_stats.rx_errors++;
		chan->errstats.Rx_pci_errors++;
		goto rx_comp_error;
	}

	len=rx_el->reg&RxDMA_HI_DMA_DATA_LENGTH_MASK;

	/* In Transparent mode, our RX buffer will always be
	* aligned to the 32bit (word) boundary, because
	* the RX buffers are all of equal length  */
	len=(((chan->mru>>2)-len)<<2) - (~(0x03)&RxDMA_LO_ALIGNMENT_BIT_MASK);

	memset(wan_skb_data(skb),0,sizeof(wp_rx_element_t));
	wan_skb_put(skb,len);

	/* The rx size is big enough, thus
	* send this buffer up the stack
	* and allocate another one */
	wan_skb_pull(skb, sizeof(wp_rx_element_t));

	wan_skb_reverse(skb);

	return 0;

rx_comp_error:

	return -1;
}


static int aft_dma_rx_tdmv(sdla_t *card, private_area_t *chan, netskb_t *skb)
{
	unsigned int err;
	err=aft_rx_post_complete_voice(card,chan,skb);
	if (err==0){
		if (wan_tracing_enabled(&chan->trace_info) >= 1){
			if (card->u.aft.tdmv_dchan == 0){
				wan_capture_trace_packet_offset(card, &chan->trace_info, skb,
					IS_T1_CARD(card)?24:16, TRC_INCOMING_FRM);
			}
		}else{
			wan_capture_trace_packet(card, &chan->trace_info,
				skb, TRC_INCOMING_FRM);
		}

		if (chan->tdmv_zaptel_cfg){
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
			WAN_TDMV_CALL(rx_tx, (card,skb), err);
			if (err == 0) {
				chan->if_stats.rx_frame_errors++;
				aft_init_requeue_free_skb(chan, skb);
				return 0;
			}

			if (card->wan_tdmv.sc){
				WAN_TDMV_CALL(is_rbsbits, (&card->wan_tdmv), err);
				if (err == 1){
					wan_set_bit(AFT_FE_TDM_RBS,&card->u.aft.port_task_cmd);
					WAN_TASKQ_SCHEDULE((&card->u.aft.port_task));
				}
			}
#endif
		} else {
#ifdef AFT_TDM_API_SUPPORT
			/* TDM VOICE API */
			aft_tdm_api_rx_tx_channelized(card,chan,skb);
#endif
		}

		wan_skb_reverse(skb);

		wan_skb_queue_tail(&chan->wp_tx_pending_list,skb);
		chan->if_stats.rx_packets++;
		chan->if_stats.rx_bytes += wan_skb_len(skb);
		return 0;
	}

	return 1;
}


static int aft_realign_skb_pkt(private_area_t *chan, netskb_t *skb)
{
	unsigned char *data=wan_skb_data(skb);
	int len = wan_skb_len(skb);

	if (len > chan->dma_mru){
		DEBUG_EVENT("%s: Critical error: Tx unalign pkt(%d) > MTU buf(%d)!\n",
			chan->if_name,len,chan->dma_mru);
		return -ENOMEM;
	}

	if (!chan->tx_realign_buf){
		chan->tx_realign_buf=wan_malloc(chan->dma_mru);
		if (!chan->tx_realign_buf){
			DEBUG_EVENT("%s: Error: Failed to allocate tx memory buf\n",
				chan->if_name);
			return -ENOMEM;
		}else{
			DEBUG_EVENT("%s: AFT Realign buffer allocated Len=%d\n",
				chan->if_name,chan->dma_mru);

		}
	}

	memcpy(chan->tx_realign_buf,data,len);

	wan_skb_init(skb,0);
	wan_skb_trim(skb,0);

	if (wan_skb_tailroom(skb) < len){
		DEBUG_EVENT("%s: Critical error: Tx unalign pkt tail room(%i) < unalign len(%i)!\n",
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

	chan->opstats.Data_frames_Tx_realign_count++;

	return 0;
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

static void aft_fe_intr_ctrl(sdla_t *card, int status)
{
	wan_smp_flag_t	smp_flags;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	__aft_fe_intr_ctrl(card, status);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
}

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static void xilinx_aft_port_task (void * card_ptr)
# else
static void xilinx_aft_port_task (struct work_struct *work)
# endif
#elif defined(__WINDOWS__)
static void xilinx_aft_port_task (IN PKDPC Dpc, IN PVOID card_ptr, IN PVOID SystemArgument1, IN PVOID SystemArgument2)	
#else
static void xilinx_aft_port_task (void * card_ptr, int arg)
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
	wan_smp_flag_t	smp_flags;

	DEBUG_TEST("%s()\n", __FUNCTION__);

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		DEBUG_TEST("%s(): CARD_DOWN bit set\n", __FUNCTION__);
		return;
	}

	DEBUG_TEST("%s: AFT PORT TASK CMD=0x%X!\n",
		card->devname,card->u.aft.port_task_cmd);

	card->hw_iface.hw_lock(card->hw,&smp_flags);

	if (wan_test_bit(AFT_FE_INTR,&card->u.aft.port_task_cmd)){
		aft_fe_intr_ctrl(card, 0);
		front_end_interrupt(card,0,1);

		wan_clear_bit(AFT_FE_INTR,&card->u.aft.port_task_cmd);

		aft_fe_intr_ctrl(card, 1);
	}

	if (wan_test_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd)){
		aft_fe_intr_ctrl(card, 0);
		if (card->wandev.fe_iface.polling){
			wan_smp_flag_t	smp_flags;
			int		delay = 0;

			delay = card->wandev.fe_iface.polling(&card->fe);
			if (delay){
				card->wandev.fe_iface.add_timer(&card->fe, delay);
			}
			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			handle_front_end_state(card);
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
		}

		aft_fe_intr_ctrl(card, 1);
		wan_clear_bit(AFT_FE_POLL,&card->u.aft.port_task_cmd);
	}

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
	if (wan_test_bit(AFT_FE_TDM_RBS,&card->u.aft.port_task_cmd)){
		int	err;
		aft_fe_intr_ctrl(card, 0);
		WAN_TDMV_CALL(rbsbits_poll, (&card->wan_tdmv, card), err);
		aft_fe_intr_ctrl(card, 1);
		wan_clear_bit(AFT_FE_TDM_RBS,&card->u.aft.port_task_cmd);
	}
#endif


	card->hw_iface.hw_unlock(card->hw,&smp_flags);
}


/*
* ******************************************************************
* Proc FS function
*/
static int wan_aft_get_info(void* pcard, struct seq_file *m, int *stop_cnt)
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

static int aft_dev_open_private(sdla_t *card, private_area_t *chan)
{
	int err=0;

	/* Initialize the router start time.
	* Used by wanpipemon debugger to indicate
	* how long has the interface been up */
	wan_getcurrenttime(&chan->router_start_time, NULL);

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
			chan->if_name);

		/* Select an HDLC logic channel for configuration */
		card->hw_iface.bus_read_4(
			card->hw,
			XILINX_TIMESLOT_HDLC_CHAN_REG,
			&reg);

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

		card->hw_iface.bus_read_4(
			card->hw,
			XILINX_TIMESLOT_HDLC_CHAN_REG,
			&reg1);

		DEBUG_CFG("%s: Writting to REG(0x64)=0x%X   Reg(0x60)=0x%X\n",
			chan->if_name,reg,reg1);

		xilinx_write_ctrl_hdlc(card,
			chan->first_time_slot,
			XILINX_HDLC_CONTROL_REG,
			reg);
		if (err){
			DEBUG_EVENT("%s:%d wait for timeslot failed\n",
				__FUNCTION__,__LINE__);
			return err;
		}
	}

	xilinx_dev_enable(card,chan);
	wan_set_bit(0,&chan->up);

	chan->ignore_modem=0x0F;

	return err;

}

static int aft_dev_open(sdla_t *card, private_area_t *gchan)
{
	private_area_t *chan=gchan;
	int err=0;

	if (chan->channelized_cfg){
		for (chan=gchan; chan != NULL; chan=chan->next){
			err=aft_dev_open_private(card,chan);
		}
	}else{
		err=aft_dev_open_private(card,chan);
	}
	return err;
}

static void aft_dev_close_private(sdla_t *card, private_area_t *chan)
{
	chan->common.state = WAN_DISCONNECTED;
	xilinx_dev_close(card,chan);
	chan->ignore_modem=0x00;
}

static void aft_dev_close(sdla_t *card, private_area_t *gchan)
{
	private_area_t *chan=gchan;

	if (chan->channelized_cfg){

		for (chan=gchan; chan != NULL; chan=chan->next){

			aft_dev_close_private(card,chan);

			DEBUG_TEST("%s: Closing Ch=%ld\n",
				chan->if_name,chan->logic_ch_num);

			wan_clear_bit(0,&chan->up);
		}
	}else{
		aft_dev_close_private(card,chan);
		wan_clear_bit(0,&chan->up);
	}
	return;
}


/*
* ******************************************************************
* Init TDMV interface
*/
static int aft_tdmv_init(sdla_t *card, wandev_conf_t *conf)
{

	int err;

	DEBUG_EVENT("%s:    TDMV Span      = %d : %s\n",
		card->devname,
		card->tdmv_conf.span_no,
		card->tdmv_conf.span_no?"Enabled":"Disabled");

	err=0;

	return 0;
}

static int aft_tdmv_free(sdla_t *card)
{
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	if (card->tdmv_conf.span_no &&
		card->wan_tdmv.sc) {
			int	err;
			WAN_TDMV_CALL(remove, (card), err);
	}
#endif
	return 0;
}

static int aft_tdmv_if_init(sdla_t *card, private_area_t *chan, wanif_conf_t *conf)
{

	int	err = 0;
	int 	dchan=card->u.aft.tdmv_dchan;

	if (IS_T1_CARD(card) && card->u.aft.tdmv_dchan){
		dchan--;
	}

	if (chan->common.usedby != TDM_VOICE && chan->common.usedby != TDM_VOICE_API){
		return 0;
	}

	if (!card->tdmv_conf.span_no){
		return -EINVAL;
	}

	if (!conf->hdlc_streaming) {

		if (chan->common.usedby == TDM_VOICE) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
			WAN_TDMV_CALL(check_mtu, (card, conf->active_ch, &card->wandev.mtu), err);
#endif
		}

		if (chan->common.usedby == TDM_VOICE_API) {
#ifdef AFT_TDM_API_SUPPORT
			err=wp_tdmapi_check_mtu(card, conf->active_ch, 8, &card->wandev.mtu);
#endif
		}

		if (err){
			DEBUG_EVENT("Error: TMDV mtu check failed!");
			return -EINVAL;
		}

		chan->mtu = chan->mru = card->u.aft.cfg.mru = card->wandev.mtu;

	}


	/* If DCHAN is enabled, set this timeslot, so zaptel
	* configures it.  However, the wp_tdmv_software_init()
	* will remove it from the timeslot list. */
	if (card->u.aft.tdmv_dchan){
		wan_set_bit(dchan,&conf->active_ch);
	}

	/* The TDMV drivers always starts from number
	* ZERO. Wanpipe driver doesn't allow timeslot
	* ZERO. Thus, the active_ch map must me adjusted
	* before calling tdmv_reg */
	if (IS_E1_CARD(card)){
		conf->active_ch=conf->active_ch>>1;
	}

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	if (chan->tdmv_zaptel_cfg){

		WAN_TDMV_CALL(reg, (card, &conf->tdmv, conf->active_ch, conf->hwec.enable, chan->common.dev), err);
		if (err < 0){
			DEBUG_EVENT("%s: Error: Failed to register TDMV channel!\n",
				chan->if_name);

			return -EINVAL;
		}

		card->wan_tdmv.brt_enable = 1;
		conf->hdlc_streaming=0;

		WAN_TDMV_CALL(software_init, (&card->wan_tdmv), err);
		if (err){
			return err;
		}
	}
#endif

	if (card->u.aft.tdmv_dchan){
		wan_clear_bit(dchan,&conf->active_ch);
	}

	return 0;
}

static int aft_tdmv_if_free(sdla_t *card, private_area_t *chan)
{
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	if (chan->common.usedby == TDM_VOICE){
		int err;
		WAN_TDMV_CALL(unreg, (card,chan->time_slot_map), err);
		if (err){
			return err;
		}
	}
#endif
	return 0;
}

#ifdef AFT_TDM_API_SUPPORT
static int aft_read_rbs_bits(void *chan_ptr, u32 ch, u8 *rbs_bits)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	wan_smp_flag_t flags;
	sdla_t *card;

	if (!chan_ptr){
		return -EINVAL;
	}
	card=(sdla_t*)chan->common.card;

	card->hw_iface.hw_lock(card->hw,&flags);
	*rbs_bits = card->wandev.fe_iface.read_rbsbits(
		&card->fe,
		ch,
		WAN_TE_RBS_UPDATE);
	card->hw_iface.hw_unlock(card->hw,&flags);

	return 0;

}

static int aft_write_rbs_bits(void *chan_ptr, u32 ch, u8 rbs_bits)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	wan_smp_flag_t flags;
	sdla_t *card;
	int err;

	if (!chan_ptr){
		return -EINVAL;
	}

	card=(sdla_t*)chan->common.card;
	card->hw_iface.hw_lock(card->hw,&flags);
	err = card->wandev.fe_iface.set_rbsbits(&card->fe,
		ch,
		rbs_bits);
	card->hw_iface.hw_unlock(card->hw,&flags);

	return err;
}


static int aft_write_hdlc_frame(void *chan_ptr, netskb_t *skb,  wp_api_hdr_t *hdr)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	sdla_t *card=chan->common.card;
	wan_smp_flag_t smp_flags;
	int err=-EINVAL;
			
	DEBUG_TDMAPI("%s(): line: %d\n", __FUNCTION__, __LINE__);

	if (!chan_ptr || !chan->common.dev || !card){
		WAN_ASSERT(1);
		return -EINVAL;
	}

	if (chan->common.usedby != TDM_VOICE_DCHAN
#if defined(__WINDOWS__)
		&& chan->common.usedby != API
#endif
		) {
		return -EINVAL;
	}

	if (wan_skb_len(skb) > chan->mtu) {
		return -EINVAL;
	}

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

	if (wan_skb_queue_len(&chan->wp_tx_pending_list) > MAX_TX_BUF){
		xilinx_dma_tx(card,chan);
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		return -EBUSY;
	}

	wan_skb_unlink(skb);
	wan_skb_queue_tail(&chan->wp_tx_pending_list,skb);
	xilinx_dma_tx(card,chan);

	err=0;
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	return err;
}

static int aft_tdm_api_update_state_channelized(sdla_t *card, private_area_t *chan, int state)
{
	int x;

	for (x=0;x<card->u.aft.num_of_time_slots;x++){
		if (!wan_test_bit(x,&chan->tdmapi_timeslots)){
			continue;
		}
		if (is_tdm_api(chan,&chan->wp_tdm_api_dev_idx[x])){
			wan_event_t	event;

			event.type	= WAN_EVENT_LINK_STATUS;
			event.channel	= 0;
			if (state == WAN_CONNECTED) {
				event.link_status= WAN_EVENT_LINK_STATUS_CONNECTED;
			} else {
				event.link_status= WAN_EVENT_LINK_STATUS_DISCONNECTED;
			}

			if (card->wandev.event_callback.linkstatus) {
				card->wandev.event_callback.linkstatus(card, &event);
			}
		}
	}

	return 0;
}

static int aft_tdm_api_rx_tx_channelized(sdla_t *card, private_area_t *chan, netskb_t *skb)
{

	u8 *rxbuf = wan_skb_data(skb);
	u8 *txbuf = wan_skb_data(skb);
	int rxbuf_len = wan_skb_len(skb);
	int x,y;
	int offset=0;

	if (rxbuf_len != chan->mru) {
		chan->if_stats.rx_errors++;
		return -EINVAL;
	}

	for (y=0;y<WP_TDM_API_CHUNK_SZ;y++){
		for (x=0;x<card->u.aft.num_of_time_slots;x++){

			if (!wan_test_bit(x,&chan->tdmapi_timeslots)){
				continue;
			}

			chan->wp_tdm_api_dev_idx[x].rx_data[y] =
				rxbuf[offset++];

			if (y == WP_TDM_API_CHUNK_SZ-1) {
				wanpipe_tdm_api_rx_tx(&chan->wp_tdm_api_dev_idx[x],
					chan->wp_tdm_api_dev_idx[x].rx_data,
					chan->wp_tdm_api_dev_idx[x].tx_data,
					WP_TDM_API_CHUNK_SZ);
			}
		}
	}

	offset=0;
	for (y=0;y<WP_TDM_API_CHUNK_SZ;y++){
		for (x=0;x<card->u.aft.num_of_time_slots;x++){
			if (!wan_test_bit(x,&chan->tdmapi_timeslots)){
				continue;
			}
			txbuf[offset++] = chan->wp_tdm_api_dev_idx[x].tx_data[y];
		}
	}

	return 0;
}

#endif

static void aft_critical_shutdown (sdla_t *card)
{

#ifdef __LINUX__
	printk(KERN_ERR "%s: Error: Card Critically Shutdown!\n",
		card->devname);
#else
	DEBUG_EVENT("%s: Error: Card Critically Shutdown!\n",
		card->devname);
#endif

	/* TE1 - Unconfiging, only on shutdown */
	if (IS_TE1_CARD(card)) {
		if (card->wandev.fe_iface.unconfig){
			card->wandev.fe_iface.unconfig(&card->fe);
		}
	}

	port_set_state(card,WAN_DISCONNECTED);
	disable_data_error_intr(card,DEVICE_DOWN);
	wan_set_bit(CARD_DOWN,&card->wandev.critical);

	aft_red_led_ctrl(card, AFT_LED_ON);
	card->wandev.fe_iface.led_ctrl(&card->fe, AFT_LED_OFF);
}




/****** End ****************************************************************/
