/*****************************************************************************
* sdla_bitstrm.c WANPIPE(tm) Multiprotocol WAN Link Driver. Bit Stream module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*		Gideon Hack  
*
* Copyright:	(c) 1995-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
* Jan 03, 2004  David Rokhvarg	Added new IOCTL calls for the "lineprobe".
* Aug 10, 2002  Nenad Corbic	New bitstreaming code
* Sep 20, 2001  Nenad Corbic    The min() function has changed for 2.4.9
*                               kernel. Thus using the wp_min() defined in
*                               wanpipe.h
* Aug 14, 2001	Nenad Corbic	Inital version, based on Chdlc module.
*      			        Using Gideons new bitstreaming firmware.
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_wanrouter.h>		/* WAN router definitions */
#include <linux/wanpipe.h>		/* WANPIPE common user API definitions */
#include <linux/sdlapci.h>
#include <linux/wanpipe_api_deprecated.h>

#include <linux/sdla_bitstrm.h>		/* BSTRM firmware API definitions */
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
#include <linux/if_wanpipe.h>		
#include <linux/wanproc.h>
#include <linux/wanpipe_syncppp.h>


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
# include "wanpipe_lapb_kernel.h"
#endif



/****** Defines & Macros ****************************************************/

/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   		0x01
#define TMR_INT_ENABLED_UPDATE		0x02
#define TMR_INT_ENABLED_CONFIG		0x10
#define TMR_INT_ENABLED_TE		0x20

#define MAX_IP_ERRORS	10

#define	BSTRM_DFLT_DATA_LEN	1500		/* default MTU */
#define BSTRM_HDR_LEN		1

#define BSTRM_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_BH_BUFF	200

#undef PRINT_DEBUG
#ifdef PRINT_DEBUG
#define dbg_printk(format, a...) printk(format, ## a)
#else
#define dbg_printk(format, a...)
#endif  


#define INC_CRC_CNT(a)   if (++a >= MAX_CRC_QUEUE) a=0;
#define GET_FIN_CRC_CNT(a)  { if (--a < 0) a=MAX_CRC_QUEUE-1; \
		              if (--a < 0) a=MAX_CRC_QUEUE-1; }

#define FLIP_CRC(a,b)  { b=0; \
			 b |= (a&0x000F)<<12 ; \
			 b |= (a&0x00F0) << 4; \
			 b |= (a&0x0F00) >> 4; \
			 b |= (a&0xF000) >> 12; }

#define DECODE_CRC(a) { a=( (((~a)&0x000F)<<4) | \
		            (((~a)&0x00F0)>>4) | \
			    (((~a)&0x0F00)<<4) | \
			    (((~a)&0xF000)>>4) ); }
#define BITSINBYTE 8

#define MAX_CRC_QUEUE 3

#define NO_FLAG 	0
#define OPEN_FLAG 	1
#define CLOSING_FLAG 	2

#define HDLC_ENCODE	0
#define HDLC_ENCODE_IDLE_UPDATE 0
#define HDLC_DECODE	0	

#define MAX_TX_QUEUE_SIZE	50	
#define MAX_RX_FREE_SKB 	10
#define MAX_MTU_SIZE    	1600
#define MAX_CHAN_TX_UP_SIZE 	1200 

#define MAX_T1_CHAN_TX_UP_SIZE  50*NUM_OF_T1_CHANNELS		//1200 bytes
#define MAX_E1_CHAN_TX_UP_SIZE  40*(NUM_OF_E1_CHANNELS-1)	//1240 bytes
#define MAX_E1_UNFRM_CHAN_TX_UP_SIZE  40*(NUM_OF_E1_CHANNELS)	//1240 bytes


//(MAX_MTU_SIZE-100)
#define SWITCH_TX_UP_DFLT	240*3  
//Max Rx Pkt

#define TX_QUEUE_LOW		(MAX_TX_QUEUE_SIZE*1/10)
#define TX_QUEUE_MID		(MAX_TX_QUEUE_SIZE*5/10)
#define TX_QUEUE_HIGH		(MAX_TX_QUEUE_SIZE*9/10) 

#define TX_BH_CRIT	1
#define RX_BH_CRIT	0

#define TX_IDLE_FLAG	0x04
#define BRD_IDLE_FLAG   0x02

#define QUEUE_SYNC	0
#define QUEUE_LOW	1
#define QUEUE_HIGH  	2

/* chan->tq_control fileds */
#define WAIT_DEVICE_BUFFERS  0
#define WAIT_DEVICE_FAST  1
#define WAIT_DEVICE_SLOW  2    		 

atomic_t intr_cnt;
atomic_t intr_skip;

atomic_t tx_data;
atomic_t rx_data;

#define HDLC_ENG_BUF_LEN 5000


#if 0
# define HDLC_IDLE_ABORT	1
#else
# undef HDLC_IDLE_ABORT	
#endif

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)


/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with BSTRM specific data
 */

typedef struct bitstrm_private_area
{
	wanpipe_common_t common;
	sdla_t		*card;
//	netdevice_t 	*dev;
	char if_name[WAN_IFNAME_SZ+1];
	unsigned long 	router_start_time;
	unsigned long   router_up_time;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	u8		config_bstrm;
	unsigned char  mc;			/* Mulitcast support on/off */
	char udp_pkt_src;
	char update_comms_stats;		/* updating comms stats */

	u8 true_if_encoding;

	unsigned char rx_decode_buf[HDLC_ENG_BUF_LEN];
	unsigned int rx_decode_len;
	unsigned char rx_decode_bit_cnt;
	unsigned char rx_decode_onecnt;
	
	unsigned char tx_decode_buf[HDLC_ENG_BUF_LEN];
	unsigned int tx_decode_len;
	unsigned char tx_decode_bit_cnt;
	unsigned char tx_decode_onecnt;

	unsigned long  hdlc_flag;
	unsigned short rx_orig_crc;
	unsigned short rx_crc[MAX_CRC_QUEUE];
	unsigned short crc_fin;
	unsigned short tx_crc_fin;
	unsigned short rx_crc_tmp;
	unsigned short tx_crc_tmp;
	int crc_cur;
	int crc_prv;
	unsigned short tx_crc;
	unsigned char tx_flag;
	unsigned char tx_flag_offset;
	unsigned char tx_flag_offset_data;
	unsigned char tx_flag_idle;

	struct sk_buff_head rx_free_queue;
	struct sk_buff_head rx_used_queue;
	struct sk_buff_head tx_queue;

	unsigned long time_slot_map;
	unsigned int time_slots;
	struct sk_buff *rx_skb;

	struct sk_buff *tx_skb;	
	unsigned long	rx_timeout;
	unsigned short  rx_max_timeout;

	unsigned char   tx_idle_flag;

	struct net_device_stats ifstats;
	netdevice_t 	*sw_dev;
	unsigned char	sw_if_name[WAN_IFNAME_SZ+1];
	unsigned int 	max_tx_up_size;
	unsigned long 	tq_control;
	unsigned char	hdlc_eng;
	unsigned short 	max_tx_queue_sz;
	unsigned char   ignore_modem;

	unsigned int protocol;
	unsigned char udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t udp_pkt_len;
	unsigned char seven_bit_hdlc;
	unsigned char bits_in_byte;

	unsigned char debug_stream;
	unsigned char debug_stream_1;
	unsigned char debug_char;

	wan_bitstrm_conf_if_t cfg;

	unsigned char	rbs_on;
	unsigned char	rbs_chan;
	unsigned char 	rbs_sig;
	
	netdevice_t *annexg_dev;
	unsigned char label[WAN_IF_LABEL_SZ+1];


	//FIXME: add driver stats as per frame relay!
} bitstrm_private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/* variable for tracking how many interfaces to open for WANPIPE on the
   two ports */

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);
static int del_if(wan_device_t *wandev, netdevice_t *dev);


/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd);

static struct net_device_stats* if_stats (netdevice_t* dev);
static int if_send (struct sk_buff* skb, netdevice_t* dev);

/* BSTRM Firmware interface functions */
static int bstrm_configure 	(sdla_t* card, void* data);
static int bstrm_comm_enable 	(sdla_t* card);
static int bstrm_read_version 	(sdla_t* card, char* str);
static int bstrm_set_intr_mode 	(sdla_t* card, unsigned mode);
static int set_adapter_config	(sdla_t* card);
static int bstrm_send (sdla_t* card, void* data, unsigned len, unsigned char flag);
static int bstrm_read_comm_err_stats (sdla_t* card);
static int bstrm_read_op_stats (sdla_t* card);
static int bstrm_error (sdla_t *card, int err, wan_mbox_t *mb);


static int bstrm_disable_comm_shutdown (sdla_t *card);
static void if_tx_timeout (netdevice_t *dev);


/* Miscellaneous BSTRM Functions */
static int set_bstrm_config (sdla_t* card);
static void init_bstrm_tx_rx_buff( sdla_t* card);
static int process_bstrm_exception(sdla_t *card);
static int process_global_exception(sdla_t *card);
static int update_comms_stats(sdla_t* card,
        bitstrm_private_area_t* bstrm_priv_area);
static void port_set_state (sdla_t *card, int);
static int config_bstrm (sdla_t *card);
static void disable_comm (sdla_t *card);
static int bstrm_comm_disable (sdla_t *card);
static int bstrm_set_FE_config (sdla_t *card);


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev);
static netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t* annexg_dev_name);
static int get_map(wan_device_t*, netdevice_t*, struct seq_file* m, int*);
static void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats);
#endif

/* Interrupt handlers */
static WAN_IRQ_RETVAL wpbit_isr (sdla_t* card);
static void rx_intr (sdla_t* card);
static void timer_intr(sdla_t *);

  /* Bottom half handlers */
static void bstrm_rx_bh (unsigned long data);
static void bstrm_tx_bh (unsigned long data);
static int bstrm_bh_cleanup (sdla_t *card, struct sk_buff *skb);
static struct sk_buff * bh_enqueue (sdla_t *card);
  

/* Miscellaneous functions */
static int intr_test( sdla_t* card);
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,  
				bitstrm_private_area_t* bstrm_priv_area, 
				char local);

/* Bitstream decoding functions */
static void init_crc(void);
static void calc_rx_crc(bitstrm_private_area_t *chan);
static void calc_tx_crc(bitstrm_private_area_t *chan, unsigned char byte);
static void decode_byte (bitstrm_private_area_t *chan, unsigned char *byte);
static void hdlc_decode (bitstrm_private_area_t *chan, unsigned char *buf, int len);
static void tx_up_decode_pkt(bitstrm_private_area_t *chan);

static void init_rx_hdlc_eng(sdla_t *card, bitstrm_private_area_t *chan, int hard);

static int hdlc_encode(bitstrm_private_area_t *chan,struct sk_buff **skb);
static void encode_byte (bitstrm_private_area_t *chan, unsigned char *byte, int flag);

static int bstrm_get_config_info(void* priv, struct seq_file* m, int*);

static void 
wanpipe_switch_datascope_tx_up(bitstrm_private_area_t *chan,struct sk_buff *skb);

static void bstrm_enable_timer (void* card_id);
static void bstrm_handle_front_end_state(void* card_id);
static int bstrm_bh_data_tx_up(sdla_t *card, struct sk_buff *skb, bitstrm_private_area_t * chan);
static void bstrm_switch_send(sdla_t *card, bitstrm_private_area_t * chan, struct sk_buff *skb);
static int bstrm_bind_dev_switch (sdla_t *card, bitstrm_private_area_t *chan, char *sw_dev_name);
static int bstrm_unbind_dev_switch (sdla_t *card, bitstrm_private_area_t *chan);

static int protocol_init (sdla_t*card,netdevice_t *dev,
		          bitstrm_private_area_t*chan,
			  wanif_conf_t* conf);

static int protocol_stop (sdla_t *card, netdevice_t *dev);
static int protocol_start (sdla_t *card, netdevice_t *dev);
static int protocol_shutdown (sdla_t *card, netdevice_t *dev);

static void send_ppp_term_request (netdevice_t *dev);

static int bstrm_set_te_signaling_config (void* card_id,unsigned long ts_sig_map);
static int bstrm_disable_te_signaling (void* card,unsigned long);
static int bstrm_read_te_signaling_config (void* card);

static int send_rbs_oob_msg (sdla_t *card, bitstrm_private_area_t *chan);


#define LINEPROBE
#ifdef LINEPROBE
static int api_enable_comms(sdla_t * card);
static int api_disable_comms(sdla_t* card);
static int setup_api_timeslots(sdla_t* card);
#endif

const int MagicNums[8] = { 0x1189, 0x2312, 0x4624, 0x8C48, 0x1081, 0x2102, 0x4204, 0x8408 };
unsigned short CRC_TABLE[256];
const char FLAG[]={ 0x7E, 0xFC, 0xF9, 0xF3, 0xE7, 0xCF, 0x9F, 0x3F };
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
int wpbit_init (sdla_t* card, wandev_conf_t* conf)
{
	unsigned char port_num;
	int i,err;
	struct sk_buff *skb;
	unsigned long max_permitted_baud = 0;
	unsigned long smp_flags;

	union
		{
		char str[80];
		} u;

	volatile wan_mbox_t* mb;
	wan_mbox_t* mb1;
	unsigned long timeout;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_BITSTRM) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Find out which Port to use */
	if ((conf->comm_port == WANOPT_PRI) || (conf->comm_port == WANOPT_SEC)){
		if (card->next){
			if (conf->comm_port != card->next->u.b.comm_port){
				card->u.b.comm_port = conf->comm_port;
			}else{
				DEBUG_EVENT( "%s: ERROR - %s port used!\n",
        		        	card->wandev.name, PORT(conf->comm_port));
				return -EINVAL;
			}
		}else{
			card->u.b.comm_port = conf->comm_port;
		}
	}else{
		DEBUG_EVENT( "%s: ERROR - Invalid Port Selected!\n",
                			card->wandev.name);
		return -EINVAL;
	}
	

	/* Initialize protocol-specific fields */
	if (card->type != SDLA_S514){

		DEBUG_EVENT( "%s: ERROR - T1/E1 Bitstrm doesn't support S508 cards!\n",
				card->devname); 
		return -EOPNOTSUPP;

	}else{ 
		/* for a S514 adapter, set a pointer to the actual mailbox in the */
		/* allocated virtual memory area */
		if (card->u.b.comm_port == WANOPT_PRI){
			card->mbox_off = PRI_BASE_ADDR_MB_STRUCT;
		}else{
			card->mbox_off = SEC_BASE_ADDR_MB_STRUCT;
		}	
	}

	mb = &card->wan_mbox;
	mb1 = &card->wan_mbox;
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
			DEBUG_EVENT(
				"%s: Initialization not completed by adapter\n",
				card->devname);
			DEBUG_EVENT( "Please contact Sangoma representative.\n");
			return -EIO;
		}
	}

	card->wandev.ignore_front_end_status = conf->ignore_front_end_status;

	//ALEX_TODAY err=check_conf_hw_mismatch(card,conf->te_cfg.media);
	err = (card->hw_iface.check_mismatch) ? 
			card->hw_iface.check_mismatch(card->hw,conf->fe_cfg.media) : -EINVAL;
	if (err){
		return err;
	}

	card->u.b.serial=0;

			
	if (IS_TE1_MEDIA(&conf->fe_cfg)){
		
		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_te_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;

		card->wandev.fe_enable_timer = bstrm_enable_timer;
		card->wandev.te_link_state = bstrm_handle_front_end_state;

		conf->electrical_interface = 
			(IS_T1_CARD(card)) ? WANOPT_V35 : WANOPT_RS232;

		if (card->u.b.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

		if (IS_FR_FEUNFRAMED(&card->fe)){
			card->u.b.serial=1;
		}

		DEBUG_EVENT("%s: Config Media = %s\n",
			card->devname, FE_MEDIA_DECODE(&card->fe));
		
	}else if (IS_56K_MEDIA(&conf->fe_cfg)){

		memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
		sdla_56k_iface_init(&card->fe, &card->wandev.fe_iface);
		card->fe.name		= card->devname;
		card->fe.card		= card;
		card->fe.write_fe_reg = card->hw_iface.fe_write;
		card->fe.read_fe_reg	 = card->hw_iface.fe_read;
		
		if (card->u.c.comm_port == WANOPT_PRI){
			conf->clocking = WANOPT_EXTERNAL;
		}

		card->u.b.serial=1;
		
		DEBUG_EVENT("%s: Config Media = %s\n",
			card->devname, FE_MEDIA_DECODE(&card->fe));

	}else{
		card->u.b.serial=1;
		/* FIXME: Remove this line */
		card->fe.fe_status = FE_CONNECTED;
		
		DEBUG_EVENT("%s: Config Media = Unknown\n",
			card->devname);
	}

	if (card->u.b.serial){
		DEBUG_EVENT("%s: Configuring Driver for Serial Mode!\n",
				card->devname);
	}

	/* Read firmware version.  Note that when adapter initializes, it
	 * clears the mailbox, so it may appear that the first command was
	 * executed successfully when in fact it was merely erased. To work
	 * around this, we execute the first command twice.
	 */

	if (bstrm_read_version(card, u.str))
		return -EIO;

	DEBUG_EVENT( "%s: Running Bit Streaming firmware v%s\n",
		card->devname, u.str); 

	if (set_adapter_config(card)) {
		DEBUG_EVENT( "%s: Failed to set adapter type!\n",
				card->devname);
		return -EIO;
	}

	card->isr			= &wpbit_isr;
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

	// Proc fs functions
	card->wandev.get_config_info 	= &bstrm_get_config_info;

	/* Initialize Bit Stream Configuration */
	memcpy(&card->u.b.cfg,&conf->u.bitstrm,sizeof(wan_bitstrm_conf_t));

#if 0
	card->u.b.sync_options = conf->sync_options;
	card->u.b.cfg.rx_sync_char = conf->rx_sync_char;
	card->u.b.cfg.monosync_tx_time_fill_char = conf->monosync_tx_time_fill_char;
	
	card->u.b.cfg.max_length_tx_data_block = conf->max_length_tx_data_block;
	card->u.b.cfg.rx_complete_length = conf->rx_complete_length;
	card->u.b.cfg.rx_complete_timer = conf->rx_complete_timer;
#endif

	/* reset the number of times the 'update()' proc has been called */
	card->u.b.update_call_count = 0;
	
	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 

	card->wandev.clocking = conf->clocking;

	port_num = card->u.b.comm_port;

	/* Setup Port Bps */
	if(card->wandev.clocking) {
		if(port_num == WANOPT_PRI) {
			/* For Primary Port 0 */
               		max_permitted_baud =
				(card->type == SDLA_S514) ?
				PRI_MAX_BAUD_RATE_S514 : 
				PRI_MAX_BAUD_RATE_S508;

		}else if(port_num == WANOPT_SEC) {
			/* For Secondary Port 1 */
                        max_permitted_baud =
                               (card->type == SDLA_S514) ?
                                SEC_MAX_BAUD_RATE_S514 :
                                SEC_MAX_BAUD_RATE_S508;
                        }
  
			if(conf->bps > max_permitted_baud) {
				conf->bps = max_permitted_baud;
				DEBUG_EVENT( "%s: Baud too high!\n",
					card->wandev.name);
 				DEBUG_EVENT( "%s: Baud rate set to %lu bps\n", 
					card->wandev.name, max_permitted_baud);
			}
			card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
  	}

	/* Setup the Port MTU */
	if(port_num == WANOPT_PRI) {
		/* For Primary Port 0 */
		card->wandev.mtu =  wp_min(conf->mtu, PRI_MAX_LENGTH_TX_DATA_BLOCK);
	} else if(port_num == WANOPT_SEC) { 
		/* For Secondary Port 1 */
		card->wandev.mtu = wp_min(conf->mtu, SEC_MAX_LENGTH_TX_DATA_BLOCK);
	}

	/* Set up the interrupt status area */
	/* Read the BSTRM Configuration and obtain: 
	 *	Ptr to shared memory infor struct
         * Use this pointer to calculate the value of card->u.b.flags !
 	 */
	mb1->wan_data_len = 0;
	mb1->wan_command = READ_BSTRM_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb1);
	if(err != OK) {
		bstrm_error(card, err, mb1);
		return -EIO;
	}

      	card->flags_off = 
       		(((BSTRM_CONFIGURATION_STRUCT *)mb1->wan_data)->
			ptr_shared_mem_info_struct);

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

	card->u.b.rx_discard_off =
		card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, BSTRM_info_struct) +
		offsetof(BSTRM_INFORMATION_STRUCT, Rx_discard_count);

	card->u.b.tx_idle_off =
		card->flags_off +
		offsetof(SHARED_MEMORY_INFO_STRUCT, BSTRM_info_struct) +
		offsetof(BSTRM_INFORMATION_STRUCT, Tx_idle_count);

	if (!card->wandev.piggyback){	
		int err;

		/* This is for the ports link state */
		card->wandev.state = WAN_DUALPORT;
		card->u.b.state = WAN_DISCONNECTED;

		
		/* Perform interrupt testing */
		err = intr_test(card);

		if(err || (card->timer_int_enabled == 0 /* MAX_INTR_TEST_COUNTER*/)) { 
			DEBUG_EVENT( "%s: Interrupt test failed (%i)\n",
					card->devname, card->timer_int_enabled);
			DEBUG_EVENT( "%s: Please choose another interrupt\n",
					card->devname);
			return -EIO;
		}
		DEBUG_EVENT( "%s: Interrupt test passed (%i)\n", 
				card->devname, card->timer_int_enabled);
		card->configured = 1;
	}

	/* If we are using BSTRM in backup mode, this flag will
	 * indicate not to look for IP addresses in config_bstrm()*/
	card->backup = conf->backup;

	DEBUG_EVENT( "%s: Initializing CRC tables\n",card->devname);
	init_crc();

	card->u.b.tq_working=0;

	/* Allocate and initialize BH circular buffer */
	/* Add 1 to MAX_BH_BUFF so we don't have test with (MAX_BH_BUFF-1) */
	skb_queue_head_init(&card->u.b.rx_isr_queue);
	skb_queue_head_init(&card->u.b.rx_isr_free_queue);

	memset(card->u.b.time_slot_map,0,sizeof(card->u.b.time_slot_map));
	card->u.b.time_slots=NUM_OF_T1_CHANNELS;

	if (IS_E1_CARD(card)){
		card->u.b.time_slots=NUM_OF_E1_CHANNELS;
	}

	if (IS_TE1_MEDIA(&conf->fe_cfg)){
		int tx_time_slots=card->u.b.time_slots;
		
		if (IS_E1_CARD(card) && !IS_FR_FEUNFRAMED(&card->fe) && card->u.b.time_slots == 32){
			tx_time_slots=31;
		}
		
		if (card->u.b.cfg.max_length_tx_data_block % tx_time_slots){
		
			DEBUG_EVENT( "\n");
			DEBUG_EVENT( "%s: Cfg max_length_tx_data_block : %d, time_slots:  %d\n",
					card->devname,
					card->u.b.cfg.max_length_tx_data_block,
				      	tx_time_slots);

			card->u.b.cfg.max_length_tx_data_block-=
				(card->u.b.cfg.max_length_tx_data_block%
				 tx_time_slots);

			card->u.b.cfg.max_length_tx_data_block+=tx_time_slots;
		
#if 0
			DEBUG_EVENT( "%s: Error: Max tx data block %i not multiple of channels %i\n",
					card->devname,card->u.b.cfg.max_length_tx_data_block,
					card->u.b.time_slots);
			bstrm_skb_queue_purge(&card->u.b.rx_isr_free_queue);
			return -EINVAL;
#else
			DEBUG_EVENT( "%s: Warning: Adjusting max tx block to %i\n",
					card->devname,card->u.b.cfg.max_length_tx_data_block);
#endif
		}

		if (card->u.b.cfg.rx_complete_length % card->u.b.time_slots){

			DEBUG_EVENT( "%s: Cfg rx_complete_length : %d, time_slots:  %d\n",
					card->devname,
					card->u.b.cfg.rx_complete_length,
				      	card->u.b.time_slots);

			card->u.b.cfg.rx_complete_length-=
				(card->u.b.cfg.rx_complete_length%
				 card->u.b.time_slots);

			card->u.b.cfg.rx_complete_length+=card->u.b.time_slots;
			DEBUG_EVENT( "%s: Warning: Adjusting max rx block to %i\n",
					card->devname,card->u.b.cfg.rx_complete_length);
			
		}


		if (card->u.b.cfg.max_length_tx_data_block >= MAX_E1_CHANNELS*50){
			DEBUG_EVENT("%s: Error: Tx data block exceeds max=%i\n",
					card->devname,MAX_E1_CHANNELS*50);
			return -EINVAL;
		}
	}

	card->u.b.tx_chan_multiple=
		card->u.b.cfg.max_length_tx_data_block/card->u.b.time_slots;

	DEBUG_EVENT( "\n");	
	DEBUG_EVENT( "%s: Configuring: \n",card->devname);
	if (IS_TE1_CARD(card)){
		DEBUG_EVENT( "%s:      ChanNum     =%i\n",
				card->devname, card->u.b.time_slots);
		DEBUG_EVENT( "%s:      TxChanMult  =%i\n",
				card->devname, card->u.b.tx_chan_multiple);
	}
	DEBUG_EVENT( "%s:      TxMTU       =%i\n",
			card->devname, card->u.b.cfg.max_length_tx_data_block);
	DEBUG_EVENT( "%s:      RxMTU       =%i\n",
			card->devname, card->u.b.cfg.rx_complete_length);
	DEBUG_EVENT( "%s:      IdleFlag    =0x%x\n",
				card->devname,
				card->u.b.cfg.monosync_tx_time_fill_char);

	/* RBS Map only supports 24 T1 channels */
	card->u.b.cfg.rbs_map&=0x00FFFFFF;

	if (IS_T1_CARD(card)){

		DEBUG_EVENT( "%s:      RBS Control =%s  Map=0x%08X\n",
					card->devname,
					card->u.b.cfg.rbs_map?
					"ON":"OFF",card->u.b.cfg.rbs_map);
	}else{
		if (card->u.b.cfg.rbs_map){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("%s: Warning: Robbit only supported on T1!\n",
					card->devname);
			card->u.b.cfg.rbs_map=0;
		}
	}

	DEBUG_EVENT("\n");

	/* This is for the ports link state */
	card->wandev.state = WAN_CONNECTING;
	card->u.b.state = WAN_DISCONNECTED;

	/* Initialize the task queue */
	tasklet_init(&card->u.b.wanpipe_rx_task, bstrm_rx_bh, (unsigned long)card);
	tasklet_init(&card->u.b.wanpipe_tx_task, bstrm_tx_bh, (unsigned long)card);

	for (i=0;i<10;i++){
		skb=dev_alloc_skb(card->u.b.cfg.rx_complete_length+50+sizeof(api_rx_hdr_t));
		if (!skb){
			DEBUG_EVENT( "%s: Failed to allocate rx isr queue mem!\n",
					card->devname);
			bstrm_skb_queue_purge(&card->u.b.rx_isr_free_queue);
			card->wandev.state = WAN_UNCONFIGURED;
			return -ENOMEM;
			
		}
		skb_queue_tail(&card->u.b.rx_isr_free_queue,skb);
	}


