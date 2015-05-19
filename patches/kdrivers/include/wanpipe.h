/*****************************************************************************
* wanpipe.h	WANPIPE(tm) Multiprotocol WAN Link Driver.
*		User-level API definitions.
*
* Author: 	Nenad Corbic <ncorbic@sangoma.com>
* 		Alex Feldman <al.feldman@sangoma.com>
*		Gideon Hack  	
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Nov 27,  2007 David Rokhvarg	Implemented functions/definitions for
*                               Sangoma MS Windows Driver and API.
*
* Nov 3,  2000  Nenad Corbic    Added config_id to sdla_t structure.
*                               Used to determine the protocol running.
* Jul 13, 2000  Nenad Corbic	Added SyncPPP Support
* Feb 24, 2000  Nenad Corbic    Added support for x25api driver
* Oct 04, 1999  Nenad Corbic    New CHDLC and FRAME RELAY code, SMP support
* Jun 02, 1999  Gideon Hack	Added 'update_call_count' for Cisco HDLC 
*				support
* Jun 26, 1998	David Fong	Added 'ip_mode' in sdla_t.u.p for dynamic IP
*				routing mode configuration
* Jun 12, 1998	David Fong	Added Cisco HDLC union member in sdla_t
* Dec 08, 1997	Jaspreet Singh  Added 'authenticator' in union of 'sdla_t' 
* Nov 26, 1997	Jaspreet Singh	Added 'load_sharing' structure.  Also added 
*				'devs_struct','dev_to_devtint_next' to 'sdla_t'	
* Nov 24, 1997	Jaspreet Singh	Added 'irq_dis_if_send_count', 
*				'irq_dis_poll_count' to 'sdla_t'.
* Nov 06, 1997	Jaspreet Singh	Added a define called 'INTR_TEST_MODE'
* Oct 20, 1997	Jaspreet Singh	Added 'buff_intr_mode_unbusy' and 
*				'dlci_intr_mode_unbusy' to 'sdla_t'
* Oct 18, 1997	Jaspreet Singh	Added structure to maintain global driver
*				statistics.
* Jan 15, 1997	Gene Kozin	Version 3.1.0
*				 o added UDP management stuff
* Jan 02, 1997	Gene Kozin	Version 3.0.0
*****************************************************************************/
#ifndef	_WANPIPE_H
#define	_WANPIPE_H

#include "wanpipe_api.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_events.h"
#include "wanpipe_cfg.h"
#include "wanpipe_wanrouter.h"


#if defined(__WINDOWS__) && defined (__KERNEL__)
# include "wanpipe_structs.h"
#endif

/* Due to changes between 2.4.9 and 2.4.13,
 * I decided to write my own min() and max()
 * functions */

#define wp_min(x,y) \
	({ unsigned int __x = (x); unsigned int __y = (y); __x < __y ? __x: __y; })
#define wp_max(x,y) \
	({ unsigned int __x = (x); unsigned int __y = (y); __x > __y ? __x: __y; })


#if defined(__LINUX__) || defined (__KERNEL__)	
# if defined(LINUX_2_4)||defined(LINUX_2_6)
#  ifndef AF_WANPIPE
#   define AF_WANPIPE 25
#   ifndef PF_WANPIPE
#    define PF_WANPIPE AF_WANPIPE
#   endif
#  endif
# else
#  ifndef AF_WANPIPE
#   define AF_WANPIPE 24
#   ifndef PF_WANPIPE
#    define PF_WANPIPE AF_WANPIPE
#   endif
#  endif
# endif
# define AF_ANNEXG_WANPIPE AF_WANPIPE
# define PF_ANNEXG_WANPIPE AF_ANNEXG_WANPIPE
#endif

/* Defines */

#define	WANPIPE_MAGIC	0x414C4453L	/* signature: 'SDLA' reversed */

/* IOCTL numbers (up to 16) */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define	WANPIPE_DUMP	_IOW(ROUTER_IOCTL, 16, wan_conf_t)
# define	WANPIPE_EXEC	_IOWR(ROUTER_IOCTL, 17, wan_conf_t)
#elif defined(__LINUX__)
# define	WANPIPE_DUMP	(ROUTER_USER+0)	/* dump adapter's memory */
# define	WANPIPE_EXEC	(ROUTER_USER+1)	/* execute firmware command */
#endif

#if 0
#define WAN_HWCALL(func, (x))				\
		if (card->hw_iface.##func){		\
			card->hw_iface.##func(x);	\
		}
#endif



#define MAX_CMD_BUFF 	10
#define MAX_X25_LCN 	255	/* Maximum number of x25 channels */
#define MAX_LCN_NUM	4095	/* Maximum lcn number */
#define MAX_FT1_RETRY 	100

#define TX_TIMEOUT 5*HZ


/* General Critical Flags */
enum {
	SEND_CRIT,
	PERI_CRIT,
	RX_CRIT,
	PRIV_CRIT
};

/* TE timer critical flags */
/* #define LINELB_TIMER_RUNNING 0x04 - define in sdla_te1_pmc.h */

/* Bit maps for dynamic interface configuration
 * DYN_OPT_ON : turns this option on/off 
 * DEV_DOWN   : device was shutdown by the driver not
 *              by user 
 */
#define DYN_OPT_ON	0x00
#define DEV_DOWN	0x01
#define WAN_DEV_READY	0x02

/*
 * Data structures for IOCTL calls.
 */

