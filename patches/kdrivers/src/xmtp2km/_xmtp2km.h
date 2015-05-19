/*
 * _xmtp2km.h
 *
 * Derivation: Copyright (C) 2005 Xygnada Technology, Inc.
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
 * $Id: _xmtp2km.h,v 1.1 2005/12/05 17:48:45 mike01 Exp $
 */

#ifndef __XMTP2KM_H_
#define __XMTP2KM_H_

#ifndef XMTP2KM_MAJOR
#define XMTP2KM_MAJOR 0   /* dynamic major by default */
#endif

#ifndef XMTP2KM_NR_DEVS
#define XMTP2KM_NR_DEVS 1    /* xmtp2km0 through xmtp2km3 */
#endif

struct xmtp2km_dev {
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct cdev cdev;	  /* Char device structure		*/
};

/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		/* low  nibble */


/*
 * The different configurable parameters
 */
extern int xmtp2km_major;     /* main.c */
extern int xmtp2km_nr_devs;
extern int xmtp2km_quantum;
extern int xmtp2km_qset;

extern int xmtp2km_p_buffer;	/* pipe.c */

extern int xmtp2km_ratelimit(void);

/*
 * Prototypes for shared functions
 */
int     xmtp2km_ioctl(struct inode *inode, struct file *filp,
                    unsigned int cmd, unsigned long arg);
/* ioctl definitions */
#include "xmtp2km.h"

#endif /* __XMTP2KM_H_ */
