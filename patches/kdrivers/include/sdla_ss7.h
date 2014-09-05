/*
 ****************************************************************************
 *                                                                          *
 * SS7L2API.H - the 'C' header file for the Sangoma S508/S514 SS7 code API. *
 *                                                                          *
 ****************************************************************************
*/

#ifndef _SDLA_SS7_H
#define _SDLA_SS7_H


#pragma pack(1)

/* ----------------------------------------------------------------------------
 *        Constants defining the shared memory control block (mailbox)
 * --------------------------------------------------------------------------*/

#define PRI_BASE_ADDR_MB_STRUCT	 		0xE000 	/* the base address of the mailbox structure (primary port) */
#define NUMBER_MB_RESERVED_BYTES			0x0B		/* the number of reserved bytes in the mailbox header area */
#define SIZEOF_MB_DATA_BFR					2032		/* the size of the actual mailbox data area */

/* the control block mailbox structure */
#if 0
typedef struct {
	unsigned char opp_flag;								/* the opp flag */
	unsigned char command;								/* the user command */
	unsigned short buffer_length;						/* the data length */
  	unsigned char return_code;							/* the return code */
	char MB_reserved[NUMBER_MB_RESERVED_BYTES];	/* reserved for later use */
	char data[SIZEOF_MB_DATA_BFR];					/* the data area */
} SS7_MAILBOX_STRUCT;
#endif


/* ----------------------------------------------------------------------------
 *                        Interface commands
 * --------------------------------------------------------------------------*/

/* global interface commands */
#define READ_GLOBAL_EXCEPTION_CONDITION	0x01	/* read a global exception condition from the adapter */
#define SET_GLOBAL_CONFIGURATION				0x02	/* set the global operational configuration */
#define READ_GLOBAL_CONFIGURATION			0x03	/* read the global configuration */
#define READ_GLOBAL_STATISTICS				0x04	/* retrieve the global statistics */
#define FLUSH_GLOBAL_STATISTICS				0x05	/* flush the global statistics */
#define SET_MODEM_STATUS						0x06	/* set the status of DTR and/or RTS */
#define READ_MODEM_STATUS						0x07	/* read the current status of CTS and DCD */
#undef READ_COMMS_ERROR_STATS
#define READ_COMMS_ERROR_STATS				0x08	/* read the communication error statistics */
#undef FLUSH_COMMS_ERROR_STATS
#define FLUSH_COMMS_ERROR_STATS				0x09	/* flush the communication error statistics */
#define SET_TRACE_CONFIGURATION				0x0A	/* set the line trace configuration */
#define READ_TRACE_CONFIGURATION				0x0B	/* read the line trace configuration */
#define READ_TRACE_STATISTICS					0x0C	/* read the trace statistics */
#define FLUSH_TRACE_STATISTICS				0x0D	/* flush the trace statistics */

/* SS7-level interface commands */
#define READ_SS7_CODE_VERSION					0x20	/* read the SS7 code version */
#define L2_READ_EXCEPTION_CONDITION			0x21	/* L2 - read an exception condition from the adapter */
#define L2_SET_CONFIGURATION					0x22	/* L2 - set the operational configuration */
#define L2_READ_CONFIGURATION					0x23	/* L2 - read the current configuration */
#define L2_READ_LINK_STATUS					0x24	/* L2 - read the link status */
#define L2_READ_OPERATIONAL_STATS			0x25	/* L2 - retrieve the operational statistics */
#define L2_FLUSH_OPERATIONAL_STATS			0x26	/* L2 - flush the operational statistics */
#define L2_READ_HISTORY_TABLE					0x27	/* L2 - read the history table */
#define L2_FLUSH_HISTORY_TABLE				0x28	/* L2 - flush the history table */
#define L2_SET_INTERRUPT_TRIGGERS			0x30	/* L2 - set the application interrupt triggers */
#define L2_READ_INTERRUPT_TRIGGERS			0x31	/* L2 - read the application interrupt trigger configuration */
#define L2_POWER_ON								0x40	/* L2 - power on */
#define L2_EMERGENCY								0x41	/* L2 - emergency */
#define L2_EMERGENCY_CEASES					0x42	/* L2 - emergency ceases */
#define L2_START									0x43	/* L2 - start */
#define L2_STOP									0x44	/* L2 - stop */
#define L2_RESUME									0x45	/* L2 - resume (ANSI) / local processor recovered (ITU) */
#define L2_RETRIEVE_BSNT						0x46	/* L2 - retrieve BSNT */
#define L2_RETRIEVAL_REQ_AND_FSNC			0x47	/* L2 - retrieval request and FSNC */
#define L2_CLEAR_BUFFERS						0x48	/* L2 - clear buffers (ANSI) / flush buffers (ITU) */
#define L2_CLEAR_RTB								0x49	/* L2 - clear RTB */
#define L2_LOCAL_PROCESSOR_OUTAGE			0x4A	/* L2 - local processor outage */
#define L2_CONTINUE								0x4B	/* L2 - continue (ITU) */
#define L2_SET_TX_CONG_CFG						0x50	/* L2 - set the transmission congestion configuration */
#define L2_READ_TX_CONG_STATUS				0x51	/* L2 - read the transmission congestion status */
#define L2_SET_RX_CONG_CFG						0x52	/* L2 - set the receive congestion configuration */
#define L2_READ_RX_CONG_STATUS				0x53	/* L2 - read the receive congestion status */
#define L2_DEBUG									0x5F	/* L2 - debug */



/* ----------------------------------------------------------------------------
 *                     Return codes from interface commands
 * --------------------------------------------------------------------------*/

#define OK											0x00	/* the interface command was successful */

/* return codes from global interface commands */
#define NO_GLOBAL_EXCEP_COND_TO_REPORT		0x01	/* there is no global exception condition to report */
#define LGTH_GLOBAL_CFG_DATA_INVALID		0x01	/* the length of the passed global configuration data is invalid */
#define LGTH_TRACE_CFG_DATA_INVALID			0x01	/* the length of the passed trace configuration data is invalid */
#define IRQ_TIMEOUT_VALUE_INVALID			0x02	/* an invalid application IRQ timeout value was selected */
#define TRACE_CONFIG_INVALID					0x02	/* the passed line trace configuration is invalid */
#define ADAPTER_OPERATING_FREQ_INVALID		0x03	/* an invalid adapter operating frequency was selected */
#define TRC_DEAC_TMR_INVALID					0x03	/* the trace deactivation timer is invalid */

/* return codes from command READ_GLOBAL_EXCEPTION_CONDITION */
#define EXCEP_MODEM_STATUS_CHANGE			0x10		/* a modem status change occurred */
#define EXCEP_TRC_DISABLED						0x11		/* the trace has been disabled */
#define EXCEP_APP_IRQ_TIMEOUT					0x12		/* an application IRQ timeout has occurred */
#define EXCEP_FT1_INS_ALARM_COND				0x16		/* an FT1 in-service/alarm condition has occurred */

/* return codes from SS7 L2 interface commands */
#define NO_L2_EXCEP_COND_TO_REPORT			0x21	/* there is no L2 exception condition to report */
#define L2_STOP_BEFORE_CFG						0x21	/* stop L2 communications before setting the configuration */
#define L2_INVALID_CONGESTION_STATUS		0x21	/* invalid congestion status selected */
#define L2_CFG_BEFORE_POWER_ON				0x22	/* perform a L2_SET_CONFIGURATION before L2_POWER_ON */
#define LGTH_L2_CFG_DATA_INVALID 			0x22	/* the length of the passed configuration data is invalid */
#define LGTH_INT_TRIGGERS_DATA_INVALID		0x22	/* the length of the passed interrupt trigger data is invalid */
#define LGTH_L2_TX_CONG_DATA_INVALID 		0x22	/* the length of the passed Tx congestion data is invalid */
#define LGTH_L2_RX_CONG_DATA_INVALID 		0x22	/* the length of the passed Rx congestion data is invalid */
#define INVALID_L2_CFG_DATA					0x23	/* the passed SS7 configuration data is invalid */
#define INVALID_L2_TX_CONG_CFG				0x23	/* the passed Tx congestion configuration data is invalid */
#define INVALID_L2_RX_CONG_CFG				0x23	/* the passed Rx congestion configuration data is invalid */
#define L2_INVALID_FSNC							0x23	/* an invalid FSNC value was selected */
#define INVALID_IRQ_SELECTED					0x23	/* an invalid IRQ was selected in the SET_SS7_INTERRUPT_TRIGGERS */
#define IRQ_TMR_VALUE_INVALID					0x24	/* an invalid application IRQ timer value was selected */
#define L2_INVALID_STATE_FOR_CMND			0x25	/* the command is invalid for the current SS7 L2 state */
#define L2_INVALID_PROTOCOL_FOR_CMND		0x26	/* the command is invalid for the protocol specification */
#define INVALID_SS7_COMMAND					0x8F	/* the defined SS7 interface command is invalid */

