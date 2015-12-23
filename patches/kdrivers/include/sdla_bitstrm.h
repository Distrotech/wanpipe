/**********************************************************************************
 *                                                                                *
 * BSTRMAPI.H - the 'C' header file for the Sangoma S508/S514 BITSTREAM code API. *
 *                                                                                *
 **********************************************************************************
*/

#ifndef _SDLA_BITSTRM_H_
#define _SDLA_BITSTRM_H_

#include <linux/if_wanpipe.h>

#pragma pack(1)

/* ----------------------------------------------------------------------------
 *        Constants defining the shared memory control block (mailbox)
 * --------------------------------------------------------------------------*/

#define PRI_BASE_ADDR_MB_STRUCT 		 	0xE000 	/* the base address of the mailbox structure (primary port) */
#define SEC_BASE_ADDR_MB_STRUCT 		 	0xE800 	/* the base address of the mailbox structure (secondary port) */
#define NUMBER_MB_RESERVED_BYTES			0x0B		/* the number of reserved bytes in the mailbox header area */
#define SIZEOF_MB_DATA_BFR					2032		/* the size of the actual mailbox data area */

#if 0
/* the control block mailbox structure */
typedef struct {
	unsigned char opp_flag;								/* the opp flag */
	unsigned char command;								/* the user command */
	unsigned short buffer_length;						/* the data length */
  	unsigned char return_code;							/* the return code */
	char MB_reserved[NUMBER_MB_RESERVED_BYTES];	/* reserved for later use */
	char data[SIZEOF_MB_DATA_BFR];					/* the data area */
} BSTRM_MAILBOX_STRUCT;
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
#undef	READ_COMMS_ERROR_STATS
#define READ_COMMS_ERROR_STATS				0x08	/* read the communication error statistics */
#undef	FLUSH_COMMS_ERROR_STATS
#define FLUSH_COMMS_ERROR_STATS				0x09	/* flush the communication error statistics */

/* BSTRM-level interface commands */
#define READ_BSTRM_CODE_VERSION				0x20	/* read the BSTRM code version */
#define READ_BSTRM_EXCEPTION_CONDITION		0x21	/* read an BSTRM exception condition from the adapter */
#define SET_BSTRM_CONFIGURATION				0x22	/* set the BSTRM configuration */
#define READ_BSTRM_CONFIGURATION				0x23	/* read the BSTRM configuration */
#define ENABLE_BSTRM_COMMUNICATIONS			0x24	/* enable BSTRM communications */
#define DISABLE_BSTRM_COMMUNICATIONS		0x25	/* disable BSTRM communications */
#define READ_BSTRM_STATUS						0x26	/* read the BSTRM status */
#define READ_BSTRM_OPERATIONAL_STATS		0x27	/* retrieve the BSTRM operational statistics */
#define FLUSH_BSTRM_OPERATIONAL_STATS		0x28	/* flush the BSTRM operational statistics */
#define SET_BSTRM_INTERRUPT_TRIGGERS		0x30	/* set the BSTRM application interrupt triggers */
#define READ_BSTRM_INTERRUPT_TRIGGERS		0x31	/* read the BSTRM application interrupt trigger configuration */



/* ----------------------------------------------------------------------------
 *                     Return codes from interface commands
 * --------------------------------------------------------------------------*/

#define OK											0x00	/* the interface command was successful */

/* return codes from global interface commands */
#define NO_GLOBAL_EXCEP_COND_TO_REPORT		0x01	/* there is no BSTRM exception condition to report */
#define LGTH_GLOBAL_CFG_DATA_INVALID		0x01	/* the length of the passed global configuration data is invalid */
#define IRQ_TIMEOUT_VALUE_INVALID			0x02	/* an invalid application IRQ timeout value was selected */
#define ADAPTER_OPERATING_FREQ_INVALID		0x03	/* an invalid adapter operating frequency was selected */
#define TRANSMIT_TIMEOUT_INVALID				0x04	/* the block transmit timeout is invalid */

/* return codes from command READ_GLOBAL_EXCEPTION_CONDITION */
#define EXCEP_MODEM_STATUS_CHANGE			0x10		/* a modem status change occurred */
#define EXCEP_APP_IRQ_TIMEOUT					0x12		/* an application IRQ timeout has occurred */

