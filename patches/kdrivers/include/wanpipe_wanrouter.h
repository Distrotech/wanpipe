/*****************************************************************************
* wanpipe_wanrouter.h	Definitions for the WAN Multiprotocol Router Module.
*		This module provides API and common services for WAN Link
*		Drivers and is completely hardware-independent.
*
* Author: 	Nenad Corbic <ncorbic@sangoma.com>
* 		Alex Feldman <al.feldman@sangoma.com>
*		David Rokhvarg <davidr@sangoma.com>
*		Gideon Hack 	
* Additions:    Arnaldo Melo
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
* May 25, 2001  Alex Feldman	Added T1/E1 support  (TE1).
* Jul 21, 2000  Nenad Corbic	Added WAN_FT1_READY State
* Feb 24, 2000  Nenad Corbic    Added support for socket based x25api
* Jan 28, 2000  Nenad Corbic    Added support for the ASYNC protocol.
* Oct 04, 1999  Nenad Corbic 	Updated for 2.1.0 release
* Jun 02, 1999  Gideon Hack	Added support for the S514 adapter.
* May 23, 1999  Arnaldo Melo    Added local_addr to wanif_conf_t
*                               WAN_DISCONNECTING state added
* Jul 20, 1998	David Fong	Added Inverse ARP options to 'wanif_conf_t'
* Jun 12, 1998	David Fong	Added Cisco HDLC support.
* Dec 16, 1997	Jaspreet Singh	Moved 'enable_IPX' and 'network_number' to
*				'wanif_conf_t'
* Dec 05, 1997	Jaspreet Singh	Added 'pap', 'chap' to 'wanif_conf_t'
*				Added 'authenticator' to 'wan_ppp_conf_t'
* Nov 06, 1997	Jaspreet Singh	Changed Router Driver version to 1.1 from 1.0
* Oct 20, 1997	Jaspreet Singh	Added 'cir','bc','be' and 'mc' to 'wanif_conf_t'
*				Added 'enable_IPX' and 'network_number' to 
*				'wan_device_t'.  Also added defines for
*				UDP PACKET TYPE, Interrupt test, critical values
*				for RACE conditions.
* Oct 05, 1997	Jaspreet Singh	Added 'dlci_num' and 'dlci[100]' to 
*				'wan_fr_conf_t' to configure a list of dlci(s)
*				for a NODE 
* Jul 07, 1997	Jaspreet Singh	Added 'ttl' to 'wandev_conf_t' & 'wan_device_t'
* May 29, 1997 	Jaspreet Singh	Added 'tx_int_enabled' to 'wan_device_t'
* May 21, 1997	Jaspreet Singh	Added 'udp_port' to 'wan_device_t'
* Apr 25, 1997  Farhan Thawar   Added 'udp_port' to 'wandev_conf_t'
* Jan 16, 1997	Gene Kozin	router_devlist made public
* Jan 02, 1997	Gene Kozin	Initial version (based on wanpipe.h).
*****************************************************************************/

#ifndef	_WANPIPE_ROUTER_H
#define	_WANPIPE_ROUTER_H

#include "wanpipe_cfg_def.h"
#include "wanpipe_api_hdr.h"

#define	ROUTER_NAME	"wanrouter"	/* in case we ever change it */
#define	ROUTER_IOCTL	'W'		/* for IOCTL calls */
#define	ROUTER_MAGIC	0x524D4157L	/* signature: 'WANR' reversed */

#define CHECK_ROUTER_MAGIC(magic)	WAN_ASSERT(magic != ROUTER_MAGIC)