#if 0
//NENAD
	{
	int x;
	card->u.b.tx_scratch_buf_len=1200;
	for (x=0;x<card->u.b.tx_scratch_buf_len;x++){
		card->u.b.tx_scratch_buf[x]=0x7E;//(x%32);
	}
	}
#endif


	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	err=config_bstrm(card);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	if (err){
		DEBUG_EVENT( "%s: Failed to configure adapter!\n",card->devname);
		disable_comm(card);

		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		bstrm_skb_queue_purge(&card->u.b.rx_isr_free_queue);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

		card->wandev.state = WAN_UNCONFIGURED;
		return err;
	}


	set_bit(SEND_CRIT,&card->wandev.critical);

	card->disable_comm = &disable_comm;
	DEBUG_TEST("%s: Config DONE\n",card->devname);
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
 *      3) BSTRM operational statistics on the adapter.
 * The board level statistics are read during a timer interrupt. Note that we 
 * read the error and operational statistics during consecitive timer ticks so
 * as to minimize the time that we are inside the interrupt handler.
 *
 */

static int update (wan_device_t* wandev)
{
	sdla_t* card = wandev->priv;
 	netdevice_t* dev;
        bitstrm_private_area_t* bstrm_priv_area;
	//unsigned long smp_flags;
	
	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	/* more sanity checks */
	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical))
                return -EAGAIN;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if(dev == NULL)
		return -ENODEV;

	if((bstrm_priv_area=wan_netif_priv(dev)) == NULL)
		return -ENODEV;

       	if(bstrm_priv_area->update_comms_stats){
		return -EAGAIN;
	}

#if 0
	bstrm_priv_area->ifstats.rx_fifo_errors=0;
	bstrm_priv_area->ifstats.rx_frame_errors=0;
	bstrm_priv_area->ifstats.tx_carrier_errors=0;
	bstrm_priv_area->ifstats.rx_bytes=0;
	bstrm_priv_area->ifstats.tx_bytes=0;
	bstrm_priv_area->ifstats.rx_packets=0;
	bstrm_priv_area->ifstats.tx_packets=0;
	bstrm_priv_area->ifstats.tx_fifo_errors=0;
#endif

#if 0
	DEBUG_EVENT( "%s:%s: Tx B=%i:  Rx U=%i:  Rx F=%i:  Rx-U-BH=%i Rx-F-Bh=%i Idle 0x%x\n",
			card->devname,
			dev->name,
			skb_queue_len(&bstrm_priv_area->tx_queue),
			skb_queue_len(&bstrm_priv_area->rx_used_queue),
			skb_queue_len(&bstrm_priv_area->rx_free_queue),
			skb_queue_len(&card->u.b.rx_isr_queue),
			skb_queue_len(&card->u.b.rx_isr_free_queue),
			bstrm_priv_area->tx_idle_flag);
#endif
#if 0
	DEBUG_EVENT( "%s: Tx Data = %i Rx Data = %i\n",
			card->devname, atomic_read(&tx_data),atomic_read(&rx_data)); 


	DEBUG_EVENT( "%s: Sw Dev = %s\n",
			bstrm_priv_area->if_name,
			bstrm_priv_area->sw_dev ? 
			bstrm_priv_area->sw_dev->name : "None");
	DEBUG_EVENT( "%s: ISR=%li Rx=%li Tx=%li RxBh=%li RxBhBsy=%li\n",
			card->devname,
			card->statistics.isr_entry,
			card->statistics.isr_rx,
			card->statistics.isr_tx,
			card->statistics.poll_entry,
			card->statistics.poll_tbusy_bad_status);	
#endif

#if 0
	if (bstrm_priv_area->common.usedby == WP_SWITCH && bstrm_priv_area->sw_dev){
		bitstrm_private_area_t *sw_chan=
			(bitstrm_private_area_t *)bstrm_priv_area->sw_dev->priv;	
		sdla_t *sw_card=sw_chan->card;

		DEBUG_EVENT( "SWITCH DIFF: ISR=%li  Rx=%li Tx=%li RxBh=%li\n",
				card->statistics.isr_entry-sw_card->statistics.isr_entry,
				card->statistics.isr_rx-sw_card->statistics.isr_rx,
				card->statistics.isr_tx-sw_card->statistics.isr_tx,
				card->statistics.poll_entry-sw_card->statistics.poll_entry);
	}

	DEBUG_TEST("%s: Intr cnt %i Intr skip %i Irq Equalize %i ISR SKIP=0x%x\n",
			card->devname,
			atomic_read(&intr_cnt),
			atomic_read(&intr_skip),
			card->irq_equalize,
			card->u.b.wait_for_buffers);
	DEBUG_EVENT( "\n");

#endif
	
#if 0
	/* TE1	Change the update_comms_stats variable to 3,
	 * 	only for T1/E1 card, otherwise 2 for regular
	 *	S514/S508 card.
	 *	Each timer interrupt will update only one type
	 *	of statistics.
	 */
	bstrm_priv_area->update_comms_stats = 
		(IS_TE1_CARD(card) || IS_56K_CARD(card)) ? 3 : 2;
	
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	if (test_bit(0,&card->in_isr)){
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
		return -EBUSY;
	}
	update_comms_stats(card, bstrm_priv_area);
	bstrm_priv_area->update_comms_stats--;
	update_comms_stats(card, bstrm_priv_area);
	bstrm_priv_area->update_comms_stats--;
	update_comms_stats(card, bstrm_priv_area);
	bstrm_priv_area->update_comms_stats--;
	
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
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
	int i;
	struct sk_buff *skb;
	sdla_t* card = wandev->priv;
	bitstrm_private_area_t* bstrm_priv_area;
	unsigned long smp_flags;
	int err=0;

	DEBUG_EVENT( "%s: Configuring Interface: %s\n",
			card->devname, conf->name);
 
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		DEBUG_EVENT( "%s: Invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}

	if (card->u.b.serial && card->wandev.new_if_cnt){
		DEBUG_EVENT( "%s: Error: multipe interfaces are only allowed on T1/E1 cards!\n",
				card->devname);
		return -EEXIST;
	}
		
	/* allocate and initialize private data */
	bstrm_priv_area = kmalloc(sizeof(bitstrm_private_area_t), GFP_KERNEL);
	
	if(bstrm_priv_area == NULL) 
		return -ENOMEM;

	memset(bstrm_priv_area, 0, sizeof(bitstrm_private_area_t));

	memcpy(&bstrm_priv_area->cfg, &conf->u.bitstrm, sizeof(wan_bitstrm_conf_if_t));

	bstrm_priv_area->card = card; 

	bstrm_priv_area->common.sk=NULL;
	bstrm_priv_area->common.state = WAN_DISCONNECTED;
	bstrm_priv_area->common.dev = dev;
	
	bstrm_priv_area->common.config_id=card->wandev.config_id;

	bstrm_priv_area->debug_stream=0;
	bstrm_priv_area->debug_char=0xFF;
	
	/* Initialize circular buffers for each interface
	 * and allocate skb buffers. This must be done first
	 * because the new_if_error, tries to purge the
	 * rx_free_queue. 
	 *
	 * THUS: DO NOT GO TO new_if_error ABOVE HERE !
	 */
	
	skb_queue_head_init(&bstrm_priv_area->rx_free_queue);
	skb_queue_head_init(&bstrm_priv_area->rx_used_queue);
	skb_queue_head_init(&bstrm_priv_area->tx_queue);

	/* initialize interface name */
	strcpy(bstrm_priv_area->if_name, conf->name);

	for (i=0;i<MAX_RX_FREE_SKB;i++){
		skb=dev_alloc_skb(MAX_MTU_SIZE);
		if (!skb){
			err=-ENOMEM;
			goto new_if_error;
		}
		skb_queue_tail(&bstrm_priv_area->rx_free_queue,skb);
	}

	/* Obtain a free skb buffer that will be used to construct
	 * a bitstream */
	bstrm_priv_area->rx_skb=skb_dequeue(&bstrm_priv_area->rx_free_queue);
	if (!bstrm_priv_area->rx_skb){
		/* Santiy check this should never happen because
		 * we just allocated it */
		err=-ENOMEM;
		goto new_if_error;
	}


	if (IS_TE1_CARD(card)){

		/* Channel definition section. If not channels defined 
		 * return error */

		if (IS_E1_CARD(card) && !IS_FR_FEUNFRAMED(&card->fe)){
			conf->active_ch=conf->active_ch&0x7FFFFFFF;
		}

		if ((bstrm_priv_area->time_slot_map=conf->active_ch) == 0){
			DEBUG_EVENT( "%s: Invalid Channel Selection 0x%lX\n",
					card->devname,bstrm_priv_area->time_slot_map);
			err=-EINVAL;
			goto new_if_error;
		}

		DEBUG_EVENT( "%s: %s: Iface cfg channels = 0x%08lX\n",
				card->devname,conf->name,bstrm_priv_area->time_slot_map);
	
		
		/* Default the max_tx_up_size */
		bstrm_priv_area->max_tx_up_size=MAX_T1_CHAN_TX_UP_SIZE;
		if (IS_E1_CARD(card)){
			if (IS_FR_FEUNFRAMED(&card->fe)){
				bstrm_priv_area->max_tx_up_size=MAX_E1_UNFRM_CHAN_TX_UP_SIZE;
			}else{
				bstrm_priv_area->max_tx_up_size=MAX_E1_CHAN_TX_UP_SIZE;
			}
		}

			
		/* If user defined value exists, us it */
		if (bstrm_priv_area->cfg.max_tx_up_size){
			bstrm_priv_area->max_tx_up_size=bstrm_priv_area->cfg.max_tx_up_size;
		}
		
		for (i=0;i<card->u.b.time_slots;i++){
			if (test_bit(i,&bstrm_priv_area->time_slot_map)){	
				bstrm_priv_area->time_slots++;
			}
		}
	
		if (bstrm_priv_area->max_tx_up_size % bstrm_priv_area->time_slots){
			bstrm_priv_area->max_tx_up_size-=
					bstrm_priv_area->max_tx_up_size % 
					bstrm_priv_area->time_slots;
			bstrm_priv_area->max_tx_up_size+=bstrm_priv_area->time_slots;
		}
		
	}else{
		bstrm_priv_area->max_tx_up_size=MAX_T1_CHAN_TX_UP_SIZE;
		if (bstrm_priv_area->cfg.max_tx_up_size){
			bstrm_priv_area->max_tx_up_size=bstrm_priv_area->cfg.max_tx_up_size;
		}
	}

	bstrm_priv_area->rx_max_timeout=HZ;
	bstrm_priv_area->rx_timeout=jiffies;
	bstrm_priv_area->tx_idle_flag=card->u.b.cfg.monosync_tx_time_fill_char;
	
	bstrm_priv_area->hdlc_eng=conf->hdlc_streaming;
	bstrm_priv_area->bits_in_byte=BITSINBYTE;
	if (bstrm_priv_area->hdlc_eng){
		bstrm_priv_area->tx_idle_flag=0x7E;
		bstrm_priv_area->seven_bit_hdlc = bstrm_priv_area->cfg.seven_bit_hdlc;
		if (bstrm_priv_area->seven_bit_hdlc){
			bstrm_priv_area->bits_in_byte = 7;
		}
	}

	bstrm_priv_area->max_tx_queue_sz=MAX_TX_QUEUE_SIZE;
	if (bstrm_priv_area->cfg.max_tx_queue_size){
		bstrm_priv_area->max_tx_queue_sz=bstrm_priv_area->cfg.max_tx_queue_size;
	}

	/* Setup wanpipe as a router (WANPIPE) or as an API */
	if( strcmp(conf->usedby, "WANPIPE") == 0) {

		bstrm_priv_area->protocol=conf->protocol;
		bstrm_priv_area->common.usedby=WANPIPE;
		bstrm_priv_area->hdlc_eng=1;
		bstrm_priv_area->tx_idle_flag=0x7E;

		DEBUG_EVENT( "%s: Running in WANPIPE mode !\n",
			wandev->name);

		if (conf->protocol != WANOPT_NO){
			wan_netif_set_priv(dev,bstrm_priv_area);
			if ((err=protocol_init(card,dev,bstrm_priv_area,conf)) != 0){
				goto new_if_error;
			}

			if (conf->ignore_dcd == WANOPT_YES || conf->ignore_cts == WANOPT_YES){
				DEBUG_EVENT( "%s: Ignore modem changes DCD/CTS\n",card->devname);
				bstrm_priv_area->ignore_modem=1;
			}else{
				DEBUG_EVENT( "%s: Restart protocol on modem changes DCD/CTS\n",
						card->devname);
			}
		}
		
	} else if( strcmp(conf->usedby, "API") == 0) {
		bstrm_priv_area->common.usedby=API;
		bstrm_priv_area->protocol=0;
		DEBUG_EVENT( "%s: Running in API mode !\n",
			wandev->name);
		
	} else if( strcmp(conf->usedby, "STACK") == 0) {
		bstrm_priv_area->common.usedby=STACK;
		bstrm_priv_area->protocol=0;
		DEBUG_EVENT( "%s: Running in STACK mode !\n",
			wandev->name);
			
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG	
	}else if (strcmp(conf->usedby, "ANNEXG") == 0) {
		printk(KERN_INFO "%s:%s: Interface running in ANNEXG mode!\n",
			wandev->name,bstrm_priv_area->if_name);
		bstrm_priv_area->common.usedby=ANNEXG;	
			
		if (strlen(conf->label)){
			strncpy(bstrm_priv_area->label,conf->label,WAN_IF_LABEL_SZ);
		}
#endif
		
	} else if( strcmp(conf->usedby, "SWITCH") == 0) {
		bstrm_priv_area->common.usedby=WP_SWITCH;
		bstrm_priv_area->protocol=0;
		DEBUG_EVENT( "%s: Running in SWITCH mode !\n",
			wandev->name);

#if 1
		if (card->u.b.cfg.rx_complete_length != card->u.b.cfg.max_length_tx_data_block){
			DEBUG_EVENT( "%s: Config Error: SWITCH tx and rx data size must be equal! (Tx=%i != Rx=%i)\n",
					card->devname,
					card->u.b.cfg.max_length_tx_data_block,
					card->u.b.cfg.rx_complete_length);
			err=-EINVAL;
			goto new_if_error;
		}
		
		bstrm_priv_area->max_tx_up_size=card->u.b.cfg.rx_complete_length;
#endif

		set_bit(WAIT_DEVICE_BUFFERS,&bstrm_priv_area->tq_control);
		
	}else{
		DEBUG_EVENT( "%s: Error, Invalid operation mode, API only!\n",
						wandev->name);
	
		err=-EINVAL;
		goto new_if_error;
	}

	/* prepare network device data space for registration */

	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);

	wan_netif_set_priv(dev,bstrm_priv_area);
	bstrm_priv_area->common.dev = dev;

	bstrm_priv_area->hdlc_flag=0;
	set_bit(NO_FLAG,&bstrm_priv_area->hdlc_flag);
	DEBUG_EVENT( "\n");

	bstrm_priv_area->rx_crc[0]=-1;
	bstrm_priv_area->rx_crc[1]=-1;
	bstrm_priv_area->rx_crc[2]=-1;
	bstrm_priv_area->tx_crc=-1;
	bstrm_priv_area->tx_flag= 0x7E; //card->u.b.cfg.monosync_tx_time_fill_char;
	bstrm_priv_area->tx_flag_idle= 0x7E; //card->u.b.cfg.monosync_tx_time_fill_char;

	if (bstrm_priv_area->common.usedby == WP_SWITCH){
		strncpy(bstrm_priv_area->sw_if_name,conf->sw_dev_name,WAN_IFNAME_SZ);
	}

	if (IS_TE1_CARD(card)){	
		/* Check that the time slot is not being used. If it is
		 * stop the interface setup.  Notice, though we proceed
		 * to check for all timeslots before we start binding 
		 * the channels in.  This way, we don't have to go back
		 * and clean the time_slot_map */ 
		for (i=0;i<card->u.b.time_slots;i++){
			if (test_bit(i,&bstrm_priv_area->time_slot_map)){

				DEBUG_CFG( "Configuring %s for timeslot %i\n",
						conf->name, i+1);

				bstrm_priv_area->rbs_chan=i+1;

				if (IS_T1_CARD(card)){
					if (wan_test_bit(i,&card->u.b.cfg.rbs_map)){
						bstrm_priv_area->rbs_on=1;
					}
				}

				if (IS_E1_CARD(card)){
					if (card->u.b.time_slot_map[i+1]){
						DEBUG_EVENT( "%s: Channel/Time Slot resource conflict!\n",
								card->devname); 
						DEBUG_EVENT( "%s: %s: Channel %i, aready in use!\n",
								card->devname,dev->name,(i+1));
						
						err=-EEXIST;
						goto new_if_error;
					}
				}else{	
					if (card->u.b.time_slot_map[i]){
						DEBUG_EVENT( "%s: Channel/Time Slot resource conflict!\n",
								card->devname); 
						DEBUG_EVENT( "%s: %s: Channel %i, aready in use!\n",
								card->devname,dev->name,(i+1));
						
						err=-EEXIST;
						goto new_if_error;
					}
				}
			}
		}


		if (bstrm_priv_area->rbs_on){
			if (bstrm_priv_area->time_slots > 1){
				DEBUG_EVENT("%s:%s: Error: Interface with multiple timeslots %i!\n",
						card->devname,bstrm_priv_area->if_name,
						bstrm_priv_area->time_slots);
				DEBUG_EVENT("%s:%s:        Configured for Robbit Signalling!!!\n",
						card->devname,bstrm_priv_area->if_name);
				DEBUG_EVENT("%s:%s:        Config for single timeslot per iface!\n",
						card->devname,bstrm_priv_area->if_name);
				DEBUG_EVENT("\n");
				err = -EINVAL;
				goto new_if_error;
			}

			if (bstrm_priv_area->hdlc_eng && !bstrm_priv_area->seven_bit_hdlc){
				DEBUG_EVENT("%s:%s: Warning: Forcing hdlc eng to 7bit hdlc, rbs on!\n",
						card->devname,bstrm_priv_area->if_name);
				bstrm_priv_area->seven_bit_hdlc=1;
			}
		}

		/* WARNING: The new_if() function should not fail
		 *          After this point */
		
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		/*Bind an interface to a time slot map. The resource
		 * check was done above. We need to lock this area because
		 * the tx isr and rx bottom half are running */
		for (i=0;i<card->u.b.time_slots;i++){
			if (test_bit(i,&bstrm_priv_area->time_slot_map)){	
				if (IS_E1_CARD(card)){
					card->u.b.time_slot_map[i+1] = bstrm_priv_area;
				}else{
					card->u.b.time_slot_map[i] = bstrm_priv_area;
				}
			}
		}
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
		
	}

	DEBUG_EVENT( "\n");
	DEBUG_EVENT( "%s: Configuring %s: \n",
			card->devname,
			bstrm_priv_area->if_name);

	if (IS_TE1_CARD(card)){
	DEBUG_EVENT( "%s:      TimeSlot Bind Map =0x%08lX\n",
			card->devname,
			bstrm_priv_area->time_slot_map);
	}
	DEBUG_EVENT( "%s:      Tx Idle Flag      =0x%X\n",
			card->devname,
			bstrm_priv_area->tx_idle_flag);

	DEBUG_EVENT( "%s:      Max Tx Queue Size =%d\n",
			card->devname,
			bstrm_priv_area->max_tx_queue_sz);

	if (IS_TE1_CARD(card)){
	DEBUG_EVENT( "%s:      Max Tx Up Size    =%i\n",
			card->devname,
			bstrm_priv_area->max_tx_up_size);
	}
	
	DEBUG_EVENT( "%s:      HDLC Engine       =%s  Bits=%d\n",
			card->devname,
			bstrm_priv_area->hdlc_eng?"ON":"OFF",
			bstrm_priv_area->seven_bit_hdlc?7:8);

	DEBUG_EVENT( "\n");

	card->wandev.new_if_cnt++;
	
	return 0;

