/*****************************************************************************
* wanpipe_tdm_api.h 
* 		
* 		WANPIPE(tm) AFT TE1 Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2005 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Oct 04, 2005	Nenad Corbic	Initial version.
*
* Jul 25, 2006	David Rokhvarg	<davidr@sangoma.com>	Ported to Windows.
*****************************************************************************/

#ifndef __WANPIPE_TDM_API_H_
#define __WANPIPE_TDM_API_H_ 


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe.h"
#include "wanpipe_codec_iface.h"
#include "wanpipe_api_iface.h"
#include "wanpipe_cdev_iface.h"

#if defined(__WINDOWS__) && defined(__KERNEL__)
# define inline __inline
#endif


#ifdef WAN_KERNEL

#define WP_TDM_API_MAX_LEN 	4096+sizeof(wp_api_hdr_t)
#define WP_TDM_API_EVENT_MAX_LEN sizeof(wp_api_event_t)+sizeof(wp_api_hdr_t)

#define WP_TDM_API_CHUNK_SZ 	8
#define WP_RM_RXFLASHTIME		0 /* Default disable flash */
enum {
	WP_TDM_HDLC_TX,
	WP_TDM_DOWN
};


#if 1
#undef WAN_TDMAPI_DTMF_TEST
#else
#define WAN_TDMAPI_DTMF_TEST 1
typedef struct wp_dtmf_seq
{
	u8 dtmf_tone;
}wp_dtmf_seq_t;

static _inline unsigned char wp_get_next_expected_digit(unsigned char current_digit)
{
	switch(current_digit)
	{
	case '0':
		return '1';
	case '1':
		return '2';
	case '2':
		return '3';
	case '3':
		return '4';
	case '4':
		return '5';
	case '5':
		return '6';
	case '6':
		return '7';
	case '7':
		return '8';
	case '8':
		return '9';
	case '9':
		return 'A';
	case 'A':
		return 'B';
	case 'B':
		return 'C';
	case 'C':
		return 'D';
	case 'D':
		return '#';
	case '#':
		return '*';
	case '*':
		return '0';
	default:
		return '?';
	}
}

#endif

#define WPTDM_CHAN_OP_MODE(tdm_api) (tdm_api->operation_mode==WP_TDM_OPMODE_CHAN)
#define WPTDM_SPAN_OP_MODE(tdm_api) (tdm_api->operation_mode)
#define WPTDM_WIN_LEGACY_OP_MODE(tdm_api) (tdm_api->operation_mode==WP_TDM_OPMODE_LEGACY_WIN_API)

#define WPTDM_WIN_LEGACY_API_MODE(tdm_api) (tdm_api->api_mode==WP_TDM_API_MODE_LEGACY_WIN_API)

#define TDMAPI_MAX_BUFFER_MULTIPLIER 8

typedef struct wanpipe_tdm_api_card_dev {

	int ref_cnt;
	unsigned int 	rbs_poll_timeout;
	unsigned long 	rbs_poll_cnt;
	unsigned long 	rbs_long_poll_cnt;
	unsigned int	max_timeslots;
	unsigned int	sig_timeslot_map;
	unsigned int	rbscount;
	int				rbs_enable_cnt;
} wanpipe_tdm_api_card_dev_t;

