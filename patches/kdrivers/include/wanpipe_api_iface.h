/******************************************************************************//**
 * \file wanpipe_api_iface.h
 * \brief WANPIPE(tm) Driver API Interface -
 * \brief Provides IO/Event API Only
 *
 * Authors: Nenad Corbic <ncorbic@sangoma.com>
 *			David Rokhvarg <davidr@sangoma.com>
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

#ifndef __WANPIPE_API_IFACE_H_
#define __WANPIPE_API_IFACE_H_

#include "wanpipe_api_hdr.h"

/*!
  \typedef sng_fd_t
  \brief Windows/Unix file handle abstraction
 */
#if defined(__WINDOWS__)
typedef HANDLE sng_fd_t;
#else
typedef int sng_fd_t;
#endif

/* Indicate to library that new features exist */
/*!
  \def WP_API_FEATURE_DTMF_EVENTS
  \brief Indicates to developer that dtmf feature is available
  \def WP_API_FEATURE_FE_ALARM
  \brief Indicates to developer that fe alarm feature is available
  \def WP_API_FEATURE_EVENTS
  \brief Indicates to developer that events feature is available
  \def WP_API_FEATURE_LINK_STATUS
  \brief Indicates to developer that link status feature is available
  \def WP_API_FEATURE_POL_REV
  \brief Indicates to developer that polarity reversal feature is available
  \def WP_API_FEATURE_LOGGER
  \brief Indicates to developer that Wanpipe Logger API feature is available
  \def WP_API_FEATURE_RM_GAIN
  \brief Indicates to developer that analog hardware gain feature is available
  \def WP_API_FEATURE_LOOP
  \brief Indicates to developer that loop feature is available
  \def WP_API_FEATURE_HWEC
  \brief Indicates to developer that Hardware Echo canceller feature is available
  \def WP_API_FEATURE_BUFFER_MULT
  \brief Indicates to developer that buffer multiplier featere is available
  \def WP_API_FEATURE_RX_TX_ERRS
  \brief Indicates to developer that rx tx error reporting feature is available
  \def WP_API_FEATURE_EC_CHAN_STAT
  \brief Indicates to developer that echo channel status feature is available
*/
#define WP_API_FEATURE_DTMF_EVENTS	1
#define WP_API_FEATURE_FE_ALARM		1
#define WP_API_FEATURE_EVENTS		1
#define WP_API_FEATURE_LINK_STATUS	1
#define WP_API_FEATURE_POL_REV		1
#define WP_API_FEATURE_LOGGER		1
#define WP_API_FEATURE_FAX_EVENTS   1
#define WP_API_FEATURE_RM_GAIN		1
#define WP_API_FEATURE_LOOP			1
#define WP_API_FEATURE_BUFFER_MULT  1
#define WP_API_FEATURE_RX_TX_ERRS	1
#define WP_API_FEATURE_EC_CHAN_STAT 1
#define WP_API_FEATURE_LIBSNG_HWEC	1
#define WP_API_FEATURE_DRIVER_GAIN  1
#define WP_API_FEATURE_FE_RW        1
#define WP_API_FEATURE_HWEC_PERSIST 1
#define WP_API_FEATURE_FAX_TYPE_EVENTS 1
#define WP_API_FEATURE_HARDWARE_RESCAN     1
#define WP_API_FEATURE_LED_CTRL     1
#define WP_API_FEATURE_SS7_FORCE_RX 1
#define WP_API_FEATURE_SS7_CFG_STATUS 1
#define WP_API_FEATURE_LIBSNG_HWEC_DTMF_REMOVAL 1


/*!
  \enum WANPIPE_IOCTL_CODE
  \brief Wanpipe IOCTL Commands

  Wanpipe System/IOCTL Commands for API Devices
  The system calls perform, configration/management/operation/io 
 */
enum WANPIPE_IOCTL_CODE {
	WANPIPE_IOCTL_WRITE=1,				/*!< Write cmd, Windows Only */
	WANPIPE_IOCTL_READ,					/*!< Read cmd, Windows Only */
	WANPIPE_IOCTL_MGMT,					/*!< Mgmnt cmd, driver/port configuration/debugging */
	WANPIPE_IOCTL_SET_IDLE_TX_BUFFER,	/*!< Set idle buffer on a device */
	WANPIPE_IOCTL_API_POLL,				/*!< Poll cmd, Windows Only */
	WANPIPE_IOCTL_SET_SHARED_EVENT,		/*!< Shared Event cmd, Windows Only */
	WANPIPE_IOCTL_PORT_MGMT,			/*!< Port Mgmnt Event cmd */
	WANPIPE_IOCTL_PORT_CONFIG,			/*!< Port Config Event */
	WANPIPE_IOCTL_API_CMD,				/*!< Wanpipe API command  */
	WANPIPE_IOCTL_PIPEMON,				/*!< PIPEMON command, debugging */
	WANPIPE_IOCTL_SNMP,					/*!< SNMP statistics, not implemented */
	WANPIPE_IOCTL_SNMP_IFSPEED,			/*!< SNMP statistics, not implemented */
	WANPIPE_IOCTL_DEVEL,				/*!< Development Cmds, use only for hw debugging */
	WANPIPE_IOCTL_WRITE_NON_BLOCKING,	/*!< Non-Blocking Write cmd, Windows Only */
	WANPIPE_IOCTL_READ_NON_BLOCKING,	/*!< Non-Blocking Read cmd, Windows Only */
	WANPIPE_IOCTL_CDEV_CTRL,			/*!< Non-Blocking Cdev control cmd, Windows Only */
	WANPIPE_IOCTL_LOGGER_CMD,			/*!< Wanpipe Logger command */
};