new_if_error:

	if (bstrm_priv_area){
		bstrm_skb_queue_purge(&bstrm_priv_area->rx_free_queue);

		if (bstrm_priv_area->rx_skb){
			dev_kfree_skb_any(bstrm_priv_area->rx_skb);
			bstrm_priv_area->rx_skb=NULL;
		}

		protocol_shutdown(card,dev);
		
		bstrm_priv_area->common.dev=NULL;
		kfree(bstrm_priv_area);
	}

	wan_netif_set_priv(dev,NULL);

	return err;
}

/*============================================================================
 * Delete logical channel.
 */
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	bitstrm_private_area_t* bstrm_priv_area = wan_netif_priv(dev);
	bitstrm_private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = wandev->priv;
	int i;
	unsigned long smp_flags;

	if (!bstrm_priv_area){
		return 0;
	}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG	
	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		netdevice_t *tmp_dev;
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
	
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	
	if (test_bit(0,&card->in_isr)){
		DEBUG_EVENT( "%s: Del if critical with ISR\n",
				card->devname);
	}

	if (IS_TE1_CARD(card)){
		/* Un Bind an interface to a time slot map */
		for (i=0;i<card->u.b.time_slots;i++){
			if (test_bit(i,&bstrm_priv_area->time_slot_map)){
				if (IS_E1_CARD(card)){
					card->u.b.time_slot_map[i+1] = NULL;
				}else{
					card->u.b.time_slot_map[i] = NULL;
				}
			}
		}
	}
	if (bstrm_priv_area->rx_skb){
		dev_kfree_skb_any(bstrm_priv_area->rx_skb);
		bstrm_priv_area->rx_skb=NULL;
	}

	if (bstrm_priv_area->tx_skb){
		dev_kfree_skb_any(bstrm_priv_area->tx_skb);
		bstrm_priv_area->tx_skb=NULL;
	}

	bstrm_skb_queue_purge(&bstrm_priv_area->tx_queue);
	bstrm_skb_queue_purge(&bstrm_priv_area->rx_free_queue);
	bstrm_skb_queue_purge(&bstrm_priv_area->rx_used_queue);

	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	protocol_shutdown(card,dev);

	card->wandev.new_if_cnt--;

	return 0;
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
	bitstrm_private_area_t* bstrm_priv_area = wan_netif_priv(dev);
	sdla_t* card = bstrm_priv_area->card;
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
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&if_do_ioctl);

	/* Initialize media-specific parameters */
	if (!bstrm_priv_area->protocol){
		dev->flags		|= IFF_POINTOPOINT;
		dev->flags		|= IFF_NOARP;
	}
	
	/* Enable Mulitcasting if user selected */
	if (!bstrm_priv_area->protocol && bstrm_priv_area->mc == WANOPT_YES){
		dev->flags 	|= IFF_MULTICAST;
	}
	
	if (!bstrm_priv_area->protocol){
		dev->type	= ARPHRD_PPP;
	}

	if (bstrm_priv_area->common.usedby == WP_SWITCH){
		dev->mtu = card->u.b.cfg.max_length_tx_data_block;
	}else{
		if (!bstrm_priv_area->protocol){
			dev->mtu = card->wandev.mtu;
		}
	}
	
	/* for API usage, add the API header size to the requested MTU size */
	if(bstrm_priv_area->common.usedby == API) {
		dev->mtu += sizeof(api_tx_hdr_t);
	}

	if (!bstrm_priv_area->protocol){
		dev->hard_header_len	= 0;
	}
	
	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

	/* Set transmit buffer queue length 
	 * If too low packets will not be retransmitted 
         * by stack.
	 */
        dev->tx_queue_len = 100;
   
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
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	bitstrm_private_area_t* chan= (bitstrm_private_area_t*)wan_netif_priv(dev);
	unsigned long smp_flags;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	int err=0;
	custom_control_call_t custom_control_pkt;
	
	if (!chan){
		return -ENODEV;
	}
	card=chan->card;

	NET_ADMIN_CHECK();

	switch(cmd)
	{
	
		case SIOC_WANPIPE_GET_TIME_SLOTS:
			err=card->u.b.time_slots;
			break;
		
		case SIOC_WANPIPE_GET_MEDIA_TYPE:
			err=card->fe.fe_cfg.media;
			break;
			
		case SIOC_WANPIPE_BIND_SK:

			if (!ifr){
				err= -EINVAL;
				break;
			}
			
			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_bind_api_to_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

			if (!err){
				chan->ifstats.tx_fifo_errors=0;
				chan->ifstats.rx_errors=0;
			}
			break;

		case SIOC_WANPIPE_UNBIND_SK:

			if (!ifr){
				err= -EINVAL;
				break;
			}

			spin_lock_irqsave(&card->wandev.lock,smp_flags);
			err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

			if (chan->hdlc_eng){
				init_rx_hdlc_eng(card,chan,1);
			}
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

		
		case SIOC_WANPIPE_BITSTRM_T1E1_CFG:

			DEBUG_DBG("%s: SIOC_WANPIPE_BITSTRM_T1E1_CFG\n", card->devname);

			if (card->comm_enabled){
				spin_lock_irqsave(&card->wandev.lock, smp_flags);
				bstrm_disable_comm_shutdown (card);
				spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
			}

			if (copy_from_user(&card->fe.fe_cfg,ifr->ifr_data,sizeof(sdla_fe_cfg_t))){
				DEBUG_EVENT("%s: Error: Failed to copy from user in ioctl()\n",
						card->devname);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			if (IS_TE1_CARD(card)) {
			
				DEBUG_DBG("%s: (IOCTL) for %s selecting : \n",
					card->devname, 
					(IS_T1_CARD(card))?"T1":"E1");

				if(IS_T1_CARD(card)){

					DEBUG_DBG("INTERFACE_LEVEL_V35\n");
					
					card->wandev.electrical_interface = WANOPT_V35;
					card->u.b.cfg.rx_complete_length = 720;
					card->u.b.cfg.max_length_tx_data_block = 720;
					card->u.b.time_slots = NO_ACTIVE_RX_TIME_SLOTS_T1;
				}else{
					DEBUG_DBG("INTERFACE_LEVEL_RS232\n");
					
					card->wandev.electrical_interface = WANOPT_RS232;
					//Must be less than original 720, because
					//default .conf file is for T1.
					//It is max len which can be pushed into 
					//the "pre allocated" sock buff.
					card->u.b.cfg.rx_complete_length = 704;	 //divisible by 32
					card->u.b.cfg.max_length_tx_data_block = 682;//divisible by 31
					card->u.b.time_slots = NO_ACTIVE_RX_TIME_SLOTS_E1;
				}
				if (card->wandev.fe_iface.post_unconfig){
					card->wandev.fe_iface.post_unconfig(&card->fe);
				}
				if (card->wandev.fe_iface.unconfig){
					card->wandev.fe_iface.unconfig(&card->fe);
				}
			}

			err=config_bstrm(card);
			spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
		
			return err;
			
		case SIOC_CUSTOM_BITSTRM_COMMANDS:

			DEBUG_DBG("SIOC_CUSTOM_BITSTRM_COMMANDS\n");
			
			if(copy_from_user(&custom_control_pkt, ifr->ifr_data, sizeof(custom_control_call_t)) ){
				return -EFAULT;
			}

			switch(custom_control_pkt.control_code) {
		
			case SET_BIT_IN_PMC_REGISTER:
				
				if (IS_TE1_CARD(card)){
					unsigned char val = 0;
					unsigned char new_val;
						
					val = card->fe.read_fe_reg(card->hw, 0, 1, custom_control_pkt.reg);
					DEBUG_DBG("%s: read val : 0x%02X\n", card->devname, val);

					new_val = val | (0x01 << custom_control_pkt.bit_number);
					
					err = card->fe.write_fe_reg(card->hw, 0, 1, custom_control_pkt.reg, new_val);
					if(err){
						printk(	KERN_INFO
							"%s:SET_BIT_IN_PMC_REGISTER command failed! err : 0x%02X\n",
								card->devname, err);
					}

					val = card->fe.read_fe_reg(card->hw, 0, 1, custom_control_pkt.reg);
					DEBUG_DBG("%s: read val after OR : 0x%02X\n", card->devname, val);

				}else{
					DEBUG_EVENT(
						"%s:SET_BIT_IN_PMC_REGISTER command is invalid for the card type!\n",
						card->devname);
					err = 1;
				}
				break;
				
			case RESET_BIT_IN_PMC_REGISTER:
				
				if (IS_TE1_CARD(card)){
					unsigned char val = 0;
					unsigned char new_val;
						
					val = card->fe.read_fe_reg(card->hw, 0, 1, custom_control_pkt.reg);
					new_val = val &= (~(0x01 << custom_control_pkt.bit_number));
					
					err = card->fe.write_fe_reg(card->hw, 0, 1, custom_control_pkt.reg, new_val);
					if(err){
						DEBUG_EVENT(
						"%s:RESET_BIT_IN_PMC_REGISTER command failed! err : 0x%02X\n",
						card->devname, err);
					}
				}else{
					DEBUG_EVENT(
						"%s:RESET_BIT_IN_PMC_REGISTER command is invalid for the card type!\n",
						card->devname);
					err = 1;
				}
				
				break;
			}
			
			return err;

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
			if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,ifr->ifr_data,sizeof(wan_udp_hdr_t))){
				atomic_set(&chan->udp_pkt_len,0);
				return -EFAULT;
			}

			spin_lock_irqsave(&card->wandev.lock, smp_flags);

			/* We have to check here again because we don't know
			 * what happened during spin_lock */
			if (test_bit(0,&card->in_isr)) {
				DEBUG_EVENT( "%s:%s Pipemon command failed, Driver busy: try again.\n",
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
			
			if (atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
				DEBUG_EVENT( "%s: Error: Pipemon buf too bit on the way up! %i\n",
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

		case SIOC_WRITE_RBS_SIG:
			{
			unsigned char abcd_bits;	

			if (!chan->rbs_on){
				DEBUG_EVENT("%s: Error: Write RBS bits signalling turned off!\n",
						chan->if_name);
				return -EINVAL;
			}
			
			if (copy_from_user(&abcd_bits,ifr->ifr_data,1)){
				return -EFAULT;
			}
			
			card->wandev.fe_iface.set_rbsbits(
						&card->fe,
						chan->rbs_chan,
						abcd_bits);
			err=0;
			}
			break;
			
		case SIOC_READ_RBS_SIG:

			if (!chan->rbs_on){
				DEBUG_EVENT("%s: Error: Write RBS bits signalling turned off!\n",
						chan->if_name);
				return -EINVAL;
			}
		
			
			if (copy_to_user(ifr->ifr_data,&chan->rbs_sig,1)){
				return -EFAULT;
			}
			
			err=0;
			break;
			
		default:
			if (chan->protocol == WANCONFIG_PPP || 
			    chan->protocol == WANCONFIG_CHDLC){
				return wp_sppp_do_ioctl(dev,ifr,cmd);		
			}
			return -EOPNOTSUPP;
	}
	return err;
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
	bitstrm_private_area_t* bstrm_priv_area = wan_netif_priv(dev);
	sdla_t* card = bstrm_priv_area->card;
	struct timeval tv;
	int err = 0;

	/* Only one open per interface is allowed */

	if (open_dev_check(dev))
		return -EBUSY;

	do_gettimeofday(&tv);
	bstrm_priv_area->router_start_time = tv.tv_sec;

	netif_start_queue(dev);

	protocol_start(card,dev);

	if (bstrm_priv_area->common.usedby == WP_SWITCH){
		int err;
		err=bstrm_bind_dev_switch(card,bstrm_priv_area,bstrm_priv_area->sw_if_name);
		if (err){
			DEBUG_EVENT( "%s: Error: Device %s failed to bind to %s\n",
					card->devname,bstrm_priv_area->if_name,
					bstrm_priv_area->sw_if_name);
			
			return err;
		}else{
			bitstrm_private_area_t *sw_chan;
			sdla_t *sw_card;
			
			if (bstrm_priv_area->sw_dev && 
			    bstrm_priv_area->common.state != WAN_CONNECTED){
				
				sw_chan=(bitstrm_private_area_t *)wan_netif_priv(bstrm_priv_area->sw_dev);
				sw_card=sw_chan->card;
				
				if (!sw_card->comm_enabled || !card->comm_enabled){
					DEBUG_EVENT( "%s: Switch not connected: %s \n",
							bstrm_priv_area->common.dev->name,
							bstrm_priv_area->sw_dev->name);
				}else{
					unsigned long smp_flags;
					DEBUG_EVENT( "%s: Setting switch to connected %s\n",
							bstrm_priv_area->common.dev->name,
							bstrm_priv_area->sw_dev->name);

					wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
					sw_chan->common.state = WAN_CONNECTED;
					bstrm_priv_area->common.state=WAN_CONNECTED;

					set_bit(WAIT_DEVICE_BUFFERS,&bstrm_priv_area->tq_control);
					set_bit(WAIT_DEVICE_BUFFERS,&sw_chan->tq_control);
					
					wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
				}
			}
		}
	}else{
		bstrm_priv_area->common.state=card->wandev.state;
		DEBUG_EVENT( "%s: Interface %s: %s\n",
				card->devname,
				dev->name,
				STATE_DECODE(bstrm_priv_area->common.state));
	}
	
	wanpipe_open(card);

	return err;
}

/*============================================================================
 * Close network interface.
 * o if this is the last close, then disable communications and interrupts.
 * o reset flags.
 */


static int if_close (netdevice_t* dev)
{
	bitstrm_private_area_t* bstrm_priv_area = wan_netif_priv(dev);
	sdla_t* card = bstrm_priv_area->card;
	unsigned long smp_flags;
			
	stop_net_queue(dev);

	protocol_stop(card,dev);

	if (bstrm_priv_area->common.usedby == WP_SWITCH){
		wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
		bstrm_unbind_dev_switch(card,bstrm_priv_area);
		wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	}

	bstrm_priv_area->common.state=WAN_DISCONNECTED;
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	wan_update_api_state(bstrm_priv_area);
	wan_unbind_api_from_svc(bstrm_priv_area,bstrm_priv_area->common.sk);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

#if defined(LINUX_2_1)
	dev->start=0;
#endif
	wanpipe_close(card);
	return 0;
}


static void disable_comm (sdla_t *card)
{
	unsigned long	smp_flags=0;

	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

	if (card->u.b.cfg.rbs_map){
		bstrm_disable_te_signaling(card,card->u.b.cfg.rbs_map);
	}
	
	if (card->comm_enabled){
		bstrm_disable_comm_shutdown (card);
	}else{
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);	
	}

	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	
	tasklet_kill(&card->u.b.wanpipe_rx_task);
	tasklet_kill(&card->u.b.wanpipe_tx_task);
	
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	bstrm_skb_queue_purge(&card->u.b.rx_isr_queue);
	bstrm_skb_queue_purge(&card->u.b.rx_isr_free_queue);
	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);


	/* TE1 - Unconfiging */
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


/*============================================================================
 * Handle transmit timeout event from netif watchdog
 */
static void if_tx_timeout (netdevice_t *dev)
{
    	bitstrm_private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	
	/* If our device stays busy for at least 5 seconds then we will
	 * kick start the device by making dev->tbusy = 0.  We expect
	 * that our device never stays busy more than 5 seconds. So this                 
	 * is only used as a last resort.
	 */

	++card->wandev.stats.collisions;

	printk (KERN_INFO "%s: Transmit timed out on %s\n", card->devname,dev->name);
	netif_wake_queue (dev);

	if (chan->common.usedby == API){
		wan_wakeup_api(chan);
	}else if (chan->common.usedby == STACK){
		wanpipe_lip_kick(chan,0);
	}
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	if (chan->common.usedby == ANNEXG && 
			  chan->annexg_dev){
		if (IS_FUNC_CALL(lapb_protocol,lapb_mark_bh)){
			lapb_protocol.lapb_mark_bh(chan->annexg_dev);
		}
	}
#endif
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
	bitstrm_private_area_t *bstrm_priv_area = wan_netif_priv(dev);
	sdla_t *card = bstrm_priv_area->card;
	unsigned long smp_flags;

	if (skb == NULL){
		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		DEBUG_EVENT( "%s: interface %s got kicked!\n",
			card->devname, dev->name);

		wake_net_dev(dev);
		return 0;
	}

	spin_lock_irqsave(&card->wandev.lock,smp_flags);	
	if (skb_queue_len(&bstrm_priv_area->tx_queue) > bstrm_priv_area->max_tx_queue_sz){
		bstrm_priv_area->tick_counter = jiffies;
		if (card->u.b.serial){
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_BLOCK);
		}
		WAN_NETIF_STOP_QUEUE(dev);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
		return 1;
	}
	spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

	
#if defined(LINUX_2_1)
	if (dev->tbusy){

		/* If our device stays busy for at least 5 seconds then we will
		 * kick start the device by making dev->tbusy = 0.  We expect 
		 * that our device never stays busy more than 5 seconds. So this
		 * is only used as a last resort. 
		 */
                ++card->wandev.stats.collisions;
		if((jiffies - bstrm_priv_area->tick_counter) < (5 * HZ)) {
			return 1;
		}
		if_tx_timeout(dev);
	}
#endif

	if (test_bit(WAIT_DEVICE_BUFFERS,&bstrm_priv_area->tq_control) &&
	    skb_queue_len(&bstrm_priv_area->tx_queue) >= TX_QUEUE_MID){		

		bitstrm_private_area_t *sw_chan=NULL;

		if (net_ratelimit()){
			DEBUG_EVENT("%s: Balancing switching devices, limiting tx!\n",
					bstrm_priv_area->if_name);
		}
		dev_kfree_skb_any(skb);

		if (bstrm_priv_area->sw_dev){
			sw_chan=(bitstrm_private_area_t *)wan_netif_priv(bstrm_priv_area->sw_dev);
			if (sw_chan){
				if (test_bit(WAIT_DEVICE_BUFFERS,&sw_chan->tq_control) &&
					skb_queue_len(&sw_chan->tx_queue) >= TX_QUEUE_MID){
					
					DEBUG_EVENT("%s: Switching devices balanced %d %d\n",
							sw_chan->if_name,
							skb_queue_len(&bstrm_priv_area->tx_queue),
							skb_queue_len(&sw_chan->tx_queue));	

					sw_chan->tq_control=0;
					bstrm_priv_area->tq_control=0;
				}
			}
		} 	

	}else if(card->u.b.state != WAN_CONNECTED || 
	   	bstrm_priv_area->common.state != WAN_CONNECTED){

       		++card->wandev.stats.tx_dropped;
		bstrm_priv_area->ifstats.tx_carrier_errors++;
		
		dev_kfree_skb_any(skb);
	
	}else if(test_bit(SEND_CRIT,&card->wandev.critical)){

		++card->wandev.stats.tx_dropped;
		bstrm_priv_area->ifstats.tx_carrier_errors++;
		DEBUG_EVENT( "SEND CRIT\n");

		dev_kfree_skb_any(skb);
		
	}else{

			
#if 0
		//FIXME: NENAD DEBUG
		dev_kfree_skb_any(skb);
		dev->trans_start = jiffies;
		goto if_send_exit_crit;	
#endif

		/* If it's an API packet pull off the API
		 * header. Also check that the packet size
		 * is larger than the API header
	         */

		wan_skb_unlink(skb);

		if (bstrm_priv_area->common.usedby == WP_SWITCH){
			wanpipe_switch_datascope_tx_up(bstrm_priv_area,skb);
		}

		if (bstrm_priv_area->common.usedby == API){
			if (skb->len <= sizeof(api_tx_hdr_t)){
				++card->wandev.stats.tx_dropped;
				bstrm_priv_area->ifstats.tx_dropped++;
				dev_kfree_skb_any(skb);
				goto if_send_exit_crit;
			}
			skb_pull(skb,sizeof(api_tx_hdr_t));
		}
		
		if (bstrm_priv_area->common.usedby != WP_SWITCH &&
		    bstrm_priv_area->hdlc_eng){
			if (hdlc_encode(bstrm_priv_area,&skb) != 0){
				++card->wandev.stats.tx_dropped;
				bstrm_priv_area->ifstats.tx_dropped++;
				
				dev_kfree_skb_any(skb);
				goto if_send_exit_crit;
			}
		}

#if 0
		{
			int i;
			DEBUG_EVENT( "\n");
			DEBUG_EVENT( "\n");
			DEBUG_EVENT( "Tx Packet: idle 0x%02X \n",bstrm_priv_area->tx_flag_idle);
			for (i=0;i<skb->len;i++){
				printk("%02X ",skb->data[i]);
			}
			printk("\n");
		}
#endif

		card->wandev.stats.tx_packets++;
		card->wandev.stats.tx_bytes+=skb->len;
		bstrm_priv_area->ifstats.tx_packets++;
		bstrm_priv_area->ifstats.tx_bytes += skb->len;

		spin_lock_irqsave(&card->wandev.lock,smp_flags);
		if (bstrm_priv_area->common.usedby != WP_SWITCH &&
		    bstrm_priv_area->hdlc_eng){
			bstrm_priv_area->tx_idle_flag = bstrm_priv_area->tx_flag_idle;
		}

		skb_queue_tail(&bstrm_priv_area->tx_queue,skb);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

		bstrm_priv_area->tick_counter = jiffies;
		
		if (card->u.b.serial && 
		    !test_bit(WAIT_DEVICE_BUFFERS,&bstrm_priv_area->tq_control)){
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_BLOCK);
		}
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	 	dev->trans_start = jiffies;
#endif
	
	}