/* return codes from command L2_READ_EXCEPTION_CONDITION */
#define L2_EXCEP_IN_SERVICE					0x30	/* the link is 'In service' */
#define L2_EXCEP_OUT_OF_SERVICE				0x31	/* the link is 'Out of service' */
#define L2_EXCEP_REMOTE_PROC_OUTAGE			0x32	/* the remote processor has reported an outage */
#define L2_EXCEP_REMOTE_PROC_RECOVERED		0x33	/* the remote processor has recovered after an outage */
#define L2_EXCEP_RTB_CLEARED					0x34	/* a L2_CLEAR_RTB command has been completed */
#define L2_EXCEP_RB_CLEARED					0x35	/* a L2_CLEAR_BUFFERS command has been completed */
#define L2_EXCEP_BSNT							0x36	/* a L2_RETRIEVE_BSNT command has been completed */
#define L2_EXCEP_RETRIEVAL_COMPLETE			0x37	/* a L2_RETRIEVAL_REQ_AND_FSNC command has been completed */
#define L2_EXCEP_CONG_OUTBOUND				0x38	/* outbound congestion status change */
#define L2_EXCEP_CONG_INBOUND					0x39	/* inbound congestion status change */
#define L2_EXCEP_CORRECT_SU					0x3A	/* a correct SU was received after a link inactivity timeout */
#define L2_EXCEP_TX_MSU_DISC_LGTH_ERR		0x3E	/* an outbound MSU was discarded due to a length error */
#define L2_EXCEP_RX_MSU_DISC					0x3F	/* a received MSU was discarded due to a format error */

/* qualifiers for the L2_EXCEP_OUT_OF_SERVICE return code */
typedef struct {											/* the structure used for retrieving the 'out of service' qualifier */
	unsigned char OOS_qualifier;						/* the 'out of service' qualifier */
} L2_EXCEP_OOS_STRUCT;

/* L2_EXCEP_OUT_OF_SERVICE qualifiers */
#define L2_OOS_L3_CMND_STOP					0x10	/* an application level L2_STOP command was issued */
#define L2_OOS_L3_CMND_PROC_OUT				0x11	/* an application level L2_LOCAL_PROCESSOR_OUTAGE command was issued */
#define L2_OOS_IAC_ALIGN_NOT_POSS			0x21	/* link failure - alignment not possible (alignment control) */
#define L2_OOS_TXC_T6_EXP						0x30	/* link failure - T6 expired (transmission control) */
#define L2_OOS_TXC_T7_EXP						0x31	/* link failure - T7 expired (transmission control) */
#define L2_OOS_RXC_SIO							0x40	/* link failure - SIO received (reception control) */
#define L2_OOS_RXC_SIN							0x41	/* link failure - SIN received (reception control) */
#define L2_OOS_RXC_SIE							0x42	/* link failure - SIE received (reception control) */
#define L2_OOS_RXC_SIOS							0x43	/* link failure - SIOS received (reception control) */
#define L2_OOS_RXC_ABNORMAL_BSNR				0x45	/* link failure - abnormal BSNR (reception control) */
#define L2_OOS_RXC_ABNORMAL_FIBR				0x46	/* link failure - abnormal FIBR (reception control) */
#define L2_OOS_SUERM_ERR						0x50	/* link failure - SU in error (SUERM) */
#define L2_OOS_SUERM_INACTIVITY				0x51	/* link failure - inactivity timeout (SUERM) */




/* ----------------------------------------------------------------------------
 * Constants for the SET_GLOBAL_CONFIGURATION/READ_GLOBAL_CONFIGURATION commands
 * --------------------------------------------------------------------------*/

/* the global configuration structure */
typedef struct {
	unsigned short adapter_config_options;			/* adapter configuration options */
	unsigned short app_IRQ_timeout;					/* application IRQ timeout */
	unsigned long adapter_operating_frequency;	/* adapter operating frequency */
} GLOBAL_CONFIGURATION_STRUCT;

/* settings for the 'adapter_config_options' */
#define ADPTR_CFG_S514							0x0001 /* S514 adapter */

/* settings for the 'app_IRQ_timeout' */
#define MAX_APP_IRQ_TIMEOUT_VALUE			5000	/* the maximum permitted IRQ timeout */



/* ----------------------------------------------------------------------------
 *             Constants for the READ_GLOBAL_STATISTICS command
 * --------------------------------------------------------------------------*/

/* the global statistics structure */
typedef struct {
	unsigned short app_IRQ_timeout_count;        /* application IRQ timeout count */
} GLOBAL_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *             Constants for the READ_COMMS_ERROR_STATS command
 * --------------------------------------------------------------------------*/

/* the communications error statistics structure */
typedef struct {
	unsigned short Rx_overrun_err_count; 			/* receiver overrun error count */
	unsigned short CRC_err_count;						/* receiver CRC error count */
	unsigned short Rx_abort_count; 					/* abort frames received count */
	unsigned short Rx_dis_pri_bfrs_full_count;	/* receiver disabled count */
	unsigned short reserved_1;							/* reserved */
	unsigned short reserved_2;
	unsigned short reserved_3;
	unsigned short DCD_state_change_count;			/* DCD state change count */
	unsigned short CTS_state_change_count;			/* CTS state change count */
} COMMS_ERROR_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *                  Constants used for line tracing
 * --------------------------------------------------------------------------*/

/* the trace configuration structure (SET_TRACE_CONFIGURATION/READ_TRACE_CONFIGURATION commands) */
typedef struct {
	unsigned char trace_config;						/* trace configuration */
	unsigned short trace_deactivation_timer;		/* trace deactivation timer */
	unsigned long ptr_trace_stat_el_cfg_struct;	/* a pointer to the line trace element configuration structure */
} LINE_TRACE_CONFIG_STRUCT;

/* 'trace_config' bit settings */
#define TRACE_INACTIVE							0x00	/* trace is inactive */
#define TRACE_ACTIVE								0x01	/* trace is active */
#define TRACE_DELAY_MODE						0x04	/* operate the trace in the delay mode */
#define TRACE_FISU								0x08	/* trace FISUs */
#define TRACE_LSSU								0x10	/* trace LSSUs */
#define TRACE_MSU									0x20	/* trace MSUs */

/* permitted range for the 'trace_deactivation_timer' */
#define MIN_TRC_DEAC_TMR_VAL					0		/* the minimum trace deactivation timer value */
#define MAX_TRC_DEAC_TMR_VAL					8000	/* the maximum trace deactivation timer value */

/* the line trace status element configuration structure */
typedef struct {
	unsigned short number_trace_status_elements;	/* number of line trace elements */
	unsigned long base_addr_trace_status_elements;/* base address of the trace element list */
	unsigned long next_trace_element_to_use;		/* pointer to the next trace element to be used */
	unsigned long base_addr_trace_buffer;			/* base address of the trace data buffer */
	unsigned long end_addr_trace_buffer;			/* end address of the trace data buffer */
} TRACE_STATUS_EL_CFG_STRUCT;

/* the line trace status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short trace_length;						/* trace length */
	unsigned char trace_type;							/* trace type */
	unsigned short trace_time_stamp;					/* time stamp */
	unsigned short trace_reserved_1;					/* reserved for later use */
	unsigned long trace_reserved_2;					/* reserved for later use */
	unsigned long ptr_data_bfr;						/* pointer to the trace data buffer */
} TRACE_STATUS_ELEMENT_STRUCT;