typedef struct sdla_dump	/* WANPIPE_DUMP */
{
	unsigned long	magic;	/* for verification */
	unsigned long	offset;	/* absolute adapter memory address */
	unsigned long	length;	/* block length */
	void*		ptr;	/* -> buffer */
} sdla_dump_t;

typedef struct sdla_exec	/* WANPIPE_EXEC */
{
	unsigned long	magic;	/* for verification */
	void*		cmd;	/* -> command structure */
	void*		data;	/* -> data buffer */
} sdla_exec_t;

typedef struct wan_procfs
{
	unsigned long 	magic;	/* for verification */
	int		cmd;
	unsigned long	max_len;
	unsigned long	offs;
	int		is_more;
	void*		data;
} wan_procfs_t;

/* UDP management stuff */
typedef struct wum_header
{
	unsigned char signature[8];	/* 00h: signature */
	unsigned char type;		/* 08h: request/reply */
	unsigned char command;		/* 09h: commnand */
	unsigned char reserved[6];	/* 0Ah: reserved */
} wum_header_t;

/*************************************************************************
 Data Structure for global statistics
*************************************************************************/

typedef struct {
	struct sk_buff *skb;
} bh_data_t, cmd_data_t;
 

/* This is used for interrupt testing */
#define INTR_TEST_MODE	0x02

#define	WUM_SIGNATURE_L	0x50495046
#define	WUM_SIGNATURE_H	0x444E3845

#define	WUM_KILL	0x50
#define	WUM_EXEC	0x51



#if defined(WAN_KERNEL)
/****** Kernel Interface ****************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_cfg.h"
#include "wanpipe_kernel.h"
#include "sdlasfm.h"
#include "sdladrv.h"

#if defined(__LINUX__)
# ifndef KERNEL_VERSION
#  define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
# endif

# if defined(LINUX_2_6)
#  include <linux/workqueue.h>
# elif defined(LINUX_2_4)
#  include <linux/tqueue.h>
# endif

# if defined(LINUX_2_4) || defined(LINUX_2_6)
#  include <linux/serial.h>
#  if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
#   include <linux/serialP.h>
#  endif
#  include <linux/serial_reg.h>
#  include <asm/serial.h>
# endif

# include <linux/tty.h>
# include <linux/tty_driver.h>
# include <linux/tty_flip.h>
#endif

#define MAX_E1_CHANNELS 32

#if defined(__WINDOWS__)
#define MAX_FR_CHANNELS	MAX_NUMBER_OF_PROTOCOL_INTERFACES /*100*/
#else
#define MAX_FR_CHANNELS (1007+1)
#endif

#define WAN_ENABLE	0x01
#define WAN_DISABLE	0x02

#ifndef	min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef	max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

#define	is_digit(ch) (((ch)>=(unsigned)'0'&&(ch)<=(unsigned)'9')?1:0)
#define	is_alpha(ch) ((((ch)>=(unsigned)'a'&&(ch)<=(unsigned)'z')||\
	 	  ((ch)>=(unsigned)'A'&&(ch)<=(unsigned)'Z'))?1:0)
#define	is_hex_digit(ch) ((((ch)>=(unsigned)'0'&&(ch)<=(unsigned)'9')||\
	 	  ((ch)>=(unsigned)'a'&&(ch)<=(unsigned)'f')||\
	 	  ((ch)>=(unsigned)'A'&&(ch)<=(unsigned)'F'))?1:0)


/****** Data Structures *****************************************************/

#if defined(__LINUX__)
typedef	struct
{	/****** X.25 specific data **********/
	netdevice_t *svc_to_dev_map[MAX_X25_LCN];
	netdevice_t *pvc_to_dev_map[MAX_X25_LCN];
	netdevice_t *tx_dev;
	netdevice_t *cmd_dev;
	u32 no_dev;
	unsigned long	hdlc_buf_status_off;
	atomic_t tx_interrupts_pending;

	/* FIXME: Move this out of the union */
	u16 timer_int_enabled;
	netdevice_t *poll_device;
	atomic_t command_busy;

	u32 udp_type;
	u8  udp_pkt_src;
	u32 udp_lcn;
	netdevice_t * udp_dev;
	wan_udp_pkt_t udp_pkt_data;
	atomic_t udp_pkt_len;

	u8 LAPB_hdlc;		/* Option to turn off X25 and run only LAPB */
	u8 logging;		/* Option to log call messages */
	u8 oob_on_modem;	/* Option to send modem status to the api */
	u16 num_of_ch;		/* Number of channels configured by the user */
	wan_taskq_t	x25_poll_task;
	struct timer_list x25_timer;
	/* Proc fs */
	wan_x25_conf_t 	x25_adm_conf;
	wan_x25_conf_t 	x25_conf;
	atomic_t tx_interrupt_cmd;
	struct sk_buff_head trace_queue;
	wan_ticks_t trace_timeout;
	unsigned short trace_lost_cnt;

	unsigned long  card_ready;
} sdla_x25_t;
#endif

