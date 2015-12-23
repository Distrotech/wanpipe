/***************************************************************************//**
 * \file libhpsangoma.c
 * \brief Sangoma High Performance TDM API - Span Based Library
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
 * *******************************************************************************
 */

#include "libhpsangoma.h"
#include "libhpsangoma_priv.h"

/*********************************************************//**
  PRIVATE STRUCTURES
 *************************************************************/

sangoma_hptdm_log_func_t lib_log = NULL;

/*!
  \fn int sangoma_hp_tdm_chan_push(struct sangoma_hptdm_chan *chan, char *data, int len)
  \brief Channel Method: User tx a chunk into a channel
  \param chan channel object
  \param data pointer to user voice chunk
  \param len size of user voice chunk
  \return -1 = packet too large, -2 = channel closed, 1 = busy, 0 = tx ok
*/

static int sangoma_hp_tdm_chan_push(struct sangoma_hptdm_chan *chan, char *data, int len)
{
	hp_tmd_chunk_t *tx_chunk;
	int free_slots;

	if (!chan->init) {
		return -2;
	}

	if (len >= SMG_HP_MAX_CHAN_DATA) {
		/* Data Too Big */
		lib_printf(0,NULL,"chan_push c%i tx chunk len %i too big\n",
				chan->span_no+1,chan->chan_no+1,chan->tx_idx_in,len);
		return -1;
	}

	/* Lock */
	if (chan->tx_idx_in >= chan->tx_idx_out) {
		free_slots=SMG_HP_TDM_CHUNK_IDX_SZ-(chan->tx_idx_in-chan->tx_idx_out);
	} else {
		free_slots=chan->tx_idx_out-chan->tx_idx_in;
	}
	/* Un Lock */

	if (!free_slots) {
		/* We have just overruned the tx buffer */
		lib_printf(0,NULL,"chan_push c%i failed no free slots in %i out %i\n",
				chan->span_no+1,chan->chan_no+1, chan->tx_idx_in,chan->tx_idx_out);
		return 1;
	}

	tx_chunk = &chan->tx_idx[chan->tx_idx_in];
	if (tx_chunk->init) {
		/* This should NEVER happen the chunk should be free */
		lib_printf(15,NULL,"chan_push s%ic%i tx chunk overrun in %i \n",
				chan->span_no+1,chan->chan_no+1,chan->tx_idx_in);
		return 1;
	}

	memset(tx_chunk,0,sizeof(hp_tmd_chunk_t));
	memcpy(&tx_chunk->data,data,len);
	tx_chunk->len=len;
	tx_chunk->init=1;

	lib_printf(15,NULL,"chan_push s%ic%i tx chunk in %i \n",
				chan->span_no+1,chan->chan_no+1,chan->tx_idx_in);

	chan->tx_idx_in++;
	if (chan->tx_idx_in >= SMG_HP_TDM_CHUNK_IDX_SZ) {
		chan->tx_idx_in=0;
	}

	return 0;
}

/*********************************************************//**
  Internal Span Methods
 *************************************************************/



/*!
  \fn int sangoma_hp_tdm_open_chan(sangoma_hptdm_span_t *span, sangoma_hptdm_chan_reg_t *cfg, unsigned int chan_no, sangoma_hptdm_chan_t **chan_ptr)
  \brief Span Method: Open a channel inside of span
  \param span span object
  \param cfg channel registration structure
  \param chan_no channel number
  \param chan_ptr pass user the channel pointer reference
  \return 0 = pass, non zero fail
*/

static int sangoma_hp_tdm_open_chan(sangoma_hptdm_span_t *span,
			      sangoma_hptdm_chan_reg_t *cfg,
			      unsigned int chan_no,
			      sangoma_hptdm_chan_t **chan_ptr)
{
	sangoma_hptdm_chan_t *chan;

	if (!span->init) {
		return -1;
	}

	if (chan_no >= SMG_HP_TDM_MAX_CHANS) {
		lib_printf(0,NULL,"open_chan failed chan_no %i >= max chans %i\n",
				chan_no, SMG_HP_TDM_MAX_CHANS);
		return -1;
	}

	if (!cfg->rx_data || !cfg->p) {
		return -1;
	}

	if (span->chan_idx[chan_no].chan_no_hw < 0) {
		lib_printf(0,NULL,"open_chan failed chan_no s%ic%i is not mapped to hardware\n",
				span->span_no+1,chan_no+1);
		return -1;
	}

	chan = &span->chan_idx[chan_no].chan;
	if (chan->init) {
		/* Chan Busy */
		lib_printf(0,NULL,"open_chan failed chan_no s%ic%i is busy\n",
				span->span_no+1,chan_no+1);
		return 1;
	}

	memset(chan,0,sizeof(sangoma_hptdm_chan_t));

	chan->chan_no = chan_no;
	chan->span = span;
	memcpy(&chan->chan_reg, cfg, sizeof(sangoma_hptdm_chan_reg_t));

	chan->push = sangoma_hp_tdm_chan_push;

	chan->init=1;
	lib_printf(15,NULL,"open_chan chan_no s%ic%i ok\n",
				span->span_no+1,chan_no+1);

	*chan_ptr = chan;

	return 0;

}


