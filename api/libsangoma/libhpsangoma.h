/*****************************************************************************
 * libhpsangoma.h : Sangoma High Performance TDM API - Span Based Library
 *
 * Author(s):	Nenad Corbic <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 2005-2008 Nenad Corbic <ncorbic@sangoma.com>
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ============================================================================
 */

#ifndef _LIB_HP_SNAGOMA_H
#define _LIB_HP_SNAGOMA_H

#include "libsangoma.h"

#ifdef WIN32

#error "WINDOWS NOT DEFINED"

#else
/* L I N U X */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <poll.h>
#include <signal.h>


#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_common.h>
#include <linux/wanpipe_cfg.h>
#include <linux/wanpipe.h>
#include <linux/if_wanpipe.h>
#include <linux/wanpipe_api.h>
#endif

#define SMG_HP_MAX_CHAN_DATA 1024
#define SMG_HP_TDM_CHUNK_IDX_SZ 16
#define SMG_HP_TDM_MAX_CHANS 31
#define SMG_HP_MAX_SPAN_DATA (31*160)+32

#define hp_tdmapi_rx_event_t wp_api_hdr_t
#define hp_tdmapi_tx_event_t wp_api_hdr_t

#define SANGOMA_HPTDM_VERSION 2

/*---------------------------------------------------------
 * PUBLIC DEFINITIONS
 */

typedef void (*sangoma_hptdm_log_func_t)(int level, FILE *fp, const char *file, const char *func, int line, const char *fmt, ...);

/*!
  \struct sangoma_hptdm_span_reg_t
  \brief Span registration structure
 */
typedef struct sangoma_hptdm_span_reg
{
	/*! pointer to user object used with callback functions */
	void *p;
	/*! callback function to implement library logging */
	sangoma_hptdm_log_func_t log;
	/*! callback function to span global events for all channels in a span */
	int (*rx_event)(void *p, hp_tdmapi_rx_event_t *data);
}sangoma_hptdm_span_reg_t;


/*!
  \struct hp_tmd_chunk_t
  \brief A chunk structure used to implement a chunk buffer
 */
typedef struct hp_tdm_chunk
{
	/*! chunk init flag used to determine if chunk is being used*/
	int init;
	/*! length of the current data chunk */
	int len;
	/*! current offset to write/read to/from the chunk data structure. */
	int offset;
	/*! chunk data location  */
	char data[SMG_HP_MAX_CHAN_DATA];
}hp_tmd_chunk_t;

/*!
  \struct sangoma_hptdm_chan_reg_t
  \brief Chan registration structure
 */
typedef struct sangoma_hptdm_chan_reg
{
	/*! pointer to user object used with callback functions */
	void *p;
	/*! callback function used to pass rx chunk to user application */
	int (*rx_data)(void *p, char *data, int len);
	/*! callback function used to pass channel specific event to user application */
	int (*rx_event)(void *p, hp_tdmapi_rx_event_t *data);

}sangoma_hptdm_chan_reg_t;

/*!
  \struct sangoma_hptdm_chan_t
  \brief Channel structure, describes a single timeslot/channel in a span.
 */
typedef struct sangoma_hptdm_chan
{
	/*! channel init flag used to determine if chan is being used */
	int init;

	/*! channel number: starting from 0 */
	int chan_no;

	/*! span number the current channel belongs to:  starting from 0 */
	int span_no;

	/*! span object pointer the current channel belongs */
	void *span;

	/*! Rx chunk buffer used to rx data from the span */
	hp_tmd_chunk_t rx_chunk;

	/*! Circular buffer of tx data chunks used to tx data to span */
	hp_tmd_chunk_t tx_idx[SMG_HP_TDM_CHUNK_IDX_SZ];

	/*! Circular buffer index for user to chan tx */
	int tx_idx_in;

	/*! Circular buffer index for chan to span tx */
	int tx_idx_out;

	/*! Callback func called by user to push chunk of data into the channel */
	int (*push)(struct sangoma_hptdm_chan *, char *data, int len);

	/*! Channel registration struct used to store user data, cfg and callbacks */
	sangoma_hptdm_chan_reg_t chan_reg;

}sangoma_hptdm_chan_t;

/*!
  \struct sangoma_hptdm_chan_map_t
  \brief Structure describing a array index of a channel inside the span structure.
 *
 * Structure describing a array index of a channel inside the span structure.
 * Furthermore the index * also servers as a map from hardware channels to library channels.
 * Hardware might be configured for channels 1-15.17-31, however library always
 * provides all 31 channels
 */
typedef struct sangoma_hptdm_chan_map
{
	/*! A hardware channel number that is mapped to the current channel structure.
	 *  Hardware might be configured for channels 1-15.17-31, however library always
	 *  provides all 31 channels */
	int chan_no_hw;

	/*! A channel structure  */
	sangoma_hptdm_chan_t chan;

}sangoma_hptdm_chan_map_t;


/*!
  \struct sangoma_hptdm_span_t
  \brief Span structure. Structure describing a single span.
 */
