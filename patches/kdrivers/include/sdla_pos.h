/*****************************************************************************
* POSAPI.H - Sangoma POS Adapter. Application Program Interface definitions.
* (c) Sangoma Technologies Inc., 1993-1994
* ----------------------------------------------------------------------------
* Date:         Revision:                                       	By:
* May 25, 1994  2.32 - CONFIGSTRUC has been changed. If half_duplex is
*                      non-zero then half-duplex ASYNCH mode is
*                      selected. Zero selects full-duplex mode. Default
*                      mode is half-duplex (defined as DFT_HALFDUPLEX). EK
*
* Jan.19, 1994  2.10 - POSSETUPSTRUC has been changed to accomodate     EK
*                      2-byte address field.
*
* Jan.11, 1994  2.00 - Initial.                                 	EK
*****************************************************************************/

/****** GENERAL DATA TYPES DEFINITIONS **************************************/

#ifndef __SDLA_POS_H_
#define __SDLA_POS_H_

#pragma pack(1)

/*----- General Purpose Constants ------------------------------------------*/
#define TRUE            1       /* values for BOOL data type */
#define FALSE           0
#ifndef NULL
#define NULL		0
#endif

/*----- POS and ASYNC Mode Definitions -------------------------------------*/
#define SDLC_ALL        0       /* Pass all received frames */
#define SDLC_INFO       1       /* Pass all I-frames (default) */
#define SDLC_SELECT     2       /* Pass only I-frames with matching address */
#define ASYNC_4800      0       /* RS485 line speed:  4800 bps */
#define ASYNC_9600      1       /* RS485 line speed:  9600 bps */
#define ASYNC_19200     2       /* RS485 line speed: 19200 bps */
#define ASYNC_38400     3       /* RS485 line speed: 38400 bps */
#define ASYNC_MAXSPEED  ASYNC_38400
#define PRIMARY_ATTR    0       /* frame belongs to primary station */
#define SECONDARY_ATTR  1       /* frame belongs to secondary station */

/*----- Default Configuration and Setup Parameters -------------------------*/
#define WINDOW_SIZE     0x2000  /* size of shared memory window */
#define TIME_OUT        2       /* board response time-out, sec */
#define MAX_DATA        1030    /* maximum I-frame data field length */
#define MIN_DATA        128     /* minimum I-frame data field length */
#define DFT_POSDATA     265     /* default I-frame data field length */
#define DFT_SDLCLINES   1       /* default number of monitored POS lines */
#define DFT_SDLCMODE    SDLC_INFO
#define DFT_ASYNCLINES  1       /* default number of monitored asynch lines */
#define DFT_ASYNCSPEED  ASYNC_19200
#define DFT_HALFDUPLEX  TRUE    /* default asynch mode */

/*----- Mailbox interface constants ----------------------------------------*/
#define CLEAR           0x00    /* no activity */
#define EXE_CMD         0x01    /* execute mailbox command flag */
#define SDLC1_RXRDY     0x01    /* bit 0 - POS#1 frames available */
#define SDLC2_RXRDY     0x02    /* bit 1 - POS#2 frames available */
#define ASYNC1_RXRDY    0x01    /* bit 0 - ASYNC#1 data available */
#define ASYNC1_TXRDY    0x02    /* bit 1 - ASYNC#1 transmit buffer empty */
#define ASYNC2_RXRDY    0x04    /* bit 2 - ASYNC#2 data available */
#define ASYNC2_TXRDY    0x08    /* bit 3 - ASYNC#2 transmit buffer empty */

/*----- POS Commands -------------------------------------------------------*/
#define CONFIGURE       0x01    /* Configure POS                            */
#define SEND_ASYNC      0x02    /* Send block of data through ASYNC port    */
#define RECEIVE_ASYNC   0x03    /* Receive block of bata through ASYNC port */
#define ENABLE_POS      0x04    /* Start listening on a POS loop            */
#define DISABLE_POS     0x05    /* Stop listening on a POS loop             */
#define RECEIVE_POS     0x06    /* Receive a frame buffered from POS loop   */
#define READ_ERR_STATS  0x07    /* Retrieve POS communic. error statistics  */
#define FLUSH_ERRORS    0x08    /* Reset all error counters                 */
#define SEND_RECV_STATE 0x09    /* Get transmit/receive buffers state       */
#define POS_SETUP       0x0A    /* Set POS loop monitoring mode             */
#define RESET_CARD      0x0B    /* Clear all buffers, set all default modes */

/*----- POS Return Codes ---------------------------------------------------*/
#define POS_OK          0x00    /* Command executed successfully            */
#define POS_BADPORT     0x01    /* Invalid port number                      */
#define POS_WASDONE     0x02    /* Command has already been executed        */
#define POS_DISABLED    0x03    /* Port is disabled                         */
#define POS_DENIED      0x33    /* Board temporarily unaccessable           */
#define INVALID_CMD     0x80    /* Unrecognized command code                */
#define INVALID_PAR     0x81    /* Invalid parameter value                  */
#define POS_FAULT       0x0A    /* Board hung                               */
#define POS_INIT_OK     0xF0    /* Board initialized                        */

#define BYTE 	unsigned char
#define WORD	unsigned short
/****** DATA STRUCTURES *****************************************************/