/* the line trace statistics structure */
typedef struct {
	unsigned long frames_traced_count;				/* number of frames traced */
	unsigned long trc_frms_not_recorded_count;	/* number of trace frames discarded */
	unsigned short trc_disabled_internally_count;/* number of times the trace was disabled internally */
} LINE_TRACE_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 * Constants for the L2_SET_CONFIGURATION/L2_READ_CONFIGURATION command
 * --------------------------------------------------------------------------*/

/* the SS7 L2 configuration structure */
typedef struct {
	unsigned long baud_rate;							/* the baud rate */	
	unsigned short line_config_options;				/* line configuration options */
	unsigned short modem_config_options;			/* modem configuration options */
	unsigned short modem_status_timer;				/* timer for monitoring modem status changes */
	unsigned short L2_API_options;					/* L2 API options */
	unsigned short L2_protocol_options;				/* L2 protocol options */
	unsigned short L2_protocol_specification;		/* L2 protocol specification */
	unsigned short L2_stats_history_options;		/* L2 operational statistics options */
	unsigned short max_length_MSU_SIF;				/* maximum length of the MSU Signal Information Field */
	unsigned short max_unacked_Tx_MSUs;				/* maximum number of unacknowledged outgoing MSUs */
	unsigned short link_inactivity_timer;			/* link inactivity timer */
	unsigned short T1_timer;							/* T1 'aligned/ready' timer */
	unsigned short T2_timer;							/* T2 'not aligned' timer */
	unsigned short T3_timer;							/* T3 'aligned' timer */
	unsigned short T4_timer_emergency;				/* T4 'emergency proving period' timer */
	unsigned short T4_timer_normal;					/* T4 'normal proving period' timer */
	unsigned short T5_timer;							/* T5 'sending SIB' timer */
	unsigned short T6_timer;							/* T6 'remote congestion' timer */
	unsigned short T7_timer;							/* T7 'excessive delay of acknowledgement' timer */
	unsigned short T8_timer;							/* T8 'errored interval' timer */
	unsigned short N1;									/* maximum sequence number values for retransmission (PCR) */
	unsigned short N2;									/* maximum MSU octets for retransmission (PCR) */
	unsigned short Tin;									/* normal alignment error rate monitor threshold */
	unsigned short Tie;									/* emergency alignment error rate monitor threshold */
	unsigned short SUERM_error_threshold;			/* SUERM error rate threshold */
	unsigned short SUERM_number_octets;				/* SUERM octet counter */
	unsigned short SUERM_number_SUs;					/* SUERM number SUs/SU error */
	unsigned short SIE_interval_timer;				/* timer interval between LSSUs (SIE) */
	unsigned short SIO_interval_timer;				/* timer interval between LSSUs (SIO) */
	unsigned short SIOS_interval_timer;				/* timer interval between LSSUs (SIOS) */
	unsigned short FISU_interval_timer;				/* timer interval between FISUs */
	unsigned long ptr_shared_mem_info_struct;		/* a pointer to the shared memory area information structure */
	unsigned long ptr_L2_Tx_stat_el_cfg_struct;	/* a pointer to the transmit status element configuration structure */
	unsigned long ptr_L2_Rx_stat_el_cfg_struct;	/* a pointer to the receive status element configuration structure */
} L2_CONFIGURATION_STRUCT;

/* settings for the 'line_config_options' */
#define INTERFACE_LEVEL_V35					0x0000 /* V.35 interface level */
#define INTERFACE_LEVEL_RS232					0x0001 /* RS-232 interface level */

/* settings for the 'modem_config_options' */
#define DONT_RAISE_DTR_RTS_ON_EN_COMMS		0x0001 /* don't automatically raise DTR and RTS when performing an */
																 /* ENABLE_SS7_COMMUNICATIONS command */
#define DONT_REPORT_CHG_IN_MODEM_STAT 		0x0002 /* don't report changes in modem status to the application */

/* bit settings for the 'L2_protocol_options' */
#define LSSU_TWO_BYTE_SF						0x0001 /* LSSUs have a 2-byte Status Field */
#define MODULO_4096								0x0001 /* modulo 4096 */
#define AUTO_START_WHEN_OUT_OF_SERVICE		0x0010 /* automatically start link when out of service */
#define AUTO_RECOVERY_AFTR_REM_PROC_REC	0x0020 /* automatic local recovery after remote processor recovery */
#define LINK_INACTIVITY_TMR_SUERM			0x0040 /* include a link inactivity timer in the SUERM */
#define PREVENTIVE_CYCLIC_RETRANS			0x1000 /* use preventive cyclic retransmission */

/* bit settings for the 'L2_protocol_specification' */
#define L2_PROTOCOL_ANSI						0x0001 /* L2 protocol is ANSI T1.111.3 */
#define L2_PROTOCOL_ITU							0x0002 /* L2 protocol is ITU-T Q.703 */
#define L2_PROTOCOL_NTT							0x0004 /* L2 protocol is NTT (Japan) */

/* settings for the 'L2_stats_history_options' */
#define L2_TX_SIF_BYTE_COUNT_STAT			0x0001 /* record the number of MSU SIF bytes transmitted */
#define L2_RX_SIF_BYTE_COUNT_STAT			0x0002 /* record the number of MSU SIF bytes received */
#define L2_TX_THROUGHPUT_STAT					0x0004 /* compute the MSU transmit throughput */
#define L2_RX_THROUGHPUT_STAT					0x0008 /* compute the MSU frame receive throughput */
#define L2_HIST_ALL								0x0100 /* include all state transactions in the history table */
#define L2_HIST_MANUAL_FLUSH					0x0200 /* manually flush the L2 history table */

/* permitted minimum and maximum values for setting the SS7 configuration - note that all timer values are in units */
/* of 1/100th of a second */
#define MAX_BAUD_RATE							64000 	/* maximum baud rate */
#define MIN_PERMITTED_MODEM_TIMER			0		  	/* minimum modem status timer */
#define MAX_PERMITTED_MODEM_TIMER			5000	  	/* maximum modem status timer */
#define MAX_LENGTH_MSU_SIF						272		/* maximum length of the MSU Signal Information Field */
#define MAX_UNACKED_TX_MSUs					126		/* maximum number of unacknowledged outgoing MSUs */
#define MIN_INACTIVITY_TIMER					1			/* minimum link inactivity timer value */
#define MAX_INACTIVITY_TIMER					60000		/* maximum link inactivity timer value */
#define MIN_T1_TIMER								1			/* minimum T1 timer value */
#define MAX_T1_TIMER								60000		/* maximum T1 timer value */
#define MIN_T2_TIMER								1			/* minimum T2 timer value */
#define MAX_T2_TIMER								60000		/* maximum T2 timer value */
#define MIN_T3_TIMER								1			/* minimum T3 timer value */
#define MAX_T3_TIMER								60000		/* maximum T3 timer value */
#define MIN_T4_TIMER								1			/* minimum T4 timer value */
#define MAX_T4_TIMER								60000		/* maximum T4 timer value */
#define MIN_T5_TIMER								1			/* minimum T5 timer value */
#define MAX_T5_TIMER								60000		/* maximum T5 timer value */
#define MIN_T6_TIMER								1			/* minimum T6 timer value */
#define MAX_T6_TIMER								60000		/* maximum T6 timer value */
#define MIN_T7_TIMER								1			/* minimum T7 timer value */
#define MAX_T7_TIMER								60000		/* maximum T7 timer value */
#define MIN_T8_TIMER								1			/* minimum T8 timer value */
#define MAX_T8_TIMER								60000		/* maximum T8 timer value */
#define MIN_N1										1			/* minimum N1 */
#define MAX_N1										4095		/* maximum N1 */
#define MIN_N2										1			/* minimum N2 */
#define MAX_N2										64000		/* maximum N2 */
#define MIN_Tin									1			/* minimum normal alignment error rate monitor threshold */
#define MAX_Tin									1000 		/* maximum normal alignment error rate monitor threshold */
#define MIN_Tie									1			/* minimum emergency alignment error rate monitor threshold */
#define MAX_Tie									1000 		/* maximum emergency alignment error rate monitor threshold */
#define MIN_SUERM_ERROR_THRESHOLD			1			/* minimum SUERM error rate threshold */
#define MAX_SUERM_ERROR_THRESHOLD			1024		/* maximum SUERM error rate threshold */
#define MIN_SUERM_NUMBER_OCTETS				1			/* minimum SUERM octet counter */
#define MAX_SUERM_NUMBER_OCTETS				1024		/* maximum SUERM octet counter */
#define MIN_SUERM_NUMBER_SUs					1			/* minimum SUERM number SUs/SU error */
#define MAX_SUERM_NUMBER_SUs					1024		/* maximum SUERM number SUs/SU error */
#define MIN_SIE_INTERVAL_TIMER				1			/* minimum timer interval between LSSUs (SIE) */
#define MAX_SIE_INTERVAL_TIMER				60000		/* maximum timer interval between LSSUs (SIE) */
#define MIN_SIO_INTERVAL_TIMER				1			/* minimum timer interval between LSSUs (SIO) */
#define MAX_SIO_INTERVAL_TIMER				60000		/* maximum timer interval between LSSUs (SIO) */
#define MIN_SIOS_INTERVAL_TIMER				1			/* minimum timer interval between LSSUs (SIOS) */
#define MAX_SIOS_INTERVAL_TIMER				60000		/* maximum timer interval between LSSUs (SIOS) */
#define MIN_FISU_INTERVAL_TIMER				1			/* minimum timer interval between FISUs */
#define MAX_FISU_INTERVAL_TIMER				60000		/* maximum timer interval between FISUs */



