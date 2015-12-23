/************************************************************************************
 *                                                                                  *
 * PHYAPI.H - the 'C' header file for the Sangoma S508/S514 PHY-level ATM code API. *
 *                                                                                  *
 ************************************************************************************
*/

#ifndef __SDLA_ATM_H_
#define __SDLA_ATM_H_


#pragma pack(1)

/* ----------------------------------------------------------------------------
 *        Constants defining the shared memory control block (mailbox)
 * --------------------------------------------------------------------------*/

#define BASE_ADDR_MB_STRUCT 		 		0xE000 	/* the base address of the mailbox structure */
#define NUMBER_MB_RESERVED_BYTES			0x0B		/* the number of reserved bytes in the mailbox header area */
#define SIZEOF_MB_DATA_BFR					240		/* the size of the actual mailbox data area */

/* the control block mailbox structure */
typedef struct {
	unsigned char opp_flag;								/* the opp flag */
	unsigned char command;								/* the user command */
	unsigned short buffer_length;						/* the data length */
  	unsigned char return_code;							/* the return code */
	char MB_reserved[NUMBER_MB_RESERVED_BYTES];	/* reserved for later use */
	char data[SIZEOF_MB_DATA_BFR];					/* the data area */
} ATM_MAILBOX_STRUCT;



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
#undef FLUSH_TRACE_STATISTICS
#define FLUSH_TRACE_STATISTICS				0x0D	/* flush the trace statistics */

/* PHY-level interface commands */
#undef 	PHY_READ_CODE_VERSION
#define PHY_READ_CODE_VERSION					0x20	/* read the ATM code version */
#define PHY_READ_EXCEPTION_CONDITION		0x21	/* read a PHY-level exception condition from the adapter */
#define PHY_SET_CONFIGURATION					0x22	/* set the PHY-level configuration */
#define PHY_READ_CONFIGURATION				0x23	/* read the PHY-level configuration */
#define PHY_ENABLE_COMMUNICATIONS			0x24	/* enable PHY-level communications */
#define PHY_DISABLE_COMMUNICATIONS			0x25	/* disable PHY-level communications */
#define PHY_READ_STATUS							0x26	/* read the PHY-level status */
#define PHY_READ_OPERATIONAL_STATS			0x27	/* retrieve the PHY-level operational statistics */
#define PHY_FLUSH_OPERATIONAL_STATS			0x28	/* flush the PHY-level operational statistics */
#define PHY_RESYNCHRONIZE_RECEIVER			0x29	/* resynchronize the receiver */
#define PHY_SET_TX_UNDERRUN_CONFIG 			0x2A	/* set the transmit underrun cell configuration */
#define PHY_READ_TX_UNDERRUN_CONFIG 		0x2B	/* read the transmit underrun cell configuration */
#define PHY_SET_INTERRUPT_TRIGGERS			0x30	/* set the PHY-level application interrupt triggers */
#define PHY_READ_INTERRUPT_TRIGGERS			0x31	/* read the PHY-level application interrupt trigger configuration */



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

/* return codes from command READ_GLOBAL_EXCEPTION_CONDITION */
#define EXCEP_MODEM_STATUS_CHANGE			0x10		/* a modem status change occurred */
#define EXCEP_APP_IRQ_TIMEOUT					0x12		/* an application IRQ timeout has occurred */

/* return codes from PHY-level interface commands */
#define PHY_NO_EXCEP_COND_TO_REPORT			0x21	/* there is no PHY-level exception condition to report */
#define PHY_COMMS_DISABLED						0x21	/* communications are not currently enabled */
#define PHY_COMMS_ENABLED						0x21	/* communications are currently enabled */
#define PHY_DISABLE_COMMS_BEFORE_CFG		0x21	/* communications must be disabled before setting the configuration */
#define PHY_CFG_BEFORE_COMMS_ENABLED		0x22	/* perform a PHY_SET_CONFIGURATION before enabling comms */
#define PHY_LGTH_CFG_DATA_INVALID			0x22	/* the length of the passed configuration data is invalid */
#define PHY_LGTH_INT_TRIG_DATA_INVALID		0x22	/* the length of the passed interrupt trigger data is invalid */
#define PHY_LGTH_TX_UND_DATA_INVALID		0x22	/* the length of the passed transmit underrun data is invalid */
#define PHY_RX_NOT_SYNCHRONIZED				0x22	/* the receiver is not synchronized */
#define PHY_INVALID_IRQ_SELECTED				0x23	/* an invalid IRQ was selected in the PHY_SET_INTERRUPT_TRIGGERS */
#define PHY_INVALID_CFG_DATA					0x23	/* the passed configuration data is invalid */
#define PHY_IRQ_TMR_VALUE_INVALID			0x24	/* an invalid application IRQ timer value was selected */
#define PHY_INVALID_COMMAND					0x2F	/* the defined interface command is invalid */