/*!
  \enum WANPIPE_IOCTL_PIPEMON_CMDS
  \brief Commands used with WANPIPE_IOCTL_PIPEMON IOCTL
 */
enum WANPIPE_IOCTL_PIPEMON_CMDS {
	WANPIPEMON_ROUTER_UP_TIME = 0x50,		/*!< Check router up time */
	WANPIPEMON_ENABLE_TRACING,				/*!< Enable data tracing */
	WANPIPEMON_DISABLE_TRACING,				/*!< Disable data tracing */
	WANPIPEMON_GET_TRACE_INFO,				/*!< Get traced data frame  */
	WANPIPEMON_READ_CODE_VERSION,			/*!< Get HW Firmware Version */
	WANPIPEMON_FLUSH_OPERATIONAL_STATS,		/*!< Flush operational stats */
	WANPIPEMON_OPERATIONAL_STATS,			/*!< Get operational stats */
	WANPIPEMON_READ_OPERATIONAL_STATS,		/*!< Get operational stats */
	WANPIPEMON_READ_CONFIGURATION,			/*!< Get device configuration */
	WANPIPEMON_READ_COMMS_ERROR_STATS,		/*!< Read comm statistics*/
	WANPIPEMON_FLUSH_COMMS_ERROR_STATS,		/*!< Flush comm statistics*/
	WANPIPEMON_AFT_LINK_STATUS,				/*!< Get Device Link Status */
	WANPIPEMON_AFT_MODEM_STATUS,			/*!< Get Device Mode Status (Serial Cards Only) */
	WANPIPEMON_AFT_HWEC_STATUS,				/*!< Get Device HWEC Status (Supported or Not) */
	WANPIPEMON_DIGITAL_LOOPTEST,			/*!< Perform a digial loop data integrity test */
	WANPIPEMON_SET_FT1_MODE,				/*!< Set T1 Mode - Legacy Deprecated */

	WANPIPEMON_GET_OPEN_HANDLES_COUNTER,	/*!< Get number of open handles on this device */
	WANPIPEMON_GET_CARD_TYPE,				/*!< Get card type A101/2/4/8/A200 */
	WANPIPEMON_API_NOT_USED,				/*!< Not Used */
	WANPIPEMON_GET_HW_MAC_ADDR,				/*!< Get HWE MAC Address (Used by S518 Adsl) */
	WANPIPEMON_FLUSH_TX_BUFFERS,			/*!< Flush Tx Buffers */
	WANPIPEMON_EC_IOCTL,				/*!< HWEC Commands */
	WANPIPEMON_SET_RBS_BITS,			/*!< Set RBS Bits */
	WANPIPEMON_GET_RBS_BITS,			/*!< Read RBS Bits */
	WANPIPEMON_AFT_CUSTOMER_ID,				/*!< Get Unique Custome ID */
	WANPIPEMON_FT1_READ_STATUS,				/*!< Read T1/E1 Status */

	WANPIPEMON_DRIVER_STAT_IFSEND,			/*!< Driver statistics ifsend() */
	WANPIPEMON_DRIVER_STAT_INTR,			/*!< Driver statistics interrupts */
	WANPIPEMON_DRIVER_STAT_GEN,				/*!< Driver statistics general */
	WANPIPEMON_FLUSH_DRIVER_STATS,			/*!< Flush driver statistics */
	WANPIPEMON_GET_IBA_DATA,				/*!< Get IBA Data - Deprecated not used */
	WANPIPEMON_TDM_API,						/*!< Windows Legacy- TDM API commands */

	WANPIPEMON_READ_PERFORMANCE_STATS,
	WANPIPEMON_FLUSH_PERFORMANCE_STATS,
	WANPIPEMON_GET_BIOS_ENCLOSURE3_SERIAL_NUMBER,	/*!< Get Enclosure3 Serial Number from Motherboard BIOS. */

	WANPIPEMON_ENABLE_BERT,				/*!< Start BERT for the interface. */
	WANPIPEMON_DISABLE_BERT,			/*!< Stop BERT for the interface.  */
	WANPIPEMON_GET_BERT_STATUS,			/*!< Get BERT status/statistics (Locked/Not Locked) */
	WANPIPEMON_PERFORMANCE_STATS,       /*!< Control performance performance statistics */
	WANPIPEMON_LED_CTRL,				/*!< Control led on a port - on/off */

	/* Do not add any non-debugging commands below */
	WANPIPEMON_CHAN_SEQ_DEBUGGING,			/*!< Debugging only - enable/disable span level sequence debugging */
	WANPIPEMON_GEN_FIFO_ERR_TX,				/*!< Debugging only - generate tx fifo error */
	WANPIPEMON_GEN_FIFO_ERR_RX,				/*!< Debugging only - generate rx fifo error */
	WANPIPEMON_GEN_FE_SYNC_ERR,				/*!< Debugging only - generate fe sync error */

	/* All Public commands must be between WANPIPEMON_ROUTER_UP_TIME and WANPIPEMON_PROTOCOL_PRIVATE. */

	WANPIPEMON_PROTOCOL_PRIVATE		/*!< Private Wanpipemon commands used by lower layers */
};


typedef enum wp_bert_sequence_type{
	WP_BERT_RANDOM_SEQUENCE = 1,
    WP_BERT_ASCENDANT_SEQUENCE,
    WP_BERT_DESCENDANT_SEQUENCE
}wp_bert_sequence_type_t;

