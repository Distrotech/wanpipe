/*************************************************************************
 * wanpipe_sdlc.h	Sangoma SDLC firmware API definitions
 *
 * Author:      	Nenad Corbic <ncorbic@sangoma.com>	
 *
 * Copyright:	(c) 2002 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the term of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *===========================================================================
 * Jul 24, 2002  Nenad Corbic	Initial Version
 *=======================================================================
 *
 * Organization
 *	- Compatibility notes
 *	- Constants defining the shared memory control block (mailbox)
 *	- Interface commands
 *	- Return code from interface commands
 *	- Constants for the commands (structures for casting data)
 *	- UDP Management constants and structures
 *
 **************************************************************************/

/*
 ***************************************************************
 *                                                             *
 * SDLCHDR.C is the 'C' header file for the Sangoma SDLC code. *
 *                                                             *
 ***************************************************************
*/


/*
 *	$Log: not supported by cvs2svn $
 *	Revision 1.2  2004/06/07 15:53:08  sangoma
 *	*** empty log message ***
 *	
 *	Revision 1.1.1.1  2002/11/25 09:20:00  ncorbic
 *	Wanpipe Linux Development
 *	
 *	Revision 1.1  2002/11/08 05:15:00  ncorbic
 *	*** empty log message ***
 *	
 * 
 *    Rev 1.1   25 Feb 1998 11:27:26   gideon
 * Added the command READ_STATION_INFO_STRUCT and the 
 * 'general_operational_config_option' of DISABLE_RX_WHEN_TX.
 * The code version is now 1.02.
 * 
 * 
 *    Rev 1.0   08 Aug 1996 10:58:00   gideon
 * Initial revision.
 * 
 *
 */


/*
   Reserved data areas for this code are as follows:
   0x0010 	     - 	the adapter configuration data 
   			byte passed by the loader
   0x4700 to 0x477F  - 	primary receive buffer status table (128 bytes)
   
   0x4780 	     - 	P0_ADDR_LAST_READ_RR0_BYTE (used by the external 
   			status interrupt handler)

   0x4781	     - 	address of the CTS status (used by the external 
   			status interrupt handler)
			
   0x4800 to 0x57FF  - 	primary receive buffer (this buffer is of size 
   			4096 bytes which is large enough to accomodate 
			at least four frames of maximum permitted length, 
			plus header length)
			
   0xE000 to 0xE22F  - 	SEND mailbox (560 bytes)
   0xE230 to 0xE45F  - 	RECEIVE mailbox (560 bytes)
   0xE460 to 0xEEDF  - 	reserved for later use (2688 bytes)
   0xEEE0 to 0xEEEF  - 	general status interface bytes (16 bytes)
   0xEEE0 	     -  number of received I-frames queued for the application
   0xEEE1 	     -  global transmit buffer status
   0xEEE2 to 0xEEE3  -  current exception conditions
   0xEEE4 	     -  current modem status
   0xEEE5 	     -  trace data available byte
   0xEEF0 to 0xEEFF  -  interrupt interface bytes (16 bytes)
   0xEEF0 	     -  interrupt reporting byte
   0xEEF1 	     -  interrupt permission byte
   0xEF00 to 0xEFFF  -  status bytes on a per station basis (256 bytes)
   0xF000 to 0xFFFF  -  trace buffer (4096 bytes)
*/

#ifndef _WANPIPE_SDLC_
#define _WANPIPE_SDLC_

#pragma pack(1)

/* absolute addresses of buffers and variables on the board */
#define ADDR_DOWNLOADED_CONFIG_DATA		0x0010 	/* the offset of the downloaded configuration data on the adapter */
#define P0_OFF_PRI_RX_BFR_STATUS_TABLE		0x4700	/* Port 0 - the address of the primary status element table */
#define P0_ADDR_LAST_READ_RR0_BYTE		0x4780	/* Port 0 - the address of the previously read status of RR0 */
#define P0_ADDR_CTS_STATUS_BYTE			0x4781	/* Port 0 - address of the CTS status */
#define BASE_ADDR_PRI_RX_BFR			0x4800	/* the base address of the primary receive DMA buffer */
#define END_ADDR_PRI_RX_BFR			0x57FF	/* the end address of the primary receive DMA buffer */
#define BASE_ADDR_OF_MB_STRUCTS 		0xE000	/* the base address of the mailbox area */
#define ADDR_GLOBAL_NO_RX_I_FRMS_FOR_APP	0xEEE0	/* the address of the global number of I-frames available */
																		/* for the application */
#define ADDR_GLOBAL_TX_BFR_STATUS_BYTE		0xEEE1	/* the address of the global transmit buffer status byte */
#define ADDR_EXCEPTION_COND_INTERFACE_WORD	0xEEE2	/* the address of the exception condition interface word */
#define ADDR_CURR_MODEM_STAT_INTERFACE_BYTE 	0xEEE4 	/* the address of the current modem status interface byte */
#define ADDR_TRACE_DATA_AVAILABLE_BYTE		0xEEE5	/* the address of the byte indicating the availablity of trace */
																		/* data */
#define ADDR_INTERRUPT_REPORT_INTERFACE_BYTE	0xEEF0 	/* the address of the interrupt type reporting interface byte */
#define ADDR_INTERRUPT_PERMIT_INTERFACE_BYTE	0xEEF1 	/* the address of the interrupt permission interface byte */
#define BASE_ADDR_STATION_INFO_INTERFACE_BYTES	0xEF00 /* the base address of the per-station status bytes */
#define BASE_ADDR_TRACE_BFR			0xF000	/* the base address of the trace buffer */
#define END_ADDR_TRACE_BFR			0xFFFF	/* the end address of the trace buffer */



/* interface commands */
#define SDLC_READ 			0x00		/* receive an SDLC I-frame */
#define SDLC_WRITE 			0x01		/* transmit an SDLC I-frame */
#define SET_SDLC_CONFIGURATION		0x10		/* set the SDLC operational configuration */
#define READ_SDLC_CONFIGURATION		0x11		/* read the current SDLC configuration */
#define SET_ADAPTER_OPERATING_FREQUENCY	0x12		/* set the adapter operating frequency */
#define ENABLE_COMMUNICATIONS		0x13		/* enable communications */
#define DISABLE_COMMUNICATIONS		0x14		/* disable communications */
#define ADD_STATION			0x15		/* add and configure and SDLC station */
#define DELETE_STATION 			0x16		/* delete an SDLC station */
#define ACTIVATE_STATION 		0x17		/* activate an SDLC station */
#define DEACTIVATE_STATION 		0x18		/* deactivate an SDLC station */
#define FLUSH_I_FRAME_BUFFERS		0x19		/* flush the I-frame buffers */
#define READ_STATION_STATUS		0x20		/* read the status of a station */
#define LIST_ADDED_STATIONS 		0x21		/* return a list of 'added' stations */
#define LIST_STATIONS_IN_NRM 		0x22		/* return a list of stations currently in the NRM */
#define LIST_STATIONS_WITH_I_FRMS_AVAILABLE	0x23	/* return a list of stations with I-frames available for the */
																	/* application */