/* return codes from command PHY_READ_EXCEPTION_CONDITION */
#define PHY_EXCEP_RX_SYNC_STATE_CHANGE		0x30	/* the PHY receiver has changed state */
#define PHY_EXCEP_INVALID_HEC					0x32	/* the Rx consecutive incorrect HEC counter has expired */
#define PHY_EXCEP_RECEP_LOSS					0x33	/* the cell reception sync loss timer has expired */
#define PHY_EXCEP_RX_DISCARD					0x36	/* incoming cells were discarded */
#define PHY_EXCEP_TX_LENGTH_ERROR			0x37	/* a transmit buffer of invalid length was detected */



/* ----------------------------------------------------------------------------
 *          Constants for the READ_GLOBAL_EXCEPTION_CONDITION command
 * --------------------------------------------------------------------------*/

/* the global exception condition structure for handling a modem status change */
typedef struct {
	unsigned char modem_status_change;				/* the modem status change */
} GLOBAL_EX_MODEM_STRUCT;

/* settings for the 'modem_status_change' */
#define CHANGE_IN_DCD							0x04	/* a change in DCD occured */
#define CHANGE_IN_CTS							0x10	/* a change in CTS occured */



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
 *             Constants for the SET_MODEM_STATUS command
 * --------------------------------------------------------------------------*/

/* the set modem status structure */
typedef struct {
	unsigned char output_modem_status;        	/* the output modem status */
} SET_MODEM_STATUS_STRUCT;

/* settings for the 'output_modem_status' */
#define SET_DTR_HIGH								0x01	/* set DTR high */
#define SET_RTS_HIGH								0x02	/* set RTS high */



/* ----------------------------------------------------------------------------
 *             Constants for the READ_MODEM_STATUS command
 * --------------------------------------------------------------------------*/

/* the read modem status structure */
typedef struct {
	unsigned char input_modem_status;        		/* the input modem status */
} READ_MODEM_STATUS_STRUCT;

/* settings for the 'input_modem_status' */
#define DCD_HIGH									0x08	/* DCD is high */
#define CTS_HIGH									0x20	/* CTS is high */



/* ----------------------------------------------------------------------------
 *             Constants for the READ_COMMS_ERROR_STATS command
 * --------------------------------------------------------------------------*/

/* the communications error statistics structure */
typedef struct {
	unsigned short Rx_overrun_err_count; 			/* receiver overrun error count */
	unsigned short reserved_0;							/* reserved for later use */
	unsigned short reserved_1;							/* reserved for later use */
	unsigned short DCD_state_change_count;			/* DCD state change count */
	unsigned short CTS_state_change_count;			/* CTS state change count */
} COMMS_ERROR_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *                  Constants used for line tracing
 * --------------------------------------------------------------------------*/

/* the trace configuration structure (SET_TRACE_CONFIGURATION/READ_TRACE_CONFIGURATION commands) */
typedef struct {
	unsigned char trace_config;						/* trace configuration */
	unsigned long ptr_trace_stat_el_cfg_struct;	/* a pointer to the line trace element configuration structure */
} LINE_TRACE_CONFIG_STRUCT;

/* 'trace_config' bit settings */
#define TRACE_INACTIVE							0x00	/* trace is inactive */
#define TRACE_ACTIVE								0x01	/* trace is active */
#define TRACE_LIMIT_REPEAT_CELLS				0x08	/* limit the tracing of repeated Physical Layer and Idle Cells */
#define TRACE_PHY_UNASSIGNED_CELLS			0x10	/* trace Unassigned Cells */
#define TRACE_PHY_IDLE_CELLS					0x20	/* trace Idle Cells */
#define TRACE_PHY_PHYS_LAYER_CELLS			0x40	/* trace Physical Layer Cells */
#define TRACE_PHY_NON_UNAS_PHYS_CELLS		0x80	/* trace cells other than Physical Layer and Idle Cells */

