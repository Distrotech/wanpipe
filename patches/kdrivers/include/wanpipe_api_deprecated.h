/******************************************************************************//**
 * \file wanpipe_api_deprecated.h
 * \brief WANPIPE(tm) Driver API Interface -
 * \brief Provies IO/Event API Only
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


#ifndef __WANPIPE_API_DEPRICATED_H_
#define __WANPIPE_API_DEPRICATED_H_


#pragma pack(1)

/***************************************************//**
 DEPRECATED: TX Header Structure
 Here for backward compile compatilbity
 *******************************************************/

/*
 \struct api_tdm_event_hdr_t
 \brief Deprecated: Legacy tx tdm api event
*/
typedef struct {
	u_int8_t	type;
	u_int8_t	mode;
	u_int8_t	tone;
	u_int16_t	channel;
	u_int16_t	polarity;
	u_int16_t	ohttimer;
} api_tdm_event_hdr_t;

/*
 \struct api_tx_ss7_hdr_t
 \brief Deprecated: Legacy ss7 hdr structure
*/
typedef struct {
	unsigned char	type;
	unsigned char	force_tx;
	unsigned char	data[8];
} api_tx_ss7_hdr_t;

/*
 \struct api_tx_hdlc_rpt_hdr_t
 \brief Deprecated: Legacy rtp tap hdr structure
*/
typedef struct {
	unsigned char	repeat;
	unsigned char	len;
	unsigned char	data[8];
} api_tx_hdlc_rpt_hdr_t;


/*
 \struct wan_api_tx_hdr_t
 \brief Deprecated: Legacy tx hdr structure, there is only one hdr type for tx and rx now.

 This structure is here for backward compatibility

*/
typedef struct {
	union{
		struct {
			unsigned char 	attr;
			unsigned char   misc_Tx_bits;
		}chdlc,hdlc,fr;
		struct {
			unsigned char	_rbs_rx_bits;
			unsigned int	_time_stamp;
		}wp_tx;
		struct {
			unsigned char 	sio;
			unsigned short	time_stamp;
		}ss7_legacy;
		struct {
			unsigned char 	pf;
		}lapb;
		struct {
			unsigned char  pf;
		}xdlc;
		struct {
			unsigned char 	station;
			unsigned char   misc_tx_rx_bits;
		}; 
		struct {
			union {
				api_tx_ss7_hdr_t 	ss7;
				api_tx_hdlc_rpt_hdr_t hdlc_rpt;
				api_tdm_event_hdr_t	event;
			}hdr_u;
		}aft;
		struct {
			u_int8_t max_tx_queue_length;		/* set on return from IoctlWriteCommand */
			u_int8_t current_number_of_frames_in_tx_queue;	/* set on return from IoctlWriteCommand */
		}tx_h;
		unsigned char  	reserved[WAN_MAX_HDR_SZ];
	};

#define wp_api_tx_hdr_event_type		aft.hdr_u.event.type
#define wp_api_tx_hdr_event_mode 		aft.hdr_u.event.mode
#define wp_api_tx_hdr_event_tone 		aft.hdr_u.event.tone
#define wp_api_tx_hdr_event_channel 	aft.hdr_u.event.channel
#define wp_api_tx_hdr_event_ohttimer 	aft.hdr_u.event.ohttimer
#define wp_api_tx_hdr_event_polarity 	aft.hdr_u.event.polarity



/* FR Old backdward comptabile */
#define wp_api_tx_hdr_fr_attr			fr.attr
#define wp_api_tx_hdr_fr_misc_tx_bits	fr.misc_Tx_bits

/* CHDLC Old backdward comptabile */
#define wp_api_tx_hdr_chdlc_attr			chdlc.attr
#define wp_api_tx_hdr_chdlc_misc_tx_bits	chdlc.misc_Tx_bits

/* SS7 Legacy Old backdward comptabile */
#define wp_api_tx_hdr_ss7_legacy_sio			ss7_legacy.sio
#define wp_api_tx_hdr_ss7_legacy_time_stamp		ss7_legacy.time_stamp

#define wp_api_tx_hdr_event_serial_status	aft.hdr_u.event.tone

} wan_api_tx_hdr_t;

