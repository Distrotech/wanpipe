/*****************************************************************************
* bsc_api.h     Bisync Streaming API header file.
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
* Sep 24, 1998  Jaspreet Singh  o Initial Version.
*****************************************************************************/

#ifndef __SDLA_BSTRM_H__
#define __SDLA_BSTRM_H__

#pragma       pack(1)

/* 	Status flag for determining whether to perform a check on socket receive
 * 	queue.
 */
#define NO_SK_RX_CHECK		0x00
#define TOP_CHECK_SK_RX_Q	0x01
#define BTM_CHECK_SK_RX_Q	0x02

#define NO_READ_CMD	0x00
#define READ_CMD	0x01

/* 	physical addresses on the adapter 
 */
#define SEND_MB_OFF		0x0000	/* base addr of the SEND mb area */
#define RECEIVE_MB_OFF		0x1000	/* base addr of the RECEIVE mb area */
#define INTERRUPT_REPORT_IB_OFF	0x1FF0	/* a ptr to the interrupt reporting interface byte */
#define INTERRUPT_PERMIT_IB_OFF	0x1FF1   /* a pointer to the interrupt permission interface byte */
#define PTR_MODEM_STATUS_IB	0xFFF4   /* a pointer to the modem status interface byte */


/* BSC streaming commands */
#define BSC_WRITE                      0x01   /* transmit a BSC block */
#define BSC_READ		       0x90
#define ENABLE_COMMUNICATIONS          0x02   /* enable communications */
#define DISABLE_COMMUNICATIONS         0x03   /* disable communications */
#define READ_OPERATIONAL_STATS         0x07   /* retrieve the operational statistics */
#define FLUSH_OPERATIONAL_STATS        0x08   /* flush the operational statistics */
#define READ_COMMS_ERR_STATS           0x09   /* read the communication error statistics */
#define FLUSH_COMMS_ERR_STATS          0x0A   /* flush the communication error statistics */
#define SET_MODEM_STATUS               0x0B   /* set the modem status */
#define READ_MODEM_STATUS              0x0C   /* read the current modem status (CTS and DCD status) */
#define FLUSH_BSC_BUFFERS              0x0D   /* flush any queued transmit and receive buffers */
#define READ_CONFIGURATION             0x10   /* read the current operational configuration */
#define SET_CONFIGURATION              0x11   /* set the operational configuration */
#define READ_CODE_VERSION              0x15   /* read the code version */
#define SET_INTERRUPT_TRIGGERS         0x20   /* set the interrupt triggers */
#define READ_INTERRUPT_TRIGGERS        0x21   /* read the interrupt trigger configuration */
#define READ_ADAPTER_CONFIGURATION     0xA0   /* find out what type of adapter is being used */



/* return code from BSC streaming commands */
#define COMMAND_SUCCESSFULL            0x00   /* the command was successfull */
#define COMMUNICATIONS_DISABLED        0x01   /* communications are currently disabled */   
#define COMMUNICATIONS_ENABLED         0x02   /* communications are currently enabled */
#define SET_CONFIG_BEFORE_ENABLE_COMMS 0x03   /* SET_CONFIGURATION before ENABLE_COMMUNICATIONS */
#define INVALID_CONFIGURATION_DATA     0x03   /* the passed configuration data is invalid */
#define INVALID_TX_DATA_LGTH           0x03   /* the length of the data to be transmitted is invalid */
#define CANT_FLUSH_BUSY_SENDING        0x03   /* the BSC buffers cannot be flushed as a block is being transmitted */
#define TX_BUFFERS_FULL                0x04   /* the transmit buffers are full */
#define ILLEGAL_COMMAND                0x05   /* the defined HDLC command is invalid */
#define MODEM_STATUS_CHANGE            0x10   /* a modem (DCD/CTS) status change ocurred */
#define NO_DATA_AVAILABLE		0x20

/* mailbox definitions */
/*
The two mailboxes take up a total of 8192 bytes, so each is 4096 bytes in size. However, we reserve the last 16 bytes
in the receive mailbox for miscellaneous interface bytes, so each mailbox is now 4080 bytes. If we subtract 16 bytes
for the mailbox header area, we are left with a mailbox data size of 4064 bytes.
*/
#define SIZEOF_MB_DATA_BFR      4064
#define MAILBOX_SIZE		20

/* the mailbox structure */
typedef struct {
	unsigned char opp_flag		;	/* the opp flag */
	unsigned char command		;	/* the user command */
	unsigned short buffer_length 	;	/* the data length */
	unsigned char return_code	;	/* the return code */
	unsigned char misc_Tx_Rx_bits	;	/* miscellaneous transmit and receive bits */
	unsigned char Rx_error_bits	;	/* an indication of a block received with an error */
	unsigned short Rx_time_stamp	;	/* a millisecond receive time stamp */
	unsigned char reserved[7]	;	/* reserved for later use */
	char data[SIZEOF_MB_DATA_BFR]	;	/* the data area */
} MAILBOX_STRUCT;

typedef struct {
	pid_t  pid_num		;
	MAILBOX_STRUCT 	cmdarea	; 
} CMDBLOCK_STRUCT;

/* definitions for setting the configuration */
#define MAXIMUM_BAUD_RATE              112000 /* the maximum permitted baud rate */
#define MIN_BLOCK_LENGTH               2      /* the minimum permitted block length */
#define MAX_BLOCK_LENGTH               2200   /* the maximum permitted block length */
#define MIN_NO_CONSEC_PADs_EOB         1      /* the minimum number of consecutive PADs defining the end-of-block */
#define MAX_NO_CONSEC_PADs_EOB         50     /* the maximum number of consecutive PADs defining the end-of-block */
#define MIN_ADD_LEAD_TX_SYN_CHARS      0      /* the minimum number of additional leading SYN characters */
#define MAX_ADD_LEAD_TX_SYN_CHARS      50     /* the maximum number of additional leading SYN characters */
#define MIN_NO_BITS_PER_CHAR           8      /* the minimum number of bits per character */
#define MAX_NO_BITS_PER_CHAR           8      /* the maximum number of bits per character */