/* return codes from BSTRM-level interface commands */
#define NO_BSTRM_EXCEP_COND_TO_REPORT		0x21	/* there is no BSTRM exception condition to report */
#define BSTRM_COMMS_DISABLED					0x21	/* communications are not currently enabled */
#define BSTRM_COMMS_ENABLED					0x21	/* communications are currently enabled */
#define DISABLE_COMMS_BEFORE_CFG				0x21	/* communications must be disabled before setting the configuration */
#define ENABLE_BSTRM_COMMS_BEFORE_CONN		0x21	/* communications must be enabled before using the BSTRM_CONNECT conmmand */
#define CFG_BEFORE_COMMS_ENABLED				0x22	/* perform a SET_BSTRM_CONFIGURATION before enabling comms */
#define LGTH_BSTRM_CFG_DATA_INVALID			0x22	/* the length of the passed configuration data is invalid */
#define LGTH_INT_TRIGGERS_DATA_INVALID		0x22	/* the length of the passed interrupt trigger data is invalid */
#define INVALID_IRQ_SELECTED					0x23	/* an invalid IRQ was selected in the SET_BSTRM_INTERRUPT_TRIGGERS */
#define INVALID_BSTRM_CFG_DATA				0x23	/* the passed configuration data is invalid */
#define IRQ_TMR_VALUE_INVALID					0x24	/* an invalid application IRQ timer value was selected */
#define T1_E1_AMI_NOT_SUPPORTED				0x25	/* T1/E1 - AMI decoding not supported */
#define S514_BOTH_PORTS_SAME_CLK_MODE		0x26	/* S514 - both ports must have the same clocking mode */
#define INVALID_BSTRM_COMMAND					0x4F	/* the defined BSTRM interface command is invalid */
#define COMMAND_INVALID_FOR_PORT				0x50	/* the command is invalid for the selected port */

/* return codes from command READ_BSTRM_EXCEPTION_CONDITION */
#define EXCEP_SYNC_LOST							0x30	/* the BSTRM receiver has lost synchronization */
#define EXCEP_SYNC_ACHIEVED					0x31	/* the BSTRM receiver has achieved synchronization */
#define EXCEP_RX_DISCARD						0x36	/* an incoming block of data was discarded */
#define EXCEP_TX_IDLE							0x37	/* a block was transmitted from the idle buffer */




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
 *          Constants for the READ_BSTRM_EXCEPTION_CONDITION command
 * --------------------------------------------------------------------------*/

/* the structure returned on a return code EXCEP_RX_DISCARD and EXCEP_TX_IDLE */
typedef struct {
	unsigned long Rx_discard_count;					/* number of incoming blocks discarded */
	unsigned long Tx_idle_count;						/* number of Tx blocks transmitted from the idle code buffer (T1/E1) */
} RX_DISC_TX_IDLE_EXCEP_STRUCT;



/* ----------------------------------------------------------------------------
 *             Constants for the READ_COMMS_ERROR_STATS command
 * --------------------------------------------------------------------------*/

/* the communications error statistics structure */
typedef struct {
	unsigned short Rx_overrun_err_count; 			/* receiver overrun error count */
	unsigned short Rx_dis_pri_bfrs_full_count;	/* receiver disabled count */
	unsigned short pri_missed_Tx_DMA_int_count;	/* primary port - missed Tx DMA interrupt count */
	unsigned short sync_lost_count;					/* synchronization lost count */
	unsigned short sync_achieved_count;				/* synchronization achieved count */
	unsigned short P0_T1_E1_sync_failed_count;	/* T1/E1 synchronization failure count */
	unsigned short reserved_1;							/* reserved for later use */
	unsigned short DCD_state_change_count;			/* DCD state change count */
	unsigned short CTS_state_change_count;			/* CTS state change count */
} COMMS_ERROR_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *  Constants for the SET_BSTRM_CONFIGURATION/READ_BSTRM_CONFIGURATION command
 * --------------------------------------------------------------------------*/

