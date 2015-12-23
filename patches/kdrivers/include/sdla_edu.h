/*************************************************************************
 * sdla_edu.h	Sangoma Educatonal Kit definitions
 *
 * Author:     	David Rokhvarg <davidr@sangoma.com>	
 *
 * Copyright:	(c) 1984-2004 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the term of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
===========================================================================
 *
 * Sep 18, 2001  David Rokhvarg  Initial version.
 *
 * Jul 30, 2003  David Rokhvarg  v 1.0
 * 	1. sdla_load() is not used anymore,
 * 	   all the loading is done from user mode instead.
 * 	   All IOCTLs used for loading are not valid anymore.
 * 	2. New IOCTLs added for starting and stopping
 * 	   the card.
 * 	 
 * Nov 17, 2004  David Rokhvarg  v 2.0
 * 	1. Removed globals and put into private area
 * 	2. New IOCTLs added for TCP/IP handling.
 *
===========================================================================
 Organization
	- Constants defining the shared memory control block (mailbox)
	- IOCTL codes
	- Structures for performing IOCTLs

*************************************************************************/

#ifndef _SDLA_EDU_H
# define _SDLA_EDU_H

/*------------------------------------------------------------------------
   Notes:

	Compiler	Platform
	------------------------
	GNU C		Linux

------------------------------------------------------------------------*/

enum SDLA_COMMAND{

	SDLA_CMD_READ_BYTE		= 10,
	SDLA_CMD_WRITE_BYTE,
	
	SDLA_CMD_READ_PORT_BYTE,
	SDLA_CMD_WRITE_PORT_BYTE,
	
	SDLA_CMD_READ_BLOCK,
	SDLA_CMD_WRITE_BLOCK,
	
	SDLA_CMD_GET_CARDS,
	SDLA_CMD_REGISTER,
	SDLA_CMD_DEREGISTER,
	SDLA_CMD_PREPARE_TO_LOAD,
	SDLA_CMD_LOAD_COMPLETE,
	SDLA_CMD_ZERO_WINDOW,

	SDLA_CMD_START_S514,
	SDLA_CMD_STOP_S514,

	SDLA_CMD_IS_TX_DATA_AVAILABLE,
	SDLA_CMD_COPLETE_TX_REQUEST,

	SDLA_CMD_INDICATE_RX_DATA_AVAILABLE,
	SDLA_CMD_GET_PCI_ADAPTER_TYPE
};

typedef struct edu_exec_cmd{
	unsigned char  ioctl;                   // the ioctl code
	unsigned char  return_code;
	unsigned short buffer_length;           // the data length
	unsigned long  offset;			// offset on the card
}edu_exec_cmd_t;
 
#define MAX_TCP_IP_DATA_LEN	800L
#define MAX_TX_RX_DATA_SIZE	2000
typedef struct{
	//caller's card number. only card 0 can do IP transactions.
	unsigned char	cardnumber;
	//operation status
	unsigned char	status;
	//length of data in 'data'
	unsigned long	buffer_length;
	unsigned char	data[MAX_TX_RX_DATA_SIZE];
}TX_RX_DATA;

//
//possible 'status' values in 'TX_RX_DATA' structure on completion of:
//	1.	SDLA_CMD_COPLETE_TX_REQUEST
//	2.	SDLA_CMD_IS_TX_DATA_AVAILABLE
//	3.	SDLA_CMD_INDICATE_RX_DATA_AVAILABLE
//
enum TX_RX_STATUS{
	TX_SUCCESS = 1,
	TX_ERR_PROTOCOL_DOWN,
	TX_ERR_HARDWARE,
	TX_RX_REQUEST_INVALID
};

#if !defined(BUILD_FOR_UNIX)
#define	IOCTL_SDLA		CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
	unsigned char cardnumber;
	unsigned long offset;
	unsigned char value;
} XFER_BYTE_CONTROL;

#define	MAX_CARDS	1	//card 0 and card 1 only

/* structure used for reading and writing blocks */
typedef struct {
	void*		buffer;
	unsigned short 	num_bytes;
	unsigned long	offset;
	unsigned char	card_number;
} SDLA_BLOCKOP;

typedef struct {
	unsigned char	cardnumber;
	unsigned char 	rcode;
} CARD_TRANSACTION;


#define MAX_IOCTL_DATA_LEN	4096
typedef struct{

	unsigned long	command;
	unsigned long	buffer_length;
  	unsigned char	return_code;
	unsigned char	data[MAX_IOCTL_DATA_LEN];
}SDLA_CTL_BLOCK;


typedef struct{

	void* ndis_adapter;

}SDLA_DEV_EXTENSION;

//RESULT MESSAGES

#define	SDLAR_NOT_REGISTERED	0L
#define	SDLAR_DEREGISTERED	1L
#define	SDLAR_RELEASED		2L
#define	SDLAR_REGISTERED        1L
#define	SDLAR_ALLOCATED		2L

#define	SDLAR_CARD_BLOCKED	0L
#define	SDLAR_PREPARED		2L
#define	SDLAR_NOT_PREPARED	0L
#define	SDLAR_ERROR_PREPARING	3L

#define DRV_NAME	"EduKit"

#endif 	//#if !defined(BUILD_FOR_UNIX)

#endif	//_SDLA_EDU_H