typedef struct
{	/****** frame relay specific data ***/
	unsigned long	rxmb_base_off;	/* -> first Rx buffer */
	unsigned long	rxmb_last_off;	/* -> last Rx buffer */
	unsigned long	rx_base_off;	/* S508 receive buffer base */
	unsigned long	rx_top_off;	/* S508 receive buffer end */
	unsigned short	node_dlci[100];
	unsigned short	dlci_num;
	netdevice_t*	dlci_to_dev_map[MAX_FR_CHANNELS];
	atomic_t	tx_interrupts_pending;

	/* FIXME: Move this out of the union */
	unsigned short	timer_int_enabled;

	int		udp_type;
	char		udp_pkt_src;
	unsigned	udp_dlci;
	wan_udp_pkt_t	udp_pkt_data;
	atomic_t	udp_pkt_len;
	void*		trc_el_base;	/* first trace element */
	void*		trc_el_last;	/* last trace element */
	void*		curr_trc_el;	/* current trace element */
	unsigned short	trc_bfr_space;	/* trace buffer space */
	netdevice_t*	arp_dev;
#if defined(__LINUX__)
	spinlock_t	if_send_lock;
#endif
	unsigned char	issue_fs_on_startup;
	/* Proc fs */
	unsigned short 	t391;
	unsigned short 	t392;
	unsigned short 	n391;
	unsigned short 	n392;
	unsigned short 	n393;
	void*	 	update_dlci;
	unsigned char 	auto_dlci_cfg;
} sdla_fr_t;

typedef struct
{	/****** PPP-specific data ***********/
	char if_name[WAN_IFNAME_SZ+1];	/* interface name */
	unsigned long	txbuf_off;	/* -> current Tx buffer */
	unsigned long	txbuf_base_off;	/* -> first Tx buffer */
	unsigned long	txbuf_last_off;	/* -> last Tx buffer */
	unsigned long	txbuf_next_off;  /* Next Tx buffer to use */ 
	unsigned long	rxbuf_base_off;	/* -> first Rx buffer */
	unsigned long	rxbuf_last_off;	/* -> last Rx buffer */
	unsigned long	rx_base_off;	/* S508 receive buffer base */
	unsigned long	rx_top_off;	/* S508 receive buffer end */
	unsigned long	rxbuf_next_off;  /* Next Rx buffer to use */
	char ip_mode;		/* STATIC/HOST/PEER IP Mode */
	char authenticator;	/* Authenticator for PAP/CHAP */
	/* FIXME: Move this out of the union */
	unsigned char comm_enabled; /* Is comm enabled or not */
	unsigned char peer_route;   /* Process Peer Route */	
} sdla_ppp_t;

typedef struct 
{	/* Cisco HDLC-specific data */
	char if_name[WAN_IFNAME_SZ+1];	/* interface name */
	int	comm_port;/* Communication Port O or 1 */
	unsigned char usedby;  /* Used by WANPIPE or API */
	unsigned long	rxmb_off;	/* Receive mail box */
	/*unsigned long	flags_off;*/	/* flags */
	unsigned long	txbuf_off;	/* -> current Tx buffer */
	unsigned long	txbuf_base_off;	/* -> first Tx buffer */
	unsigned long	txbuf_last_off;	/* -> last Tx buffer */
	unsigned long	rxbuf_base_off;	/* -> first Rx buffer */
	unsigned long	rxbuf_last_off;	/* -> last Rx buffer */
	unsigned long	rx_base_off;	/* S508 receive buffer base */
	unsigned long	rx_top_off;	/* S508 receive buffer end */
	void* tx_status;	/* Tx status element */
	void* rx_status;	/* Rx status element */
	unsigned char receive_only; /* high speed receivers */
	unsigned short protocol_options;
	unsigned short kpalv_tx;	/* Tx kpalv timer */
	unsigned short kpalv_rx;	/* Rx kpalv timer */
	unsigned short kpalv_err;	/* Error tolerance */
	unsigned short slarp_timer;	/* SLARP req timer */
	unsigned state;			/* state of the link */
	unsigned char api_status;
	unsigned char update_call_count;
	unsigned short api_options;	/* for async config */
	unsigned char  async_mode;
        unsigned short tx_bits_per_char;
        unsigned short rx_bits_per_char;
        unsigned short stop_bits;
        unsigned short parity;
	unsigned short break_timer;
        unsigned short inter_char_timer;
        unsigned short rx_complete_length;
        unsigned short xon_char;
        unsigned short xoff_char;
	/* FIXME: Move this out of the union */
	unsigned char comm_enabled; /* Is comm enabled or not */
	/* FIXME: Move this out of the union */
	unsigned char backup;
	int 		TracingEnabled;		/* For enabling Tracing */
	unsigned long 	curr_trace_addr;	/* Used for Tracing */
	unsigned long 	start_trace_addr;
	unsigned long 	end_trace_addr;
	unsigned long 	base_addr_trace_buffer;
	unsigned long 	end_addr_trace_buffer;
	unsigned short 	number_trace_elements;
	unsigned  	available_buffer_space;
	unsigned long 	router_start_time;
	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;	
	/* FIXME: Move this out of the union */
	unsigned short 	timer_int_enabled;
	unsigned long 	router_up_time;
#if defined(__LINUX__)
	spinlock_t if_send_lock;
#endif
	void * prot;
} sdla_chdlc_t;

typedef struct 
{
	void* tx_status;	/* Tx status element */
	void* rx_status;	/* Rx status element */
	void* trace_status;	/* Trace status element */
	void* txbuf;		/* -> current Tx buffer */
	void* txbuf_base;	/* -> first Tx buffer */
	void* txbuf_last;	/* -> last Tx buffer */
	void* rxbuf_base;	/* -> first Rx buffer */
	void* rxbuf_last;	/* -> last Rx buffer */
	void* tracebuf;		/* -> current Trace buffer */
	void* tracebuf_base;	/* -> current Trace buffer */
	void* tracebuf_last;	/* -> current Trace buffer */
	unsigned rx_base;	/* receive buffer base */
	unsigned rx_end;	/* receive buffer end */
	unsigned trace_base;	/* trace buffer base */
	unsigned trace_end;	/* trace buffer end */
} sdla_hdlc_t;

