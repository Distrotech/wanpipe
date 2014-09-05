/******************************************************************************//**
 * \file wanpipe_api_hdr.h
 * \brief WANPIPE(tm) Driver API Headers and Defines
 *
 * Authors: Nenad Corbic <ncorbic@sangoma.com>
 *			David Rokhvarg <davidr@sangoma.com>
 *			Alex Fledman <alex@sangoma.com>
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

#ifndef __WANPIPE_API_HDR__
#define __WANPIPE_API_HDR__


#pragma pack(1)

/*!
  \def WAN_MAX_EVENT_SZ
  \brief Size of wanpipe api header used in tx/rx

  The WAN_MAX_HDR_SZ is the size of wp_api_event_t structure
  It can be used to confirm compilation problems.
  if (sizeof(wp_api_event_t) != WAN_MAX_HDR_SZ) ...
 */
#define WAN_MAX_EVENT_SZ 64
#define WAN_MAX_EVENT_SZ_UNION WAN_MAX_EVENT_SZ-(sizeof(u_int8_t)*4)-(sizeof(u_int32_t)*2)
/*!
  \struct wp_api_event
  \brief Wanpipe API Event Structure

  \typedef wp_api_event_t
  \brief Wanpipe API Event Structure
  \typedef wp_tdm_api_event_t
  \brief Wanpipe API Event Structure
 */
typedef struct wp_api_event
{
	u_int8_t	event_type;			/*!< Event Type defined in enum wanpipe_tdm_api_events */
	u_int8_t	mode;				/*!< Mode (Enable/Disable), WP_TDMAPI_EVENT_DISABLE, WP_TDMAPI_EVENT_ENABLE */
	u_int8_t	channel;			/*!< TDM channel num, integer from 1 to 32 */
	u_int8_t	span;				/*!< TDM span num, integer from 1 to 255 */
	u_int32_t	time_stamp_sec;		/*!< timestamp in sec */
	u_int32_t	time_stamp_usec;	/*!< timestamp in microseconds */
	union {
		struct {
			u_int32_t	alarm;		/*!< contains bit map of T1/E1 alarms */
		} te1_alarm;
		struct {
			u_int8_t	rbs_bits;	/*!< contains bit map of T1/E1 rbs bits */
		} te1_rbs;
		struct {
			u_int8_t	state;		/*!< contains rx hook state: WP_TDMAPI_EVENT_RXHOOK_OFF, WP_TDMAPI_EVENT_RXHOOK_ON */
			u_int8_t	sig;		/*!<  */
		} rm_hook;
		struct {
			u_int8_t	state;		/*!< Ring state, WP_TDMAPI_EVENT_RING_PRESENT, WP_TDMAPI_EVENT_RING_STOP */
		} rm_ring;
		struct {
			u_int16_t	type;		/*!< Ring tone type, enum WP_TDMAPI_EVENT_TONE_xxx */
		} rm_tone;
		struct {
			u_int8_t	digit;		/*!< DTMF: digit  */
			u_int8_t	port;		/*!< DTMF: SOUT/ROUT */
			u_int8_t	type;		/*!< DTMF: PRESET/STOP */
		} dtmf;
		struct {
			u_int16_t	polarity;	/*!< */
			u_int16_t	ohttimer;	/*!< */
			u_int16_t	polarity_reverse;	/*!< */
		} rm_common;
		struct {
			int32_t gain;
		}rm_gain;
		struct{
			u_int16_t	status;		/*!< Link Status (connected/disconnnected) */
		} linkstatus;
		struct {
			u_int32_t	status;		/*!< Serial Modem Status: DCD/CTS */
		} serial;
		unsigned char reserved[WAN_MAX_EVENT_SZ_UNION];	/*!< Padding up to WAN_MAX_EVENT_SZ */
	};

/***************************************************//**
 These defines MUST be used to access wp_api_event_t
 structure.  The wp_api_event_t should not be accessed
 directly, in order to keep backward compile compatibility.
 *******************************************************/
#define wp_api_event_type 				event_type
#define wp_api_event_mode 				mode
#define wp_api_event_channel 			channel
#define wp_api_event_span				span
#define wp_api_event_alarm 				te1_alarm.alarm
#define wp_api_event_rbs_bits 			te1_rbs.rbs_bits
#define wp_api_event_hook_state 		rm_hook.state
#define wp_api_event_hook_sig 			rm_hook.sig
#define wp_api_event_ring_state 		rm_ring.state
#define wp_api_event_tone_type 			rm_tone.type
#define wp_api_event_dtmf_digit 		dtmf.digit
#define wp_api_event_dtmf_type 			dtmf.type
#define wp_api_event_dtmf_port 			dtmf.port
#define wp_api_event_ohttimer 			rm_common.ohttimer
#define wp_api_event_polarity 			rm_common.polarity
#define wp_api_event_polarity_reverse	rm_common.polarity_reverse
#define wp_api_event_link_status		linkstatus.status
#define wp_api_event_serial_status		serial.status
#define wp_api_event_time_stamp_sec		time_stamp_sec
#define wp_api_event_time_stamp_usec	time_stamp_usec
#define wp_api_event_gain_value			rm_gain.gain
} wp_api_event_t;


