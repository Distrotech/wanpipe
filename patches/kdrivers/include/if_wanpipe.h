/*****************************************************************************
* if_wanpipe.h	Header file for the Sangoma AF_WANPIPE Socket 	
*
* Author: 	Nenad Corbic 	
*
* Copyright:	(c) 2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
* Jan 28, 2000	Nenad Corbic 	Initial Version
*
*****************************************************************************/

#ifndef __LINUX_IF_WAN_PACKET_H
#define __LINUX_IF_WAN_PACKET_H

#include <linux/sockios.h>

struct wan_sockaddr_ll
{
	unsigned short	sll_family;
	unsigned short	sll_protocol;
	int		sll_ifindex;
	unsigned short	sll_hatype;
	unsigned char	sll_pkttype;
	unsigned char	sll_halen;
	unsigned char	sll_addr[8];
	unsigned char   sll_device[14];
	unsigned char 	sll_card[14];

	unsigned int	sll_active_ch;
	unsigned char	sll_prot;
	unsigned char	sll_prot_opt;
	unsigned short  sll_mult_cnt;
	unsigned char	sll_seven_bit_hdlc;
};

typedef struct wan_debug_hdr{
	unsigned char free;
	unsigned char wp_sk_state;
	int rcvbuf;
	int sndbuf;
	int rmem;
	int wmem;
	int sk_count;
	unsigned char bound;
	char name[14];
	unsigned char d_state;
	unsigned char svc;
	unsigned short lcn;
	unsigned char mbox;
	unsigned char cmd_busy;
	unsigned char command;
	unsigned poll;
	unsigned poll_cnt;
	int rblock;	
} wan_debug_hdr_t;

#pragma pack(1)
typedef struct {
	unsigned char	error_flag;
	unsigned short	time_stamp;
	unsigned char	channel;
	unsigned char   direction;
	unsigned char	reserved[11];
} wp_sock_rx_hdr_t;

typedef struct {
        wp_sock_rx_hdr_t	api_rx_hdr;
        unsigned char   	data[1];
} wp_sock_rx_element_t;
#pragma pack()

#define MAX_NUM_DEBUG  		10

#define LAPB_PROT       	0x15
#define WP_LAPB_PROT		LAPB_PROT

#define X25_PROT       		0x16
#define WP_X25_PROT		X25_PROT

#define PVC_PROT       		0x17
#define WP_PVC_PROT		PVC_PROT

#define SS7_PROT       		0x18	
#define WP_SS7_PROT		SS7_PROT

#define SS7_MONITOR_PROT 	0x19
#define WP_SS7_MONITOR_PROT	SS7_MONITOR_PROT

#define DSP_PROT		0x20
#define WP_DSP_PROT		DSP_PROT

#define	SS7_FISU 	0x01
#define	SS7_LSSU	0x02
#define	SS7_MSU 	0x04
#define	RAW_HDLC 	0x08
#define SS7_ALL_PROT    (SS7_MSU|SS7_LSSU|SS7_FISU)

#define SS7_FISU_BIT_MAP 0
#define SS7_LSSU_BIT_MAP 1
#define SS7_MSU_BIT_MAP  2
#define HDLC_BIT_MAP     3

#define SS7_FISU_LEN 5
#define SS7_MSU_LEN 8
#define SS7_MSU_END_LEN 278

#define DECODE_SS7_PROT(a)     ((a==SS7_MSU)?"SS7 MSU":\
				(a==SS7_LSSU)?"SS7 LSSU":\
				(a==SS7_FISU)?"SS7 FISU":\
				(a==RAW_HDLC)?"RAW HDLC":\
				(a==(SS7_MSU|SS7_LSSU))?"SS7 MSU LSSU":\
				(a==(SS7_MSU|SS7_FISU))?"SS7 MSU FISU":\
				(a==(SS7_LSSU|SS7_FISU))?"SS7 LSSU FISU":\
				(a==(SS7_LSSU|SS7_FISU|SS7_MSU))?"SS7 MSU LSSU FISU":\
				"Unknown SS7 Prot")
#define DELTA_PROT_OPT 1
#define MULTI_PROT_OPT 2

typedef struct
{
	wan_debug_hdr_t debug[MAX_NUM_DEBUG];
}wan_debug_t;