/* ----------------------------------------------------------------------------
 *             Constants for the L2_READ_LINK_STATUS command
 * --------------------------------------------------------------------------*/

/* the SS7 status structure */
typedef struct {
	unsigned short L2_link_status;					/* L2 link status */
	unsigned short no_Tx_MSU_bfrs_occupied;		/* number of Tx MSU buffers occupied */
	unsigned short no_Rx_MSUs_avail_for_L3;		/* number of MSUs available for the application */
	unsigned char receiver_status;					/* receiver status (enabled/disabled) */
} L2_LINK_STATUS_STRUCT;

/* settings for 'L2_link_status' variable */
#define L2_LINK_STAT_POWER_OFF				0x0001	/* power off */	
#define L2_LINK_STAT_OUT_OF_SERVICE			0x0002	/* out of service */
#define L2_LINK_STAT_INIT_ALIGN				0x0004	/* initial alignment */
#define L2_LINK_STAT_ALIGNED_READY			0x0008	/* aligned ready */
#define L2_LINK_STAT_ALIGNED_NOT_READY		0x0010	/* aligned/not ready */
#define L2_LINK_STAT_IN_SERVICE				0x0020	/* in service */
#define L2_LINK_STAT_PROCESSOR_OUTAGE		0x0040	/* processor outage */



/* ----------------------------------------------------------------------------
 *           Constants for the L2_READ_OPERATIONAL_STATS command
 * --------------------------------------------------------------------------*/

/* the L2 operational statistics structure */
typedef struct {

/* MSU transmission statistics */
	unsigned long MSU_Tx_ack_count;						/* number of MSUs transmitted and acknowledged */
	unsigned long SIF_bytes_Tx_ack_count; 				/* number of SIF bytes transmitted and acknowledged */
	unsigned long MSU_re_Tx_count;							/* number of MSUs re-transmitted */
	unsigned long SIF_bytes_re_Tx_count; 				/* number of SIF bytes re-transmitted */
	unsigned long MSU_Tx_throughput;						/* transmit throughput */
	unsigned long no_hs_for_MSU_Tx_thruput_comp;	/* 1/100th second time used for the Tx throughput computation */
	unsigned short Tx_MSU_disc_lgth_err_count;			/* number of outgoing MSUs discarded (length error) */
	unsigned long reserved_MSU_Tx_stat1;					/* reserved for later use */
	unsigned long reserved_MSU_Tx_stat2;					/* reserved for later use */
	unsigned long reserved_MSU_Tx_stat3;					/* reserved for later use */

/* MSU reception statistics */
	unsigned long MSU_Rx_count;								/* number of MSUs received */
	unsigned long SIF_bytes_Rx_count;						/* number of SIF bytes received */
	unsigned long MSU_Rx_throughput;						/* receive throughput */
	unsigned long no_hs_for_MSU_Rx_thruput_comp;	/* 1/100th second time used for the Rx throughput computation */
	unsigned short Rx_MSU_disc_short_count;				/* received MSUs discarded (too short) */
	unsigned short Rx_MSU_disc_long_count;				/* received MSUs discarded (too long) */
	unsigned short Rx_MSU_disc_bad_LI_count;				/* received MSUs discarded (bad LI) */
	unsigned long Rx_MSU_disc_cong_count;				/* received MSUs discarded (congestion) */
	unsigned long reserved_MSU_Rx_stat1;					/* reserved for later use */
	unsigned long reserved_MSU_Rx_stat2;					/* reserved for later use */
	unsigned long reserved_MSU_Rx_stat3;					/* reserved for later use */

/* General SU transmission/reception statistics */
	unsigned long SU_Tx_count;								/* number of SUs transmitted */
	unsigned long SU_Rx_count;								/* number of SUs received */
	unsigned long LSSU_Tx_count;							/* number of LSSUs transmitted */
	unsigned long LSSU_Rx_count;							/* number of LSSUs received */
	unsigned long FISU_Tx_count;							/* number of FISUs transmitted */
	unsigned long FISU_Rx_count;							/* number of FISUs received */
	unsigned long reserved_SU_stat1;					/* reserved for later use */
	unsigned long reserved_SU_stat2;					/* reserved for later use */

/* Incoming SUs with a format error statistics */
	unsigned short Rx_SU_disc_short_count;					/* SUs discarded - too short */
	unsigned short Rx_SU_disc_long_count;					/* SUs discarded - too long */
	unsigned short Rx_LSSU_disc_short_count;				/* LSSUs discarded - too short */
	unsigned short Rx_LSSU_disc_long_count;					/* LSSUs discarded - too long */
	unsigned short Rx_LSSU_disc_invalid_SF_count;			/* LSSUs discarded - invalid Status Field */
	unsigned short Rx_FISU_disc_short_count;				/* FISUs discarded - too short */
	unsigned short Rx_FISU_disc_long_count;					/* FISUs discarded - too long */
	unsigned long reserved_SU_format_err1;				/* reserved for later use */
	unsigned long reserved_SU_format_err2;				/* reserved for later use */
	unsigned long reserved_SU_format_err3;				/* reserved for later use */
	unsigned long reserved_SU_format_err4;				/* reserved for later use */

/* Incoming frames discarded statistics */
	unsigned long Rx_SU_disc_RC_idle_count;					/* SUs discarded - Reception Control in 'idle' state */
	unsigned long Rx_SU_disc_MSU_FISU_NA_count;			/* SUs discarded - MSU/FISU not accepted */

/* SS7 timeout/retry statistics */
	unsigned long To_retry_reserved_stat1;				/* reserved for later use */
	unsigned long To_retry_reserved_stat2;				/* reserved for later use */
	unsigned long To_retry_reserved_stat3;				/* reserved for later use */

/* link state statistics */
	unsigned short link_in_service_count;				/* number of times that the link went 'in service' */
	unsigned short link_start_fail_T1_exp_count;		/* link 'start' failure (T1 expired) count */
	unsigned short align_fail_T2_exp_count;				/* link alignment failure (T2 expired) count */
	unsigned short align_fail_T3_exp_count;				/* link alignment failure (T3 expired) count */
	unsigned short link_fail_abnrm_BSNR_Rx_count;		/* link failure (abnormal BSNR received) count */
	unsigned short link_fail_abnrm_FIBR_Rx_count;		/* link failure (abnormal FIBR received) count */
	unsigned short link_fail_T6_exp_count;				/* link failure (T6 expired) count */
	unsigned short link_fail_T7_exp_count;				/* link failure (T6 expired) count */
	unsigned short link_fail_SUERM_count;				/* link failure (SUERM) count */
	unsigned short link_fail_EIM_count;					/* link failure (EIM) count */
	unsigned short link_fail_inactivity_count;			/* link failure (inactivity timeout) count */
	unsigned short link_out_of_serv_SIO_Rx_count;		/* link 'out of service' due to reception of SIO */
	unsigned short link_out_of_serv_SIN_Rx_count;		/* link 'out of service' due to reception of SIN */
	unsigned short link_out_of_serv_SIE_Rx_count;		/* link 'out of service' due to reception of SIE */
	unsigned short link_out_of_serv_SIOS_Rx_count;	/* link 'out of service' due to reception of SIOS */
	unsigned long link_status_reserved_stat1;			/* reserved for later use */
	unsigned long link_status_reserved_stat2;			/* reserved for later use */

/* processor outage statistics */
	unsigned short loc_proc_out_count;					/* local processor outage count */
	unsigned short rem_proc_out_count;					/* rem processor outage count */

/* miscellaneous statistics */
	unsigned long NACK_Tx_count;							/* number of negative acknowledgements transmitted */
	unsigned long NACK_Rx_count;							/* number of negative acknowledgements received */
	unsigned long abnrm_BSNR_Rx_count;					/* 'abnormal BSNR received' count */
	unsigned long abnrm_FIBR_Rx_count;					/* 'abnormal FIBR received' count */
	unsigned long abnrm_FSNR_Rx_count;					/* 'abnormal FSNR received' count */
	unsigned long reserved_misc_stat1;					/* reserved for later use */
	unsigned long reserved_misc_stat2;					/* reserved for later use */
	unsigned long reserved_misc_stat3;					/* reserved for later use */
	unsigned long reserved_misc_stat4;					/* reserved for later use */

/* DAEDR statistics */
	unsigned long DAEDR_OCM_7_consec_1s;					/* DAEDR - number of octet counting mode conditions (7 consecutive 1s) */
	unsigned long DAEDR_OCM_Nmax;							/* DAEDR - number of octet counting mode conditions (Nmax + 1) */

/* congestion statistics */
	unsigned long start_outbound_cong_count;			/* start of outbound congestion */
	unsigned long start_inbound_cong_disc_count;		/* start of inbound 'congestion discard' */
	unsigned long start_inbound_cong_acc_count;		/* start of inbound 'congestion accept' */
	unsigned long start_auto_inbound_cong_count;		/* start of inbound congestion (instigated internally) */

} L2_OPERATIONAL_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *           Constants for the L2_READ_HISTORY_TABLE command
 * --------------------------------------------------------------------------*/