/* Backward compatible */
#define api_tx_hdr_t wan_api_tx_hdr_t



typedef struct {
	wan_api_tx_hdr_t 	wan_api_tx_hdr;
#if defined(__WINDOWS__)
		/* zero-sized array does not comply to ANSI 'C' standard! */
        unsigned char  		data[1];
#else
        unsigned char  		data[0];
#endif
}wan_api_tx_element_t;

/* Backward compatible */
# define api_tx_element_t wan_api_tx_element_t

#pragma pack()


/***************************************************//**
 DEPRECATED: Backward compatible API Commands
 *******************************************************/

#define wp_tdm_api_rx_hdr_t 	 wp_api_hdr_t
#define wp_tdm_api_rx_element_t  wp_api_element_t
#define wp_tdm_api_tx_hdr_t 	 wp_api_hdr_t
#define wp_tdm_api_tx_element_t  wp_api_element_t

/* Backward compatible IOCTLS */
#if defined (__WINDOWS__)
#define _IOCTL_CODE					WANPIPE_IOCTL_CODE
#define IOCTL_WRITE 				WANPIPE_IOCTL_WRITE
#define IOCTL_READ 					WANPIPE_IOCTL_READ
#define IOCTL_MGMT  				WANPIPE_IOCTL_MGMT
#define IOCTL_SET_IDLE_TX_BUFFER 	WANPIPE_IOCTL_SET_IDLE_TX_BUFFER
#define IOCTL_API_POLL				WANPIPE_IOCTL_API_POLL
#define IOCTL_SET_SHARED_EVENT		WANPIPE_IOCTL_SET_SHARED_EVENT
#define	IOCTL_PORT_MGMT				WANPIPE_IOCTL_PORT_MGMT
#define IOCTL_PORT_CONFIG			WANPIPE_IOCTL_PORT_CONFIG
#define SIOC_WAN_TDMV_API_IOCTL		WANPIPE_IOCTL_TDM_API
#define SIOC_WANPIPE_PIPEMON		WANPIPE_IOCTL_PIPEMON
#define SIOC_WANPIPE_SNMP_IFSPEED	WANPIPE_IOCTL_SNMP_IFSPEED
#define SIOC_WANPIPE_SNMP			WANPIPE_IOCTL_SNMP
#define SIOC_WAN_DEVEL_IOCTL		WANPIPE_IOCTL_DEVEL
#endif

#define WP_TDMAPI_EVENT_FE_ALARM 	WP_TDMAPI_EVENT_ALARM
#define SIOC_WP_TDM_GET_LINK_STATUS SIOC_WP_TDM_GET_FE_STATUS

#define ROUTER_UP_TIME 				WANPIPEMON_ROUTER_UP_TIME
#define ENABLE_TRACING				WANPIPEMON_ENABLE_TRACING
#define DISABLE_TRACING				WANPIPEMON_DISABLE_TRACING
#define GET_TRACE_INFO				WANPIPEMON_GET_TRACE_INFO
#define READ_CODE_VERSION			WANPIPEMON_READ_CODE_VERSION
#define FLUSH_OPERATIONAL_STATS		WANPIPEMON_FLUSH_OPERATIONAL_STATS
#define OPERATIONAL_STATS			WANPIPEMON_OPERATIONAL_STATS
#define READ_OPERATIONAL_STATS		WANPIPEMON_READ_OPERATIONAL_STATS
#define READ_CONFIGURATION			WANPIPEMON_READ_CONFIGURATION
#define READ_COMMS_ERROR_STATS		WANPIPEMON_READ_COMMS_ERROR_STATS
#define FLUSH_COMMS_ERROR_STATS		WANPIPEMON_FLUSH_COMMS_ERROR_STATS
#define AFT_LINK_STATUS				WANPIPEMON_AFT_LINK_STATUS
#define AFT_MODEM_STATUS			WANPIPEMON_AFT_MODEM_STATUS
#define AFT_HWEC_STATUS				WANPIPEMON_AFT_HWEC_STATUS
#define DIGITAL_LOOPTEST			WANPIPEMON_DIGITAL_LOOPTEST
#define SET_FT1_MODE				WANPIPEMON_SET_FT1_MODE
#define WAN_EC_IOCTL				WANPIPEMON_EC_IOCTL
#define WAN_SET_RBS_BITS			WANPIPEMON_SET_RBS_BITS
#define WAN_GET_RBS_BITS			WANPIPEMON_GET_RBS_BITS
#define GET_OPEN_HANDLES_COUNTER	WANPIPEMON_GET_OPEN_HANDLES_COUNTER
#define WAN_GET_HW_MAC_ADDR			WANPIPEMON_GET_HW_MAC_ADDR
#define FLUSH_TX_BUFFERS			WANPIPEMON_FLUSH_TX_BUFFERS
#define WAN_TDMV_API_IOCTL			WANPIPEMON_TDM_API


