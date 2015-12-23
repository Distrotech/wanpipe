/******************************************************************************//**
 * \file wanpipe_api_legacy.h
 * \brief WANPIPE(tm) Driver Legacy API
 *
 * Authors: Nenad Corbic <ncorbic@sangoma.com>
 *			David Rokhvarg <davidr@sangoma.com>
 *			Alex Feldman <alex@sangoma.com>
 *
 * Copyright (c) 2007 - 08, Sangoma Technologies
 * All rights reserved.
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sangoma Technologies nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ===============================================================================
 */

#ifndef __WANPIPE_API_LEGACY__
#define __WANPIPE_API_LEGACY__

#pragma pack(1)


/****************************************************************//**
 * WANPIPE LEGACY API Structure
 *******************************************************************/


#define WAN_API_MAX_DATA	4096
typedef struct{
	unsigned char	pktType;
	unsigned short	length;
	unsigned char	result;
	union {
		struct {
			unsigned char	arg1;
			unsigned short	time_stamp;
		} chdlc;
		struct {
	        	unsigned char   attr;
	        	unsigned short  time_stamp;
		} fr;
		struct {
			unsigned char	qdm;
			unsigned char	cause;
			unsigned char	diagn;
			unsigned short	lcn;
		} x25;
		struct {
			unsigned char  station;
			unsigned char  PF_bit;
			unsigned short poll_interval;
			unsigned char  general_mailbox_byte;
		}sdlc;
		struct {
			unsigned char  exception;
		}xdlc;
	} wan_protocol;
#define wan_apihdr_chdlc_error_flag	wan_protocol.chdlc.arg1
#define wan_apihdr_chdlc_attr		wan_protocol.chdlc.arg1
#define wan_apihdr_chdlc_time_stamp	wan_protocol.chdlc.time_stamp
#define wan_apihdr_fr_attr		wan_protocol.fr.attr
#define wan_apihdr_fr_time_stamp	wan_protocol.fr.time_stamp
#define wan_apihdr_x25_qdm		wan_protocol.x25.qdm
#define wan_apihdr_x25_cause		wan_protocol.x25.cause
#define wan_apihdr_x25_diagn		wan_protocol.x25.diagn
#define wan_apihdr_x25_lcn		wan_protocol.x25.lcn

#define wan_apihdr_sdlc_station		wan_protocol.sdlc.station
#define wan_apihdr_sdlc_pf		wan_protocol.sdlc.PF_bit
#define wan_apihdr_sdlc_poll_interval	wan_protocol.sdlc.poll_interval
#define wan_apihdr_sdlc_general_mb_byte	wan_protocol.sdlc.general_mailbox_byte

#define wan_apihdr_xdlc_exception	wan_protocol.xdlc.exception
} wan_api_hdr_t;

typedef struct{
	wan_api_hdr_t	api_hdr;
	unsigned char	data[WAN_API_MAX_DATA];
#define wan_api_pktType			api_hdr.pktType
#define wan_api_length			api_hdr.length
#define wan_api_result			api_hdr.result
#define wan_api_chdlc_error_flag	api_hdr.wan_apihdr_chdlc_error_flag
#define wan_api_chdlc_time_stamp	api_hdr.wan_apihdr_chdlc_time_stamp
#define wan_api_chdlc_attr		api_hdr.wan_apihdr_chdlc_attr
#define wan_api_chdlc_misc_Tx_bits	api_hdr.wan_apihdr_chdlc_misc_Tx_bits
#define wan_api_fr_attr			api_hdr.wan_apihdr_fr_attr
#define wan_api_fr_time_stamp		api_hdr.wan_apihdr_fr_time_stamp
#define wan_api_x25_qdm			api_hdr.wan_apihdr_x25_qdm
#define wan_api_x25_cause		api_hdr.wan_apihdr_x25_cause
#define wan_api_x25_diagn		api_hdr.wan_apihdr_x25_diagn
#define wan_api_x25_lcn			api_hdr.wan_apihdr_x25_lcn
#define	wan_api_sdlc_station		api_hdr.wan_apihdr_sdlc_station
#define wan_api_sdlc_pf			api_hdr.wan_apihdr_sdlc_pf
#define wan_api_sdlc_poll_interval	api_hdr.wan_apihdr_sdlc_poll_interval
#define wan_api_sdlc_general_mb_byte	api_hdr.wan_apihdr_sdlc_general_mb_byte
#define wan_api_xdlc_exception		api_hdr.wan_apihdr_xdlc_exception
} wan_api_t;


