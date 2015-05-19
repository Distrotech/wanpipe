/*****************************************************************************
* lib_api.h		Sangoma Command Line Arguments parsing library
*
* Author(s):	Nenad Corbic	<ncorbic@sangoma.com>
*				David Rokhvarg	<davidr@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/
#include "libsangoma.h"

extern char	read_enable;
extern char write_enable;
extern char primary_enable;
extern int 	tx_cnt;
extern int 	rx_cnt;
extern int	tx_size;
extern int 	tx_delay;
extern int 	tx_data;
extern int	files_used;
extern int	verbose;
extern int	ds_prot;
extern int	ds_prot_opt;
extern int	ds_max_mult_cnt;
extern unsigned int ds_active_ch;
extern int 	ds_7bit_hdlc;
extern int 	direction;

extern int	tx_channels;
extern int	diagn;
extern int	cause;

extern int	wanpipe_port_no;
extern int	wanpipe_if_no;

extern int	dtmf_enable_octasic;
extern int	dtmf_enable_remora;
extern int	remora_hook;
extern int	usr_period;
extern int	set_codec_slinear;
extern int	set_codec_none;
extern int	rbs_events;
extern int	rx2tx;
extern int  flush_period;
extern int  stats_period;
extern int  hdlc_repeat;
extern int  ss7_cfg_status;
extern int  buffer_multiplier;
extern int  fe_read_test;
extern int  hw_pci_rescan;
extern int 	force_open;

extern int		fe_read_cmd;
extern uint32_t fe_read_reg;
extern int		fe_write_cmd;
extern uint32_t fe_write_reg;
extern uint32_t fe_write_data;
extern int		stats_test;
extern int		flush_stats_test;

extern float 	rx_gain;
extern float 	tx_gain;
extern int rx_gain_cmd;
extern int tx_gain_cmd;

extern unsigned char tx_file[WAN_IFNAME_SZ];
extern unsigned char rx_file[WAN_IFNAME_SZ];

#define TX_ADDR_STR_SZ  100

extern unsigned char daddr[TX_ADDR_STR_SZ];
extern unsigned char saddr[TX_ADDR_STR_SZ];
extern unsigned char udata[TX_ADDR_STR_SZ];


#define	TX_FILE_USED	1
#define RX_FILE_USED	2


extern int init_args(int argc, char *argv[]);

extern void usage(char *api_name);
extern void u_delay(int usec);