#define WP_BERT_DECODE_SEQUENCE_TYPE(sequence)					\
	((sequence) == WP_BERT_RANDOM_SEQUENCE) ? "WP_BERT_RANDOM_SEQUENCE" :			\
	((sequence) == WP_BERT_ASCENDANT_SEQUENCE) ? "WP_BERT_ASCENDANT_SEQUENCE" :		\
	((sequence) == WP_BERT_DESCENDANT_SEQUENCE) ? "WP_BERT_DESCENDANT_SEQUENCE" :	\
		"(Unknown BERT sequence)"

#define	WP_BERT_STATUS_OUT_OF_SYNCH	0
#define WP_BERT_STATUS_IN_SYNCH		1

/*!
  \struct _wp_bert_status
  \brief BERT status and statistics

  \typedef wp_bert_status_t
*/
typedef struct _wp_bert_status{

	unsigned char state;	/*!< Current state of BERT */
	unsigned int errors;	/*!< Nuber of errors during BERT */
	unsigned int synchonized_count; /*!< Number of times BERT got into synch */

}wp_bert_status_t;



/*!
  \struct wan_api_ss7_status
  \brief SS7 Hardare/Driver configuration status

  \typedef wan_api_ss7_status_t
*/
typedef struct wan_api_ss7_cfg_status {
	unsigned char ss7_hw_enable;
	unsigned char ss7_hw_mode;
	unsigned char ss7_hw_lssu_size;
	unsigned char ss7_driver_repeat;
} wan_api_ss7_cfg_status_t; 

/*!
  \enum wanpipe_api_cmds
  \brief Commands used with WANPIPE_IOCTL_API_CMD IOCTL
 */
enum wanpipe_api_cmds
{

	WP_API_CMD_GET_USR_MTU_MRU, 	/*!< Get Device tx/rx (CHUNK) in bytes, multiple of 8 */
	WP_API_CMD_SET_USR_PERIOD,		/*!< Set chunk period in miliseconds eg: 1ms = 8bytes */
	WP_API_CMD_GET_USR_PERIOD,		/*!< Get configured chunk period in miliseconds eg: 1ms = 8bytes */
	WP_API_CMD_SET_HW_MTU_MRU, 		/*!< Set hw tx/rx chunk in bytes  eg: 1ms = 8bytes */
	WP_API_CMD_GET_HW_MTU_MRU, 		/*!< Get hw tx/rx chunk in bytes  eg: 1ms = 8bytes */
	WP_API_CMD_SET_CODEC,  			/*!< Set device codec (ulaw/alaw/slinear) */
	WP_API_CMD_GET_CODEC, 			/*!< Get configured device codec (ulaw/alaw/slinear) */
	WP_API_CMD_SET_POWER_LEVEL,		/*!< Not implemented */
	WP_API_CMD_GET_POWER_LEVEL, 	/*!< Not implemented */
	WP_API_CMD_TOGGLE_RX,  			/*!< Disable/Enable RX on this device */
	WP_API_CMD_TOGGLE_TX,  			/*!< Disable/Enable TX on this device */
	WP_API_CMD_GET_HW_CODING, 		/*!< Get HW coding configuration (ulaw or alaw) */
	WP_API_CMD_SET_HW_CODING, 		/*!< Set HW coding (ulaw or alaw) */
	WP_API_CMD_GET_FULL_CFG,		/*!< Get full device configuration */
	WP_API_CMD_SET_EC_TAP,  		/*!< Not implemented */
	WP_API_CMD_GET_EC_TAP,  		/*!< Not implemented */
	WP_API_CMD_ENABLE_RBS_EVENTS,	/*!< Enable RBS Event monitoring */
	WP_API_CMD_DISABLE_RBS_EVENTS,	/*!< Disable RBS Event monitoring */
	WP_API_CMD_WRITE_RBS_BITS,		/*!< Write RBS bits (ABCD) */
	WP_API_CMD_READ_RBS_BITS,		/*!< Read RBS bits (ABCD) */
	WP_API_CMD_GET_STATS,			/*!< Get device statistics */
	WP_API_CMD_FLUSH_BUFFERS,		/*!< Flush Buffers */
	WP_API_CMD_FLUSH_TX_BUFFERS,	/*!< Flush Tx Buffers */
	WP_API_CMD_FLUSH_RX_BUFFERS,	/*!< Flush Rx Buffers */
	WP_API_CMD_FLUSH_EVENT_BUFFERS, /*!< Flush Event Buffers */
	WP_API_CMD_READ_EVENT,			/*!<  */
	WP_API_CMD_SET_EVENT,			/*!<  */
	WP_API_CMD_SET_RX_GAINS,		/*!<  */
	WP_API_CMD_SET_TX_GAINS,		/*!<  */
	WP_API_CMD_CLEAR_RX_GAINS,		/*!<  */
	WP_API_CMD_CLEAR_TX_GAINS,		/*!<  */
	WP_API_CMD_GET_FE_ALARMS,		/*!<  */
	WP_API_CMD_ENABLE_HWEC,			/*!<  */
	WP_API_CMD_DISABLE_HWEC,		/*!<  */
	WP_API_CMD_SET_FE_STATUS,		/*!<  */
	WP_API_CMD_GET_FE_STATUS,		/*!<  */
	WP_API_CMD_GET_HW_DTMF,			/*!< Get Status of the HW DTMF. Enabled(1) or Disabled(0) */
	WP_API_CMD_DRV_MGMNT,			/*!<  */
	WP_API_CMD_RESET_STATS,			/*!< Reset device statistics */
	WP_API_CMD_DRIVER_VERSION,		/*!< Driver Version */
	WP_API_CMD_FIRMWARE_VERSION,	/*!< Firmware Version */
	WP_API_CMD_CPLD_VERSION,		/*!< CPLD Version */
	WP_API_CMD_OPEN_CNT,			/*!< Open Cnt */
	WP_API_CMD_SET_TX_Q_SIZE,		/*!< Set TX Queue Size */
	WP_API_CMD_GET_TX_Q_SIZE,		/*!< Get TX Queue Size */
	WP_API_CMD_SET_RX_Q_SIZE,		/*!< Set RX Queue Size */
	WP_API_CMD_GET_RX_Q_SIZE,		/*!< Get RX Queue Size */
	WP_API_CMD_EVENT_CTRL,			/*!<  */
	WP_API_CMD_NOTSUPP,				/*!<  */
	WP_API_CMD_SET_RM_RXFLASHTIME,	/*!< Set rxflashtime for FXS */
	WP_API_CMD_SET_IDLE_FLAG,		/*!< Set Idle Flag (char) for a BitStream (Voice) channel */
	WP_API_CMD_GET_HW_EC,			/*!< Check Status of HW Echo Cancelation. Enabled(1) or Disabled(0) */
	WP_API_CMD_GET_HW_FAX_DETECT,	/*!< Check Status of HW Fax Detect. Enabled(1) or Disabled(0) */
	WP_API_CMD_ENABLE_LOOP,			/*!< Remote Loop the channel */
	WP_API_CMD_DISABLE_LOOP,		/*!< Disable remote loop */
	WP_API_CMD_BUFFER_MULTIPLIER,	/*!< Set Buffer Multiplier - for SPAN voice mode only */
	WP_API_CMD_GET_HW_EC_CHAN,		/*!< Get status of hwec for the current timeslot */
	WP_API_CMD_GET_HW_EC_PERSIST,	/*!< Check if hwec persist mode is on or off */
	WP_API_CMD_EC_IOCTL,			/*!< Execute command in HWEC module of the Driver */
	WP_API_CMD_SS7_FORCE_RX,		/*!< Force SS7 Receive */
	WP_API_CMD_SS7_GET_CFG_STATUS,	/*!< Get current ss7 configuration status */