/* the line trace status element configuration structure */
typedef struct {
	unsigned short number_trace_status_els;		/* number of line trace elements */
	unsigned long base_addr_trace_status_els;		/* base address of the trace element list */
	unsigned long next_trace_el_to_use;				/* pointer to the next trace element to be used */
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

/* settings for the 'trace_type' */
#define TRACE_INCOMING							0x00	/* an incoming block/cell has been traced */
#define TRACE_OUTGOING							0x01	/* an outgoing block/cell has been traced */
#define TRACE_HEC_ERROR							0x80	/* the traced cell has a HEC error */

/* the line trace statistics structure */
typedef struct {
	unsigned long blocks_traced_count;				/* number of blocks traced */
	unsigned long trc_blocks_discarded_count;		/* number of trace blocks discarded */
} LINE_TRACE_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *          Constants for the PHY_READ_EXCEPTION_CONDITION command
 * --------------------------------------------------------------------------*/

/* the structure returned on a return code PHY_EXCEP_RX_SYNC_STATE_CHANGE */
/* Note that the definitions for the 'Rx_sync_status' are as per the PHY_READ_STATUS command */
typedef struct {
	unsigned char Rx_sync_status;						/* receiver synchronization status */
} PHY_RX_SYNC_EXCEP_STRUCT;

/* the structure returned on a return code PHY_EXCEP_RX_DISCARD */
typedef struct {
	unsigned long Rx_discard_count;					/* number of incoming blocks discarded */
} PHY_RX_DISC_EXCEP_STRUCT;



/* ----------------------------------------------------------------------------
 *  Constants for the PHY_SET_CONFIGURATION/PHY_READ_CONFIGURATION command
 * --------------------------------------------------------------------------*/

/* the PHY-level configuration structure */
typedef struct {
	unsigned long baud_rate;							/* the baud rate */	
	unsigned short line_config_options;				/* line configuration options */
	unsigned short modem_config_options;			/* modem configuration options */
	unsigned short modem_status_timer;				/* timer for monitoring modem status changes */
	unsigned short API_options;						/* API options */
	unsigned short protocol_options;					/* protocol options */
	unsigned short HEC_options;						/* HEC options */
	unsigned char custom_Rx_COSET;					/* the custom COSET value used when checking the HEC value in received cells */
	unsigned char custom_Tx_COSET;					/* the custom COSET value used when setting the HEC value in transmitted cells */
	unsigned short buffer_options;					/* Tx/Rx buffer options */
	unsigned short max_cells_in_Tx_block;			/* the maximum number of cells in an outgoing block */
	unsigned char Tx_underrun_cell_GFC;				/* the GFC value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_PT;				/* the PT value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_CLP;				/* the CLP value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_payload;		/* the payload character in a Tx underrun cell */
	unsigned short number_cells_Tx_underrun;		/* the number of cells to be transmitted in an underrun condition */
	unsigned short max_cells_in_Rx_block;			/* the maximum number of cells in an incoming block */
	unsigned short Rx_hunt_timer;						/* receiver hunt timer */
	unsigned short Rx_sync_bytes;						/* receiver synchronization bytes */
	unsigned short Rx_sync_offset;					/* offset of the receiver synchronization bytes */
	unsigned short cell_Rx_sync_loss_timer;		/* cell reception synchronization loss timer */
	unsigned short Rx_HEC_check_timer;				/* the Rx HEC check timer */
	unsigned short Rx_bad_HEC_timer;					/* the time period for monitoring cells with bad HEC values */
	unsigned short Rx_max_bad_HEC_count;			/* the maximum number of bad HEC count */
	unsigned short number_cells_Rx_discard;		/* the number of cells to be discarded in a buffer overrun condition */
	unsigned short statistics_options;				/* operational statistics options */
	unsigned long ptr_shared_mem_info_struct;		/* a pointer to the shared memory area information structure */
	unsigned long ptr_Tx_stat_el_cfg_struct;		/* a pointer to the transmit status element configuration structure */
	unsigned long ptr_Rx_stat_el_cfg_struct;		/* a pointer to the receive status element configuration structure */
} PHY_CONFIGURATION_STRUCT;

/* settings for the 'line_config_options' */
#define PHY_INTERFACE_LEVEL_V35				0x0001 /* V.35 interface level */
#define PHY_INTERFACE_LEVEL_RS232			0x0002 /* RS-232 interface level */

/* settings for the 'modem_config_options' */
#define PHY_MANUAL_DTR_RTS						0x0001 /* don't automatically raise DTR and RTS when performing an */
																 /* PHY_ENABLE_COMMUNICATIONS command */
#define PHY_IGNORE_CHG_MODEM_STATUS			0x0002 /* don't report changes in modem status to the application */

/* settings for the 'API_options' */
#define PHY_DISCARD_RX_UNASSIGNED_CELLS	0x0001 /* discard incoming Unassigned Cells */
#define PHY_DISCARD_RX_IDLE_CELLS			0x0002 /* discard incoming Idle Cells */
#define PHY_DISCARD_RX_PHYS_LAYER_CELLS	0x0004 /* discard incoming Physical Layer Cells */
#define PHY_TRANSPARENT_TX_RX_CELLS			0x0010 /* cells passed to and from the application transparently */
#define PHY_DECODED_TX_RX_CELLS				0x0020 /* cells decoded and formatted by the PHY firmware */
#define PHY_APP_REVERSES_BIT_ORDER			0x0040 /* let application revers bits */

/* settings for the 'protocol_options' */
#define PHY_UNI									0x0001 /* configure as a User-Network Interface */	
#define PHY_NNI									0x0002 /* configure as a Network Node Interface */
#define PHY_MANUAL_RX_SYNC						0x0100 /* use user-defined Rx synchronization parameters */

/* settings for the 'HEC_options' */
#define PHY_DISABLE_RX_HEC_CHECK				0x0001 /* disable the checking of the HEC value in received cells */
#define PHY_DISABLE_AUTO_SYNC_BAD_HEC		0x0002 /* disable automatic resynchronization on receipt of cells with bad HEC values */
#define PHY_DISABLE_RX_COSET					0x0010 /* disable XOR with COSET when checking the HEC value in received cells */
#define PHY_DISABLE_TX_COSET					0x0020 /* disable XOR with COSET when setting the HEC value in transmitted cells */
#define PHY_CUSTOM_RX_COSET					0x0040 /* use a custom COSET when checking the HEC value in received cells */
#define PHY_CUSTOM_TX_COSET					0x0080 /* use a custom COSET when setting the HEC value in transmitted cells */

/* bit settings for the 'buffer_options' */
#define PHY_TX_ONLY								0x0001 /* transmit only (no reception) */
#define PHY_RX_ONLY								0x0002 /* receive only (no transmission) */
#define PHY_SINGLE_TX_BUFFER					0x0010 /* configure a single transmit buffer */

/* settings for the 'statistics_options' */
#define PHY_TX_BYTE_COUNT_STAT				0x0001 /* record the number of bytes transmitted */
#define PHY_RX_BYTE_COUNT_STAT				0x0002 /* record the number of bytes received */
#define PHY_TX_THROUGHPUT_STAT				0x0004 /* compute the transmit throughput */
#define PHY_RX_THROUGHPUT_STAT				0x0008 /* compute the receive throughput */
#define PHY_INCL_UNDERRUN_TX_THRUPUT		0x0010 /* include Tx underrun cells in Tx throughput */
#define PHY_INCL_DISC_RX_THRUPUT				0x0020 /* include discarded (idle/unassigned) Rx cells in Rx throughput */

/* permitted minimum and maximum values for setting the PHY configuration */
#define PHY_MAX_BAUD_RATE_S508				2666666 /* maximum baud rate (S508) */
#define PHY_MAX_BAUD_RATE_S514				2750000 /* maximum baud rate (S514) */
#define PHY_MIN_MODEM_TIMER					0		  /* minimum modem status timer */
#define PHY_MAX_MODEM_TIMER					6000	  /* maximum modem status timer */
#define PHY_MIN_RX_HUNT_TIMER					1		  /* minimum receiver hunt timer */
#define PHY_MAX_RX_HUNT_TIMER					6000	  /* maximum receiver hunt timer */
#define PHY_MIN_RX_SYNC_OFFSET				0		  /* minimum offset of receiver synchronization bytes */
#define PHY_MAX_RX_SYNC_OFFSET				50		  /* maximum offset of receiver synchronization bytes */
#define PHY_MIN_CELLS_IN_TX_BLOCK			1	 	  /* minimum number of cells in an outgoing block */
#define PHY_MAX_CELLS_IN_TX_BLOCK			96	 	  /* maximum number of cells in an outgoing block */
#define PHY_MIN_CELLS_TX_UNDERRUN			1		  /* minimum number of cells to be transmitted in an underrun condition */
#define PHY_MAX_CELLS_TX_UNDERRUN			5		  /* minimum number of cells to be transmitted in an underrun condition */
#define PHY_MIN_CELLS_IN_RX_BLOCK			1	 	  /* minimum number of cells in an incoming block */
#define PHY_MAX_CELLS_IN_RX_BLOCK			96	 	  /* maximum number of cells in an incoming block */
#define PHY_MAX_RX_SYNC_LOSS_TIMER			6000	  /* maximum cell reception sync loss timer */
#define PHY_MAX_RX_HEC_CHECK_TIMER			6000	  /* maximum receive HEC check timer */
#define PHY_MIN_RX_BAD_HEC_TIMER				5		  /* minimum time for monitoring cells with bad HEC values */
#define PHY_MAX_RX_BAD_HEC_TIMER				60000   /* maximum time for monitoring cells with bad HEC values */
#define PHY_MIN_RX_BAD_HEC_COUNT				1	  	  /* the minimum bad HEC counter */
#define PHY_MAX_RX_BAD_HEC_COUNT				10000	  /* the maximum bad HEC counter */
#define PHY_MAX_RX_INCORRECT_HEC_COUNT		100	  /* maximum receive consecutive incorrect HEC counter */
#define PHY_MIN_CELLS_RX_DISCARD				1		  /* minimum number of cells to be discarded in a buffer overrun condition */
#define PHY_MAX_CELLS_RX_DISCARD				5		  /* maximum number of cells to be discarded in a buffer overrun condition */



/* ----------------------------------------------------------------------------
 *             Constants for the PHY_READ_STATUS command
 * --------------------------------------------------------------------------*/

/* the PHY-level status structure */
typedef struct {
	unsigned char Rx_sync_status;						/* receiver synchronization status */
 	unsigned char excep_conditions;					/* PHY exception conditions */
	unsigned char no_Rx_blocks_avail;				/* number of Rx blocks available for the application */
	unsigned short Rx_sync_time;						/* receiver synchronization time */
	unsigned short Rx_sync_bytes;						/* receiver synchronization bytes */
	unsigned short Rx_sync_offset;					/* offset of the receiver synchronization bytes */
} PHY_STATUS_STRUCT;

/* settings for the 'Rx_sync_status' variable */
#define PHY_RX_SYNC_LOST					0x00		/* synchronization has been lost */
#define PHY_RX_HUNT							0x01		/* receiver in hunt state */
#define PHY_RX_PRESYNC						0x02		/* receiver in presync state */
#define PHY_RX_SYNCHRONIZED				0x80		/* receiver synchronized */



/* ----------------------------------------------------------------------------
 *           Constants for the PHY_READ_OPERATIONAL_STATS command
 * --------------------------------------------------------------------------*/

/* the PHY-level operational statistics structure */
typedef struct {

	/* transmission statistics */
	unsigned long blocks_Tx_count;					/* number of blocks transmitted */
	unsigned long bytes_Tx_count; 					/* number of bytes transmitted */
	unsigned long Tx_throughput;						/* transmit throughput */
	unsigned long no_ms_for_Tx_thruput_comp;		/* millisecond time used for the Tx throughput computation */
	unsigned long Tx_underrun_cell_count;			/* number of underrun cells transmitted */
	unsigned long Tx_length_error_count;			/* number of blocks transmitted with a length error */
	unsigned long reserved_Tx_stat0;					/* reserved for later use */
	unsigned long reserved_Tx_stat1;					/* reserved for later use */

	/* reception statistics */
	unsigned long blocks_Rx_count;					/* number of blocks received */
	unsigned long bytes_Rx_count;						/* number of bytes received */
	unsigned long Rx_throughput;						/* receive throughput */
	unsigned long no_ms_for_Rx_thruput_comp;		/* millisecond time used for the Rx throughput computation */
	unsigned long Rx_blocks_discard_count;			/* number of incoming blocks discarded */
	unsigned long Rx_Idle_Cell_discard_count;		/* number of incoming Idle Cells discarded */
	unsigned long Rx_Unas_Cell_discard_count;		/* number of incoming Unassigned Cells discarded */
	unsigned long Rx_Phys_Lyr_Cell_discard_count;/* number of incoming Physical Layer Cells discarded */
	unsigned long Rx_bad_HEC_count;					/* number of incoming cells with a bad HEC */
	unsigned long reserved_Rx_stat0;					/* reserved for later use */
	unsigned long reserved_Rx_stat1;					/* reserved for later use */

	/* synchronization statistics */
	unsigned long Rx_sync_attempt_count;			/* receiver synchronization attempt count */
	unsigned long Rx_sync_achieved_count;			/* receiver synchronization achieved count */
	unsigned long Rx_sync_failure_count;			/* receiver synchronization failure count */
	unsigned long Rx_hunt_attempt_count;			/* Rx hunt attempt count */
	unsigned long Rx_hunt_char_sync_count;			/* Rx hunt character synchronization count */
	unsigned long Rx_hunt_timeout_count;			/* Rx hunt timeout count */
	unsigned long Rx_hunt_achieved_count;			/* Rx hunt achieved count */
	unsigned long Rx_hunt_failure_count;			/* Rx hunt failure count */
	unsigned long Rx_presync_attempt_count;		/* Rx presync attempt count */
	unsigned long Rx_presync_achieved_count;		/* Rx presync achieved count */
	unsigned long Rx_presync_failure_count;		/* Rx presync failure count */
	unsigned long Rx_resync_bad_HEC_count;			/* Rx re-synchronization due to cells received with a bad HEC */
	unsigned long Rx_resync_reception_loss_count;/* Rx re-synchronization due to loss of reception */
	unsigned long Rx_resync_overrun_count;			/* Rx re-synchronization due receiver overrun */
	unsigned long reserved_Rx_sync_stat0;			/* reserved for later use */
	unsigned long reserved_Rx_sync_stat1;			/* reserved for later use */
} PHY_OPERATIONAL_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *            Constants for the PHY_SET_TX_UNDERRUN_CONFIG command
 * --------------------------------------------------------------------------*/

/* the PHY transmit underrun cell structure */
typedef struct {
	unsigned char Tx_underrun_cell_GFC;				/* the GFC value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_PT;				/* the PT value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_CLP;				/* the CLP value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_payload;		/* the payload character in a Tx underrun cell */
} PHY_TX_UNDERRUN_STRUCT;



/* ----------------------------------------------------------------------------
 *                 Constants for using application interrupts
 * --------------------------------------------------------------------------*/

/* the structure used for the PHY_SET_INTERRUPT_TRIGGERS/PHY_READ_INTERRUPT_TRIGGERS command */
typedef struct {
	unsigned char interrupt_triggers;			/* interrupt trigger configuration */
	unsigned char IRQ;								/* IRQ to be used */
	unsigned short interrupt_timer;				/* interrupt timer */
	unsigned short misc_interrupt_bits;			/* miscellaneous interrupt bits */
} PHY_INT_TRIGGERS_STRUCT;

/* 'interrupt_triggers' bit settings */
#define PHY_INT_RX								0x01	/* interrupt on reception */
#define PHY_INT_TX								0x02	/* interrupt on transmission */
#define PHY_INT_COMMAND_COMPLETE				0x04	/* interrupt when an interface command is complete */
#define PHY_INT_TIMER							0x08	/* interrupt on a defined 1/100th second timeout */
#define PHY_INT_GLOBAL_EXCEP_COND 			0x10	/* interrupt on a global exception condition */
#define PHY_INT_PHY_EXCEP_COND				0x20	/* interrupt on a PHY-level exception condition */
#define PHY_INT_TRACE							0x80	/* interrupt when trace data is available */

/* 'interrupt_timer' limitation */
#define MAX_INTERRUPT_TIMER_VALUE 			60000		/* the maximum permitted timer interrupt value */

/* interrupt types indicated at 'interrupt_type' byte of the INTERRUPT_INFORMATION_STRUCT */
#define PHY_NO_INT_PEND							0x00	/* no interrups are pending */
#define PHY_RX_INT_PEND							0x01	/* a receive interrupt is pending */
#define PHY_TX_INT_PEND							0x02	/* a transmit interrupt is pending */
#define PHY_COMMAND_COMPLETE_INT_PEND		0x04	/* a 'command complete' interrupt is pending */
#define PHY_TIMER_INT_PEND						0x08	/* a timer interrupt is pending */
#define PHY_GLOBAL_EXCEP_COND_INT_PEND 	0x10	/* a global exception condition interrupt is pending */
#define PHY_PHY_EXCEP_COND_INT_PEND 		0x20	/* a PHY exception condition interrupt is pending */
#define PHY_TRACE_INT_PEND						0x80	/* a trace data interrupt is pending */



/* ----------------------------------------------------------------------------
 *                   Constants for block transmission
 * --------------------------------------------------------------------------*/

/* the block transmit status element configuration structure */
typedef struct {
	unsigned short number_Tx_status_els;			/* number of transmit status elements */
	unsigned long base_addr_Tx_status_els;			/* base address of the transmit element list */
	unsigned long next_Tx_status_el_to_use;		/* pointer to the next transmit element to be used */
} PHY_TX_STATUS_EL_CFG_STRUCT;

/* the block transmit status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short block_length;						/* length of the block to be transmitted */
	unsigned char misc_Tx_bits;						/* miscellaneous Tx bits */
	unsigned char Tx_underrun_cell_GFC;				/* the GFC value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_PT;				/* the PT value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_CLP;				/* the CLP value in a Tx underrun cell */
	unsigned char Tx_underrun_cell_payload;		/* the payload character in a Tx underrun cell */
	unsigned char reserved[4];							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} PHY_TX_STATUS_EL_STRUCT;