/* definitions for the 'Rx_block_type' */
#define RX_BLOCK_TYPE_1                1
#define RX_BLOCK_TYPE_2                2
#define RX_BLOCK_TYPE_3                3

/* definitions for 'parity' */
#define NO_PARITY                      0  /* no parity */
#define ODD_PARITY                     1  /* odd parity */
#define EVEN_PARITY                    2  /* even parity */

/* definitions for the 'statistics_options' */
#define RX_STATISTICS                  0x0001 /* enable receiver statistics */
#define RX_TIME_STAMP                  0x0002 /* enable the receive time stamp */
#define TX_STATISTICS                  0x0100 /* enable transmitter statistics */

/* definitions for the 'misc_config_options' */
#define STRIP_PARITY_BIT_FROM_DATA     0x0001 /* strip the parity bit from the data for block types 1 & 3 */

/* definitions for the 'modem_config_options' */
#define DONT_RAISE_DTR_RTS_EN_COMMS    0x0001 /* don't raise DTR and RTS when enabling communications */

/* the configuration structure */
typedef struct {
	unsigned long baud_rate			;                 /* the baud rate */
	unsigned long adapter_frequency		;         /* the adapter frequecy */
	unsigned short max_data_length		;          /* the maximum length of a BSC data block */
	unsigned short EBCDIC_encoding 		;          /* EBCDIC/ASCII encoding */
	unsigned short Rx_block_type 		;            /* the type of BSC block to be received */
	unsigned short no_consec_PADs_EOB 	;       /* the number of consecutive PADs indicating the end of the block */
	unsigned short no_add_lead_Tx_SYN_chars ; /* the number of additional leading transmit SYN characters */
	unsigned short no_bits_per_char 	;         /* the number of bits per character */
	unsigned short parity 			;                   /* parity */
	unsigned short misc_config_options 	;      /* miscellaneous configuration options */
	unsigned short statistics_options 	;       /* statistic options */
	unsigned short modem_config_options 	;     /* modem configuration options */
} CONFIGURATION_STRUCT;



/* definitions for reading the communications errors */
/* the communications error structure */
typedef struct {
   unsigned short Rx_overrun_err_count;      /* the number of receiver overrun errors */
   unsigned short BCC_err_count;             /* the number of receiver BCC errors */
   unsigned short nxt_Rx_bfr_occupied_count; /* the number of times a block had to be discarded due to buffering */
                                             /* limitations */
   unsigned short Rx_too_long_int_lvl_count; /* the number of blocks received which exceeded the maximum permitted */
                                             /* block size (interrupt level) */
   unsigned short Rx_too_long_app_lvl_count; /* the number of blocks received which exceeded the configured maximum */
                                             /* block size (application level) */
   unsigned short comms_err_res_Rx;          /* reserved (later use) for a receive statistic */
   unsigned short Tx_underrun_count;         /* the number of transmit underruns */
   unsigned short comms_err_res_Tx;          /* reserved (later use) for a transmit statistic */
   unsigned short DCD_state_change_count;    /* the number of times DCD changed state */
   unsigned short CTS_state_change_count;    /* the number of times CTS changed state */
} COMMUNICATIONS_ERROR_STRUCT;



/* definitions for reading the operational statistics */
/* the operational statistics structure */
typedef struct {
   unsigned long no_blocks_Rx	;         /* the number of data blocks received and made available for the application */
   unsigned long no_bytes_Rx	;          /* the number of bytes received and made available for the application */
   unsigned long no_blocks_Tx	;         /* the number of data blocks transmitted */
   unsigned long no_bytes_Tx	;          /* the number of bytes transmitted */
} OPERATIONAL_STATS_STRUCT;



/* definitions for interrupt usage */
/* 'interrupt_triggers' bit mapping set by a SET_INTERRUPT_TRIGGERS command */
#define INTERRUPT_ON_RX_BLOCK	0x01   /* interrupt when an data block is 
					  available for the application */
#define NO_INTERRUPTS		0x00
#define INTERRUPT_ON_TIMER	0x20

/* interrupt types indicated at 'ptr_interrupt_interface_byte' */
#define RX_INTERRUPT_PENDING           0x01   /* a receive interrupt is pending */

typedef struct {
	char	int_type;
	char 	int_allowed;
} BSC_INT_BYTES;


/* definitions for data block transmission */
/* the setting of the 'misc_Tx_Rx_bits' in the mailbox */
#define TX_BLK_TRANSPARENT             0x01   /* send a transparent text block */
#define ETB_DEFINES_EOB                0x02   /* an ETB character defines the end of this text block */
#define ITB_DEFINES_EOB                0x04   /* an ITB character defines the end of this text block */
#define USR_DEFINES_TX_DATA            0x10   /* the user application defines the data to be transmitted */



/* definitions for the 'Rx_error_bits' in the RECEIVE mailbox */
#define RX_BCC_ERROR                   0x01   /* a receive BCC error occurred */
#define RX_OVERRUN_ERROR               0x02   /* a receive overrun error occurred */
#define RX_EXCESSIVE_LGTH_ERR_INT_LVL  0x10   /* the received block was too long (interrupt level) */
#define RX_EXCESSIVE_LGTH_ERR_APP_LVL  0x20   /* the received block was too long (application level) */

#pragma       pack()

#endif