/* IOCTL codes for /proc/router/<device> entries (up to 255) */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define ROUTER_SETUP    	_IOW(ROUTER_IOCTL,  1, wan_conf_t) /* configure device  */
# define ROUTER_DOWN     	_IOWR(ROUTER_IOCTL, 2, wan_conf_t)/* shut down device  */
# define ROUTER_STAT     	_IOW(ROUTER_IOCTL,  3, wan_conf_t) /* get device status */
# define ROUTER_IFNEW    	_IOW(ROUTER_IOCTL,  4, wan_conf_t) /* add interface     */
# define ROUTER_IFDEL    	_IOW(ROUTER_IOCTL,  5, wan_conf_t) /* del interface     */
# define ROUTER_IFNEW_LIP    	_IOW(ROUTER_IOCTL,  6, wan_conf_t) /* add interface     */
# define ROUTER_IFDEL_LIP    	_IOW(ROUTER_IOCTL,  7, wan_conf_t) /* del interface     */
# define ROUTER_IFSTAT   	_IOW(ROUTER_IOCTL,  8, wan_conf_t) /* get interface status */
# define ROUTER_DEBUGGING 	_IOW(ROUTER_IOCTL,  9, wan_conf_t)/* get interface status */
# define ROUTER_VER 		_IOWR(ROUTER_IOCTL, 10, wan_conf_t) /* get router version */
# define ROUTER_PROCFS		_IOWR(ROUTER_IOCTL, 11, wan_conf_t) /* get procfs info */
# define ROUTER_DEBUG_READ 	_IOWR(ROUTER_IOCTL, 12, wan_conf_t) /* get dbg_msg */
# define ROUTER_USER     	_IOW(ROUTER_IOCTL, 16, u_int)     /* driver specific calls */
# define ROUTER_USER_MAX 	_IOW(ROUTER_IOCTL, 31, u_int)
# define SIOC_WANPIPE_PIPEMON	_IOWR('i', 150, struct ifreq) /* get monitor statistics */
# define SIOC_WANPIPE_DEVICE	_IOWR('i', 151, struct ifreq) /* set generic device */
# define SIOC_WAN_DEVEL_IOCTL	_IOWR('i', 152, struct ifreq) /* get hwprobe string */
# define SIOC_WANPIPE_DUMP	_IOWR('i', 153, struct ifreq) /* get memdump string (GENERIC) */
# define SIOC_AFT_CUSTOMER_ID	_IOWR('i', 154, struct ifreq) /* get AFT customer ID */
# define SIOC_WAN_EC_IOCTL	_IOWR('i', 155, struct ifreq) /* Echo Canceller interface */
# define SIOC_WAN_FE_IOCTL	_IOWR('i', 156, struct ifreq) /* FE interface */
#else

enum router_ioctls
{
	ROUTER_SETUP	= ROUTER_IOCTL<<8,	/* configure device */
	ROUTER_DOWN,				/* shut down device */
	ROUTER_STAT,				/* get device status */
	ROUTER_IFNEW,				/* add interface */
	ROUTER_IFDEL,				/* delete interface */
	ROUTER_IFSTAT,				/* get interface status */
	ROUTER_VER,				/* get router version */
	ROUTER_DEBUGGING,			/* get router version */
	ROUTER_DEBUG_READ,			/* get router version */

	ROUTER_IFNEW_LAPB,			/* add new lapb interface */
	ROUTER_IFDEL_LAPB,			/* delete a lapb interface */
	ROUTER_IFNEW_X25,			/* add new x25 interface */
	ROUTER_IFDEL_X25,			/* delete a x25 interface */
	ROUTER_IFNEW_DSP,			/* add new dsp interface */
	ROUTER_IFDEL_DSP,			/* delete a dsp interface */

	ROUTER_IFNEW_LIP,
	ROUTER_IFDEL_LIP,

	ROUTER_USER,				/* driver-specific calls */
	ROUTER_USER_MAX	= ROUTER_USER+31
};


/* Backward Compatibility for non sangoma drivers */
#ifdef __KERNEL__
# ifndef __LINUX__
#  define __LINUX__
# endif

# ifndef WAN_KERNEL
#  define WAN_KERNEL
# endif
#endif

#endif

#if defined(__WINDOWS__)
#undef __LINUX__
#endif

#ifndef SPROTOCOL /* conflict with CISCO_IP in wanpipe_sppp.h */