/* the BSTRM configuration structure */
typedef struct {
	unsigned long baud_rate;							/* the baud rate */	
	unsigned short line_config_options;				/* line configuration options */
	unsigned short modem_config_options;			/* modem configuration options */
	unsigned short modem_status_timer;				/* timer for monitoring modem status changes */
	unsigned short API_options;						/* API options */
	unsigned short SYNC_options;						/* sync options */
	unsigned short Rx_sync_char;						/* receive sync character */
	unsigned char monosync_Tx_time_fill_char;		/* transmit time fill character (monosync mode) */
	unsigned short buffer_options;					/* Tx/Rx buffer options */
	unsigned short max_length_Tx_data_block;		/* the maximum length of a Tx data block */
	unsigned short Rx_complete_length;				/* length of receive block */
	unsigned short Rx_complete_timer;				/* the Rx completion timer */
	unsigned short statistics_options;				/* operational statistics options */
	unsigned long ptr_shared_mem_info_struct;		/* a pointer to the shared memory area information structure */
	unsigned long ptr_Tx_stat_el_cfg_struct;		/* a pointer to the transmit status element configuration structure */
	unsigned long ptr_Rx_stat_el_cfg_struct;		/* a pointer to the receive status element configuration structure */
} BSTRM_CONFIGURATION_STRUCT;

/* settings for the 'line_config_options' */
#define INTERFACE_LEVEL_V35			0x0000 /* V.35 interface level */

#define INTERFACE_LEVEL_RS232			0x0001 /* RS-232 interface level */

#define NRZI_ENCODING				0x0010 /* NRZI data encoding */

/* settings for the 'modem_config_options' */
#define DONT_RAISE_DTR_RTS_ON_EN_COMMS		0x0001 /* don't automatically raise DTR and RTS when performing an */
																 /* ENABLE_BSTRM_COMMUNICATIONS command */
#define DONT_REPORT_CHG_IN_MODEM_STAT 		0x0002 /* don't report changes in modem status to the application */

/* settings for the 'API_options' */
#define MANUAL_RESYNC_AFTER_SYNC_LOSS		0x0001 /* manual re-synchronization after synchronization loss */
#define PERMIT_APP_INT_TX_RX_BFR_ERR		0x0002 /* permit a BSTRM exception condition interrupt on a Tx or Rx */
																 /* buffer error */

/* settings for the 'SYNC_options' */
#define MONOSYNC_8_BIT_SYNC					0x0001 /* monosync, 8-bit sync character */	
#define MONOSYNC_6_BIT_SYNC					0x0002 /* monosync, 6-bit sync character */
#define BISYNC_16_BIT_SYNC						0x0004 /* bisync, 16-bit sync character */
#define BISYNC_12_BIT_SYNC						0x0008 /* bisync, 12-bit sync character */
#define EXTERNAL_SYNC_8_BIT_TIME_FILL		0x0010 /* external sync, 8-bit sync character */
#define EXTERNAL_SYNC_6_BIT_TIME_FILL		0x0020 /* external sync, 6-bit sync character */
#define SYNC_CHAR_LOAD_INHIBIT				0x0100 /* inhibit loading of the received sync character */
/* ??????????????wwwwwww */
#define T1_E1_ENABLE_TX_ALIGN_CHECK			0x1000 /* T1/E1 - enable checking of the transmit channel alignment */

/* bit settings for the 'buffer_options' */
#define TX_ONLY									0x0001 /* transmit only for this port (no reception) */
#define RX_ONLY									0x0002 /* receive only for this port (no transmission) */
#define SINGLE_TX_BUFFER						0x0010 /* configure a single transmit buffer */
#define SEC_DMA_RX								0x0100 /* secondary port - configure for high speed DMA receive mode */

/* settings for the 'statistics_options' */
#define TX_DATA_BYTE_COUNT_STAT				0x0001 /* record the number of data bytes transmitted */
#define RX_DATA_BYTE_COUNT_STAT				0x0002 /* record the number of data bytes received */
#define TX_THROUGHPUT_STAT						0x0004 /* compute the data transmit throughput */
#define RX_THROUGHPUT_STAT						0x0008 /* compute the data receive throughput */

/* permitted minimum and maximum values for setting the BSTRM configuration */
#define PRI_MAX_BAUD_RATE_S508				2666666 /* primary port - maximum baud rate (S508) */
#define SEC_MAX_BAUD_RATE_S508				258064  /* secondary port - maximum baud rate (S508) */
#define PRI_MAX_BAUD_RATE_S514				2750000 /* primary port - maximum baud rate (S514) */
#define SEC_MAX_BAUD_RATE_S514				515625  /* secondary port - maximum baud rate (S514) */
#define MIN_PERMITTED_MODEM_TIMER			0		  /* minimum modem status timer */
#define MAX_PERMITTED_MODEM_TIMER			5000	  /* maximum modem status timer */
#define PRI_MAX_LENGTH_TX_DATA_BLOCK		4096	  /* primary port - maximum length of the Tx data block */
#define SEC_MAX_LENGTH_TX_DATA_BLOCK		2048	  /* secondary port - maximum length of the Tx data block */
#define MAX_RX_COMPLETE_LENGTH				4096    /* the maximum length of receive data block */
#define MAX_RX_COMPLETE_TIMER					60000   /* the maximum Rx completion timer value */



