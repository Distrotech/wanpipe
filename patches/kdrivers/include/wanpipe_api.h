/******************************************************************************//**
 * \file wanpipe_api.h
 * \brief WANPIPE(tm) Driver API - Provides FULL Wanpipe Driver API Support
 * \brief Full API includes: Management/Configuration/IO/Events 
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

#ifndef _AFT_CORE_API_H__
#define _AFT_CORE_API_H__

#include "aft_core_user.h"
#include "wanpipe_api_hdr.h"
#include "wanpipe_api_iface.h"
#include "wanpipe_codec_iface.h"

/***************************************************************//**
 * WANPIPE API HEADER & EVENT STRUCTURE
 * Common Unified API Header for all Protocols
 *
 * (This file is also included at the top. This include
 *  has been added so its easier to follow the api header)
 *****************************************************************/
#include "wanpipe_api_hdr.h"


/****************************************************************//**
 * WANPIPE LEGACY API
 *******************************************************************/
#include "wanpipe_api_legacy.h"


/*================================================================
 * UDP API Structure
 *================================================================*/

#define GLOBAL_UDP_SIGNATURE		"WANPIPE"
#define GLOBAL_UDP_SIGNATURE_LEN	7
#define UDPMGMT_UDP_PROTOCOL 		0x11

#pragma pack(1)

/*!
  \struct wan_mgmt
  \brief Wanpipe UDP Management Structure

  \typedef wan_mgmt_t
*/
typedef struct wan_mgmt{
	unsigned char	signature[8];
	unsigned char	request_reply;
	unsigned char	id;
	unsigned char	reserved[6];
} wan_mgmt_t;