/* identifiers for displaying proc file data for dual port adapters */
#define PROC_DATA_PORT_0 0x8000	/* the data is for port 0 */
#define PROC_DATA_PORT_1 0x8001	/* the data is for port 1 */

/* NLPID for packet encapsulation (ISO/IEC TR 9577) */
#define	NLPID_IP	0xCC	/* Internet Protocol Datagram */
#define CISCO_IP	0x00	/* Internet Protocol Datagram - CISCO only */
#define	NLPID_SNAP	0x80	/* IEEE Subnetwork Access Protocol */
#define	NLPID_CLNP	0x81	/* ISO/IEC 8473 */
#define	NLPID_ESIS	0x82	/* ISO/IEC 9542 */
#define	NLPID_ISIS	0x83	/* ISO/IEC ISIS */
#define	NLPID_Q933	0x08	/* CCITT Q.933 */

#endif

/****** Data Types **********************************************************/

/*----------------------------------------------------------------------------
 * WAN Link Status Info (for ROUTER_STAT IOCTL).
 */
typedef struct wandev_stat
{
	unsigned state;		/* link state */
	unsigned ndev;		/* number of configured interfaces */

	/* link/interface configuration */
	unsigned connection;	/* permanent/switched/on-demand */
	unsigned media_type;	/* Frame relay/PPP/X.25/SDLC, etc. */
	unsigned mtu;		/* max. transmit unit for this device */

	/* physical level statistics */
	unsigned modem_status;	/* modem status */
	unsigned rx_frames;	/* received frames count */
	unsigned rx_overruns;	/* receiver overrun error count */
	unsigned rx_crc_err;	/* receive CRC error count */
	unsigned rx_aborts;	/* received aborted frames count */
	unsigned rx_bad_length;	/* unexpetedly long/short frames count */
	unsigned rx_dropped;	/* frames discarded at device level */
	unsigned tx_frames;	/* transmitted frames count */
	unsigned tx_underruns;	/* aborted transmissions (underruns) count */
	unsigned tx_timeouts;	/* transmission timeouts */
	unsigned tx_rejects;	/* other transmit errors */

	/* media level statistics */
	unsigned rx_bad_format;	/* frames with invalid format */
	unsigned rx_bad_addr;	/* frames with invalid media address */
	unsigned tx_retries;	/* frames re-transmitted */
	unsigned reserved[16];	/* reserved for future use */
} wandev_stat_t;


/* Front-End status */
enum fe_status {
	FE_UNITIALIZED = 0x00,
	FE_DISCONNECTED,
	FE_CONNECTED,
	FE_CONNECTING,
	FE_DISCONNECTING
};
#define WAN_FE_UNITIALIZED	FE_UNITIALIZED
#define	WAN_FE_DISCONNECTED	FE_DISCONNECTED
#define	WAN_FE_CONNECTED	FE_CONNECTED

/* 'modem_status' masks */
#define	WAN_MODEM_CTS	0x0001	/* CTS line active */
#define	WAN_MODEM_DCD	0x0002	/* DCD line active */
#define	WAN_MODEM_DTR	0x0010	/* DTR line active */
#define	WAN_MODEM_RTS	0x0020	/* RTS line active */

/* modem status changes */
#define WAN_DCD_HIGH			0x08
#define WAN_CTS_HIGH			0x20

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
typedef struct wan_conf
{
	devname_t	devname;
	void*		arg;
} wan_conf_t;
#endif



#if defined(WAN_KERNEL)

# include "wanpipe_includes.h"
# include "wanpipe_debug.h"
# include "wanpipe_common.h"
# include "wanpipe_events.h"
# include "wanpipe_cfg.h"

# if defined (CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
#  include "sdla_tdmv.h"
#  include "sdla_tdmv_dummy.h"
# endif