/* the L2 operational statistics structure */

typedef struct {
	unsigned char function;
	unsigned short action;
	unsigned short status_before_action;
	unsigned char LSSU_SF;
	unsigned short time;
} L2_HISTORY_STRUCT;

/* L2 history 'function' definitions */
#define L2_HISTORY_LSC 							0x00	/* Link State Control */
#define L2_HISTORY_IAC					 		0x01	/* Initial Alignment Control */
#define L2_HISTORY_POC							0x02	/* Processor Outage Control */
#define L2_HISTORY_DAEDR					 	0x03	/* Delimination, Alignment and Error Detection (Receiving) */
#define L2_HISTORY_DAEDT						0x04	/* Delimination, Alignment and Error Detection (Transmitting) */
#define L2_HISTORY_TXC							0x05	/* Transmission Control */
#define L2_HISTORY_RXC					 		0x06	/* Reception Control */
#define L2_HISTORY_AERM						 	0x07	/* Alignment Error Rate Monitor */
#define L2_HISTORY_SUERM						0x08	/* Signal Unit Error Rate Monitor */
#define L2_HISTORY_CC							0x09	/* Congestion Control */
#define L2_HISTORY_EIM							0x0A	/* Errored Interval Monitor */

/* L2 history 'action' definitions (Link State Control) */
#define LSC_ACT_POWER_ON_RETRIEVE_BSNT		0x0001	/* power on / retrieve BSNT */
#define LSC_ACT_START_RTB_CLEARED			0x0002	/* start / RTB cleared */
#define LSC_ACT_EMERGENCY_CONTINUE			0x0004	/* emergency / continue (ITU) */
#define LSC_ACT_EMERG_CEASES					0x0008	/* emergency ceases */
#define LSC_ACT_LOC_PROC_OUT					0x0010	/* local processor outage */
#define LSC_ACT_RESUME							0x0020	/* resume */
#define LSC_ACT_CLEAR_BFRS						0x0040	/* clear buffers */
#define LSC_ACT_ALIGN_CPLT_RET_REQ_FSNC	0x0080	/* alignment complete / retrieval request and FSNC */
#define LSC_ACT_STOP								0x0100	/* stop */
#define LSC_ACT_LINK_FAIL						0x0200	/* link failure */
#define LSC_ACT_ALIGN_NOT_POSS_CLR_RTB		0x0400	/* aligment not possible / clear RTB */
#define LSC_ACT_FISU_MSU_RX					0x0800	/* FISU/MSU received */
#define LSC_ACT_SIPO								0x1000	/* SIPO */
#define LSC_ACT_SIO_SIOS						0x2000	/* SIO, SIOS */
#define LSC_ACT_T1_EXPIRED_NO_PROC_OUT		0x4000	/* timer T1 expired / no processor outage (ITU) */
#define LSC_ACT_SIO_SIN_SIE_SIOS 			0x8000	/* SIO, SIN, SIE, SIOS */

/* L2 history 'status_before_action' definitions (Link State Control) */
#define LSC_STAT_POWER_OFF						0x0001	/* power off */	
#define LSC_STAT_OUT_OF_SERVICE				0x0002	/* out of service */
#define LSC_STAT_INIT_ALIGN					0x0004	/* initial alignment */
#define LSC_STAT_ALIGNED_READY				0x0008	/* aligned ready */
#define LSC_STAT_ALIGNED_NOT_READY			0x0010	/* aligned/not ready */
#define LSC_STAT_IN_SERVICE					0x0020	/* in service */
#define LSC_STAT_PROCESSOR_OUTAGE			0x0040	/* processor outage */
#define LSC_STAT_EMERGENCY						0x0080	/* emergency */
#define LSC_STAT_LOC_PROC_OUT					0x0100	/* local processor outage */
#define LSC_STAT_REM_PROC_OUT					0x0200	/* remote processor outage (ANSI) */
#define LSC_STAT_PROC_OUT						0x0400	/* processor outage (ITU) */
#define LSC_STAT_T1_RUNNING					0x0800	/* timer T1 running */
#define LSC_STAT_LVL_3_IND_RX					0x1000	/* Level 3 indication received */

/* L2 history 'action' definitions (Initial Alignment Control) */
#define IAC_ACT_EMERGENCY						0x0001	/* emergency */
#define IAC_ACT_START							0x0002	/* start */
#define IAC_ACT_SIO_SIN							0x0004	/* SIO, SIN */
#define IAC_ACT_SIE								0x0008	/* SIE */
#define IAC_ACT_STOP								0x0010	/* stop */
#define IAC_ACT_T2_EXPIRED						0x0020	/* timer T2 expired */
#define IAC_ACT_SIN								0x0040	/* SIN */
#define IAC_ACT_T3_EXPIRED						0x0080	/* timer T3 expired */
#define IAC_ACT_SIOS								0x0100	/* SIOS */
#define IAC_ACT_CORRECT_SU						0x0200	/* correct SU */
#define IAC_ACT_T4_EXPIRED						0x0400	/* timer T4 expired */
#define IAC_ACT_ABRT_PRV						0x0800	/* abort proving */
#define IAC_ACT_SIO								0x1000	/* SIO */

