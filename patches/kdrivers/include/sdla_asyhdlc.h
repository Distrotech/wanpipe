/*
 ***********************************************************************************
 *                                                                                 *
 * ASYAPI.H - the 'C' header file for the Sangoma S508/S514 asynchronous code API. *
 *                                                                                 *
 ***********************************************************************************
*/

#ifndef _SDLA_ASYHDLC_H_
#define _SDLA_ASYHDLC_H_

#pragma pack(1)

/* ----------------------------------------------------------------------------
 *        Constants defining the shared memory control block (mailbox)
 * --------------------------------------------------------------------------*/

#define PRI_BASE_ADDR_MB_STRUCT 		 	0xE000 	/* the base address of the mailbox structure (primary port) */
#define SEC_BASE_ADDR_MB_STRUCT 		 	0xE800 	/* the base address of the mailbox structure (secondary port) */
#define NUMBER_MB_RESERVED_BYTES			0x0B		/* the number of reserved bytes in the mailbox header area */
#define SIZEOF_MB_DATA_BFR					2032		/* the size of the actual mailbox data area */

/* the control block mailbox structure */
typedef struct {
	unsigned char opp_flag;								/* the opp flag */
	unsigned char command;								/* the user command */
	unsigned short buffer_length;						/* the data length */
  	unsigned char return_code;							/* the return code */
	char MB_reserved[NUMBER_MB_RESERVED_BYTES];	/* reserved for later use */
	char data[SIZEOF_MB_DATA_BFR];					/* the data area */
} ASY_MAILBOX_STRUCT;



/* ----------------------------------------------------------------------------
 *                        Interface commands
 * --------------------------------------------------------------------------*/

/* interface commands */
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
#define READ_ASY_CODE_VERSION					0x20	/* read the code version */
#define SET_ASY_INTERRUPT_TRIGGERS			0x30	/* set the application interrupt triggers */
#define READ_ASY_INTERRUPT_TRIGGERS			0x31	/* read the application interrupt trigger configuration */
#define SET_ASY_CONFIGURATION					0xE2	/* set the asychronous operational configuration */
#define READ_ASY_CONFIGURATION				0xE3	/* read the current asychronous operational configuration */
#define ENABLE_ASY_COMMUNICATIONS			0xE4	/* enable asychronous communications */
#define DISABLE_ASY_COMMUNICATIONS			0xE5	/* disable asychronous communications */
#define READ_ASY_OPERATIONAL_STATS			0xE7	/* retrieve the asychronous operational statistics */
#define FLUSH_ASY_OPERATIONAL_STATS			0xE8	/* flush the asychronous operational statistics */
#define TRANSMIT_ASY_BREAK_SIGNAL			0xEC	/* transmit an asychronous break signal */


/* ----------------------------------------------------------------------------
 *                     Return codes from interface commands
 * --------------------------------------------------------------------------*/

#define OK						0x00	/* the interface command was successful */
#define COMMAND_OK					0x00	/* the interface command was successful */

/* return codes from global interface commands */
#define NO_GLOBAL_EXCEP_COND_TO_REPORT		0x01	/* there is no global exception condition to report */
#define LGTH_GLOBAL_CFG_DATA_INVALID		0x01	/* the length of the passed global configuration data is invalid */
#define IRQ_TIMEOUT_VALUE_INVALID			0x02	/* an invalid application IRQ timeout value was selected */
#define ADAPTER_OPERATING_FREQ_INVALID		0x03	/* an invalid adapter operating frequency was selected */
#define TRANSMIT_TIMEOUT_INVALID				0x04	/* the frame transmit timeout is invalid */


/* return codes from command READ_GLOBAL_EXCEPTION_CONDITION */
#define EXCEP_MODEM_STATUS_CHANGE			0x10		/* a modem status change occurred */
#define EXCEP_APP_IRQ_TIMEOUT					0x12		/* an application IRQ timeout has occurred */
#define EXCEP_ASY_BREAK_RECEIVED				0x17		/* a break sequence has been received (asynchronous mode only) */