typedef struct wanpipe_tdm_api_dev {

	u32 	init;
	void 	*chan;/* pointer to private_area_t */
	void 	*card;/* pointer to sdla_t */
	char 	name[WAN_IFNAME_SZ];
	wan_mutexlock_t lock;
	wan_spinlock_t irq_lock;

	u32	used, open_cnt;
	u8	tdm_span;
	u8	tdm_chan;
	u8	state;
	u8	hdlc_framing;
	u32	active_ch;		/* ALEX */

	u8 *rx_buf;
	u32 rx_buf_len;
	u8 *tx_buf;
	u32 tx_buf_len;
	
	wanpipe_api_dev_cfg_t cfg;

	wan_skb_queue_t 	wp_rx_list;
	wan_skb_queue_t 	wp_rx_free_list;
	netskb_t 			*rx_skb;

	wan_skb_queue_t 	wp_tx_list;
	wan_skb_queue_t 	wp_tx_free_list;
	netskb_t 			*tx_skb;
	u8					tx_channelized;

	wan_skb_queue_t 	wp_event_list;
	wan_skb_queue_t 	wp_event_bh_list;
	wan_skb_queue_t 	wp_event_free_list;
	wan_taskq_t 		wp_api_task;

	wan_skb_queue_t 	wp_dealloc_list;

	wp_api_hdr_t	*rx_hdr;
	wp_api_hdr_t	tx_hdr;
	
	u32				busy;
	u32				kick;
	
	u32 			critical;
	u32				master;
	
	/* Used For non channelized drivers */
	u8			rx_data[WP_TDM_API_CHUNK_SZ];
	u8			tx_data[WP_TDM_API_CHUNK_SZ];

	u8* rx_gain;
	u8* tx_gain;
	
#if 0
	int (*get_hw_mtu_mru)(void *chan);
	int (*set_hw_mtu_mru)(void *chan, u32 hw_mtu_mru);
	int (*get_usr_mtu_mru)(void *chan);
	int (*set_usr_mtu_mru)(void *chan, u32 usr_mtu_mru);
#endif	
	 
	int (*event_ctrl)(void *chan, wan_event_ctrl_t*);
	int (*read_rbs_bits)(void *chan, u32 ch, u8 *rbs_bits);
	int (*write_rbs_bits)(void *chan, u32 ch, u8 rbs_bits);
	int (*write_hdlc_frame)(void *chan, netskb_t *skb,  wp_api_hdr_t *hdr);
	int (*write_hdlc_check)(void *chan, int lock, int bufs);
	int (*write_hdlc_timeout)(void *chan, int lock);
	int (*pipemon)(void* card, void* chan, void *udata);
	int (*driver_ctrl)(void *chan_ptr, int cmd, wanpipe_api_cmd_t *api_cmd);

	u32		 		rbs_rx_bits[NUM_OF_E1_CHANNELS];	/* Rx RBS Bits */
#ifdef WAN_TDMAPI_DTMF_TEST
	wp_dtmf_seq_t 	dtmf_check[NUM_OF_E1_CHANNELS+1];	/* Rx RBS Bits */
 	wp_dtmf_seq_t 	dtmf_check_event[NUM_OF_E1_CHANNELS+1];	/* Rx RBS Bits */
#endif
	u8		 		dtmfsupport;
	void 			*cdev;
	u8				operation_mode;/* WP_TDM_OPMODE */
	u8				api_mode;
	u32				mtu_mru;
	
	void *			mtp1_dev;
	u32				buffer_multiplier;
	u32				timeslots;
	u8				loop_reenable_hwec;
}wanpipe_tdm_api_dev_t;


typedef struct wanpipe_tdm_api_span {
	unsigned int timeslot_map;
	wanpipe_tdm_api_dev_t chans[NUM_OF_E1_CHANNELS+1];
	int usage;
}wanpipe_tdm_api_span_t;
 
static __inline int __wp_tdm_get_open_cnt( wanpipe_tdm_api_dev_t *tdm_api)
{
	return tdm_api->open_cnt;
}
 

static __inline int wp_tdm_get_open_cnt( wanpipe_tdm_api_dev_t *tdm_api)
{
	wan_smp_flag_t flags;
	int cnt;
	wan_mutex_lock(&tdm_api->lock,&flags);
	cnt=__wp_tdm_get_open_cnt(tdm_api);
	wan_mutex_unlock(&tdm_api->lock,&flags);
	return cnt;
}

static __inline int wp_tdm_inc_open_cnt( wanpipe_tdm_api_dev_t *tdm_api)
{
	wan_smp_flag_t flags;
	int cnt;
	wan_mutex_lock(&tdm_api->lock,&flags);
	tdm_api->open_cnt++;
	cnt = tdm_api->cfg.open_cnt = tdm_api->open_cnt;
	wan_mutex_unlock(&tdm_api->lock,&flags);
	return cnt;
}

static __inline int wp_tdm_dec_open_cnt( wanpipe_tdm_api_dev_t *tdm_api)
{
	wan_smp_flag_t flags;
	int cnt;
	wan_mutex_lock(&tdm_api->lock,&flags);
	tdm_api->open_cnt--;
	cnt = tdm_api->cfg.open_cnt = tdm_api->open_cnt;
	wan_mutex_unlock(&tdm_api->lock,&flags);
	return cnt;
}

static __inline int is_tdm_api(void *chan, wanpipe_tdm_api_dev_t *tdm_api)
{
	if (!tdm_api) {
		return 0;
	}

	if (wan_test_bit(0,&tdm_api->init)) {
		return 1;
	}
	
	if (tdm_api->chan == chan){
		return 1;
	}
	return 0;
}


static __inline int is_tdm_api_stopped(wanpipe_tdm_api_dev_t *tdm_api)
{

	return wan_test_bit(0,&tdm_api->busy);
}

static __inline void wp_tdm_api_stop(wanpipe_tdm_api_dev_t *tdm_api)
{
	wan_set_bit(0,&tdm_api->busy);
}

static __inline void wp_tdm_api_start(wanpipe_tdm_api_dev_t *tdm_api)
{

	wan_clear_bit(0,&tdm_api->busy);
}