/*!
  \fn int sangoma_hp_tdm_close_chan(sangoma_hptdm_chan_t *chan)
  \brief Span Method: Close channel
  \param chan channel object
  \return 0 = pass, non zero fail
*/

static int sangoma_hp_tdm_close_chan(sangoma_hptdm_chan_t *chan)
{
	chan->init=0;
	chan->chan_reg.p=NULL;
	lib_printf(15,NULL,"close_chan chan_no s%ic%i ok\n",
				chan->span_no+1,chan->chan_no+1);
	return 0;
}


/*!
  \fn sangoma_hp_tdm_is_chan_closed (sangoma_hptdm_chan_t *chan)
  \brief Span Method: Test if channel is closed
  \param chan channel object
  \return 0 = channel is NOT closed,  non zero channel IS closed
*/
static int sangoma_hp_tdm_is_chan_closed (sangoma_hptdm_chan_t *chan)
{
	return (chan->init == 0) ? 1:0;
}

/*!
  \fn sangoma_hp_tdm_close_span(sangoma_hptdm_span_t *span)
  \brief Span Method: Close span
  \param span span object
  \return 0 = pass, non zero fail
*/
static int sangoma_hp_tdm_close_span(sangoma_hptdm_span_t *span)
{
	int i;
	sangoma_hptdm_chan_t *chan=NULL;

	for (i=0;i<SMG_HP_TDM_MAX_CHANS;i++) {
		chan = &span->chan_idx[i].chan;
		if (chan->init) {
			chan->init=0;
		}
	}

	close(span->sock);
	span->sock=-1;

	return 0;
}

/*!
  \brief Span Method: User passes cmd to the span
  \param span span object
  \return 0 = pass, non zero fail
*/
static int sangoma_hp_tdm_event_ctrl_span(sangoma_hptdm_span_t *span, hp_tdmapi_tx_event_t *event)
{
	int err;

	if (!span->init || span->sock < 0) {
		return -1;
	}

	err = ioctl(span->sock,SIOC_WANPIPE_API,event);
	if (err < 0){
		lib_printf(0,NULL,"Error: SPAN %i Failed to execute event!\n",
				span->span_no+1);
		return -1;
	}

	return 0;
}

/*!
  \brief Span Method: User requests full span configuration
  \param span span object
  \return 0 = pass, non zero fail
*/
static int sangoma_hp_tdm_event_get_cfg(sangoma_hptdm_span_t *span, wan_if_cfg_t *if_cfg)
{
	memcpy(if_cfg,&span->span_cfg,sizeof(if_cfg));
	return 0;
}

/*!
  \brief Span Method: handle the span
  \param span span object
  \return 0 = pass, non zero fail
*/
static int sangoma_hp_tdm_run_span(sangoma_hptdm_span_t *span)
{
	int err=0;
	sangoma_status_t sangstatus;
	unsigned int flags;

	if (!span->init) {
		lib_printf(0, NULL, "Span %i not initialized %i\n",span->span_no+1);
		return -1;
	}

	lib_printf(15, NULL, "Starting RUN SPAN %i Sock=%i\n",span->span_no+1, span->sock);

	if (span->sock < 0) {
		err = sangoma_hptdm_span_open(span);
		if (err) {
			usleep(500000);
			err=-2;
			goto sangoma_hp_tdm_run_span_exit;
		}
	}

	sangstatus = sangoma_waitfor(span->waitobj, (POLLIN|POLLPRI), &flags, 0);

	if (sangstatus == SANG_STATUS_SUCCESS) {

		if (flags & POLLPRI){
			err=sangoma_hp_tdm_handle_oob_event(span);
			if (err) {
				lib_printf(0, NULL, "RUN SPAN: %i oob err %i\n",
					span->span_no+1, err);
				err=-3;
				goto sangoma_hp_tdm_run_span_exit;
			}
		}
		if (flags & POLLIN){
			err=sangoma_hp_tdm_handle_read_event(span);
			if (err) {
				lib_printf(0, NULL, "RUN SPAN: %i read err %i\n",
					span->span_no+1, err);
				err=-4;
				goto sangoma_hp_tdm_run_span_exit;
			}

			err=sangoma_hp_tdm_handle_write_event(span);
			if (err) {
				lib_printf(0, NULL, "RUN SPAN: %i write err %i\n",
					span->span_no+1, err);
				err=-5;
				goto sangoma_hp_tdm_run_span_exit;
			}
		}

	} else if (sangstatus == SANG_STATUS_APIPOLL_TIMEOUT) {
		/* Timeout continue */
		return 0;

	} else {
		err = -1;
		/* Error */
		if (errno == EAGAIN) {
			goto sangoma_hp_tdm_run_span_exit;
		}

		err=-6;
	}

sangoma_hp_tdm_run_span_exit:

	if (err < 0) {
		if (span->sock >= 0) {
			sangoma_close(&span->sock);
			span->sock=-1;
		}
	}

	return err;
}