#define WANPIPE_PROTOCOL_PRIVATE	WANPIPEMON_DRIVER_PRIVATE

#if defined (__WINDOWS__)
/* definitions for compile-compatibility with older source code */
#define SIOC_AFT_CUSTOMER_ID		WANPIPEMON_AFT_CUSTOMER_ID
#endif



/***************************************************//**
 Backward Compatible Defines for wp_api_event_t structre
 *******************************************************/

#define wp_tdm_api_event_type 				wp_api_event_type
#define wp_tdm_api_event_mode 				wp_api_event_mode
#define wp_tdm_api_event_channel 			wp_api_event_channel
#define wp_tdm_api_event_span				wp_api_event_span
#define wp_tdm_api_event_alarm 				wp_api_event_alarm
#define wp_tdm_api_event_rbs_bits 			wp_api_event_rbs_bits
#define wp_tdm_api_event_hook_state 		wp_api_event_hook_state
#define wp_tdm_api_event_hook_sig 			wp_api_event_hook_sig
#define wp_tdm_api_event_ring_state 		wp_api_event_ring_state
#define wp_tdm_api_event_tone_type 			wp_api_event_tone_type
#define wp_tdm_api_event_dtmf_digit 		wp_api_event_dtmf_digit
#define wp_tdm_api_event_dtmf_type 			wp_api_event_dtmf_type
#define wp_tdm_api_event_dtmf_port 			wp_api_event_dtmf_port
#define wp_tdm_api_event_ohttimer 			wp_api_event_ohttimer
#define wp_tdm_api_event_polarity 			wp_api_event_polarity
#define wp_tdm_api_event_link_status		wp_api_event_link_status
#define wp_serial_event_status				wp_api_event_serial_status
#define wp_tdm_api_event_time_stamp_sec		wp_api_event_time_stamp_sec
#define wp_tdm_api_event_time_stamp_usec	wp_api_event_time_stamp_usec





/***************************************************//**
 Backward Compatible Defines 
 *******************************************************/

/* Backward compatible define to wp_api_event_t */
#define wp_tdm_api_event_t wp_api_event_t


#if defined (__WINDOWS__)
#define net_device_stats	wanpipe_chan_stats
#define net_device_stats_t	wanpipe_chan_stats_t
#define OID_WANPIPEMON_IOCTL	19720123 /* used by wanpipe.sys (Sangoma NDIS) driver */
#endif


#define wan_api_rx_hdr_t  	wp_api_hdr_t
#define api_rx_hdr_t		wp_api_hdr_t
#define api_header_t 		wp_api_hdr_t
#define wp_tdm_api_rx_hdr_t	wp_api_hdr_t

#define TX_HDR_STRUCT 		wp_api_hdr_t
#define RX_HDR_STRUCT 		wan_api_rx_hdr_t

#define wan_rxapi_xdlc_state		hdr.wan_hdr_xdlc_state
#define wan_rxapi_xdlc_address		hdr.wan_hdr_xdlc_address
#define wan_rxapi_xdlc_exception	hdr.wan_hdr_xdlc_exception

#define api_header		hdr
#define api_rx_hdr		hdr


#define api_rx_element_t 		wp_api_element_t
#define wan_api_rx_element_t 	wp_api_element_t

