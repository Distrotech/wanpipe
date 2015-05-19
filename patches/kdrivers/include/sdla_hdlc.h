/*****************************************************************************
* sdla_hdlc.h   HDLC API header file.
*
* Author:       Jaspreet Singh  <jaspreet@sangoma.com>
*
* Copyright:    (c) 1998-1997 Sangoma Technologies Inc.
*
*               This program is free software; you can redistribute it and/or
*               modify it under the terms of the GNU General Public License
*               as published by the Free Software Foundation; either version
*               2 of the License, or (at your option) any later version.
* ============================================================================
* Oct 14, 1998  Jaspreet Singh  o Initial Version.
*****************************************************************************/

#ifndef __SDLA_HDLC_H__
#define __SDLA_HDLC_H__

#pragma       pack(1)


/*      Status flag for determining whether to perform a check on socket receive *      queue.
 */
#define NO_SK_RX_CHECK          0x00
#define TOP_CHECK_SK_RX_Q       0x01
#define BTM_CHECK_SK_RX_Q       0x02


#define NO_READ_CMD     0x00
#define READ_CMD        0x01





/* 
 *	Constants defining the shared memory control block (mailbox)
 */

/* the base address of the HDLC mailbox structure on the adapter */
#define HDLC_MB_STRUCT_OFFSET		0x0000 	
/* the number of reserved bytes in the mailbox header area */
#define NUMBER_HDLC_MB_RES_BYTES	0x0A
/* the size of the actual mailbox data area */
#define SIZEOF_HDLC_MB_DATA_BFR		1008
#define MAX_NO_DATA_BYTES_IN_I_FRAME	4099 /* maximum length of the HDLC I-field */

/* Just the header, excluding the data area */
#define	MAILBOX_SIZE	30
/* 
 *	the control block mailbox structure for API purposes. 
 */
typedef struct {

	unsigned char opp_flag		;		/* the opp flag */
	unsigned char command		;		/* the command code */
	unsigned short buffer_length	;		/* the data length */
	unsigned char return_code	;		/* the return code */
	unsigned char PF_bit		;		/* the HDLC P/F bit */
	char MB_reserved[NUMBER_HDLC_MB_RES_BYTES] ;  /* for later use */
	char data[MAX_NO_DATA_BYTES_IN_I_FRAME] ;	/* the data area */

} HDLC_MAILBOX_STRUCT;


/* The following structure definition is for driver use ONLY */

typedef struct {

	unsigned char opp_flag		;		/* the opp flag */
	unsigned char command		;		/* the command code */
	unsigned short buffer_length	;		/* the data length */
	unsigned char return_code	;		/* the return code */
	unsigned char PF_bit		;		/* the HDLC P/F bit */
	char MB_reserved[NUMBER_HDLC_MB_RES_BYTES] ;  /* for later use */
	char data[SIZEOF_HDLC_MB_DATA_BFR] ;	/* the data area */

} TRUE_HDLC_MAILBOX_STRUCT;


typedef struct {
	pid_t			pid_num	;
	HDLC_MAILBOX_STRUCT	cmdarea	;

} CMDBLOCK_STRUCT;

/*
 *	Interface commands
 */

/* 
 *	global interface commands 
 */

#define READ_GLOBAL_EXCEPTION_CONDITION	0x01	/* read a global exception condition from the adapter */
#define SET_GLOBAL_CONFIGURATION	0x02	/* set the global operational configuration */
#define READ_GLOBAL_CONFIGURATION	0x03	/* read the global configuration */
#define READ_GLOBAL_STATISTICS		0x04	/* retrieve the global statistics */
#define FLUSH_GLOBAL_STATISTICS		0x05	/* flush the global statistics */
#define SET_MODEM_STATUS		0x06	/* set the status of DTR and/or RTS */
#define READ_MODEM_STATUS		0x07	/* read the current status of CTS and DCD */
#define READ_COMMS_ERROR_STATS		0x08	/* read the communication error statistics */
#define FLUSH_COMMS_ERROR_STATS		0x09	/* flush the communication error statistics */
#define SET_TRACE_CONFIGURATION		0x0A	/* set the line trace configuration */
#define READ_TRACE_CONFIGURATION	0x0B	/* read the line trace configuration */
#define READ_TRACE_STATISTICS		0x0C	/* read the trace statistics */
#define FLUSH_TRACE_STATISTICS		0x0D	/* flush the trace statistics */
#define FT1_MONITOR_STATUS_CTRL		0x1E	/* set the status of the S508/FT1 monitoring */
#define SET_FT1_MODE			0x1F	/* set the operational mode of the S508/FT1 module */

/* 
 *	HDLC-level interface commands 
 */

