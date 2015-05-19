/*****************************************************************************
 * libhpsangoma.h : Sangoma High Performance TDM API - Span Based Library
 *
 * Author(s):	Nenad Corbic <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 2008 Nenad Corbic <ncorbic@sangoma.com>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 */

#ifndef _LIB_HP_SANGOMA_PRIV_H
#define _LIB_HP_SANGOMA_PRIV_H

#include "libhpsangoma.h"


/*!
  \brief library print function based on registered span callback function
  \return void
 */
#define lib_printf(level, fp, fmt, ...) if (lib_log) lib_log(level, fp, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

extern sangoma_hptdm_log_func_t lib_log;


/*---------------------------------------------------------
 * PRIVATE - FUNCTIONS
 */

/*!
  \brief Span read data from a hw interface
  \param fd socket file descriptor to span hw interface
  \param hdrbuf pointer to header buffer used for misc oob data - not used
  \param hdrlen size of hdr buffer - always standard 16 bytes
  \param databuf pointer to span data that includes all timeslots * chunk size
  \param datalen  size of span data: timeslots * chunk size
  \param flag  selects normal (0) or oob (MSG_OOB) read queue
  \return 0 = pass, non zero fail
 */
int sangoma_readmsg_hp_tdm(int fd, void *hdrbuf, int hdrlen, void *databuf, int datalen, int flag);


/*!
  \brief Span write data into a hw interface
  \param fd socket file descriptor to span hw interface
  \param hdrbuf pointer to header buffer used for misc oob data - not used
  \param hdrlen size of hdr buffer - always standard 16 bytes
  \param databuf pointer to span data that includes all timeslots * chunk size
  \param datalen  size of span data: timeslots * chunk size
  \param flag  selects normal (0) or oob (MSG_OOB) read queue
  \return 0 = pass, non zero fail
 */
int sangoma_writemsg_hp_tdm(int fd, void *hdrbuf, int hdrlen, void *databuf, unsigned short datalen, int flag);

/*!
  \brief Execute a management ioctl command
  \param span span object
  \return 0 = pass, non zero fail
 */
int do_wanpipemon_cmd(sangoma_hptdm_span_t *span);


/*!
  \brief Get full span hw configuration
  \param span span object
  \return 0 = pass, non zero fail
 */
unsigned char sangoma_get_cfg(sangoma_hptdm_span_t *span);


/*!
  \brief Open socket to span hw interface
  \param span span object
  \return 0 = pass, non zero fail
 */
int sangoma_hptdm_span_open(sangoma_hptdm_span_t *span);


/*!
  \brief Read oob event from hw and push oob event up to the user. Called after select()
  \param span span object
  \return 0 = pass, non zero fail
 */
int sangoma_hp_tdm_handle_oob_event(sangoma_hptdm_span_t *span);


/*!
  \brief Multiplex span rx data and push it up to each channel
  \param span span object
  \return 0 = pass, non zero fail
 */
int sangoma_hp_tdm_push_rx_data(sangoma_hptdm_span_t *span);


/*!
  \brief Read data from hw interface and pass it up to the rxdata handler:  called by run_span()
  \param span span object
  \return 0 = pass, non zero fail
 */
int sangoma_hp_tdm_handle_read_event(sangoma_hptdm_span_t *span);


/*!
  \brief Pull data from all chans and write data to hw interface: called by run_span()
  \param span span object
  \return 0 = pass, non zero fail
 */
int sangoma_hp_tdm_handle_write_event(sangoma_hptdm_span_t *span);




#endif

