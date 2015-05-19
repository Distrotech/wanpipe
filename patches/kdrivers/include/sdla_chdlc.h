/*************************************************************************
 sdla_chdlc.h	Sangoma Cisco HDLC firmware API definitions

 Author:      	Gideon Hack
		Nenad Corbic <ncorbic@sangoma.com>	

 Copyright:	(c) 1995-2000 Sangoma Technologies Inc.

		This program is free software; you can redistribute it and/or
		modify it under the term of the GNU General Public License
		as published by the Free Software Foundation; either version
		2 of the License, or (at your option) any later version.

===========================================================================
  Oct 04, 1999  Nenad Corbic    Updated API support
  Jun 02, 1999  Gideon Hack     Changes for S514 usage.
  Oct 28, 1998	Jaspreet Singh	Made changes for Dual Port CHDLC.
  Jun 11, 1998	David Fong	Initial version.
===========================================================================

 Organization
	- Compatibility notes
	- Constants defining the shared memory control block (mailbox)
	- Interface commands
	- Return code from interface commands
	- Constants for the commands (structures for casting data)
	- UDP Management constants and structures

*************************************************************************/

#ifndef _SDLA_CHDLC_H
#define _SDLA_CHDLC_H

#if defined(__LINUX__)
# include "if_wanpipe.h"
#endif

#if defined (__KERNEL__)
# include "wanpipe_sppp_iface.h"
#endif

/*------------------------------------------------------------------------
   Notes:

	All structres defined in this file are byte-aligned.  

	Compiler	Platform
	------------------------
	GNU C		Linux

------------------------------------------------------------------------*/

#pragma pack(1)

/* ----------------------------------------------------------------------------
 *        Constants defining the shared memory control block (mailbox)
 * --------------------------------------------------------------------------*/

#define PRI_BASE_ADDR_MB_STRUCT 	0xE000 	/* the base address of the mailbox structure on the adapter */
#define SEC_BASE_ADDR_MB_STRUCT 	0xE800 	/* the base address of the mailbox structure on the adapter */
#define SIZEOF_MB_DATA_BFR		2032	/* the size of the actual mailbox data area */
#define NUMBER_MB_RESERVED_BYTES	0x0B	/* the number of reserved bytes in the mailbox header area */


#define MIN_LGTH_CHDLC_DATA_CFG  	300 	/* min length of the CHDLC data field (for configuration purposes) */
#define PRI_MAX_NO_DATA_BYTES_IN_FRAME  15354 /* PRIMARY - max length of the CHDLC data field */

#if 0
typedef struct {
	unsigned char opp_flag ;			/* the opp flag */
	unsigned char command ;			/* the user command */
	unsigned short buffer_length ;		/* the data length */
  	unsigned char return_code ;		/* the return code */
	unsigned char MB_reserved[NUMBER_MB_RESERVED_BYTES] ;	/* reserved for later */
	unsigned char data[SIZEOF_MB_DATA_BFR] ;	/* the data area */
} CHDLC_MAILBOX_STRUCT;
#endif


/* ----------------------------------------------------------------------------
 *                        Interface commands
 * --------------------------------------------------------------------------*/

/* global interface commands */
#define READ_GLOBAL_EXCEPTION_CONDITION	0x01
#define SET_GLOBAL_CONFIGURATION	0x02
/*#define READ_GLOBAL_CONFIGURATION	0x03*/
#define READ_GLOBAL_STATISTICS		0x04
#define FLUSH_GLOBAL_STATISTICS		0x05
#define SET_MODEM_STATUS		0x06	/* set status of DTR or RTS */
#define READ_MODEM_STATUS		0x07	/* read status of CTS and DCD */

#undef  READ_COMMS_ERROR_STATS
#define READ_COMMS_ERROR_STATS		0x08

#undef 	FLUSH_COMMS_ERROR_STATS
#define FLUSH_COMMS_ERROR_STATS		0x09
#define SET_TRACE_CONFIGURATION		0x0A	/* set the line trace config */
#define READ_TRACE_CONFIGURATION	0x0B	/* read the line trace config */
#define READ_TRACE_STATISTICS		0x0C	/* read the trace statistics */
#define FLUSH_TRACE_STATISTICS		0x0D	/* flush the trace statistics */
#define FT1_MONITOR_STATUS_CTRL		0x1C	/* set the status of the S508/FT1 monitoring */
#define SET_FT1_CONFIGURATION		0x18	/* set the FT1 configuration */
#define READ_FT1_CONFIGURATION		0x19	/* read the FT1 configuration */
#define TRANSMIT_ASYNC_DATA_TO_FT1	0x1A	/* output asynchronous data to the FT1 */
#define RECEIVE_ASYNC_DATA_FROM_FT1	0x1B	/* receive asynchronous data from the FT1 */
#define FT1_MONITOR_STATUS_CTRL		0x1C	/* set the status of the FT1 monitoring */

#define READ_FT1_OPERATIONAL_STATS	0x1D	/* read the S508/FT1 operational statistics */

#undef 	SET_FT1_MODE
#define SET_FT1_MODE			0x1E	/* set the operational mode of the S508/FT1 module */