#define WAN_MAX_DATA_SIZE	5000
#define MAX_LGTH_UDP_MGNT_PKT WAN_MAX_DATA_SIZE

#define WAN_MAILBOX_SIZE	16
#define WAN_MAX_POS_DATA_SIZE	1030
#define WAN_MAX_MBOX_DATA_SIZE	2032

#define WAN_MAX_CMD_DATA_SIZE	990

/*!
  \struct wan_cmd_api_
  \brief Wanpipe UDP CMD Structure

  \typedef wan_cmd_api_t
*/
typedef struct wan_cmd_api_
{
	unsigned int	cmd;
	unsigned short	len;
	unsigned char	bar;
	unsigned int	offset;
	int 		ret; 		/* return value */
	unsigned char	data[WAN_MAX_CMD_DATA_SIZE];
} wan_cmd_api_t;




/*!
  \struct wan_cmd
  \brief Wanpipe Legacy Command Structure

  This structure is used as part of the wan_udp_hdr_t strcutre.
  Its used to hold the command and return return_code

  \typedef wan_cmd_t
*/
typedef struct wan_cmd {
	union {
		struct {
			unsigned char  opp_flag;		/*!< Used by legacy S514 cards as cmd complete flag */
			unsigned char  command;			/*!< Wanpipemon Command: enum WANPIPE_IOCTL_PIPEMON_CMDS */
			unsigned short data_len;		/*!< Length of the data/result buffer */
			unsigned char  return_code;		/*!< Result  0=OK  otherwise Error */
			union {
				struct {
					unsigned char	PF_bit;		/* the HDLC P/F bit */
				} hdlc;
				struct {
					unsigned short	dlci;		/* DLCI number */
					unsigned char	attr;		/* FECN, BECN, DE and C/R bits */
					unsigned short	rxlost1;	/* frames discarded at int. level */
					unsigned int	rxlost2;	/* frames discarded at app. level */
				} fr;
				struct {
					unsigned char	pf;			/* P/F bit */
					unsigned short	lcn;		/* logical channel */
					unsigned char	qdm;		/* Q/D/M bits */
					unsigned char	cause;		/* cause field */
					unsigned char	diagn;		/* diagnostics */
					unsigned char	pktType;	/* packet type */
				} x25;
				struct {
					unsigned char	misc_Tx_Rx_bits; /* miscellaneous transmit and receive bits */
					unsigned char	Rx_error_bits; /* an indication of a block received with an error */
					unsigned short	Rx_time_stamp; /* a millisecond receive time stamp */
					unsigned char	port;		/* comm port */
				} bscstrm;
				struct {
					unsigned char 	misc_tx_rx_bits;
					unsigned short 	heading_length;
					unsigned short 	notify;
					unsigned char 	station;
					unsigned char 	poll_address;
					unsigned char 	select_address;
					unsigned char 	device_address;
					unsigned char 	notify_extended;
				} bsc;
				struct {
					unsigned char	sdlc_address;
					unsigned char	PF_bit;
					unsigned short	poll_interval;
					unsigned char	general_mailbox_byte;
				} sdlc;
				struct {
					unsigned char	force;
				} fe;
			} wan_protocol;
		} wan_p_cmd;
		struct {
			unsigned char opp_flag;
			unsigned char pos_state;
			unsigned char async_state;
		} wan_pos_cmd;
		unsigned char mbox[WAN_MAILBOX_SIZE];
	} wan_cmd_u;
#define wan_cmd_opp_flag		wan_cmd_u.wan_p_cmd.opp_flag
#define wan_cmd_command			wan_cmd_u.wan_p_cmd.command
#define wan_cmd_data_len		wan_cmd_u.wan_p_cmd.data_len
#define wan_cmd_return_code		wan_cmd_u.wan_p_cmd.return_code
#define wan_cmd_hdlc_PF_bit		wan_cmd_u.wan_p_cmd.wan_protocol.hdlc.PF_bit
#define wan_cmd_fe_force		wan_cmd_u.wan_p_cmd.wan_protocol.fe.force
#define wan_cmd_fr_dlci			wan_cmd_u.wan_p_cmd.wan_protocol.fr.dlci
#define wan_cmd_fr_attr			wan_cmd_u.wan_p_cmd.wan_protocol.fr.attr
#define wan_cmd_fr_rxlost1		wan_cmd_u.wan_p_cmd.wan_protocol.fr.rxlost1
#define wan_cmd_fr_rxlost2		wan_cmd_u.wan_p_cmd.wan_protocol.fr.rxlost2
#define wan_cmd_x25_pf			wan_cmd_u.wan_p_cmd.wan_protocol.x25.pf
#define wan_cmd_x25_lcn			wan_cmd_u.wan_p_cmd.wan_protocol.x25.lcn
#define wan_cmd_x25_qdm			wan_cmd_u.wan_p_cmd.wan_protocol.x25.qdm
#define wan_cmd_x25_cause		wan_cmd_u.wan_p_cmd.wan_protocol.x25.cause
#define wan_cmd_x25_diagn		wan_cmd_u.wan_p_cmd.wan_protocol.x25.diagn
#define wan_cmd_x25_pktType		wan_cmd_u.wan_p_cmd.wan_protocol.x25.pktType
#define wan_cmd_bscstrm_misc_bits	wan_cmd_u.wan_p_cmd.wan_protocol.bscstrm.misc_Tx_Rx_bits
#define wan_cmd_bscstrm_Rx_err_bits	wan_cmd_u.wan_p_cmd.wan_protocol.bscstrm.Rx_error_bits
#define wan_cmd_bscstrm_Rx_time_stamp	wan_cmd_u.wan_p_cmd.wan_protocol.bscstrm.Rx_time_stamp
#define wan_cmd_bscstrm_port		wan_cmd_u.wan_p_cmd.wan_protocol.bscstrm.port
#define wan_cmd_bsc_misc_bits		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.misc_tx_rx_bits
#define wan_cmd_bsc_heading_len		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.heading_length
#define wan_cmd_bsc_notify		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.notify
#define wan_cmd_bsc_station		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.station
#define wan_cmd_bsc_poll_addr		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.poll_address
#define wan_cmd_bsc_select_addr		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.select_address
#define wan_cmd_bsc_device_addr		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.device_address
#define wan_cmd_bsc_notify_ext		wan_cmd_u.wan_p_cmd.wan_protocol.bsc.notify_extended
#define wan_cmd_sdlc_address		wan_cmd_u.wan_p_cmd.wan_protocol.sdlc.sdlc_address
#define wan_cmd_sdlc_pf			wan_cmd_u.wan_p_cmd.wan_protocol.sdlc.PF_bit
#define wan_cmd_sdlc_poll_interval	wan_cmd_u.wan_p_cmd.wan_protocol.sdlc.poll_interval
#define wan_cmd_sdlc_general_mb_byte	wan_cmd_u.wan_p_cmd.wan_protocol.sdlc.general_mailbox_byte

#define wan_cmd_pos_opp_flag		wan_cmd_u.wan_pos_cmd.opp_flag
#define wan_cmd_pos_pos_state		wan_cmd_u.wan_pos_cmd.pos_state
#define wan_cmd_pos_async_state		wan_cmd_u.wan_pos_cmd.async_state
} wan_cmd_t;


