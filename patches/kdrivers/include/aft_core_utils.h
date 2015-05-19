/*****************************************************************************
* aft_core_utils.h
* 		
* 		WANPIPE(tm) AFT CORE Hardware Support - Utilities
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================*/


#ifndef __AFT_CORE_UTILS_H_
#define __AFT_CORE_UTILS_H_

#include "aft_core_private.h"

int 	aft_read_security(sdla_t *card);
int 	aft_front_end_mismatch_check(sdla_t * card);
int 	aft_realign_skb_pkt(private_area_t *chan, netskb_t *skb);
void 	aft_wdt_set(sdla_t *card, unsigned char val);
void 	aft_wdt_reset(sdla_t *card);
int 	process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				private_area_t*,
				int local_dev);

void 	aft_channel_txdma_ctrl(sdla_t *card, private_area_t *chan, int on);
void 	aft_channel_rxdma_ctrl(sdla_t *card, private_area_t *chan, int on);
void 	aft_channel_txintr_ctrl(sdla_t *card, private_area_t *chan, int on);
void 	aft_channel_rxintr_ctrl(sdla_t *card, private_area_t *chan, int on);

int 	aft_alloc_rx_dma_buff(sdla_t *card, private_area_t *chan, int num, int irq);
int 	aft_init_requeue_free_skb(private_area_t *chan, netskb_t *skb);

void	 aft_tx_dma_skb_init(private_area_t *chan, netskb_t *skb);

int 	aft_tslot_sync_ctrl(sdla_t *card, private_area_t *chan, int mode);

int 	aft_tdmapi_mtu_check(sdla_t *card_ptr, unsigned int *mtu);

int 	aft_devel_ioctl(sdla_t *card, struct ifreq *ifr);

void 	aft_tx_fifo_under_recover (sdla_t *card, private_area_t *chan);

unsigned char aft_write_ec (void *pcard, unsigned short off, unsigned char value);
unsigned char aft_read_cpld(sdla_t *card, unsigned short cpld_off);
unsigned char aft_read_ec (void *pcard, unsigned short off);
unsigned char aft_read_customer_id(sdla_t *card);

int 	aft_read(sdla_t *card, wan_cmd_api_t *api_cmd);
int 	aft_write_cpld(void *pcard, unsigned short off,unsigned char data);

#if defined(__LINUX__)
int if_change_mtu(netdevice_t *dev, int new_mtu);
#endif

int 	update_comms_stats(sdla_t* card);
int 	aft_handle_clock_master (sdla_t *card_ptr);
void 	wanpipe_wake_stack(private_area_t* chan);
int 	aft_find_master_if_and_dchan(sdla_t *card, int *master_if, u32 active_ch);

void 	aft_set_ss7_force_rx(sdla_t *card, private_area_t *chan);
void 	aft_clear_ss7_force_rx(sdla_t *card, private_area_t *chan);
int 	update_comms_stats(sdla_t* card);
int 	aft_find_master_if_and_dchan(sdla_t *card, int *master_if,u32 active_ch);
int 	aft_hwec_config (sdla_t *card, private_area_t *chan, wanif_conf_t *conf, int ctrl);
int 	aft_fifo_intr_ctrl(sdla_t *card, int ctrl);
int 	aft_tdm_intr_ctrl(sdla_t *card, int ctrl);
int 	aft_tdm_ring_rsync(sdla_t *card);

int 	aft_core_send_serial_oob_msg (sdla_t *card);
int 	wan_user_process_udp_mgmt_pkt(void* card_ptr, void* chan_ptr, void *udata);
int     aft_check_and_disable_dchan_optimization(sdla_t *card, private_area_t *chan, char *usedby);

int 	aft_chip_set_clock(sdla_t *card);

/*=================================================================
 * Used for debugging only
 *================================================================*/

static __inline wan_time_t aft_calc_elapsed(struct timeval *started, struct timeval *ended)
{
	wan_time_t ms;
	
#if 0
	if (started->tv_usec == 0) {
		return 0;
	}
#endif

	if (ended->tv_sec < started->tv_sec || ended->tv_usec < started->tv_usec) {
		return 0;
	}

	ms = ((ended->tv_sec - started->tv_sec)*1000) + ((ended->tv_usec-started->tv_usec)/1000);

	return ms;

#if 0
	if (ended->tv_usec  > started->tv_usec) {
		return ended->tv_usec -  started->tv_usec;
	} else {
		return 0;
	}
#endif
}

#define AFT_PERFT_TIMING_START(card,var)  if (card->aft_perf_stats_enable) aft_timing_start(&card->aft_perf_stats.var)

static int __inline aft_timing_start(aft_driver_timing_t *drv_timing)
{
	do_gettimeofday(&drv_timing->timing_tv);
	return 0;
}

#define AFT_PERFT_TIMING_STOP_AND_CALC(card,var)  if (card->aft_perf_stats_enable) aft_timing_stop_calculate_elapsed(&card->aft_perf_stats.var)

static int __inline aft_timing_stop_calculate_elapsed(aft_driver_timing_t *drv_timing)
{

	unsigned long elapsed=0;
	unsigned long cum=0;
	int i;
	int div=0;
	int limit=0;

	struct timeval current_tv;
	do_gettimeofday(&current_tv);
	elapsed=aft_calc_elapsed(&drv_timing->timing_tv,&current_tv);

	if (elapsed > drv_timing->max_latency) {
		drv_timing->max_latency=elapsed;
	}
	if (drv_timing->min_latency == 0 || drv_timing->min_latency > elapsed) {
		drv_timing->min_latency=elapsed;
	}
	drv_timing->latency=elapsed;

	drv_timing->sma[drv_timing->sma_idx] = elapsed;
	drv_timing->sma_idx++;
	if (drv_timing->sma_idx >= MAX_SMA_IDX) {
		drv_timing->sma_idx=0;
	}

	for (i=0;i<MAX_SMA_IDX;i++) {
		if (drv_timing->sma[i] == 0) {
			continue;
		}
		cum+=drv_timing->sma[i];
		div++;
	}

	if (div) {
		cum=cum/div;
	}

	drv_timing->latency_avg = cum;

	if (drv_timing->latency_avg > 19) {
		limit=20;
	} else if (drv_timing->latency_avg > 9) {
		limit=10;
	} else if (drv_timing->latency_avg < 1) {
		limit=5;
	}

	drv_timing->limit=limit;

	if (elapsed > drv_timing->latency_avg + limit) {
		drv_timing->above_avg++;	
	}
	if (drv_timing->latency_avg >  limit) {
		if (drv_timing->latency_avg - limit > elapsed) {
			drv_timing->below_avg++;
		}
	}

	return 0;
}


#if 0
void 	wp_tdmv_api_chan_rx_tx(sdla_t *card,
			      private_area_t *chan,
			      unsigned char *rxdata, unsigned char *tx_data);
void 	wp_tdmv_api_rx_tx (sdla_t *card, private_area_t *chan);
#endif

#if 0
void 	aft_list_descriptors(private_area_t *chan);
#endif
#if 0
void	aft_list_dma_chain_regs(sdla_t *card);
#endif

#if 0
void 	aft_list_tx_descriptors(private_area_t *chan);
#endif
#if 0
void 	aft_display_chain_history(private_area_t *chan);
void 	aft_chain_history(private_area_t *chan,u8 end, u8 cur, u8 begin, u8 loc);
#endif 

#endif