#define READ_OPERATIONAL_STATISTICS	0x24		/* retrieve the operational statistics */
#define FLUSH_OPERATIONAL_STATISTICS	0x25		/* flush the operational statistics */
#define READ_STATION_INFO_STRUCT	0x26		/* return the STATION_INFORMATION structure to the application */
#define SEND_TEST_FRAME			0x30		/* send a TEST frame */
#define SEND_SIM_RIM_FRAME		0x31		/* send a SIM or a RIM frame */
#define SEND_XID_RESPONSE_FRAME		0x32		/* send an XID response frame */
#define SET_PRI_STATION_XID_I_FIELD	0x33		/* set the XID field for a primary SDLC station */
#define LIST_PRI_STATIONS_ISSUING_XID_FRAMES 0x34	/* return a list of primary stations issuing XID frames */
#define SET_MODEM_STATUS		0x40		/* set the DTR/RTS state */
#define READ_MODEM_STATUS 		0x41		/* read the current modem status (CTS and DCD status) */
#define READ_COMMS_ERR_STATISTICS	0x42		/* read the communication error statistics */
#define FLUSH_COMMS_ERR_STATISTICS 	0x43		/* flush the communication error statistics */
#define SET_INTERRUPT_TRIGGERS		0x50		/* set the interrupt triggers */
#define READ_INTERRUPT_TRIGGERS		0x51		/* read the interrupt trigger configuration */
#define SET_TRACE_CONFIGURATION		0x60		/* set the trace configuration */
#define READ_TRACE_DATA			0x61		/* read trace data */
#define READ_TRACE_STATISTICS		0x62		/* read the trace statistics */
#define READ_CODE_VERSION		0x70		/* read the SDLC code version */
#define READ_EXCEPTION_CONDITION	0x71		/* read any exception conditions from the adapter */
#define DISCARD_INCOMMING_INFORMATION_FRAMES	0x72	/* discard incomming I-frames */
#define BRIDGE_RECEIVER_AND_TRANSMITTER	0x73		/* bridge the receiver and transmitter */
#define SET_APPLICATION_IRQ_TIMEOUT	0x74		/* set a timeout for the application to respond to an IRQ */
#define READ_SDLC_ADAPTER_CONFIGURATION	0xA0		/* read the adapter type and operating speed */
#define DUMMY_MB_COMMAND		0xFF		/* a dummy command used internally */



/* return codes */
#define COMMAND_OK				0x00
#define OK 					0x00	/* the interface call was successfull */
#define SET_SDLC_CONFIG_BEFORE_ADD_STATION 	0x01	/* set the SDLC configuration before a station is added */
#define SET_SDLC_CONFIG_BEFORE_ENABLE_COMMS 	0x01	/* set the configuration before communications are enabled */
#define SET_SDLC_CONFIG_BEFORE_SET_INTS 	0x01	/* set the configuration before enabling interrupts */
#define INVALID_CONFIGURATION_DATA		0x01	/* the passed configuration data is invalid */
#define COMMUNICATIONS_NOT_ENABLED		0x01	/* communications are not enabled */
#define INVALID_SDLC_ADDRESS			0x02	/* the passed SDLC address is invalid */
#define INVALID_TRACE_CONFIGURATION		0x02	/* the trace configuration data is invalid */
#define TRACE_HAS_NOT_BEEN_ACTIVATED		0x02	/* the trace has not yet been activated */
#define STATION_NOT_ADDED 			0x03	/* the station has not been added */
#define STATION_ALREADY_ADDED 			0x03	/* the station has already been added */
#define STATION_ALREADY_ACTIVATED 		0x04	/* the station has already been activated */
#define STATION_ALREADY_DEACTIVATED 		0x04	/* the station has already been deactivated */
#define STATION_DISCONNECTED 			0x04	/* the station is currently disconnected */
#define POLL_INTERVAL_INVALID			0x04	/* the set poll interval is invalid */	
#define INVALID_INTERRUPT_TMR_VALUE_SELECTED	0x04	/* the selected interrupt timer value is invalid */
#define NO_RX_I_FRAMES_AVAILABLE_FOR_APP 	0x05	/* no frames are available for the application */
#define INVALID_FRAME_LENGTH			0x05	/* the passed transmit buffer length is invalid */
#define NO_TRACE_DATA_AVAILABLE			0x05	/* no trace data is available for the application */
#define I_FRM_NOT_QUEUED_FOR_TX 		0x06	/* the I-frame has not been queued for transmission */
#define CMND_INVALID_FOR_SECONDARY_STATION 	0x06	/* the command is invalid for a secondary SDLC station */
#define CMND_INVALID_FOR_PRIMARY_STATION 	0x06	/* the command is invalid for a primary SDLC station */
#define TEST_FRM_BFR_CURRENTLY_IN_USE		0x07	/* the TEST frame buffer is currently in use */
#define XID_FRM_BFR_CURRENTLY_IN_USE		0x07	/* the XID frame buffer is currently in use */
#define XID_I_FIELD_BFRS_FULL			0x07	/* the XID Information field buffers are all occupied */
#define NO_XID_CMND_RECEIVED_FROM_PRI		0x08	/* no XID command has been received from the primary station */
#define CMND_INVALID_FOR_CURRENT_SDLC_STATE	0x08	/* the command is invalid for the current SDLC state */
#define PRI_STATION_ALREADY_HAS_XID_DEFINED	0x09	/* the primary station already has an XID field defined */
#define ILLEGAL_COMMAND				0x10	/* the interface command used is invalid */
#define BUSY_WITH_BAUD_CALIBRATION		0x40	/* the adapter is busy with a baud rate calibration */
#define BAUD_CALIBRATION_FAILED			0x41	/* the adapter failed the baud rate calibration */


/* miscellaneous constants used when executing mailbox commands */

/* for the SET_CONFIGURATION command */
#define LGTH_CONFIG_DATA 			0x1F 		/* the length of the configuration data */
#define SECONDARY_STATION			0x00		/* the station is configured as a primary */
#define PRIMARY_STATION				0x01		/* the station is configured as a secondary */

/* offset within the data area of various configuration parameters */
#define OFF_STATION_CONFIG_IN_CFG_DATA		0x00		/* the offset of the station configuration */
#define OFF_BAUD_RATE_IN_CFG_DATA		0x01		/* the offset of the baud rate */
#define OFF_MAX_I_FIELD_LGTH_IN_CFG_DATA	0x05		/* the offset of the maximum I-field length */
#define OFF_GENL_OP_CFG_BITS_IN_CFG_DATA	0x07		/* the offset of the operational configuration bits */
#define OFF_PRI_STATION_SLOW_POLL_INTERVAL_IN_CFG_DATA 0x11	/* the offset of the primary station slow poll interval */
#define OFF_SEC_STATION_RESPONSE_TO_IN_CFG_DATA 0x13			/* the offset of the secondary station response timeout */
#define OFF_NO_CONSEC_SEC_TOs_IN_NRM_IN_CFG_DATA 0x15			/* the offset of the permitted number of consecutive */
																				/* secondary stations timeouts */
#define OFF_MAX_LGTH_I_FLD_PRI_XID_FRM_IN_CFG_DATA 0x17		/* the offset of the maximum length of the XID data */
																				/* field for a primary station */
#define OFF_OP_FLG_BIT_DELAY_IN_CFG_DATA	0x19		/* the offset of the opening flag bit delay within the data area */
#define OFF_RTS_DROP_DELAY_IN_CFG_DATA		0x1B		/* the offset of the RTS dropping delay within the data area */
#define OFF_CTS_TIMEOUT_IN_CFG_DATA		0x1D		/* the offset of the permitted CTS timeout within the data area */

/* minimum and maximum values for the configuration parameters */
#define MAX_PERMITTED_BAUD_RATE			200000		/* the maximum permitted baud rate */
#define MIN_PERMITTED_I_FIELD_LENGTH		1		/* the minimum permitted I-frame length */
#define MAX_PERMITTED_I_FIELD_LENGTH		544		/* the maximum permitted I-frame length */
#define MAX_ADD_OP_FLG_BIT_DELAY_COUNT		200		/* the maximum permitted opening flag bit delay */
#define MAX_ADD_DROP_RTS_BIT_DELAY		200		/* the maximum permitted DRT drop bit delay */
#define MIN_CTS_DELAY_1000THS_SEC		10		/* the minimum permitted CTS timeout bit delay (1/100ths second) */
#define MAX_CTS_DELAY_1000THS_SEC		2000		/* the maximum permitted CTS timeout bit delay (1/100ths second) */
#define MIN_PRI_STATION_SLOW_POLL_INTERVAL 1			/* the minimum slow poll interval */
#define MAX_PRI_STATION_SLOW_POLL_INTERVAL 60000		/* the maximum slow poll interval */
#define MIN_SEC_STATION_RESPONSE_TO 		1		/* the minimum secondary station response timeout */
#define MAX_SEC_STATION_RESPONSE_TO		15000		/* the maximum secondary station response timeout */
#define MIN_NO_CONSEC_SEC_TOs_IN_NRM 		0		/* the minimum number of consecutive secondary timeouts */
#define MAX_NO_CONSEC_SEC_TOs_IN_NRM 		200		/* the maximum number of consecutive secondary timeouts */
#define MAX_LGTH_I_FLD_PRI_XID_FRAME		255		/* the maximum length of a primary station XID data field */