/***************************************************//**
 * Wanpipe Rx/Tx API Header Structure
 *
 * This is a UNIFIED common API Header structure for all protocols
 * and all APIs.
 *
 * data_length
 *  	Windows uses this varialbe for data length,
 *  	Linux legacy uses this variable as time_stamp
 *******************************************************/

/*!
  \def WAN_MAX_HDR_SZ
  \brief Size of wanpipe api header used in tx/rx

  The WAN_MAX_HDR_SZ is the size of wp_api_event_t structure
  It can be used to confirm compilation problems.
  if (sizeof(wp_api_event_t) != WAN_MAX_HDR_SZ) ...
 */
#define WAN_MAX_HDR_SZ 64
#define WAN_MAX_HDR_SZ_UNION WAN_MAX_HDR_SZ-sizeof(u_int8_t)-sizeof(u_int16_t)-(sizeof(u_int32_t)*2)

/*!
  \struct wp_api_hdr
  \brief Wanpipe API Header Structure

  \typedef wp_api_hdr_t
  \brief Wanpipe API Header Structure
 */
typedef struct wp_api_hdr
{
	u_int8_t	operation_status;		/*!< Status defined in enum SANG_STATUS_T */
	u_int16_t	data_length;			/*!< Length of tx/rx data packet */
	u_int32_t	time_stamp_sec;			/*!< timestamp in seconds */
	u_int32_t   time_stamp_usec;		/*!< timestamp in miliseconds */

	union {
		/***********************************************//**
		The rx_h and tx_h are to be used with all AFT Hardware
		****************************************************/
		struct {
			u_int16_t	crc;									/*!< bit map of possible errors: CRC/ABORT/FRAME/DMA  */
			u_int8_t	max_rx_queue_length;					/*!< max data queue configured */
			u_int8_t	current_number_of_frames_in_rx_queue;	/*!< current buffers in device rx queue */
			u_int32_t	errors;									/*!< number of rx errors since driver start */
		}rx_h;

		struct {
			u_int8_t	max_tx_queue_length;					/*!< max data queue configured */
			u_int8_t	current_number_of_frames_in_tx_queue;	/*!< current buffers in device tx queue */
			u_int32_t	tx_idle_packets;						/*!< number of tx idle packes transmitted */
			u_int32_t	errors;									/*!< number of tx errors since driver start */
		}tx_h;

		/***********************************************//**
		Defines below are Deprecated and are for
		backward compability
		****************************************************/
		struct {
			u_int8_t	status;
		}serial;
		struct {
			u_int8_t    exception;
			u_int8_t 	pf;
		}lapb;
		struct {
			u_int8_t	state;
			u_int8_t	address;
			u_int16_t   exception;
		}xdlc;
		struct {
			u_int8_t	channel;
			u_int8_t    direction;
		}bitstrm;
		struct {
			u_int8_t	channel;
			u_int8_t    status;
		}aft_legacy_rbs;
		struct {
			u_int8_t	repeat;
			u_int8_t	len;
			u_int8_t	data[8];
		}rpt;
		struct {
			u_int8_t	type;
			u_int8_t	force_tx;
			u_int8_t	data[8];
		}ss7_hw;
		u_int8_t reserved[WAN_MAX_HDR_SZ_UNION];
	};


/***************************************************//**
 These defines MUST be used to access wp_api_hdr_t
 structure.  The wp_api_hdr_t should not be accessed
 directly, in order to keep backward compile compatibility.
 *******************************************************/

#define wp_api_hdr_operation_status				operation_status
#define wp_api_hdr_data_length 					data_length
#define wp_api_hdr_time_stamp_sec				time_stamp_sec
#define wp_api_hdr_time_stamp_use				time_stamp_usec

#define wp_api_legacy_rbs_channel				aft_legacy_rbs.channel
#define wp_api_legacy_rbs_status				aft_legacy_rbs.status

#define wp_api_rx_hdr_crc						rx_h.crc
#define wp_api_rx_hdr_error_map					rx_h.crc
#define wp_api_rx_hdr_max_queue_length			rx_h.max_rx_queue_length
#define wp_api_rx_hdr_number_of_frames_in_queue	rx_h.current_number_of_frames_in_rx_queue
#define wp_api_rx_hdr_time_stamp_sec			time_stamp_sec
#define wp_api_rx_hdr_time_stamp_use			time_stamp_usec
#define wp_api_rx_hdr_errors					rx_h.errors

#define wp_api_tx_hdr_max_queue_length			tx_h.max_tx_queue_length
#define wp_api_tx_hdr_number_of_frames_in_queue	tx_h.current_number_of_frames_in_tx_queue
#define wp_api_tx_hdr_tx_idle_packets			tx_h.tx_idle_packets
#define wp_api_tx_hdr_time_stamp_sec			time_stamp_sec
#define wp_api_tx_hdr_time_stamp_use			time_stamp_usec
#define wp_api_tx_hdr_errors					tx_h.errors

/***********************************************//**
 Defines below are Deprecated and are for
 backward compability
****************************************************/

#if !defined(__WINDOWS__)
#define wp_api_rx_hdr_error_flag				operation_status
#define wp_api_rx_hdr_station					operation_status
#define wp_api_rx_hdr_time_stamp				data_length
#endif

#define wp_api_tx_hdr_hdlc_rpt_len				rpt.len
#define wp_api_tx_hdr_hdlc_rpt_data				rpt.data
#define wp_api_tx_hdr_hdlc_rpt_repeat			rpt.repeat

#define wp_api_tx_hdr_aft_ss7_type				ss7_hw.type
#define wp_api_tx_hdr_aft_ss7_force_tx			ss7_hw.force_tx
#define wp_api_tx_hdr_aft_ss7_data				ss7_hw.data

/* XDLC Old backdward comptabile */
#define wp_api_rx_hdr_xdlc_state 			 	xdlc.state
#define wp_api_rx_hdr_xdlc_address			 	xdlc.address
#define wp_api_rx_hdr_xdlc_exception		 	xdlc.exception

#define wan_hdr_xdlc_state						xdlc.state
#define wan_hdr_xdlc_address					xdlc.address
#define wan_hdr_xdlc_exception					xdlc.exception

/* CHDLC Old backdward comptabile */
#define wp_api_rx_hdr_chdlc_error_flag			wp_api_rx_hdr_error_flag
#define wp_api_rx_hdr_chdlc_time_stamp			wp_api_rx_hdr_time_stamp
#define wp_api_rx_hdr_chdlc_time_sec			time_stamp_sec
#define wp_api_rx_hdr_chdlc_time_usec			time_stamp_usec

#define wan_hdr_chdlc_error_flag				wp_api_rx_hdr_chdlc_error_flag
#define wan_hdr_chdlc_time_stamp				wp_api_rx_hdr_chdlc_time_stamp
#define wan_hdr_chdlc_time_sec					wp_api_rx_hdr_chdlc_time_sec
#define wan_hdr_chdlc_time_usec					wp_api_rx_hdr_chdlc_time_usec

/* BITSTRM Old backdward comptabile */
#define wp_api_rx_hdr_bitstrm_error_flag		wp_api_rx_hdr_error_flag
#define wp_api_rx_hdr_bitstrm_time_stamp		wp_api_rx_hdr_time_stamp
#define wp_api_rx_hdr_bitstrm_time_sec			time_stamp_sec
#define wp_api_rx_hdr_bitstrm_time_usec			time_stamp_usec
#define wp_api_rx_hdr_bitstrm_channel			bitstrm.channel
#define wp_api_rx_hdr_bitstrm_direction			bitstrm.direction

#define wan_hdr_bitstrm_error_flag				wp_api_rx_hdr_bitstrm_error_flag
#define wan_hdr_bitstrm_time_stamp				wp_api_rx_hdr_bitstrm_data_length
#define wan_hdr_bitstrm_time_sec				wp_api_rx_hdr_bitstrm_time_sec
#define wan_hdr_bitstrm_time_usec				wp_api_rx_hdr_bitstrm_time_usec
#define wan_hdr_bitstrm_channel					wp_api_rx_hdr_bitstrm_channel
#define wan_hdr_bitstrm_direction				wp_api_rx_hdr_bitstrm_direction

/* HDLC Old backdward comptabile */
#define wp_api_rx_hdr_hdlc_error_flag			wp_api_rx_hdr_error_flag
#define wp_api_rx_hdr_hdlc_time_stamp			wp_api_rx_hdr_time_stamp
#define wp_api_rx_hdr_hdlc_time_sec				time_stamp_sec
#define wp_api_rx_hdr_hdlc_time_usec			time_stamp_usec

#define wan_hdr_hdlc_error_flag					wp_api_rx_hdr_error_flag
#define wan_hdr_hdlc_time_stamp					wp_api_rx_hdr_time_stamp

/* LAPBS Old backdward comptabile */
#define wp_api_rx_hdr_lapb_pf					lapb.pf
#define wp_api_rx_hdr_lapb_exception			lapb.exception
#define wp_api_rx_hdr_lapb_time_sec				time_stamp_sec
#define wp_api_rx_hdr_lapb_time_usec			time_stamp_usec

#define wan_hdr_lapb_pf							wp_api_rx_hdr_lapb_pf
#define wan_hdr_lapb_exception					wp_api_rx_hdr_lapb_exception

/* FR Old backdward comptabile */
#define wp_api_rx_hdr_fr_attr					wp_api_rx_hdr_error_flag
#define wp_api_rx_hdr_fr_time_stamp				wp_api_rx_hdr_time_stamp
#define wp_api_rx_hdr_fr_time_sec				time_stamp_sec
#define wp_api_rx_hdr_fr_time_usec				time_stamp_usec

#define wan_hdr_fr_attr							wp_api_rx_hdr_fr_attr
#define wan_hdr_fr_time_stamp					wp_api_rx_hdr_fr_time_stamp
#define wan_hdr_fr_time_sec						wp_api_rx_hdr_fr_time_sec
#define wan_hdr_fr_time_usec					wp_api_rx_hdr_fr_time_usec

/* SS7 Legacy Old backdward comptabile */
#define wp_api_rx_hdr_ss7_legacy_sio			wp_api_rx_hdr_error_flag
#define wp_api_rx_hdr_ss7_legacy_time_stamp		wp_api_rx_hdr_time_stamp

#define wp_api_rx_hdr_event_serial_status		serial.status

} wp_api_hdr_t;


