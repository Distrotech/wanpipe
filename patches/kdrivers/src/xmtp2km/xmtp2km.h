/*
 * xmtp2km.h -- definitions for the char module
 *
 * Derived work: Copyright (C) 2005 Xygnada Technology, Inc.
 * Derived from the original work as follows:
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#ifndef _XMTP2KM_H_
#define _XMTP2KM_H_

/* needed for the _IOW etc stuff used later */
#include <linux/ioctl.h> 

/*
 * Ioctl definitions
 */

/* Use '7' as magic number */
#define XMTP2KM_IOC_MAGIC  '7'

#define XMTP2KM_IOCRESET    _IO(XMTP2KM_IOC_MAGIC, 0)

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define XMTP2KM_IOCS_BINIT 	_IOW (XMTP2KM_IOC_MAGIC,  1, uint8_t *)
#define XMTP2KM_IOCS_OPSPARMS 	_IOW (XMTP2KM_IOC_MAGIC,  2, uint8_t *)
#define XMTP2KM_IOCS_PWR_ON 	_IOW (XMTP2KM_IOC_MAGIC,  3, uint8_t *)
#define XMTP2KM_IOCS_EMERGENCY 	_IOW (XMTP2KM_IOC_MAGIC,  4, uint8_t *)
#define XMTP2KM_IOCS_EMERGENCY_CEASES 	_IOW (XMTP2KM_IOC_MAGIC,  5, uint8_t *)
#define XMTP2KM_IOCS_STARTLINK 	_IOW (XMTP2KM_IOC_MAGIC,  6, uint8_t *)
#define XMTP2KM_IOCG_GETMSU 	_IOR (XMTP2KM_IOC_MAGIC,  7, uint8_t *)
#define XMTP2KM_IOCS_PUTMSU 	_IOW (XMTP2KM_IOC_MAGIC,  8, uint8_t *)
#define XMTP2KM_IOCX_GETTBQ 	_IOWR(XMTP2KM_IOC_MAGIC,  9, uint8_t *)
#define XMTP2KM_IOCS_LNKRCVY 	_IOW (XMTP2KM_IOC_MAGIC,  10, uint32_t)
#define XMTP2KM_IOCX_GETBSNT 	_IOWR(XMTP2KM_IOC_MAGIC,  11, uint8_t *)
#define XMTP2KM_IOCS_STOPLINK 	_IOW (XMTP2KM_IOC_MAGIC,  12, uint8_t *)
#define XMTP2KM_IOCG_GETOPM 	_IOR (XMTP2KM_IOC_MAGIC,  13, uint8_t *)
#define XMTP2KM_IOCS_STOP_FAC 	_IOR (XMTP2KM_IOC_MAGIC,  14, uint8_t *)

/*
 * The other entities only have "Tell" and "Query", because they're
 * not printed in the book, and there's no need to have all six.
 * (The previous stuff was only there to show different ways to do it.
 */
/* ... more to come */
#define XMTP2KM_IOCHARDRESET _IO(XMTP2KM_IOC_MAGIC, 15) /* debugging tool */

#define XMTP2KM_IOC_MAXNR 15

#endif
