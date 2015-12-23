/*****************************************************************************
* aft_tdm_hp.c: Example code for HP TDM API Library
*
* Author(s):    Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
*/

#ifndef __AFT_TDM_HP_H_
#define __AFT_TDM_HP_H_

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <sys/mman.h>
#include <syslog.h>

#include <linux/wanpipe.h>
#include <libsangoma.h>
#include <libhpsangoma.h>

/*! Define maximum spans and chans */
#define MAX_SPANS 16
#define MAX_CHANS 31

/*! Use the syslog in debug output */
#define USE_SYSLOG 1

/*! Set the pthread stack to something normal */
#define MGD_STACK_SIZE 1024 * 240

void __log_printf(int level, FILE *fp, char *file, const char *func, int line, char *fmt, ...);

#define ysleep(usec) sched_yield() ; usleep(usec);
#define log_printf(level, fp, fmt, ...) __log_printf(level, fp, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/*!
 * Indicates that sytem is running. If not set stop program.
 * Set the pthread stack to something normal */
enum {
	SYSTEM_RUNNING=1,
};

/*!
 * Span index structure. Defines the program span structure along with
 * supporting variables needed to operate span
 */
typedef struct span_idx
{
	/*! init flag used to determine if span is being used*/
	int init;
	/*! span number - integer starting from 0 */
	int span_no;
	/*! pthread id - because each span will have its own thread */
	pthread_t thread;
	/*! span object - returned by sangoma_hptdm_api_span_init() function */
	sangoma_hptdm_span_t *span;

	/* Other private span variables insert here */
} span_idx_t;


/*!
 * Channel index structure. Defines the program channel structure along with
 * supporting variables needed to operate a chan.
 */
typedef struct chan_idx
{
	/*! init flag used to determine if span is being used*/
	int init;
	/*! channel number - integer starting from 0 */
	int chan_no;
	/*! span number - owner of the current channel - integer starting from 0 */
	int span_no;
	/*! pthread id for this channel, because every channel will have its own thread */
	pthread_t thread;

	/*! channel object obtaind by span->open_chan() method */
	sangoma_hptdm_chan_t *chan;

	/* Other private channel variables insert here */
} chan_idx_t;


/*! Miscallaneous bit utilities -
 *  not thread safe should use pthread lock arount bit set/clear operations
 */

#define aft_test_flag(p,flag)		({ \
					(p & (flag)); \
					})

#define aft_set_flag(p,flag)		do { \
					(p |= (flag)); \
					} while (0)

#define aft_clear_flag(p,flag)		do { \
					(p &= ~(flag)); \
					} while (0)


#endif