/* L2 history 'status_before_action' definitions (Initial Alignment Control) */
#define IAC_STAT_IDLE 							0x0001	/* idle */
#define IAC_STAT_NOT_ALIGNED					0x0002	/* not aligned */
#define IAC_STAT_ALIGNED						0x0004	/* aligned */
#define IAC_STAT_PROVING						0x0008	/* proving */
#define IAC_STAT_FURTHER_PRV					0x0010	/* further proving */
#define IAC_STAT_EMERGENCY						0x0020	/* emergency */
#define IAC_STAT_T2_RUNNING					0x0040	/* timer T2 running */
#define IAC_STAT_T3_RUNNING					0x0080	/* timer T3 running */
#define IAC_STAT_T4_RUNNING					0x0100	/* timer T4 running */

/* L2 history 'action' definitions (Processor Outage Control) */
#define POC_ACT_LOC_PROC_OUT					0x0001	/* local processor outage */
#define POC_ACT_REM_PROC_OUT					0x0002	/* remote processor outage */
#define POC_ACT_STOP								0x0004	/* stop */
#define POC_ACT_LOC_PROC_RECOVERED			0x0008	/* local processor recovered */
#define POC_ACT_REM_PROC_RECOVERED			0x0010	/* remote processor recovered */

/* L2 history 'status_before_action' definitions (Processor Outage Control) */
#define POC_STAT_IDLE 							0x0001	/* idle */
#define POC_STAT_LOC_PROC_OUT					0x0002	/* local processor outage */
#define POC_STAT_REM_PROC_OUT					0x0004	/* remote processor outage */
#define POC_STAT_BOTH_PROC_OUT				0x0008	/* both processors out */

/* L2 history 'action' definitions (Delimination, Alignment and Error Detection - Receiving) */
#define DAEDR_ACT_START							0x0001	/* start */
#define DAEDR_ACT_7_CONSEC_ONES				0x0002	/* 7 consecutive one's */
#define DAEDR_ACT_Nmax_PLS_1_OCT_NO_FLG	0x0004	/* Nmax + 1 octets without flags */
#define DAEDR_ACT_16_OCTETS					0x0008	/* 16 octets */
#define DAEDR_ACT_BITS_RX						0x0010	/* bits received */

/* L2 history 'status_before_action' definitions (Delimination, Alignment and Error Detection - Receiving) */
#define DAEDR_STAT_IDLE 						0x0001	/* idle */
#define DAEDR_STAT_IN_SERVICE					0x0002	/* in service */
#define DAEDR_STAT_OCTET_COUNT_MODE			0x0004	/* octet counting mode */

/* L2 history 'action' definitions (Delimination, Alignment and Error Detection - Transmitting) */
#define DAEDT_ACT_START							0x0001	/* start */
#define DAEDT_ACT_SIGNAL_UNIT					0x0002	/* signal unit */

/* L2 history 'status_before_action' definitions (Delimination, Alignment and Error Detection - Transmitting) */
#define DAEDT_STAT_IDLE 						0x0001	/* idle */
#define DAEDT_STAT_IN_SERVICE					0x0002	/* in service */

/* L2 history 'action' definitions (Transmission Control) */
#define TXC_ACT_START							0x0001	/* start */
#define TXC_ACT_T6_EXPIRED						0x0002	/* timer T6 expired */
#define TXC_ACT_T7_EXPIRED						0x0004	/* timer T7 expired */
#define TXC_ACT_TX_REQ							0x0008	/* transmission request */
#define TXC_ACT_CLEAR_TB						0x0010	/* clear TB */
#define TXC_ACT_CLEAR_RTB						0x0020	/* clear RTB */
#define TXC_ACT_SEND_FISU						0x0040	/* send FISU */
#define TXC_ACT_SEND_MSU						0x0080	/* send MSU */
#define TXC_ACT_MSG_FOR_TX						0x0100 	/* message for transmission */
#define TXC_ACT_RET_REQ_AND_FSNC				0x0200	/* retrieval request and FSNC */
#define TXC_ACT_SEND_LSSU						0x0400 	/* send LSSU (includes 'send SIB', 'send SIOS,SIPO' and */
																	/* 'send SIO, SIN, SIE') */
#define TXC_ACT_FSNX_VALUE						0x0800   /* FSNX value */
#define TXC_ACT_NACK_TO_BE_TX					0x1000 	/* NACK to be sent */
#define TXC_ACT_BSNR_AND_BIBR					0x2000 	/* BSNR and BIBR */
#define TXC_ACT_SIB_RX							0x4000	/* SIB received */
#define TXC_ACT_FLUSH_BFRS						0x8000	/* flush buffers (ITU) */

/* L2 history 'status_before_action' definitions (Transmission Control) */
#define TXC_STAT_IDLE							0x0001	/* idle */
#define TXC_STAT_IN_SERVICE					0x0002	/* in service */
#define TXC_STAT_RTB_FULL						0x0004	/* RTB full */
#define TXC_STAT_LSSU_AVAIL					0x0008	/* LSSU available */
#define TXC_STAT_MSU_INHIB						0x0010	/* MSU inhibited */
#define TXC_STAT_CLEAR_RTB						0x0020	/* clear RTB */
#define TXC_STAT_SIB_RX							0x0040	/* SIB received */
#define TXC_STAT_PCR_FORCED_RE_TX			0x0080	/* forced retransmission (PCR) */
#define TXC_STAT_T6_RUNNING					0x0100	/* timer T6 running */
#define TXC_STAT_T7_RUNNING					0x0200	/* timer T7 running */

/* L2 history 'action' definitions (Reception Control) */
#define RXC_ACT_START							0x0001	/* start */
#define RXC_ACT_RETRIEVE_BSNT					0x0002	/* retrieve BSNT */
#define RXC_ACT_FSNT_VALUE						0x0004 	/* FSNT value */
#define RXC_ACT_SIGNAL_UNIT					0x0008	/* signal unit */
#define RXC_ACT_REJ_MSU_FISU					0x0010	/* reject MSU/FISU */
#define RXC_ACT_ACCEPT_MSU_FISU				0x0020	/* accept MSU/FISU */
#define RXC_ACT_ALIGN_FSNX						0x0040	/* align FSNX / retrieve FSNX (ITU) */
#define RXC_ACT_CLEAR_RB						0x0080	/* clear RB */
#define RXC_ACT_STOP								0x0100	/* stop */
#define RXC_ACT_CONGESTION_DISC				0x0200	/* congestion discard */
#define RXC_ACT_CONGESTION_ACC				0x0400	/* congestion accept */
#define RXC_ACT_NO_CONGESTION					0x0800	/* no congestion */
#define RXC_ACT_SIGNAL_UNIT_LSSU				0x1000	/* signal unit (LSSU) */

/* L2 history 'status_before_action' definitions (Reception Control) */
#define RXC_STAT_IDLE							0x0001	/* idle */
#define RXC_STAT_IN_SERVICE					0x0002	/* in service */
#define RXC_STAT_MSU_FISU_ACCEPTED 			0x0004	/* MSU/FISU accepted */
#define RXC_STAT_ABNORMAL_BSNR				0x0008	/* abnormal BSNR */
#define RXC_STAT_ABNORMAL_FIBR				0x0010	/* abnormal FIBR */
#define RXC_STAT_CONGESTION_DISC				0x0020	/* congestion discard */
#define RXC_STAT_CONGESTION_ACC				0x0040	/* congestion accept */

/* L2 history 'action' definitions (Alignment Error Rate Monitor) */
#define AERM_ACT_SET_Ti_TO_Tin				0x0001	/* set Ti to Tin */
#define AERM_ACT_START							0x0002	/* start */
#define AERM_ACT_SET_Ti_TO_Tie				0x0004	/* set Ti to Tie */
#define AERM_ACT_STOP							0x0008	/* stop */
#define AERM_ACT_SU_IN_ERROR					0x0010	/* SU in error */

/* L2 history 'status_before_action' definitions (Alignment Error Rate Monitor) */
#define AERM_STAT_IDLE							0x0001	/* idle */
#define AERM_STAT_MONITORING					0x0002	/* monitoring */

/* L2 history 'action' definitions (Signal Unit Error Rate Monitor) */
#define SUERM_ACT_START							0x0001	/* start */
#define SUERM_ACT_STOP							0x0002	/* stop */
#define SUERM_ACT_SU_IN_ERROR					0x0004	/* SU in error */
#define SUERM_ACT_CORRECT_SU					0x0008	/* correct SU */