/* 'general_operational_config_bits' settings used by the SET_CONFIGURATION command */
#define SWITCHED_CTS_RTS			0x0001	/* switched CTS/RTS */
#define IDLE_FLAGS				0x0002	/* the line idle condition is flags (and not ones) */
#define NRZI_ENCODING				0x0010	/* NRZI encoding */
#define FM0_ENCODING				0x0020	/* FM0 encoding (used in a 4 wire, V.35 configuration) */
#define APP_REQUESTS_PASSING_EXCEPT_COND	0x0100	/* exception conditions are only passed to the application on a */
																	/* READ_EXCEPTION_CONDITION command */
#define DISABLE_RX_WHEN_TX			0x0200	/* disable the receiver while transmitting a frame */
#define INTERFACE_LEVEL_V35			0x8000	/* V.35 interface */



/************************************************************************
 * 'protocol_config_bits' 
 * 	settings used by the SET_CONFIGURATION command 
 ************************************************************************/

#define APP_TO_ISSUE_DISC_AFTER_RX_RD		0x0001	
/* the application is responsible for issuing a DISC when 
 * notified that an RD frame has been received */

#define APP_TO_ISSUE_SIM_AFTER_RX_RIM		0x0002	
/* the application is responsible for issuing a SIM when 
 * notified that an RIM frame has been received */

#define NRM_TIMEOUT_FOR_SECONDARY 		0x0004	
/* the secondary will timeout while in the NRM (and 
 * automatically enter the NDM) if not polled by the 
 * primary at the configured poll rate */

#define PERMANENT_SEC_DISC_MODE			0x0008	
/* on reception of a DISC command, the secondary 
 * permanently  enters the disconnected mode, and 
 * will issue DMs in response to incomming SNRMs 
 * (i.e, will not re-enter ABM until an ACTIVATE_STATION 
 * command is performed by the application) */

#define ISSUE_SNRM_AFTER_RX_DM_IN_NRM		0x0010	
/* a primary station should automatically issue 
 * a SNRM and attempt to re-enter the ABM after 
 * receiving a DM while in the NRM */

#define ISSUE_DISC_AFTER_RX_FRMR		0x0020	
/* a primary station should automatically issue a 
 * DISC (and not a SNRM) after receiving a FRMR 
 * while in the NRM */

/************************************************************
 * 'exception_condition_reporting_config' 
 * 	settings used by the SET_CONFIGURATION command 
 ************************************************************/ 	


#define DONT_REPORT_STATIONS_GOING_ACTIVE	0x01	
/* don't report stations going active */

#define DONT_REPORT_STATIONS_GOING_INACTIVE	0x02	
/* don't report stations going inactive */


/************************************************************
 * 'modem_config_bits' 
 * 	settings used by the SET_CONFIGURATION command 
 ************************************************************/

#define DONT_RAISE_DTR_RTS_ON_EN_COMMS		0x01
/* don't automatically raise DTR and RTS when 
 * performing an ENABLE_COMMUNICATIONS
 * command and the ADD_STATION command */

#define MAX_POLL_INTERVAL 			60000		
/* the maximum permitted poll interval
 * for the READ_STATUS command */

#define LGTH_STATUS_DATA			0x06		
/* the number of bytes included in the status 
 * data (READ_STATUS) */

#define OFF_COMMS_STAT_IN_STATS_DATA 		0x00		
/* the offset of the communications status 
 * within the data area */

#define OFF_NO_FRMS_TO_TX_IN_STATS_DATA 	0x01		
/* the offset of the number of queued outgoing 
 * frames */

#define OFF_INT_REPORT_BYTE_IN_STATS_DATA 0x05		
/* the offset of the interrupt status byte 
 * within the data area */

/****************************************************
 * for the SET_INTERRUPT_TRIGGERS command 
 ***************************************************/

#define LGTH_INTERRUPT_TRIGGER_DATA		0x03		
/* the length of the interrupt trigger 
 * configuration data */

#define OFF_TRIGGER_BITS_IN_SET_INT_DATA	0x00		
/* offset of the interrupt trigger map within 
 * the data area */

#define OFF_TX_FRM_LGTH_IN_SET_INT_DATA		0x01	
/* offset of the transmit frame length within 
 * the data area */

#define OFF_INT_TIMER_VALUE_IN_SET_INT_DATA 	0x01	
/* offset of the interrupt timer value within 
 * the data area */ 

#define MAX_INTERRUPT_TIMER_VALUE 		60000	
/* the maximum permitted interrupt timer value */

/************************************************
 * for the OPERATE_DATALINE_MONITOR command 
 ************************************************/

#define OFF_LN_TRC_SETUP			0x00	
/* the offset within the mailbox structure data 
 * area of the line trace setup parameter */

#define OFF_TRC_DEACTIVATION_TMR_BYTE		0x01		
/* the offset within the mailbox of the trace 
 * deactivation timer */


/************************************************
 * for the SET_APPLICATION_IRQ_TIMEOUT command
 ************************************************/
 
#define OFF_IRQ_TO_IN_SET_APP_IRQ_TO_CMND	0x00	
/* offset of the IRQ timeout within the data area */

#define MIN_IRQ_TIMEOUT_VALUE			0x00	
/* the minimum permitted IRQ timeout */

#define MAX_IRQ_TIMEOUT_VALUE			5000	
/* the maximum permitted IRQ timeout */

/* miscellaneous commands */
#define OFF_TX_BFR_SIZE_IN_DATA_AREA		0x0A		
/* READ_CONFIGURATION - the offset of the 
 * transmit buffer size within the data area */

#define LGTH_COMMS_ERR_STAT_DATA 		0x0A
/* READ_COMMS_ERR_STATISTICS - the number of 
 * bytes included in the communication error data */

#define LGTH_GLOBAL_OP_STATS  			42
/* the length of the global operational statistic data */

#define LGTH_SPECIFIC_STATION_STATUS 		0x03	
/* the length of the station-specific status information */

#define LGTH_GLOBAL_STATION_STATUS 		0x05
/* the length of the general station information */

#define LGTH_CODE_VERSION_STRING		0x04
/* READ_CODE_VERSION - the length of the code version string */

#define OFF_DISCARD_MODE_IN_DATA_AREA		0x00	
/* DISCARD_INCOMMING_INFORMATION_FRAMES - the
 * offset of the discard mode parameter within 
 * the data area */

#define OFF_BRIDGE_MODE_IN_DATA_AREA		0x00	
/* BRIDGE_RECEIVER_AND_TRANSMITTER - the offset 00
 * of the bridging  bridging mode parameter within 
 * the data area */


/****************************************************
 * exception conditions reported to the application 
 ***************************************************/ 

#define EXCEPTION_CONDITION_BASE_VALUE		0x20	
/* a base value used for reporting exception conditions */

#define EXCEP_COND_STATE_CHANGE			0x20	
/* the SDLC station changed state */

#define EXCEP_COND_TIMEOUT_NRM			0x21	
/* a timeout occurred while the stations was in the NRM */

#define EXCEP_COND_RD_FRM_RX_WHILE_IN_NRM	0x22	
/* a RD was received while the link was in the NRM */

#define EXCEP_COND_DM_FRM_RX_WHILE_IN_NRM	0x23	
/* a DM was received while the link was in the NRM */

#define EXCEP_COND_SNRM_FRM_RX_WHILE_IN_NRM	0x24	
/* a SNRM was received while the link was in the NRM */