#define TX_RX_DATA_STRUCT 		wp_api_element_t
#define TX_DATA_STRUCT 			TX_RX_DATA_STRUCT
#define RX_DATA_STRUCT 			TX_RX_DATA_STRUCT




/***************************************************//**
 DEPRECATED: Backward compatible API Commands
 Deprecated by: 
 *******************************************************/


/*
  \enum wanpipe_tdm_api_events
  \brief DEPRECATED: Wanpipe Commands associated with WANPIPE_IOCTL_TDM_API Ioctl

  Deprecated by enum wanpipe_tdm_api_events.

  The TDM API commands are used to enable/disable tdm functions
  on a TDM API device. These commands are deprecated and the
  enum wanpipe_tdm_api_events commands defined in wanpipe_api_iface.h.
 */
enum wanpipe_tdm_api_events {
	WP_TDMAPI_EVENT_NONE 				= WP_API_EVENT_NONE,
	WP_TDMAPI_EVENT_DTMF 				= WP_API_EVENT_DTMF,
	WP_TDMAPI_EVENT_RM_DTMF 			= WP_API_EVENT_RM_DTMF,
	WP_TDMAPI_EVENT_RXHOOK				= WP_API_EVENT_RXHOOK,
	WP_TDMAPI_EVENT_RING				= WP_API_EVENT_RING,
	WP_TDMAPI_EVENT_TONE				= WP_API_EVENT_TONE,
	WP_TDMAPI_EVENT_RING_DETECT 		= WP_API_EVENT_RING_DETECT,
	WP_TDMAPI_EVENT_TXSIG_KEWL			= WP_API_EVENT_TXSIG_KEWL,
	WP_TDMAPI_EVENT_TXSIG_START			= WP_API_EVENT_TXSIG_START,
	WP_TDMAPI_EVENT_TXSIG_OFFHOOK		= WP_API_EVENT_TXSIG_OFFHOOK,
	WP_TDMAPI_EVENT_TXSIG_ONHOOK		= WP_API_EVENT_TXSIG_ONHOOK,
	WP_TDMAPI_EVENT_ONHOOKTRANSFER		= WP_API_EVENT_ONHOOKTRANSFER,
	WP_TDMAPI_EVENT_SETPOLARITY			= WP_API_EVENT_SETPOLARITY,
	WP_TDMAPI_EVENT_BRI_CHAN_LOOPBACK	= WP_API_EVENT_BRI_CHAN_LOOPBACK,
	WP_TDMAPI_EVENT_RING_TRIP_DETECT 	= WP_API_EVENT_RING_TRIP_DETECT,
	WP_TDMAPI_EVENT_RBS					= WP_API_EVENT_RBS,
	WP_TDMAPI_EVENT_ALARM				= WP_API_EVENT_ALARM,
	WP_TDMAPI_EVENT_LINK_STATUS			= WP_API_EVENT_LINK_STATUS,
	WP_TDMAPI_EVENT_MODEM_STATUS		= WP_API_EVENT_MODEM_STATUS
};


enum wanpipe_tdm_api_cmds {

