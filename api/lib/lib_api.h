/*****************************************************************************
* lib_api.h	Common API library
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/


extern char	read_enable;
extern char 	write_enable;
extern char 	primary_enable;
extern int 	tx_cnt;
extern int 	rx_cnt;
extern int	tx_size;
extern int 	tx_delay;
extern int 	tx_data;
extern int	tx_ss7_type;
extern int	rx_ss7_timer;
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
extern int	card_cnt;
extern int	i_cnt;

extern char tx_file[WAN_IFNAME_SZ];
extern char rx_file[WAN_IFNAME_SZ];

#define TX_ADDR_STR_SZ  100

extern char daddr[TX_ADDR_STR_SZ];
extern char saddr[TX_ADDR_STR_SZ];
extern char udata[TX_ADDR_STR_SZ];


#define	TX_FILE_USED	1
#define RX_FILE_USED	2



extern int init_args(int argc, char *argv[]);

extern void usage(char *api_name);
extern void u_delay(int usec);

extern char card_name[WAN_IFNAME_SZ];
extern char if_name[WAN_IFNAME_SZ];

extern char sw_if_name[WAN_IFNAME_SZ];
extern char sw_card_name[WAN_IFNAME_SZ];