#define READ_HDLC_CODE_VERSION		0x20	/* read the HDLC code version */
#define READ_HDLC_EXCEPTION_CONDITION	0x21	/* read an HDLC exception condition from the adapter */
#define SET_HDLC_CONFIGURATION		0x22	/* set the HDLC operational configuration */
#define READ_HDLC_CONFIGURATION		0x23	/* read the current HDLC operational configuration */
#define ENABLE_HDLC_COMMUNICATIONS	0x24	/* enable HDLC communications */
#define DISABLE_HDLC_COMMUNICATIONS	0x25	/* disable HDLC communications */
#define HDLC_CONNECT			0x26	/* enter the HDLC ABM state */
#define HDLC_DISCONNECT			0x27	/* enter the HDLC disconnected state */
#define READ_HDLC_LINK_STATUS		0x28	/* read the HDLC link status */
#define READ_HDLC_OPERATIONAL_STATS	0x29	/* retrieve the HDLC operational statistics */
#define FLUSH_HDLC_OPERATIONAL_STATS	0x2A	/* flush the HDLC operational statistics */
#define SET_HDLC_BUSY_CONDITION		0x2B	/* force the HDLC code into a busy condition */
#define SEND_UI_FRAME			0x2C	/* transmit an Unnumbered Information frame */
#define SET_HDLC_INTERRUPT_TRIGGERS	0x30	/* set the HDLC application interrupt triggers */
#define READ_HDLC_INTERRUPT_TRIGGERS	0x31	/* read the HDLC application interrupt trigger configuration */


#define HDLC_SEND_NO_WAIT		0xE0	/* send I frames : Poll  */
#define HDLC_SEND_WAIT			0xE1	/* send I frames : Interrupt*/
#define HDLC_READ_NO_WAIT		0xE2	/* receive I frames : Poll */
#define HDLC_READ_WAIT			0xE3   /* receive I frames : Interrupt*/
#define HDLC_READ_TRACE_DATA		0xE4	/* receive Trace data */


/*
 *	Return codes from interface commands
 */

#define OK		0x00	/* the interface command was successfull */ 

/* 
 *	return codes from global interface commands 
 */

#define NO_GLOBAL_EXCEP_COND_TO_REPORT	0x01	/* there is no HDLC exception condition to report */
#define LGTH_GLOBAL_CFG_DATA_INVALID	0x01	/* the length of the passed global configuration data is invalid */
#define LGTH_TRACE_CFG_DATA_INVALID	0x01	/* the length of the passed trace configuration data is invalid */
#define IRQ_TIMEOUT_VALUE_INVALID	0x02	/* an invalid application IRQ timeout value was selected */
#define TRACE_CONFIG_INVALID		0x02	/* the passed line trace configuration is invalid */
#define ADAPTER_OPERATING_FREQ_INVALID	0x03	/* an invalid adapter operating frequency was selected */
#define TRC_DEAC_TMR_INVALID		0x03	/* the trace deactivation timer is invalid */
#define S508_FT1_ADPTR_NOT_PRESENT	0x0E	/* the S508/FT1 adapter is not present */
#define S508_FT1_MODE_SELECTION_BUSY	0x0F	/* the S508/FT1 adapter is busy selecting the operational mode */

/* 
 *	return codes from command READ_GLOBAL_EXCEPTION_CONDITION 
 */
#define EXCEP_MODEM_STATUS_CHANGE	0x10		/* a modem status change occurred */
#define EXCEP_TRC_DISABLED		0x11		/* the trace has been disabled */
	
/*	
 *	return codes from HDLC-level interface commands 
 */