	/* Add only debugging commands here */
    WP_API_CMD_GEN_FIFO_ERR_TX=500,
	WP_API_CMD_GEN_FIFO_ERR_RX,
	WP_API_CMD_START_CHAN_SEQ_DEBUG,
	WP_API_CMD_STOP_CHAN_SEQ_DEBUG
};

/*!
  \enum wanpipe_cdev_ctrl_cmds
  \brief Commands used with WANPIPE_IOCTL_CDEV_CTRL IOCTL
 */
enum wanpipe_cdev_ctrl_cmds
{
	WP_CDEV_CMD_SET_DPC_TIMEDIFF_MONITORING_OPTION=1,		/* DPC() monitoring */
	WP_CDEV_CMD_SET_TX_INTERRUPT_TIMEDIFF_MONITORING_OPTION,/* TX ISR() monitoring */
	WP_CDEV_CMD_SET_RX_INTERRUPT_TIMEDIFF_MONITORING_OPTION,/* RX ISR() monitoring */
	WP_CDEV_CMD_PRINT_INTERRUPT_TIMEDIFF_MONITORING_INFO,	/* print ISR() monitoring info to Wanpipelog */
	WP_CDEV_CMD_GET_INTERFACE_NAME							/* Retrun Interface Name to user-mode application */
};

/*!
  \enum wanpipe_api_events
  \brief Events available on wanpipe api device

  The events are can be enabled or disabled by application.
  Events are passed up to the user application
  by the driver.  If event occours and applicatoin has enabled
  such event, then event is passed up to the api device.
  User application will receive a poll() signal idicating
  that event has occoured.

 */
enum wanpipe_api_events
{
	WP_API_EVENT_NONE,				/*!<  */
	WP_API_EVENT_RBS,				/*!< Tx: Enable Disable RBS Opertaion Mode (T1: RBS E1: CAS) */
	WP_API_EVENT_ALARM,				/*!<  */
	WP_API_EVENT_DTMF,				/*!< Enable Disable HW DTMF Detection  */
	WP_API_EVENT_RM_DTMF,			/*!<  */
	WP_API_EVENT_RXHOOK,			/*!<  */
	WP_API_EVENT_RING,				/*!<  */
	WP_API_EVENT_RING_DETECT,		/*!<  */
	WP_API_EVENT_RING_TRIP_DETECT,	/*!<  */
	WP_API_EVENT_TONE,				/*!<  */
	WP_API_EVENT_TXSIG_KEWL,		/*!<  */
	WP_API_EVENT_TXSIG_START,		/*!<  */
	WP_API_EVENT_TXSIG_OFFHOOK,		/*!<  */
	WP_API_EVENT_TXSIG_ONHOOK,		/*!<  */
	WP_API_EVENT_ONHOOKTRANSFER,	/*!<  */
	WP_API_EVENT_SETPOLARITY,		/*!<  */
	WP_API_EVENT_BRI_CHAN_LOOPBACK,	/*!<  */
	WP_API_EVENT_LINK_STATUS,		/*!<  */
	WP_API_EVENT_MODEM_STATUS,		/*!<  */
	WP_API_EVENT_POLARITY_REVERSE,	/*!<  */
	WP_API_EVENT_FAX_DETECT,		/*!< Enable Disable HW Fax Detection */
	WP_API_EVENT_SET_RM_TX_GAIN,		/*!< Set Tx Gain for FXO/FXS */
	WP_API_EVENT_SET_RM_RX_GAIN,		/*!< Set Rx Gain for FXO/FXS */
	WP_API_EVENT_FAX_1100,         	/*!< IN: FAX 1100 Tone event */
	WP_API_EVENT_FAX_2100,          /*!< IN: FAX 2100 Tone event */
	WP_API_EVENT_FAX_2100_WSPR,     /*!< IN: FAX 2100 WSPR Tone event */
};