#if 0
#define	SIOC_WANPIPE_GET_CALL_DATA	(SIOCPROTOPRIVATE + 0)
#define	SIOC_WANPIPE_SET_CALL_DATA	(SIOCPROTOPRIVATE + 1)
#define SIOC_WANPIPE_ACCEPT_CALL	(SIOCPROTOPRIVATE + 2)
#define SIOC_WANPIPE_CLEAR_CALL	        (SIOCPROTOPRIVATE + 3)
#define SIOC_WANPIPE_RESET_CALL	        (SIOCPROTOPRIVATE + 4)
#endif

enum {
	SIOC_WANPIPE_DEBUG = (SIOCPROTOPRIVATE),
	SIOC_WANPIPE_SET_NONBLOCK,
	SIOC_WANPIPE_CHECK_TX,
	SIOC_WANPIPE_SOCK_STATE,
	SIOC_WANPIPE_SOCK_FLUSH_BUFS
};

#define SIOC_ANNEXG_SOCK_STATE SIOC_WANPIPE_SOCK_STATE

enum {
	SIOCC_IF_WANPIPE_RESERVED = (SIOCDEVPRIVATE),
	SIOC_WANPIPE_PIPEMON,
	SIOC_WANPIPE_SNMP,
	SIOC_WANPIPE_SNMP_IFSPEED,

#if 0
	SIOC_WANPIPE_DEVICE,	/* GENERIC */
	SIOC_WANPIPE_DUMP,	/* GENERIC */
#endif
	
	SIOC_WAN_DEVEL_IOCTL,	/* uses wan_cmd_api_t */
	SIOC_WAN_EC_IOCTL,
	SIOC_WAN_FE_IOCTL,

	SIOC_WANAPI_DEVPRIVATE = SIOCDEVPRIVATE + 20,

	SIOC_ANNEXG_SET_NONBLOCK,
	SIOC_ANNEXG_CHECK_TX,
	SIOC_ANNEXG_DEV_STATE,
	
	SIOC_ANNEXG_BIND_SK,
	SIOC_ANNEXG_UNBIND_SK,
	SIOC_ANNEXG_GET_SK,
	SIOC_ANNEXG_KICK,

	SIOC_ANNEXG_PLACE_CALL,
	SIOC_ANNEXG_CLEAR_CALL,

	SIOC_WANPIPE_GET_TIME_SLOTS,
	SIOC_WANPIPE_GET_MEDIA_TYPE,

	SIOC_WANPIPE_GET_DEVICE_CONFIG_ID,

	SIOC_WANPIPE_TDM_API,

	SIOC_WANPIPE_DEVPRIVATE

};

#define WAN_DEVPRIV_SIOC(_val) (SIOC_WANPIPE_DEVPRIVATE+(_val))

#define SIOC_WANPIPE_BIND_SK 	SIOC_ANNEXG_BIND_SK 
#define SIOC_WANPIPE_UNBIND_SK 	SIOC_ANNEXG_UNBIND_SK
#define SIOC_WANPIPE_GET_SK 	SIOC_ANNEXG_GET_SK
#define SIOC_WANPIPE_KICK 	SIOC_ANNEXG_KICK
#define SIOC_WANPIPE_DEV_STATE  SIOC_ANNEXG_DEV_STATE

#define DECODE_API_CMD(cmd) ((cmd==SIOC_WANPIPE_PIPEMON)?"PIPEMON" : \
		             (cmd==SIOC_WANPIPE_SNMP)?"SNMP": \
			     (cmd==SIOC_ANNEXG_CHECK_TX)?"CHECK TX": \
			     (cmd==SIOC_ANNEXG_SOCK_STATE)?"SOCK STATE": \
			     (cmd==SIOC_ANNEXG_BIND_SK)?"BIND SK" : \
			     (cmd==SIOC_ANNEXG_UNBIND_SK)?"UNBIND SK" : \
			     (cmd==SIOC_ANNEXG_GET_SK)?"GET SK" : \
			     (cmd==SIOC_ANNEXG_DEV_STATE)?"DEV STATE" : \
			     (cmd==SIOC_ANNEXG_KICK)? "KICK" : "UNKNOWN")
			  

#define SIOC_WANPIPE_BSC_CMD 		SIOC_WANPIPE_EXEC_CMD
#define SIOC_WANPIPE_POS_CMD 		SIOC_WANPIPE_EXEC_CMD