#define EXCEP_COND_RIM_FRM_RX			0x25	
/* a RIM was received */

#define EXCEP_COND_XID_FRM_RX			0x26	
/* an XID frame was received */

#define EXCEP_COND_TEST_FRM_RX			0x27	
/* a TEST frame was received */

#define EXCEP_COND_FRMR_FRM_RX_TX		0x28	
/* a FRMR condition occurred */

#define EXCEP_COND_CHANGE_IN_MODEM_STATUS	0x30	
/* a modem status change occurred */


/****************************************************
 * clocking types 
 ****************************************************/

#define EXTERNAL_CLOCKING 			0x00		/* external clocking */
#define INTERNAL_CLOCKING 			0x01		/* internal clocking */




/* bit settings of RTS and DTR used for WR5 of the 
 * ESCC and channel 0 of the ASCI 
 * (SET_MODEM_STATUS command) */
#define DTR_LOW_RTS_LOW				0x00		/* DTR low, RTS low */
#define RTS_HIGH				0x02		/* RTS high */
#define DTR_HIGH				0x80		/* RTS high */

/* modem status changes */
#define DCD_HIGH			0x08
#define CTS_HIGH			0x20

/****************************************************
 * definitions for triggering interrupts 
 **************************************************/

/* 'interrupt_triggers' bit mapping set by a 
 * SET_INTERRUPT_TRIGGERS command */

#define INTERRUPT_ON_RX_FRAME			0x01		
/* interrupt when an incomming frame is available 
 * for reception  by the application */

#define INTERRUPT_ON_TX_FRAME			0x02		/* interrupt when a frame may be transmitted */
#define INTERRUPT_ON_COMMAND_COMPLETE		0x08		/* interrupt when an interface command is complete */
#define INTERRUPT_ON_EXCEPTION_CONDITION	0x10		/* interrupt on an exception condition */
#define INTERRUPT_ON_TIMER			0x20		/* interrupt on a timer */
#define INTERRUPT_ON_TRACE_DATA_AVAILABLE	0x40		/* interrupt if trace data is available for the application */

/* interrupt types indicated at 'ptr_interrupt_interface_byte' */
#define NO_INTERRUPTS_PENDING			0x00		/* no interrups are currently pending */
#define RX_INTERRUPT_PENDING			0x01		/* a receive interrupt is pending */
#define TX_INTERRUPT_PENDING			0x02		/* a transmit interrupt is pending */
#define COMMAND_COMPLETE_INTERRUPT_PENDING 	0x08	/* a 'command complete' interrupt is pending */
#define EXCEPTION_CONDITION_INTERRUPT_PENDING 0x10	/* an exception condition interrupt is pending */
#define TIMER_INTERRUPT_PENDING			0x20		/* a timer interrupt is pending */
#define TRACE_INTERRUPT_PENDING			0x40		/* a trace interrupt is pending */

/* bit mappings used for the transmit and receive interface bytes */
#define NO_FRAMES_QUEUED_FOR_TRANSMISSION	0x00 /* no frames are currently queued for transmission */
#define AT_LEAST_ONE_FRAME_QUEUED_FOR_TRANSMISSION	0x01 /* at least one frame is currently queued for transmission */
#define NO_FRAMES_AVAILABLE_FOR_RECEPTION	0x00 /* no frames are currently queued for reception */
#define AT_LEAST_ONE_FRAME_AVAILABLE_FOR_RECEPTION	0x01 /* at least one frame is currently queued for reception */



/* definitions for the interrupt level receive buffers */
#define NUMBER_RX_INT_LVL_BUFFERS 		4			/* the number of interrupt level receive buffers */
#define SIZEOF_INT_LVL_RX_BFR_STRUCT 		556		/* the size of each interrupt level receive structure */
#define FIRST_INT_LVL_RX_STRUCTURE 		0x00		/* the first interrupt level receive structure number */
#define MAX_FRM_LGTH_INCL_HDR_CRC_BYTES 	548 		/* the maximum permitted frame length (including headed and CRC */
																	/* bytes) */



/* for modem status initialization and reporting */
#define LINE_LOW									0x00		/* the initial CTS and DCD status */
#define OFF_MODEM_STAT 							0x00		/* the offset within the mailbox structure data area of the modem */
																	/* status byte in a READ_MODEM_STAT command */
#define DCD_CTS_MASK			 					0x28		/* a mask to show bits 5 and 3 (CTS and DCD) when reading the */
																	/* modem status in RR0 of the SCC */


/* mailbox definitions */
#define SIZEOF_MB_STRUCTS 						560 		/* the size of the mailbox structures */
#define SIZEOF_MB_DATA_AREA 					544		/* the size of the mailbox data area */
#define NO_RESERVED_BYTES_IN_MB 				0x06		/* the number of reserved bytes in the mailbox structure */
#define SEND_MAILBOX								0x00		/* SEND mailbox structure */
#define RECEIVE_MAILBOX							0x01		/* RECEIVE mailbox structure */



/* transmit and receive I-frame buffering */
#define SIZEOF_RX_I_FRM_STORAGE_BFR_AREA 	10000		/* the size of the I-frame receive buffer area */
#define SIZEOF_TX_I_FRM_STORAGE_BFR_AREA 	10000		/* the size of the I-frame transmit buffer area */
#define MAX_NO_RX_I_FRM_STORAGE_BUFFERS 	35			/* the maximum number of receive I-frame storage buffers */
#define MAX_NO_TX_I_FRM_STORAGE_BUFFERS 	35			/* the maximum number of transmit I-frame storage buffers */



/* SDLC control field types */
#define RR 		0x01
#define RNR		0x05
#define REJ 		0x09
#define INFORMATION 	0x00
#define UI 		0x03
#define SIM 		0x07
#define RIM 		0x07
#define DM		0x0F
#define UP 		0x23
#define DISC 		0x43
#define RD 		0x43
#define UA 		0x63
#define SNRM		0x83
#define FRMR 		0x87
#define XID 		0xAF
#define CFGR 		0xC7
#define TEST 		0xE3
#define BCN		0xEF



/* constants for the formation and interpretation of SDLC frames */
#define OFF_PF_BIT	 						0x04		/* offset of the P/F bit in the SDLC control field */
#define OFF_Nr_FIELD 							0x05		/* offset of the Nr bits in the SDLC control field */
#define OFF_Ns_FIELD 							0x01		/* offset of the Ns bits in the SDLC control field */
#define REM_PF_BIT_MASK 						0xEF		/* mask to remove the P/F bit from an SDLC control field */
#define SET_PF_BIT_MASK			 				0x10		/* mask to include the P/F bit in an SDLC control field */
#define REM_rrr_MASK 							0x1F		/* mask to remove the rrr bits from an SDLC control field */
#define REM_sss_MASK 							0xF1		/* mask to remove the sss bits from an SDLC control field */
#define EXTRACT_rrr_MASK		 				0xE0		/* mask to extract the rrr bits from an SDLC control field */
#define EXTRACT_sss_MASK 						0x0E		/* mask to extract the sss bits from an SDLC control field */
#define BIT_0_SET 								0x01		/* used to test the received frame control field type */
#define OFF_ADDR_FLD 							0x00		/* the offset of the SDLC address field in the receiver buffer area */
#define OFF_CNTRL_FLD 							0x01		/* the offset of the SDLC control field in the receiver buffer area */
#define OFF_INFO_FLD			 					0x02		/* the offset of the SDLC information field in the receiver buffer */
#define SDLC_HDR_LGTH 							0x02		/* the number of bytes in the SDLC address and control fields */
#define CRC_LGTH 									0x02		/* the number of bytes in the CRC field */
#define BROADCAST_ADDRESS						0xFF		/* the SDLC broadcast address */



/* overall SDLC states */
#define NDM 										0x00		/* the device is currently in the NDM */
#define NRM 										0x01		/* the device is in the NRM (information transfer mode) */