/*!
  \def WP_API_EVENT_SET
  \brief Option to write a particular command
  \def WP_API_EVENT_GET
  \brief Option to read a particular command
  \def WP_API_EVENT_ENABLE
  \brief Option to enable command
  \def WP_API_EVENT_DISABLE
  \brief Option to disable command
  \def WP_API_EVENT_MODE_DECODE
  \brief Decode disable/enable command

*/
#define WP_API_EVENT_SET		0x01
#define WP_API_EVENT_GET		0x02
#define WP_API_EVENT_ENABLE		0x01
#define WP_API_EVENT_DISABLE	0x02

#define WP_API_EVENT_MODE_DECODE(mode)					\
		((mode) == WP_API_EVENT_ENABLE) ? "Enable" :		\
		((mode) == WP_API_EVENT_DISABLE) ? "Disable" :		\
						"(Unknown mode)"


/*!
  \def  WPTDM_A_BIT
  \brief RBS BIT A
  \def  WPTDM_B_BIT
  \brief RBS BIT B
  \def  WPTDM_C_BIT
  \brief RBS BIT C
  \def  WPTDM_C_BIT
  \brief RBS BIT C
 */
#define WPTDM_A_BIT 			WAN_RBS_SIG_A
#define WPTDM_B_BIT 			WAN_RBS_SIG_B
#define WPTDM_C_BIT 			WAN_RBS_SIG_C
#define WPTDM_D_BIT 			WAN_RBS_SIG_D


/*!
  \def WP_API_EVENT_RXHOOK_OFF
  \brief Rx Off Hook indication value
  \def WP_API_EVENT_RXHOOK_ON
  \brief Rx ON Hook indication value
  \def WP_API_EVENT_RXHOOK_FLASH
  \brief Rx WINK FLASH indication value
  \def WP_API_EVENT_RXHOOK_DECODE
  \brief Print out the hook state
*/
#define WP_API_EVENT_RXHOOK_OFF	0x01
#define WP_API_EVENT_RXHOOK_ON	0x02
#define WP_API_EVENT_RXHOOK_FLASH 0x03
#define WP_API_EVENT_RXHOOK_DECODE(state)				\
		((state) == WP_API_EVENT_RXHOOK_OFF) ? "Off-hook" :	\
		((state) == WP_API_EVENT_RXHOOK_FLASH) ? "Flash" :	\
		((state) == WP_API_EVENT_RXHOOK_ON) ? "On-hook" :	\
						"(Unknown state)"
/*!
  \def WP_API_EVENT_RING_PRESENT
  \brief Ring Present Value
  \def WP_API_EVENT_RING_STOP
  \brief Ring Stop Value
  \def WP_API_EVENT_RING_DECODE
  \brief Print out the Ring state
*/
#define WP_API_EVENT_RING_PRESENT	0x01
#define WP_API_EVENT_RING_STOP	0x02
#define WP_API_EVENT_RING_DECODE(state)				\
		((state) == WP_API_EVENT_RING_PRESENT) ? "Ring Present" :	\
		((state) == WP_API_EVENT_RING_STOP) ? "Ring Stop" :	\
						"(Unknown state)"

/*!
  \def WP_API_EVENT_RING_TRIP_PRESENT
  \brief Ring Trip Present Value
  \def WP_API_EVENT_RING_TRIP_STOP
  \brief Ring Trip Stop Value
  \def WP_API_EVENT_RING_TRIP_DECODE
  \brief Print out the Ring Trip state
*/
#define WP_API_EVENT_RING_TRIP_PRESENT	0x01
#define WP_API_EVENT_RING_TRIP_STOP	0x02
#define WP_API_EVENT_RING_TRIP_DECODE(state)				\
		((state) == WP_API_EVENT_RING_TRIP_PRESENT) ? "Ring Present" :	\
		((state) == WP_API_EVENT_RING_TRIP_STOP) ? "Ring Stop" :	\
						"(Unknown state)"
/*Link Status */
/*!
  \def WP_API_EVENT_LINK_STATUS_CONNECTED
  \brief Link Connected state value
  \def WP_API_EVENT_LINK_STATUS_DISCONNECTED
  \brief Link Disconnected state value
  \def WP_API_EVENT_LINK_STATUS_DECODE
  \brief Print out the Link state
*/
#define WP_API_EVENT_LINK_STATUS_CONNECTED		0x01
#define WP_API_EVENT_LINK_STATUS_DISCONNECTED	0x02
#define WP_API_EVENT_LINK_STATUS_DECODE(status)					\
		((status) == WP_API_EVENT_LINK_STATUS_CONNECTED) ? "Connected" :		\
		((status) == WP_API_EVENT_LINK_STATUS_DISCONNECTED)  ? "Disconnected" :		\
							"Unknown"

/*Polarity Reversal for FXO  */
/*!
  \def WP_API_EVENT_POL_REV_POS_TO_NEG
  \brief Polarity Reversal Postive to Negative
  \def WP_API_EVENT_POL_REV_NEG_TO_POS
  \brief Polarity Reversal Negative to Positive
  \def WP_API_EVENT_POLARITY_REVERSE_DECODE
  \brief Print out the Polarity state
*/
#define WP_API_EVENT_POL_REV_POS_TO_NEG		0x01
#define WP_API_EVENT_POL_REV_NEG_TO_POS		0x02
#define WP_API_EVENT__POL_REV_NEG_TO_POS	WP_API_EVENT_POL_REV_NEG_TO_POS
#define WP_API_EVENT_POLARITY_REVERSE_DECODE(polarity_reverse)					\
		((polarity_reverse) == WP_API_EVENT_POL_REV_POS_TO_NEG) ? "+ve to -ve" :		\
		((polarity_reverse) == WP_API_EVENT_POL_REV_NEG_TO_POS)  ? "-ve to +ve" :		\
							"Unknown"