/* settings for the 'misc_Tx_bits' */
#define PHY_UPDATE_TX_UNDERRUN_CONFIG			0x01	/* update the transmit underrun cell configuration */



/* ----------------------------------------------------------------------------
 *                   Constants for block reception
 * --------------------------------------------------------------------------*/

/* the block receive status element configuration structure */
typedef struct {
	unsigned short number_Rx_status_els;			/* number of receive status elements */
	unsigned long base_addr_Rx_status_els;			/* base address of the receive element list */
	unsigned long next_Rx_status_el_to_use;		/* pointer to the next receive element to be used */
} PHY_RX_STATUS_EL_CFG_STRUCT;

/* the block receive status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short block_length;						/* length of the received block */
	unsigned char misc_Rx_bits;						/* miscellaneous Rx bits */
	unsigned short time_stamp;							/* receive time stamp */
	unsigned char reserved[6];							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} PHY_RX_STATUS_EL_STRUCT;

/* settings for the 'misc_Rx_bits' */
#define RX_OVERRUN_ERROR						0x04	/* the incoming block has an overrun error */
#define HEC_ERROR									0x08	/* the received cell has a HEC error */

/* structure used for transmitting and receiving cells that are decoded and formatted by the PHY firmware */
typedef struct {
	unsigned char reserved[5];							/* reserved for internal use */
	unsigned char payload[48];							/* information payload */
	unsigned short GFC_VPI;								/* GFC (Generic Flow Control)/VPI (Virtual Path Identifier) */
	unsigned short VCI;									/* VCI (Virtual Channel Identifier) */
	unsigned char PT;										/* PT (Payload Type) */
	unsigned char CLP;									/* CLP (Cell Loss Priority) */
	unsigned char HEC;									/* HEC (Header Error Control) */
} PHY_TX_RX_DECODE_STRUCT;