#define NO_HDLC_EXCEP_COND_TO_REPORT	0x21	/* there is no HDLC exception condition to report */
#define HDLC_COMMS_DISABLED		0x21	/* communications are not currently enabled */
#define HDLC_COMMS_ENABLED		0x21	/* communications are currently enabled */
#define DISABLE_HDLC_COMMS_BEFORE_CFG	0x21	/* HDLC communications must be disabled before setting the configuration */
#define ENABLE_HDLC_COMMS_BEFORE_CONN	0x21	/* communications must be enabled before using the HDLC_CONNECT conmmand */
#define HDLC_CFG_BEFORE_COMMS_ENABLED	0x22	/* perform a SET_HDLC_CONFIGURATION before enabling comms */
#define SET_TRACE_CFG			0x22	/* perform a SET_TRACE_CONFIGURATION */
#define LGTH_HDLC_CFG_DATA_INVALID 	0x22	/* the length of the passed HDLC configuration data is invalid */
#define LGTH_HDLC_INT_DATA_INVALID	0x22	/* the length of the passed interrupt trigger data is invalid */
#define HDLC_LINK_NOT_IN_ABM		0x22	/* the HDLC link is not currently in the ABM */
#define HDLC_LINK_CURRENTLY_IN_ABM	0x22	/* the HDLC link is currently in the ABM */
#define NO_TX_BFRS_AVAILABLE		0x23	/* no buffers available for transmission */ 
#define INVALID_HDLC_APP_IRQ_SELECTED	0x23	/* in invalid IRQ was selected in the SET_HDLC_INTERRUPT_TRIGGERS */
#define INVALID_HDLC_CFG_DATA		0x23	/* the passed HDLC configuration data is invalid */
#define UI_FRM_TX_BFR_IN_USE		0x23	/* the buffer used for UI frame transmission is currently in use */
#define T3_LESS_THAN_T1			0x24	/* the configured T3 value is less than T1 */
#define HDLC_IRQ_TMR_VALUE_INVALID	0x24	/* an invalid application IRQ timer value was selected */
#define UI_FRM_TX_LGTH_INVALID		0x24	/* the length of the UI frame to be transmitted is invalid */
#define T4_LESS_THAN_T1			0x25	/* the configured T4 value is less than T1 */
#define BFR_LGTH_EXCESSIVE_FOR_BFR_CFG	0x26	/* the configured buffer length is excessive for the configuration */
#define INVALID_HDLC_COMMAND		0x4F	/* the defined HDLC interface command is invalid */

/* 
 *	return codes from command READ_HDLC_EXCEPTION_CONDITION 
 */
#define EXCEP_SABM_RX		0x30	/* a SABM frame was recvd in the ABM */
#define EXCEP_DISC_RX		0x31	/* a DISC frame was recvd in the ABM  */
#define EXCEP_DM_RX		0x32	/* a DM frame was recvd in the ABM  */
#define EXCEP_UA_RX		0x33	/* a UA frame was recvd in the ABM */
#define EXCEP_FRMR_RX		0x34	/* a FRMR frame was recvd in the ABM */
#define EXCEP_UI_RX		0x37	/* a UI frame was received */
#define EXCEP_SABM_TX_DM_RX	0x38	/* a SABM frame was transmitted due to the reception of a DM */
#define EXCEP_SABM_TX_UA_RX	0x39	/* a SABM frame was transmitted due to the reception of a UA */
/* while the link was in the ABM */
#define EXCEP_SABM_TX_FRMR_RX	0x3A	/* a SABM frame was transmitted due to the reception of a FRMR */
/* while the link was in the ABM */
#define EXCEP_SABM_TX_UNSOLIC_RESP_RX	0x3B	/* a SABM frame was transmitted due to the reception of an */
/* unsolicited response with the F bit set */
#define EXCEP_SABM_TX_N2_EXPIRY	0x3C	/* a SABM frame was transmitted due to an N2 count expiry */
#define EXCEP_FRMR_TX		0x3F	/* a FRMR frame was transmitted */
#define EXCEP_SABM_RETRY_LIM_EXCEEDED	0x40	/* the SABM retry limit was exceeded */
#define EXCEP_DISC_RETRY_LIM_EXCEEDED	0x41	/* the DISC retry limit was exceeded */
#define EXCEP_FRMR_RETRY_LIM_EXCEEDED	0x42	/* the FRMR retry limit was exceeded */
#define EXCEP_T3_TIMEOUT_EXCEEDED	0x45	/* the T3 timeout limit has been exceeded */

#define NO_TRACE_BUFFERS		0x60 	/* No trace buffers are avail */
#define TRACE_BUFFER_TOO_BIG		0x61
#define TRACE_BUFFER_NOT_STORED		0x62	 

/*
 * 	Constants for the SET_GLOBAL_CONFIGURATION/READ_GLOBAL_CONFIGURATION 
 *	commands
 */

/*	
 *	the global configuration structure 
 */
typedef struct {

	unsigned short adapter_config_options;	     /* configuration options */
	unsigned short app_IRQ_timeout;		   /* application IRQ timeout */
	unsigned long adapter_operating_frequency;	/* operating frequency*/

} GLOBAL_CONFIGURATION_STRUCT;

/*	settings for the 'app_IRQ_timeout' */
#define MAX_APP_IRQ_TIMEOUT_VALUE	5000	/* the maximum permitted IRQ 
						   timeout */



/* 
 *	Constants for the READ_GLOBAL_STATISTICS command
 */

/* 
 *	the global statistics structure 
 */
typedef struct {

	unsigned short app_IRQ_timeout_count;

} GLOBAL_STATS_STRUCT;



/*
 *	Constants for the READ_COMMS_ERROR_STATS command
 */