/* L2 history 'status_before_action' definitions (Signal Unit Error Rate Monitor) */
#define SUERM_STAT_IDLE							0x0001	/* idle */
#define SUERM_STAT_IN_SERVICE					0x0002	/* in service */

/* L2 history 'action' definitions (Congestion Control) */
#define CC_ACT_BUSY								0x0001	/* busy */
#define CC_ACT_NORMAL							0x0002	/* normal */
#define CC_ACT_STOP								0x0004	/* stop */
#define CC_ACT_T5_EXPIRED						0x0008	/* timer T5 expired */

/* L2 history 'status_before_action' definitions (Congestion Control) */
#define CC_STAT_IDLE								0x0001	/* idle */
#define CC_STAT_LVL_2							0x0002	/* level 2 congestion */
#define CC_STAT_T5_RUNNING						0x0004	/* timer T5 running */

/* L2 history 'action' definitions (Errored Interval Monitor) */
#define EIM_ACT_START							0x0001	/* start */
#define EIM_ACT_STOP								0x0002	/* stop */
#define EIM_ACT_T8_EXPIRED						0x0004	/* timer T8 expired */
#define EIM_ACT_CORRECT_SU						0x0008	/* correct SU */
#define EIM_ACT_SU_IN_ERROR					0x0010	/* SU in error */

/* L2 history 'status_before_action' definitions (Errored Interval Monitor) */
#define EIM_STAT_IDLE							0x0001	/* idle */
#define EIM_STAT_MONITORING					0x0002	/* monitoring */
#define EIM_STAT_INTERVAL_ERROR				0x0004	/* interval error */
#define EIM_STAT_SU_RECEIVED					0x0008	/* SU received */
#define EIM_STAT_T8_RUNNING					0x0010	/* timer T8 running */

/* L2 history 'LSSU_SF' definitions (LSSU Status Field) */
#define LSSU_SIO									0x00	/* status indication "O" (out of alignment) */
#define LSSU_SIN									0x01	/* status indication "N" (normal alignment) */
#define LSSU_SIE									0x02	/* status indication "E" (emergency alignment) */
#define LSSU_SIOS									0x03	/* status indication "OS" (out of service) */
#define LSSU_SIPO									0x04	/* status indication "PO" (processor outage) */
#define LSSU_SIB									0x05	/* status indication "B" (busy) */
#define NO_LSSU									0xFF	/* no LSSU transmitted or received */



/* ----------------------------------------------------------------------------
 *               Constants for the L2_RETRIEVE_BSNT command
 * --------------------------------------------------------------------------*/

/* the structure used for retrieving the BSNT value */
typedef struct {
	unsigned short BSNT;									/* the retrieved BSNT value */
} L2_RETRIEVE_BSNT_STRUCT;



/* ----------------------------------------------------------------------------
 *            Constants for the L2_RETRIEVAL_REQ_AND_FSNC command
 * --------------------------------------------------------------------------*/

/* the structure used for handling the L2_RETRIEVAL_REQ_AND_FSNC command and the L2_EXCEP_RETRIEVAL_COMPLETE */
/* exception condition */
typedef struct {
	unsigned short FSNC;									/* the FSNC value to be used in buffer retrieval */
	unsigned short number_MSUs;						/* number of MSU buffers to be retrieved */
	unsigned long ptr_first_MSU_bfr;					/* pointer to the first MSU buffer to be retrieved */
} L2_RETRIEVAL_STRUCT;



/* ----------------------------------------------------------------------------
 *               Constants for the L2_SET_TX_CONG_CFG command
 * --------------------------------------------------------------------------*/

typedef struct {
	unsigned short Tx_cong_config;					/* transmission congestion configuration */
	unsigned short Tx_cong_onset_1;					/* transmit congestion onset threshold #1 */
	unsigned short Tx_cong_abatement_1;				/* transmit congestion abatement threshold #1 */
	unsigned short Tx_cong_discard_1;				/* transmit congestion discard threshold #1 */
	unsigned short Tx_cong_onset_2;					/* transmit congestion onset threshold #2 */
	unsigned short Tx_cong_abatement_2;				/* transmit congestion abatement threshold #2 */
	unsigned short Tx_cong_discard_2;				/* transmit congestion discard threshold #2 */
	unsigned short Tx_cong_onset_3;					/* transmit congestion onset threshold #3 */
	unsigned short Tx_cong_abatement_3;				/* transmit congestion abatement threshold #3 */
	unsigned short Tx_cong_discard_3;				/* transmit congestion discard threshold #3 */
} L2_SET_TX_CONG_CFG_STRUCT;

/* 'Tx_cong_config' settings */
#define NO_TX_NEW_MSUs_SIB_RX				0x0001	/* no new MSUs are transmitted on SIB reception */
#define L2_EXCEP_TX_CONG_THRESHOLDS		0x0010	/* cause an L2 exception condition on congestion thresholds */
#define L2_EXCEP_TX_CONG_SIB_RX			0x0020	/* cause an L2 exception condition on reception of a SIB LSSU */



/* ----------------------------------------------------------------------------
 *               Constants for the L2_READ_TX_CONG_STATUS command
 * --------------------------------------------------------------------------*/

typedef struct {
	unsigned short no_MSU_Tx_bfrs_configured;		/* the number of MSU transmit buffers configured */
	unsigned short no_MSU_Tx_bfrs_occupied;		/* the number of MSU transmit buffers occupied */
	unsigned char Tx_cong_status;						/* transmission congestion status */
	unsigned char Tx_discard_status;					/* transmission discard status */
	unsigned char SIB_Rx_status;						/* LSSU SIB reception status */
} L2_READ_TX_CONG_STATUS_STRUCT;




/* ----------------------------------------------------------------------------
 *               Constants for the L2_SET_RX_CONG_CFG command
 * --------------------------------------------------------------------------*/

typedef struct {
	unsigned short Rx_cong_config;					/* reception congestion configuration */
	unsigned short Rx_cong_bfr_count;				/* the number of MSU receive buffers occupied to trigger congestion */
} L2_SET_RX_CONG_CFG_STRUCT;

/* 'Rx_cong_config' bit settings */
#define L2_RX_CONGESTION_ACCEPT				0x0001	/* congestion accept */
#define L2_RX_CONGESTION_DISCARD				0x0002	/* congestion discard */
#define L2_AUTO_RX_FLOW_CTRL					0x0010 	/* automatic receive flow control */



/* ----------------------------------------------------------------------------
 *               Constants for the L2_READ_RX_CONG_STATUS command
 * --------------------------------------------------------------------------*/

typedef struct {
	unsigned short no_MSU_Rx_bfrs_configured;		/* the number of MSU receive buffers configured */
	unsigned short no_MSU_Rx_bfrs_occupied;		/* the number of MSU receive buffers occupied */
	unsigned char Rx_cong_status;						/* reception congestion status */
} L2_READ_RX_CONG_STATUS_STRUCT;



/* ----------------------------------------------------------------------------
 *                 Constants for using application interrupts
 * --------------------------------------------------------------------------*/

/* the structure used for the L2_SET_INTERRUPT_TRIGGERS/L2_READ_INTERRUPT_TRIGGERS command */
typedef struct {
	unsigned char L2_interrupt_triggers;			/* SS7 L2 interrupt trigger configuration */
	unsigned char IRQ;									/* IRQ to be used */
	unsigned short interrupt_timer;					/* interrupt timer */
	unsigned short misc_interrupt_bits;				/* miscellaneous interrupt bits */
} L2_INT_TRIGGERS_STRUCT;