if_send_exit_crit:

	spin_lock_irqsave(&card->wandev.lock,smp_flags);
	start_net_queue(dev);
	spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
	
	return 0;
}


/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	bitstrm_private_area_t* bstrm_priv_area;

	if ((bstrm_priv_area=wan_netif_priv(dev)) == NULL)
		return NULL;

	return &bstrm_priv_area->ifstats;
}

/****** Cisco HDLC Firmware Interface Functions *******************************/

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str. 
 */
static int bstrm_read_version (sdla_t* card, char* str)
{
	int data_len = 0;

	wan_mbox_t* mb = &card->wan_mbox;
	int rc;
	memset(mb, 0, sizeof(wan_cmd_t));
	mb->wan_data_len = 0;
	mb->wan_command = READ_BSTRM_CODE_VERSION;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (rc != WAN_CMD_TIMEOUT){
		if (str) {  /* is not null */
			data_len = mb->wan_data_len;
			memcpy(str, mb->wan_data, data_len);
			str[data_len] = '\0';
		}
	}else{
		bstrm_error(card,rc,mb);
	}
	return (rc);
}

/*-----------------------------------------------------------------------------
 *  Configure BSTRM firmware.
 */
static int bstrm_configure (sdla_t* card, void* data)
{
	int err;
	
	wan_mbox_t *mb = &card->wan_mbox;
	memset(mb, 0, sizeof(wan_cmd_t));
	memcpy(mb->wan_data, data, sizeof(BSTRM_CONFIGURATION_STRUCT));
	mb->wan_command = SET_BSTRM_CONFIGURATION;
	mb->wan_data_len = sizeof(BSTRM_CONFIGURATION_STRUCT);  

	err=card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if (err){
		DEBUG_EVENT("%s: BSTRM CFG ERR: Len=%i  Data=0x%X\n",
			card->devname,mb->wan_data_len,mb->wan_data[0]);		
	}
	return err;
}


/*============================================================================
 * Set interrupt mode -- HDLC Version.
 */

static int bstrm_set_intr_mode (sdla_t* card, unsigned mode)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int rc;
	BSTRM_INT_TRIGGERS_STRUCT* int_data =
		 (BSTRM_INT_TRIGGERS_STRUCT *)mb->wan_data;

	memset(mb, 0, sizeof(wan_cmd_t));
	int_data->BSTRM_interrupt_triggers 	= mode;
	int_data->IRQ				= card->wandev.irq;	//ALEX_TODAY card->hw.irq;
	int_data->interrupt_timer               = 1;
   
	mb->wan_data_len = sizeof(BSTRM_INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_BSTRM_INTERRUPT_TRIGGERS;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (rc == WAN_CMD_TIMEOUT)
		bstrm_error (card, rc, mb);
	return rc;
}


/*===========================================================
 * bstrm_disable_comm_shutdown
 *
 * Shutdown() disables the communications. We must
 * have a sparate functions, because we must not
 * call bstrm_error() hander since the private
 * area has already been replaced */

static int bstrm_disable_comm_shutdown (sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int rc;
	BSTRM_INT_TRIGGERS_STRUCT* int_data =
		 (BSTRM_INT_TRIGGERS_STRUCT *)mb->wan_data;

	memset(mb, 0, sizeof(wan_cmd_t));
	/* Disable Interrutps */
	int_data->BSTRM_interrupt_triggers 	= 0;
	int_data->IRQ				= card->wandev.irq;	//ALEX_TODAY card->hw.irq;
	int_data->interrupt_timer               = 1;
   
	mb->wan_command = SET_BSTRM_INTERRUPT_TRIGGERS;
	mb->wan_data_len = sizeof(BSTRM_INT_TRIGGERS_STRUCT);
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	/* Disable Communications */
	mb->wan_command = DISABLE_BSTRM_COMMUNICATIONS;
	mb->wan_data_len = 0;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	card->comm_enabled = 0;
	
	return 0;
}

/*============================================================================
 * Enablenications.
 */

static int bstrm_comm_enable (sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int rc;

	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_BSTRM_COMMUNICATIONS;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (rc == WAN_CMD_TIMEOUT)
		bstrm_error(card, rc, mb);
	else
		card->comm_enabled = 1;

	return rc;
}

/*============================================================================
 * Read communication error statistics.
 */
static int bstrm_read_comm_err_stats (sdla_t* card)
{
        int rc;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_COMMS_ERROR_STATS;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (rc == WAN_CMD_TIMEOUT)
                bstrm_error(card,rc,mb);
        return rc;
}


/*============================================================================
 * Read BSTRM operational statistics.
 */
static int bstrm_read_op_stats (sdla_t* card)
{
        int rc;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_BSTRM_OPERATIONAL_STATS;
        rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (rc == WAN_CMD_TIMEOUT)
                bstrm_error(card,rc,mb);
        return rc;
}


/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card,
	bitstrm_private_area_t* bstrm_priv_area)
{
        wan_mbox_t* mb = &card->wan_mbox;
  	COMMS_ERROR_STATS_STRUCT* err_stats;
        BSTRM_OPERATIONAL_STATS_STRUCT *op_stats;

	if(bstrm_priv_area->update_comms_stats == 3) {
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

	 }else { 
		/* 2. On the second timer interrupt, read the comms error 
	 	 * statistics 
	 	 */
		if(bstrm_priv_area->update_comms_stats == 2) {
			if(bstrm_read_comm_err_stats(card))
				return 1;
			err_stats = (COMMS_ERROR_STATS_STRUCT *)mb->wan_data;
			card->wandev.stats.rx_over_errors = 
				err_stats->Rx_overrun_err_count;
			card->wandev.stats.rx_fifo_errors = 
				err_stats->Rx_dis_pri_bfrs_full_count; 
			card->wandev.stats.rx_missed_errors =
				card->wandev.stats.rx_fifo_errors;
		} else {

        		/* on the third timer interrupt, read the operational 
			 * statistics 
		 	 */
        		if(bstrm_read_op_stats(card))
                		return 1;
			op_stats = (BSTRM_OPERATIONAL_STATS_STRUCT *)mb->wan_data;
		}
	}

	return 0;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int bstrm_send (sdla_t* card, void* data, unsigned len, unsigned char flag)
{
	BSTRM_DATA_TX_STATUS_EL_STRUCT txbuf;

	card->hw_iface.peek(card->hw, card->u.b.txbuf_off, &txbuf, sizeof(txbuf));
	if (txbuf.opp_flag)
		return 1;

	if (flag){
		txbuf.misc_Tx_bits = (volatile unsigned char)UPDATE_TX_TIME_FILL_CHAR;
		txbuf.Tx_time_fill_char = flag;
	}else{
		txbuf.misc_Tx_bits=0;
	}	

	txbuf.block_length = len;

	/* WARNING:
	 * The txbuf->block_lenght and txbuf->opp_flag should
	 * never execute one after another.  Since opp_flag and
	 * block_lenght reside beside each other in memory, 
	 * some ISA busses will write, OUT OF ORDER, to the 
	 * oppflag before the block length is updated */

	card->hw_iface.poke(card->hw, txbuf.ptr_data_bfr, data, len);
	
	txbuf.opp_flag = 1;		/* start transmission */
	card->hw_iface.poke(card->hw, card->u.b.txbuf_off, &txbuf, sizeof(txbuf));
	
	/* Update transmit buffer control fields */
	card->u.b.txbuf_off += sizeof(txbuf);
	if (card->u.b.txbuf_off > card->u.b.txbuf_last_off)
		card->u.b.txbuf_off = card->u.b.txbuf_base_off;
	return 0;
}

/*============================================================================
 * Enable timer interrupt  
 */
static void bstrm_enable_timer (void* card_id)
{
	sdla_t* 		card = (sdla_t*)card_id;

	card->u.b.timer_int_enabled |= TMR_INT_ENABLED_TE;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	return;
}

/****** Firmware Error Handler **********************************************/

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int bstrm_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

	case CMD_TIMEOUT:
		DEBUG_EVENT( "%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		break;

	case T1_E1_AMI_NOT_SUPPORTED:
		DEBUG_EVENT( "%s: AMI decoding not supported\n",
				card->devname);
		break;
		
	case S514_BOTH_PORTS_SAME_CLK_MODE:
		if(cmd == SET_BSTRM_CONFIGURATION) {
			DEBUG_EVENT(
			 "%s: Configure both ports for the same clock source\n",
				card->devname);
			break;
		}

	default:
		DEBUG_EVENT( "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

/********** Bottom Half Handlers ********************************************/

/* NOTE: There is no API, BH support for Kernels lower than 2.2.X.
 *       DO NOT INSERT ANY CODE HERE, NOTICE THE 
 *       PREPROCESSOR STATEMENT ABOVE, UNLESS YOU KNOW WHAT YOU ARE
 *       DOING 
 */

static void bstrm_tx_bh (unsigned long data)
{
#define card ((sdla_t*)data)
	BSTRM_DATA_TX_STATUS_EL_STRUCT txbuf;
	bitstrm_private_area_t* chan;	
	int ch=0;
	char kick_tx_isr=0;
	char frame_multiple=0;

	set_bit(8, &card->u.b.tq_working);
	
	if (test_bit(SEND_CRIT,&card->wandev.critical)){
		clear_bit(SEND_CRIT,&card->wandev.critical);
	}
	
	card->hw_iface.peek(card->hw, card->u.b.txbuf_off, &txbuf, sizeof(txbuf));
	if (test_bit(0,(unsigned long*)&txbuf.opp_flag)){
		DEBUG_EVENT( "%s: Error: Tx buffer busy in Tx intr!\n",
				card->devname);
		goto tx_bh_exit;
	}

#if 0
	//FIXME: NENAD DEBUG
	goto tx_nenad_skip;
#endif	
	card->u.b.tx_scratch_buf_len=0;
	
	if (card->u.b.serial){
		/* For non T1/E1 cards, send a whole skb buffer
		 * down the stack.  i.e. no channelization */
		struct sk_buff *skb;
		netdevice_t	*dev;

		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));

		if (!dev || !(dev->flags & IFF_UP)){
			DEBUG_EVENT("%s: Dev down\n",__FUNCTION__);
			goto tx_bh_exit;				
		}
		
		chan = (bitstrm_private_area_t *)wan_netif_priv(dev); 
		if (!chan){
			goto tx_bh_exit;
		}
	
		if (chan->common.usedby == WP_SWITCH && 
		    test_bit(WAIT_DEVICE_BUFFERS,&chan->tq_control)){
			DEBUG_EVENT("%s: Warning: Tx Intr while waiting for buffers\n",
					chan->common.dev->name);
		}

		if (is_queue_stopped(chan->common.dev)){	
			if (chan->common.usedby == API){
				start_net_queue(chan->common.dev);	
				wan_wakeup_api(chan);
			}else if (chan->common.usedby == WP_SWITCH){
				start_net_queue(chan->common.dev);
			}else{
				wake_net_dev(chan->common.dev);
			}
		}

		skb=skb_dequeue(&chan->tx_queue);	
		if (!skb){
			goto tx_bh_exit;		
		}
	
		if (bstrm_send(card,skb->data,skb->len,chan->hdlc_eng?skb->cb[0]:0)){
			DEBUG_EVENT( "%s: Error: Tx Bh: failed to send buffer busy!\n",
					card->devname);
		}

		dev_kfree_skb_any(skb);

		if (skb_queue_len(&chan->tx_queue)){
			card->hw_iface.set_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_BLOCK);
		}
		goto tx_bh_exit;
	}

	for (;;){

		/* Do not tx on E1 channel 0 since it is a signalling
		 * channel */
		
		if (!IS_FR_FEUNFRAMED(&card->fe) && card->u.b.time_slots == NUM_OF_E1_CHANNELS && ch == 0){
			goto tx_time_slot_handled;
		}
	
		chan=(bitstrm_private_area_t*)card->u.b.time_slot_map[ch];
		if (!chan || !(chan->common.dev->flags & IFF_UP)){
			DEBUG_TEST("%s: Timeslot %i not configured!\n",
					card->devname,(ch+1));
			card->u.b.tx_scratch_buf[card->u.b.tx_scratch_buf_len]=
			card->u.b.cfg.monosync_tx_time_fill_char;
			card->u.b.tx_scratch_buf_len++;
			goto tx_time_slot_handled;
		}

		if (!chan->tx_skb){

			if (is_queue_stopped(chan->common.dev)){	
				if (chan->common.usedby == API){
					start_net_queue(chan->common.dev);	
					wan_wakeup_api(chan);
				}else if (chan->common.usedby == STACK){
					start_net_queue(chan->common.dev);	
					wanpipe_lip_kick(chan,0);
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
				}else if (chan->common.usedby == ANNEXG && 
			  		chan->annexg_dev){
					if (IS_FUNC_CALL(lapb_protocol,lapb_mark_bh)){
						lapb_protocol.lapb_mark_bh(chan->annexg_dev);
					}	
#endif		
				}else if (chan->common.usedby == WP_SWITCH){
					start_net_queue(chan->common.dev);
				}else{
					wake_net_dev(chan->common.dev);
				}
			}
			
			if (chan->common.usedby == WP_SWITCH &&
			    test_bit(WAIT_DEVICE_BUFFERS,&chan->tq_control)){
				card->u.b.tx_scratch_buf[card->u.b.tx_scratch_buf_len]=chan->tx_idle_flag;
				card->u.b.tx_scratch_buf_len++;
				goto tx_time_slot_handled;
			}
			
			chan->tx_skb = skb_dequeue(&chan->tx_queue);
			if (!chan->tx_skb){
#if 0
				if (net_ratelimit()){
				DEBUG_TEST("%s: Channel %s: no data \n",
						card->devname,chan->if_name);
				}
#endif
				card->u.b.tx_scratch_buf[card->u.b.tx_scratch_buf_len]=chan->tx_idle_flag;
				card->u.b.tx_scratch_buf_len++;
				chan->ifstats.tx_fifo_errors++;
				goto tx_time_slot_handled;
			}
		}
		
		if (!chan->tx_skb || chan->tx_skb->len == 0){
#if 0		
			if (net_ratelimit()){
			DEBUG_EVENT("%s: Channel %s: %i skb empty\n",
					card->devname,chan->if_name,ch+1);
			}
#endif
			card->u.b.tx_scratch_buf[card->u.b.tx_scratch_buf_len]=chan->tx_idle_flag;
			card->u.b.tx_scratch_buf_len++;
			chan->ifstats.tx_fifo_errors++;
			
			if (chan->tx_skb){
				dev_kfree_skb_any(chan->tx_skb);
				chan->tx_skb=NULL;
			}
			
			goto tx_time_slot_handled;
		}

		card->u.b.tx_scratch_buf[card->u.b.tx_scratch_buf_len]=chan->tx_skb->data[0];
		card->u.b.tx_scratch_buf_len++;

		skb_pull(chan->tx_skb,1);
	
		/* We must check for existance of the pointer,
		 * because the channel might be unconfigued */
		if (chan->tx_skb->len < 1){
			dev_kfree_skb_any(chan->tx_skb);
			chan->tx_skb=NULL;
		}else{
			kick_tx_isr=1;
		}
		
tx_time_slot_handled:

		/* How many times should we go around */
		if (++ch >= card->u.b.time_slots){
			ch=0;
			if (++frame_multiple >= card->u.b.tx_chan_multiple){ 
				break;
			}
		}
	}

#if 0
//NENAD
tx_nenad_skip:

#endif

	if (bstrm_send(card,&card->u.b.tx_scratch_buf[0],card->u.b.tx_scratch_buf_len,0)){
		DEBUG_EVENT( "%s: Error: Tx Bh: failed to send buffer busy!\n",
				card->devname);
	}

tx_bh_exit:
	clear_bit(8, &card->u.b.tq_working);

	
#undef card
}

static void bstrm_rx_bh (unsigned long data)
{
#define card  ((sdla_t*)data)
	struct sk_buff *skb;
	bitstrm_private_area_t* chan;
	int ch=0,stream_full=0;
	volatile unsigned char *buf;

	bstrm_test_rx_tx_discard(card);
	
	if (!skb_queue_len(&card->u.b.rx_isr_queue)){
		while(bstrm_bh_data_tx_up(card,NULL,NULL));
		card->statistics.poll_tbusy_bad_status++;
		return;
	}

	card->statistics.poll_entry++;

	while ((skb=skb_dequeue(&card->u.b.rx_isr_queue)) != NULL){
	
		if (!skb->len){
			DEBUG_EVENT( "RX Error, skb=NULL or skb->len=0 \n");
			bstrm_bh_cleanup(card,skb);
			continue;	
		}

		ch=0;

		if (card->u.b.serial){
			netdevice_t	*dev;

			dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));

			if (!dev || !(dev->flags&IFF_UP)){
				bstrm_bh_cleanup(card,skb);
				continue;
			}
			
			chan = (bitstrm_private_area_t *)wan_netif_priv(dev); 
			if (!chan){
				bstrm_bh_cleanup(card,skb);
				continue;
			}

			if (chan->common.usedby == WP_SWITCH){

				bstrm_bh_data_tx_up(card,skb,chan);
				
				bstrm_switch_send(card,chan,skb);
				
				bstrm_bh_cleanup(card,skb);

			}else{
				bstrm_bh_data_tx_up(card,skb,NULL);
				bstrm_bh_cleanup(card,skb);
			}
			continue;
		}

		for (;;){

			chan=(bitstrm_private_area_t*)card->u.b.time_slot_map[ch];
			if (!chan || !(chan->common.dev->flags & IFF_UP)){
				DEBUG_TEST("%s: Timeslot %i (ch=%i) not configured!\n",
						card->devname,(ch+1),ch);
				skb_pull(skb,1);
				card->wandev.stats.rx_over_errors=ch;
				card->wandev.stats.rx_fifo_errors++;
				goto rx_time_slot_handled;
			}
		
			if (chan->rbs_on){
				if (chan->rbs_sig != card->u.b.rbs_sig[ch]){
					chan->rbs_sig=card->u.b.rbs_sig[ch];
					send_rbs_oob_msg (card,chan);
					DEBUG_TEST("%s: RBS Change 0x%X\n",
							chan->if_name,chan->rbs_sig);
				}
			}
			
			if (!chan->rx_skb){
				/* The free queues might be busy, try
				 * here again. If we fail then we have a buffering
				 * problem. Increment a dropped packet */
				
				DEBUG_TEST("%s: Rx_SKB from free queue! Free Q Len=%i\n",
						card->devname,skb_queue_len(&chan->rx_free_queue));
				chan->rx_skb=skb_dequeue(&chan->rx_free_queue);
				if (!chan->rx_skb){
					DEBUG_EVENT( "%s: Timeslot %i no rx buf!\n",
							card->devname,(ch+1));
					chan->ifstats.rx_dropped++;
					skb_pull(skb,1);
					goto rx_time_slot_handled;
				}
				chan->rx_timeout=jiffies;
			}

			if (skb_queue_len(&chan->rx_used_queue)){
				stream_full=1;
			}

			buf=skb_put(chan->rx_skb,1);
			buf[0]=skb->data[0];
			skb_pull(skb,1);

#if 0
//NENAD
			
			if (ch > 1){
				if (buf[0] == 0x20+(ch-1)){
					if (!chan->debug_stream){
						DEBUG_EVENT("%s: Ch=%i:Got First Sync = 0x%X Len=%i\n",
								chan->common.dev->name,ch,buf[0],chan->rx_skb->len);
						chan->debug_stream=1;	
						chan->debug_char=buf[0];
					}
				}else{
					if (chan->debug_stream){
						DEBUG_EVENT("%s: Ch=%i:Out of sync First = 0x%X Idle=0x%X Len=%i\n",
								chan->common.dev->name,ch,buf[0],chan->tx_idle_flag,
								chan->rx_skb->len);
						chan->debug_stream=0;	
					}
				}
			}
#endif			
			
			if (chan->rx_skb->len >= chan->max_tx_up_size){
				
				/* A bit stream has reached maximum size.
				 * we must pass it to the next layer, ether
				 * HDLC decoder or API. For now just push it
				 * into the used queue */
			
				if (chan->common.usedby == WP_SWITCH){
					//if (skb->len != 0){
					//	DEBUG_EVENT( "%s: WP_SWITCH Sending %i, orig skb %i\n",
					//	      card->devname,chan->rx_skb->len, skb->len);
					//}
				
					/* Try to pass the packet up the stack if
					 * the API is registered */
					bstrm_bh_data_tx_up(card, NULL, chan);
					
					/* Send the packet to our switched device*/
					bstrm_switch_send(card,chan,chan->rx_skb);

					/* Re-initialize the buffer, to be reused */
					chan->rx_skb->data=(chan->rx_skb->head+16);
					chan->rx_skb->len=1;
					skb_trim(chan->rx_skb,0);

				}else{
					skb_queue_tail(&chan->rx_used_queue,chan->rx_skb);
					chan->rx_skb=NULL;
					chan->rx_skb=skb_dequeue(&chan->rx_free_queue);
					stream_full=1;
				}
				chan->rx_timeout=jiffies;
				if (chan->rx_skb && chan->rx_skb->len){
					DEBUG_EVENT( "%s: Rx got new buffer skb len=%i\n",
							chan->common.dev->name,chan->rx_skb->len);
				}
			}

rx_time_slot_handled:
			if (skb->len < 1){
				break;
			}
			
			if (++ch >= card->u.b.time_slots){
				ch=0;
			}
		
		}//for(;;)
		
		/* Initialize the rx_intr skb buffer, and get it ready
		 * for another use */

		bstrm_bh_cleanup(card,skb);

		if (!stream_full){
			continue;
		}

		stream_full=bstrm_bh_data_tx_up(card,NULL,NULL);
	}

	while (stream_full){
		stream_full=bstrm_bh_data_tx_up(card,NULL,NULL);
	}

	return;