/*
 *	the communications error statistics structure
 */

typedef struct {

	unsigned char Rx_overrun_err_count;   /* receiver overrun error count */
	unsigned char CRC_err_count;  	          /* receiver CRC error count */
	unsigned char Rx_abort_count; 	       /* abort frames received count */
	unsigned char Rx_dis_pri_bfrs_full_count;  /* receiver disabled count */
	unsigned char comms_err_stat_reserved_1;    /* reserved for later use */
	unsigned char comms_err_stat_reserved_2;    /* reserved for later use */
	unsigned char missed_Tx_und_int_count;/* missed tx underrun intr count*/
	unsigned char comms_err_stat_reserved_3;    /* reserved for later use */
	unsigned char DCD_state_change_count;       /* DCD state change count */
	unsigned char CTS_state_change_count;	    /* CTS state change count */

} COMMS_ERROR_STATS_STRUCT;




/* 
 *	Constants used for line tracing
 */

/* 
 *	the trace configuration structure (SET_TRACE_CONFIGURATION/
 *	READ_TRACE_CONFIGURATION commands)
 */

typedef struct {

	unsigned char trace_config	;    /* trace configuration */
	unsigned short trace_deactivation_timer ;  /* trace deactivation timer */
	unsigned long ptr_trace_stat_el_cfg_struct ;   /* a pointer to the line trace element configuration structure */

} LINE_TRACE_CONFIG_STRUCT;

/* 'trace_config' bit settings */
#define TRACE_INACTIVE			0x00	/* trace is inactive */
#define TRACE_ACTIVE			0x01	/* trace is active */
#define TRACE_DELAY_MODE		0x04	/* operate the trace in the 
						   delay mode */
#define TRACE_I_FRAMES			0x08	/* trace I-frames */
#define TRACE_SUPERVISORY_FRAMES	0x10	/* trace Supervisory frames */
#define TRACE_UNNUMBERED_FRAMES		0x20	/* trace Unnumbered frames */

/*
 *	the line trace status element configuration structure
 */

typedef struct {

	unsigned short number_trace_status_elements ;	/* number of line trace elements */
	unsigned long base_addr_trace_status_elements ;/* base address of the trace element list */
	unsigned long next_trace_element_to_use ;	/* pointer to the next trace element to be used */
	unsigned long base_addr_trace_buffer ;		/* base address of the trace data buffer */
	unsigned long end_addr_trace_buffer ;		/* end address of the trace data buffer */

} TRACE_STATUS_EL_CFG_STRUCT;

/*
 *	the line trace status element structure
 */

typedef struct {

	unsigned char opp_flag		;		/* opp flag */
	unsigned short trace_length	;	/* trace length */
	unsigned char trace_type	;	/* trace type */
	unsigned short trace_time_stamp	;/* time stamp */
	unsigned short trace_reserved_1	;/* reserved for later use */
	unsigned long trace_reserved_2	;	/* reserved for later use */
	unsigned long ptr_data_bfr	;	/* pointer to the trace data buffer */

} TRACE_STATUS_ELEMENT_STRUCT;

/*	
 *	the line trace statistics structure
 */

typedef struct {

	unsigned long frames_traced_count;	    /* number of frames traced*/
	unsigned long trc_frms_not_recorded_count;  /* number of trace frames 
						       discarded */
	unsigned short trc_disabled_internally_count; /* number of times the 
							 trace was disabled 
							 internally */

} LINE_TRACE_STATS_STRUCT;



/* 
 *	Constants for the SET_HDLC_CONFIGURATION command
 */

/* 
 *	the HDLC configuration structure
 */

typedef struct {

	unsigned long baud_rate			;		/* the baud rate */	
	unsigned short line_config_options	;	/* line configuration options */
	unsigned short modem_config_options	;	/* modem configuration options */
	unsigned short HDLC_API_options		;	/* HDLC API options */
	unsigned short HDLC_protocol_options	;	/* HDLC protocol options */
	unsigned short HDLC_buffer_config_options ; /* HDLC buffer configuration options */
	unsigned short HDLC_statistics_options	;	/* HDLC operational statistics options */
	unsigned short configured_as_DTE	;	/* DTE or DCE configuration */
	unsigned short max_HDLC_I_field_length	;	/* the maximum length of the HDLC I-field */
	unsigned short HDLC_I_frame_window	;	/* k - the I-frame window (maximum number of outstanding I-frames) */
	unsigned short HDLC_T1_timer		;	/* the HDLC T1 timer */
	unsigned short HDLC_T2_timer		;	/* the HDLC T2 timer */
	unsigned short HDLC_T3_timer		;	/* the HDLC T3 timer */
	unsigned short HDLC_T4_timer		;	/* the HDLC T4 timer */
	unsigned short HDLC_N2_counter		;	/* the HDLC N2 counter */
	unsigned long ptr_shared_mem_info_struct ;/* a pointer to the shared memory area information structure */
	unsigned long ptr_HDLC_Tx_stat_el_cfg_struct ;/* a pointer to the transmit status element configuration structure */
	unsigned long ptr_HDLC_Rx_stat_el_cfg_struct ;/* a pointer to the receive status element configuration structure */

} HDLC_CONFIGURATION_STRUCT;