/* ----------------------------------------------------------------------------
 *             Constants for the READ_BSTRM_STATUS command
 * --------------------------------------------------------------------------*/

/* the BSTRM status structure */
typedef struct {
	unsigned char sync_status;							/* synchronization status */
 	unsigned char BSTRM_excep_conditions;			/* BSTRM exception conditions */
	unsigned char no_Rx_data_blocks_avail;			/* number of Rx data blocks available for the application */
	unsigned char receiver_status;					/* receiver status (enabled/disabled) */
} READ_BSTRM_STATUS_STRUCT;

/* settings for the 'sync_status' variable */
#define SYNC_LOST									0x01
#define SYNC_ACHIEVED							0x02

/* settings for the 'receiver_status' variable */
#define RCVR_NOT_DISCARD						0x00	/* receiver not discarding incoming blocks */
#define RCVR_DISCARD								0x01	/* receiver discarding incoming blocks */



/* ----------------------------------------------------------------------------
 *           Constants for the READ_BSTRM_OPERATIONAL_STATS command
 * --------------------------------------------------------------------------*/

/* the BSTRM operational statistics structure */
typedef struct {

	/* data transmission statistics */
	unsigned long blocks_Tx_count;					/* number of blocks transmitted */
	unsigned long bytes_Tx_count; 					/* number of bytes transmitted */
	unsigned long Tx_throughput;						/* transmit throughput */
	unsigned long no_ms_for_Tx_thruput_comp;		/* millisecond time used for the Tx throughput computation */
	unsigned long Tx_idle_count;						/* number of times the transmitter reverted to the idle line condition (serial) OR */
																/* number of Tx blocks transmitted from the idle code buffer (T1/E1) */
	unsigned long Tx_discard_lgth_err_count;		/* number of Tx blocks discarded (length error) */
	unsigned long reserved_Tx_stat0;					/* reserved for later use */
	unsigned long reserved_Tx_stat1;					/* reserved for later use */
	unsigned long reserved_Tx_stat2;					/* reserved for later use */

	/* data reception statistics */
	unsigned long blocks_Rx_count;					/* number of blocks received */
	unsigned long bytes_Rx_count;						/* number of bytes received */
	unsigned long Rx_throughput;						/* receive throughput */
	unsigned long no_ms_for_Rx_thruput_comp;		/* millisecond time used for the Rx throughput computation */
	unsigned long Rx_discard_count;					/* number of incoming blocks discarded */
	unsigned long reserved_Rx_stat1;					/* reserved for later use */
	unsigned long reserved_Rx_stat2;					/* reserved for later use */

} BSTRM_OPERATIONAL_STATS_STRUCT;



/* ----------------------------------------------------------------------------
 *                 Constants for using application interrupts
 * --------------------------------------------------------------------------*/

/* the structure used for the SET_BSTRM_INTERRUPT_TRIGGERS/READ_BSTRM_INTERRUPT_TRIGGERS command */
typedef struct {
	unsigned char BSTRM_interrupt_triggers;		/* BSTRM interrupt trigger configuration */
	unsigned char IRQ;									/* IRQ to be used */
	unsigned short interrupt_timer;					/* interrupt timer */
	unsigned short misc_interrupt_bits;				/* miscellaneous interrupt bits */
} BSTRM_INT_TRIGGERS_STRUCT;

/* 'BSTRM_interrupt_triggers' bit settings */
#define APP_INT_ON_RX_BLOCK					0x01	/* interrupt on data block reception */
#define APP_INT_ON_TX_BLOCK					0x02	/* interrupt when an data block may be transmitted */
#define APP_INT_ON_COMMAND_COMPLETE			0x04	/* interrupt when an interface command is complete */
#define APP_INT_ON_TIMER						0x08	/* interrupt on a defined millisecond timeout */
#define APP_INT_ON_GLOBAL_EXCEP_COND 		0x10	/* interrupt on a global exception condition */
#define APP_INT_ON_BSTRM_EXCEP_COND			0x20	/* interrupt on an BSTRM exception condition */