/***********************************************//**
 Wanpipe API Element Structure
****************************************************/

/*!
  \def MAX_NO_DATA_BYTES_IN_FRAME
  \brief Maximum tx/rx data size
 */
#define MAX_NO_DATA_BYTES_IN_FRAME	8188


/*!
 \struct wp_api_element
 \brief Wanpipe API Element contains header and data 
 
 The Element Structures are suppose to be used as
 casts to a memory buffer. Windows uses the full
 size of the element structure to allocate the
 acutal memory buffer used as an element structure

 \typedef wp_api_element_t
 \brief Wanpipe API Element contains header and data 
*/

typedef struct wp_api_element{

	wp_api_hdr_t	hdr;	/*!< Header structure */
	unsigned char	data[MAX_NO_DATA_BYTES_IN_FRAME];	/*!< Maximum Tx/Rx Data buffer structure */

/***************************************************//**
 These defines MUST be used to access wp_api_element_t
 structure.  The wp_api_element_t should not be accessed
 directly, in order to keep backward compile compatibility.
 *******************************************************/

#define wp_api_el_operation_status 				hdr.wp_api_hdr_operation_status
#define wp_api_el_data_length 					hdr.wp_api_hdr_data_length
#define wp_api_el_time_stamp_sec				hdr.wp_api_hdr_time_stamp_sec
#define wp_api_el_time_stamp_use				hdr.wp_api_hdr_time_stamp_use

#define wp_api_rx_el_crc 						hdr.wp_api_rx_hdr_crc
#define wp_api_rx_el_max_queue_length 			hdr.wp_api_rx_hdr_max_queue_length
#define wp_api_rx_el_number_of_frames_in_queue 	hdr.wp_api_rx_hdr_number_of_frames_in_queue

#define wp_api_tx_el_max_queue_length 			hdr.wp_api_tx_hdr_max_queue_length
#define wp_api_tx_el_number_of_frames_in_queue 	hdr.wp_api_tx_hdr_number_of_frames_in_queue

} wp_api_element_t;