/* settings for the 'line_config_options' */
#define INTERFACE_LEVEL_V35		0x0000 /* use V.35 interface level */
#define INTERFACE_LEVEL_RS232		0x0001 /* use RS-232 interface level */

/* settings for the 'modem_config_options' */
#define DONT_RAISE_DTR_RTS_ON_EN_COMMS	0x0001 /* don't automatically raise DTR and RTS when performing an */

/* ENABLE_HDLC_COMMUNICATIONS command */

#define DONT_REPORT_CHG_IN_MODEM_STAT 	0x0002 /* don't report changes in modem status to the application */
#define DISABLE_DCD_CTS_INTERRUPTS	0x0004 /* ignore DCD and CTS interrupts on the adapter */

/* bit settings for the 'HDLC_API_options' */
#define PERMIT_HDLC_CONNECT_IN_ABM	0x0001 /* allow the use of the HDLC_CONNECT command while in the ABM */

/* bit settings for the 'HDLC_protocol_options' byte */
#define MOD_8_SELECTED		0x0000 /* use modulo 8 operation */
#define MOD_128_SELECTED	0x0001 /* use modulo 128 (extended) operation */
#define AUTO_MODULO_DETECTION	0x0002 /* use automatic modulus detection */
#define PASSIVE_LINK_SETUP	0x0004 /* no SABMs should be issued when setting up the link */
#define ENTER_DISC_PHASE_AFTR_DISC_SNT	0x0008 /* enter the disconnnected phase after issuing a DISC command */

/* settings for the 'HDLC_buffer_config_options' */
#define HDLC_I_FRM_RX_HYSTERESIS	0x000F /* the HDLC I-frame receive hysteresis */
#define HDLC_I_FRM_BFRS_3rd_LVL_PROT	0x0010 /* the HDLC I-frame buffers are to be used by a 3rd level protocol */

/* settings for the 'HDLC_statistics_options' */
#define HDLC_TX_I_FRM_BYTE_COUNT_STAT	0x0001 /* compute the number of I-frame bytes transmitted */
#define HDLC_RX_I_FRM_BYTE_COUNT_STAT	0x0002 /* compute the number of I-frame bytes received */
#define HDLC_TX_THROUGHPUT_STAT		0x0004 /* compute the I-frame transmit throughput */
#define HDLC_RX_THROUGHPUT_STAT		0x0008 /* compute the I-frame receive throughput */

/* permitted minimum and maximum values for setting the HDLC configuration */
#define MAX_BAUD_RATE 			2666000 /* maximum baud rate */
#define MIN_NO_DATA_BYTES_IN_I_FRAME	300  /* minimum length of the configured HDLC I-field */
#define MIN_PERMITTED_k_VALUE		0    /* minimum I-frame window size */
#define MAX_PERMITTED_k_VALUE		127  /* maximum I-frame window size */
#define MIN_PERMITTED_T1_VALUE		1	  /* minimum T1 */
#define MAX_PERMITTED_T1_VALUE		60000	  /* maximum T1 */
#define MIN_PERMITTED_T2_VALUE		0	  /* minimum T2 */
#define MAX_PERMITTED_T2_VALUE		60000	 /* maximum T2 */
#define MIN_PERMITTED_T3_VALUE		0	  /* minimum T3 */
#define MAX_PERMITTED_T3_VALUE		60000	  /* maximum T3 */
#define MIN_PERMITTED_T4_VALUE		0	  /* minimum T4 */
#define MAX_PERMITTED_T4_VALUE		60000	  /* maximum T4 */
#define MIN_PERMITTED_N2_VALUE		1	  /* minimum N2 */
#define MAX_PERMITTED_N2_VALUE		30	  /* maximum N2 */


/*
 *	Constants for the READ_HDLC_LINK_STATUS command
 */

/*
 *	the HDLC status structure 
 */

typedef struct {

	unsigned char HDLC_link_status;	/* HDLC link status (disconnect/ABM) */
	unsigned char modulus_type;	/* configured modulus type */
	unsigned char no_I_frms_for_app;/* number of I-frames available for the application */
	unsigned char receiver_status;	/* receiver status (enabled/disabled) */
	unsigned char LAPB_state;	/* internal LAPB state */
	unsigned char rotating_SUP_frm_count;	/* count of Supervisory frames received */

} HDLC_LINK_STATUS_STRUCT;