/*----- Command Control Block ------------------------------------------------
 * This structure is used for microcode command execution.
 *--------------------------------------------------------------------------*/

typedef struct {
  BYTE  command;                /* disp.00h: Command code                   */
  WORD  buf_len;                /* disp.01h: Length of data buffer          */
  BYTE  ret_code;               /* disp.03h: Result of previous command     */
  BYTE  port_num;               /* disp.04h: Port number (0 - general)      */
  BYTE  prim_sec;               /* disp.05h: Primary/Secondary attribute    */
  BYTE  reserved [10];          /* disp.06h .. 0Fh: reserved for later use  */
  BYTE  data [MAX_DATA];        /* disp.10h: data transfer buffer           */
} PIPCBSTRUC;


/*----- Mailbox Structure ---------------------------------------------------
 * This structure is used for microcode application program interface.
 *--------------------------------------------------------------------------*/

typedef struct {
  BYTE          exe_flag;       /* disp.00h: '1' - enable command execution */
  BYTE          sdlc_flags;     /* disp.01h: POS SDLC channels status       */
  BYTE          async_flags;    /* disp.02h: ASYNC channels status          */
  BYTE          reserved [13];  /* disp.03 .. 0Fh - reserved for later use  */
  PIPCBSTRUC    pipcb;          /* disp.10h: PIP Control Block              */
} POSMBSTRUC;


/*----- Card Configuration Data Block ----------------------------------------
 * Used for CONFIGURE command.
 *--------------------------------------------------------------------------*/

typedef struct {
  BYTE  sdlc_lines;             /* disp.00h: number of active SDLC lines    */
  WORD  sdlc_maxdata;           /* disp.01h: maximum length of the I-frame  */
  BYTE  async_lines;            /* disp.03h: number of active ASYNC lines   */
  BYTE  async_speed;            /* disp.04h: asynchronous line(s) speed     */
  BYTE  half_duplex;            /* disp.05h: half/full-duplex configuration */
} CONFIGSTRUC;


/*----- SDLC Communications Error Statistics Data Block ----------------------
 * Used for READ_ERR_STATS command.
 *--------------------------------------------------------------------------*/

typedef struct {
  WORD  overrun_err;            /* disp.00h: receiver overrun error count   */
  WORD  CRC_err;                /* disp.02h: receiver CRC error count       */
  WORD  abort_err;              /* disp.04h: aborted frames count           */
  WORD  lost_err;               /* disp.06h: buffer overflow error count    */
  WORD  excess_err;             /* disp.08h: excessive frame length count   */
} ERRSTATSTRUC;


/*----- POS Send/Receive State Data Block ------------------------------------
 * Used for SEND_RECV_STATE command.
 *--------------------------------------------------------------------------*/

typedef struct {
  WORD  POS1_received;          /* disp.00h: POS#1 queued frames count      */
  WORD  POS2_received;          /* disp.02h: POS#2 queued frames count      */
  WORD  async1_recvd;           /* disp.04h: bytes in ASYNC#1 receive buffer*/
  WORD  async1_transm;          /* disp.06h: free in ASYNC#1 transmit buffer*/
  WORD  async2_recvd;           /* disp.08h: bytes in ASYNC#2 receive buffer*/
  WORD  async2_transm;          /* disp.0Ah: free in ASYNC#2 transmit buffer*/
} POSSTATESTRUC;


/*----- POS Setup Data Block -------------------------------------------------
 * Used for POS_SETUP command.
 *--------------------------------------------------------------------------*/

typedef struct {
  BYTE  mode;                   /* disp.00h: POS operation mode */
  WORD  addr;                   /* disp.01h: address to search for */
} POSSETUPSTRUC;

#pragma pack()


/**************************************************************
 *
 * TEMPLATE
 * 
 *************************************************************/

#define CONFIGURATION_STRUCT		CONFIGSTRUC

#define WANCONFIG_FRMW			WANCONFIG_ATM	

#define OK				0
#define COMMAND_OK			OK

enum {
	ROUTER_UP_TIME = 0x50,
	FT1_READ_STATUS,
	ENABLE_TRACING,	
	FT1_MONITOR_STATUS_CTRL,
	ENABLE_READ_FT1_STATUS,
	ENABLE_READ_FT1_OP_STATS
};

#define DCD_HIGH			0x08
#define CTS_HIGH			0x20

#define MIN_WP_PRI_MTU 		1500
#define MAX_WP_PRI_MTU 		MIN_WP_PRI_MTU 
#define DEFAULT_WP_PRI_MTU 	MIN_WP_PRI_MTU

#define MIN_WP_SEC_MTU 		1500
#define MAX_WP_SEC_MTU 		MIN_WP_SEC_MTU 
#define DEFAULT_WP_SEC_MTU 	MIN_WP_SEC_MTU


/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   		0x01
#define TMR_INT_ENABLED_UPDATE		0x02
#define TMR_INT_ENABLED_CONFIG		0x10
#define TMR_INT_ENABLED_TE		0x20

#define SIOC_WANPIPE_EXEC_CMD		SIOC_WANPIPE_DEVPRIVATE
#define SIOC_WANPIPE_POS_STATUS_CMD	SIOC_WANPIPE_DEVPRIVATE+1

#ifdef __KERNEL__

#define PRI_BASE_ADDR_MB_STRUCT		0xE000

#endif
#endif