/*!
  \struct wan_udp_hdr
  \brief Wanpipe UDP Structure used for Maintenance and Debugging

  This structure is used in conjunction with WANPIPE_IOCTL_PIPEMON ioctl.
  Commands used with this structure are defined in enum WANPIPE_IOCTL_PIPEMON_CMDS

  \typedef wan_udp_hdr_t
*/
typedef struct wan_udp_hdr{
	wan_mgmt_t	wan_mgmt;		/*!< Used by legacy S514 code, not used for AFT */
	wan_cmd_t	wan_cmd;		/*!< Stores command/result/data size  */
	union {
		struct {
			wan_trace_info_t	trace_info;					/*!< Used to pass data trace information */
			unsigned char		data[WAN_MAX_DATA_SIZE];	/*!< Data/Result buffer */
		} chdlc, adsl, atm, ss7,bitstrm,aft;
#define xilinx aft
		//unsigned char data[WAN_MAX_DATA_SIZE];
	} wan_udphdr_u;
#define wan_udphdr_signature			wan_mgmt.signature
#define wan_udphdr_request_reply		wan_mgmt.request_reply
#define wan_udphdr_id					wan_mgmt.id
#define wan_udphdr_opp_flag				wan_cmd.wan_cmd_opp_flag
#define wan_udphdr_command				wan_cmd.wan_cmd_command
#define wan_udphdr_data_len				wan_cmd.wan_cmd_data_len
#define wan_udphdr_return_code			wan_cmd.wan_cmd_return_code
#define wan_udphdr_fe_force				wan_cmd.wan_cmd_fe_force
#define wan_udphdr_hdlc_PF_bit			wan_cmd.wan_cmd_hdlc_PF_bit
#define wan_udphdr_fr_dlci				wan_cmd.wan_cmd_fr_dlci
#define wan_udphdr_fr_attr				wan_cmd.wan_cmd_fr_attr
#define wan_udphdr_fr_rxlost1			wan_cmd.wan_cmd_fr_rxlost1
#define wan_udphdr_fr_rxlost2			wan_cmd.wan_cmd_fr_rxlost2
#define wan_udphdr_x25_pf				wan_cmd.wan_cmd_x25_pf
#define wan_udphdr_x25_lcn				wan_cmd.wan_cmd_x25_lcn
#define wan_udphdr_x25_qdm				wan_cmd.wan_cmd_x25_qdm
#define wan_udphdr_x25_cause			wan_cmd.wan_cmd_x25_cause
#define wan_udphdr_x25_diagn			wan_cmd.wan_cmd_x25_diagn
#define wan_udphdr_x25_pktType			wan_cmd.wan_cmd_x25_pktType
#define wan_udphdr_bscstrm_misc_bits		wan_cmd.wan_cmd_bscstrm_misc_bits
#define wan_udphdr_bscstrm_Rx_err_bits		wan_cmd.wan_cmd_bscstrm_Rx_err_bits
#define wan_udphdr_bscstrm_Rx_time_stamp	wan_cmd.wan_cmd_bscstrm_Rx_time_stamp
#define wan_udphdr_bscstrm_port				wan_cmd.wan_cmd_bscstrm_port
#define wan_udphdr_bsc_misc_bits			wan_cmd.wan_cmd_bsc_misc_bit
#define wan_udphdr_bsc_misc_heading_len		wan_cmd.wan_cmd_bsc_misc_heading_len
#define wan_udphdr_bsc_misc_notify			wan_cmd.wan_cmd_bsc_misc_notify
#define wan_udphdr_bsc_misc_station			wan_cmd.wan_cmd_bsc_misc_station
#define wan_udphdr_bsc_misc_poll_add		wan_cmd.wan_cmd_bsc_misc_poll_addr
#define wan_udphdr_bsc_misc_select_addr		wan_cmd.wan_cmd_bsc_misc_select_addr
#define wan_udphdr_bsc_misc_device_addr		wan_cmd.wan_cmd_bsc_misc_device_addr
#define wan_udphdr_chdlc_num_frames		wan_udphdr_u.chdlc.trace_info.num_frames
#define wan_udphdr_chdlc_ismoredata		wan_udphdr_u.chdlc.trace_info.ismoredata
#define wan_udphdr_chdlc_data			wan_udphdr_u.chdlc.data

#define wan_udphdr_bitstrm_num_frames	wan_udphdr_u.bitstrm.trace_info.num_frames
#define wan_udphdr_bitstrm_ismoredata	wan_udphdr_u.bitstrm.trace_info.ismoredata
#define wan_udphdr_bitstrm_data			wan_udphdr_u.bitstrm.data

#define wan_udphdr_adsl_num_frames		wan_udphdr_u.adsl.trace_info.num_frames
#define wan_udphdr_adsl_ismoredata		wan_udphdr_u.adsl.trace_info.ismoredata
#define wan_udphdr_adsl_data			wan_udphdr_u.adsl.data
#define wan_udphdr_atm_num_frames		wan_udphdr_u.atm.trace_info.num_frames
#define wan_udphdr_atm_ismoredata		wan_udphdr_u.atm.trace_info.ismoredata
#define wan_udphdr_atm_data				wan_udphdr_u.atm.data
#define wan_udphdr_ss7_num_frames		wan_udphdr_u.ss7.trace_info.num_frames
#define wan_udphdr_ss7_ismoredata		wan_udphdr_u.ss7.trace_info.ismoredata
#define wan_udphdr_ss7_data				wan_udphdr_u.ss7.data

#define wan_udphdr_aft_trace_info		wan_udphdr_u.aft.trace_info
#define wan_udphdr_aft_num_frames		wan_udphdr_u.aft.trace_info.num_frames
#define wan_udphdr_aft_ismoredata		wan_udphdr_u.aft.trace_info.ismoredata
#define wan_udphdr_aft_data				wan_udphdr_u.aft.data
#define wan_udphdr_data					wan_udphdr_aft_data

} wan_udp_hdr_t;



/*================================================================
 * KERNEL UDP API Structure
 *================================================================*/


#if defined(WAN_KERNEL)