/* settings for the 'HDLC_link_status' variable */
#define HDLC_LINK_DISCONNECTED	0x00	/* the HDLC link is disconnected */
#define HDLC_LINK_IN_AB	0x01	/* the HDLC link is in the ABM (connected) */




/*
 *	Constants for the READ_HDLC_OPERATIONAL_STATS command
 */

/*
 *	the HDLC operational statistics structure 
 */

typedef struct {

	/* Information frame transmission statistics */
	unsigned long I_frames_Tx_ack_count;	/* I-frames transmitted (and acknowledged) count */
	unsigned long I_bytes_Tx_ack_count;	/* I-bytes transmitted (and acknowledged) count */
	unsigned long I_frm_Tx_throughput;	/* I-frame transmit throughput */
	unsigned long no_ms_for_HDLC_Tx_thruput_comp;/* millisecond time used for Tx throughput computation */
	unsigned long I_frames_retransmitted_count;	/* I-frames re-transmitted count */
	unsigned long I_bytes_retransmitted_count;	/* I-bytes re-transmitted count */
	unsigned short I_frms_not_Tx_lgth_err_count;	/* number of I-frames not transmitted (length error) */
	unsigned short Tx_I_frms_disc_st_chg_count;	/* the number of I-frames discarded (change in the LAPB state) */
	unsigned long reserved_I_frm_Tx_stat;			/* reserved for later use */

	/* Information frame reception statistics */
	unsigned long I_frames_Rx_buffered_count;	/* I-frames received (and buffered) count */
	unsigned long I_bytes_Rx_buffered_count;	/* I-bytes received (and buffered) count */
	unsigned long I_frm_Rx_throughput;		/* I-frame receive throughput */
	unsigned long no_ms_for_HDLC_Rx_thruput_comp;/* millisecond time used for Rx throughput computation */
	unsigned short I_frms_Rx_too_long_count;	/* I-frames received of excessive length count */
	unsigned short I_frms_Rx_seq_err_count;		/* out of sequence I-frames received count */
	unsigned long reserved_I_frm_Rx_stat1;		/* reserved for later use */
	unsigned long reserved_I_frm_Rx_stat2;		/* reserved for later use */
	unsigned long reserved_I_frm_Rx_stat3;		/* reserved for later use */

	/* Supervisory frame transmission/reception statistics */
	unsigned short RR_Tx_count;	/* RR frames transmitted count */
	unsigned short RR_Rx_count;	/* RR frames received count */
	unsigned short RNR_Tx_count;	/* RNR frames transmitted count */
	unsigned short RNR_Rx_count;	/* RNR frames received count */
	unsigned short REJ_Tx_count;	/* REJ frames transmitted count */
	unsigned short REJ_Rx_count;	/* REJ frames received count */

	/* Unnumbered frame transmission/reception statistics */
	unsigned short SABM_Tx_count;	/* SABM frames transmitted count */
	unsigned short SABM_Rx_count;	/* SABM frames received count */
	unsigned short SABME_Tx_count;	/* SABME frames transmitted count */
	unsigned short SABME_Rx_count;	/* SABME frames received count */
	unsigned short DISC_Tx_count;	/* DISC frames transmitted count */
	unsigned short DISC_Rx_count;	/* DISC frames received count */
	unsigned short DM_Tx_count;	/* DM frames transmitted count */
	unsigned short DM_Rx_count;	/* DM frames received count */
	unsigned short UA_Tx_count;	/* UA frames transmitted count */
	unsigned short UA_Rx_count;	/* UA frames received count */
	unsigned short FRMR_Tx_count;	/* FRMR frames transmitted count */
	unsigned short FRMR_Rx_count;	/* FRMR frames received count */
	unsigned short UI_Tx_count;	/* UI frames transmitted count */
	unsigned short UI_Rx_buffered_count;/* UI frames received and buffered count */
	unsigned long reserved_Sup_Unnum_stat1; /* reserved for later use */
	unsigned long reserved_Sup_Unnum_stat2;	/* reserved for later use */

	/* Incomming frames with a format error statistics */
	unsigned short Rx_frm_shorter_32_bits_count;	/* frames received of less than 32 bits in length count */
	unsigned short Rx_I_fld_Sup_Unnum_frm_count;	/* Supervisory/Unnumbered frames received with */

	/* illegal I-fields count */
	unsigned short Rx_frms_invld_HDLC_addr_count;/* frames received with an invalid HDLC address count */
	unsigned short Rx_invld_HDLC_ctrl_fld_count;	/* frames received of an invalid/unsupported */
											/* control field count */
	unsigned long reserved_frm_format_err1;	/* reserved for later use */
	unsigned long reserved_frm_format_err2;	/* reserved for later use */

	/* FRMR/UI reception error statistics */
	unsigned short Rx_FRMR_frms_discard_count;/* incomming FRMR frames discarded count */
	unsigned short Rx_UI_frms_discard_count;/* incomming UI frames discarded count */
	unsigned short UI_frms_Rx_invld_lgth_count;/* UI frames of invalid length received count */
	unsigned short reserved_Rx_err_stat1;	/* reserved for later use */
	unsigned long reserved_Rx_err_stat2;	/* reserved for later use */
	unsigned long reserved_Rx_err_stat3;	/* reserved for later use */

	/* HDLC timeout/retry statistics */
	unsigned short T1_timeout_count;	/* T1 timeouts count */
	unsigned short T3_timeout_count;	/* T3 timeouts count */
	unsigned short T4_timeout_count;	/* T4 timeouts count */
	unsigned short reserved_timeout_stat;	/* reserved for later use */
	unsigned short N2_threshold_reached_count;/* N2 threshold reached count */
	unsigned short reserved_threshold_stat;	/* reserved for later use */
	unsigned long To_retry_reserved_stat;	/* reserved for later use */

	/* miscellaneous statistics */
	unsigned long reserved_misc_stat1;	/* reserved for later use */
	unsigned long reserved_misc_stat2;	/* reserved for later use */
	unsigned long reserved_misc_stat3;	/* reserved for later use */
	unsigned long reserved_misc_stat4;	/* reserved for later use */

} HDLC_OPERATIONAL_STATS_STRUCT;