#undef card
}


static void bstrm_switch_send(sdla_t *card, bitstrm_private_area_t * chan, struct sk_buff *skb)
{
	struct sk_buff *new_skb;

	if (!skb)
		return;

	if (chan->sw_dev && (chan->sw_dev->flags&IFF_UP)){

		if (is_queue_stopped(chan->sw_dev)){
			DEBUG_EVENT( "%s:%s: (Debug): Warning: dev busy %s\n",
					card->devname,chan->if_name,chan->sw_dev->name); 
			goto switch_send_done;
		}

		if (!test_bit(WAIT_DEVICE_BUFFERS,&chan->tq_control)){
		
			if (skb_queue_len(&chan->tx_queue) <  TX_QUEUE_LOW){
				DEBUG_EVENT("%s: Device queue low %i\n",
						chan->common.dev->name,
						skb_queue_len(&chan->tx_queue));

			}else if (skb_queue_len(&chan->tx_queue) > TX_QUEUE_HIGH){
				DEBUG_EVENT("%s: Device queue high %i\n",
						chan->common.dev->name,
						skb_queue_len(&chan->tx_queue));
			}
		}
			
		new_skb=skb_copy(skb,GFP_ATOMIC);
		if (!new_skb){
			DEBUG_EVENT( "%s: ERROR: BH SW failed to allocate memory !\n",
					card->devname);
			card->wandev.stats.rx_errors++;
			chan->ifstats.rx_errors++;
		}else{
			if (!WAN_NETDEV_TEST_XMIT(chan->sw_dev) || WAN_NETDEV_XMIT(new_skb,chan->sw_dev) != 0){
				kfree(new_skb);
				DEBUG_EVENT( "%s: Failed to send on SW dev %s\n",
						chan->if_name,chan->sw_dev->name);
				goto switch_send_done;
			}
			card->wandev.stats.rx_packets++;
			card->wandev.stats.rx_bytes+=skb->len;
			chan->ifstats.rx_packets++;
			chan->ifstats.rx_bytes+=skb->len;
		}
	}else{
		chan->ifstats.rx_fifo_errors++;
	}

switch_send_done:
	return;
}

static int bstrm_bh_data_tx_up(sdla_t*card, struct sk_buff *non_te1_skb, bitstrm_private_area_t *chan_ptr)
{
	struct wan_dev_le	*devle = NULL;
	netdevice_t 		*dev;
	bitstrm_private_area_t 	*chan;
	struct sk_buff *new_skb,*skb;
	int err=0;
	int switch_dev=0;
		
	/* Test if channel is specified */
	if ((chan=chan_ptr) != NULL){
		dev=chan->common.dev;

		/* Channel exists, the WP_SWITCH code is trying
		 * to send a packet up the stack.  Thus
		 * check if this device is actually in WP_SWITCH
		 * mode */
		if (dev && chan->common.usedby == WP_SWITCH){

			/* If API socket is not bound in
			 * then exit */
			if (chan->common.sk == NULL){
				return 0;
			}else{

				/* API exists, then get the
				 * skb buffer from the rx_skb */
				if (card->u.b.serial){
					skb=non_te1_skb;
				}else{
					skb=chan->rx_skb;
				}
				
				/* During Switching, only a single
				 * buffer is used, not a queue */
				if (skb){
				
					/* Buffer exists, thus, set the
					 * switch flag, indicating below
					 * not to re-initialize this buffer */
					switch_dev=1;
					goto switch_hdlc_send; 
				}else{

					/* This should never happen */
					return 0;
				}
			}
		}
	}
	
	/* The Serial cards, don't use queues, thus,
	 * they give us the skb buffer */
	if (card->u.b.serial && non_te1_skb==NULL){
		return err;	
	}

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (dev == NULL || wan_netif_priv(dev) == NULL)
			continue;
		chan = wan_netif_priv(dev);
	
		if (!card->u.b.serial){
			skb = skb_dequeue(&chan->rx_used_queue);
			if (!skb){
				continue;
			}

			if (skb_queue_len(&chan->rx_used_queue)){
				err=1;
			}
		}else{
			skb = non_te1_skb;
			if (!skb){
				return err;
			}
		}

		/* This is just a precaution, WP_SWITCH device
		 * shouldnt be here */
		if (chan->common.usedby == WP_SWITCH){
			goto tx_up_skb_recover;

		}else if (chan->common.usedby == API){
			/* Test if the API socket is bound in */
			if (chan->common.sk == NULL){
				chan->ifstats.rx_errors++;
				goto tx_up_skb_recover;
			}
		}

switch_hdlc_send:
		
		if (chan->hdlc_eng){
#if 0
			DEBUG_EVENT("HDLC: UP\n");
			DEBUG_EVENT( "PKT: ");
			{
				int i;
				for (i=0;i<skb->len;i++){
					if (skb->data[i] != 0x7E){
						DEBUG_EVENT("%s: Data = 0x%X\n",card->devname,skb->data);
					}
					//printk("%02X ",skb->data[i]);
				}
				//printk("\n");
			}
#endif
			hdlc_decode(chan,
				    (skb->data),
				    (skb->len));
		}else{

			if (skb_headroom(skb) < sizeof(api_rx_hdr_t)){
				new_skb=skb_copy_expand(skb,sizeof(api_rx_hdr_t),0,GFP_ATOMIC);
			}else{
				new_skb=skb_copy(skb,GFP_ATOMIC);
			}

			if (!new_skb){
				DEBUG_EVENT( "%s: ERROR: Failed to clone skb buffer, no memory !\n",
						card->devname);
				++card->wandev.stats.rx_errors;
				chan->ifstats.rx_errors++;
			
			}else{
				unsigned char *buf;	
				int api_err;

#if 0
//NENAD
			if (chan->debug_char == 0x25){
			int i,x;
			static int out_of_sync_off=0;
			for(i=0;i<new_skb->len;i++){	
				if (new_skb->data[i] == chan->debug_char){
					if (!chan->debug_stream_1){
						DEBUG_EVENT("%s: Got Second Sync = 0x%X Offset=%i\n",
								chan->common.dev->name,new_skb->data[i],i);
						chan->debug_stream_1=1;

						if (out_of_sync_off && out_of_sync_off < i){
							for (x=out_of_sync_off;x<=i;x++){
								DEBUG_EVENT("0x%X\n",new_skb->data[x]);
							}
							out_of_sync_off=0;
						}
					}
				}else{
					if (chan->debug_stream_1){
						DEBUG_EVENT("%s: Out of sync Sec = 0x%X Idle=0x%X Offset=%i\n",
								chan->common.dev->name,new_skb->data[i],
								chan->tx_idle_flag,i);
						chan->debug_stream_1=0;	
						out_of_sync_off=i;
					}
				}
			}
			}
#endif		

				if (chan->common.usedby == STACK){	
					
					if (wanpipe_lip_rx(chan,new_skb) != 0){
						wan_skb_free(new_skb);
						++card->wandev.stats.rx_dropped;
						++chan->ifstats.rx_dropped;
						goto tx_up_skb_recover;
					}

//ANNEXG RECEIVE
					 
				} else{
					
					buf = skb_push(new_skb,sizeof(api_rx_hdr_t));
					memset(buf, 0, sizeof(api_rx_hdr_t));

					new_skb->protocol = htons(PVC_PROT);
					wan_skb_reset_mac_header(new_skb);
					new_skb->dev      = dev;
					new_skb->pkt_type = WAN_PACKET_DATA;

					api_err=wan_api_rx(chan,new_skb);

					if (api_err != 0){
						/* Sock full cannot send, queue us for another
						 * try */
						if (net_ratelimit()){
							DEBUG_EVENT( "%s: Error: Rx sock full err %d used 0x%lx sk %p\n",
									card->devname,\
									err,
									chan->common.used,
									chan->common.sk);
						}
						wan_skb_free(new_skb);

						++card->wandev.stats.rx_dropped;
						++chan->ifstats.rx_dropped;

						wan_wakeup_api(chan);
						goto tx_up_skb_recover;
					}

				}
				
				++card->wandev.stats.rx_packets;
				card->wandev.stats.rx_bytes+=skb->len;
				chan->ifstats.rx_packets++;
				chan->ifstats.rx_bytes+=skb->len;
			
			} //if new_skb
			
		}//if HDLC_DECODE

tx_up_skb_recover:
			
		if (switch_dev){
			err=0;
			break;
		}
		
		if (!card->u.b.serial){
			skb->data=(skb->head+16);
			skb->len=1;
			skb_trim(skb,0);
			skb_queue_tail(&chan->rx_free_queue,skb);
		}else{
			break;
		}
	}//for()

	return err;
}

static int bstrm_bh_cleanup (sdla_t *card,struct sk_buff *skb)
{
	if (!skb)
		return 0;

	/* Reset the buffer to zero */
	skb->data = skb->head + 16;
	/* This is a trick. The skb_trim checks if the skb->len is greater
	 * than the new len.  Thus, fake the skb->len to 1 */
	skb->len=1;
	skb_trim(skb,0);

	skb_queue_tail(&card->u.b.rx_isr_free_queue,skb);
	return 0;
}

static struct sk_buff * bh_enqueue (sdla_t *card)
{
	/* Check for full */
	struct sk_buff *skb;

	skb=skb_dequeue(&card->u.b.rx_isr_free_queue);
	if (!skb){
		DEBUG_TEST( "%s: Error: No skb in bh_enqueue\n",
				card->devname);
		return NULL;
	}

	if (skb->len){
		DEBUG_EVENT( "%s: Error: Bh Enqueue Skb len %i\n",
				card->devname,skb->len);
		return NULL;
	}

	return skb;
}

/* END OF API BH Support */


/* HDLC Bitstream Decode Functions */

static void hdlc_decode (bitstrm_private_area_t *chan, unsigned char *buf, int len)
{
	int i;
	int word_len=len-(len%4);
	int found=0;

	/* Before decoding the packet, make sure that the current
	 * bit stream contains data. Decoding is very expensive,
	 * thus perform word (32bit) comparision test */
#if 1	
	for (i=0;i<word_len;i+=4){
		if ((*(unsigned int*)&buf[i]) != *(unsigned int*)buf){
			found=1;
			break;
		}
	}

	if ((len%4) && !found){
		for (i=word_len;i<len;i++){
			if (buf[i]!=buf[0]){
				found=1;
				break;
			}
		}
	}

#if 0
	{
		int x,init;
		init=0;
		for (x=0;x<len;x++){
			switch (buf[x]){

			case 0xFF:
			case 0x7E:
			case 0xFC:
			case 0xF9:
			case 0xF3:
			case 0xE7:
			case 0xCF:
			case 0x9F:
			case 0x3F:	
				goto skip_print_p;
			}

			if (init == 0){
				init++;
				DEBUG_EVENT( "\n");
				DEBUG_EVENT( "\n");
				DEBUG_EVENT( "RX Packet Xoffset %i: ",x);
			}

			printk("%x ",buf[x]);
skip_print_p:
		}

		if (init){
			printk("\n");
			DEBUG_EVENT( "\n");
		}
	}
#endif

#else
	found=1;
#endif
	/* Data found within the bitstream, proceed to decode
	 * the bitstream and pull out data packets */
	if (found){
	//	DEBUG_EVENT( "\n");
	//	DEBUG_EVENT( "HDLC DEC: Found packet offset%i\n ",i);
	//	DEBUG_EVENT( "Packet: ");
		for (i=0; i<len; i++){
			decode_byte(chan, &buf[i]);
		}
	}else{
		/* Found only flags, pass few flags into
		 * the hdlc engine just in case the packet
		 * ended on block boundary */
		for (i=0; i<4; i++){
			decode_byte(chan, &buf[i]);
		}

	}

	return;
}

static void decode_byte (bitstrm_private_area_t *chan, unsigned char *raw_byte)
{
	int i;
	unsigned long byte=*raw_byte;
	sdla_t *card=chan->card;

	/* Test each bit in an incoming bitstream byte.  Search
	 * for an hdlc flag 0x7E, six 1s in a row.  Once the
	 * flag is obtained, construct the data packets. 
	 * The complete data packets are sent up the API stack */
	
	for (i=0; i<BITSINBYTE; i++){

		if (chan->seven_bit_hdlc && i==7){
			continue;
		}
		
		if (test_bit(i,&byte)){
			/* Got a 1 */
			
			++chan->rx_decode_onecnt;
			
			/* Make sure that we received a valid flag,
			 * before we start decoding incoming data */
			if (!test_bit(NO_FLAG,&chan->hdlc_flag)){ 
				chan->rx_decode_buf[chan->rx_decode_len] |= (1 << chan->rx_decode_bit_cnt);
				
				if (++chan->rx_decode_bit_cnt >= BITSINBYTE){

					/* Completed a byte of data, update the
					 * crc count, and start on the next 
					 * byte.  */
					calc_rx_crc(chan);
#ifdef PRINT_PKT
					printk(" %02X", data);
#endif
					++chan->rx_decode_len;

					if (chan->rx_decode_len > HDLC_ENG_BUF_LEN){
						DEBUG_EVENT("%s: Error: Rx hdlc overflow\n",
									chan->if_name);
						init_rx_hdlc_eng(card,chan,1);
						++chan->ifstats.rx_frame_errors;
						++card->wandev.stats.rx_frame_errors;
					}else{
						chan->rx_decode_buf[chan->rx_decode_len]=0;
						chan->rx_decode_bit_cnt=0;
						chan->hdlc_flag=0;
						set_bit(CLOSING_FLAG,&chan->hdlc_flag);
					}
				}
			}
		}else{
			/* Got a zero */
			if (chan->rx_decode_onecnt == 5){
				
				/* bit stuffed zero detected,
				 * do not increment our decode_bit_count.
				 * thus, ignore this bit*/
				
			
			}else if (chan->rx_decode_onecnt == 6){
				
				/* Got a Flag */
				if (test_bit(CLOSING_FLAG,&chan->hdlc_flag)){
				
					/* Got a closing flag, thus asemble
					 * the packet and send it up the 
					 * stack */
					chan->hdlc_flag=0;
					set_bit(OPEN_FLAG,&chan->hdlc_flag);
				
					if (chan->rx_decode_len >= 3){
						
						GET_FIN_CRC_CNT(chan->crc_cur);
						FLIP_CRC(chan->rx_crc[chan->crc_cur],chan->crc_fin);
						DECODE_CRC(chan->crc_fin);
				
						/* Check CRC error before passing data up
						 * the API socket */
						if (chan->crc_fin==chan->rx_orig_crc){
							tx_up_decode_pkt(chan);
						}else{
							DEBUG_TEST("%s: Rx crc error, len=%i\n",
									chan->if_name,chan->rx_decode_len);
							++chan->ifstats.rx_crc_errors;
							++card->wandev.stats.rx_crc_errors;

							init_rx_hdlc_eng(card,chan,1);
						}
					}else{
						DEBUG_TEST("%s: Rx abt error\n",chan->if_name);
						++chan->ifstats.rx_frame_errors;
						++card->wandev.stats.rx_frame_errors;	
					}

				}else if (test_bit(NO_FLAG,&chan->hdlc_flag)){
					/* Got a very first flag */
					chan->hdlc_flag=0;	
					set_bit(OPEN_FLAG,&chan->hdlc_flag);
				}

				/* After a flag, initialize the decode and
				 * crc buffers and get ready for the next 
				 * data packet */
				init_rx_hdlc_eng(card,chan,0);
			}else{
				/* Got a valid zero, thus increment the
				 * rx_decode_bit_cnt, as a result of which
				 * a zero is left in the consturcted
				 * byte.  NOTE: we must have a valid flag */
				
				if (!test_bit(NO_FLAG,&chan->hdlc_flag)){ 	
					if (++chan->rx_decode_bit_cnt >= BITSINBYTE){
						calc_rx_crc(chan);
#ifdef PRINT_PKT
						printk(" %02X", data);
#endif
						++chan->rx_decode_len;

						if (chan->rx_decode_len > HDLC_ENG_BUF_LEN){
							DEBUG_EVENT("%s: Error: Rx hdlc overflow\n",
									chan->if_name);
							init_rx_hdlc_eng(card,chan,1);
							++chan->ifstats.rx_frame_errors;
							++card->wandev.stats.rx_frame_errors;	
						}else{
							chan->rx_decode_buf[chan->rx_decode_len]=0;
							chan->rx_decode_bit_cnt=0;
							chan->hdlc_flag=0;
							set_bit(CLOSING_FLAG,&chan->hdlc_flag);
						}
					}
				}
			}
			chan->rx_decode_onecnt=0;
		}
	}
	
	return;
}


static void init_crc(void)
{
	int i,j;

	for(i=0;i<256;i++){
		CRC_TABLE[i]=0;
		for (j=0;j<BITSINBYTE;j++){
			if (i & (1<<j)){
				CRC_TABLE[i] ^= MagicNums[j];
			}
		}
	}
}

static void calc_rx_crc(bitstrm_private_area_t *chan)
{
	INC_CRC_CNT(chan->crc_cur);

	/* Save the incoming CRC value, so it can be checked
	 * against the calculated one */
	chan->rx_orig_crc = (((chan->rx_orig_crc<<8)&0xFF00) | chan->rx_decode_buf[chan->rx_decode_len]);
	
	chan->rx_crc_tmp = (chan->rx_decode_buf[chan->rx_decode_len] ^ chan->rx_crc[chan->crc_prv]) & 0xFF;
	chan->rx_crc[chan->crc_cur] =  chan->rx_crc[chan->crc_prv] >> 8;
	chan->rx_crc[chan->crc_cur] &= 0x00FF;
	chan->rx_crc[chan->crc_cur] ^= CRC_TABLE[chan->rx_crc_tmp];
	chan->rx_crc[chan->crc_cur] &= 0xFFFF;
	INC_CRC_CNT(chan->crc_prv);
}

static void calc_tx_crc(bitstrm_private_area_t *chan,unsigned char byte)
{
	chan->tx_crc_tmp = (byte ^ chan->tx_crc) & 0xFF;
	chan->tx_crc =  chan->tx_crc >> 8;
	chan->tx_crc &= 0x00FF;
	chan->tx_crc ^= CRC_TABLE[chan->tx_crc_tmp];
	chan->tx_crc &= 0xFFFF;
}

static void tx_up_decode_pkt(bitstrm_private_area_t *chan)
{
	unsigned char *buf;
	sdla_t *card = chan->card;
	
	struct sk_buff *skb = dev_alloc_skb(chan->rx_decode_len+sizeof(api_rx_hdr_t));
	if (!skb){
		DEBUG_EVENT( "%s: HDLC Tx up: failed to allocate memory!\n",
				chan->card->devname);
		chan->card->wandev.stats.rx_dropped++;
		chan->ifstats.rx_dropped++;
		return;
	}

	if (chan->common.usedby==STACK){
				
		buf = skb_put(skb,chan->rx_decode_len-2);
		memcpy(buf, 
		       chan->rx_decode_buf, 
		       chan->rx_decode_len-2);
	
		if (wanpipe_lip_rx(chan,skb) != 0){
			dev_kfree_skb_any(skb);
			chan->card->wandev.stats.rx_dropped++;
			chan->ifstats.rx_dropped++;
		}else{
			chan->card->wandev.stats.rx_packets++;
			chan->card->wandev.stats.rx_bytes += chan->rx_decode_len-2;
			chan->ifstats.rx_packets++;
			chan->ifstats.rx_bytes+=chan->rx_decode_len-2;
		}
	
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG		
	} else if (chan->common.usedby == ANNEXG) {

		if (!chan->annexg_dev) {
			dev_kfree_skb_any(skb);
			++card->wandev.stats.rx_dropped;	
			chan->ifstats.rx_dropped++;
			return;
		} 

		if ((chan->rx_decode_len) <= 2) {
			DEBUG_EVENT("%s: Bad Rx Frame Length %i\n",
					card->devname,chan->rx_decode_len);
			dev_kfree_skb_any(skb);
			++card->wandev.stats.rx_dropped;
			chan->ifstats.rx_dropped++;
			return;
		}

		buf = skb_put(skb,chan->rx_decode_len-2);
		memcpy(buf, 
		       chan->rx_decode_buf, 
		       chan->rx_decode_len-2);
	
		skb->protocol = htons(ETH_P_X25);
		skb->dev = chan->annexg_dev;
		wan_skb_reset_mac_header(skb);
	
		if (IS_FUNC_CALL(lapb_protocol,lapb_rx)) {
			lapb_protocol.lapb_rx(chan->annexg_dev,skb);
			card->wandev.stats.rx_packets++;
			card->wandev.stats.rx_bytes += chan->rx_decode_len-2;
			chan->ifstats.rx_packets++;
			chan->ifstats.rx_bytes+=chan->rx_decode_len-2;
		} else {
			dev_kfree_skb_any(skb);
			++card->wandev.stats.rx_dropped;
		}
		
#endif
	}else if (chan->common.usedby==API || chan->common.usedby==WP_SWITCH){

		buf = skb_put(skb,sizeof(api_rx_hdr_t));
		memset(buf, 0, sizeof(api_rx_hdr_t));

		buf = skb_put(skb,chan->rx_decode_len);
		memcpy(buf, 
		       chan->rx_decode_buf, 
		       chan->rx_decode_len);

		skb->protocol = htons(PVC_PROT);
		wan_skb_reset_mac_header(skb);
		skb->dev      = chan->common.dev; 
		skb->pkt_type = WAN_PACKET_DATA;

		if (wan_api_rx(chan,skb) != 0){
			dev_kfree_skb_any(skb);
			chan->card->wandev.stats.rx_dropped++;
			chan->ifstats.rx_dropped++;
		}else{
			chan->card->wandev.stats.rx_packets++;
			chan->card->wandev.stats.rx_bytes += chan->rx_decode_len;
			chan->ifstats.rx_packets++;
			chan->ifstats.rx_bytes+=chan->rx_decode_len;
		}
	}else{
	
		if (chan->rx_decode_len <= 2){
			dev_kfree_skb_any(skb);
			chan->card->wandev.stats.rx_frame_errors++;
			chan->ifstats.rx_frame_errors++;
		}else{
			skb_reserve(skb,4);

			buf=skb_put(skb,(chan->rx_decode_len-2));
			memcpy(buf, 
			       chan->rx_decode_buf, 
			       chan->rx_decode_len-2);

			chan->card->wandev.stats.rx_packets++;
			chan->card->wandev.stats.rx_bytes += chan->rx_decode_len;
			chan->ifstats.rx_packets++;
			chan->ifstats.rx_bytes+=chan->rx_decode_len;

			wan_skb_reset_mac_header(skb);
			skb->dev      = chan->common.dev;
			
			if (chan->protocol == WANCONFIG_PPP || 
			    chan->protocol == WANCONFIG_CHDLC){
				
				skb->protocol = htons(ETH_P_WAN_PPP);
				wp_sppp_input(chan->common.dev,skb);
			
			}else{
				skb->protocol = htons(ETH_P_IP);
				netif_rx(skb);
			}
		}
	}
	return;

}