/* CHDLC-level interface commands */
#define READ_CHDLC_CODE_VERSION		0x20	
#define READ_CHDLC_EXCEPTION_CONDITION	0x21	/* read exception condition from the adapter */
#define SET_CHDLC_CONFIGURATION		0x22
#define READ_CHDLC_CONFIGURATION	0x23
#define ENABLE_CHDLC_COMMUNICATIONS	0x24
#define DISABLE_CHDLC_COMMUNICATIONS	0x25
#define READ_CHDLC_LINK_STATUS		0x26

#define SET_MISC_TX_RX_PARAMETERS	0x29	
/* set miscellaneous Tx/Rx parameters */
#define START_BAUD_CALIBRATION		0x2A	
/* calibrate an external clock */
#define READ_BAUD_CALIBRATION_RESULT	0x2B	

#define RESET_ESCC			0x2C
/* read the result of a baud rate calibration */

#define SET_CHDLC_INTERRUPT_TRIGGERS	0x30	
/* set application interrupt triggers */
#define READ_CHDLC_INTERRUPT_TRIGGERS	0x31	
/* read application interrupt trigger configuration */

/* Special UDP drivers management commands */
#define CPIPE_ROUTER_UP_TIME				WANPIPEMON_ROUTER_UP_TIME
#define CPIPE_ENABLE_TRACING				WANPIPEMON_ENABLE_TRACING
#define CPIPE_DISABLE_TRACING				WANPIPEMON_DISABLE_TRACING
#define CPIPE_GET_TRACE_INFO				WANPIPEMON_GET_TRACE_INFO
#define DIGITAL_LOOPTEST					WANPIPEMON_DIGITAL_LOOPTEST
#define CPIPE_GET_IBA_DATA					WANPIPEMON_GET_IBA_DATA
#define CPIPE_FT1_READ_STATUS				WANPIPEMON_FT1_READ_STATUS
#define CPIPE_DRIVER_STAT_IFSEND			WANPIPEMON_DRIVER_STAT_IFSEND
#define CPIPE_DRIVER_STAT_INTR				WANPIPEMON_DRIVER_STAT_INTR
#define CPIPE_DRIVER_STAT_GEN				WANPIPEMON_DRIVER_STAT_GEN	
#define CPIPE_FLUSH_DRIVER_STATS			WANPIPEMON_FLUSH_DRIVER_STATS


/* Driver specific commands for API */
#define	CHDLC_READ_TRACE_DATA		0xE4	/* read trace data */
#define TRACE_ALL                       0x00
#define TRACE_PROT			0x01
#define TRACE_DATA			0x02

#define DISCARD_RX_ERROR_FRAMES	0x0001

/* ----------------------------------------------------------------------------
 *                     Return codes from interface commands
 * --------------------------------------------------------------------------*/

#define COMMAND_OK				0x00

/* return codes from global interface commands */
#define NO_GLOBAL_EXCEP_COND_TO_REPORT		0x01	/* there is no CHDLC exception condition to report */
#define LGTH_GLOBAL_CFG_DATA_INVALID		0x01	/* the length of the passed global configuration data is invalid */
#define LGTH_TRACE_CFG_DATA_INVALID		0x01	/* the length of the passed trace configuration data is invalid */
#define IRQ_TIMEOUT_VALUE_INVALID		0x02	/* an invalid application IRQ timeout value was selected */
#define TRACE_CONFIG_INVALID			0x02	/* the passed line trace configuration is invalid */
#define ADAPTER_OPERATING_FREQ_INVALID		0x03	/* an invalid adapter operating frequency was selected */
#define TRC_DEAC_TMR_INVALID			0x03	/* the trace deactivation timer is invalid */
#define S508_FT1_ADPTR_NOT_PRESENT		0x0C	/* the S508/FT1 adapter is not present */
#define INVALID_FT1_STATUS_SELECTION            0x0D    /* the S508/FT1 status selection is invalid */
#define FT1_OP_STATS_NOT_ENABLED		0x0D    /* the FT1 operational statistics have not been enabled */
#define FT1_OP_STATS_NOT_AVAILABLE		0x0E    /* the FT1 operational statistics are not currently available */
#define S508_FT1_MODE_SELECTION_BUSY		0x0E	/* the S508/FT1 adapter is busy selecting the operational mode */

/* return codes from command READ_GLOBAL_EXCEPTION_CONDITION */
#define EXCEP_MODEM_STATUS_CHANGE		0x10		/* a modem status change occurred */
#define EXCEP_TRC_DISABLED			0x11		/* the trace has been disabled */
#define EXCEP_IRQ_TIMEOUT			0x12		/* IRQ timeout */
#define EXCEP_CSU_DSU_STATE_CHANGE		0x16