typedef struct {
	wan_cmd_t	wan_cmd;
	union {
		struct {
			unsigned char  command;
			unsigned short data_len;
			unsigned char  return_code;
			unsigned char  port_num;
			unsigned char  attr;
			unsigned char  reserved[10];
			unsigned char  data[WAN_MAX_POS_DATA_SIZE];
		} pos_data;
		unsigned char data[WAN_MAX_MBOX_DATA_SIZE];
	} wan_u_data;
#define wan_opp_flag			wan_cmd.wan_cmd_opp_flag
#define wan_command			wan_cmd.wan_cmd_command
#define wan_data_len			wan_cmd.wan_cmd_data_len
#define wan_return_code			wan_cmd.wan_cmd_return_code
#define wan_hdlc_PF_bit			wan_cmd.wan_cmd_hdlc_PF_bit
#define wan_fr_dlci			wan_cmd.wan_cmd_fr_dlci
#define wan_fr_attr			wan_cmd.wan_cmd_fr_attr
#define wan_fr_rxlost1			wan_cmd.wan_cmd_fr_rxlost1
#define wan_fr_rxlost2			wan_cmd.wan_cmd_fr_rxlost2
#define wan_x25_pf			wan_cmd.wan_cmd_x25_pf
#define wan_x25_lcn			wan_cmd.wan_cmd_x25_lcn
#define wan_x25_qdm			wan_cmd.wan_cmd_x25_qdm
#define wan_x25_cause			wan_cmd.wan_cmd_x25_cause
#define wan_x25_diagn			wan_cmd.wan_cmd_x25_diagn
#define wan_x25_pktType			wan_cmd.wan_cmd_x25_pktType
#define wan_bscstrm_misc_bits		wan_cmd.wan_cmd_bscstrm_misc_bits
#define wan_bscstrm_Rx_err_bits		wan_cmd.wan_cmd_bscstrm_Rx_error_bits
#define wan_bscstrm_Rx_time_stamp	wan_cmd.wan_cmd_bscstrm_Rx_time_stamp
#define wan_bscstrm_port		wan_cmd.wan_cmd_bscstrm_port
#define wan_bsc_misc_bits		wan_cmd.wan_cmd_bsc_misc_bits
#define wan_bsc_heading_len		wan_cmd.wan_cmd_bsc_heading_length
#define wan_bsc_notify			wan_cmd.wan_cmd_bsc_notify
#define wan_bsc_station			wan_cmd.wan_cmd_bsc_station
#define wan_bsc_poll_addr		wan_cmd.wan_cmd_bsc_poll_address
#define wan_bsc_select_addr		wan_cmd.wan_cmd_bsc_select_address
#define wan_bsc_device_addr		wan_cmd.wan_cmd_bsc_device_address
#define wan_bsc_notify_ext		wan_cmd.wan_cmd_bsc_notify_extended
#define wan_sdlc_address		wan_cmd.wan_cmd_sdlc_address
#define wan_sdlc_pf			wan_cmd.wan_cmd_sdlc_pf
#define wan_sdlc_poll_interval		wan_cmd.wan_cmd_sdlc_poll_interval
#define wan_sdlc_general_mb_byte	wan_cmd.wan_cmd_sdlc_general_mb_byte
#define wan_data			wan_u_data.data

#define wan_pos_opp_flag		wan_cmd.wan_cmd_pos_opp_flag
#define wan_pos_pos_state		wan_cmd.wan_cmd_pos_pos_state
#define wan_pos_async_state		wan_cmd.wan_cmd_pos_async_state
#define wan_pos_command			wan_u_data.pos_data.command
#define wan_pos_data_len		wan_u_data.pos_data.data_len
#define wan_pos_return_code		wan_u_data.pos_data.return_code
#define wan_pos_port_num		wan_u_data.pos_data.port_num
#define wan_pos_attr			wan_u_data.pos_data.attr
#define wan_pos_data			wan_u_data.pos_data.data
} wan_mbox_t;

#define WAN_MBOX_INIT(mbox)	memset(mbox, 0, sizeof(wan_cmd_t));


#pragma pack()

#endif /* __WANPIPE_API_LEGACY__ */