static inline int wp_tdmapi_check_mtu(void* pcard, unsigned long timeslot_map, int chunksz, int *mtu)
{
	sdla_t	*card = (sdla_t*)pcard;
	int	x, num_of_channels = 0, max_channels;

	max_channels = GET_TE_CHANNEL_RANGE(&card->fe);
	for (x = 0; x < max_channels; x++) {
		if (wan_test_bit(x,&timeslot_map)){
			num_of_channels++;
		}
	}
	*mtu = chunksz * num_of_channels;
	return 0;
}

static inline u8 wp_tdmapi_get_span(sdla_t *card)
{
	return card->tdmv_conf.span_no;
}

static inline int wp_tdmapi_alloc_q(wanpipe_tdm_api_dev_t *tdm_api, wan_skb_queue_t *queue, int size, int cnt)
{
	netskb_t *skb;
	int i;
#if defined(__WINDOWS__) && defined(DBG_QUEUE_IRQ_LOCKING)
	KIRQL  OldIrql;
#endif

	if (!size || size > WP_TDM_API_MAX_LEN) {
		return -1;
	}	

	for (i=0;i<cnt;i++) {
		skb=wan_skb_kalloc(size);
		if (!skb) {
			if (WAN_NET_RATELIMIT()) {
			DEBUG_EVENT("%s: Error: Failed to Allocatet Memory %s:%d\n",
					tdm_api->name, __FUNCTION__,__LINE__);
			}
			return -1;
		}
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);

#if defined(__WINDOWS__) && defined(DBG_QUEUE_IRQ_LOCKING)
		/* this is an emulation of irq lock priority */
		KeRaiseIrql(DISPATCH_LEVEL + 1, &OldIrql);//fixme: remove it!!
#endif
		
		wan_skb_queue_tail(queue,skb);

#if defined(__WINDOWS__) && defined(DBG_QUEUE_IRQ_LOCKING)
		KeLowerIrql(OldIrql);//fixme: remove it!!
#endif
	}

	return 0;
}


extern int wanpipe_tdm_api_rx_tx (wanpipe_tdm_api_dev_t *tdm_api, 
				 u8 *rx_data, u8 *tx_data, int len);	  
extern int wanpipe_tdm_api_reg(wanpipe_tdm_api_dev_t *tdm_api);
extern int wanpipe_tdm_api_unreg(wanpipe_tdm_api_dev_t *tdm_api);
extern int wanpipe_tdm_api_update_state(wanpipe_tdm_api_dev_t *tdm_api, int state);
extern int wanpipe_tdm_api_kick(wanpipe_tdm_api_dev_t *tdm_api);
extern int wanpipe_tdm_api_span_rx(wanpipe_tdm_api_dev_t *tdm_api, netskb_t *skb);
extern int wanpipe_tdm_api_is_rbsbits(sdla_t *card);
extern int wanpipe_tdm_api_rbsbits_poll(sdla_t * card);
extern int wanpipe_tdm_api_span_rx_tx(sdla_t *card, wanpipe_tdm_api_span_t *tdm_span, u32 buf_sz, unsigned long mask, int circ_buf_len);
/* FIXME: Create proper TDM API Interface */
#if 0
#define WAN_TDMAPI_CALL(func, args, err)					\
	if (card->tdmv_iface.func){					\
		err = card->tdmv_iface.func args;			\
	}else{								\
		DEBUG_EVENT("%s: Internal Error (%s:%d)!\n",\
				card->devname,				\
				__FUNCTION__,__LINE__);			\
		err = -EINVAL;						\
	}

typedef struct wan_tdm_api_iface_
{
	int	(*check_mtu)(void*, unsigned long, int *);
	int	(*create)(void* pcard, wanif_conf_t*);
	int	(*remove)(void* pcard);
	int	(*reg)(void*, wan_tdmv_if_conf_t*, unsigned int, unsigned char, void*);
	int	(*unreg)(void* pcard, unsigned long ts_map);
	int	(*software_init)(wan_tdmv_t*);
	int 	(*state)(void*, int);
	int	(*running)(void*);
	int	(*rx_tx)(void*, netskb_t*);
	int	(*rx_chan)(wan_tdmv_t*, int, unsigned char*, unsigned char*);
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN)
	int	(*rx_dchan)(wan_tdmv_t*, int, unsigned char*, unsigned int); 
#endif
	int	(*rx_tx_span)(void *pcard);
	int	(*is_rbsbits)(wan_tdmv_t *);
	int	(*rbsbits_poll)(wanpipe_tdm_api_dev_t *, void*);
	int	(*init)(void*, wanif_conf_t*);
	int	(*free)(wanpipe_tdm_api_dev_t*);
	int	(*polling)(void*);
	int	(*buf_rotate)(void *pcard,u32,unsigned long);
	int	(*ec_span)(void *pcard);

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	int	(*update_regs)(void*, int, u8*);	/* USB-FXO */
#endif
} wan_tdm_api_iface_t;

#endif
#endif

#endif