/* definitions for constructing the I-field for FRMR frames */
#define LGTH_FRMR_I_FLD 						0x03		/* the length of a FRMR I-field */
#define OFF_C_FLD_FRMR 							0x02		/* the offset within the FRMR I-field of the control field of the */
																	/* rejected command */
#define OFF_Nr_Ns_FLD_FRMR 					0x03		/* the offset within the FRMR I-field of the Nr and Ns counts of */
																	/* the station reporting the frame reject */
#define OFF_STATUS_FLD_FRMR 					0x04		/* the offset within the FRMR I-field of the FRMR status field */
#define FRMR_STAT_CTRL_FLD_INVALID 			0x01		/* the control field of the received frame was invalid */
#define FRMR_STAT_I_FLD_PRESENT 				0x02		/* the received frame was invalid as it contained an I-field */
																	/* which is not permitted with the command */
#define FRMR_STAT_I_FLD_TOO_LONG				0x04		/* the I-field in the received frame exceeded the configured */
																	/* maximum */
#define FRMR_STAT_RX_Nr_INVALID 				0x08		/* the received Nr count is invalid */
#define FRMR_STAT_REPEAT_ORG_FRMR			0x80		/* a bit setting if the original FRMR must be retransmitted */



/* definitions for the SDLC state machine */

/* state when setting up to transmit a frame */
#define SENDING_I_FRAME 						0x00		/* sending an I-frame */
#define SENDING_RR0		 						0x01		/* sending an RR (P/F bit reset) */
#define SENDING_RR1 								0x02		/* sending an RR (P/F bit set) */
#define SENDING_RNR0 							0x03		/* sending an RNR (P/F bit reset) */
#define SENDING_RNR1 							0x04		/* sending an RNR (P/F bit set) */
#define SENDING_REJ0			 					0x05		/* sending an REJ (P/F bit reset) */
#define SENDING_REJ1 							0x06		/* sending an REJ (P/F bit set) */
#define SENDING_SNRM								0x10		/* sending a SNRM */
#define SENDING_UA 								0x11		/* sending a UA */
#define SENDING_DISC_RD							0x12		/* sending a DISC or RD */
#define SENDING_DM 								0x13		/* sending a DM */
#define SENDING_SIM_RIM			 				0x14		/* sending a SIM or RIM */
#define SENDING_XID 								0x15		/* sending an XID */
#define SENDING_TEST 							0x16		/* sending a TEST frame */
#define SENDING_FRMR			 					0x17		/* sending a FRMR */

/* waiting to receive a frame */
#define AWAITING_INCOMMING_FRAME				0x20		/* awaiting an incomming frame */
#define WAITING_TO_POLL_SECONDARY			0x21		/* waiting to poll a secondary device */

/* states after transmitting a frame */
#define I_FRAME_TRANSMITTED					0x30		/* an I-frame has been transmitted */
#define RR1_TRANSMITTED							0x31		/* an RR (P/F bit set) has been transmitted */
#define RNR1_TRANSMITTED						0x32		/* an RNR (P/F bit set) has been transmitted */
#define REJ1_TRANSMITTED						0x33		/* an REJ (P/F bit set) has been transmitted */
#define SNRM_TRANSMITTED						0x34		/* a SNRM has been transmitted */
#define DISC_TRANSMITTED						0x35		/* a DISC frame has been transmitted */
#define SIM_TRANSMITTED							0x36		/* a SIM has been transmitted */
#define TEST_TRANSMITTED						0x37		/* a TEST frame has been transmitted */
#define XID_TRANSMITTED							0x38		/* an XID frame has been transmitted */



/* definitions for the 'status' variable in the STATION_INFORMATION structure */
#define STATION_UNINITIALIZED					0x0000	/* the station is uninitialized */
#define STATION_ADDED							0x0001	/* the station has been added */
#define STATION_ACTIVATED						0x0002	/* the station has been activated */
#define STATION_IN_NRM							0x0004	/* the station is in the NRM */
#define STATION_IN_FRMR_MODE					0x0008	/* the station is in the FRMR mode */
#define PRI_STATION_HAS_XID_DEFINED			0x0010	/* the primary station has an XID defined */
#define SEC_STATION_HAS_RX_XID_CMND			0x0010	/* the secondary station has received an XID command */
#define REMOTE_STATION_FLOW_CTRL_BLOCKED	0x0100	/* the remote station is flow control blocked */
#define LOCAL_STATION_FLOW_CTRL_BLOCKED	0x0200	/* the local station is flow control blocked */
#define LOCAL_STATION_SENDING_REJ			0x0400	/* the local station is issuing a REJ */
#define APP_CMND_SND_SIM_RIM_FRM				0x0800	/* the station must issue a SIM or RIM frame */
#define APP_CMND_SND_TEST_FRM					0x1000	/* the station must issue a TEST frame */
#define APP_CMND_SND_XID_FRM					0x2000	/* the station must issue an XID frame */
#define APP_CMND_SND_SNRM_FRMR_FRM			0x4000	/* the station must issue a SNRM or FRMR frame */
#define APP_CMND_SND_DISC_RD_FRM				0x8000	/* the station must issue a DISC or RD frame */

/* a test value to see if an asynchronous frame must be issued at this station */
#define ISSUE_APP_DRIVEN_FRAME	 (APP_CMND_SND_TEST_FRM | APP_CMND_SND_XID_FRM | APP_CMND_SND_SNRM_FRMR_FRM | APP_CMND_SND_DISC_RD_FRM | APP_CMND_SND_SIM_RIM_FRM)



/* definitions for the setting of the 'exception_cond_for_app' word */
#define STATIONS_CHANGED_STATE		  	0x0001	
/* the station has changed state */

#define RESERVED_FOR_STATIONS_GOING_INACTIVE 	0x0002	
/* used when the station becomes inactive */

#define REMOTE_STATION_TIMEOUT_NRM		0x0010	
/* a timeout occurred while the link was in the NRM */

#define RD_RX_FROM_SEC_WHILE_IN_NRM		0x0020	
/* a RD frame has been received from the secondary station */

#define DM_RX_FROM_SEC_WHILE_IN_NRM		0x0040	
/* a DM frame has been received from the secondary station */

#define RIM_FRM_RECEIVED_FROM_SECONDARY		0x0080	
/* a RIM frame has been received from the secondary station */

#define XID_FRM_AVAILABLE_FOR_APPLICATION	0x0100	
/* a XID frame is available for the application */

#define TEST_FRM_AVAILABLE_FOR_APPLICATION	0x0200	
/* a TEST frame is available for the application */

#define FRMR_CONDITION_TO_REPORT		0x0400	
/* there is a FRMR condition to report */

#define SNRM_RX_FROM_PRI_WHILE_IN_NRM		0x0800	
/* a SNRM was received from the primary station 
 * while the link  was in the NRM */

#define CHANGE_IN_MODEM_STATUS			0x8000	
/* a change in modem status occurred */



/* definitions for TEST/trace receive error buffer usage */
#define MAX_LGTH_TEST_FRAME			265
/* the maximum length of a TEST frame */

#define TEST_TRC_RX_ERR_FRM_BFR_NOT_IN_USE 	0x0000	
/* the buffer is available for use */

#define TEST_TRC_RX_ERR_FRM_BFR_IN_USE		0x8000	
/* the buffer is in use */



/* definitions for XID buffer usage */
#define SIZEOF_XID_I_FIELD_STORAGE_BFR		1000		
/* the size of the XID data storage buffer */

#define MAX_NUMBER_XID_I_FIELDS_STORED		100		
/* the maximum number of XID data fields stored at any one time */

#define MAX_LGTH_XID_FRAME			265
/* the maximum length of an XID frame */

#define XID_FRM_BFR_NOT_IN_USE			0x0000	
/* the buffer is available for use */

#define XID_FRM_BFR_IN_USE			0x8000	
/* the buffer is in use */



/* flags used by the 'send_Supervisory_or_I_frame()'
 * procedure to see if an I-frame may be transmitted */
#define I_FRAME_TRANSMISSION_FORBIDDEN 		0x00		
/* I-frame transmission is forbidden */