/* return codes from CHDLC-level interface commands */
#define NO_CHDLC_EXCEP_COND_TO_REPORT		0x21	/* there is no CHDLC exception condition to report */
#define CHDLC_COMMS_DISABLED			0x21	/* communications are not currently enabled */
#define CHDLC_COMMS_ENABLED			0x21	/* communications are currently enabled */
#define DISABLE_CHDLC_COMMS_BEFORE_CFG		0x21	/* CHDLC communications must be disabled before setting the configuration */
#define ENABLE_CHDLC_COMMS_BEFORE_CONN		0x21	/* communications must be enabled before using the CHDLC_CONNECT conmmand */
#define CHDLC_CFG_BEFORE_COMMS_ENABLED		0x22	/* perform a SET_CHDLC_CONFIGURATION before enabling comms */
#define LGTH_CHDLC_CFG_DATA_INVALID 		0x22	/* the length of the passed CHDLC configuration data is invalid */
#define LGTH_INT_TRIGGERS_DATA_INVALID		0x22	/* the length of the passed interrupt trigger data is invalid */
#define INVALID_IRQ_SELECTED			0x23	/* in invalid IRQ was selected in the SET_CHDLC_INTERRUPT_TRIGGERS */
#define INVALID_CHDLC_CFG_DATA			0x23	/* the passed CHDLC configuration data is invalid */
#define IRQ_TMR_VALUE_INVALID			0x24	/* an invalid application IRQ timer value was selected */
#define LARGER_PERCENT_TX_BFR_REQUIRED		0x24	/* a larger Tx buffer percentage is required */
#define LARGER_PERCENT_RX_BFR_REQUIRED		0x25	/* a larger Rx buffer percentage is required */
#define S514_BOTH_PORTS_SAME_CLK_MODE		0x26	/* S514 - both ports must have same clock mode */
#define BAUD_CALIBRATION_NOT_DONE		0x29	/* a baud rate calibration has not been performed */
#define BUSY_WITH_BAUD_CALIBRATION		0x2A	/* adapter busy with a baud rate calibration */
#define BAUD_CAL_FAILED_NO_TX_CLK		0x2B	/* baud rate calibration failed (no transmit clock) */
#define BAUD_CAL_FAILED_BAUD_HI			0x2C	/* baud rate calibration failed (baud too high) */
#define CANNOT_DO_BAUD_CAL						0x2D	/* a baud rate calibration cannot be performed */

#define INVALID_CMND_HDLC_STREAM_MODE           0x4E    /* the CHDLC interface command is invalid for HDLC streaming mode */
#define INVALID_CHDLC_COMMAND			0x4F	/* the defined CHDLC interface command is invalid */

/* return codes from command READ_CHDLC_EXCEPTION_CONDITION */
#define EXCEP_LINK_ACTIVE			0x30	/* the CHDLC link has become active */
#define EXCEP_LINK_INACTIVE_MODEM		0x31	/* the CHDLC link has become inactive (modem status) */
#define EXCEP_LINK_INACTIVE_KPALV		0x32	/* the CHDLC link has become inactive (keepalive status) */
#define EXCEP_IP_ADDRESS_DISCOVERED		0x33	/* the IP address has been discovered */
#define EXCEP_LOOPBACK_CONDITION		0x34	/* a loopback condition has occurred */


/* return code from command CHDLC_SEND_WAIT and CHDLC_SEND_NO_WAIT */
#define LINK_DISCONNECTED			0x21
#define NO_TX_BFRS_AVAIL			0x24

typedef struct {
	unsigned char modem_status;
} MODEM_STATUS_STRUCT;

#define SET_MODEM_DTR_HIGH	0x01
#define SET_MODEM_RTS_HIGH	0x02

#define MODEM_CTS_MASK	0x20
#define MODEM_DCD_MASK	0x08

/* ----------------------------------------------------------------------------
 * Constants for the SET_GLOBAL_CONFIGURATION/READ_GLOBAL_CONFIGURATION commands
 * --------------------------------------------------------------------------*/

/* the global configuration structure */
typedef struct {
	unsigned short adapter_config_options ;	/* adapter config options */
	unsigned short app_IRQ_timeout ;		/* application IRQ timeout */
	unsigned int adapter_operating_frequency ;	/* adapter operating frequency */
	unsigned short frame_transmit_timeout ;
} GLOBAL_CONFIGURATION_STRUCT;

/* settings for the 'app_IRQ_timeout' */
#define MAX_APP_IRQ_TIMEOUT_VALUE	5000	/* the maximum permitted IRQ timeout */



/* ----------------------------------------------------------------------------
 *             Constants for the READ_GLOBAL_STATISTICS command
 * --------------------------------------------------------------------------*/

/* the global statistics structure */
typedef struct {
	unsigned short app_IRQ_timeout_count ;
} GLOBAL_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *             Constants for the READ_COMMS_ERROR_STATS command
 * --------------------------------------------------------------------------*/

/* the communications error statistics structure */
typedef struct {
	unsigned short Rx_overrun_err_count ;
	unsigned short CRC_err_count ;	/* receiver CRC error count */
	unsigned short Rx_abort_count ; 	/* abort frames recvd count */
	unsigned short Rx_dis_pri_bfrs_full_count ;/* receiver disabled */
	unsigned short comms_err_stat_reserved_1 ;/* reserved for later */
	unsigned short sec_Tx_abort_msd_Tx_int_count ; /* secondary - abort frames transmitted count (missed Tx interrupt) */
	unsigned short missed_Tx_und_int_count ;	/* missed tx underrun interrupt count */
        unsigned short sec_Tx_abort_count ;   /*secondary-abort frames tx count */
	unsigned short DCD_state_change_count ; /* DCD state change */
	unsigned short CTS_state_change_count ; /* CTS state change */
} COMMS_ERROR_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *                  Constants used for line tracing
 * --------------------------------------------------------------------------*/