	SIOC_WP_TDM_GET_USR_MTU_MRU		= WP_API_CMD_GET_USR_MTU_MRU, 		/*!< Get Device tx/rx (CHUNK) in bytes, multiple of 8 */
	SIOC_WP_TDM_SET_USR_PERIOD		= WP_API_CMD_SET_USR_PERIOD,	/*!< Set chunk period in miliseconds eg: 1ms = 8bytes */
	SIOC_WP_TDM_GET_USR_PERIOD		= WP_API_CMD_GET_USR_PERIOD,	/*!< Get configured chunk period in miliseconds eg: 1ms = 8bytes */
	SIOC_WP_TDM_SET_HW_MTU_MRU		= WP_API_CMD_SET_HW_MTU_MRU, 	/*!< Set hw tx/rx chunk in bytes  eg: 1ms = 8bytes */
	SIOC_WP_TDM_GET_HW_MTU_MRU		= WP_API_CMD_GET_HW_MTU_MRU, 	/*!< Get hw tx/rx chunk in bytes  eg: 1ms = 8bytes */
	SIOC_WP_TDM_SET_CODEC			= WP_API_CMD_SET_CODEC,  		/*!< Set device codec (ulaw/alaw/slinear) */
	SIOC_WP_TDM_GET_CODEC			= WP_API_CMD_GET_CODEC, 		/*!< Get configured device codec (ulaw/alaw/slinear) */
	SIOC_WP_TDM_SET_POWER_LEVEL		= WP_API_CMD_SET_POWER_LEVEL,	/*!< Not implemented */
	SIOC_WP_TDM_GET_POWER_LEVEL		= WP_API_CMD_GET_POWER_LEVEL, 	/*!< Not implemented */
	SIOC_WP_TDM_TOGGLE_RX			= WP_API_CMD_TOGGLE_RX,  		/*!< Disable/Enable RX on this device */
	SIOC_WP_TDM_TOGGLE_TX			= WP_API_CMD_TOGGLE_TX,  		/*!< Disable/Enable TX on this device */
	SIOC_WP_TDM_GET_HW_CODING		= WP_API_CMD_GET_HW_CODING, 	/*!< Get HW coding configuration (ulaw or alaw) */
	SIOC_WP_TDM_SET_HW_CODING		= WP_API_CMD_SET_HW_CODING, 	/*!< Set HW coding (ulaw or alaw) */
	SIOC_WP_TDM_GET_FULL_CFG		= WP_API_CMD_GET_FULL_CFG,		/*!< Get full device configuration */
	SIOC_WP_TDM_SET_EC_TAP			= WP_API_CMD_SET_EC_TAP,  		/*!< Not implemented */
	SIOC_WP_TDM_GET_EC_TAP			= WP_API_CMD_GET_EC_TAP,  		/*!< Not implemented */
	SIOC_WP_TDM_ENABLE_RBS_EVENTS	= WP_API_CMD_ENABLE_RBS_EVENTS,	/*!< Enable RBS Event monitoring */
	SIOC_WP_TDM_DISABLE_RBS_EVENTS	= WP_API_CMD_DISABLE_RBS_EVENTS,	/*!< Disable RBS Event monitoring */
	SIOC_WP_TDM_WRITE_RBS_BITS		= WP_API_CMD_WRITE_RBS_BITS,		/*!< Write RBS bits (ABCD) */
	SIOC_WP_TDM_GET_STATS			= WP_API_CMD_GET_STATS,			/*!< Get device statistics */
	SIOC_WP_TDM_FLUSH_BUFFERS		= WP_API_CMD_FLUSH_BUFFERS,		/*!< Flush Buffers */
	SIOC_WP_TDM_READ_EVENT			= WP_API_CMD_READ_EVENT,			/*!<  */
	SIOC_WP_TDM_SET_EVENT			= WP_API_CMD_SET_EVENT,			/*!<  */
	SIOC_WP_TDM_SET_RX_GAINS		= WP_API_CMD_SET_RX_GAINS,			/*!<  */
	SIOC_WP_TDM_SET_TX_GAINS		= WP_API_CMD_SET_TX_GAINS,			/*!<  */
	SIOC_WP_TDM_CLEAR_RX_GAINS		= WP_API_CMD_CLEAR_RX_GAINS,		/*!<  */
	SIOC_WP_TDM_CLEAR_TX_GAINS		= WP_API_CMD_CLEAR_TX_GAINS,		/*!<  */
	SIOC_WP_TDM_GET_FE_ALARMS		= WP_API_CMD_GET_FE_ALARMS,		/*!<  */
	SIOC_WP_TDM_ENABLE_HWEC			= WP_API_CMD_ENABLE_HWEC,			/*!<  */
	SIOC_WP_TDM_DISABLE_HWEC		= WP_API_CMD_DISABLE_HWEC,			/*!<  */
	SIOC_WP_TDM_SET_FE_STATUS		= WP_API_CMD_SET_FE_STATUS,		/*!<  */
	SIOC_WP_TDM_GET_FE_STATUS		= WP_API_CMD_GET_FE_STATUS,		/*!<  */
	SIOC_WP_TDM_GET_HW_DTMF			= WP_API_CMD_GET_HW_DTMF,			/*!<  */
	SIOC_WP_TDM_DRV_MGMNT			= WP_API_CMD_DRV_MGMNT,			/*!<  */
	