/*!
  \def WP_API_EVENT_TONE_DIAL
  \brief Dial tone value
  \def WP_API_EVENT_TONE_BUSY
  \brief Busy tone value
  \def WP_API_EVENT_TONE_RING
  \brief Ring tone value
  \def WP_API_EVENT_TONE_CONGESTION
  \brief Contestion tone value
  \def WP_API_EVENT_TONE_DTMF
  \brief Define tone indicates TONE type DTMF
  \def WP_API_EVENT_TONE_FAXCALLING
  \brief Define tone indicates TONE type FAXCALLING 
  \def WP_API_EVENT_TONE_FAXCALLED
  \brief Define tone indicates TONE type FAXCALLED 
*/
#define	WP_API_EVENT_TONE_DIAL		0x01
#define	WP_API_EVENT_TONE_BUSY		0x02
#define	WP_API_EVENT_TONE_RING		0x03
#define	WP_API_EVENT_TONE_CONGESTION	0x04
#define WP_API_EVENT_TONE_DTMF		0x05
#define WP_API_EVENT_TONE_FAXCALLING	0x06
#define WP_API_EVENT_TONE_FAXCALLED	0x07

/* BRI channels list */
/*!
  \def WAN_BRI_BCHAN1
  \brief BRI Channel 1
  \def WAN_BRI_BCHAN2
  \brief BRI Channel 2
  \def WAN_BRI_DCHAN
  \brief BRI Dchan Channel
*/
#define	WAN_BRI_BCHAN1		0x01
#define	WAN_BRI_BCHAN2		0x02
#define	WAN_BRI_DCHAN		0x03


/*!
  \def  WP_PORT_NAME_FORM
  \brief String define of a wanpipe port name

  \def WP_INTERFACE_NAME_FORM
  \brief String define of a wanpipe interface name
*/
#define WP_PORT_NAME_FORM		"wanpipe%d"
#define WP_INTERFACE_NAME_FORM	"wanpipe%d_if%d"
#define WP_CTRL_DEV_NAME		"wanpipe_ctrl"
#define WP_CONFIG_DEV_NAME		"wanpipe"
#define WP_TIMER_DEV_NAME_FORM	"wanpipe_timer%d"
#define WP_LOGGER_DEV_NAME		"wanpipe_logger"

#pragma pack(1)


/*!
  \struct wanpipe_chan_stats
  \brief TDM API channel statistics

  \typedef wanpipe_chan_stats_t
*/
typedef struct wanpipe_chan_stats
{
	unsigned int	rx_packets;		/* total packets received	*/
	unsigned int	tx_packets;		/* total packets transmitted	*/
	unsigned int	rx_bytes;		/* total bytes received 	*/
	unsigned int	tx_bytes;		/* total bytes transmitted	*/

	unsigned int	rx_errors;		/* total counter of receiver errors. see 'detailed rx_errors' */
	unsigned int	tx_errors;		/* total counter of transmitter errors. see 'detailed tx_errors' */

	unsigned int	rx_dropped;		/* Counter of dropped received packets. 
									 * Possible cause: internal driver error, check Driver Message Log. */
	unsigned int	tx_dropped;		/* Counter of dropped transmit packets. 
									 * Possible cause: internal driver error, check Driver Message Log. */
	unsigned int	multicast;		/* Linux Network Interface: multicast packets received	*/
	unsigned int	collisions;		/* Linux Network Interface: eth collisions counter */

	/* detailed rx_errors: */
	unsigned int	rx_length_errors;	/* Received HDLC frame is longer than the MRU. 
										 * Usualy means "no closing HDLC flag". */
	unsigned int	rx_over_errors;		/* Receiver ring buff overflow at driver level (not at hardware) */
	unsigned int	rx_crc_errors;		/* Received HDLC frame with CRC error.
										 * Possible cause: bit errors on the line. */
	unsigned int	rx_frame_errors;	/* Received HDLC frame alignment error */
	unsigned int	rx_fifo_errors;		/* Receiver FIFO overrun at hardware level.
										 * Possible cause: driver was too slow to re-program Rx DMA. */
	unsigned int	rx_missed_errors;	/* deprecated */

	/* detailed tx_errors */
	unsigned int	tx_aborted_errors;	/* deprecated. The same as tx_fifo_errors. */
	unsigned int	tx_carrier_errors;	/* counter of times transmitter was not in operational state */

	unsigned int	tx_fifo_errors;		/* Transmitter FIFO overrun at hardware level.
										 * Possible cause: driver was too slow to re-program Tx DMA. */
	unsigned int	tx_heartbeat_errors;/* deprecated */
	unsigned int	tx_window_errors;	/* deprecated */

	unsigned int	tx_idle_packets;	/* Counter of Idle Tx Data transmissions.
										 * Occurs in Voice/BitStream mode.
										 * Cause: User application supplies data too slowly. */

	unsigned int 	errors;		/* Total of ALL errors. */

	/* current state of transmitter queue */
	unsigned int	current_number_of_frames_in_tx_queue;
	unsigned int	max_tx_queue_length;

	/* current state of receiver queue */
	unsigned int	current_number_of_frames_in_rx_queue;
	unsigned int	max_rx_queue_length;

	/* current state of Event queue */
	unsigned int	max_event_queue_length;
	unsigned int	current_number_of_events_in_event_queue;

	unsigned int	rx_events;			/* Total counter of all API Events. (On/Off Hook, DTMF, Ring...)*/
	unsigned int	rx_events_dropped;	/* Counter of discarded events due to Rx Event queue being full. 
										 * Possible cause: application too slow to "read" the events.*/
	unsigned int	rx_events_tone;		/* Counter of Tone Events, such as DTMF. */

	/* HDLC-level  abort condition */
	unsigned int	rx_hdlc_abort_counter; /* HDLC-level abort is considered an error by Sangoma HDLC engine.
											* But, since it is a part of HDLC standard, an application may choose to ignore it. */
	
}wanpipe_chan_stats_t;


