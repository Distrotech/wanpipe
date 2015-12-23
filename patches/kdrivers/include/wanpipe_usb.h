/*****************************************************************************
* wanpipe_usb.h
*	
*		WANPIPE(tm) USB Interface Support
* 		
* Authors: 	Alex Feldman <alex@sangoma.com>
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Apr 01, 2008	Alex Feldman	Initial version.
*****************************************************************************/


#ifndef __WANPIPE_USB_H
# define __WANPIPE_USB_H

#if 0
enum {
	ROUTER_UP_TIME = 0x50,
	READ_CODE_VERSION,
	READ_CONFIGURATION,
	READ_COMMS_ERROR_STATS,
	FLUSH_COMMS_ERROR_STATS,
	READ_OPERATIONAL_STATS,
	FLUSH_OPERATIONAL_STATS
};
#endif

/* this structure is a mirror from sdladrv.h */
typedef struct {

	int		core_notready_cnt;

	int		cmd_overrun;
	int		cmd_timeout;
	int		cmd_invalid;

	int		rx_sync_err_cnt;
	int		rx_start_fr_err_cnt;
	int		rx_start_err_cnt;
	int		rx_cmd_reset_cnt;
	int		rx_cmd_drop_cnt;
	int		rx_cmd_unknown;
	int		rx_overrun_cnt;
	int		rx_underrun_cnt;
	int		tx_overrun_cnt;
	int		tx_notready_cnt;

	unsigned char	fifo_status;
	unsigned char	uart_status;
	unsigned char	hostif_status;

} wp_usb_comm_err_stats_t;

typedef struct {
	
	int		isr_no;

} wp_usb_op_stats_t;


#if !defined(__WINDOWS__)/* use 'wan_udphdr_aft_data'! */
#undef  wan_udphdr_data
#define wan_udphdr_data	wan_udphdr_u.aft.data
#endif


#ifdef WAN_KERNEL



#endif


#endif /* __WANPIPE_USB_H */