/*---------------------------------------------------------
  PUBLIC STRUCTURES
 ----------------------------------------------------------*/


/*
  \brief Initialize and Configure Span - private functions not to be used directly!
  \param span_no span number - integer
  \param cfg span registration struct
  \param version library version number added by the macro
  \return NULL: fail,  Span Object: pass
 *
 * The __sangoma_hptdm_api_span_init() function must NOT be called directly!
 *  One MUST use defined sangoma_hptdm_api_span_init() macro instead
 */

sangoma_hptdm_span_t * __sangoma_hptdm_api_span_init(int span_no, sangoma_hptdm_span_reg_t *cfg, int version)
{
	int err,i,ch=0;
	sangoma_hptdm_span_t *span;

	span = malloc(sizeof(sangoma_hptdm_span_t));
	if (!span) {
		return NULL;
	}

	memset(span,0,sizeof(sangoma_hptdm_span_t));

	span->span_no=span_no;
	sprintf(span->if_name,"wptdm_s%ic1",span_no+1);

	if (cfg) {
		memcpy(&span->span_reg,cfg,sizeof(sangoma_hptdm_span_reg_t));
		if (!lib_log) {
			lib_log = cfg->log;
		}
	}

	err=sangoma_hptdm_span_open(span);
	if (err) {
		free(span);
		return NULL;
	}

	if (span->span_cfg.media == WAN_MEDIA_E1) {
		span->span_cfg.active_ch = span->span_cfg.active_ch >> 1;
	}

	lib_printf(0,NULL,"Span %i Configuration\n",span->span_no+1);
	lib_printf(0,NULL,"Used By\t:%i\n",span->span_cfg.usedby);
	lib_printf(0,NULL,"Media\t:%i\n",span->span_cfg.media);
	lib_printf(0,NULL,"Active Ch\t:0x%08X\n",span->span_cfg.active_ch);
	lib_printf(0,NULL,"Chunk Sz\t:%i\n",span->span_cfg.chunk_sz);
	lib_printf(0,NULL,"HW Coding\t:%i\n",span->span_cfg.hw_coding);
	lib_printf(0,NULL,"If Number\t:%i\n",span->span_cfg.interface_number);


	/* Map all channels to the actually configued on hardware */
	for (i=0;i<SMG_HP_TDM_MAX_CHANS;i++) {
		span->chan_idx[i].chan_no_hw=-1;
		if (span->span_cfg.active_ch & (1<<i)) {
			span->chan_idx[i].chan_no_hw=ch;
			lib_printf(0,NULL,"Chan %i Mapped to %i",i,ch);
			ch++;
			span->max_chans++;
		} else {
			lib_printf(0,NULL,"Chan %i Not Mapped",i);
		}
	}

	lib_printf(0,NULL,"Total Chans\t:%i\n",span->max_chans);

	/* Must be configurable */
	span->chunk_sz=span->span_cfg.chunk_sz;
	span->tx_size=span->max_chans*span->chunk_sz;

	span->init=1;
	span->idle=0xFF;

	span->open_chan = sangoma_hp_tdm_open_chan;
	span->close_chan = sangoma_hp_tdm_close_chan;
	span->is_chan_closed = sangoma_hp_tdm_is_chan_closed;
	span->run_span =sangoma_hp_tdm_run_span;
	span->close_span = sangoma_hp_tdm_close_span;
	span->event_ctrl = sangoma_hp_tdm_event_ctrl_span;
	span->get_cfg = sangoma_hp_tdm_event_get_cfg;

	lib_printf(5, NULL, "Span %i Initialized\n",span->span_no+1);

	return span;
}

/*
  \brief Free, Un-Initialize Span
  \param span_no span object
  \return 0 = pass, non zero fail
 */

int sangoma_hptdm_api_span_free(sangoma_hptdm_span_t *span)
{
	if (span->sock >= 0) {
		span->close_span(span);
	}

	free(span);
	span=NULL;

	return 0;
}