#define WP_AFT_CHAN_ERROR_STATS(chan_stats,var) chan_stats.var++;chan_stats.errors++

#define WP_AFT_RX_ERROR_SUM(chan_stats) chan_stats.rx_errors+ \
										chan_stats.rx_dropped+ \
										chan_stats.rx_length_errors + \
										chan_stats.rx_crc_errors + \
										chan_stats.rx_frame_errors + \
										chan_stats.rx_fifo_errors + \
										chan_stats.rx_missed_errors + \
										chan_stats.rx_hdlc_abort_counter
											 
#define WP_AFT_TX_ERROR_SUM(chan_stats) chan_stats.tx_errors+ \
										chan_stats.tx_dropped + \
										chan_stats.tx_aborted_errors + \
										chan_stats.tx_carrier_errors + \
										chan_stats.tx_fifo_errors+ \
										chan_stats.tx_heartbeat_errors + \
										chan_stats.tx_window_errors 
										
typedef struct _DRIVER_VERSION {
	unsigned int major;
	unsigned int minor;
	unsigned int minor1; 
	unsigned int minor2; 
}wan_driver_version_t, DRIVER_VERSION, *PDRIVER_VERSION;

#define WANPIPE_API_CMD_SZ 512
#define WANPIPE_API_DEV_CFG_MAX_SZ 337

/* The the union size is max-cmd-result-span-chan-data_len */
#define WANPIPE_API_CMD_SZ_UNION  WANPIPE_API_CMD_SZ - (sizeof(unsigned int)*3) - (sizeof(unsigned char)*2)


/* Each time you add a parameter to the wanpipe_api_dev_cfg_t you must update
   WANPIPE_API_CMD_RESERVED_SZ as well as WANPIPE_API_DEV_CFG_SZ */

/*                                         rxflashtime     hw_ec,hw_fax,loop */ 
#define WANPIPE_API_CMD_RESERVED_SZ 128 - sizeof(int)*1 - sizeof(char)*3

/* The sizeof WANPIPE_API_DEV_CFG_SZ must account for every variable in
   wanpipe_api_dev_cfg_t strcture */

/*                                20 int             4 chars */
#define WANPIPE_API_DEV_CFG_SZ sizeof(int)*20 + sizeof(char)*4 + WANPIPE_API_CMD_RESERVED_SZ + sizeof(wanpipe_chan_stats_t)


/*!
  \struct wanpipe_api_dev_cfg
  \brief TDM API Device Configuration Structure

  Contains the configuration of a wanpipe api device.
  This configuration should only be used for CHAN operation mode.

  \typedef wanpipe_api_dev_cfg_t
 */
typedef struct wanpipe_api_dev_cfg
{
		unsigned int hw_tdm_coding;	/* Set/Get HW TDM coding: uLaw muLaw */
		unsigned int hw_mtu_mru;	/* Set/Get HW TDM MTU/MRU */
		unsigned int usr_period;	/* Set/Get User Period in ms */
		unsigned int tdm_codec;		/* Set/Get TDM Codec: SLinear */
		unsigned int power_level;	/* Set/Get Power level treshold */
		unsigned int rx_disable;	/* Enable/Disable Rx */
		unsigned int tx_disable;	/* Enable/Disable Tx */
		unsigned int usr_mtu_mru;	/* Set/Get User TDM MTU/MRU */
		unsigned int ec_tap;		/* Echo Cancellation Tap */
		unsigned int rbs_poll;		/* Enable/Disable RBS Polling */
		unsigned int rbs_rx_bits;	/* Rx RBS Bits */
		unsigned int rbs_tx_bits;	/* Tx RBS Bits */
		unsigned int hdlc;			/* HDLC based device */
		unsigned int idle_flag;		/* IDLE flag to Tx */
		unsigned int fe_alarms;		/* FE Alarms detected */
		unsigned char fe_status;	/* FE status - Connected or Disconnected */
		unsigned int hw_dtmf;		/* HW DTMF enabled */
		unsigned int open_cnt;		/* Open cnt */
		unsigned int rx_queue_sz;
		unsigned int tx_queue_sz;
		/* Any new paramets should decrement the reserved size */
		unsigned int rxflashtime;	/* Set Rxflash time for Wink-Flash */
		unsigned char hw_ec;
		unsigned char hw_fax;
		unsigned char loop;

		unsigned char reserved[WANPIPE_API_CMD_RESERVED_SZ];
		/* Duplicate the structure below */
		wanpipe_chan_stats_t stats;	/* TDM Statistics */
}wanpipe_api_dev_cfg_t;