#pragma pack()


/**************************************************************
 Sangoma Memory descriptors - passed to/from kernel/user space
***************************************************************/

#pragma pack(1)         

/*!
  \struct	wan_iovec
  \brief	Memory Descriptor of a single buffer.
			This structure used internally by libsangoma.

  \typedef	wan_iovec_t
  \brief	Memory Descriptor of a single buffer.
			This structure used internally by libsangoma.
*/
typedef struct wan_iovec
{
	u_int32_t iov_len;
	
	void * WP_POINTER_64 iov_base;
#ifndef __x86_64__
	u_int32_t reserved;
#endif
}wan_iovec_t;

/*!
  \struct	wan_iovec_list
  \brief	Fixed-length List of Memory Descriptors 

  \typedef	wan_iovec_list_t
  \brief	Fixed-length List of Memory Descriptors 
*/
#define WAN_IOVEC_LIST_LEN	5
typedef struct wan_iovec_list
{
	wan_iovec_t	iovec_list[WAN_IOVEC_LIST_LEN];/*!< in 'iovec_list', only buffers with non-NULL 'iov_base' should be accessed */

}wan_iovec_list_t;

/*!
  \struct	wan_msghdr
  \brief	Variable-length List of Memory Descriptors

  \typedef	wan_msghdr_t
  \brief	Variable-length List of Memory Descriptors
*/
typedef struct wan_msghdr {
	u_int32_t  msg_iovlen; /*!< Number of blocks */
	wan_iovec_t * WP_POINTER_64 msg_iov;	/*!< Data blocks */
#ifndef __x86_64__
	u_int32_t reserved;
#endif
}wan_msghdr_t;