/* 'interrupt_timer' limitation */
#define MAX_INTERRUPT_TIMER_VALUE 			60000		/* the maximum permitted timer interrupt value */

/* interrupt types indicated at 'interrupt_type' byte of the INTERRUPT_INFORMATION_STRUCT */
#define NO_APP_INTS_PEND						0x00	/* no interrups are pending */
#define RX_APP_INT_PEND							0x01	/* a receive interrupt is pending */
#define TX_APP_INT_PEND							0x02	/* a transmit interrupt is pending */
#define COMMAND_COMPLETE_APP_INT_PEND		0x04	/* a 'command complete' interrupt is pending */
#define TIMER_APP_INT_PEND						0x08	/* a timer interrupt is pending */
#define GLOBAL_EXCEP_COND_APP_INT_PEND 	0x10	/* a global exception condition interrupt is pending */
#define BSTRM_EXCEP_COND_APP_INT_PEND 		0x20	/* an BSTRM exception condition interrupt is pending */



/* ----------------------------------------------------------------------------
 *                   Constants for data block transmission
 * --------------------------------------------------------------------------*/

/* the data block transmit status element configuration structure */
typedef struct {
	unsigned short number_Tx_status_elements;		/* number of transmit status elements */
	unsigned long base_addr_Tx_status_elements;	/* base address of the transmit element list */
	unsigned long next_Tx_status_element_to_use;	/* pointer to the next transmit element to be used */
} BSTRM_TX_STATUS_EL_CFG_STRUCT;

/* the data block transmit status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short block_length;						/* length of the block to be transmitted */
	unsigned char misc_Tx_bits;						/* miscellaneous transmit bits */
	unsigned short Tx_time_fill_char;				/* transmit time fill character */
	unsigned short reserved_0;							/* reserved for internal use */
	unsigned long reserved_1;							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} BSTRM_DATA_TX_STATUS_EL_STRUCT;

/* settings for the 'misc_Tx_bits' */
#define UPDATE_TX_TIME_FILL_CHAR				0x01	/* update the transmit time fill character */



/* ----------------------------------------------------------------------------
 *                   Constants for data block reception
 * --------------------------------------------------------------------------*/

/* the data block receive status element configuration structure */
typedef struct {
	unsigned short number_Rx_status_elements;		/* number of receive status elements */
	unsigned long base_addr_Rx_status_elements;	/* base address of the receive element list */
	unsigned long next_Rx_status_element_to_use;	/* pointer to the next receive element to be used */
} BSTRM_RX_STATUS_EL_CFG_STRUCT;

/* the data block receive status element structure */
typedef struct {
	unsigned char opp_flag;								/* opp flag */
	unsigned short block_length;						/* length of the received block */
	unsigned char error_flag;							/* block errors */
	unsigned short time_stamp;							/* receive time stamp */
	unsigned long reserved_0;							/* reserved for internal use */
	unsigned short reserved_1;							/* reserved for internal use */
	unsigned long ptr_data_bfr;						/* pointer to the data area */
} BSTRM_DATA_RX_STATUS_EL_STRUCT;

/* settings for the 'error_flag' */
#define RX_OVERRUN_ERROR						0x04	/* the incoming block has an overrun error */



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

/* the BSTRM information structure */
typedef struct {
	unsigned char sync_status;							/* synchronization status */
 	unsigned char BSTRM_excep_conditions;			/* BSTRM exception conditions */
	unsigned char no_Rx_data_blocks_avail;			/* number of Rx data blocks available for the application */
	unsigned long Rx_discard_count;					/* number of incoming blocks discarded */
	unsigned long Tx_idle_count;						/* number of Tx blocks transmitted from the idle code buffer (T1/E1) */
	unsigned char BSTRM_info_reserved[5];			/* reserved */
} BSTRM_INFORMATION_STRUCT;

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

/* settings for the 'FT1_INS_alarm_condition' */
#define BITSTRM_FT1_IN_SERVICE		0x00	/* the FT1 is in-service */
#define BITSTRM_BLUE_ALARM 		'B'	/* blue alarm condition */
#define BITSTRM_YELLOW_ALARM 		'Y'	/* yellow alarm condition */
#define BITSTRM_RED_ALARM 		'R'	/* red alarm condition */