/* Packet types */

#define WAN_PACKET_HOST		0		/* To us		*/
#define WAN_PACKET_BROADCAST	1		/* To all		*/
#define WAN_PACKET_MULTICAST	2		/* To group		*/
#define WAN_PACKET_OTHERHOST	3		/* To someone else 	*/
#define WAN_PACKET_OUTGOING		4		/* Outgoing of any type */
/* These ones are invisible by user level */
#define WAN_PACKET_LOOPBACK		5		/* MC/BRD frame looped back */
#define WAN_PACKET_FASTROUTE	6		/* Fastrouted frame	*/


/* AF Socket specific */
#define WAN_PACKET_DATA 	0
#define WAN_PACKET_CMD 		1
#define WAN_PACKET_ERR	        2

/* Packet socket options */

#define WAN_PACKET_ADD_MEMBERSHIP		1
#define WAN_PACKET_DROP_MEMBERSHIP		2

#define WAN_PACKET_MR_MULTICAST	0
#define WAN_PACKET_MR_PROMISC	1
#define WAN_PACKET_MR_ALLMULTI	2


#ifdef __KERNEL__

#include <linux/wanpipe_kernel.h>

#define MAX_PARENT_PROT_NUM 10

/* Private wanpipe socket structures. */
#if 0
struct wanpipe_opt
{
	void   *mbox;		/* Mail box  */
	void   *card; 		/* Card bouded to */
	netdevice_t *dev;	/* Bounded device */
	unsigned short lcn;	/* Binded LCN */
	unsigned char  svc;	/* 0=pvc, 1=svc */
	unsigned char  timer;   /* flag for delayed transmit*/	
	struct timer_list tx_timer;
	unsigned poll_cnt;
	unsigned char force;	/* Used to force sock release */
	atomic_t packet_sent;   
	void *datascope;
};
#else
struct wanpipe_opt
{
	netdevice_t *dev;	/* Bounded device */
	void *datascope;
	unsigned short num;
};
#endif

#define MAX_SOCK_CRC_QUEUE 3
#define MAX_SOCK_HDLC_BUF 2000
#define MAX_SOCK_HDLC_LIMIT MAX_SOCK_HDLC_BUF-500

typedef	struct wanpipe_hdlc_decoder{
	unsigned char 	rx_decode_buf[MAX_SOCK_HDLC_BUF];
	unsigned int  	rx_decode_len;
	unsigned char 	rx_decode_bit_cnt;
	unsigned char 	rx_decode_onecnt;
	
	unsigned long	hdlc_flag;
	unsigned short 	rx_orig_crc;
	unsigned short 	rx_crc[MAX_SOCK_CRC_QUEUE];
	unsigned short 	crc_fin;

	unsigned short 	rx_crc_tmp;
	int 		crc_cur;
	int 		crc_prv;
}wanpipe_hdlc_decoder_t;


typedef struct wanpipe_hdlc_engine
{

	wanpipe_hdlc_decoder_t tx_decoder;
	wanpipe_hdlc_decoder_t rx_decoder;

	struct sk_buff	*raw_rx_skb;
	struct sk_buff	*raw_tx_skb;

	struct sk_buff_head data_q;

	atomic_t 	refcnt;

	unsigned char	bound;

	unsigned long	active_ch;
	unsigned short  timeslots;
	struct wanpipe_hdlc_engine *next;

	int 		skb_decode_size;
	unsigned char	seven_bit_hdlc;

	void *sk_to_prot_map[MAX_PARENT_PROT_NUM];

}wanpipe_hdlc_engine_t;

typedef struct hdlc_list
{
	wanpipe_hdlc_engine_t *hdlc;
	struct hdlc_list *next;
}wanpipe_hdlc_list_t;

#define MAX_SOCK_CHANNELS 33
typedef struct wanpipe_parent
{
	wanpipe_hdlc_list_t *time_slot_hdlc_map[MAX_SOCK_CHANNELS];	
	wanpipe_hdlc_engine_t *hdlc_eng_list;
	unsigned char hdlc_enabled;
	
	rwlock_t lock;
	signed char time_slots;
	char media;
	unsigned char seven_bit_hdlc;

	struct tasklet_struct rx_task;
	struct sk_buff_head rx_queue;
	
	void *sk_to_prot_map[MAX_PARENT_PROT_NUM];
}wanpipe_parent_t;