/*
 *	Constants for the SEND_UI_FRAME command
 */

#define MAX_LENGTH_UI_DATA	512	/* maximum UI frame data length */

/* 
 *	the structure used for UI frame transmission/reception 
 */

typedef struct {
	unsigned char HDLC_address;		/* HDLC address in the frame */
	unsigned char UI_reserved;		/* reserved for internal use */
	char data[MAX_LENGTH_UI_DATA];		/* UI data area */
} UI_FRAME_STRUCT;




/* 
 *	Constants for using application interrupts
 */

/*
 *	the structure used for the SET_HDLC_INTERRUPT_TRIGGERS/
 *	READ_HDLC_INTERRUPT_TRIGGERS command 
 */

typedef struct {

	unsigned char HDLC_interrupt_triggers;	/* HDLC interrupt trigger configuration */
	unsigned char IRQ;			/* IRQ to be used */
	unsigned short interrupt_timer;		/* interrupt timer */

} HDLC_INT_TRIGGERS_STRUCT;

/* 'HDLC_interrupt_triggers' bit settings */
#define APP_INT_ON_RX_FRAME	0x01	/* interrupt on I-frame reception */
#define APP_INT_ON_TX_FRAME	0x02	/* interrupt when an I-frame may be transmitted */
#define APP_INT_ON_COMMAND_COMPLETE	0x04	/* interrupt when an interface command is complete */
#define APP_INT_ON_TIMER		0x08	/* interrupt on a defined millisecond timeout */
#define APP_INT_ON_GLOBAL_EXCEP_COND 	0x10	/* interrupt on a global exception condition */
#define APP_INT_ON_HDLC_EXCEP_COND	0x20	/* interrupt on an HDLC exception condition */
#define APP_INT_ON_TRACE_DATA_AVAIL	0x80	/* interrupt when trace data is available */


/* 
 *	the HDLC interrupt information structure 
 */
typedef struct {

 	unsigned char interrupt_type		; /* type of interrupt triggered */
 	unsigned char interrupt_permission	;	/* interrupt permission mask */
	unsigned char int_info_reserved[14]	;	/* reserved */

} HDLC_INTERRUPT_INFO_STRUCT;

/* interrupt types indicated at 'interrupt_type' byte of the 
   HDLC_INTERRUPT_INFO_STRUCT */
#define NO_APP_INTS_PEND	0x00	/* no interrups are pending */
#define RX_APP_INT_PEND		0x01   /*receive interrupt is pending */
#define TX_APP_INT_PEND		0x02	/* a transmit interrupt is pending */
#define COMMAND_COMPLETE_APP_INT_PEND	0x04	/* a 'command complete' interrupt is pending */
#define TIMER_APP_INT_PEND	0x08	/* a timer interrupt is pending */
#define GLOBAL_EXCEP_COND_APP_INT_PEND 	0x10	/* a global exception condition interrupt is pending */
#define HDLC_EXCEP_COND_APP_INT_PEND 	0x20	/* an HDLC exception condition interrupt is pending */
#define TRACE_DATA_AVAIL_APP_INT_PEND 	0x80	/* a trace data available interrupt is pending */