/* the shared memory area information structure */
typedef struct {
	GLOBAL_INFORMATION_STRUCT global_info_struct;		/* the global information structure */
	BSTRM_INFORMATION_STRUCT BSTRM_info_struct;			/* the BSTRM information structure */
	INTERRUPT_INFORMATION_STRUCT interrupt_info_struct;/* the interrupt information structure */
	FT1_INFORMATION_STRUCT FT1_info_struct;				/* the FT1 information structure */
} SHARED_MEMORY_INFO_STRUCT;


/* Special UDP drivers management commands */
#define BPIPE_ROUTER_UP_TIME				WANPIPEMON_ROUTER_UP_TIME
#define BPIPE_ENABLE_TRACING				WANPIPEMON_ENABLE_TRACING
#define BPIPE_DISABLE_TRACING				WANPIPEMON_DISABLE_TRACING
#define BPIPE_GET_TRACE_INFO				WANPIPEMON_GET_TRACE_INFO

#define CPIPE_GET_IBA_DATA					WANPIPEMON_GET_IBA_DATA

#define BPIPE_FT1_READ_STATUS				WANPIPEMON_DRIVER_STAT_IFSEND
#define BPIPE_DRIVER_STAT_IFSEND			WANPIPEMON_DRIVER_STAT_IFSEND
#define BPIPE_DRIVER_STAT_INTR				WANPIPEMON_DRIVER_STAT_INTR
#define BPIPE_DRIVER_STAT_GEN				WANPIPEMON_DRIVER_STAT_GEN
#define BPIPE_FLUSH_DRIVER_STATS			WANPIPEMON_FLUSH_DRIVER_STATS



/* modem status changes */
#define DCD_HIGH			0x08
#define CTS_HIGH			0x20

#ifdef UDPMGMT_SIGNATURE
 #undef UDPMGMT_SIGNATURE
 #define UDPMGMT_SIGNATURE	"BTPIPEAB"
#endif
/* valid ip_protocol for UDP management */
#define UDPMGMT_UDP_PROTOCOL 0x11

#pragma pack()



enum {
	SIOC_WANPIPE_BITSTRM_T1E1_CFG = SIOC_WANPIPE_PIPEMON + 1,
	SIOC_CUSTOM_BITSTRM_COMMANDS,

	SIOC_WRITE_RBS_SIG = SIOC_WANPIPE_DEVPRIVATE,
	SIOC_READ_RBS_SIG 
};

enum {
	COMMS_ALREADY_ENABLED=1,
	COMMS_ALREADY_DISABLED,
	IF_IN_DISCONNECED_STATE,
	FRONT_END_IN_DISCONNECTED_STATE
};


typedef struct{
	unsigned char control_code;
	unsigned short reg;
	unsigned char bit_number;
}custom_control_call_t;

//values for 'control_code'
enum {
	SET_BIT_IN_PMC_REGISTER,
	RESET_BIT_IN_PMC_REGISTER
};

#ifdef __KERNEL__

#define OPERATE_T1E1_AS_SERIAL		0x8000  /* For bitstreaming only 
						 * Allow the applicatoin to 
						 * E1 front end */


static inline void bstrm_skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb=skb_dequeue(list))!=NULL)
		dev_kfree_skb_any(skb);
}

static inline void bstrm_test_rx_tx_discard (sdla_t *card)
{
	unsigned long cnt;
	card->hw_iface.peek(card->hw, card->u.b.rx_discard_off,
		                &cnt,sizeof(unsigned long));
	if (cnt){
		DEBUG_EVENT("%s: Error: Rx discard cnt %lu\n",
				card->devname,cnt);
		cnt=0;
		card->hw_iface.poke(card->hw, card->u.b.rx_discard_off,
		                &cnt,sizeof(unsigned long));
	}

	card->hw_iface.peek(card->hw, card->u.b.rx_discard_off,
		                &cnt,sizeof(unsigned long));
	if (cnt){
		DEBUG_EVENT("%s: Error: Tx idle cnt %lu\n",
				card->devname,cnt);
		cnt=0;
		card->hw_iface.poke(card->hw, card->u.b.tx_idle_off,
		                &cnt,sizeof(unsigned long));
	}
}

#endif


#endif