/* the trace configuration structure (SET_TRACE_CONFIGURATION/READ_TRACE_CONFIGURATION commands) */
typedef struct {
	unsigned char trace_config ;		/* trace configuration */
	unsigned short trace_deactivation_timer ;	/* trace deactivation timer */
	unsigned int ptr_trace_stat_el_cfg_struct ;	/* a pointer to the line trace element configuration structure */
} LINE_TRACE_CONFIG_STRUCT;

/* 'trace_config' bit settings */
#define TRACE_INACTIVE		0x00	/* trace is inactive */
#define TRACE_ACTIVE		0x01	/* trace is active */
#define TRACE_DELAY_MODE	0x04	/* operate the trace in delay mode */
#define TRACE_DATA_FRAMES	0x08	/* trace Data frames */
#define TRACE_SLARP_FRAMES	0x10	/* trace SLARP frames */
#define TRACE_CDP_FRAMES	0x20	/* trace CDP frames */

/* the line trace status element configuration structure */
typedef struct {
	unsigned short number_trace_status_elements ;	/* number of line trace elements */
	unsigned int base_addr_trace_status_elements ;	/* base address of the trace element list */
	unsigned int next_trace_element_to_use ;	/* pointer to the next trace element to be used */
	unsigned int base_addr_trace_buffer ;		/* base address of the trace data buffer */
	unsigned int end_addr_trace_buffer ;		/* end address of the trace data buffer */
} TRACE_STATUS_EL_CFG_STRUCT;

/* the line trace status element structure */
typedef struct {
	unsigned char opp_flag ;			/* opp flag */
	unsigned short trace_length ;		/* trace length */
	unsigned char trace_type ;		/* trace type */
	unsigned short trace_time_stamp ;	/* time stamp */
	unsigned short trace_reserved_1 ;	/* reserved for later use */
	unsigned int trace_reserved_2 ;		/* reserved for later use */
	unsigned int ptr_data_bfr ;		/* ptr to the trace data buffer */
} TRACE_STATUS_ELEMENT_STRUCT;

/* "trace_type" bit settings */
#define TRACE_INCOMING 			0x00
#define TRACE_OUTGOINGING 		0x01
#define TRACE_INCOMING_ABORTED 		0x10
#define TRACE_INCOMING_CRC_ERROR 	0x20
#define TRACE_INCOMING_OVERRUN_ERROR 	0x40



/* the line trace statistics structure */
typedef struct {
	unsigned int frames_traced_count ;	/* number of frames traced */
	unsigned int trc_frms_not_recorded_count ;	/* number of trace frames discarded */
} LINE_TRACE_STATS_STRUCT;


/* ----------------------------------------------------------------------------
 *               Constants for the FT1_MONITOR_STATUS_CTRL command
 * --------------------------------------------------------------------------*/

#define DISABLE_FT1_STATUS_STATISTICS	0x00    /* disable the FT1 status and statistics monitoring */
#define ENABLE_READ_FT1_STATUS		0x01    /* read the FT1 operational status */
#define ENABLE_READ_FT1_OP_STATS	0x02    /* read the FT1 operational statistics */
#define FLUSH_FT1_OP_STATS		0x04 	/* flush the FT1 operational statistics */




/* ----------------------------------------------------------------------------
 *               Constants for the SET_CHDLC_CONFIGURATION command
 * --------------------------------------------------------------------------*/

/* the CHDLC configuration structure */
typedef struct {
	unsigned int baud_rate ;		/* the baud rate */	
	unsigned short line_config_options ;	/* line configuration options */
	unsigned short modem_config_options ;	/* modem configration options */
	unsigned short modem_status_timer ;	/* timer for monitoring modem status changes */
	unsigned short CHDLC_API_options ;	/* CHDLC API options */
	unsigned short CHDLC_protocol_options ;	/* CHDLC protocol options */
	unsigned short percent_data_buffer_for_Tx ;	/* percentage data buffering used for Tx */
	unsigned short CHDLC_statistics_options ;	/* CHDLC operational statistics options */
	unsigned short max_CHDLC_data_field_length ;	/* the maximum length of the CHDLC Data field */
	unsigned short transmit_keepalive_timer ;		/* the transmit keepalive timer */
	unsigned short receive_keepalive_timer ;		/* the receive keepalive timer */
	unsigned short keepalive_error_tolerance ;	/* the receive keepalive error tolerance */
	unsigned short SLARP_request_timer ;		/* the SLARP request timer */
	unsigned int IP_address ;			/* the IP address */
	unsigned int IP_netmask ;			/* the IP netmask */
	unsigned int ptr_shared_mem_info_struct ;	/* a pointer to the shared memory area information structure */
	unsigned int ptr_CHDLC_Tx_stat_el_cfg_struct ;	/* a pointer to the transmit status element configuration structure */
	unsigned int ptr_CHDLC_Rx_stat_el_cfg_struct ;	/* a pointer to the receive status element configuration structure */
} CHDLC_CONFIGURATION_STRUCT;