/* return codes from interface commands */
#define xxxCOMMS_DISABLED							0x21	/* communications are not currently enabled */
#define xxxCOMMS_ENABLED							0x21	/* communications are currently enabled */
#define LGTH_INT_TRIGGERS_DATA_INVALID		0x22	/* the length of the passed interrupt trigger data is invalid */
#define INVALID_IRQ_SELECTED					0x23	/* an invalid IRQ was selected in the SET_ASY_INTERRUPT_TRIGGERS */
#define IRQ_TMR_VALUE_INVALID					0x24	/* an invalid application IRQ timer value was selected */
#define DISABLE_ASY_COMMS_BEFORE_CFG 		0xE1	/* communications must be disabled before setting the configuration */
#define ASY_COMMS_ENABLED						0xE1	/* communications are currently enabled */
#define ASY_COMMS_DISABLED						0xE1	/* communications are currently disabled */
#define ASY_CFG_BEFORE_COMMS_ENABLED		0xE2	/* perform a SET_ASY_CONFIGURATION before enabling comms */
#define LGTH_ASY_CFG_DATA_INVALID  			0xE2	/* the length of the passed configuration data is invalid */
#define INVALID_ASY_CFG_DATA					0xE3	/* the passed configuration data is invalid */
#define ASY_BREAK_SIGNAL_BUSY					0xEC	/* a break signal is being transmitted */
#define INVALID_COMMAND							0xFF	/* the defined interface command is invalid */



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
	unsigned short frame_transmit_timeout;			/* frame transmission timeout */
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
	unsigned short FT1_INS_alarm_count;				/* FT1 in-service/alarm condition count */
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
	unsigned short Rx_parity_err_count;				/* receiver parity error count */
	unsigned short Rx_framing_err_count; 			/* framing errors received count */
	unsigned short DCD_state_change_count;			/* DCD state change count */
	unsigned short CTS_state_change_count;			/* CTS state change count */
} COMMS_ERROR_STATS_STRUCT;




/* ----------------------------------------------------------------------------
 *                 Constants for using application interrupts
 * --------------------------------------------------------------------------*/

/* the structure used for the SET_ASY_INTERRUPT_TRIGGERS/READ_ASY_INTERRUPT_TRIGGERS command */
typedef struct {
	unsigned char interrupt_triggers;				/* interrupt trigger configuration */
	unsigned char IRQ;									/* IRQ to be used */
	unsigned short interrupt_timer;					/* interrupt timer */
	unsigned short misc_interrupt_bits;				/* miscellaneous interrupt bits */
} ASY_INT_TRIGGERS_STRUCT;

/* 'interrupt_triggers' bit settings */
#define APP_INT_ON_RX							0x01	/* interrupt on reception */
#define APP_INT_ON_RX_FRAME APP_INT_ON_RX
#define APP_INT_ON_TX							0x02	/* interrupt when data may be transmitted */
#define APP_INT_ON_COMMAND_COMPLETE			0x04	/* interrupt when an interface command is complete */
#define APP_INT_ON_TIMER						0x08	/* interrupt on a defined millisecond timeout */
#define APP_INT_ON_GLOBAL_EXCEP_COND 		0x10	/* interrupt on a global exception condition */

/* 'interrupt_timer' limitation */
#define MAX_INTERRUPT_TIMER_VALUE 			60000		/* the maximum permitted timer interrupt value */

/* interrupt types indicated at 'interrupt_type' byte of the INTERRUPT_INFORMATION_STRUCT */
#define NO_APP_INTS_PEND						0x00	/* no interrups are pending */
#define RX_APP_INT_PEND							0x01	/* a receive interrupt is pending */
#define TX_APP_INT_PEND							0x02	/* a transmit interrupt is pending */
#define COMMAND_COMPLETE_APP_INT_PEND		0x04	/* a 'command complete' interrupt is pending */
#define TIMER_APP_INT_PEND						0x08	/* a timer interrupt is pending */
#define GLOBAL_EXCEP_COND_APP_INT_PEND 	0x10	/* a global exception condition interrupt is pending */



/* ----------------------------------------------------------------------------
 *   Constants for the SET_ASY_CONFIGURATION/READ_ASY_CONFIGURATION command
 * --------------------------------------------------------------------------*/