#define I_FRAME_TRANSMISSION_PERMITTED 		0x01		
/* I-frame transmission is permitted */



/* bit mappings for the user interface byte on a 
 * per station basis */
#define STATION_IS_IN_NRM			0x01		
/* the station is in the NRM */

#define STATION_HAS_I_FRM_FOR_APP		0x10	
/* the station has incomming I-frames available */

#define TX_WINDOW_OPEN_FOR_STATION		0x20	
/* the transmit window is open for this station */

#define SET_ALL_STATION_INTERFACE_BITS		0xFF	
/* set all interface bits for this station */



/* bit settings for the interface byte 
 * ADDR_GLOBAL_TX_BFR_STATUS_BYTE */

#define NO_SPACE_AVAILABLE_IN_TX_BFR		0x00	
/* no space is available in the global transmit buffer */

#define SPACE_AVAILABLE_IN_TX_BFR		0x01
/* space is currently available in the global transmit buffer */



/* definitions for line tracing bit settings for the 
 * 'line_trace_config' variable */

#define TRACE_ACTIVE			0x01		/* the trace is active */
#define TRACE_TIMER_ACTIVE		0x02		/* the trace timer is active */
#define TRACE_DELAY_MODE		0x04		/* enable the trace delay mode */
#define FLUSH_TRACE_STATISTICS		0x08		/* flush the trace statistics */
#define TRACE_I_FRMS			0x10		/* trace SDLC I-frames */
#define TRACE_SUPERVISORY_FRMS		0x20		/* trace Supervisory frames */
#define TRACE_UNNUM_FRMS		0x40		/* trace Unnumbered frames */
#define RETURN_TRC_SETUP		0x80		/* return the current line trace setup to the user */

/* the 'trc_deactivation_tmr' */
#define MAX_DEACTIVATION_TMR_VALUE	0x80		/* the maximum timer value */
#define HIGH_FOUR_BITS_OF_DEAC_TMR	0xF0		/* mask for preserving the high 4 bits of this timer */

/* trace types */
#define TRC_INCOMMING_FRAME		0x00		/* trace an incomming frame */
#define TRC_OUTGOING_FRAME		0x01		/* trace an outgoing frame */
#define TRC_FRAME_ERROR			0x02		/* trace a frame error */

/* trace error types */
#define NO_TRC_ERR			0x00		/* no trace error has occured */
#define TRC_RX_ABORT_ERR		0x12		/* trace a received abort frame */
#define TRC_RX_CRC_ERR			0x22		/* trace a received frame with a CRC error */
#define TRC_RX_OVRN_ERR			0x32		/* trace a received frame with a receiver overrun error */
#define TRC_RX_EX_FRM_LGTH		0x42		/* trace a received frame of excessive length */
#define TRC_TX_ABORT_TX_INT		0x72		/* trace an outgoing frame aborted due to a missed transmit */
																	/* interrupt */
#define TRC_TX_ABORT_UND_INT		0x82	
/* trace an outgoing frame aborted due to 
 * a missed transmit underrun interrupt */

/* general parameters for line tracing */
#define LGTH_LNE_TRC_STR_HDR		14 	
/* the header length of the structure used for line tracing */

#define DUMMY_CRC_TX_TRC		0xFFFF	
/* the CRC used for outgoing frames */

#define RX_STATUS_BYTE_TRACE_ACTIVE	0x80	
/* bit setting of the 'rx_status_byte' to indicate 
 * that the  trace is active */

#define LGTH_TRACE_STATISTIC_DATA	0x08		
/* the length of the trace statistic data */

/* bit settings for the interface byte 
 * ADDR_TRACE_DATA_AVAILABLE_BYTE */

#define NO_TRACE_DATA_CURRENLY_AVAILABLE	0x00	
/* trace data is available */

#define TRACE_DATA_CURRENLY_AVAILABLE		0x01	
/* no trace data is available */


/* the change in station status - used when 
 * checking to see if we must report this status 
 * change to the application */

#define STATION_ENTERING_NRM		0x01	
/* the station is entering NRM */

#define STATION_ENTERING_NDM		0x02
/* the station is entering NDM */



/* general constants */
#define MAX_SDLC_STATIONS_SUPPORTED 		255		
/* the maximum number of SDLC addresses 
 * supported (254 "real" addesses plus an 
 * all-poll address) */

#define HIGHEST_SDLC_ADDRESS 			0xFE
/* the highest valid SDLC station address */

#define SDLC_WINDOW 				0x07
/* the SDLC window */

#define HIGHEST_TIME_CONSTANT_VALUE		256
/* the highest time constant value permitted 
 * when setting the baud rate */

#define NUMBER_BITS_IN_FLAG_CHAR		8		
/* the number of bits in the HDLC flag character */

#define NUMBER_BITS_IN_FIVE_CHARS		40
/* the number of bits in five characters */

#define PROCESS_NEXT_STATION			0x80
/* an indication that the primary station must 
 * process the next station in the queue */

#define INTERNAL_I_FRM_BFR_FLUSH		0x80
/* a dummy command used for an internal I-frame buffer flush */

#define DEFAULT_APP_IRQ_RESPONSE_TIMEOUT	250
/* the default application IRQ timeout value */

#define RTS_LOW		0x00		/* the initial state of RTS */
#define NO 		0x00		/* logical 'no' */
#define YES 		0x01		/* logical 'yes' */
#define RESET 		0x00		/* flag in a reset state */
#define SET 		0x01		/* flag in a set state */
#define BIT_1_SET	0x02		/* bit one set mask */
#define ZERO		0x00		/* zero count */

#define HIGH_BIT_SET_IN_UNSIGNED_CHAR		0x80	
/* test for the high bit being set in an 
 * unsigned character */

/* general byte counts */

#define ONE_BYTE 	0x01		
#define TWO_BYTES 	0x02
#define THREE_BYTES 	0x03
#define FOUR_BYTES 	0x04
#define TWENTY_BYTES	20
#define FOUR_BITS	4
#define EIGHT_BITS	8



/* a general head/tail buffer list used for linked list pragmatics */
typedef struct {
	 char	head;
	 char	tail;
} BFR_LIST;



/* the I-frame receive structure */
typedef struct {
	char *ptr_Rx_I_frm;	/* a pointer to the I-frame */
	unsigned lgth_Rx_I_frm;	/* the length of the I-frame */
	char PF_bit;		/* the P/F bit as received */
} RX_I_FRAME_STRUCTURE;



/* the I-frame transmit structure */
typedef struct {
	char *ptr_Tx_I_frm;	/* a pointer to the I-frame */
	unsigned lgth_Tx_I_frm;	/* the length of the I-frame */
	char PF_bit;		/* the P/F bit to be transmitted */
} TX_I_FRAME_STRUCTURE;



/* SDLC station statistics */
/* Note - a number of the statistics only use 
 * four bits so as to save on data space. 
 * These statistics start at 0 and increment to a
 * maximum value of 15, before rotating over 
 * the top to 1. */

