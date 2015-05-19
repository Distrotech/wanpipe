/****************************************************************************
 *
 * kern_if.h :
 *
 * Protoypes for kernel function wrappers.
 *
 * Copyright (C) 2004  Xygnada Technology, Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 ****************************************************************************/
/* protoypes of functions in the public part of the xmtp2km kernel module */

#ifndef __XMTP2_KERN_IF__
#define __XMTP2_KERN_IF__


enum {
	XMTP2_VERIFY_READ,
	XMTP2_VERIFY_WRITE
};

void * xmtp2km_kmalloc (const unsigned int bsize);
void xmtp2km_kfree (void * p_buffer);
int xmtp2km_access_ok   (int type, const void *p_addr, unsigned long n);
unsigned long xmtp2km_copy_to_user   (void *p_to, const void *p_from, unsigned long n);
unsigned long xmtp2km_copy_from_user (void *p_to, const void *p_from, unsigned long n);
int xmtp2km_strncmp (const char *p_s1, const char *p_s2, const unsigned int n);
void * xmtp2km_memset(void * p_mb, const int init_value, const unsigned int n);
void * xmtp2km_memcpy(void * p_to, const void * p_from, const unsigned int n);
void   xmtp2_printk(const char * fmt, ...);
void* xmtp2_memset(void *b, int c, int len);

#endif
