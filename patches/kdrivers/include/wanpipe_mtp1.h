/*****************************************************************************
* wanpipe_mtp1.h
*
* 		WANPIPE(tm) MTP1 Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2009 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================*/



enum {
	WP_MTP1_HDLC_8BIT,
	WP_MTP1_HDLC_7BIT,
	WP_MTP1_HDLC_BIT_0_FIRST,
	WP_MTP1_HDLC_BIT_7_FIRST,	
};

/* Possible error_flagRX packet errors */ 
enum {
	WP_MTP1_FIFO_ERROR_BIT,
	WP_MTP1_CRC_ERROR_BIT,
	WP_MTP1_ABORT_ERROR_BIT,
	WP_MTP1_FRAME_ERROR_BIT,
	WP_MTP1_DMA_ERROR_BIT
};

typedef struct wp_mtp1_cfg{

	u8 hdlc_op_mode;	/* 8bit or 7bit hdlc */
	u8 hdlc_bit_endian;	/* bit 0 first */
	u32 param_N;
	u32 max_mru;
	
} wp_mtp1_cfg_t;


typedef struct wp_mtp1_stats{
	
	u32 rx_octet_count;	
	u32 rx_suerm_count;
	u32 tx_octet_count;
	
} wp_mtp1_stats_t;


typedef struct wp_mtp1_reg {
	void *priv_ptr;
	int (*rx_data)(void *priv_ptr, u8 *data, int len, uint32_t err_code);
	int (*rx_suerm)(void *priv_ptr);
	int (*tx_data)(void *priv_ptr, u8 *data, int len);
	int (*wakeup)(void *priv_ptr);
	int (*trace)(void *priv_ptr, u8 *data, int len, int dir);
	wp_mtp1_cfg_t cfg;
}wp_mtp1_reg_t;


extern void *wp_mtp1_register(wp_mtp1_reg_t *reg);
extern int wp_mtp1_free(void *mtp1);
extern int wp_mtp1_rx_handler(void *mtp1, u8 *data, int len);
extern int wp_mtp1_tx_data(void *mtp1, netskb_t *skb, wp_api_hdr_t *hdr, netskb_t *rskb);
extern int wp_mtp1_tx_bh_handler(void *mtp1, u8 *data, int mtu);
extern int wp_mtp1_dealloc_q_len(void *mtp1);
extern int wp_mtp1_poll_check(void *mtp1);
extern int wp_mtp1_poll(void *mtp1, wan_spinlock_t *lock);
extern int wp_mtp1_tx_check(void *mtp1);