#if defined(__LINUX__)
typedef struct 
{
	char if_name[WAN_IFNAME_SZ+1];	/* interface name */
	int	comm_port;/* Communication Port O or 1 */
	unsigned char usedby;  /* Used by WANPIPE or API */
	unsigned long rxmb_off;		/* Receive mail box */
	/* unsigned long flags_off; */	/* flags */
	unsigned long txbuf_off;	/* -> current Tx buffer */
	unsigned long txbuf_base_off;	/* -> first Tx buffer */
	unsigned long txbuf_last_off;	/* -> last Tx buffer */
	unsigned long rxbuf_base_off;	/* -> first Rx buffer */
	unsigned long rxbuf_last_off;	/* -> last Rx buffer */
	unsigned long rx_base_off;	/* S508 receive buffer base */
	unsigned long rx_top_off;	/* S508 receive buffer end */
	void* tx_status;	/* FIXME: Not used Tx status element */
	void* rx_status;	/* FIXME: Not used Rx status element */
	unsigned short protocol_options;
	unsigned state;			/* state of the link */
	unsigned char api_status;
	unsigned char update_call_count;
	unsigned short api_options;	/* for async config */
        unsigned short tx_bits_per_char;
        unsigned short rx_bits_per_char;
        unsigned short stop_bits;
        unsigned short parity;
	
	unsigned long  tq_working;
	void *time_slot_map[MAX_E1_CHANNELS];
	struct tasklet_struct wanpipe_rx_task;
	struct tasklet_struct wanpipe_tx_task;
	unsigned char time_slots;
	unsigned char tx_scratch_buf[MAX_E1_CHANNELS*50];
	unsigned short tx_scratch_buf_len;
	atomic_t tx_interrupts;
	unsigned short tx_chan_multiple;
	unsigned int wait_for_buffers;
	struct sdla *sw_card;
	struct sk_buff_head rx_isr_queue;
	struct sk_buff_head rx_isr_free_queue;
	
	/* FIXME: Move this out of the union */
	unsigned short timer_int_enabled;
	unsigned long  tx_idle_off;
	unsigned long  rx_discard_off;

	wan_bitstrm_conf_t cfg;

	unsigned char rbs_sig[32];
	unsigned char serial;

} sdla_bitstrm_t;
#endif

typedef struct 
{
	char if_name[WAN_IFNAME_SZ+1];	/* interface name */
	int	comm_port;/* Communication Port O or 1 */
	unsigned char usedby;  /* Used by WANPIPE or API */
	unsigned char state;
	/* unsigned long flags_off; */	/* flags */
	unsigned long rxmb_off;		/* Receive mail box */
	unsigned long txbuf_off;	/* -> current Tx buffer */
	unsigned long txbuf_base_off;	/* -> first Tx buffer */
	unsigned long txbuf_last_off;	/* -> last Tx buffer */
	unsigned long rxbuf_base_off;	/* -> first Rx buffer */
	unsigned long rxbuf_last_off;	/* -> last Rx buffer */
	unsigned long rx_base_off;	/* S508 receive buffer base */
	unsigned long rx_top_off;	/* S508 receive buffer end */
	void* tx_status;	/* Tx status element */
	void* rx_status;	/* Rx status element */
	
	unsigned int line_cfg_opt;
	unsigned int modem_cfg_opt;
	unsigned int modem_status_timer;
	unsigned int api_options;
	unsigned int protocol_options;
	unsigned int protocol_specification;
	unsigned int stats_history_options;
	unsigned int max_length_msu_sif;
	unsigned int max_unacked_tx_msus;
	unsigned int link_inactivity_timer;
	unsigned int t1_timer;
	unsigned int t2_timer;
	unsigned int t3_timer;
	unsigned int t4_timer_emergency;
	unsigned int t4_timer_normal;
	unsigned int t5_timer;
	unsigned int t6_timer;
	unsigned int t7_timer;
	unsigned int t8_timer;
	unsigned int n1;
	unsigned int n2;
	unsigned int tin;
	unsigned int tie;
	unsigned int suerm_error_threshold;
	unsigned int suerm_number_octets;
	unsigned int suerm_number_sus;
	unsigned int sie_interval_timer;
	unsigned int sio_interval_timer;
	unsigned int sios_interval_timer;
	unsigned int fisu_interval_timer;
} sdla_ss7_t;

typedef struct 
{
	char if_name[WAN_IFNAME_SZ+1];	/* interface name */
	int	comm_port;/* Communication Port O or 1 */
	unsigned char usedby;  /* Used by WANPIPE or API */
	unsigned char state;
 	/*FIXME: Move this out of the union */
	unsigned char comm_enabled;
} sdla_sdlc_t;

typedef struct
{
	void *adapter;
	unsigned char EncapMode;
} sdla_adsl_t;