typedef struct sangoma_hptdm_span
{
	/*! span init flag used to determine if span is being used */
	int init;

	/*! span number - integer starting from 0 */
	int span_no;

	/*! span hw interface name to which span is bounded to */
	char if_name[100];

	/*! span socket file descriptor used to rx/tx data to and from hw interface */
	int sock;

	/*! waitable object for the socket */
	sangoma_wait_obj_t *waitobj;

	/*! chunk size for each channel inside the span */
	int chunk_sz;

	/*! total number of channels configured in the span */
	int max_chans;

	/*! total tx data size to hw interface. tx_size = max_chans * chunk_sz  */
	int tx_size;

	/*! idle flag used to fill an unused channel  */
	unsigned char idle;

	/*! bit map of configured timeslots obtained from hw interface  */
	unsigned int timeslot_cfg;

	/*! hw data encoding: ULAW/ALAW obtained from hw interface  */
	unsigned int hwcoding;

	/*! array of maximum number of channels in a span  */
	sangoma_hptdm_chan_map_t chan_idx[SMG_HP_TDM_MAX_CHANS];

	/*! span rx data block: used to receive a block of data from hw interface: recv()  */
	char rx_data[SMG_HP_MAX_SPAN_DATA];

	/*! span rx data len, obtained after a recv() call to hw interface  */
	int rx_len;

	/*! span tx data block: used to transmit a block of data to hw interface: send() */
	char tx_data[SMG_HP_MAX_SPAN_DATA];

	/*! span tx data block len: passed to send() function */
	int tx_len;

	/*! span registration functions: contains user callback functions */
	sangoma_hptdm_span_reg_t span_reg;

	/*! span config structure obtained from hw interface via managment ioctl call. */
	wan_if_cfg_t span_cfg;

	/*! tdm_api event structure used to obtain TDM API Events */
	wanpipe_api_t tdm_api;

	/*! span managment structure used to execute mgmnt ioctl commands to hw interface */
	wan_udp_hdr_t wan_udp;

	/*! \brief Method: open a channel inside a span
	 *  \param span span object
	 *  \param chan_reg channel registration structure: callbacks and cfg
	 *  \param chan_no channel number - integer starting from 0
	 *  \param chan_ptr user container for channel object passed up to the user.
	 */
	int (*open_chan)(struct sangoma_hptdm_span *span,
			      sangoma_hptdm_chan_reg_t *chan_reg,
			      unsigned int chan_no,
			      sangoma_hptdm_chan_t **chan_ptr);

	/*! \brief Method: close a channel inside the span
	 *  \param chan chan object
	 */
	int (*close_chan)(sangoma_hptdm_chan_t *chan);

	/*! \brief Method: check if channel is closed
	 *  \param chan chan object 	 */
	int (*is_chan_closed)(sangoma_hptdm_chan_t *chan);

	/*! \brief Method: run main span execution logic: rx/tx/oob
	 *  \param span span object
	 *
	 *  Run main span execution logic. This function peforms all socket operations
	 *  on a hw interface. Rx/Tx/Oob.
	 *  Receives data and demultiplexes it to channels.
	 *  Receives oob data and passes user events global to all channels.
	 *  Multiplexes all channel tx data into a single tx data block and
	 *  passes it to hw iface.
	 */
	int (*run_span)(struct sangoma_hptdm_span *span);

	/*! \brief Method: close span
	 *  \param span span object */
	int (*close_span)(struct sangoma_hptdm_span *span);

	/*! \brief Method: used by user app to execute events on current span
	 *  \param span span object
	 *  \param event event object
	 */
	int (*event_ctrl)(struct sangoma_hptdm_span *span, hp_tdmapi_tx_event_t *event);

	/*! \brief Method: request full span configuration from a current span
	 *  \param span span object
	 *  \param cfg configuratoin object to be filled by span
	 */
	int (*get_cfg)(struct sangoma_hptdm_span *span, wan_if_cfg_t *cfg);


}sangoma_hptdm_span_t;

/*---------------------------------------------------------
 * PUBLIC FUNCTIONS
 */

/*!
  \def sangoma_hptdm_api_span_init(span,cfg)
  \brief Initialize and Configure a Span
  \param span_no span number - integer
  \param cfg span registration struct
  \return NULL: fail,  Span Object: pass
 */

/* Initialize and Configure a Span  */
#define sangoma_hptdm_api_span_init(span,cfg) __sangoma_hptdm_api_span_init(span, cfg, SANGOMA_HPTDM_VERSION);

/*!
  \fn sangoma_hptdm_span_t * __sangoma_hptdm_api_span_init(int span_no, sangoma_hptdm_span_reg_t *cfg, int version)
  \brief Initialize and Configure Span - private functions not to be used directly!
  \param span_no span number - integer
  \param cfg span registration struct
  \param version library version number added by the macro
  \return NULL: fail,  Span Object: pass
 *
 * The __sangoma_hptdm_api_span_init() function must NOT be called directly!
 *  One MUST use defined sangoma_hptdm_api_span_init() macro instead
 */

 /*
  * The __sangoma_hptdm_api_span_init() function must NOT be called directly!
  * One MUST use defined sangoma_hptdm_api_span_init() macro instead
  */
extern sangoma_hptdm_span_t * __sangoma_hptdm_api_span_init(int span_no, sangoma_hptdm_span_reg_t *cfg, int version);


/*!
  \fn int sangoma_hptdm_api_span_free(sangoma_hptdm_span_t *span)
  \brief Free, Un-Initialize Span
  \param span_no span object
 \return 0 = pass, non zero fail
 */
extern int sangoma_hptdm_api_span_free(sangoma_hptdm_span_t *span);





#endif