/* ----------------------------------------------------------------------------
 *         Constants defining the shared memory information area
 * --------------------------------------------------------------------------*/

/* the global information structure */
typedef struct {
 	unsigned char global_status;						/* global status */
 	unsigned char modem_status;						/* current modem status */
 	unsigned char global_excep_conditions;			/* global exception conditions */
	unsigned char glob_info_reserved[5];			/* reserved */
	unsigned char code_name[4];						/* code name */
	unsigned char code_version[4];					/* code version */
} GLOBAL_INFORMATION_STRUCT;

/* the PHY information structure */
typedef struct {
	unsigned char Rx_sync_status;						/* receiver synchronization status */
 	unsigned char PHY_excep_conditions;				/* PHY exception conditions */
	unsigned char no_Rx_blocks_avail;				/* number of Rx blocks available for the application */
	unsigned char PHY_info_reserved[13];			/* reserved */
} PHY_INFORMATION_STRUCT;

/* the interrupt information structure */
typedef struct {
 	unsigned char interrupt_type;						/* type of interrupt triggered */
 	unsigned char interrupt_permission;				/* interrupt permission mask */
	unsigned char int_info_reserved[14];			/* reserved */
} INTERRUPT_INFORMATION_STRUCT;

/* the front-end information structure */
typedef struct {
 	unsigned char parallel_port_A_input;			/* input - parallel port A */
 	unsigned char parallel_port_B_input;			/* input - parallel port B */
	unsigned char FT1_INS_alarm_condition;			/* the current FT1 in-service/alarm condition */
	unsigned char FT1_info_reserved[13];			/* reserved */
} FE_INFORMATION_STRUCT;