static void init_rx_hdlc_eng(sdla_t *card, bitstrm_private_area_t *bstrm_priv_area, int hard)
{
	if (hard){
		bstrm_priv_area->hdlc_flag=0;
		set_bit(NO_FLAG,&bstrm_priv_area->hdlc_flag);
	}

	bstrm_priv_area->rx_decode_len=0;
	bstrm_priv_area->rx_decode_buf[bstrm_priv_area->rx_decode_len]=0;
	bstrm_priv_area->rx_decode_bit_cnt=0;
	bstrm_priv_area->rx_decode_onecnt=0;
	
	bstrm_priv_area->rx_crc[0]=-1;
	bstrm_priv_area->rx_crc[1]=-1;
	bstrm_priv_area->rx_crc[2]=-1;
	bstrm_priv_area->crc_cur=0; 
	bstrm_priv_area->crc_prv=0;
}

static int hdlc_encode(bitstrm_private_area_t *chan,struct sk_buff **skb_ptr)
{
	struct sk_buff *skb=*skb_ptr;
	unsigned char crc_tmp;
	int i;
	unsigned char *data_ptr;
#if 0
	chan->tx_decode_len=0;

	/* First byte of the outgoing bit stream must be a flag.
	 * The current flag has been properly bit shifted based on the
	 * last packet we sent */
	DEBUG_TX("TX: Flag Idle 0x%02X, Offset Data 0x%02X,  BitCnt %i\n",
			chan->tx_flag_idle,chan->tx_flag_offset_data,chan->tx_flag_offset);

	chan->tx_decode_buf[chan->tx_decode_len] = chan->tx_flag_idle;
	chan->tx_decode_len++;
	chan->tx_decode_buf[chan->tx_decode_len] = chan->tx_flag_idle;
	chan->tx_decode_len++;

	/* Initialize the next byte in the encode buffer as
	 * well as the crc bytes */
	chan->tx_decode_buf[chan->tx_decode_len]=0;
	chan->tx_crc=-1;
	chan->tx_crc_fin=0;

	/* Before encoding data bytes, we must update the bit
	 * shift offset, casued by previous bit suffing. This,
	 * will shift the whole data packet by maximum of 7 bits */
	chan->tx_decode_bit_cnt=chan->tx_flag_offset;
	chan->tx_decode_buf[chan->tx_decode_len]=chan->tx_flag_offset_data;
	chan->tx_decode_onecnt=0;
#else
	chan->tx_decode_len=0;
	chan->tx_crc=-1;
	chan->tx_crc_fin=0;
	chan->tx_decode_onecnt=0;

	memset(&chan->tx_decode_buf[0],0,3);
	chan->tx_decode_bit_cnt=0;
	
#ifdef HDLC_IDLE_ABORT	
	chan->tx_flag_idle=0x7E;
	chan->tx_flag_offset_data=0;
	chan->tx_flag_offset=0;
	encode_byte(chan,&chan->tx_flag_idle,2);
#else
	encode_byte(chan,&chan->tx_flag_idle,2);
	encode_byte(chan,&chan->tx_flag_idle,2);
	encode_byte(chan,&chan->tx_flag_offset_data,2);
	
	if (!chan->seven_bit_hdlc || chan->tx_flag_offset < 5){
		chan->tx_decode_len--;
	}
#endif
	

	DEBUG_TX("TX: Flag Idle 0x%02X, Offset Data 0x%02X,  FlagBitCnt %i, DataBitCnt %i\n",
			chan->tx_flag_idle,
			chan->tx_flag_offset_data,
			chan->tx_flag_offset,
			chan->tx_decode_bit_cnt);

	if (chan->seven_bit_hdlc){
		chan->tx_decode_bit_cnt=
			((chan->tx_flag_offset+2)%chan->bits_in_byte);
	}else{
		chan->tx_decode_bit_cnt=chan->tx_flag_offset;
	}
	
	DEBUG_TX("TX: DataBitCnt %i\n",
			chan->tx_decode_bit_cnt);

	chan->tx_decode_onecnt=0;
#endif
	/* For all bytes in an incoming data packet, calculate
	 * crc bytes, and encode each byte into the outgoing
	 * bit stream (encoding buffer).  */
	for (i=0;i<skb->len;i++){
		calc_tx_crc(chan,skb->data[i]);
		encode_byte(chan,&skb->data[i],0);	
	}

	/* Decode and bit shift the calculated CRC values */
	FLIP_CRC(chan->tx_crc,chan->tx_crc_fin);
	DECODE_CRC(chan->tx_crc_fin);

	/* Encode the crc values into the bit stream 
	 * encode buffer */
	crc_tmp=(chan->tx_crc_fin>>8)&0xFF;	
	encode_byte(chan,&crc_tmp,0);
	crc_tmp=(chan->tx_crc_fin)&0xFF;
	encode_byte(chan,&crc_tmp,0);

	/* End the bit stream encode buffer with the
	 * closing flag */

	
	encode_byte(chan,&chan->tx_flag,1);

#ifdef HDLC_IDLE_ABORT
	chan->tx_flag_idle=0xFF;
	chan->tx_flag_offset_data=0;	
	encode_byte(chan,&chan->tx_flag_idle,2);
#endif

	/* We will re-use the incoming skb buffer to
	 * store the newly created encoded bit stream.
	 * Therefore, reset the skb buffer to 0, thus
	 * erasing the incoming data packet */
	skb_trim(skb,0);

	/* Make sure that the current skb buffer has
	 * enough room to hold the newly created encoded
	 * bit stream.  Due to bit stuffing, hdlc headers
	 * and crc the encoded bit stream is bigger than
	 * the incoming data packet.  If we need more room
	 * reallocate a new buffer */
	if (skb_tailroom(skb) < chan->tx_decode_len){
		struct sk_buff *skb2 = NULL;		
		skb2 = dev_alloc_skb(chan->tx_decode_len+15);
		if (!skb2){
			return -ENOMEM;
		}

		wan_dev_kfree_skb(skb,FREE_WRITE);
		skb=skb2;
		*skb_ptr=skb2;
	
		wan_skb_unlink(skb);
	}
	
	/* Copy the encoded bit stream into the skb buffer which will be
	 * sent down to the card */
	data_ptr=skb_put(skb,chan->tx_decode_len);
	memcpy(data_ptr,chan->tx_decode_buf,chan->tx_decode_len);
		
#if 0
	{
		int i;
		DEBUG_EVENT( "ENCPKT: ");
		for (i=0;i<chan->tx_decode_len;i++){
			printk("%02X ",	chan->tx_decode_buf[i]);		
		}
		printk("\n");
		DEBUG_EVENT( "\n");
	}
	
#endif
	
	/* Reset the encode buffer */
	chan->tx_decode_len=0;

	/* Record the tx idle flag that
	 * should follow after this packet
	 * is sent out the port */
	skb->cb[0]=chan->tx_flag_idle;

#if 0
	{
		int i;
		unsigned char *data=wan_skb_data(skb);
		DEBUG_EVENT("PKT: ");
		for (i=0;i<wan_skb_len(skb);i++){
			printk("%02X ",data[i]);
		}
		printk("\n");
	}
#endif	
	return 0;
}

static void encode_byte (bitstrm_private_area_t *chan, unsigned char *byte_ptr, int flag)
{
	int j;
	unsigned long byte=*byte_ptr;
	



	for (j=0;j<BITSINBYTE;j++){

		if (test_bit(j,&byte)){
			/* Got 1 */
			chan->tx_decode_buf[chan->tx_decode_len] |= (1<< chan->tx_decode_bit_cnt);		
			if (++chan->tx_decode_bit_cnt >= chan->bits_in_byte){
				++chan->tx_decode_len;
				chan->tx_decode_buf[chan->tx_decode_len]=0;
				chan->tx_decode_bit_cnt=0;
			}

			if (++chan->tx_decode_onecnt == 5){
				/* Stuff a zero bit */
				if (!flag){
					if (++chan->tx_decode_bit_cnt >= chan->bits_in_byte){
						++chan->tx_decode_len;
						chan->tx_decode_buf[chan->tx_decode_len]=0;
						chan->tx_decode_bit_cnt=0;
					}
				}
				chan->tx_decode_onecnt=0;
			}
		}else{
			/* Got 0 */
			chan->tx_decode_onecnt=0;
			if (++chan->tx_decode_bit_cnt >= chan->bits_in_byte){
				++chan->tx_decode_len;
				chan->tx_decode_buf[chan->tx_decode_len]=0;
				chan->tx_decode_bit_cnt=0;
			}
		}
	}

	if (flag == 1){
		/* The closing flag has been encoded into the 
		 * buffer. We must check how much has the last flag
		 * bit shifted due to bit stuffing of previous data.
		 * The maximum bit shift is 7 bits, thus a standard
		 * flag 0x7E can be have 7 different values.  The
		 * FLAG buffer will give us a correct flag, based
		 * on the bit shift count. */
		chan->tx_flag_idle = FLAG[chan->tx_decode_bit_cnt];
		chan->tx_flag_offset=chan->tx_decode_bit_cnt;
	
		/* The bit shifted part of the flag, that crossed the byte
		 * boudary, must be saved, and inserted at the beginning of 
		 * the next outgoing packet */
		chan->tx_flag_offset_data=chan->tx_decode_buf[chan->tx_decode_len];
	}

	return;
}


/****** Interrupt Handlers **************************************************/

/*============================================================================
 * Cisco HDLC interrupt service routine.
 */
static WAN_IRQ_RETVAL wpbit_isr (sdla_t* card)
{
	unsigned char	intr_type = 0x00;
	GLOBAL_INFORMATION_STRUCT global_info;
	int i;

	if (!card->hw){
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
	}

	card->statistics.isr_entry++;
	
	/* if critical due to peripheral operations
	 * ie. update() or getstats() then reset the interrupt and
	 * wait for the board to retrigger.
	 */
	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		DEBUG_EVENT( "ISR CRIT TO PERI\n");
		goto isr_done;
	}

	card->hw_iface.peek(card->hw, card->intr_type_off, &intr_type, 1);
	switch(intr_type) {

	case RX_APP_INT_PEND:	/* 0x01: receive interrupt */
		card->statistics.isr_rx++;
		rx_intr(card);
		break;

	case TX_APP_INT_PEND:	/* 0x02: transmit interrupt */

		card->statistics.isr_tx++;

		// Do not disable Tx interrupt
		// for TE1 cards.
		if(card->u.b.serial) {
			card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TX_BLOCK);
		}

		if (!card->u.b.sw_card){
		 	tasklet_hi_schedule(&card->u.b.wanpipe_tx_task);
		}else{
			bstrm_tx_bh((unsigned long)card);
		}
		break;

	case COMMAND_COMPLETE_APP_INT_PEND:/* 0x04: cmd cplt */
		++ card->timer_int_enabled;
		break;

	case BSTRM_EXCEP_COND_APP_INT_PEND:	/* 0x20 */
		process_bstrm_exception(card);
		break;

	case GLOBAL_EXCEP_COND_APP_INT_PEND:
		process_global_exception(card);

		if (IS_56K_CARD(card) || IS_TE1_CARD(card)) {
			FRONT_END_STATUS_STRUCT	FE_status; 
			
			card->hw_iface.peek(card->hw, card->fe_status_off,
					&FE_status, sizeof(FE_status));
			FE_status.opp_flag = 0x01;
			card->hw_iface.poke(card->hw, card->fe_status_off,
					&FE_status, sizeof(FE_status));
		}

		break;

	case TIMER_APP_INT_PEND:
		timer_intr(card);
		break;

	default:
		card->statistics.isr_spurious++;
		DEBUG_EVENT( "%s: spurious interrupt 0x%02X!\n", 
				card->devname, intr_type);
		card->hw_iface.peek(card->hw, card->flags_off + offsetof(SHARED_MEMORY_INFO_STRUCT, global_info_struct),
			&global_info, sizeof(global_info));
		DEBUG_EVENT( "Code name: ");
		for(i = 0; i < 4; i ++){
			printk("%c", global_info.code_name[i]); 
		}
		printk("\n");
		DEBUG_EVENT( "\nCode version: ");
	 	for(i = 0; i < 4; i ++){
			printk("%c", global_info.code_version[i]); 
		}
		printk("\n");	
		break;
	}

isr_done:

	clear_bit(0,&card->in_isr);
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}

/*============================================================================
 * Receive interrupt handler.
 */
static void rx_intr (sdla_t* card)
{
	BSTRM_DATA_RX_STATUS_EL_STRUCT rxbuf;
	struct sk_buff *skb;
	unsigned len;
	unsigned addr;
	unsigned char *buf;
	int i;

	card->hw_iface.peek(card->hw, card->u.b.rxmb_off, &rxbuf, sizeof(rxbuf));
	addr = rxbuf.ptr_data_bfr;
	if (rxbuf.opp_flag != 0x01) {
		GLOBAL_INFORMATION_STRUCT global_info;
			
		card->hw_iface.peek(card->hw, card->flags_off + offsetof(SHARED_MEMORY_INFO_STRUCT, global_info_struct),
			&global_info, sizeof(global_info));
		DEBUG_EVENT( 
			"%s: corrupted Rx buffer @ 0x%X, flag = 0x%02X!\n", 
			card->devname, (unsigned)card->u.b.rxmb_off, rxbuf.opp_flag);
                DEBUG_EVENT( "Code name: ");
                for(i = 0; i < 4; i ++)
                        DEBUG_EVENT( "%c",
                                global_info.code_name[i]);
                DEBUG_EVENT( "\nCode version: ");
                for(i = 0; i < 4; i ++)
                        DEBUG_EVENT( "%c",
                                global_info.code_version[i]);
                DEBUG_EVENT( "\n");


		/* Bug Fix: Mar 6 2000
                 * If we get a corrupted mailbox, it measn that driver 
                 * is out of sync with the firmware. There is no recovery.
                 * If we don't turn off all interrupts for this card
                 * the machine will crash. 
                 */
		DEBUG_EVENT( "%s: Critical router failure ...!!!\n", card->devname);
		DEBUG_EVENT( "Please contact Sangoma Technologies !\n");
		bstrm_set_intr_mode(card,0);	
		return;
	}

	len  = rxbuf.block_length;
	if (len != card->u.b.cfg.rx_complete_length){
		DEBUG_EVENT( "%s: Invalid rx packet: Len=%i < %i\n",
				card->devname,len,card->u.b.cfg.rx_complete_length);
	}

	/* Obtain a free skb packet from the bh rx queue.
	 * The packet has been pre allocated to the max rx
	 * size */
	skb = bh_enqueue(card);
	if (!skb){
		++card->wandev.stats.rx_dropped;
		if (net_ratelimit()){
			DEBUG_EVENT( "%s: Critical Error! Rx dropped Rx CH Free=%i Full=%i\n",
					card->devname,
					wan_skb_queue_len(&card->u.b.rx_isr_queue),
					wan_skb_queue_len(&card->u.b.rx_isr_free_queue));
		}
		goto kick_rx_bh;
	}

	buf = skb_put(skb, len);
	card->hw_iface.peek(card->hw, addr, buf, len);

//	DEBUG_EVENT( "PKT (rxbug=%lX, addr=%X, len=%d): ", (unsigned long)card->u.b.rxmb, addr,len);
//	{
//		int i, flag;
//		for (i=0;i<len;i++){
//			if (i < 10) printk("%02X ",buf[i]);
//		}
//		printk("\n");
//	}

	if (card->u.b.cfg.rbs_map){
		int channel_range = GET_TE_CHANNEL_RANGE(&card->fe);
		card->hw_iface.peek(card->hw, card->fe.te_param.ptr_te_Rx_sig_off,
				&card->u.b.rbs_sig[0], channel_range);
		//card->hw_iface.peek(card->hw, card->u_fe.te_iface.ptr_te_Rx_sig_off,
		//		&card->u.b.rbs_sig[0], channel_range);
	}

	skb_queue_tail(&card->u.b.rx_isr_queue,skb);

kick_rx_bh:

	if (!card->u.b.sw_card){
		tasklet_hi_schedule(&card->u.b.wanpipe_rx_task);
	}else{
		bstrm_rx_bh((unsigned long)card);
	}
	
	/* Release buffer element and calculate a pointer to the next one */
	rxbuf.opp_flag = 0x00;
	card->hw_iface.poke(card->hw, card->u.b.rxmb_off, &rxbuf, sizeof(rxbuf));
	card->u.b.rxmb_off += sizeof(rxbuf);
	if (card->u.b.rxmb_off > card->u.b.rxbuf_last_off){
		card->u.b.rxmb_off = card->u.b.rxbuf_base_off;
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
        netdevice_t* dev;
        bitstrm_private_area_t* bstrm_priv_area = NULL;

	/* TE timer interrupt */
	if (card->u.b.timer_int_enabled & TMR_INT_ENABLED_TE) {
		card->wandev.fe_iface.polling(&card->fe);
		bstrm_handle_front_end_state(card);
		card->u.b.timer_int_enabled &= ~TMR_INT_ENABLED_TE;
	}
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
        if (dev == NULL){
		card->u.b.timer_int_enabled = 0;
		goto timer_intr_exit;
	}
	
        bstrm_priv_area = wan_netif_priv(dev);

	/* process a udp call if pending */
#if 0
       	if(card->u.b.timer_int_enabled & TMR_INT_ENABLED_UDP) {
               	process_udp_mgmt_pkt(card, dev, bstrm_priv_area);
		card->u.b.timer_int_enabled &= ~TMR_INT_ENABLED_UDP;
        }
#endif

	/* read the communications statistics if required */
	if(card->u.b.timer_int_enabled & TMR_INT_ENABLED_UPDATE) {
		update_comms_stats(card, bstrm_priv_area);
                if(!(--bstrm_priv_area->update_comms_stats)) {
			card->u.b.timer_int_enabled &= 
				~TMR_INT_ENABLED_UPDATE;
		}
        }

timer_intr_exit:

	/* only disable the timer interrupt if there are no udp or statistic */
	/* updates pending */
        if(!card->u.b.timer_int_enabled) {
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
        }
}

/*------------------------------------------------------------------------------
  Miscellaneous Functions
	- set_bstrm_config() used to set configuration options on the board
------------------------------------------------------------------------------*/

static int set_bstrm_config(sdla_t* card)
{
	BSTRM_CONFIGURATION_STRUCT cfg;
	int rc;
	//wan_mbox_t* mb = &card->wan_mbox;
	
	memset(&cfg, 0, sizeof(BSTRM_CONFIGURATION_STRUCT));

	if(card->wandev.clocking){
		cfg.baud_rate = card->wandev.bps;
	}
	
	cfg.line_config_options = (card->wandev.electrical_interface == WANOPT_RS232) ?
		INTERFACE_LEVEL_RS232 : INTERFACE_LEVEL_V35;

	cfg.modem_config_options	= 0;
	cfg.modem_status_timer		= 500;

	cfg.API_options = 0;  /*PERMIT_APP_INT_TX_RX_BFR_ERR*/

	DEBUG_EVENT("%s: Sync Options = 0x%X\n",
			card->devname, card->u.b.cfg.sync_options);

	cfg.SYNC_options = card->u.b.cfg.sync_options;
	cfg.Rx_sync_char = card->u.b.cfg.rx_sync_char;
	cfg.monosync_Tx_time_fill_char = card->u.b.cfg.monosync_tx_time_fill_char;
	
	cfg.buffer_options=0;
	
	cfg.max_length_Tx_data_block =  card->u.b.cfg.max_length_tx_data_block;
	cfg.Rx_complete_length = card->u.b.cfg.rx_complete_length;
	cfg.Rx_complete_timer = card->u.b.cfg.rx_complete_timer;

	cfg.statistics_options = TX_DATA_BYTE_COUNT_STAT | 
				 RX_DATA_BYTE_COUNT_STAT |
				 TX_THROUGHPUT_STAT |
				 RX_THROUGHPUT_STAT;

	rc = bstrm_configure(card, &cfg);

	return rc;
}


/*============================================================================
 * Process global exception condition
 */
static int process_global_exception(sdla_t *card)
{
	wan_mbox_t* mbox = &card->wan_mbox;
	int rc;

	mbox->wan_data_len = 0;
	mbox->wan_command = READ_GLOBAL_EXCEPTION_CONDITION;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mbox);
	if(rc != WAN_CMD_TIMEOUT ){
	
		switch(mbox->wan_return_code) {
         
	      	case EXCEP_MODEM_STATUS_CHANGE:

			if (IS_TE1_CARD(card)){
				card->wandev.fe_iface.isr(&card->fe);
				bstrm_handle_front_end_state(card);
				break;
			}

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

				bstrm_handle_front_end_state(card);
				break;
			
			}

			DEBUG_EVENT( "%s: Modem status change\n",
				card->devname);

			switch(mbox->wan_data[0] & (DCD_HIGH | CTS_HIGH)) {
				case (DCD_HIGH):
					DEBUG_EVENT( "%s: DCD high, CTS low\n",card->devname);
					break;
				case (CTS_HIGH):
                                        DEBUG_EVENT( "%s: DCD low, CTS high\n",card->devname); 
					break;
                                case ((DCD_HIGH | CTS_HIGH)):
                                        DEBUG_EVENT( "%s: DCD high, CTS high\n",card->devname);
                                        break;
				default:
                                        DEBUG_EVENT( "%s: DCD low, CTS low\n",card->devname);
                                        break;
			}
			break;

                default:
                        DEBUG_EVENT( "%s: Global exception %x\n",
				card->devname, mbox->wan_return_code);
                        break;
                }
	}
	return 0;
}