#if defined (__LINUX__)
# include <linux/fs.h>		/* support for device drivers */
# include <linux/proc_fs.h>	/* proc filesystem pragmatics */
# include <linux/inet.h>	/* in_aton(), in_ntoa() prototypes */
# ifndef KERNEL_VERSION
#  define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
# endif
#endif


#define REG_PROTOCOL_FUNC(str)		wan_set_bit(0, (unsigned long*)&str.init)
#define UNREG_PROTOCOL_FUNC(str)	wan_clear_bit(0, (unsigned long*)&str.init)
#define IS_PROTOCOL_FUNC(str)		wan_test_bit(0, (unsigned long*)&str.init)

#define IS_FUNC_CALL(str, func)					\
	(IS_PROTOCOL_FUNC(str) && str.func) ? 1 : 0

/****** Kernel Interface ****************************************************/

typedef struct wan_rtp_chan
{
	netskb_t *rx_skb;
	netskb_t *tx_skb;
	u32 rx_ts;
	u32 tx_ts;
}wan_rtp_chan_t;


/*----------------------------------------------------------------------------
 * WAN device data space.
 */
struct wan_dev_le {
	WAN_LIST_ENTRY(wan_dev_le)	dev_link;
	netdevice_t			*dev;
};
#define WAN_DEVLE2DEV(devle)	(devle && devle->dev) ? devle->dev : NULL
WAN_LIST_HEAD(wan_dev_lhead, wan_dev_le);