/* the asynchronous configuration structure */
typedef struct {
	unsigned long baud_rate;							/* the baud rate */	
	unsigned short line_config_options;				/* line configuration options */
	unsigned short modem_config_options;			/* modem configuration options */
	unsigned short API_options;						/* asynchronous API options */
	unsigned short protocol_options;					/* asynchronous protocol options */
	unsigned short Tx_bits_per_char;					/* number of bits per transmitted character */
	unsigned short Rx_bits_per_char;					/* number of bits per received character */
	unsigned short stop_bits;							/* number of stop bits per character */
	unsigned short parity;								/* parity definition */
	unsigned short break_timer;						/* the break signal timer */
	unsigned short Rx_inter_char_timer;				/* the receive inter-character timer */
	unsigned short Rx_complete_length;				/* the receive 'buffer complete' length */
	unsigned short XON_char;							/* the XON character */
	unsigned short XOFF_char;							/* the XOFF character */
	unsigned short statistics_options;				/* operational statistics options */
	unsigned long ptr_shared_mem_info_struct;		/* a pointer to the shared memory area information structure */
	unsigned long ptr_asy_Tx_stat_el_cfg_struct;	/* a pointer to the transmit status element configuration structure */
	unsigned long ptr_asy_Rx_stat_el_cfg_struct;	/* a pointer to the receive status element configuration structure */
} ASY_CONFIGURATION_STRUCT;

/* settings for the 'line_config_options' */
#define INTERFACE_LEVEL_V35					0x0000 /* V.35 interface level */
#define INTERFACE_LEVEL_RS232					0x0001 /* RS-232 interface level */

/* settings for the 'modem_config_options' */
#define DONT_RAISE_DTR_RTS_ON_EN_COMMS		0x0001 /* don't automatically raise DTR and RTS when performing an */
																 /* ENABLE_ASY_COMMUNICATIONS command */
#define DONT_REPORT_CHG_IN_MODEM_STAT 		0x0002 /* don't report changes in modem status to the application */

/* settings for the 'asy_statistics_options' */
#define ASY_TX_THROUGHPUT_STAT				0x0004 /* compute the transmit throughput */
#define ASY_RX_THROUGHPUT_STAT				0x0008 /* compute the receive throughput */

/* permitted minimum and maximum values for setting the asynchronous configuration */
#define MIN_ASY_BAUD_RATE						50			/* maximum baud rate */
#define MAX_ASY_BAUD_RATE						250000	/* minimum baud rate */
#define MIN_ASY_BITS_PER_CHAR					5			/* minimum number of bits per character */
#define MAX_ASY_BITS_PER_CHAR					8			/* maximum number of bits per character */
#define MAX_BREAK_TMR_VAL						5000		/* maximum break signal timer */
#define MAX_ASY_RX_INTER_CHAR_TMR			30000		/* maximum receive inter-character timer */
#define MAX_ASY_RX_CPLT_LENGTH				2000		/* maximum receive 'length complete' value */

/* bit settings for the 'asy_API_options' */
#define ASY_RX_DATA_TRANSPARENT				0x0001	/* do not strip parity and unused bits from received characters */

/* bit settings for the 'asy_protocol_options' */
#define ASY_RTS_HS_FOR_RX						0x0001	/* RTS handshaking is used for reception control */
#define ASY_XON_XOFF_HS_FOR_RX				0x0002	/* XON/XOFF handshaking is used for reception control */
#define ASY_XON_XOFF_HS_FOR_TX				0x0004	/* XON/XOFF handshaking is used for transmission control */
#define ASY_DCD_HS_FOR_TX						0x0008	/* DCD handshaking is used for transmission control */
#define ASY_CTS_HS_FOR_TX						0x0020	/* CTS handshaking is used for transmission control */
#define ASY_HDLC_FRAMING						0x0100	/* use HDLC framing */
#define ASY_HDLC_PASS_RX_CRC_TO_APP			0x0200	/* pass received HDLC CRC bytes to the application */
#define ASY_HDLC_PASS_RX_BAD_TO_APP			0x0400	/* pass received bad frames to the application */
#define ASY_DIS_RX_WHEN_TX						0x1000	/* disable the receiver when transmitting */

/* bit settings for the 'stop_bits' definition */
#define ONE_STOP_BIT								1			/* representation for 1 stop bit */
#define TWO_STOP_BITS							2			/* representation for 2 stop bits */
#define ONE_AND_A_HALF_STOP_BITS				3			/* representation for 1.5 stop bits */