/* the shared memory area information structure */
typedef struct {
	GLOBAL_INFORMATION_STRUCT global_info_struct;		/* the global information structure */
	PHY_INFORMATION_STRUCT PHY_info_struct;				/* the PHY information structure */
	INTERRUPT_INFORMATION_STRUCT interrupt_info_struct;/* the interrupt information structure */
	FE_INFORMATION_STRUCT FE_info_struct;					/* the front-end information structure */
} SHARED_MEMORY_INFO_STRUCT;


#pragma pack()


#undef  wan_udphdr_data
#define wan_udphdr_data	wan_udphdr_u.atm.data

#ifdef __KERNEL__
#undef wan_udp_data
#define wan_udp_data wan_udp_hdr.wan_udphdr_u.atm.data
#endif


//#define SHARED_MEMORY_INFO_STRUCT	void
//#define CONFIGURATION_STRUCT		PHY_CONFIGURATION_STRUCT
//#define INTERRUPT_INFORMATION_STRUCT	void
#define DATA_RX_STATUS_EL_STRUCT	PHY_RX_STATUS_EL_STRUCT
#define DATA_TX_STATUS_EL_STRUCT	PHY_TX_STATUS_EL_STRUCT
#define INT_TRIGGERS_STRUCT		PHY_INT_TRIGGERS_STRUCT
#define TX_STATUS_EL_CFG_STRUCT		PHY_TX_STATUS_EL_CFG_STRUCT
#define RX_STATUS_EL_CFG_STRUCT		PHY_RX_STATUS_EL_CFG_STRUCT
//#define TRACE_STATUS_EL_CFG_STRUCT	void
//#define TRACE_STATUS_ELEMENT_STRUCT	void
//#define LINE_TRACE_CONFIG_STRUCT	void
//#define COMMS_ERROR_STATS_STRUCT	void
#define OPERATIONAL_STATS_STRUCT	PHY_OPERATIONAL_STATS_STRUCT