typedef struct wan_device
{
	unsigned int magic;	/* magic number */
	char* name;			/* -> WAN device name (ASCIIZ) */
	void* priv;			/* -> driver private data */
	unsigned config_id;		/* Configuration ID */
					/****** hardware configuration ******/
	unsigned ioport;		/* adapter I/O port base #1 */
	char S514_cpu_no[1];		/* PCI CPU Number */
	unsigned char S514_slot_no;	/* PCI Slot Number */
	unsigned char S514_bus_no;	/* PCI Bus Number */
	int irq;			/* interrupt request level */
	int dma;			/* DMA request level */
	unsigned int bps;		/* data transfer rate */
	unsigned int mtu;		/* max physical transmit unit size */
	unsigned int udp_port;          /* UDP port for management */
	unsigned char ttl;		/* Time To Live for UDP security */
	unsigned int enable_tx_int; 	/* Transmit Interrupt enabled or not */
	char electrical_interface;			/* RS-232/V.35, etc. */
	char clocking;			/* external/internal */
	char line_coding;		/* NRZ/NRZI/FM0/FM1, etc. */
	char station;			/* DTE/DCE, primary/secondary, etc. */
	char connection;		/* permanent/switched/on-demand */
	char signalling;		/* Signalling RS232 or V35 */
	char read_mode;			/* read mode: Polling or interrupt */
	char new_if_cnt;		/* Number of interfaces per wanpipe */
	char del_if_cnt;		/* Number of times del_if() gets called */
	unsigned char piggyback;        /* Piggibacking a port */

					/****** status and statistics *******/
	char state;			/* device state */
	char api_status;		/* device api status */
	struct net_device_stats stats; 	/* interface statistics */
	unsigned reserved[16];		/* reserved for future use */
	unsigned long critical;		/* critical section flag */
	wan_spinlock_t	lock;           /* Support for SMP Locking */

					/****** device management methods ***/
	int (*setup) (struct wan_device *wandev, wandev_conf_t *conf);
	int (*shutdown) (struct wan_device *wandev, wandev_conf_t* conf);
	int (*update) (struct wan_device *wandev);
#if defined(__LINUX__)
	int (*ioctl) (struct wan_device *wandev, unsigned int cmd, unsigned long arg);
#else
	int (*ioctl) (struct wan_device *wandev, u_long cmd, caddr_t arg);
#endif
	int (*new_if) (struct wan_device *wandev, netdevice_t *dev, wanif_conf_t *conf);
	int (*del_if) (struct wan_device *wandev, netdevice_t *dev);
	
	/****** maintained by the router ****/
#if 0
	struct wan_device*	next;	/* -> next device */
	netdevice_t*	dev;	/* list of network interfaces */
#endif
	WAN_LIST_ENTRY(wan_device)	next;	/* -> next device */
	struct wan_dev_lhead		dev_head;
	wan_spinlock_t			dev_head_lock;			
	unsigned ndev;			/* number of interfaces */

#if defined(__LINUX__)
	struct proc_dir_entry *dent;	/* proc filesystem entry */
	struct proc_dir_entry *link;	/* proc filesystem entry per link */

	// Proc fs functions
	int (*get_config_info) 		(void*, struct seq_file* m, int *);
	int (*get_status_info) 		(void*, struct seq_file* m, int *);
	
	wan_get_info_t*	get_dev_config_info;
	wan_get_info_t*	get_if_info;
	write_proc_t*	set_dev_config;
	write_proc_t*	set_if_info;
#endif

	int 	(*get_info)(void*, struct seq_file* m, int *);
	void	(*fe_enable_timer) (void* card_id);
	void	(*te_report_rbsbits) (void* card_id, int channel, unsigned char rbsbits);
	void	(*te_report_alarms) (void* card_id, uint32_t alarms);
	void	(*te_link_state)  (void* card_id);
	void	(*te_link_reset)  (void* card_id);
	int		(*te_signaling_config) (void* card_id, unsigned long);
	int		(*te_disable_signaling) (void* card_id, unsigned long);
	int		(*te_read_signaling_config) (void* card_id);
	int		(*report_dtmf) (void* card_id, int, unsigned char);
	void	(*ec_enable_timer) (void* card_id);
	void	(*ec_clock_ctrl) (void* card_id);
	
	struct {
		void	(*rbsbits) (void* card_id, int, unsigned char);
		void	(*alarms) (void* card_id, wan_event_t*);
		void	(*tone) (void* card_id, wan_event_t*);	
		void	(*hook) (void* card_id, wan_event_t*);	
		void	(*ringtrip) (void* card_id, wan_event_t*);	
		void	(*ringdetect) (void* card_id, wan_event_t*);	
		void	(*linkstatus) (void* card_id, wan_event_t*);
		void	(*polarityreverse) (void* card_id, wan_event_t*);
	} event_callback;
	
	unsigned char 		ignore_front_end_status;
	unsigned char		line_idle;
	unsigned int		card_type;
	atomic_t			if_cnt;
	atomic_t			if_up_cnt;
	wan_sdlc_conf_t		sdlc_cfg;
	wan_bscstrm_conf_t 	bscstrm_cfg;
	int 				(*debugging) (struct wan_device *wandev);
	int 				(*debug_read) (void*, void*);
	int 				comm_port;

#if defined(__LINUX__)
	spinlock_t		get_map_lock;
	int 			(*get_map)(struct wan_device*,netdevice_t*,struct seq_file* m, int *);
	int 			(*bind_annexg) (netdevice_t *dev, netdevice_t *adev);
	netdevice_t 	*(*un_bind_annexg) (struct wan_device *wandev,netdevice_t *adev);
	void 			(*get_active_inactive)(struct wan_device*,netdevice_t*,void*);
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	int 					(*wanpipe_ioctl) (netdevice_t*, struct ifreq*, int);
#endif
	unsigned char			macAddr[ETHER_ADDR_LEN];

	sdla_fe_iface_t			fe_iface;
	sdla_fe_notify_iface_t	fe_notify_iface;

	void					*ec_dev;
	
	unsigned long			ec_enable_map;
	unsigned long			fe_ec_map;
	wan_ticks_t				ec_intmask;
		
	int						(*ec_enable)(void *pcard, int, int);
	int						(*__ec_enable)(void *pcard, int, int);
	int						(*ec_state) (void* card_id, wan_hwec_dev_state_t *ec_state);
	int 						(*ec_tdmapi_ioctl)(void *ec_dev_t, wan_iovec_list_t *iovec_list);


	unsigned char			(*write_ec)(void*, unsigned short, unsigned char);
	unsigned char			(*read_ec)(void*, unsigned short);
	int						(*hwec_reset)(void* card_id, int);
	int						(*hwec_enable)(void* card_id, int, int);
	
	unsigned long			rtp_tap_call_map;
	unsigned long			rtp_tap_call_status;
	wan_rtp_chan_t			rtp_chan[32];
	void 					*rtp_dev;
	int   					rtp_len;
	void					(*rtp_tap)(void *card, u8 chan, u8* rx, u8* tx, u32 len);
	void 					*port_cfg;

} wan_device_t;