/*============================================================================
 * Process chdlc exception condition
 */
static int process_bstrm_exception(sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int rc;

	mb->wan_data_len = 0;
	mb->wan_command = READ_BSTRM_EXCEPTION_CONDITION;
	rc = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(rc != WAN_CMD_TIMEOUT) {
		switch (rc) {

		case NO_BSTRM_EXCEP_COND_TO_REPORT:
			DEBUG_EVENT( "%s: No exceptions reported.\n",
						card->devname);
			break;
			
		case EXCEP_RX_DISCARD:
		case EXCEP_TX_IDLE:
		{
			RX_DISC_TX_IDLE_EXCEP_STRUCT *idle_strct = 
				(RX_DISC_TX_IDLE_EXCEP_STRUCT *)mb->wan_data;			

			if (idle_strct->Rx_discard_count){
				DEBUG_EVENT( "%s: Error Rx discard %lu\n",
						card->devname,idle_strct->Rx_discard_count);
			}
			if (idle_strct->Tx_idle_count){
				DEBUG_EVENT( "%s: Error Tx idle count %lu\n",
						card->devname,idle_strct->Tx_idle_count);
			}
		}	
			break;
		
		case EXCEP_SYNC_LOST:
			DEBUG_EVENT("%s: Bitstrm Exception: Sync Lost!\n",card->devname);
			break;
			
		case EXCEP_SYNC_ACHIEVED:
			DEBUG_EVENT("%s: Bitstrm Exception: Sync Achieved!\n",card->devname);
			break;
			
		default:
			DEBUG_EVENT( "%s: Bstrm Exception condition 0x%X\n",
						card->devname,rc);
			break;
		}
	}
	return 0;
}


/*=============================================================================
 * Process UDP management packet.
 */

static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				bitstrm_private_area_t* bstrm_priv_area,
				char local_dev) 
{
	unsigned char *buf;
	unsigned int len;
	struct sk_buff *new_skb;
	int udp_mgmt_req_valid = 1;
	wan_udp_pkt_t *wan_udp_pkt;
	wan_mbox_t *mb = &card->wan_mbox;
	struct timeval tv;
	int err;

	wan_udp_pkt = (wan_udp_pkt_t *) bstrm_priv_area->udp_pkt_data;

	if(!local_dev){
		
		if(bstrm_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK){

			/* Only these commands are support for remote debugging.
			 * All others are not */
			switch(wan_udp_pkt->wan_udp_command){

				case READ_GLOBAL_STATISTICS:
				case READ_MODEM_STATUS:  
				case READ_BSTRM_STATUS:
				case READ_COMMS_ERROR_STATS:
				case READ_BSTRM_OPERATIONAL_STATS:
				case BPIPE_ROUTER_UP_TIME:

				/* These two commands are executed for
				 * each request */
				case READ_BSTRM_CONFIGURATION:
				case READ_BSTRM_CODE_VERSION:
					udp_mgmt_req_valid = 1;
					break;
				default:
					udp_mgmt_req_valid = 0;
					break;
			} 
		}
	}else{
		udp_mgmt_req_valid=1;
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

   	} else {

		switch(wan_udp_pkt->wan_udp_command) {
	
		case BPIPE_ROUTER_UP_TIME:
			do_gettimeofday( &tv );
			bstrm_priv_area->router_up_time = tv.tv_sec - 
					bstrm_priv_area->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					bstrm_priv_area->router_up_time;	
			mb->wan_data_len = sizeof(unsigned long);
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
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			break;

		case WAN_GET_PROTOCOL:
 		   	wan_udp_pkt->wan_udp_chdlc_num_frames = card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_LINUX_PLATFORM;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	mb->wan_data_len = wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

#ifdef LINEPROBE
		case ENABLE_BSTRM_COMMUNICATIONS:
			DEBUG_DBG("%s: process_udp_mgmt_pkt(): ENABLE_BSTRM_COMMUNICATIONS\n",card->devname);
			if (IS_TE1_CARD(card)){	
				/*
				 * If application enables communications, there
				 * must be no alarms.If no alarms, manualy (arg
				 * 2 is 1) poll the alarms again to set the 
				 * Front End to the correct state. */
				card->wandev.fe_iface.read_alarm(&card->fe, 1);
				setup_api_timeslots(card);
			}
			wan_udp_pkt->wan_udp_return_code = api_enable_comms(card);
			break;

		case DISABLE_BSTRM_COMMUNICATIONS:
			DEBUG_DBG("%s: process_udp_mgmt_pkt():DISABLE_BSTRM_COMMUNICATIONS\n",card->devname);
			wan_udp_pkt->wan_udp_return_code = api_disable_comms(card);
			break;
#endif //LINEPROBE
			
		default:
			/* it's a board command */
			mb->wan_command = wan_udp_pkt->wan_udp_command;
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			if (mb->wan_data_len) {
				memcpy(&mb->wan_data, 
				       (unsigned char *)wan_udp_pkt->wan_udp_data, 
				       mb->wan_data_len);
	      		} 
			/* run the command on the board */
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err == OK) {
				/* copy the result back to our buffer */
				memcpy(&wan_udp_pkt->wan_udp_cmd, mb, sizeof(wan_cmd_t)); 
				
				if (mb->wan_data_len) {
					memcpy(&wan_udp_pkt->wan_udp_data, 
					       &mb->wan_data, 
					       mb->wan_data_len); 
				}
			}
		}

     	} /* end of else */

     	/* Fill UDP TTL */
	wan_udp_pkt->wan_ip_ttl= card->wandev.ttl; 

	if (local_dev){
		wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
		return 0;
	}

     	len = wan_reply_udp(card,bstrm_priv_area->udp_pkt_data, mb->wan_data_len);
	

     	if(bstrm_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK){
		/* Must check if we interrupted if_send() routine. The
		 * tx buffers might be used. If so drop the packet */
	   	//if (!test_bit(SEND_CRIT,&card->wandev.critical)) {
		//
		//	if(!bstrm_send(card, bstrm_priv_area->udp_pkt_data, len)) {
		//		++ card->wandev.stats.tx_packets;
		//		card->wandev.stats.tx_bytes += len;
		//	}
		//}
	} else {	
	
		/* Pass it up the stack
    		   Allocate socket buffer */
		if ((new_skb = dev_alloc_skb(len)) != NULL) {
			/* copy data into new_skb */

 	    		buf = skb_put(new_skb, len);
  	    		memcpy(buf, bstrm_priv_area->udp_pkt_data, len);

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

	atomic_set(&bstrm_priv_area->udp_pkt_len,0);
 	
	return 0;
}

#ifdef LINEPROBE

static int api_enable_comms(sdla_t * card)
{
	int rc;
	
	if (IS_TE1_CARD(card)){
		if (card->fe.fe_status == FE_CONNECTED) {
			if (card->u.b.state != WAN_CONNECTED) {
				if(card->comm_enabled == 0) {
					rc = bstrm_comm_enable (card);
					if (rc == 0){
						port_set_state(card, WAN_CONNECTED);
					}
				}else{
					rc = COMMS_ALREADY_ENABLED;
				}
			}else{
				rc = IF_IN_DISCONNECED_STATE;
			}	
		}else{
			rc = FRONT_END_IN_DISCONNECTED_STATE;
		}
	}else{
		rc = 0;
		/* FIXME: Remove this line */
		card->fe.fe_status = FE_CONNECTED;
		card->u.b.state = WAN_DISCONNECTED;
		port_set_state(card, WAN_CONNECTED);
	}

	DEBUG_DBG("api_enable_comms(): rc : 0x%X\n", rc);
	return rc;
}

static int api_disable_comms(sdla_t* card)
{
	int rc;
	
	port_set_state(card, WAN_DISCONNECTED);

	if(card->comm_enabled != 0) {
		rc = bstrm_comm_disable (card);
	}else{
		rc = COMMS_ALREADY_DISABLED;
	}

	DEBUG_DBG("api_disable_comms(): rc : 0x%X\n", rc);
	return rc;
}

static int setup_api_timeslots(sdla_t* card)
{
	int i;
	bitstrm_private_area_t* bstrm_priv_area;
	netdevice_t* dev;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev) return -EINVAL;
	bstrm_priv_area = wan_netif_priv(dev);
	
	memset(card->u.b.time_slot_map,0,sizeof(card->u.b.time_slot_map));

	bstrm_priv_area->time_slot_map = 0xFFFFFFFF;
	card->u.b.time_slots=NUM_OF_T1_CHANNELS;
	
	if (IS_E1_CARD(card)){
		bstrm_priv_area->time_slot_map &= 0x7FFFFFFF;
		card->u.b.time_slots=NUM_OF_E1_CHANNELS;
	}
	
	bstrm_priv_area->max_tx_up_size=MAX_T1_CHAN_TX_UP_SIZE;

	if (IS_E1_CARD(card)){
		//must be divisible by 31 and less than MAX_T1_CHAN_TX_UP_SIZE. 
		bstrm_priv_area->max_tx_up_size =
		       MAX_T1_CHAN_TX_UP_SIZE - (MAX_T1_CHAN_TX_UP_SIZE % (NUM_OF_E1_CHANNELS-1));//1178
	}
	
	bstrm_priv_area->time_slots = 0;

	for (i=0;i<card->u.b.time_slots;i++){
		if (test_bit(i,&bstrm_priv_area->time_slot_map)){	
			bstrm_priv_area->time_slots++;
		}
	}
	
	if (bstrm_priv_area->max_tx_up_size % bstrm_priv_area->time_slots){
		bstrm_priv_area->max_tx_up_size-=
				bstrm_priv_area->max_tx_up_size % 
				bstrm_priv_area->time_slots;
		bstrm_priv_area->max_tx_up_size+=bstrm_priv_area->time_slots;
	}
	
	DEBUG_DBG("\n");
	DEBUG_DBG("%s: Configuring %s: \n",
			card->devname,
			bstrm_priv_area->if_name);
	DEBUG_DBG("%s:      Tx Idle Flag   =0x%X\n",
			card->devname,
			bstrm_priv_area->tx_idle_flag);

	
	DEBUG_DBG("%s:      Max Tx Up Size =%i\n",
			card->devname,
			bstrm_priv_area->max_tx_up_size);
	
	DEBUG_DBG("%s:      HDLC Enc/Dec   =%s  Bits=%d\n",
			card->devname,
			bstrm_priv_area->hdlc_eng?"ON":"OFF",
			bstrm_priv_area->seven_bit_hdlc?7:8);

	bstrm_priv_area->hdlc_flag=0;
	set_bit(NO_FLAG,&bstrm_priv_area->hdlc_flag);
	DEBUG_DBG("\n");

	bstrm_priv_area->rx_crc[0]=-1;
	bstrm_priv_area->rx_crc[1]=-1;
	bstrm_priv_area->rx_crc[2]=-1;
	bstrm_priv_area->tx_crc=-1;
	bstrm_priv_area->tx_flag= 0x7E; //card->u.b.cfg.monosync_tx_time_fill_char;
	bstrm_priv_area->tx_flag_idle= 0x7E; //card->u.b.cfg.monosync_tx_time_fill_char;

	if (IS_TE1_CARD(card)){	

		DEBUG_DBG("%s: entering bind loop (card->u.b.time_slots : %d).\n",
				card->devname, card->u.b.time_slots);

		/*Bind an interface to a time slot map.*/
		for (i=0;i<card->u.b.time_slots;i++){

			if (test_bit(i,&bstrm_priv_area->time_slot_map)){	
				if (IS_E1_CARD(card)){

					DEBUG_DBG("Binding interface %s for timeslot %i\n",
						bstrm_priv_area->if_name, i+1);

					card->u.b.time_slot_map[i+1] = bstrm_priv_area;
				}else{

					DEBUG_DBG("Binding interface %s for timeslot %i\n",
						bstrm_priv_area->if_name, i);
					
					card->u.b.time_slot_map[i] = bstrm_priv_area;
				}
				
				bstrm_priv_area->time_slots++;
			}
		}
	}

	card->u.b.tx_chan_multiple = 30;
	
	DEBUG_DBG("%s: Configuring: \n",card->devname);
	if (IS_TE1_CARD(card)){
		DEBUG_DBG("%s:      ChanNum    =%i\n",
				card->devname, card->u.b.time_slots);
		DEBUG_DBG("%s:      TxChanMult =%i\n",
				card->devname, card->u.b.tx_chan_multiple);
	}
	DEBUG_DBG("%s:      TxMTU      =%i\n",
			card->devname, card->u.b.cfg.max_length_tx_data_block);
	DEBUG_DBG("%s:      RxMTU      =%i\n",
			card->devname, card->u.b.cfg.rx_complete_length);
	DEBUG_DBG("%s:      IdleFlag   =0x%x\n",
				card->devname,
				card->u.b.cfg.monosync_tx_time_fill_char);
	
	return 0;
}

#endif //LINEPROBE


/*============================================================================
 * Initialize Receive and Transmit Buffers.
 */

static void init_bstrm_tx_rx_buff( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	unsigned long tx_config_off = 0;
	unsigned long rx_config_off = 0;
	BSTRM_TX_STATUS_EL_CFG_STRUCT tx_config;
	BSTRM_RX_STATUS_EL_CFG_STRUCT rx_config;
	char err;
	
	mb->wan_data_len = 0;
	mb->wan_command = READ_BSTRM_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(err != OK) {
		netdevice_t	*dev;
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			bstrm_error(card,err,mb);
		}
		return;
	}

	tx_config_off = ((BSTRM_CONFIGURATION_STRUCT *)mb->wan_data)->
					ptr_Tx_stat_el_cfg_struct;
	card->hw_iface.peek(card->hw, tx_config_off, &tx_config, sizeof(tx_config));
       	rx_config_off = ((BSTRM_CONFIGURATION_STRUCT *)mb->wan_data)->
                           			ptr_Rx_stat_el_cfg_struct;
	card->hw_iface.peek(card->hw, rx_config_off, &rx_config, sizeof(rx_config));

      	/* Setup Head and Tails for buffers */
       	card->u.b.txbuf_base_off = tx_config.base_addr_Tx_status_elements;
       	card->u.b.txbuf_last_off = 
	card->u.b.txbuf_base_off +
		(tx_config.number_Tx_status_elements - 1) * sizeof(BSTRM_DATA_TX_STATUS_EL_STRUCT);
	
       	card->u.b.rxbuf_base_off = rx_config.base_addr_Rx_status_elements;
       	card->u.b.rxbuf_last_off = 
              		card->u.b.rxbuf_base_off +
		(rx_config.number_Rx_status_elements - 1) * sizeof(BSTRM_DATA_RX_STATUS_EL_STRUCT);

	/* Set up next pointer to be used */
       	card->u.b.txbuf_off = tx_config.next_Tx_status_element_to_use;
       	card->u.b.rxmb_off = rx_config.next_Rx_status_element_to_use;

        /* Setup Actual Buffer Start and end addresses */
        card->u.b.rx_base_off = rx_config.base_addr_Rx_status_elements;
}

/*=============================================================================
 * Perform Interrupt Test by running READ_BSTRM_CODE_VERSION command MAX_INTR
 * _TEST_COUNTER times.
 */
static int intr_test( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err,i;

	card->timer_int_enabled = 0;
	
	err = bstrm_set_intr_mode(card, APP_INT_ON_COMMAND_COMPLETE);

	if (err == CMD_OK) { 
		for (i = 0; i < MAX_INTR_TEST_COUNTER; i ++) {	
			mb->wan_data_len  = 0;
			mb->wan_command = READ_BSTRM_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != CMD_OK) 
				bstrm_error(card, err, mb);
		}
	}
	else {
		return err;
	}

	err = bstrm_set_intr_mode(card, 0);

	if (err != CMD_OK)
		return err;

	return 0;
}

/*============================================================================
 * Set PORT state.
 */
static void port_set_state (sdla_t *card, int state)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev;
	bitstrm_private_area_t	*bstrm_priv_area;

        if (card->u.b.state != state)
        {
                switch (state)
                {
                case WAN_CONNECTED:
                        printk (KERN_INFO "%s: Link connected!\n",
                                card->devname);
                      	break;

                case WAN_CONNECTING:
                        printk (KERN_INFO "%s: Link connecting...\n",
                                card->devname);
                        break;

                case WAN_DISCONNECTED:
                        printk (KERN_INFO "%s: Link disconnected!\n",
                                card->devname);
                        break;
                }

                card->wandev.state = card->u.b.state = state;
		
		WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev || !wan_netif_priv(dev)) continue;
			bstrm_priv_area = wan_netif_priv(dev);
	
			if (IS_TE1_CARD(card) && 
			    bstrm_priv_area->common.usedby == WP_SWITCH &&
			    state == WAN_CONNECTED){

				bitstrm_private_area_t *sw_chan;
				sdla_t *sw_card;
			
				if (!bstrm_priv_area->sw_dev){
					DEBUG_EVENT( "%s: Switch not present: Skip connect \n",
							bstrm_priv_area->common.dev->name);
					continue;
				}
				
				sw_chan=(bitstrm_private_area_t *)wan_netif_priv(bstrm_priv_area->sw_dev);
				sw_card=sw_chan->card;
				
				if (!sw_card->comm_enabled){
					DEBUG_EVENT( "%s: Switch not connected: %s \n",
							bstrm_priv_area->common.dev->name,
							bstrm_priv_area->sw_dev->name);
					continue;	
				}else{

					DEBUG_EVENT( "%s: Setting switch to connected %s\n",
							bstrm_priv_area->common.dev->name,
							bstrm_priv_area->sw_dev->name);
		
					set_bit(WAIT_DEVICE_SLOW,&bstrm_priv_area->tq_control);
					set_bit(WAIT_DEVICE_FAST,&sw_chan->tq_control);

					sw_chan->common.state = WAN_CONNECTED;		
				}
			}

			bstrm_priv_area->common.state = state;

			if (bstrm_priv_area->common.usedby == STACK){
				if (state == WAN_CONNECTED){
					wanpipe_lip_connect(bstrm_priv_area,0);
				}else{
					wanpipe_lip_disconnect(bstrm_priv_area,0);
				}
				
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
			} else if (bstrm_priv_area->common.usedby == ANNEXG && 
			    	   bstrm_priv_area->annexg_dev){
				if (state == WAN_CONNECTED){ 
					if (IS_FUNC_CALL(lapb_protocol,lapb_link_up)){
						lapb_protocol.lapb_link_up(bstrm_priv_area->annexg_dev);
					}
				} else {
					if (IS_FUNC_CALL(lapb_protocol,lapb_link_down)){
						lapb_protocol.lapb_link_down(bstrm_priv_area->annexg_dev);
					}
				}
			
#endif
			}else if (bstrm_priv_area->common.usedby == API){
				wan_wakeup_api(bstrm_priv_area);
				wan_update_api_state(bstrm_priv_area);

			}else if (bstrm_priv_area->common.usedby == WANPIPE){
				wake_net_dev(dev);	
			}

			/* Reset the T1 channel tx idle flag, on T1 restart,
			 * when HDLC engine is running */
			if (bstrm_priv_area->hdlc_eng && state == WAN_DISCONNECTED){
				bstrm_priv_area->tx_idle_flag=0x7E;
			}

			if (bstrm_priv_area->protocol == WANCONFIG_PPP && state == WAN_DISCONNECTED){
				send_ppp_term_request(dev);	
			}

			if (IS_TE1_CARD(card) && 
			    state == WAN_DISCONNECTED){
				struct sk_buff *skb;
				bstrm_skb_queue_purge(&bstrm_priv_area->tx_queue);	
				while((skb = skb_dequeue(&bstrm_priv_area->rx_used_queue))!=NULL){
					skb->data=(skb->head+16);
					skb->len=1;
					skb_trim(skb,0);
					skb_queue_tail(&bstrm_priv_area->rx_free_queue,skb);
				}

				if (bstrm_priv_area->common.usedby==WP_SWITCH){
					set_bit(WAIT_DEVICE_BUFFERS,&bstrm_priv_area->tq_control);
				}
			}
		}
        }
}