#pragma pack()

/****************************************************************/

/*!
  \struct _API_POLL_STRUCT
  \brief Windows poll structure used to implement blocking poll for read/write/event

  This structure is only used by WINDOWS code

  \typedef API_POLL_STRUCT
  \brief Windows poll structure used to implement blocking poll for read/write/event
 */
typedef struct _API_POLL_STRUCT
{
	unsigned char	operation_status;	/*!< operation completion status, check on return */
	u_int32_t		user_flags_bitmap;	/*!< bitmap of events API user is interested to receive */
	u_int32_t		poll_events_bitmap;	/*!< bitmap of events available for API user */
}API_POLL_STRUCT;


/*!
  \enum SANG_STATUS
  \brief Wanpipe API Return codes.

   Extra care should be taken when changing SANG_STATUS_xxx definitions
   to keep compatibility with existing binary files!

   \typedef SANG_STATUS_T
   \brief Wanpipe API Return codes.
 */
typedef enum SANG_STATUS
{

	SANG_STATUS_SUCCESS=0,					/*!< An operation completed successfully */

	/*************************************//**
	 Return codes specific for data reception: 
	******************************************/
	SANG_STATUS_RX_DATA_TIMEOUT,			/*!< No data was received. NOT an error. */
	SANG_STATUS_RX_DATA_AVAILABLE,			/*!< Data was received. */
	SANG_STATUS_NO_DATA_AVAILABLE,			/*!< There is no RX data in API receive queue */

	/*************************************//**
	 Return codes specific for data transmission:
	******************************************/
	SANG_STATUS_TX_TIMEOUT,					/*!< Transmit command timed out  */
	SANG_STATUS_TX_DATA_TOO_LONG,			/*!< Longer than MTU */
	SANG_STATUS_TX_DATA_TOO_SHORT,			/*!< Shorter than minimum */
	SANG_STATUS_TX_HDR_TOO_SHORT,			/*!< Tx Header is too short */

	/*************************************//**
	 Return codes specific for line status:
	******************************************/
	SANG_STATUS_LINE_DISCONNECTED,			/*!< Physical line (T1/E1/Serial) is disconnected. */
	SANG_STATUS_PROTOCOL_DISCONNECTED,		/*!< The Communication Protocol (CHDLC/Frame Relay/PPP/LAPD...) is disconnected  */
	SANG_STATUS_LINE_CONNECTED,				/*!< Physical line (T1/E1/Serial) is connected. */
	SANG_STATUS_PROTOCOL_CONNECTED,			/*!< The Communication Protocol (CHDLC/Frame Relay/PPP/LAPD...) is connected */

	/*************************************//**
	 Return codes specific for general errors:
	******************************************/

	SANG_STATUS_COMMAND_ALREADY_RUNNING,	/*!< Two threads attempting to access device at the same time. */
	SANG_STATUS_BUFFER_TOO_SMALL,			/*!< Buffer passed to API is too small.  */
	SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY,	/*!< Kernel error */
	SANG_STATUS_FAILED_ALLOCATE_MEMORY,		/*!< Memory allocatin failure  */
	SANG_STATUS_INVALID_DEVICE_REQUEST,		/*!< Command is invalid for device type. */
	SANG_STATUS_INVALID_PARAMETER,			/*!< Invalid parameter */
	SANG_STATUS_GENERAL_ERROR,				/*!< General interal error */
	SANG_STATUS_DEVICE_BUSY,				/*!< Device is busy */
	SANG_STATUS_INVALID_DEVICE,				/*!< Invalid device selected */
	SANG_STATUS_IO_ERROR,					/*!< IO error on device */
	SANG_STATUS_UNSUPPORTED_FUNCTION,		/*!< Unsupported command or function */
	SANG_STATUS_UNSUPPORTED_PROTOCOL,		/*!< Unsupported protocol selected */
	SANG_STATUS_DEVICE_ALREADY_EXIST,		/*!< Device already exists */
	SANG_STATUS_DEV_INIT_INCOMPLETE,		/*!< Device initialization failed or not done */
	SANG_STATUS_TRACE_QUEUE_EMPTY,			/*!< Trace queue empty */
	SANG_STATUS_OPTION_NOT_SUPPORTED,		/*!< Unsupported command or option */

	/*************************************//**
	Wanpipe API Event Available
	******************************************/
	SANG_STATUS_API_EVENT_AVAILABLE,		/*!< Wanpipe API Event is available */


	/*************************************//**
	API Operation Internal errors 
	******************************************/
	SANG_STATUS_CAN_NOT_STOP_DEVICE_WHEN_ALREADY_STOPPED,		/*!<  Failed to stop device, already stopped */
	SANG_STATUS_CAN_NOT_RUN_TWO_PORT_CMDS_AT_THE_SAME_TIME,		/*!<  Failed to execute command, busy due to collision */
	SANG_STATUS_ASSOCIATED_IRP_SYSTEM_BUFFER_NULL_ERROR,		/*!<  */
	SANG_STATUS_STRUCTURE_SIZE_MISMATCH_ERROR,		/*!< Header size mistmatch between user & driver. Recompilation is necessary */


	/*************************************//**
	Windows Specific definitions
	******************************************/
	SANG_STATUS_REGISTRY_ERROR=180,				/*!< Windows Registry Error  */
	SANG_STATUS_IO_PENDING,						/*!< Asynchronous IO  */
	SANG_STATUS_APIPOLL_TIMEOUT,				/*!< API Poll timeout - no events or data  */
	SANG_STATUS_NO_FREE_BUFFERS,
	SANG_STATUS_SHARED_EVENT_INIT_ERROR,		/*!< Driver can not use 'shared event' supplied by the API user  */

	/****************************************//**
	  For internal API use only.
  	  Range reserved for internal API use start
     *******************************************/
	SANG_STATUS_DATA_QUEUE_EMPTY=190,			/*!< queue empty */
	SANG_STATUS_DATA_QUEUE_FULL,				/*!< queue empty full */
	SANG_STATUS_INVALID_IRQL					/*!< Driver routine was called at invalid IRQL. */

}SANG_STATUS_T;