#if defined(__LINUX__)
typedef struct 
{
	unsigned long rxmb_off;		/* Receive mail box */
	/* unsigned long flags_off; */		/* flags */
	unsigned long txbuf_off;		/* -> current Tx buffer */
	unsigned long txbuf_base_off;	/* -> first Tx buffer */
	unsigned long txbuf_last_off;	/* -> last Tx buffer */
	unsigned long rxbuf_base_off;	/* -> first Rx buffer */
	unsigned long rxbuf_last_off;	/* -> last Rx buffer */
	unsigned long rx_base_off;	/* S508 receive buffer base */
	void* tx_status;	/* Tx status element */
	void* rx_status;	/* Rx status element */
	wan_tasklet_t 		wanpipe_rx_task;
	struct sk_buff_head 	wp_rx_free_list;
	struct sk_buff_head 	wp_rx_used_list;
	struct sk_buff_head 	wp_rx_data_list;
	struct sk_buff_head 	wp_tx_prot_list;
	unsigned char		state;
	wan_timer_t		atm_timer;	
	void 			*tx_dev;
	void 			*trace_info;
	void 			*atm_device;
	wan_atm_conf_t		atm_cfg;
} sdla_atm_t;
#endif

#if defined(__LINUX__)
typedef struct 
{
	void* rxmb;		/* Receive mail box */
	void* flags;		/* flags */
	void* tx_status;	/* Tx status element */
	void* rx_status;	/* Rx status element */
	void* txbuf;		/* -> current Tx buffer */
	void* txbuf_base;	/* -> first Tx buffer */
	void* txbuf_last;	/* -> last Tx buffer */
	void* rxbuf_base;	/* -> first Rx buffer */
	void* rxbuf_last;	/* -> last Rx buffer */
	unsigned rx_base;	/* S508 receive buffer base */
	wan_tasklet_t 		wanpipe_rx_task;
	struct sk_buff_head 	wp_rx_free_list;
	struct sk_buff_head 	wp_rx_used_list;
	unsigned char		state;
} sdla_pos_t;
#endif

typedef struct 
{
	unsigned long time_slot_map;
	unsigned long logic_ch_map;
	unsigned char num_of_time_slots;
	unsigned char top_logic_ch;
	sdla_base_addr_t bar;
	void *trace_info;
	void *dev_to_ch_map[MAX_E1_CHANNELS];
	void *rx_dma_ptr;
	void *tx_dma_ptr;
	wan_xilinx_conf_t	cfg;
	unsigned long  	dma_mtu_off;
	unsigned short 	dma_mtu;
	unsigned char 	state_change_exit_isr;
	unsigned long 	active_ch_map;
	unsigned long 	fifo_addr_map;
	unsigned long 	fifo_addr_map_l2;
	wan_timer_t 	bg_timer;
	unsigned int 	bg_timer_cmd;
	wan_bitmap_t 	tdmv_sync;
	unsigned int	chip_cfg_status;	
	wan_taskq_t 	port_task;
	wan_bitmap_t 	port_task_cmd;
	unsigned long	wdt_rx_cnt;
	wan_ticks_t	wdt_tx_cnt;
	unsigned int	security_id;
	unsigned int	security_cnt;
	unsigned char	firm_ver;
	unsigned char	firm_id;
	unsigned int	chip_security_cnt;
	wan_ticks_t	rx_timeout,gtimeout;
	unsigned int	comm_enabled;
	unsigned int	lcfg_reg;
	unsigned int    tdmv_master_if_up;
	unsigned int	tdmv_mtu;
	unsigned int	tdmv_zaptel_cfg;
	netskb_t	*tdmv_api_rx;
	netskb_t	*tdmv_api_tx;
	wan_skb_queue_t	tdmv_api_tx_list;

	unsigned int 	tdmv_dchan_cfg_on_master;
	unsigned int	tdmv_chan;	
	unsigned int	tdmv_dchan;	
	unsigned int	tdmv_dchan_active_ch;
	void 			*tdmv_chan_ptr;

	unsigned char	tdmv_hw_tone;
	
	wan_bitmap_t	led_ctrl;
	unsigned int	tdm_intr_status;
	sdla_mem_handle_t	bar_virt;
	unsigned char	tdm_rx_dma_toggle[32];
	unsigned char	tdm_tx_dma_toggle[32];
	wan_dma_descr_t	*tdm_tx_dma[32];
	wan_dma_descr_t	*tdm_rx_dma[32];
	unsigned int	tdm_logic_ch_map;

	wan_ticks_t	sec_chk_cnt;
	wan_skb_queue_t	rtp_tap_list;
	unsigned int	serial_status;
	unsigned char	global_tdm_irq;
	unsigned char	global_poll_irq;
	unsigned int	tdm_api_cfg;
	unsigned int	tdm_api_dchan_cfg;
	wan_ticks_t     rbs_timeout;

} sdla_xilinx_t;

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
typedef struct
{
	unsigned char	num_of_time_slots;
	wan_taskq_t 	port_task;
	unsigned int 	port_task_cmd;
	void		*dev_to_ch_map[10];		// temporary
	unsigned int	tdm_logic_ch_map;

} wp_usb_t;
#endif
 
enum {
	AFT_CHIP_CONFIGURED,
	AFT_FRONT_END_UP,
	AFT_TDM_GLOBAL_ISR,
	AFT_TDM_RING_BUF,
	AFT_TDM_FAST_ISR,
	AFT_TDM_SW_RING_BUF,
	AFT_TDM_RING_SYNC_RESET,
	AFT_TDM_FREE_RUN_ISR,
	AFT_TDM_FE_SYNC_CNT
};

typedef struct 
{
	unsigned long	current_offset;
	unsigned long	total_len;
	unsigned long	total_num;
	unsigned long	status;
} sdla_debug_t;