typedef struct wanpipe_datascope
{
	unsigned long prot_type;
	unsigned int active_ch;
	struct sock *parent_sk;
	unsigned char delta;
	unsigned short max_mult_cnt;
	union {
		struct {
			unsigned short tx_fisu_skb_csum;
			unsigned short tx_lssu_skb_csum;
			void *tx_fisu_skb;
			void *tx_lssu_skb;

			unsigned short rx_fisu_skb_csum;
			unsigned short rx_lssu_skb_csum;
			void *rx_fisu_skb;
			void *rx_lssu_skb;
		}ss7;
	}u;

	unsigned char parent;
	wanpipe_hdlc_engine_t *hdlc_eng;
	wanpipe_parent_t *parent_priv;

}wanpipe_datascope_t;



#define STACK_IF_REQ 0x04
#define WANPIPE_HEADER_SZ 16


#ifdef LINUX_2_6
#define SK_PRIV(x)  ((struct wanpipe_opt*)(x->sk_user_data))
#define SK_PRIV_INIT(x,val) (x)->sk_user_data=(void*)(val)
#else
#define SK_PRIV(x)  ((struct wanpipe_opt*)(x->user_data))
#define SK_PRIV_INIT(x,val) (x)->user_data=(void*)(val)
#endif

#define DATA_SC(x)  ((wanpipe_datascope_t*)(SK_PRIV(x)->datascope)) 
#define DATA_SC_INIT(x,val)  (SK_PRIV(x)->datascope = (void*)(val))
#define PPRIV(x)    ((wanpipe_parent_t*)(DATA_SC(x)->parent_priv))
#define PPRIV_INIT(x,val)    (DATA_SC(x)->parent_priv = (void*)(val))

#define wp_dev_hold(dev) atomic_inc(&(dev)->refcnt)

#define wp_dev_put(dev) if (atomic_dec_and_test(&dev->refcnt)){ \
				DEBUG_TEST("%s: %d: Freeing ptr %p\n",__FUNCTION__,__LINE__,dev);\
				wan_free(dev); \
			}

#define wp_sock_hold(sk) 	atomic_inc(&(sk)->sk_refcnt)
#define wp_sock_put(sk)		atomic_dec_and_test(&(sk)->sk_refcnt)


struct wanpipe_api_register_struct 
{
	unsigned char init;
	int (*wanpipe_api_sock_rcv)(struct sk_buff *skb, netdevice_t *dev,  struct sock *sk);
	int (*wanpipe_api_connected)(struct net_device *dev, struct sock *sk);
	int (*wanpipe_api_disconnected)(struct sock *sk);
	int (*wanpipe_listen_rcv) (struct sk_buff *skb,  struct sock *sk);
	int (*sk_buf_check) (struct sock *sk, int len);
	int (*sk_poll_wake) (struct sock *sk);
	int (*wanpipe_api_connecting)(struct net_device *dev, struct sock *sk);
};


extern int  register_wanpipe_api_socket (struct wanpipe_api_register_struct *wan_api_reg);
extern void unregister_wanpipe_api_socket (void);

extern int bind_api_to_protocol (netdevice_t *dev, char *, unsigned short protocol, void *sk);
extern int bind_api_listen_to_protocol (netdevice_t *dev, char *,unsigned short protocol, void *sk_id);
extern int unbind_api_listen_from_protocol (unsigned short protocol, void *sk_id);

extern int wanpipe_lip_rx(void *chan, void *sk_id);
extern int wanpipe_lip_connect(void *chan, int );
extern int wanpipe_lip_disconnect(void *chan, int);
extern int wanpipe_lip_kick(void *chan,int);
extern int wanpipe_lip_get_if_status(void *chan, void *m);

extern int protocol_connected (netdevice_t *dev, void *sk_id);
extern int protocol_connecting (netdevice_t *dev, void *sk_id);
extern int protocol_disconnected (void *sk_id);
extern int wanpipe_api_sock_rx (struct sk_buff *skb, netdevice_t *dev, void *sk_id);
extern int wanpipe_api_listen_rx (struct sk_buff *skb, void *sk_id);
extern int wanpipe_api_buf_check (void *sk_id, int len);
extern int wanpipe_api_poll_wake(void *sk_id);

extern void signal_user_state_change(void);





#endif

#endif