/* settings for the 'line_config_options' */
#define INTERFACE_LEVEL_V35			0x0000 
/* V.35 interface level */

#define INTERFACE_LEVEL_RS232			0x0001 
/* RS-232 interface level */

#define NRZI_ENCODING				0x0010 
/* NRZI data encoding */

#define IDLE_MARK				0x0020 
/* idle line condition is mark (not flags) */


/* settings for the 'modem_config_options' */

#define DONT_RAISE_DTR_RTS_ON_EN_COMMS		0x0001
/* don't automatically raise DTR and RTS when performing an
   ENABLE_CHDLC_COMMUNICATIONS command */

#define DONT_REPORT_CHG_IN_MODEM_STAT 		0x0002
/* don't report changes in modem status to the application */

#define SWITCHED_CTS_RTS			0x0010 
/* switched CTS/RTS */


/* bit settings for the 'CHDLC_protocol_options' byte */

#define IGNORE_DCD_FOR_LINK_STAT		0x0001
/* ignore DCD in determining the CHDLC link status */

#define IGNORE_CTS_FOR_LINK_STAT		0x0002
/* ignore CTS in determining the CHDLC link status */

#define IGNORE_KPALV_FOR_LINK_STAT		0x0004
/* ignore keepalive frames in determining the CHDLC link status */ 

#define INSTALL_FAST_INT_HANDLERS		0x1000 
/* install 'fast' interrupt handlers for improved dual-port perf. */

#define USE_32_BIT_CRC				0x2000 
/* use a 32-bit CRC */

#define SINGLE_TX_BUFFER			0x4000 
/* configure a single transmit buffer */

#define HDLC_STREAMING_MODE			0x8000

/*   settings for the 'CHDLC_statistics_options' */

#define CHDLC_TX_DATA_BYTE_COUNT_STAT		0x0001
/* record the number of Data bytes transmitted */

#define CHDLC_RX_DATA_BYTE_COUNT_STAT		0x0002
/* record the number of Data bytes received */

#define CHDLC_TX_THROUGHPUT_STAT		0x0004
/* compute the Data frame transmit throughput */

#define CHDLC_RX_THROUGHPUT_STAT		0x0008
/* compute the Data frame receive throughput */


/* permitted minimum and maximum values for setting the CHDLC configuration */
#define PRI_MAX_BAUD_RATE_S508	2666666 /* PRIMARY   - maximum baud rate (S508) */
#define SEC_MAX_BAUD_RATE_S508	258064 	/* SECONDARY - maximum baud rate (S508) */
#define PRI_MAX_BAUD_RATE_S514  2750000 /* PRIMARY   - maximum baud rate (S508) */
#define SEC_MAX_BAUD_RATE_S514  515625  /* SECONDARY - maximum baud rate (S508) */
 
#define MIN_MODEM_TIMER	0			/* minimum modem status timer */
#define MAX_MODEM_TIMER	5000			/* maximum modem status timer */

#define SEC_MAX_NO_DATA_BYTES_IN_FRAME  2048 /* SECONDARY - max length of the CHDLC data field */

#define MIN_Tx_KPALV_TIMER	0	  /* minimum transmit keepalive timer */
#define MAX_Tx_KPALV_TIMER	60000	  /* maximum transmit keepalive timer */
#define DEFAULT_Tx_KPALV_TIMER	10000	  /* default transmit keepalive timer */

#define MIN_Rx_KPALV_TIMER	10	  /* minimum receive keepalive timer */
#define MAX_Rx_KPALV_TIMER	60000	  /* maximum receive keepalive timer */
#define DEFAULT_Rx_KPALV_TIMER	10000	  /* default receive keepalive timer */

#define MIN_KPALV_ERR_TOL	1	  /* min kpalv error tolerance count */
#define MAX_KPALV_ERR_TOL	20	  /* max kpalv error tolerance count */
#define DEFAULT_KPALV_ERR_TOL	3	  /* default value */

#define MIN_SLARP_REQ_TIMER	0	  /* min transmit SLARP Request timer */
#define MAX_SLARP_REQ_TIMER	60000	  /* max transmit SLARP Request timer */
#define DEFAULT_SLARP_REQ_TIMER	0	  /* default value -- no SLARP */



/* ----------------------------------------------------------------------------
 *             Constants for the READ_CHDLC_LINK_STATUS command
 * --------------------------------------------------------------------------*/

/* the CHDLC status structure */
typedef struct {
	unsigned char CHDLC_link_status ;	/* CHDLC link status */
	unsigned char no_Data_frms_for_app ;	/* number of Data frames available for the application */
	unsigned char receiver_status ;	/* enabled/disabled */
	unsigned char SLARP_state ;	/* internal SLARP state */
} CHDLC_LINK_STATUS_STRUCT;