/* 'L2_interrupt_triggers' bit settings */
#define APP_INT_ON_RX_FRAME					0x01	/* interrupt on MSU reception */
#define APP_INT_ON_TX_FRAME					0x02	/* interrupt when an MSU may be transmitted */
#define APP_INT_ON_COMMAND_COMPLETE			0x04	/* interrupt when an interface command is complete */
#define APP_INT_ON_TIMER						0x08	/* interrupt on a defined millisecond timeout */
#define APP_INT_ON_GLOBAL_EXCEP_COND 		0x10	/* interrupt on a global exception condition */
#define APP_INT_ON_L2_EXCEP_COND				0x20	/* interrupt on an SS7 L2 exception condition */
#define APP_INT_ON_TRACE_DATA_AVAIL			0x80	/* interrupt when trace data is available */

/* 'interrupt_timer' limitation */
#define MAX_INTERRUPT_TIMER_VALUE 			60000		/* the maximum permitted timer interrupt value */

/* interrupt types indicated at 'interrupt_type' byte of the INTERRUPT_INFORMATION_STRUCT */
#define NO_APP_INTS_PEND						0x00	/* no interrups are pending */
#define RX_APP_INT_PEND							0x01	/* a receive interrupt is pending */
#define TX_APP_INT_PEND							0x02	/* a transmit interrupt is pending */
#define COMMAND_COMPLETE_APP_INT_PEND		0x04	/* a 'command complete' interrupt is pending */
#define TIMER_APP_INT_PEND						0x08	/* a timer interrupt is pending */
#define GLOBAL_EXCEP_COND_APP_INT_PEND 	0x10	/* a global exception condition interrupt is pending */
#define L2_EXCEP_COND_APP_INT_PEND			0x20	/* an SS7 L2 exception condition interrupt is pending */
#define TRACE_DATA_AVAIL_APP_INT_PEND		0x80	/* a trace data available interrupt is pending */



/* ----------------------------------------------------------------------------
 *                      Constants for MSU transmission
 * --------------------------------------------------------------------------*/

/* the MSU transmit status element configuration structure */
typedef struct {
	unsigned short number_Tx_status_elements;		/* number of transmit status elements */
	unsigned long base_addr_Tx_status_elements;	/* base address of the transmit element list */
	unsigned long next_Tx_status_element_to_use;	/* pointer to the next transmit element to be used */
} L2_TX_STATUS_EL_CFG_STRUCT;

/* the MSU transmit status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short frame_length;						/* length of the frame to be transmitted */
	unsigned char SIO;									/* Service Information Octet */
	unsigned char misc_Tx_bits;						/* miscellaneous bits */
	unsigned char reserved_1;							/* reserved for internal use */
	unsigned short reserved_2;							/* reserved for internal use */
	unsigned long reserved_3;							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} L2_MSU_TX_STATUS_EL_STRUCT;



/* ----------------------------------------------------------------------------
 *                      Constants for MSU reception
 * --------------------------------------------------------------------------*/

/* the MSU receive status element configuration structure */
typedef struct {
	unsigned short number_Rx_status_elements;		/* number of receive status elements */
	unsigned long base_addr_Rx_status_elements;	/* base address of the receive element list */
	unsigned long next_Rx_status_element_to_use;	/* pointer to the next receive element to be used */
} L2_RX_STATUS_EL_CFG_STRUCT;

/* the MSU receive status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short frame_length;						/* length of the received frame */
	unsigned char SIO;									/* Service Information Octet */
	unsigned char misc_Rx_bits;						/* miscellaneous bits */
	unsigned short time_stamp;							/* receive time stamp */
	unsigned char reserved_1;							/* reserved for internal use */
	unsigned long reserved_2;							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} L2_MSU_RX_STATUS_EL_STRUCT;



/* ----------------------------------------------------------------------------
 *         Constants defining the shared memory information area
 * --------------------------------------------------------------------------*/

/* the global information structure */
typedef struct {
 	unsigned char global_status;						/* global status */
 	unsigned char modem_status;						/* current modem status */
 	unsigned char global_excep_conditions;			/* global exception conditions */
	unsigned char Rx_disabled_Rx_bfrs_full;		/* receiver disabled */
	unsigned char glob_info_reserved[4];			/* reserved */
	unsigned char code_name[4];						/* code name */
	unsigned char code_version[4];					/* code version */
} GLOBAL_INFORMATION_STRUCT;

/* the SS7 L2 information structure */
typedef struct {
	unsigned char L2_in_service;						/* SS7 L2 'in service' flag */
 	unsigned short L2_excep_conditions;				/* SS7 L2 exception conditions */
	unsigned short no_MSU_Tx_bfrs_occupied;		/* the number of MSU transmit buffers occupied */
	unsigned char Tx_cong_status;						/* transmission congestion status */
	unsigned char Tx_discard_status;					/* transmission discard status */
	unsigned char SIB_Rx_status;						/* LSSU SIB reception status */
	unsigned char L2_info_reserved[8];				/* reserved */
} L2_INFORMATION_STRUCT;

typedef struct {
	unsigned char reserved[16];						/* reserved */
} RES_INFORMATION_STRUCT;

/* the interrupt information structure */
typedef struct {
 	unsigned char interrupt_type;						/* type of interrupt triggered */
 	unsigned char interrupt_permission;				/* interrupt permission mask */
	unsigned char int_info_reserved[14];			/* reserved */
} INTERRUPT_INFORMATION_STRUCT;

/* the FT1 information structure */
typedef struct {
 	unsigned char parallel_port_A_input;			/* input - parallel port A */
 	unsigned char parallel_port_B_input;			/* input - parallel port B */
	unsigned char FT1_INS_alarm_condition;			/* the current FT1 in-service/alarm condition */
	unsigned char FT1_info_reserved[13];			/* reserved */
} FT1_INFORMATION_STRUCT;

/* the shared memory area information structure */
typedef struct {
	GLOBAL_INFORMATION_STRUCT global_info_struct;/* the global information structure */
	L2_INFORMATION_STRUCT L2_info_struct;			/* the SS7 L2 information structure */
	RES_INFORMATION_STRUCT res0_info_struct;		/* reserved information structure */
	RES_INFORMATION_STRUCT res1_info_struct;		/* reserved information structure */
	INTERRUPT_INFORMATION_STRUCT interrupt_info_struct;/* the interrupt information structure */
	FT1_INFORMATION_STRUCT FT1_info_struct;		/* the FT1 information structure */
} SHARED_MEMORY_INFO_STRUCT;



/* ----------------------------------------------------------------------------
 *                      Constants for SS7 L2 debugging
 * --------------------------------------------------------------------------*/
/* the SS7 debug structure */
typedef struct {
	unsigned short LSC_action;
	unsigned short LSC_status;
	unsigned short IAC_action;
	unsigned short IAC_status;
	unsigned short POC_action;
	unsigned short POC_status;
	unsigned short DAEDR_action;
	unsigned short DAEDR_status;
	unsigned short DAEDT_action;
	unsigned short DAEDT_status;
	unsigned short TXC_action;
	unsigned short TXC_status;
	unsigned char Tx_LSSU_SF;
	unsigned short RXC_action;
	unsigned short RXC_status;
	unsigned short CC_action;
	unsigned short CC_status;
	unsigned short AERM_action;
	unsigned short AERM_status;
	unsigned short SUERM_action;
	unsigned short SUERM_status;
	unsigned short EIM_action;
	unsigned short EIM_status;
	unsigned short FSNL;
	unsigned short FSNF;
	unsigned short FSNT_TXC;
	unsigned short FSNT_RXC;
	unsigned short FSNX_TXC;
	unsigned short FSNX_RXC;
	unsigned short FSNR;
	unsigned short BSNR;
	unsigned short BSNT;
	unsigned char FIBT;
	unsigned char FIBR;
	unsigned char FIBX;
	unsigned char FIB;
	unsigned char BIBT;
	unsigned char BIBR;
	unsigned char BIBX;
	unsigned char BIB;
	unsigned short Cm;
	unsigned char UNF;
	unsigned char UNB;
	unsigned char RTR;
	unsigned char Cp;
	unsigned char Tx_frm_status;
} L2_DEBUG_STRUCT;

#pragma pack()


#undef  wan_udphdr_data
#define wan_udphdr_data	wan_udphdr_u.ss7.data

#ifdef __KERNEL__
#undef wan_udp_data
#define wan_udp_data wan_udp_hdr.wan_udphdr_u.ss7.data
#endif

#endif