/* bit settings for the 'parity' definition */
#define NO_PARITY									0			/* representation for no parity */
#define ODD_PARITY								1			/* representation for odd parity */
#define EVEN_PARITY								2			/* representation for even parity */



/* ----------------------------------------------------------------------------
 *    Constants for the READ_COMMS_ERROR_STATS command (asynchronous mode)
 * --------------------------------------------------------------------------*/

/* the communications error statistics structure */
typedef struct {
	unsigned short Rx_overrun_err_count; 			/* receiver overrun error count */
	unsigned short Rx_parity_err_count;				/* parity errors received count */
	unsigned short Rx_framing_err_count;			/* framing errors received count */
	unsigned short DCD_state_change_count;			/* DCD state change count */
	unsigned short CTS_state_change_count;			/* CTS state change count */
} ASY_COMMS_ERROR_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *         Constants for the READ_ASY_OPERATIONAL_STATS command
 * --------------------------------------------------------------------------*/

/* the asynchronous operational statistics structure */
typedef struct {

	/* transmission statistics */
	unsigned long blocks_Tx_count;					/* number of blocks transmitted */
	unsigned long bytes_Tx_count; 					/* number of bytes transmitted */
	unsigned long Tx_throughput;						/* transmit throughput */
	unsigned long no_ms_for_Tx_thruput_comp;		/* millisecond time used for the Tx throughput computation */
	unsigned long Tx_block_discard_lgth_err_count;/* number of blocks discarded (length error) */

	/* reception statistics */
	unsigned long blocks_Rx_count;					/* number of blocks received */
	unsigned long bytes_Rx_count;						/* number of bytes received */
	unsigned long Rx_throughput;						/* receive throughput */
	unsigned long no_ms_for_Rx_thruput_comp;		/* millisecond time used for the Rx throughput computation */
	unsigned long Rx_bytes_discard_count;			/* received Data bytes discarded */

	/* handshaking protocol statistics */
	unsigned short XON_chars_Tx_count;				/* number of XON characters transmitted */
	unsigned short XOFF_chars_Tx_count;				/* number of XOFF characters transmitted */
	unsigned short XON_chars_Rx_count;				/* number of XON characters received */
	unsigned short XOFF_chars_Rx_count;				/* number of XOFF characters received */
	unsigned short Tx_halt_modem_low_count;		/* number of times Tx halted (modem line low) */
	unsigned short Rx_halt_RTS_low_count;			/* number of times Rx halted by setting RTS low */

	/* break statistics */
	unsigned short break_Tx_count;					/* number of break sequences transmitted */
	unsigned short break_Rx_count;					/* number of break sequences received */

} ASY_OPERATIONAL_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *                      Constants for data transmission
 * --------------------------------------------------------------------------*/

/* the data block transmit status element configuration structure */
typedef struct {
	unsigned short number_Tx_status_elements;		/* number of transmit status elements */
	unsigned long base_addr_Tx_status_elements;	/* base address of the transmit element list */
	unsigned long next_Tx_status_element_to_use;	/* pointer to the next transmit element to be used */
} ASY_TX_STATUS_EL_CFG_STRUCT;


/* the data block transmit status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short data_length;						/* length of the block to be transmitted */
	unsigned char misc_bits;							/* miscellaneous transmit bits */
	unsigned char reserved[8];							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} ASY_DATA_TX_STATUS_EL_STRUCT;



/* ----------------------------------------------------------------------------
 *                      Constants for data reception
 * --------------------------------------------------------------------------*/

/* the data block receive status element configuration structure */
typedef struct {
	unsigned short number_Rx_status_elements;		/* number of receive status elements */
	unsigned long base_addr_Rx_status_elements;	/* base address of the receive element list */
	unsigned long next_Rx_status_element_to_use;	/* pointer to the next receive element to be used */
	unsigned long base_addr_Rx_buffer;				/* base address of the receive data buffer */
	unsigned long end_addr_Rx_buffer;				/* end address of the receive data buffer */
} ASY_RX_STATUS_EL_CFG_STRUCT;

/* the data block receive status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short data_length;						/* length of the received data block */
	unsigned char misc_bits;							/* miscellaneous receive bits */
	unsigned short time_stamp;							/* receive time stamp (HDLC_STREAMING_MODE) */
	unsigned short data_buffered;						/* the number of data bytes still buffered */
	unsigned char reserved[4];							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} ASY_DATA_RX_STATUS_EL_STRUCT;