typedef struct wan_udp_pkt {
	iphdr_t		ip_hdr;
	udphdr_t	udp_hdr;
	wan_udp_hdr_t	wan_udp_hdr;

#define wan_ip				ip_hdr
#define wan_ip_v			ip_hdr.w_ip_v
#define wan_ip_hl			ip_hdr.w_ip_hl
#define wan_ip_tos			ip_hdr.w_ip_tos
#define wan_ip_len			ip_hdr.w_ip_len
#define wan_ip_id			ip_hdr.w_ip_id
#define wan_ip_off			ip_hdr.w_ip_off
#define wan_ip_ttl			ip_hdr.w_ip_ttl
#define wan_ip_p			ip_hdr.w_ip_p
#define wan_ip_sum			ip_hdr.w_ip_sum
#define wan_ip_src			ip_hdr.w_ip_src
#define wan_ip_dst			ip_hdr.w_ip_dst
#define wan_udp_sport			udp_hdr.w_udp_sport
#define wan_udp_dport			udp_hdr.w_udp_dport
#define wan_udp_len			udp_hdr.w_udp_len
#define wan_udp_sum			udp_hdr.w_udp_sum
#define wan_udp_cmd			wan_udp_hdr.wan_cmd
#define wan_udp_signature		wan_udp_hdr.wan_udphdr_signature
#define wan_udp_request_reply		wan_udp_hdr.wan_udphdr_request_reply
#define wan_udp_id			wan_udp_hdr.wan_udphdr_id
#define wan_udp_opp_flag		wan_udp_hdr.wan_udphdr_opp_flag
#define wan_udp_command			wan_udp_hdr.wan_udphdr_command
#define wan_udp_data_len		wan_udp_hdr.wan_udphdr_data_len
#define wan_udp_return_code		wan_udp_hdr.wan_udphdr_return_code
#define wan_udp_hdlc_PF_bit 		wan_udp_hdr.wan_udphdr_hdlc_PF_bit
#define wan_udp_fr_dlci 		wan_udp_hdr.wan_udphdr_fr_dlci
#define wan_udp_fr_attr 		wan_udp_hdr.wan_udphdr_fr_attr
#define wan_udp_fr_rxlost1 		wan_udp_hdr.wan_udphdr_fr_rxlost1
#define wan_udp_fr_rxlost2 		wan_udp_hdr.wan_udphdr_fr_rxlost2
#define wan_udp_x25_pf 			wan_udp_hdr.wan_udphdr_x25_pf
#define wan_udp_x25_lcn 		wan_udp_hdr.wan_udphdr_x25_lcn
#define wan_udp_x25_qdm 		wan_udp_hdr.wan_udphdr_x25_qdm
#define wan_udp_x25_cause 		wan_udp_hdr.wan_udphdr_x25_cause
#define wan_udp_x25_diagn 		wan_udp_hdr.wan_udphdr_x25_diagn
#define wan_udp_x25_pktType 		wan_udp_hdr.wan_udphdr_x25_pktType
#define wan_udp_bscstrm_misc_bits 	wan_udp_hdr.wan_udphdr_bscstrm_misc_bits
#define wan_udp_bscstrm_Rx_err_bits 	wan_udp_hdr.wan_udphdr_bscstrm_Rx_err_bits
#define wan_udp_bscstrm_Rx_time_stam 	wan_udp_hdr.wan_udphdr_bscstrm_Rx_time_stamp
#define wan_udp_bscstrm_port 		wan_udp_hdr.wan_udphdr_bscstrm_port
#define wan_udp_bsc_misc_bits 		wan_udp_hdr.wan_udphdr_bsc_misc_bits
#define wan_udp_bsc_misc_heading_len  	wan_udp_hdr.wan_udphdr_bsc_misc_heading_len
#define wan_udp_bsc_misc_notify 	wan_udp_hdr.wan_udphdr_bsc_misc_notify
#define wan_udp_bsc_misc_station 	wan_udp_hdr.wan_udphdr_bsc_misc_station
#define wan_udp_bsc_misc_poll_add 	wan_udp_hdr.wan_udphdr_bsc_misc_poll_add
#define wan_udp_bsc_misc_select_addr 	wan_udp_hdr.wan_udphdr_bsc_misc_select_addr
#define wan_udp_bsc_misc_device_addr 	wan_udp_hdr.wan_udphdr_bsc_misc_device_addr
#define wan_udp_bsc_misc_notify_ext 	wan_udp_hdr.wan_udphdr_bsc_misc_notify_ext
#define wan_udp_chdlc_num_frames	wan_udp_hdr.wan_udphdr_chdlc_num_frames
#define wan_udp_chdlc_ismoredata	wan_udp_hdr.wan_udphdr_chdlc_ismoredata
#define wan_udp_chdlc_data		wan_udp_hdr.wan_udphdr_chdlc_data

#define wan_udp_bitstrm_num_frames	wan_udp_hdr.wan_udphdr_bitstrm_num_frames
#define wan_udp_bitstrm_ismoredata	wan_udp_hdr.wan_udphdr_bitstrm_ismoredata
#define wan_udp_bitstrm_data		wan_udp_hdr.wan_udphdr_bitstrm_data

#define wan_udp_adsl_num_frames		wan_udp_hdr.wan_udphdr_adsl_num_frames
#define wan_udp_adsl_ismoredata		wan_udp_hdr.wan_udphdr_adsl_ismoredata
#define wan_udp_adsl_data		wan_udp_hdr.wan_udphdr_adsl_data
#define wan_udp_atm_num_frames		wan_udp_hdr.wan_udphdr_atm_num_frames
#define wan_udp_atm_ismoredata		wan_udp_hdr.wan_udphdr_atm_ismoredata
#define wan_udp_atm_data		wan_udp_hdr.wan_udphdr_atm_data
#define wan_udp_ss7_num_frames		wan_udp_hdr.wan_udphdr_ss7_num_frames
#define wan_udp_ss7_ismoredata		wan_udp_hdr.wan_udphdr_ss7_ismoredata
#define wan_udp_ss7_data		wan_udp_hdr.wan_udphdr_ss7_data

#define wan_udp_aft_trace_info		wan_udp_hdr.wan_udphdr_aft_trace_info
#define wan_udp_aft_num_frames		wan_udp_hdr.wan_udphdr_aft_num_frames
#define wan_udp_aft_ismoredata		wan_udp_hdr.wan_udphdr_aft_ismoredata
#define wan_udp_data				wan_udp_hdr.wan_udphdr_data
} wan_udp_pkt_t;