#define DFLT_TEMPLATE_VALUE		...

#define WANCONFIG_FRMW			WANCONFIG_ATM	

#define COMMAND_OK			OK

#define APP_INT_ON_TIMER		PHY_INT_TIMER
#define APP_INT_ON_TX_FRAME		PHY_INT_TX
#define APP_INT_ON_RX_FRAME		PHY_INT_RX
#define APP_INT_ON_GLOBAL_EXCEP_COND	PHY_INT_GLOBAL_EXCEP_COND
#define APP_INT_ON_EXCEP_COND		PHY_INT_PHY_EXCEP_COND
#define APP_INT_ON_COMMAND_COMPLETE	PHY_INT_COMMAND_COMPLETE


#define COMMAND_COMPLETE_APP_INT_PEND	PHY_COMMAND_COMPLETE_INT_PEND
#define RX_APP_INT_PEND			PHY_RX_INT_PEND
#define TX_APP_INT_PEND			PHY_TX_INT_PEND
#define EXCEP_COND_APP_INT_PEND		PHY_PHY_EXCEP_COND_INT_PEND
#define GLOBAL_EXCEP_COND_APP_INT_PEND	PHY_GLOBAL_EXCEP_COND_INT_PEND
#define TIMER_APP_INT_PEND		PHY_TIMER_INT_PEND
#define TRACE_APP_INT_PEND		PHY_TRACE_INT_PEND