/*!
  \def SDLA_DECODE_SANG_STATUS
  \brief Print decode of Sangoma Return Codes

 */
#define SDLA_DECODE_SANG_STATUS(status)	\
(abs((int)status) == SANG_STATUS_SUCCESS) ? "SANG_STATUS_SUCCESS" :\
(abs((int)status) == SANG_STATUS_COMMAND_ALREADY_RUNNING) ? "SANG_STATUS_COMMAND_ALREADY_RUNNING":\
(abs((int)status) == SANG_STATUS_BUFFER_TOO_SMALL) ? "SANG_STATUS_BUFFER_TOO_SMALL":\
(abs((int)status) == SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY) ? "SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY":\
(abs((int)status) == SANG_STATUS_FAILED_ALLOCATE_MEMORY) ? "SANG_STATUS_FAILED_ALLOCATE_MEMORY":\
(abs((int)status) == SANG_STATUS_INVALID_DEVICE_REQUEST) ? "SANG_STATUS_INVALID_DEVICE_REQUEST":\
(abs((int)status) == SANG_STATUS_INVALID_PARAMETER) ? "SANG_STATUS_INVALID_PARAMETER":\
(abs((int)status) == SANG_STATUS_DATA_QUEUE_EMPTY) ? "SANG_STATUS_DATA_QUEUE_EMPTY":\
(abs((int)status) == SANG_STATUS_DATA_QUEUE_FULL) ? "SANG_STATUS_DATA_QUEUE_FULL":\
(abs((int)status) == SANG_STATUS_RX_DATA_TIMEOUT) ? "SANG_STATUS_RX_DATA_TIMEOUT":\
(abs((int)status) == SANG_STATUS_RX_DATA_AVAILABLE) ? "SANG_STATUS_RX_DATA_AVAILABLE":\
(abs((int)status) == SANG_STATUS_TX_TIMEOUT) ? "SANG_STATUS_TX_TIMEOUT":\
(abs((int)status) == SANG_STATUS_TX_DATA_TOO_LONG) ? "SANG_STATUS_TX_DATA_TOO_LONG":\
(abs((int)status) == SANG_STATUS_TX_DATA_TOO_SHORT) ? "SANG_STATUS_TX_DATA_TOO_SHORT":\
(abs((int)status) == SANG_STATUS_LINE_DISCONNECTED) ? "SANG_STATUS_LINE_DISCONNECTED":\
(abs((int)status) == SANG_STATUS_LINE_CONNECTED) ? "SANG_STATUS_LINE_CONNECTED":\
(abs((int)status) == SANG_STATUS_PROTOCOL_DISCONNECTED) ? "SANG_STATUS_PROTOCOL_DISCONNECTED":\
(abs((int)status) == SANG_STATUS_PROTOCOL_CONNECTED) ? "SANG_STATUS_PROTOCOL_CONNECTED":\
(abs((int)status) == SANG_STATUS_GENERAL_ERROR) ? "SANG_STATUS_GENERAL_ERROR":\
(abs((int)status) == SANG_STATUS_DEVICE_BUSY) ? "SANG_STATUS_DEVICE_BUSY":\
(abs((int)status) == SANG_STATUS_INVALID_DEVICE) ? "SANG_STATUS_INVALID_DEVICE":\
(abs((int)status) == SANG_STATUS_IO_ERROR) ? "SANG_STATUS_IO_ERROR":\
(abs((int)status) == SANG_STATUS_UNSUPPORTED_FUNCTION) ? "SANG_STATUS_UNSUPPORTED_FUNCTION":\
(abs((int)status) == SANG_STATUS_UNSUPPORTED_PROTOCOL) ? "SANG_STATUS_UNSUPPORTED_PROTOCOL":\
(abs((int)status) == SANG_STATUS_DEVICE_ALREADY_EXIST) ? "SANG_STATUS_DEVICE_ALREADY_EXIST":\
(abs((int)status) == SANG_STATUS_DEV_INIT_INCOMPLETE) ? "SANG_STATUS_DEV_INIT_INCOMPLETE":\
(abs((int)status) == SANG_STATUS_API_EVENT_AVAILABLE) ? "SANG_STATUS_API_EVENT_AVAILABLE":\
(abs((int)status) == SANG_STATUS_REGISTRY_ERROR) ? "SANG_STATUS_REGISTRY_ERROR":\
(abs((int)status) == SANG_STATUS_CAN_NOT_STOP_DEVICE_WHEN_ALREADY_STOPPED) ? "SANG_STATUS_CAN_NOT_STOP_DEVICE_WHEN_ALREADY_STOPPED":\
(abs((int)status) == SANG_STATUS_CAN_NOT_RUN_TWO_PORT_CMDS_AT_THE_SAME_TIME) ? "SANG_STATUS_CAN_NOT_RUN_TWO_PORT_CMDS_AT_THE_SAME_TIME":\
(abs((int)status) == SANG_STATUS_ASSOCIATED_IRP_SYSTEM_BUFFER_NULL_ERROR) ? "SANG_STATUS_ASSOCIATED_IRP_SYSTEM_BUFFER_NULL_ERROR":\
(abs((int)status) == SANG_STATUS_STRUCTURE_SIZE_MISMATCH_ERROR) ? "SANG_STATUS_STRUCTURE_SIZE_MISMATCH_ERROR":\
(abs((int)status) == SANG_STATUS_INVALID_IRQL) ? "SANG_STATUS_INVALID_IRQL":\
(abs((int)status) == SANG_STATUS_NO_DATA_AVAILABLE) ? "SANG_STATUS_NO_DATA_AVAILABLE":\
(abs((int)status) == SANG_STATUS_IO_PENDING) ? "SANG_STATUS_IO_PENDING":\
(abs((int)status) == SANG_STATUS_APIPOLL_TIMEOUT) ? "SANG_STATUS_APIPOLL_TIMEOUT":\
(abs((int)status) == SANG_STATUS_NO_FREE_BUFFERS) ? "SANG_STATUS_NO_FREE_BUFFERS":\
(abs((int)status) == SANG_STATUS_OPTION_NOT_SUPPORTED) ? "SANG_STATUS_OPTION_NOT_SUPPORTED":\
"Status Unknown"