typedef struct {
	unsigned no_I_frms_Rx_and_stored;	/* the number of I-frames received and stored */
	char I_frms_Rx_seq_err_no_I_fld;	/* low: no I-frames Rx out of sequence, high: no I-frames Rx */
						/* with no I-field */
	char frms_Rx_bad_state_reserved;	/* low: no frames Rx in incorrect state, high: reserved */
	unsigned no_I_frms_Tx_and_acknowledged;	/* the number of I-frames transmitted and acknowledged */
	char no_I_frms_retransmitted;		/* the number of I-frames re-transmitted */
	char RR_statistics;			/* low: no RR frames Tx, high: no RR frames Rx */
	char RNR_statistics;			/* low: no RNR frames Tx, high: no RNR frames Rx */
	char REJ_statistics;			/* low: no REJ frames Tx, high: no REJ frames Rx */
	char TEST_statistics;			/* low: no TEST frames Tx, high: no TEST frames Rx */
	char XID_statistics;			/* low: no XID frames Tx, high: no XID frames Rx */
	char DISC_RD_statistics; 		/* primary - low: no DISC frames Tx, high: no RD frames Rx */
						/* secondary - low: no RD frames Tx, high: no DISC frames Rx */
	char SNRM_UA_statistics; 		/* primary - low: no SNRM frames Tx, high: no UA frames Rx */
						/* secondary - low: no UA frames Tx, high: no SNRM frames Rx */
	char DM_FRMR_statistics; 		/* primary - low: no DM frames Rx, high: no FRMR frames Rx */
						/* secondary - low: no DM frames Tx, high: no FRMR frames Tx */
	char SIM_RIM_statistics;		/* primary - low: no SIM frames Tx, high: no RIM frames Rx */
						/* secondary - low: no RIM frames Tx, high: no SIM frames Rx */
	char timeout_SNRM_DISC_statistic; 	/* primary - low: no timeouts on SNRM frames Tx, high: no */
						/* timeouts on DISC frames Tx */
	char timeout_SUP_I_SIM_statistic; 	/* primary - low: no timeouts on SUP or I frames Tx, high: no */
						/* timeouts on SIM frames Tx */
						/* secondary - low: no timeouts while in NRM */
	char timeout_TEST_XID_statistic;	/* primary - low: no timeouts on TEST frames Tx, high: no */
						/* timeouts on XID frames Tx */
} STATION_STATISTICS;



/* Secondary SDLC device parameters for keeping track 
 * of the status, sequence numbers etc. of the 
 * SDLC devices */
typedef struct {

	/* general variables */
	unsigned status;			/* the current station status */
	char internal_SDLC_state;		/* the state of the SDLC device (used for the state machine) */
	unsigned state_timer;			/* the station state timer */
	unsigned poll_timer_interval;		/* primary - the number of milliseconds between polls */
						/* secondary - the maximum permitted time between polls while */
						/* in the NRM before a secondary station enters the NDM */
	char no_consec_secondary_timeouts;	/* the number of consecutive secondary station timeouts */
						/* transmit variables */
	char no_I_frms_queued_for_Tx;		/* the number of I-frames queued for transmission */
	char no_Tx_I_frms_awaiting_acknowledgement; /* the number of transmitted I-frames awaiting acknowledgement */
	char Ns_variables;			/* low nibble - the number of the last I-frame transmitted */
						/* high nibble - the number of the last I-frame acknowledged */
	char I_frm_Tx_ack_status;		/* the I-frame transmit/acknowledgment status */
						/* receive variables */
	char Nr_variables;			/* low nibble - the number of the next I-frame we expect to */
						/* receive */
						/* high nibble - the number of I-frames currently available for */
						/* reception by the application statistics */
	STATION_STATISTICS statistics;		/* the station statistics */

} STATION_INFORMATION;



/* the structure for handling primary station XID data fields */
typedef struct {
	char SDLC_address;										/* the SDLC address associated with this XID field */
	char length_XID_I_field;								/* the length of the XID data field */
} XID_INFORMATION_STRUCT;




/* definitions for primary receive buffering */
/* the primary receive status element structure */
typedef struct {
	char opp_flag;											/* the 'opp_flag' */
	char *ptr_base_nxt_Rx_frame;							/* a pointer to the physical base address of the next frame */
	char reserved;											/* reserved */
} PRI_RX_STAT_EL_STRUCT;

#define MAX_NO_PRI_RX_BFR_STATUS_ELS		32			/* the maximum number of primary receive status elements */
#define MIN_NO_RECEIVE_BFRS			4			/* the minimum number of receive buffers */
#define MAX_NO_RECEIVE_BFRS			32			/* the maximum number of receive buffers */
#define SIZEOF_PRI_RX_STATUS_ELEMENT 		0x04		/* the size of a primary receive status element */
#define P0_MAX_SIZE_PRI_RX_BFR 			0xF000	/* Port 0 - for 128K board, the maximum size DMA Rx buffer goes */
																	/* from 0x10000 to 0x1EFFF (if no transmit buffering is used) */
#define P0_SIZE_PRI_RX_BFR_256K_RAM		0xF000	/* Port 0 - for 256K board, the DMA Rx buffer always goes from */
																	/* 0x10000 to 0x1EFFF, irrespective of the size of the Tx buffer */
#define P0_RX_DMA_AREA_BASE_ADDR_BYTE		0x00		/* Port 0 - the high byte of the base address of the DMA Rx */
																	/* buffer area */
#define P0_RX_DMA_AREA_BASE_ADDR		0x10000	/* Port 0 - the base address of the DMA Rx buffer area */
#define MAX_CONSEC_RX_BFRS_HANDLED		0x04		/* the maximum consecutive number of Rx buffers handled before */
																	/* continuing with the main 'C' loop */

/* the setting of the 'opp_flag' in the receive status element */
#define PRI_BFR_QUEUED_BUT_NOT_FREED 		0x08		/* a flag indicating that the primary receive buffer has been */
																	/* queued for application processing, but not yet freed by the */
																	/* application */
#define PRI_RX_BFR_AVAIL			0x80		/* bit to be set in the 'opp_flag' of the primary Rx status */
																	/* element to indicate that this element is available for */
																	/* processing by the application */

/* Z80 op codes for resetting a specified bit in the L register */
#define OP_CODE_RES_4_L				0xA5		/* reset bit 4 */
#define OP_CODE_RES_5_L				0xAD		/* reset bit 5 */
#define OP_CODE_RES_6_L				0xB5		/* reset bit 6 */
#define OP_CODE_RES_7_L				0xBD		/* reset bit 7 */

/* definitions for the 'P0_Tx_Rx_cfg' */
#define TX_RX_CFG_IDLE_MARK			0x01		/* we are configured to idle mark */
#define TX_RX_CFG_SW_CTS_RTS			0x02		/* we are configured for switched CTS/RTS */


/* definitions for the 'P0_Tx_status_byte' */
#define TX_STAT_DROP_RTS_AFTR_TX				0x01		/* we must drop RTS after frame transmission */
#define TX_STAT_IDLE_FLGS_AFTR_TX			0x02		/* we must idle flags after frame transmission */
#define TX_STAT_RTS_HI							0x08		/* RTS is currently set high */
#define TX_STAT_WT_CTS_HI						0x10		/* we are waiting for CTS to be set high */
#define TX_STAT_CURR_IDLE_FLAGS				0x20		/* we are currently idling flags */
#define TX_STAT_OPEN_FLG_TMR_ACTIVE			0x40		/* we have started a timer for transmitting an opening flag */
																	/* sequence */
#define TX_STAT_CLOSE_FLG_TMR_ACTIVE		0x80		/* we have started a timer for transmitting a closing flag */
																	/* sequence */


/* definitions for the 'P0_ESCC_WR10_cfg' */
#define ESCC_WR10_NRZ_IDLE_FLAGS				0x80		/* NRZ, idle flags */
#define ESCC_WR10_ENABLE_NRZI					0x20		/* enable NRZI */
#define ESCC_WR10_ENABLE_IDLE_MARK			0x08		/* enable 'idle mark' */


/* settings for the 'error_flag' */
#define RX_FRM_ABORT								0x01	/* the incoming frame was aborted */
#define RX_FRM_CRC_ERROR						0x02	/* the incoming frame has a CRC error */
#define RX_FRM_OVERRUN_ERROR					0x04	/* the incoming frame has an overrun error */


#define ONLY_INCL_ABORT_BIT_IN_RR0			0x80		/* only include the break/abort bit in the read RR0 byte */


/* definitions for the 'P0_baud_cal_status' variable */
#define BAUD_CAL_BUSY							0x01		/* busy with baud calibration */
#define BAUD_CAL_COMPLETED						0x02		/* baud calibration has been completed */
#define BAUD_CAL_COMPUTE_BAUD					0x04		/* the baud rate must be computed */
#define BAUD_CAL_FAIL_NO_CLK					0x10		/* baud calibration has failed - no Tx clock */
#define BAUD_CAL_FAIL_BAUD_HI					0x20		/* baud calibration has failed - Tx clock speed too high */
#define BAUD_CAL_SUCCESSFUL					0x80		/* baud calibration was successful */