#undef READ_CODE_VERSION
#define READ_CODE_VERSION		PHY_READ_CODE_VERSION

#undef READ_CONFIGURATION
#define READ_CONFIGURATION		PHY_READ_CONFIGURATION
#define SET_CONFIGURATION		PHY_SET_CONFIGURATION

#define DISABLE_COMMUNICATIONS		PHY_DISABLE_COMMUNICATIONS
#define ENABLE_COMMUNICATIONS		PHY_ENABLE_COMMUNICATIONS

#define SET_INTERRUPT_TRIGGERS		PHY_SET_INTERRUPT_TRIGGERS
#define READ_INTERRUPT_TRIGGERS		PHY_READ_INTERRUPT_TRIGGERS

//#define FT1_MONITOR_STATUS_CTRL		DFLT_TEMPLATE_VALUE
//#define ENABLE_READ_FT1_STATUS		DFLT_TEMPLATE_VALUE
//#define ENABLE_READ_FT1_OP_STATS	DFLT_TEMPLATE_VALUE		
//#define CPIPE_FT1_READ_STATUS		DFLT_TEMPLATE_VALUE
//#define TRACE_INACTIVE			DFLT_TEMPLATE_VALUE

//#define READ_GLOBAL_STATISTICS		DFLT_TEMPLATE_VALUE
//#define READ_MODEM_STATUS		DFLT_TEMPLATE_VALUE
//#define READ_LINK_STATUS		DFLT_TEMPLATE_VALUE
//#define READ_COMMS_ERROR_STATS		DFLT_TEMPLATE_VALUE
//#define READ_TRACE_CONFIGURATION	DFLT_TEMPLATE_VALUE	
//#define GET_TRACE_INFO			DFLT_TEMPLATE_VALUE

enum {
	FT1_READ_STATUS = WANPIPE_PROTOCOL_PRIVATE,
	FT1_MONITOR_STATUS_CTRL,
	ENABLE_READ_FT1_STATUS,
	ENABLE_READ_FT1_OP_STATS,
	READ_FT1_OPERATIONAL_STATS,
	ATM_LINK_STATUS
};

#define ATM_TRACE_CELL  0x01
#define	ATM_TRACE_PDU   0x02
#define ATM_TRACE_DATA  0x04

#undef READ_OPERATIONAL_STATS
#define READ_OPERATIONAL_STATS		PHY_READ_OPERATIONAL_STATS
//#define SET_TRACE_CONFIGURATION		DFLT_TEMPLATE_VALUE
//#define TRACE_ACTIVE			DFLT_TEMPLATE_VALUE	

#define EXCEP_IRQ_TIMEOUT		EXCEP_APP_IRQ_TIMEOUT
#define READ_EXCEPTION_CONDITION	PHY_READ_EXCEPTION_CONDITION
#define EXCEP_LINK_ACTIVE		DFLT_TEMPLATE_VALUE
#define EXCEP_LINK_INACTIVE_MODEM	DFLT_TEMPLATE_VALUE
#define EXCEP_LINK_INACTIVE_KPALV	DFLT_TEMPLATE_VALUE
#define EXCEP_IP_ADDRESS_DISCOVERED	DFLT_TEMPLATE_VALUE
#define EXCEP_LOOPBACK_CONDITION	DFLT_TEMPLATE_VALUE
#define NO_EXCEP_COND_TO_REPORT		DFLT_TEMPLATE_VALUE

//#define READ_GLOBAL_EXCEPTION_CONDITION	DFLT_TEMPLATE_VALUE
//#define EXCEP_MODEM_STATUS_CHANGE	DFLT_TEMPLATE_VALUE
#define DCD_HIGH			0x08
#define CTS_HIGH			0x20

#define INTERFACE_LEVEL_RS232		PHY_INTERFACE_LEVEL_RS232
#define INTERFACE_LEVEL_V35		PHY_INTERFACE_LEVEL_V35


#define MIN_WP_PRI_MTU 		1500
#define MAX_WP_PRI_MTU 		MIN_WP_PRI_MTU 
#define DEFAULT_WP_PRI_MTU 	MIN_WP_PRI_MTU

#define ATM_CELL_SIZE		53
#define MIN_WP_SEC_MTU 		50
#define MAX_WP_SEC_MTU 		PHY_MAX_CELLS_IN_RX_BLOCK*ATM_CELL_SIZE/ATM_OVERHEAD
#define DEFAULT_WP_SEC_MTU 	1500


/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   		0x01
#define TMR_INT_ENABLED_UPDATE		0x02
#define TMR_INT_ENABLED_CONFIG		0x10
#define TMR_INT_ENABLED_TE		0x20

#ifdef __KERNEL__

#define PRI_BASE_ADDR_MB_STRUCT		BASE_ADDR_MB_STRUCT

extern unsigned char calculate_hec_crc(unsigned char *util_ptr);

#endif
#endif