/* Adapter Data Space.
 * This structure is needed because we handle multiple cards, otherwise
 * static data would do it.
 */
typedef struct sdla
{
#if defined(__WINDOWS__)
	u8	device_type; /* must be first member of sdla_t structure! */
	struct	_win_sdla_data;
#endif/* __WINDOWS__ */

	char devname[WAN_DRVNAME_SZ+1];	/* card name */
	void*	hw;			/* hardware configuration ('sdlahw_t*') */
	wan_device_t wandev;		/* WAN device data space */
	
	unsigned	open_cnt;		/* number of open interfaces */
	wan_ticks_t	state_tick;	/* link state timestamp */
	unsigned	intr_mode;		/* Type of Interrupt Mode */
	unsigned long	in_isr;		/* interrupt-in-service flag */
	char buff_int_mode_unbusy;	/* flag for carrying out dev_tint */  
	char dlci_int_mode_unbusy;	/* flag for carrying out dev_tint */
	unsigned long configured;	/* flag for previous configurations */
	
	unsigned short irq_dis_if_send_count; /* Disabling irqs in if_send*/
	unsigned short irq_dis_poll_count;   /* Disabling irqs in poll routine*/
	unsigned short force_enable_irq;
	char TracingEnabled;		/* flag for enabling trace */
	global_stats_t statistics;	/* global statistics */
	unsigned long	mbox_off;	/* -> mailbox offset */
	wan_mbox_t	wan_mbox;	/* mailbox structure */
	unsigned long	rxmb_off;	/* -> receive mailbox */
	wan_mbox_t	wan_rxmb;	/* rx mailbox structure */
	unsigned long	flags_off;	/* -> adapter status flags */
	unsigned long	fe_status_off;	/* FE status structure offset */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	unsigned char	rx_data[MAX_PACKET_SIZE];	/* Rx buffer */
	unsigned int	rx_len;    			/* Rx data len */
	unsigned char	tx_data[MAX_PACKET_SIZE];	/* Tx buffer */
	unsigned int	tx_len;    			/* Tx data len */
#endif
	WAN_IRQ_RETVAL (*isr)(struct sdla* card);	/* interrupt service routine */
	void (*poll)(struct sdla* card); /* polling routine */
	int (*exec)(struct sdla* card, void* u_cmd, void* u_data);
					/* Used by the listen() system call */		
#if defined(__LINUX__)
	/* Wanpipe Socket Interface */
	int   (*func) (netskb_t*, struct sock *);
	struct sock *sk;
#endif

	/* Shutdown function */
	void (*disable_comm) (struct sdla *card);

	/* Secondary Port Device: Piggibacking */
	struct sdla *next;
	struct sdla *list;

#if defined(__LINUX__)
	/* TTY driver variables */
	unsigned char tty_opt;
	struct tty_struct *tty;
	unsigned int tty_minor;
	unsigned int tty_open;
	unsigned char *tty_buf;
	struct sk_buff_head 	tty_rx_empty;
	struct sk_buff_head 	tty_rx_full;
	wan_taskq_t		tty_task_queue;
#endif
	
#if defined(__WINDOWS__)
	struct
#else
	union
#endif
	{
#if defined(__LINUX__)
		sdla_x25_t	x;
		sdla_bitstrm_t	b;
		sdla_atm_t	atm;
		sdla_pos_t	pos;
#endif
		sdla_fr_t	f;
		sdla_ppp_t	p;
		sdla_chdlc_t	c;
		sdla_hdlc_t	h;
		sdla_ss7_t	s;
		sdla_sdlc_t	sdlc;
		sdla_adsl_t	adsl;
		sdla_xilinx_t	aft;
		sdla_debug_t	debug;
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
		wp_usb_t	usb;
#endif
	} u;
	unsigned char irq_equalize;

	/*????????????????*/
	/*Should be in wandev */
	unsigned int 	type;				/* card type */
	unsigned int 	adptr_type;			/* adapter type */
	unsigned char 	adptr_subtype;			/* adapter subtype */
	wan_tasklet_t	debug_task;
	wan_timer_t	debug_timer;
	unsigned long	debug_running;
	unsigned char	wan_debugging_state;		/* WAN debugging state */
	int		wan_debug_last_msg;		/* Last WAN debug message */
	int 		(*wan_debugging)(struct sdla*);/* link debugging routine */
	unsigned long	(*get_crc_frames)(struct sdla*);/* get no of CRC frames */
	unsigned long	(*get_abort_frames)(struct sdla*);/* get no of Abort frames */
	unsigned long	(*get_tx_underun_frames)(struct sdla*);/* get no of TX underun frames */
	unsigned short 	timer_int_enabled;
	unsigned char 	backup;
	unsigned long 	comm_enabled;
	unsigned long  	update_comms_stats;
	
	sdla_fe_t	fe;		/* front end structures */
	u8		fe_no_intr;	/* set to 0x01 if not FE interrupt should enabled */		
	wan_bitmap_t	fe_ignore_intr;	/* Set to 0x01 if all FE interrupts should be ignored */
	
	unsigned int	rCount;

	/* Wanpipe Socket Interface */
	int   (*get_snmp_data)(struct sdla*, netdevice_t*, void*);

	unsigned long intr_perm_off;
	unsigned long intr_type_off;

	/* Hardware interface function pointers */
	sdlahw_iface_t	hw_iface;
	
	int (*bind_api_to_svc)(struct sdla*, void *sk_id);
	
	unsigned long	spurious;

	unsigned long	intcount;
	
	wan_tdmv_conf_t		tdmv_conf;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	wan_tdmv_iface_t	tdmv_iface;
	wan_tdmv_t			wan_tdmv;
#endif


	wan_hwec_conf_t		hwec_conf;
	wan_rtp_conf_t		rtp_conf;
	
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	struct sdla*	same_card;
#endif


#if defined(__FreeBSD__)
# if defined(NETGRAPH)
	int	running;	/* something is attached so we are running */
	/* ---netgraph bits --- */
	int		upperhooks;	/* number of upper hooks attached */
	int		lowerhooks;	/* number of lower hooks attached */
	node_p		node;		/* netgraph node */
	hook_p		upper;		/* upper layer */
	hook_p		lower;		/* lower layer */
	hook_p		debug_hook;
# if defined(ALTQ)
	struct ifaltq	xmitq_hipri;	/* hi-priority transmit queue */
	struct ifaltq	xmitq;		/* transmit queue */
# else
	struct ifqueue	xmitq_hipri;	/* hi-priority transmit queue */
	struct ifqueue	xmitq;		/* transmit queue */
# endif
	int		flags;		/* state */
# define	WAN_RUNNING	0x01		/* board is active */
# define	WAN_OACTIVE	0x02		/* output is active */
	int		out_dog;	/* watchdog cycles output count-down */
# if ( __FreeBSD__ >= 3 )
	struct callout_handle handle;	/* timeout(9) handle */
# endif
	u_long		lastinbytes, lastoutbytes; /* a second ago */
	u_long		inrate, outrate;	/* highest rate seen */
	u_long		inlast;		/* last input N secs ago */
	u_long		out_deficit;	/* output since last input */
	u_char		promisc;	/* promiscuous mode enabled */
	u_char		autoSrcAddr;	/* always overwrite source address */
	void (*wan_down)(struct sdla*);
	int (*wan_up)(struct sdla*);
	void (*wan_start)(struct sdla*);
# endif /* NETGRAPH */
	/* per card statistics */
	u_long		inbytes, outbytes;	/* stats */
	u_long		oerrors, ierrors;
	u_long		opackets, ipackets;
#endif /* __FreeBSD__ */


 	/* This value is used for detecting TDM Voice
	* rsync timeout, it should be long */
	wan_ticks_t   rsync_timeout;
	
	/* This value is used for detecting Fronte end interrupt
	 * timeout, it should be long */
	wan_ticks_t   front_end_irq_timeout;

	/* SDLA TDMV Dummy interface */
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	void* sdla_tdmv_dummy;
#endif
	void* wp_tdmapi_hash[MAX_E1_CHANNELS];

	unsigned char  card_no;	
#if defined(__WINDOWS__)
	PDMA_ADAPTER	DmaAdapterObject;  /* Object for allocating memory for DMA.
										* Can NOT be called from spin-locked code!!
										* (not from regular lock too)	*/
#if 0
	wan_debug_t		wan_debug_rx_interrupt_timing;
	wan_debug_t		wan_debug_tx_interrupt_timing;
#endif
#endif

	void *tdm_api_dev;
	void *tdm_api_span;
	unsigned int wp_debug_gen_fifo_err_tx;
	unsigned int wp_debug_gen_fifo_err_rx;
	unsigned char wp_debug_chan_seq;
	unsigned int wp_rx_fifo_sanity;
	unsigned int wp_tx_fifo_sanity;

	unsigned char aft_perf_stats_enable;
	aft_driver_performance_stats_t aft_perf_stats;

	unsigned int wdt_timeout;

} sdla_t;