/* settings for the 'misc_bits' */
#define RX_HDLC_FRM_ABORT						0x01	/* the incoming frame was aborted */
#define RX_HDLC_FRM_CRC_ERROR					0x02	/* the incoming frame has a CRC error */
#define RX_FRM_OVERRUN_ERROR					0x04	/* the incoming frame has an overrun error */
#define RX_HDLC_FRM_SHORT_ERROR				0x10	/* the incoming frame was too short */
#define RX_HDLC_FRM_LONG_ERROR				0x20	/* the incoming frame was too long */


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

/* the ASY information structure */
typedef struct {
	unsigned char asy_rx_avail;
	unsigned char asy_info_reserved[15];			/* reserved */
} ASY_INFORMATION_STRUCT;

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
	GLOBAL_INFORMATION_STRUCT global_info_struct;		/* the global information structure */
	ASY_INFORMATION_STRUCT async_info_struct;				/* the asynchronous information structure */
	INTERRUPT_INFORMATION_STRUCT interrupt_info_struct;/* the interrupt information structure */
	FT1_INFORMATION_STRUCT FT1_info_struct;				/* the FT1 information structure */
} SHARED_MEMORY_INFO_STRUCT;


#define MAX_LGTH_HDLC_FRAME					2048	/* the maximum permitted length of an HDLC frame */


/* ----------------------------------------------------------------------------
 *                     HDLC Interface commands
 * --------------------------------------------------------------------------*/

#define READ_HDLC_OPERATIONAL_STATS			0xF0	/* retrieve the HDLC operational statistics */
#define FLUSH_HDLC_OPERATIONAL_STATS		0xF1	/* flush the HDLC operational statistics */

/* the HDLC operational statistics structure (returned on READ_HDLC_OPERATIONAL_STATS command) */
typedef struct {
	/* frame transmission statistics */
	unsigned long frames_Tx_count;					/* number of frames transmitted */
	unsigned long bytes_Tx_count; 					/* number of bytes transmitted */
	unsigned long Tx_frame_discard_lgth_err_count;/* number of frames discarded (length error) */

	/* frame reception statistics */
	unsigned long frames_Rx_count;					/* number of frames received */
	unsigned long bytes_Rx_count;						/* number of bytes received */
	unsigned long Rx_frame_short_count;				/* received frames too short */
	unsigned long Rx_frame_long_count;				/* received frames too long */
	unsigned long Rx_frame_discard_full_count;	/* received frames discarded (buffer full) */
	unsigned long CRC_error_count;					/* receiver CRC error count */
	unsigned long Rx_abort_count; 					/* abort frames received count */
} HDLC_OPERATIONAL_STATS_STRUCT;




#undef UDPMGMT_SIGNATURE
#define UDPMGMT_SIGNATURE	"CTPIPEAB"
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
	unsigned long	ip_src_address	;
	unsigned long	ip_dst_address	;
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

#if 0
#define CPIPE_MPPP_TRACE_ENABLE				0x60
#define CPIPE_MPPP_TRACE_DISABLE			0x61
#define CPIPE_TE1_56K_STAT				0x62	/* TE1_56K */
#define CPIPE_GET_MEDIA_TYPE				0x63	/* TE1_56K */
#define CPIPE_FLUSH_TE1_PMON				0x64	/* TE1     */
#define CPIPE_READ_REGISTER				0x65	/* TE1_56K */
#define CPIPE_TE1_CFG					0x66	/* TE1     */
#endif

/* Driver specific commands for API */
#define	CHDLC_READ_TRACE_DATA		0xE4	/* read trace data */
#define TRACE_ALL                       0x00
#define TRACE_PROT			0x01
#define TRACE_DATA			0x02

#define SIOC_ASYHDLC_RX_AVAIL_CMD	SIOC_WANPIPE_DEVPRIVATE
#define SIOC_GET_HDLC_RECEIVER_STATUS   SIOC_ASYHDLC_RX_AVAIL_CMD

/* Return codes for  SIOC_GET_HDLC_RECEIVER_STATUS cmd */
enum {
	HDLC_RX_IN_PROCESS,
	NO_HDLC_RX_IN_PROCESS
};

#pragma pack()

#endif