/* settings for the 'CHDLC_link_status' variable */
#define CHDLC_LINK_INACTIVE		0x00	/* the CHDLC link is inactive */
#define CHDLC_LINK_ACTIVE		0x01	/* the CHDLC link is active */



/* ----------------------------------------------------------------------------
 *                 Constants for using application interrupts
 * --------------------------------------------------------------------------*/

/* the structure used for the SET_CHDLC_INTERRUPT_TRIGGERS/READ_CHDLC_INTERRUPT_TRIGGERS command */
typedef struct {
	unsigned char CHDLC_interrupt_triggers ;	/* CHDLC interrupt trigger configuration */
	unsigned char IRQ ;			/* IRQ to be used */
	unsigned short interrupt_timer ;		/* interrupt timer */
	unsigned short misc_interrupt_bits ;	/* miscellaneous bits */
} CHDLC_INT_TRIGGERS_STRUCT;

/* 'CHDLC_interrupt_triggers' bit settings */
#define APP_INT_ON_RX_FRAME		0x01	/* interrupt on Data frame reception */
#define APP_INT_ON_TX_FRAME		0x02	/* interrupt when an Data frame may be transmitted */
#define APP_INT_ON_COMMAND_COMPLETE	0x04	/* interrupt when an interface command is complete */
#define APP_INT_ON_TIMER		0x08	/* interrupt on a defined millisecond timeout */
#define APP_INT_ON_GLOBAL_EXCEP_COND 	0x10	/* interrupt on a global exception condition */
#define APP_INT_ON_CHDLC_EXCEP_COND	0x20	/* interrupt on an CHDLC exception condition */
#define APP_INT_ON_TRACE_DATA_AVAIL	0x80	/* interrupt when trace data is available */

/* interrupt types indicated at 'interrupt_type' byte of the INTERRUPT_INFORMATION_STRUCT */
#define NO_APP_INTS_PEND		0x00	/* no interrups are pending */
#define RX_APP_INT_PEND			0x01	/* a receive interrupt is pending */
#define TX_APP_INT_PEND			0x02	/* a transmit interrupt is pending */
#define COMMAND_COMPLETE_APP_INT_PEND	0x04	/* a 'command complete' interrupt is pending */
#define TIMER_APP_INT_PEND		0x08	/* a timer interrupt is pending */
#define GLOBAL_EXCEP_COND_APP_INT_PEND 	0x10	/* a global exception condition interrupt is pending */
#define CHDLC_EXCEP_COND_APP_INT_PEND 	0x20	/* an CHDLC exception condition interrupt is pending */
#define TRACE_DATA_AVAIL_APP_INT_PEND	0x80	/* a trace data available interrupt is pending */


/* modem status changes */
#define DCD_HIGH			0x08
#define CTS_HIGH			0x20


/* ----------------------------------------------------------------------------
 *                   Constants for Data frame transmission
 * --------------------------------------------------------------------------*/

/* the Data frame transmit status element configuration structure */
typedef struct {
	unsigned short number_Tx_status_elements ;	/* number of transmit status elements */
	unsigned int base_addr_Tx_status_elements ;	/* base address of the transmit element list */
	unsigned int next_Tx_status_element_to_use ;	/* pointer to the next transmit element to be used */
} CHDLC_TX_STATUS_EL_CFG_STRUCT;

/* the Data frame transmit status element structure */
typedef struct {
	unsigned char opp_flag ;		/* opp flag */
	unsigned short frame_length ;	/* length of the frame to be transmitted */
	unsigned char misc_Tx_bits ;	/*  miscellaneous transmit bits */
	unsigned int reserved_2 ;	/* reserved for internal use */
	unsigned int reserved_3 ;	/* reserved for internal use */
	unsigned int ptr_data_bfr ;	/* pointer to the data area */
} CHDLC_DATA_TX_STATUS_EL_STRUCT;


/* settings for the 'misc_Tx_bits' 
 * (pertains only to switched CTS/RTS and 
 * 'idle mark' configurations) */

#define DROP_RTS_AFTER_TX 	0x01	
/* drop RTS after transmission of this frame */

#define IDLE_FLAGS_AFTER_TX	0x02	
/* idle flags after transmission of this frame */

/* settings for the 'misc_Tx_bits' for IPv4/IPv6 usage */
#define TX_DATA_IPv4		0x00 	
/* apply the IPv4 CHDLC header to the transmitted Data frame */
#define TX_DATA_IPv6		0x80 	
/* apply the IPv6 CHDLC header to the transmitted Data frame */




/* ----------------------------------------------------------------------------
 *                   Constants for Data frame reception
 * --------------------------------------------------------------------------*/

/* the Data frame receive status element configuration structure */
typedef struct {
	unsigned short number_Rx_status_elements ;	/* number of receive status elements */
	unsigned int base_addr_Rx_status_elements ;	/* base address of the receive element list */
	unsigned int next_Rx_status_element_to_use ;	/* pointer to the next receive element to be used */
	unsigned int base_addr_Rx_buffer ;		/* base address of the receive data buffer */
	unsigned int end_addr_Rx_buffer ;		/* end address of the receive data buffer */
} CHDLC_RX_STATUS_EL_CFG_STRUCT;