/****** Public Functions ****************************************************/

void wanpipe_open      (sdla_t* card);			/* wpmain.c */
void wanpipe_close     (sdla_t* card);			/* wpmain.c */

int wpx_init (sdla_t* card, wandev_conf_t* conf);	/* wpx.c */
int wpf_init (sdla_t* card, wandev_conf_t* conf);	/* wpf.c */
int wpp_init (sdla_t* card, wandev_conf_t* conf);	/* wpp.c */
int wpc_init (sdla_t* card, wandev_conf_t* conf); 	/* Cisco HDLC */
int wp_asyhdlc_init (sdla_t* card, wandev_conf_t* conf); /* Async HDLC */
int wpbsc_init (sdla_t* card, wandev_conf_t* conf);	/* BSC streaming */
int wph_init(sdla_t* card, wandev_conf_t* conf);	/* HDLC support */
int wpft1_init (sdla_t* card, wandev_conf_t* conf);     /* FT1 Config support */
int wp_mprot_init(sdla_t* card, wandev_conf_t* conf);	/* Sync PPP on top of RAW CHDLC */
int wpbit_init (sdla_t* card, wandev_conf_t* conf);	/* Bit Stream driver */
int wpedu_init(sdla_t* card, wandev_conf_t* conf);	/* Educational driver */
int wpss7_init(sdla_t* card, wandev_conf_t* conf);	/* SS7 driver */
int wp_bscstrm_init(sdla_t* card, wandev_conf_t* conf);	/* BiSync Streaming Nasdaq */
int wp_hdlc_fr_init(sdla_t* card, wandev_conf_t* conf);	/* Frame Relay over HDLC RAW Streaming */
int wp_adsl_init(sdla_t* card, wandev_conf_t* conf);	/* ADSL Driver */
int wp_sdlc_init(sdla_t* card, wandev_conf_t* conf);	/* SDLC Driver */
int wp_atm_init(sdla_t* card, wandev_conf_t* conf);	/* ATM Driver */
int wp_pos_init(sdla_t* card, wandev_conf_t* conf);	/* POS Driver */	
int wp_xilinx_init(sdla_t* card, wandev_conf_t* conf);	/* Xilinx Hardware Support */
int wp_aft_te1_init(sdla_t* card, wandev_conf_t* conf);	/* Xilinx Hardware Support */
int wp_aft_a600_init(sdla_t* card, wandev_conf_t* conf); /* Xilinx A600 Hardware Support */
int wp_aft_w400_init(sdla_t* card, wandev_conf_t* conf); /* Xilinx W400 (GSM) Hardware Support */
int wp_aft_56k_init(sdla_t* card, wandev_conf_t* conf);	/* Xilinx Hardware Support */
int wp_aft_analog_init(sdla_t* card, wandev_conf_t* conf);	/* Xilinx Hardware Support */
int wp_aft_bri_init(sdla_t* card, wandev_conf_t* conf);	/* BRI Hardware Support */
int wp_aft_serial_init(sdla_t* card, wandev_conf_t* conf);	/* Serial Hardware Support */
int wp_adccp_init(sdla_t* card, wandev_conf_t* conf);	
int wp_xilinx_if_init(sdla_t* card, netdevice_t* dev);
int wp_aft_te3_init(sdla_t* card, wandev_conf_t* conf); /* AFT TE3 Hardware Support */
int wp_aft_te1_ss7_init(sdla_t* card, wandev_conf_t* conf); /* AFT TE1 SS7 Hardware Support */
int aft_global_hw_device_init(void);
int wp_ctrl_dev_create(void);
void wp_ctrl_dev_delete(void);

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
int wp_usb_init(sdla_t* card, wandev_conf_t* conf);
#endif