/* constants for indicating the modem status */
#define SHOW_CHANGE_IN_DCD_OCCURRED			0x04		/* a change in DCD occured */
#define SHOW_CHANGE_IN_CTS_OCCURRED			0x10		/* a change in CTS occured */
#define SHOW_CTS_TIMEOUT_FAILURE_OCCURRED	0x80		/* a CTS timeout occurred */


/* Special UDP drivers management commands */
#define CPIPE_ENABLE_TRACING				0x50
#define CPIPE_DISABLE_TRACING				0x51
#define CPIPE_GET_TRACE_INFO				0x52
#define CPIPE_GET_IBA_DATA				0x53
#define CPIPE_FT1_READ_STATUS				0x54
#define CPIPE_DRIVER_STAT_IFSEND			0x55
#define CPIPE_DRIVER_STAT_INTR				0x56
#define CPIPE_DRIVER_STAT_GEN				0x57
#define CPIPE_FLUSH_DRIVER_STATS			0x58
#define CPIPE_ROUTER_UP_TIME				0x59
#define CPIPE_MPPP_TRACE_ENABLE				0x60
#define CPIPE_MPPP_TRACE_DISABLE			0x61
#define CPIPE_TE1_56K_STAT				0x62	/* TE1_56K */
#define CPIPE_GET_MEDIA_TYPE				0x63	/* TE1_56K */
#define CPIPE_FLUSH_TE1_PMON				0x64	/* TE1     */
#define CPIPE_READ_REGISTER				0x65	/* TE1_56K */
#define CPIPE_TE1_CFG					0x66	/* TE1     */

/* Driver specific commands for API */
#define	CHDLC_READ_TRACE_DATA		0xE4	/* read trace data */
#define TRACE_ALL                       0x00
#define TRACE_PROT			0x01
#define TRACE_DATA			0x02

#define UDPMGMT_SIGNATURE	"STPIPEAB"
#define UDPMGMT_UDP_PROTOCOL 0x11



typedef struct SDLC_CONFIGURATION {
	
	/* configuration variables */
	/* NOTE - these variables must be kept in this 
	 * order and so must be initialized */
	
	/* the station configuration (primary or secondary) */
	unsigned char station_configuration;		
	/* the baud rate to be generated */
	unsigned long baud_rate;	
	/* the maximum permitted Information field length supported */
	unsigned short max_I_field_length;
	/* miscellaneous operational configuration bits */
	unsigned short general_operational_config_bits;
	/* miscellaneous protocol configuration bits */
	unsigned short protocol_config_bits;
	/* bits to specify which exception conditions are reported to the application */
	unsigned short exception_condition_reporting_config;
	/* modem operation configuration bits */
	unsigned short modem_config_bits;
	/* the statistics format byte */
	unsigned short statistics_format;
	/* the slow poll interval for a primary station (milliseconds) */
	unsigned short pri_station_slow_poll_interval;
	/* the permitted secondary station_response timeout (milliseconds) */
	unsigned short permitted_sec_station_response_TO;
	/* the number of consecutive secondary timeouts while */
	/* in the NRM before a SNRM is issued */
	unsigned short no_consec_sec_TOs_in_NRM_before_SNRM_issued;
	/* the maximum Information field length permitted in an XID */
	/* frame sent by a primary station */
	unsigned short max_lgth_I_fld_pri_XID_frame;		
	/* the additional bit delay before transmitting the opening */
	/* character in a frame */
	unsigned short opening_flag_bit_delay_count;		
	/* the additional bit delay before dropping RTS */
	unsigned short RTS_bit_delay_count;
	/* the permitted CTS timeout for switched CTS/RTS configurations */
	/* (in 1000ths/second) */
	unsigned short CTS_timeout_1000ths_sec;	
	/* the adapter type and the CPU speed */
	unsigned char SDLA_configuration;			
	
}SDLC_CONFIGURATION_STRUCT;

typedef struct {
	unsigned char CHDLC_interrupt_triggers;	/* CHDLC interrupt trigger configuration */
	unsigned char IRQ;			/* IRQ to be used */
	unsigned short interrupt_timer;		/* interrupt timer */
	unsigned short misc_interrupt_bits;	/* miscellaneous bits */
}SDLC_INT_TRIGGERS_STRUCT;

/* the interrupt information structure */
typedef struct {
 	unsigned char interrupt_type;		/* type of interrupt triggered */
 	unsigned char interrupt_permission;	/* interrupt permission mask */
	unsigned char int_info_reserved[14];	/* reserved */
} INTERRUPT_INFORMATION_STRUCT;

/* the communications error statistics structure */
typedef struct {
	char Rx_overrun_err_cnt;
	char CRC_err_cnt;	
	char Rx_abort_cnt; 	
	char Rx_frm_lgth_err_cnt; 
	char Tx_abort_cnt; 
	char Tx_underrun_cnt;
	char msd_Tx_und_int_cnt;	
	char no_CTS_resp_to_RTS_cnt;
	char DCD_state_change_cnt;
	char CTS_state_change_cnt;		
} COMMS_ERROR_STATS_STRUCT;


typedef struct {

	unsigned long total_no_frms_Rx;
	unsigned long total_no_I_frms_Rx_and_stored;
	unsigned long total_no_frms_Tx;
	unsigned long total_no_I_frms_Tx_and_acknowledged;
	unsigned no_frames_Rx_address_invalid;
	unsigned no_frames_Rx_on_unconfigured_stations;
	unsigned no_short_frames_Rx;
	unsigned no_unsupported_frames_Rx;
	unsigned no_frms_Rx_incompat_station_config;
	unsigned frm_Rx_in_incorrect_state;
	unsigned no_Rx_XID_frms_disc_bfr_full;
	unsigned no_Rx_TEST_frms_disc_bfr_full;
	unsigned no_Rx_FRMR_frms_disc_bfr_full;
	unsigned Rx_resp_broadcast_XID_added_station;
	unsigned Rx_frm_discards_int_lvl;
	unsigned no_Tx_bridge_discard_count;
	unsigned no_IRQ_timeouts;

}SDLC_OPERATIONAL_STATS_STRUCT;


typedef struct {
	unsigned char station;
	unsigned char state;

}SDLC_STATE_STRUCT;

/* the line trace status element configuration structure */
typedef struct {
	unsigned short number_trace_status_elements ;	/* number of line trace elements */
	unsigned long base_addr_trace_status_elements ;	/* base address of the trace element list */
	unsigned long next_trace_element_to_use ;	/* pointer to the next trace element to be used */
	unsigned long base_addr_trace_buffer ;		/* base address of the trace data buffer */
	unsigned long end_addr_trace_buffer ;		/* end address of the trace data buffer */
} TRACE_STATUS_EL_CFG_STRUCT;

/* the line trace status element structure */
typedef struct {
	unsigned char opp_flag ;			/* opp flag */
	unsigned short trace_length ;		/* trace length */
	unsigned char trace_type ;		/* trace type */
	unsigned short trace_time_stamp ;	/* time stamp */
	unsigned short trace_reserved_1 ;	/* reserved for later use */
	unsigned long trace_reserved_2 ;		/* reserved for later use */
	unsigned long ptr_data_bfr ;		/* ptr to the trace data buffer */
} TRACE_STATUS_ELEMENT_STRUCT;


#pragma pack()

#define SIOC_WANPIPE_EXEC_CMD		SIOC_WANPIPE_DEVPRIVATE

#ifdef __KERNEL__

typedef struct wp_sdlc_reg {
	int   (*sdlc_stack_rx) (struct sk_buff *skb);
}wp_sdlc_reg_t;

extern int wanpipe_sdlc_unregister(netdevice_t *dev);
extern int wanpipe_sdlc_register(struct net_device *dev, void *wp_sdlc_reg);

/*__KERNEL__*/
#endif 

#endif