/* the Data frame receive status element structure */
typedef struct {
	unsigned char opp_flag ;		/* opp flag */
	unsigned short frame_length ;   /* length of the received frame */
        unsigned char error_flag ; /* frame errors (HDLC_STREAMING_MODE)*/
        unsigned short time_stamp ; /* receive time stamp (HDLC_STREAMING_MODE) */
        unsigned int reserved_1 ; 	/* reserved for internal use */
        unsigned short reserved_2 ; 	/* reserved for internal use */
        unsigned int ptr_data_bfr ;	/* pointer to the data area */
} CHDLC_DATA_RX_STATUS_EL_STRUCT;


/* settings for the 'error_flag' */
#define RX_FRM_ABORT		0x01
#define RX_FRM_CRC_ERROR	0x02
#define RX_FRM_OVERRUN_ERROR	0x04

/* settings for the 'error_flag' for IPv4/IPv6 usage */
#define RX_DATA_IPv4		0x00 	
/* the received Data frame is IPv4 */
#define RX_DATA_IPv6		0x80 	
/* the received Data frame is IPv6 */


/* ----------------------------------------------------------------------------
 *         Constants defining the shared memory information area
 * --------------------------------------------------------------------------*/

/* the global information structure */
typedef struct {
 	unsigned char global_status ;		/* global status */
 	unsigned char modem_status ;		/* current modem status */
 	unsigned char global_excep_conditions ;	/* global exception conditions */
	unsigned char glob_info_reserved[5] ;	/* reserved */
	unsigned char codename[4] ;		/* Firmware name */
	unsigned char codeversion[4] ;		/* Firmware version */
} GLOBAL_INFORMATION_STRUCT;

/* the CHDLC information structure */
typedef struct {
	unsigned char CHDLC_status ;		/* CHDLC status */
 	unsigned char CHDLC_excep_conditions ;	/* CHDLC exception conditions */
	unsigned char CHDLC_info_reserved[14] ;	/* reserved */
} CHDLC_INFORMATION_STRUCT;

/* the interrupt information structure */
typedef struct {
 	unsigned char interrupt_type ;		/* type of interrupt triggered */
 	unsigned char interrupt_permission ;	/* interrupt permission mask */
	unsigned char int_info_reserved[14] ;	/* reserved */
} INTERRUPT_INFORMATION_STRUCT;

/* the S508/FT1 information structure */
typedef struct {
 	unsigned char parallel_port_A_input ;	/* input - parallel port A */
 	unsigned char parallel_port_B_input ;	/* input - parallel port B */
	unsigned char FT1_info_reserved[14] ;	/* reserved */
} FT1_INFORMATION_STRUCT;

/* the shared memory area information structure */
typedef struct {
	GLOBAL_INFORMATION_STRUCT global_info_struct ;		/* the global information structure */
	CHDLC_INFORMATION_STRUCT CHDLC_info_struct ;		/* the CHDLC information structure */
	INTERRUPT_INFORMATION_STRUCT interrupt_info_struct ;	/* the interrupt information structure */
	FT1_INFORMATION_STRUCT FT1_info_struct ;			/* the S508/FT1 information structure */
} SHARED_MEMORY_INFO_STRUCT;

/* ----------------------------------------------------------------------------
 *        UDP Management constants and structures 
 * --------------------------------------------------------------------------*/

/* The embedded control block for UDP mgmt 
   This is essentially a mailbox structure, without the large data field */

#ifndef HDLC_PROT_ONLY

typedef struct {
        unsigned char  opp_flag ;                  /* the opp flag */
        unsigned char  command ;                   /* the user command */
        unsigned short buffer_length ;             /* the data length */
        unsigned char  return_code ;               /* the return code */
	unsigned char  MB_reserved[NUMBER_MB_RESERVED_BYTES] ;	/* reserved for later */
} cblock_t;

/* UDP management packet layout (data area of ip packet) */
/*
typedef struct {
	unsigned char		signature[8]	;
	unsigned char		request_reply	;
	unsigned char		id		;
	unsigned char		reserved[6]	;
	cblock_t		cblock		;
	unsigned char		num_frames	;
	unsigned char		ismoredata	;
	unsigned char 		data[SIZEOF_MB_DATA_BFR] 	;
} udp_management_packet_t;

*/

typedef struct {
	unsigned char		num_frames	;
	unsigned char		ismoredata	;
} trace_info_t;

#if 0
typedef struct {
	ip_pkt_t 		ip_pkt		;
	udp_pkt_t		udp_pkt		;
	wp_mgmt_t		wp_mgmt		;
	cblock_t                cblock          ;
	trace_info_t       	trace_info      ;
	unsigned char           data[SIZEOF_MB_DATA_BFR]      ;
} chdlc_udp_pkt_t;
#endif

typedef struct ft1_exec_cmd{
	unsigned char  command ;                   /* the user command */
        unsigned short buffer_length ;             /* the data length */
        unsigned char  return_code ;               /* the return code */
	unsigned char  MB_reserved[NUMBER_MB_RESERVED_BYTES] ;
} ft1_exec_cmd_t;