#endif

#pragma pack()

/*!
  \enum wanpipe_aft_devel_events
  \brief Wanpipe Commands associated with WANPIPE_IOCTL_DEVEL Ioctl

   The Devel Commands are used to debug the driver and firmware.
   Should only be used during debugging.
 */
enum wanpipe_aft_devel_events {
	SIOC_WAN_READ_REG = 0x01,
	SIOC_WAN_WRITE_REG,
	SIOC_WAN_HWPROBE,
	SIOC_WAN_ALL_HWPROBE,
	SIOC_WAN_ALL_READ_REG,
	SIOC_WAN_ALL_WRITE_REG,
	SIOC_WAN_ALL_SET_PCI_BIOS,
	SIOC_WAN_SET_PCI_BIOS,
	SIOC_WAN_COREREV,
	SIOC_WAN_GET_CFG,
	SIOC_WAN_FE_READ_REG,
	SIOC_WAN_FE_WRITE_REG,
	SIOC_WAN_EC_REG,
	SIOC_WAN_READ_PCIBRIDGE_REG,
	SIOC_WAN_ALL_READ_PCIBRIDGE_REG,
	SIOC_WAN_WRITE_PCIBRIDGE_REG,
	SIOC_WAN_ALL_WRITE_PCIBRIDGE_REG,
	SIOC_WAN_GET_CARD_TYPE,
	SIOC_WAN_USB_READ_REG,
	SIOC_WAN_USB_WRITE_REG,
	SIOC_WAN_USB_CPU_WRITE_REG,
	SIOC_WAN_USB_CPU_READ_REG,
	SIOC_WAN_USB_FE_WRITE_REG,
	SIOC_WAN_USB_FE_READ_REG,
	SIOC_WAN_USB_FW_DATA_READ,
	SIOC_WAN_USB_FW_DATA_WRITE,
	SIOC_WAN_USB_FWUPDATE_ENABLE,
};

#include "wanpipe_api_deprecated.h"

#endif