/*!
  \struct wanpipe_api_cmd
  \brief Wanpipe API Device Command Structure used with WANPIPE_IOCTL_API_CMD

  Wanpipe API Commands structure used to execute WANPIPE_IOCTL_API_CMD iocl commands
  All commands are defined in: 
  enum wanpipe_api_cmds
  enum wanpipe_api_events

  \typedef wanpipe_api_cmd_t
 */
typedef struct wanpipe_api_cmd
{
	unsigned int cmd;		/*!< Command defined in enum wanpipe_api_cmds */
	unsigned int result;	/*!< Result defined in: enum SANG_STATUS or SANG_STATUS_t */
	unsigned char span;		/*!< span value, integer 1 to 255 */
	unsigned char chan;		/*!< chan value, integer 1 to 32 */

	union {
		struct {
			unsigned int hw_tdm_coding;	/*!< Set/Get HW TDM coding: uLaw muLaw */
			unsigned int hw_mtu_mru;	/*!< Set/Get HW TDM MTU/MRU */
			unsigned int usr_period;	/*!< Set/Get User Period in ms */
			unsigned int tdm_codec;		/*!< Set/Get TDM Codec: SLinear */
			unsigned int power_level;	/*!< Set/Get Power level treshold */
			unsigned int rx_disable;	/*!< Enable/Disable Rx */
			unsigned int tx_disable;	/*!< Enable/Disable Tx */
			unsigned int usr_mtu_mru;	/*!< Set/Get User TDM MTU/MRU */
			unsigned int ec_tap;		/*!< Echo Cancellation Tap */
			unsigned int rbs_poll;		/*!< Enable/Disable RBS Polling */
			unsigned int rbs_rx_bits;	/*!< Rx RBS Bits */
			unsigned int rbs_tx_bits;	/*!< Tx RBS Bits */
			unsigned int hdlc;			/*!< HDLC based device */
			unsigned int idle_flag;		/*!< IDLE flag to Tx */
			unsigned int fe_alarms;		/*!< FE Alarms detected */
			unsigned char fe_status;	/*!< FE status - Connected or Disconnected */
  			unsigned int  hw_dtmf;		/*!< HW DTMF enabled */
			unsigned int open_cnt;		/*!< Open cnt */
			unsigned int rx_queue_sz;	/*!< Rx queue size */
			unsigned int tx_queue_sz;	/*!< Tx queue size */
			/* Any new paramets should decrement the reserved size */
			unsigned int rxflashtime;	/*!< Set Rxflash time for Wink-Flash */
			unsigned char hw_ec;
			unsigned char hw_fax;
			unsigned char loop;

			unsigned char reserved[WANPIPE_API_CMD_RESERVED_SZ];
			wanpipe_chan_stats_t stats;	/*!< Wanpipe API Statistics */
		};
		wp_api_event_t event;	/*!< Wanpipe API Event */
		wan_driver_version_t version;
		wan_iovec_list_t	iovec_list;
		wan_api_ss7_cfg_status_t   ss7_cfg_status;
		struct {
			unsigned char data[WANPIPE_API_CMD_SZ_UNION];
			unsigned int data_len;
		};
	};
}wanpipe_api_cmd_t;

/* \brief Initialize 'span' in wanpipe_api_cmd_t */
#define WANPIPE_API_CMD_INIT_SPAN(wp_cmd_ptr, span_no)	((wp_cmd_ptr)->span = (unsigned char)span_no)

/* \brief Initialize 'channel' in wanpipe_api_cmd_t */
#define WANPIPE_API_CMD_INIT_CHAN(wp_cmd_ptr, chan_no)	((wp_cmd_ptr)->chan = (unsigned char)chan_no)

/*!
  \struct wanpipe_api_callbacks
  \brief Wanpipe API Callback Structure

  \typedef wanpipe_api_callbacks_t
*/
typedef struct wanpipe_api_callbacks
{
	int (*wp_rbs_event)(sng_fd_t fd, unsigned char rbs_bits);
	int (*wp_dtmf_event)(sng_fd_t fd, unsigned char dtmf, unsigned char type, unsigned char port);
	int (*wp_rxhook_event)(sng_fd_t fd, unsigned char hook_state);
	int (*wp_ring_detect_event)(sng_fd_t fd, unsigned char ring_state);
	int (*wp_ring_trip_detect_event)(sng_fd_t fd, unsigned char ring_state);
	int (*wp_fe_alarm_event)(sng_fd_t fd, unsigned int fe_alarm_event);
	int (*wp_link_status_event)(sng_fd_t fd, unsigned int link_status_event);
}wanpipe_api_callbacks_t;

/*!
  \struct wanpipe_api
  \brief Wanpipe API Command Structure

  The Wanpipe API Command structure is used to execute
  enum wanpipe_api_events
  enum wanpipe_api_cmds

  \typedef wanpipe_api_t
*/
typedef struct wanpipe_api
{
	wanpipe_api_cmd_t	wp_cmd;			/*!< Contains API command info */
	wanpipe_api_callbacks_t wp_callback;	/*!< Deprecated: Callbacks used by libsangoma */
}wanpipe_api_t;

/* \brief Initialize 'span' in wanpipe_api_t->wp_cmd */
#define WANPIPE_API_INIT_SPAN(wanpipe_api_ptr, span_no)	WANPIPE_API_CMD_INIT_SPAN(&wanpipe_api_ptr->wp_cmd, span_no)

/* \brief Initialize 'channel' in wanpipe_api_t->wp_cmd */
#define WANPIPE_API_INIT_CHAN(wanpipe_api_ptr, chan_no)	WANPIPE_API_CMD_INIT_CHAN(&wanpipe_api_ptr->wp_cmd, chan_no)

#pragma pack()


/***************************************************//**
 *******************************************************/

#endif