#define SANG_SUCCESS(status)	(status == SANG_STATUS_SUCCESS)
#define SANG_ERROR(status)		(!SANG_SUCCESS(status))

#if defined(__WINDOWS__)
#if defined(WAN_KERNEL) || defined(USE_SANGOMA_ERRNO)
/*
	Cross-Platform return codes.
	SOME of these return codes are defined in errno.h, but not ALL of them.
*/
# undef EFAULT
# undef EBUSY
# undef ENODEV
# undef EINVAL
# undef EIO
# undef EPFNOSUPPORT
# undef EPROTONOSUPPORT
# undef ENOMEM
# undef EEXIST
# undef ENOBUFS
# undef EOPNOTSUPP
# undef ENXIO
# undef EAGAIN
# undef EFBIG

# define ETIMEDOUT				SANG_STATUS_GENERAL_ERROR
# define EFAULT				SANG_STATUS_GENERAL_ERROR
# define EBUSY				SANG_STATUS_DEVICE_BUSY
# define ENODEV				SANG_STATUS_INVALID_DEVICE
# define EINVAL				SANG_STATUS_INVALID_PARAMETER
# define EIO				SANG_STATUS_IO_ERROR
# define EPFNOSUPPORT		SANG_STATUS_UNSUPPORTED_FUNCTION
# define EPROTONOSUPPORT	SANG_STATUS_UNSUPPORTED_PROTOCOL
# define ENOMEM				SANG_STATUS_FAILED_ALLOCATE_MEMORY
# define EEXIST				SANG_STATUS_DEVICE_ALREADY_EXIST
# define ENOBUFS			SANG_STATUS_NO_FREE_BUFFERS /* no free tx or rx buffers */
# define EOPNOTSUPP			SANG_STATUS_OPTION_NOT_SUPPORTED
# define ENXIO				EFAULT
# define ENETDOWN			SANG_STATUS_LINE_DISCONNECTED
# define EAGAIN				SANG_STATUS_DEVICE_BUSY
# define EFBIG				SANG_STATUS_TX_DATA_TOO_LONG
# define EAFNOSUPPORT		SANG_STATUS_UNSUPPORTED_FUNCTION
#endif
#endif /* __WINDOWS */

#endif/* __WANPIPE_API_HDR__ */