	SIOC_WP_TDM_RESET_STATS			= WP_API_CMD_RESET_STATS, 			/*!< Reset device statistics */

	SIOC_WP_TDM_NOTSUPP				= WP_API_CMD_NOTSUPP,				/*!<  */

};

#define WANPIPE_IOCTL_TDM_API			WANPIPE_IOCTL_API_CMD

#define WP_TDMAPI_EVENT_ENABLE			WP_API_EVENT_ENABLE	
#define WP_TDMAPI_EVENT_DISABLE			WP_API_EVENT_DISABLE
#define WP_TDMAPI_EVENT_MODE_DECODE		WP_API_EVENT_MODE_DECODE

#define WP_TDMAPI_EVENT_RXHOOK_OFF		WP_API_EVENT_RXHOOK_OFF
#define WP_TDMAPI_EVENT_RXHOOK_ON		WP_API_EVENT_RXHOOK_ON
#define WP_TDMAPI_EVENT_RXHOOK_DECODE	WP_API_EVENT_RXHOOK_DECODE

#define WP_TDMAPI_EVENT_RING_PRESENT	WP_API_EVENT_RING_PRESENT
#define WP_TDMAPI_EVENT_RING_STOP		WP_API_EVENT_RING_STOP
#define WP_TDMAPI_EVENT_RING_DECODE		WP_API_EVENT_RING_DECODE

#define WP_TDMAPI_EVENT_RING_TRIP_PRESENT	WP_API_EVENT_RING_TRIP_PRESENT
#define WP_TDMAPI_EVENT_RING_TRIP_STOP		WP_API_EVENT_RING_TRIP_STOP
#define WP_TDMAPI_EVENT_RING_TRIP_DECODE	WP_API_EVENT_RING_TRIP_DECODE

/*Link Status */
#define WP_TDMAPI_EVENT_LINK_STATUS_CONNECTED		WP_API_EVENT_LINK_STATUS_CONNECTED
#define WP_TDMAPI_EVENT_LINK_STATUS_DISCONNECTED	WP_API_EVENT_LINK_STATUS_DISCONNECTED
#define WP_TDMAPI_EVENT_LINK_STATUS_DECODE			WP_API_EVENT_LINK_STATUS_DECODE

#define	WP_TDMAPI_EVENT_TONE_DIAL				WP_API_EVENT_TONE_DIAL
#define	WP_TDMAPI_EVENT_TONE_BUSY				WP_API_EVENT_TONE_BUSY
#define	WP_TDMAPI_EVENT_TONE_RING				WP_API_EVENT_TONE_RING
#define	WP_TDMAPI_EVENT_TONE_CONGESTION			WP_API_EVENT_TONE_CONGESTION

#define wp_tdm_chan_stats_t 	wanpipe_chan_stats_t
#define wanpipe_tdm_cfg_t 		wanpipe_api_cfg_t
#define wanpipe_tdm_api_cmd_t	wanpipe_api_cmd_t
#define wanpipe_tdm_api_event_t	wanpipe_api_callbacks_t
#define wanpipe_tdm_api_t		wanpipe_api_t
#define wanpipe_api_cfg_t		wanpipe_api_dev_cfg_t
#define wp_tdm_cmd				wp_cmd
#define wp_tdm_event			wp_callback

#define  WP_TDM_FEATURE_DTMF_EVENTS 	WP_API_FEATURE_DTMF_EVENTS
#define  WP_TDM_FEATURE_FE_ALARM 		WP_API_FEATURE_FE_ALARM
#define  WP_TDM_FEATURE_EVENTS 			WP_API_FEATURE_EVENTS
#define  WP_TDM_FEATURE_LINK_STATUS 	WP_API_FEATURE_LINK_STATUS

#define SANG_STATUS_TDM_EVENT_AVAILABLE	SANG_STATUS_API_EVENT_AVAILABLE


#endif