int wanpipe_globals_util_init(void); /* Initialize All Global Tables */

#if defined(__LINUX__)
extern int wanpipe_queue_tq (wan_taskq_t *);
extern int wanpipe_mark_bh (void);
extern int change_dev_flags (netdevice_t *, unsigned); 
extern unsigned long get_ip_address (netdevice_t *dev, int option);
extern void add_gateway(sdla_t *, netdevice_t *);


#if 0
extern void fastcall wp_tasklet_hi_schedule_per_cpu(struct tasklet_struct *t, int cpu_no);
extern void wp_tasklet_per_cpu_init (void);
#endif

//FIXME: Take it out
//extern int wan_reply_udp( unsigned char *data, unsigned int mbox_len, int trace_opt);
//extern int wan_udp_pkt_type(sdla_t* card,unsigned char *data);

extern int wan_ip_udp_setup(void* card_id, 
			    wan_rtp_conf_t *rtp_conf,
			    u32 chan,
		            unsigned char *data, unsigned int mbox_len);

extern int wanpipe_sdlc_unregister(netdevice_t *dev);
extern int wanpipe_sdlc_register(netdevice_t *dev, void *wp_sdlc_reg);
//ALEX_TODAY extern int check_conf_hw_mismatch(sdla_t *card, unsigned char media);
#endif

void adsl_vcivpi_update(sdla_t* card, wandev_conf_t* conf);

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
extern struct wanpipe_lapb_register_struct lapb_protocol;
#endif

int wan_snmp_data(sdla_t* card, netdevice_t* dev, int cmd, struct ifreq* ifr);

int wan_capture_trace_packet(sdla_t *card, wan_trace_t* trace_info, netskb_t *skb, char direction);
int wan_capture_trace_packet_buffer(sdla_t *card, wan_trace_t* trace_info, char *data, int len, char direction, int channel);
int wan_capture_trace_packet_offset(sdla_t *card, wan_trace_t* trace_info, netskb_t *skb, int off,char direction);
void debug_print_udp_pkt(unsigned char *data,int len,char trc_enabled, char direction);

#if defined(__LINUX__)
#if 0
int wan_verify_iovec(struct msghdr *m, struct iovec *iov, char *address, int mode);
int wan_memcpy_fromiovec(unsigned char *kdata, struct iovec *iov, int len);
int wan_memcpy_toiovec(struct iovec *iov, unsigned char *kdata, int len);
#endif
#endif

/* LIP ATM prototypes */
int init_atm_idle_buffer(unsigned char *buff, int buff_len, char *if_name, char hardware_flip);
int atm_add_data_to_skb(void* skb, void *data, int data_len, char *if_name);
int atm_pad_idle_cells_in_tx_skb(void *skb, void *tx_idle_skb, char *if_name);
void *atm_tx_skb_dequeue(void* wp_tx_pending_list, void *tx_idle_skb, char *if_name);


#if defined(__FreeBSD__) && defined(NETGRAPH)
int wan_ng_init_old(sdla_t*);
int wan_ng_remove_old(sdla_t*);
#endif

#if defined(__WINDOWS__)
extern int connect_to_interrupt_line(sdla_t *card);
extern void disconnect_from_interrupt_line(sdla_t *card);
int wp_aft_firmware_up_init(sdla_t* card, wandev_conf_t* conf);	/* AFT Firmware Update support */
#endif

extern sdla_t *aft_find_first_card_in_list(sdla_t *card, int type);

#endif	/* __KERNEL__ */
#endif	/* _WANPIPE_H */