/*===========================================================================
 * config_bstrm
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


static int config_bstrm (sdla_t *card)
{
	int err;
	if (card->comm_enabled){
		return 0;
	}

	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);

	/* Setup the Board for BSTRM */
	if ((err=set_bstrm_config(card))) {
		printk (KERN_INFO "%s: Failed BSTRM configuration! rc=0x%X\n",
			card->devname,err);
		return -EINVAL;
	}

	if (IS_TE1_CARD(card)) {

		if (bstrm_set_FE_config(card)){
			DEBUG_EVENT( "%s: Failed Rx discard/Tx idle configuration!\n", 
					card->devname);
			return -EINVAL;
		}


		if (IS_T1_CARD(card) && card->u.b.cfg.rbs_map){

			err=bstrm_set_te_signaling_config(card,card->u.b.cfg.rbs_map);
			if (err){
				return -EINVAL;
			}
			
			err=bstrm_read_te_signaling_config(card);
			if (err){
				return -EINVAL;
			}
		}

		
		DEBUG_EVENT( "%s: Configuring onboard %s CSU/DSU\n",
			card->devname, 
			(IS_T1_CARD(card))?"T1":"E1");

		err = -EINVAL;
		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			DEBUG_EVENT("%s: Failed %s configuration!\n",
						card->devname,
						(IS_T1_CARD(card))?"T1":"E1");
			return -EINVAL;
		}
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}

	}

	
	if (IS_56K_CARD(card)) {
		printk(KERN_INFO "%s: Configuring 56K onboard CSU/DSU\n",
			card->devname);

		err = -EINVAL;
		if (card->wandev.fe_iface.config){
			err = card->wandev.fe_iface.config(&card->fe);
		}
		if (err){
			DEBUG_EVENT("%s: Failed 56K configuration!\n",
				card->devname);
			return -EINVAL;
		}
		if (card->wandev.fe_iface.post_init){
			err=card->wandev.fe_iface.post_init(&card->fe);
		}
	}

	if (card->u.b.serial){
		/* Set interrupt mode and mask */
		if (bstrm_set_intr_mode(card, APP_INT_ON_RX_BLOCK | 
					APP_INT_ON_GLOBAL_EXCEP_COND |
					APP_INT_ON_BSTRM_EXCEP_COND | APP_INT_ON_TIMER)){
			printk (KERN_INFO "%s: Failed to set interrupt triggers!\n",
					card->devname);
			return -EINVAL;	
		}
	}else{
		/* Set interrupt mode and mask */
		if (bstrm_set_intr_mode(card, APP_INT_ON_RX_BLOCK | 
					APP_INT_ON_GLOBAL_EXCEP_COND |
					APP_INT_ON_TX_BLOCK |
					APP_INT_ON_BSTRM_EXCEP_COND | APP_INT_ON_TIMER)){
			printk (KERN_INFO "%s: Failed to set interrupt triggers!\n",
					card->devname);
			return -EINVAL;	
		}
	}
	

	/* Initialize Rx/Tx buffer control fields */
	init_bstrm_tx_rx_buff(card);

	if(IS_TE1_CARD(card) && card->wandev.ignore_front_end_status == WANOPT_NO) {
		port_set_state(card, WAN_DISCONNECTED);

	}else if (IS_56K_CARD(card) && card->wandev.ignore_front_end_status == WANOPT_NO) {
		port_set_state(card, WAN_DISCONNECTED);
		
	}else{
		if (bstrm_comm_enable(card) != 0) {
			DEBUG_EVENT("%s: Failed to enable bitstrm communications!\n",
					card->devname);
			card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
			card->comm_enabled=0;
			bstrm_set_intr_mode(card,0);
			return -EINVAL;
		}
		port_set_state(card, WAN_CONNECTED);

		DEBUG_EVENT("%s: Communications enabled on startup\n",
						card->devname);
	}

	/* Manually poll the 56K CSU/DSU to get the status */
	if (IS_56K_CARD(card)) {
		/* 56K Update CSU/DSU alarms */
		card->wandev.fe_iface.read_alarm(&card->fe, 1); 
	}

	/* For TE1 cards, do not mask out the TX interrupt.
	 * The Tx interrupt must send idle data for
	 * each DS0 channel */
	if (!card->u.b.serial){
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, APP_INT_ON_TIMER);
	}else{
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, 
				(APP_INT_ON_TIMER | APP_INT_ON_TX_BLOCK));
	}
	
	DEBUG_EVENT("%s: Config complete\n",card->devname);
	return 0; 
}


static int bstrm_get_config_info(void* priv, struct seq_file* m, int* stop_cnt) 
{
	wan_device_t*	wandev = (wan_device_t*)priv;
	sdla_t*		card = NULL;

	if (wandev == NULL || wandev->priv == NULL)
		return 0;

	card = (sdla_t*)wandev->priv;
	if (!card->comm_enabled){
		DEBUG_EVENT( "DEBUG: Enable communication for Bit Streaming protocol\n");
		bstrm_comm_enable (card);
	}else{
		DEBUG_EVENT( "DEBUG: Communication is already enabled\n");
	}
	return m->count;
}


// TE1
static int set_adapter_config (sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	ADAPTER_CONFIGURATION_STRUCT* cfg = (ADAPTER_CONFIGURATION_STRUCT*)mb->wan_data;
	int err;

	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &cfg->adapter_type); 

	if (IS_TE1_CARD(card) && card->u.b.serial){
		DEBUG_EVENT("%s: Configuring Front End in Unframed Mode!\n",
				card->devname);
		cfg->adapter_type|=OPERATE_T1E1_AS_SERIAL;
	}
	
	cfg->adapter_config = 0x00; 
	cfg->operating_frequency = 00; 
	mb->wan_data_len = sizeof(ADAPTER_CONFIGURATION_STRUCT);
	mb->wan_command = SET_ADAPTER_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != OK) {
		bstrm_error(card,err,mb);
	}
	return (err);
}

static void bstrm_handle_front_end_state(void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;
	
	if (card->wandev.ignore_front_end_status == WANOPT_YES) {
		return;
	}
	if (card->fe.fe_status == FE_CONNECTED) {
		if (card->u.b.state != WAN_CONNECTED) {
			if(card->comm_enabled == 0) {
				if (bstrm_comm_enable (card) == 0){
					DEBUG_EVENT( "%s: Communications enabled\n",
							card->devname);
			
					port_set_state(card, WAN_CONNECTED);
				}else{
					DEBUG_EVENT( "%s: Failed to enable communications\n",
							card->devname);
				}
			}
		} 
	}else{
		port_set_state(card, WAN_DISCONNECTED);

		if(card->comm_enabled != 0) {
			DEBUG_EVENT( "%s: Communications disabled\n",
					card->devname);
			bstrm_comm_disable (card);
		}
	}

}


static int bstrm_comm_disable (sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_command = DISABLE_BSTRM_COMMUNICATIONS;
	mb->wan_data_len = 0;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	card->comm_enabled = 0;
	
	return 0;

}

#define WAN_BSTRM_CHUNK	8
static int bstrm_set_FE_config (sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;
	FE_RX_DISC_TX_IDLE_STRUCT FE_str;
	int i;

	if (IS_FR_FEUNFRAMED(&card->fe)){
		return 0;
	}	
	
	if (IS_T1_CARD(card)){
		FE_str.lgth_Rx_disc_bfr = card->u.b.time_slots * WAN_BSTRM_CHUNK;
		FE_str.lgth_Tx_idle_bfr = card->u.b.time_slots * WAN_BSTRM_CHUNK;
	}else{
		FE_str.lgth_Rx_disc_bfr = card->u.b.time_slots * WAN_BSTRM_CHUNK;
		FE_str.lgth_Tx_idle_bfr = (card->u.b.time_slots-1) * WAN_BSTRM_CHUNK;
	}
	
	FE_str.Tx_idle_data_bfr[0]=card->u.b.cfg.monosync_tx_time_fill_char;
	for(i = 1; i < NO_ACTIVE_TX_TIME_SLOTS_E1; i ++) {
		FE_str.Tx_idle_data_bfr[i] = card->u.b.cfg.monosync_tx_time_fill_char; 
	}
	mb->wan_data_len = sizeof(FE_RX_DISC_TX_IDLE_STRUCT);
	memcpy(mb->wan_data, &FE_str.lgth_Rx_disc_bfr, mb->wan_data_len);
	mb->wan_command = SET_FE_RX_DISC_TX_IDLE_CFG;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(err != OK){
		bstrm_error(card,err,mb);
	}

	return err;
}


static int bstrm_bind_dev_switch (sdla_t *card, bitstrm_private_area_t*chan, char *sw_dev_name)
{
	bitstrm_private_area_t *sw_chan;
	netdevice_t *sw_dev;
	sdla_t *sw_card;
	unsigned long smp_flags;
	
	if (chan->sw_dev){
		return 0;
	}
	
	sw_dev = wan_dev_get_by_name(sw_dev_name);
	if (!sw_dev){
		DEBUG_EVENT( "%s: Device %s waiting for switch device %s\n",
				card->devname, chan->if_name,sw_dev_name);
		DEBUG_EVENT( "%s: Device %s not found\n",
				card->devname, sw_dev_name);
		return 0;
	}

	if (!wan_netif_priv(sw_dev)){
		DEBUG_EVENT( "%s: Error: Invalid switch device %s: not wanpipe device!\n",
				card->devname, sw_dev->name);
		dev_put(sw_dev);
		return -EINVAL;
	}

	if (((wanpipe_common_t*)wan_netif_priv(sw_dev))->usedby != WP_SWITCH){
		DEBUG_EVENT( "%s: Error: Device %s not configured for switching\n",
				card->devname, sw_dev->name);
		
		dev_put(sw_dev);
		return -EINVAL;
	}

	if (((wanpipe_common_t*)wan_netif_priv(sw_dev))->config_id != card->wandev.config_id){
		DEBUG_EVENT( "%s: Error: Invalid switch device (%s) config id 0x%x\n",
				card->devname, sw_dev->name,((wanpipe_common_t*)wan_netif_priv(sw_dev))->config_id);
		
		dev_put(sw_dev);
		return -EINVAL;
	}
	
	sw_chan = (bitstrm_private_area_t *)wan_netif_priv(sw_dev);
	sw_card = sw_chan->card;
	
	if ((strcmp(sw_chan->sw_if_name,chan->if_name)) != 0){
		DEBUG_EVENT( "%s: Error: Config mismatch: SW dev %s is not configured for switching!\n",
				card->devname, sw_chan->if_name);
		dev_put(sw_dev);
		return -EINVAL;
	}

	DEBUG_EVENT( "%s:%s Device (ch=0x%lx) mapped to %s (ch=0x%lx)\n",
			card->devname, chan->if_name, chan->time_slot_map,
			sw_chan->if_name, sw_chan->time_slot_map);

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);


	if (!card->u.b.sw_card){
		sw_card->u.b.sw_card = card;
		card->u.b.sw_card=sw_card;
	}

	sw_chan->sw_dev = chan->common.dev;
	chan->sw_dev = sw_dev;

	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	dev_hold(chan->common.dev);
	
	return 0;
}

static int bstrm_unbind_dev_switch (sdla_t *card, bitstrm_private_area_t *chan)
{
	if (chan->sw_dev){
		bitstrm_private_area_t *sw_chan=wan_netif_priv(chan->sw_dev);
		if (sw_chan){
			if (sw_chan->sw_dev){
				DEBUG_EVENT( "%s: Unbinding SW dev %s from %s\n",
				card->devname, sw_chan->if_name, sw_chan->sw_dev->name);
				dev_put(sw_chan->sw_dev);	
				sw_chan->sw_dev=NULL;
			}
		}

		DEBUG_EVENT( "%s: Unbinding SW dev %s from %s\n",
				card->devname, chan->if_name, chan->sw_dev->name);
		dev_put(chan->sw_dev);
		chan->sw_dev=NULL;
	}

	if (card->open_cnt == 1){
		if (card->u.b.sw_card){
			sdla_t *sw_card=card->u.b.sw_card;
			sw_card->u.b.sw_card=NULL;
			card->u.b.sw_card=NULL;
		}
	}
	return 0;
}


static int protocol_init (sdla_t *card, netdevice_t *dev,
		          bitstrm_private_area_t *chan,
			  wanif_conf_t* conf)
{
	struct ppp_device *pppdev;
	struct sppp *sp;
	
	if (chan->protocol != WANCONFIG_PPP && 
	    chan->protocol != WANCONFIG_CHDLC){
		DEBUG_EVENT( "%s:%s: Unsupported protocol %i\n",
				card->devname,chan->if_name,chan->protocol);
		return -EPROTONOSUPPORT;
	}

	pppdev=kmalloc(sizeof(struct ppp_device),GFP_KERNEL);
	if (!pppdev){
		chan->protocol=0;
		return -ENOMEM;
	}

	memset(pppdev,0,sizeof(struct ppp_device));

	chan->common.prot_ptr=(void*)pppdev;

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
	
	dev->type	= ARPHRD_PPP;

	if (chan->protocol == WANCONFIG_CHDLC){
		DEBUG_EVENT( "%s: Starting Kernel CISCO HDLC protocol\n",
				card->devname);
		sp->pp_flags |= PP_CISCO;
	}else{
		DEBUG_EVENT( "%s: Starting Kernel Sync PPP protocol\n",
				card->devname);
		sp->pp_flags &= ~PP_CISCO;
	}

	return 0;
}

static int protocol_start (sdla_t *card, netdevice_t *dev)
{
	int err=0;
	
	bitstrm_private_area_t *chan=wan_netif_priv(dev);

	if (!chan)
		return 0;

	if (chan->protocol == WANCONFIG_PPP ||
	    chan->protocol == WANCONFIG_CHDLC){
		if ((err=wp_sppp_open(dev)) != 0){
			return err;
		}
	}
	return err;
}

static int protocol_stop (sdla_t *card, netdevice_t *dev)
{
	bitstrm_private_area_t *chan=wan_netif_priv(dev);

	if (!chan)
		return 0;

	if (chan->protocol == WANCONFIG_PPP ||
	    chan->protocol == WANCONFIG_CHDLC){
		wp_sppp_close(dev);
	}

	return 0;
}



static int protocol_shutdown (sdla_t *card, netdevice_t *dev)
{
	bitstrm_private_area_t *chan=wan_netif_priv(dev);

	if (!chan)
		return 0;
	
	if (chan->protocol == WANCONFIG_PPP || 
	    chan->protocol == WANCONFIG_CHDLC){
		wp_sppp_detach(dev);
		
		WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,NULL);

		if (chan->common.prot_ptr){
			kfree(chan->common.prot_ptr);
			chan->common.prot_ptr= NULL;
		}
	}
	return 0;
}

static void send_ppp_term_request (netdevice_t *dev)
{
	struct sk_buff *new_skb;
	unsigned char *buf;
	bitstrm_private_area_t *chan=wan_netif_priv(dev);

	if (chan->ignore_modem)
		return;

	DEBUG_EVENT( "%s: (Debug) Wanpipe sending PPP TERM\n",dev->name);
	if ((new_skb = dev_alloc_skb(8)) != NULL) {
		/* copy data into new_skb */

		buf = skb_put(new_skb, 8);
		sprintf(buf,"%c%c%c%c%c%c%c%c", 0xFF,0x03,0xC0,0x21,0x05,0x98,0x00,0x07);

		/* Decapsulate pkt and pass it up the protocol stack */
		new_skb->protocol = htons(ETH_P_WAN_PPP);
		new_skb->dev = dev;
		wan_skb_reset_mac_header(new_skb);

		wp_sppp_input(dev,new_skb);
	}
}


static void 
wanpipe_switch_datascope_tx_up(bitstrm_private_area_t *chan,struct sk_buff *skb)
{

	struct sk_buff *new_skb;
	sdla_t*card=chan->card;
	api_rx_hdr_t *buf;

	if (chan->common.sk == NULL){
		return;
	}

	if (skb_headroom(skb) < sizeof(api_rx_hdr_t)){
		new_skb=skb_copy_expand(skb,sizeof(api_rx_hdr_t),0,GFP_ATOMIC);
	}else{
		new_skb=skb_copy(skb,GFP_ATOMIC);
	}

	if (!new_skb){
		DEBUG_EVENT( "%s: ERROR: Failed to clone skb buffer, no memory !\n",
				card->devname);
		return;
	}

	buf = (api_rx_hdr_t*)skb_push(new_skb,sizeof(api_rx_hdr_t));
	memset(buf, 0, sizeof(api_rx_hdr_t));
	buf->wan_hdr_bitstrm_direction = 1;
		
	new_skb->protocol = htons(PVC_PROT);
	wan_skb_reset_mac_header(new_skb);
	new_skb->dev      = chan->common.dev;
	new_skb->pkt_type = WAN_PACKET_DATA;

	if (wan_api_rx(chan,new_skb) != 0){
		/* Sock full cannot send, queue us for another
		 * try */
		dev_kfree_skb_any(new_skb);
		wan_wakeup_api(chan);
	}

	return;

}

static int bstrm_set_te_signaling_config (void* card_id, unsigned long ts_sig_map)
{
	sdla_t*	card = (sdla_t*)card_id;
	wan_mbox_t*	mb = &card->wan_mbox;
	char*		data = mb->wan_data;
	int		channel_range = (IS_T1_CARD(card)) ? 
				NUM_OF_T1_CHANNELS : NUM_OF_E1_CHANNELS;
	int		err, i;

	DEBUG_TEST("%s: Set signalling config...\n", 
			card->devname);

	memset((char *)&((te_signaling_cfg_t*)data)->sig_perm.time_slot[0], 
		TE_SIG_DISABLED, 
		sizeof(te_signaling_perm_t));

	for(i = 0; i < channel_range; i ++) {
		if (test_bit(i, (void*)&ts_sig_map)){
			((te_signaling_cfg_t*)data)->sig_perm.time_slot[i] 
				= (TE_RX_SIG_ENABLED | TE_TX_SIG_ENABLED);
				//= TE_RX_SIG_ENABLED;
		}
	}

	((te_signaling_cfg_t *)data)->sig_processing_counter = 1;

	mb->wan_data_len = sizeof(te_signaling_cfg_t);
	mb->wan_command = SET_TE1_SIGNALING_CFG;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != OK){
		DEBUG_EVENT("%s: Error: Failed Robbit SetCfg (map=0x%lX) (rc=0x%X)\n",
				card->devname,ts_sig_map,err);
		bstrm_error(card,err,mb);
	}
	return (err);
}

static int bstrm_disable_te_signaling (void* card_id, unsigned long ts_map)
{
	sdla_t*		card = (sdla_t*)card_id;
	unsigned char	ts_sig_perm[32];
	int		channel_range = GET_TE_CHANNEL_RANGE(&card->fe);
	int		i;

	memset(ts_sig_perm, 0, sizeof(ts_sig_perm));
	
	DEBUG_EVENT("%s: Disableing Robbit Signalling\n", card->devname);
	
	for(i = 0; i < channel_range; i++){
		if (test_bit(i, &ts_map)){
			ts_sig_perm[i] = TE_SIG_DISABLED;
		}
	}
	if (card->fe.te_param.ptr_te_sig_perm_off){
		card->hw_iface.poke(card->hw, card->fe.te_param.ptr_te_sig_perm_off,
				ts_sig_perm, channel_range);
	}
	// REMOVE it later!!
	//if (card->u_fe.te_iface.ptr_te_sig_perm_off){
	//	card->hw_iface.poke(card->hw, card->u_fe.te_iface.ptr_te_sig_perm_off,
	//			ts_sig_perm, channel_range);
	//}

	return 0;
}


static int bstrm_read_te_signaling_config (void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;
	wan_mbox_t* mb = &card->wan_mbox;
	char* data = mb->wan_data;
	int err;

	DEBUG_CFG("%s: Read TE Signalling config...\n", card->devname);
	((te_signaling_cfg_t *)data)->sig_processing_counter = 1;

	mb->wan_data_len = 0;
	mb->wan_command = READ_TE1_SIGNALING_CFG;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != OK) {
		DEBUG_EVENT("%s: Failed to Read Robbit Signalling! (rc=0x%X)\n",
				card->devname,err);
		bstrm_error(card,err,mb);
	}else{
		//card->u_fe.te_iface.ptr_te_sig_perm_off = 
		card->fe.te_param.ptr_te_sig_perm_off = 
               		((te_signaling_cfg_t *)data)->ptr_te_sig_perm_struct;
		//card->u_fe.te_iface.ptr_te_Rx_sig_off =
		card->fe.te_param.ptr_te_Rx_sig_off =
        	       	((te_signaling_cfg_t *)data)->ptr_te_Rx_sig_struct;
		//card->u_fe.te_iface.ptr_te_Tx_sig_off =
		card->fe.te_param.ptr_te_Tx_sig_off =
     	          	((te_signaling_cfg_t *)data)->ptr_te_Tx_sig_struct;
	}	
	
	return (err);
}

/*===============================================================
 * send_rbs_oob_msg
 *
 *    Construct an OOB Robbit Signalling Message and pass it 
 *    up the connected sock. If the sock is not bounded discard 
 *    the NEM.
 *
 *===============================================================*/

static int send_rbs_oob_msg (sdla_t *card, bitstrm_private_area_t *chan)
{
	unsigned char *buf;
	api_rx_hdr_t *api_rx_el;
	struct sk_buff *skb;
	int err=0, len=5;

	if (chan->common.usedby != API){
		return -ENODEV;
	}
		
	if (!chan->common.sk){
		return -ENODEV;
	}

	skb=wan_skb_alloc(sizeof(api_rx_hdr_t)+len);
	if (!skb){
		return -ENOMEM;
	}

	api_rx_el=(api_rx_hdr_t *)skb_put(skb,sizeof(api_rx_hdr_t));
	memset(api_rx_el,0,sizeof(api_rx_hdr_t));

	api_rx_el->wan_hdr_bitstrm_channel=chan->rbs_chan;
	
	buf = skb_put(skb,1);
	if (!buf){
		wan_skb_free(skb);
		return -ENOMEM;
	}

	buf[0]=chan->rbs_sig;

	skb->pkt_type = WAN_PACKET_ERR;
	skb->protocol=htons(PVC_PROT);
	skb->dev=chan->common.dev;

	DEBUG_TEST("%s: Sending OOB message len=%i\n",
			chan->if_name,skb->len);

	if (wan_api_rx(chan,skb)!=0){
		err=-ENODEV;
		wan_skb_free(skb);
	}

	return err;
}

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev)
{
	unsigned long smp_flags=0;
	bitstrm_private_area_t* chan = wan_netif_priv(dev);
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
	netdevice_t *dev;
	unsigned long smp_flags=0;
	sdla_t *card = wandev->priv;

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		bitstrm_private_area_t* chan;
		
		dev = WAN_DEVLE2DEV(devle);
		if (dev == NULL || (chan = wan_netif_priv(dev)) == NULL)
			continue;

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
	bitstrm_private_area_t* 	chan = wan_netif_priv(dev);
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
	bitstrm_private_area_t*	chan = wan_netif_priv(dev);

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