typedef struct {
	unsigned char  opp_flag 			;
	ft1_exec_cmd_t cmd				;
	unsigned char  data[SIZEOF_MB_DATA_BFR]      	;
} ft1_exec_t;


/* By default UDPMGMT Signature is AFT, if user
   includes the sdla_chdlc.h file then its assumed that
   they wan't to use legacy API variant */
#ifdef UDPMGMT_SIGNATURE
#undef UDPMGMT_SIGNATURE
#define UDPMGMT_SIGNATURE	"CTPIPEAB"
#endif

#define UDPMGMT_SIGNATURE_LEN	8


/* UDP/IP packet (for UDP management) layout */
/*
typedef struct {
	unsigned char	reserved[2]	;
	unsigned short	ip_length	;
	unsigned char	reserved2[4]	;
	unsigned char	ip_ttl		;
	unsigned char	ip_protocol	;
	unsigned short	ip_checksum	;
	unsigned int	ip_src_address	;
	unsigned int	ip_dst_address	;
	unsigned short	udp_src_port	;
	unsigned short	udp_dst_port	;
	unsigned short	udp_length	;
	unsigned short	udp_checksum	;
	udp_management_packet_t um_packet ;
} ip_packet_t;
*/

/* valid ip_protocol for UDP management */
#define UDPMGMT_UDP_PROTOCOL 0x11

#if 0
typedef struct {
	unsigned char	status		;
	unsigned char	data_avail	;
	unsigned short	real_length	;
	unsigned short	time_stamp	;
	unsigned char	data[1]		;
} trace_pkt_t;
#endif

#endif  //HDLC_PROT_ONLY
/* ----------------------------------------------------------------------------
 *   Constants for the SET_FT1_CONFIGURATION/READ_FT1_CONFIGURATION command
 * --------------------------------------------------------------------------*/

/* the FT1 configuration structure */
typedef struct {
	unsigned short framing_mode;
	unsigned short encoding_mode;
	unsigned short line_build_out;
	unsigned short channel_base;
	unsigned short baud_rate_kbps;					/* the baud rate (in kbps) */	
	unsigned short clock_mode;
} ft1_config_t;



/* settings for the 'framing_mode' */
#define ESF_FRAMING 	0x00	/* ESF framing */
#define D4_FRAMING  	0x01	/* D4 framing */

/* settings for the 'encoding_mode' */
#define B8ZS_ENCODING 	0x00	/* B8ZS encoding */
#define AMI_ENCODING	0x01	/* AMI encoding */

/* settings for the 'line_build_out' */
#define LN_BLD_CSU_0dB_DSX1_0_to_133	0x00	/* set build out to CSU (0db) or DSX-1 (0-133ft) */
#define LN_BLD_DSX1_133_to_266		0x01	/* set build out DSX-1 (133-266ft) */
#define LN_BLD_DSX1_266_to_399		0x02	/* set build out DSX-1 (266-399ft) */
#define LN_BLD_DSX1_399_to_533		0x03	/* set build out DSX-1 (399-533ft) */
#define LN_BLD_DSX1_533_to_655		0x04	/* set build out DSX-1 (533-655ft) */
#define LN_BLD_CSU_NEG_7dB		0x05	/* set build out to CSU (-7.5db) */
#define LN_BLD_CSU_NEG_15dB		0x06	/* set build out to CSU (-15db) */
#define LN_BLD_CSU_NEG_22dB		0x07	/* set build out to CSU (-22.5db) */

/* settings for the 'channel_base' */
#define MIN_CHANNEL_BASE_VALUE		1		/* the minimum permitted channel base value */
#define MAX_CHANNEL_BASE_VALUE		24		/* the maximum permitted channel base value */

/* settings for the 'baud_rate_kbps' */
#define MIN_BAUD_RATE_KBPS		0		/* the minimum permitted baud rate (kbps) */
#define MAX_BAUD_RATE_KBPS 		1536	/* the maximum permitted baud rate (kbps) */
#define BAUD_RATE_FT1_AUTO_CONFIG	0xFFFF /* the baud rate used to trigger an automatic FT1 configuration */

/* settings for the 'clock_mode' */
#define CLOCK_MODE_NORMAL		0x00	/* clock mode set to normal (slave) */
#define CLOCK_MODE_MASTER		0x01	/* clock mode set to master */


#define BAUD_RATE_FT1_AUTO_CONFIG   	0xFFFF
#define AUTO_FT1_CONFIG_NOT_COMPLETE	0x08
#define AUTO_FT1_CFG_FAIL_OP_MODE	0x0C
#define AUTO_FT1_CFG_FAIL_INVALID_LINE 	0x0D

#if !defined(__WINDOWS__)
#undef  wan_udphdr_data
#define wan_udphdr_data	wan_udphdr_u.chdlc.data
#endif

#ifdef __KERNEL__
#undef wan_udp_data
#define wan_udp_data wan_udp_hdr.wan_udphdr_u.chdlc.data
#endif

#pragma	pack()


#if defined(__LINUX__)
enum {
	SIOC_MBOX_CMD = SIOC_WANPIPE_DEVPRIVATE	
};
#endif

#endif	/* _SDLA_CHDLC_H */