/* 
 *	Constants for Information frame transmission
 */

/*
 *	the I-frame transmit status element configuration structure 
 */

typedef struct {

	unsigned short number_Tx_status_elements ;    /* number of transmit status elements */
	unsigned long base_addr_Tx_status_elements ;  /* base address of the transmit element list */
	unsigned long next_Tx_status_element_to_use ; /* pointer to the next transmit element to be used */

} HDLC_TX_STATUS_EL_CFG_STRUCT;

/*
 *	the I-frame transmit status element structure 
 */
typedef struct {

	unsigned char opp_flag		;		/* opp flag */
	unsigned short I_frame_length	;	/* length of the frame*/
	unsigned char P_bit		;	/* P-bit setting in the frame */
	unsigned long reserved_1	;	/* reserved for internal use */
	unsigned long reserved_2	;	/* reserved for internal use */
	unsigned long ptr_data_bfr	;	/* pointer to the data area */

} HDLC_I_FRM_TX_STATUS_EL_STRUCT;



/*
 *	Constants for Information frame reception
 */

/*
 *	the I-frame receive status element configuration structure 
 */

typedef struct {

	unsigned short number_Rx_status_elements	;	/* number of receive status elements */
	unsigned long base_addr_Rx_status_elements	;	/* base address of the receive element list */
	unsigned long next_Rx_status_element_to_use	;	/* pointer to the next receive element to be used */
	unsigned long base_addr_Rx_buffer	;		/* base address of the receive data buffer */
	unsigned long end_addr_Rx_buffer	;		/* end address of the receive data buffer */
} HDLC_RX_STATUS_EL_CFG_STRUCT;

/* 
 *	the I-frame receive status element structure 
 */
typedef struct {

	unsigned char opp_flag		;		/* opp flag */
	unsigned short I_frame_length	;	/*length of the recvd frame */
	unsigned char P_bit		;	/* P-bit setting in the frame */
	unsigned long reserved_1	;	/* reserved for internal use */
	unsigned long reserved_2	;	/* reserved for internal use */
	unsigned long ptr_data_bfr	;	/* pointer to the data area */

} HDLC_I_FRM_RX_STATUS_EL_STRUCT;



/*
 *	Constants defining the shared memory information area
 */



/*	
 *	the global information structure 
 */

typedef struct {

        unsigned char global_status		; /* global status */
        unsigned char modem_status		;/* current modem status*/
        unsigned char global_excep_conditions	; /* global exception conditions */
        unsigned char glob_info_reserved[5]	;   /* reserved */
        unsigned char code_name[4]		; /* code name */
        unsigned char code_version[4]		; /* code version */

} GLOBAL_INFORMATION_STRUCT;


/* 
 *	the S508/FT1 information structure 
 */

typedef struct {

        unsigned char parallel_port_A_input	;      /* input - parallel port A */
        unsigned char parallel_port_B_input	;       /* input - parallel port B */
        unsigned char FT1_info_reserved[14]	;       /* reserved */

} FT1_INFORMATION_STRUCT;

/* 
 *	the HDLC information structure 
 */

typedef struct {

        unsigned char HDLC_status	; 	/* HDLC status */
        unsigned char HDLC_excep_frms_Rx ;      /* HDLC exception conditions - received frames */
        unsigned char HDLC_excep_frms_Tx ;      /* HDLC exception conditions - transmitted frames */
        unsigned char HDLC_excep_miscellaneous ; /* HDLC exception conditions - miscellaneous */
        unsigned char rotating_SUP_frm_count ;   /* rotating Supervisory frame count */
        unsigned char LAPB_status	; /* internal LAPB status */
        unsigned char internal_HDLC_status ;    /* internal HDLC status */
        unsigned char HDLC_info_reserved[9] ;   /* reserved */

} HDLC_INFORMATION_STRUCT;




/* 
 *	the HDLC shared memory area information structure 
 */

typedef struct {

	GLOBAL_INFORMATION_STRUCT global_info 	;	/* the global information structure */
	FT1_INFORMATION_STRUCT FT1_info		;	/* the S508/FT1 information structure */
	HDLC_INFORMATION_STRUCT HDLC_info	;	/* the HDLC information structure */
	HDLC_INTERRUPT_INFO_STRUCT HDLC_interrupt_info ;	/* the HDLC interrupt information structure */

} HDLC_SHARED_MEMORY_INFO_STRUCT;

#pragma       pack()
#endif