WAN_LIST_HEAD(wan_devlist_, wan_device);

struct wanpipe_fw_register_struct
{
	unsigned char init;
	
	int (*bind_api_to_svc)(char *devname, void *sk_id);
	int (*bind_listen_to_link)(char *devname, void *sk_id, unsigned short protocol);
	int (*unbind_listen_from_link)(void *sk_id,unsigned short protocol);
};

/* Public functions available for device drivers */
extern int register_wan_device(wan_device_t *wandev);
extern int unregister_wan_device(char *name);

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
extern char* in_ntoa(uint32_t in);
extern int wanpipe_lip_rx(void *chan, void *sk_id);
extern int wanpipe_lip_connect(void *chan, int );
extern int wanpipe_lip_disconnect(void *chan, int);
extern int wanpipe_lip_kick(void *chan,int);
extern int wanpipe_lip_get_if_status(void *chan, void *m);
#elif defined(__LINUX__)
unsigned short wanrouter_type_trans(struct sk_buff *skb, netdevice_t *dev);
int wanrouter_encapsulate(struct sk_buff *skb, netdevice_t *dev,unsigned short type);

extern WAN_IOCTL_RET_TYPE WANDEF_IOCTL_FUNC(wanrouter_ioctl, struct file *file, unsigned int cmd, unsigned long arg);
/* Proc interface functions. These must not be called by the drivers! */
extern int wanrouter_proc_init(void);
extern void wanrouter_proc_cleanup(void);
extern int wanrouter_proc_add(wan_device_t *wandev);
extern int wanrouter_proc_delete(wan_device_t *wandev);
extern int wanrouter_proc_add_protocol(wan_device_t* wandev);
extern int wanrouter_proc_delete_protocol(wan_device_t* wandev);
extern int wanrouter_proc_add_interface(wan_device_t*,struct proc_dir_entry**,
					char*,void*);
extern int wanrouter_proc_delete_interface(wan_device_t*, char*);

extern void *sdla_get_hw_probe(void);
extern int wan_run_wanrouter(char* hwdevname, char *devname, char *action);

extern int register_wanpipe_fw_protocol (struct wanpipe_fw_register_struct *wp_fw_reg);
extern void unregister_wanpipe_fw_protocol (void);

extern void wan_skb_destructor (struct sk_buff *skb);
#endif

extern unsigned long wan_get_ip_address (netdevice_t *dev, int option);

void *wanpipe_ec_register(void*, u_int32_t, int,int, void*);
int wanpipe_ec_unregister(void*,void*);
int wanpipe_ec_isr(void*);
int wanpipe_ec_poll(void*,void*);
int wanpipe_ec_ready(void*);
int wanpipe_ec_event_ctrl(void*,void*,wan_event_ctrl_t*);
int wanpipe_ec_tdm_api_ioctl(void *pec_dev, wan_iovec_list_t *iovec_list);
int wanpipe_wandev_create(void);
int wanpipe_wandev_free(void);

int wan_device_new_if (wan_device_t *wandev, wanif_conf_t *u_conf, int user);
int wan_device_shutdown (wan_device_t *wandev, wandev_conf_t *u_conf);
int wan_device_setup (wan_device_t *wandev, wandev_conf_t *u_conf, int user);
wan_device_t *wan_find_wandev_device(char *name);


#endif	/* __KERNEL__ */
#endif	/* _ROUTER_H */
