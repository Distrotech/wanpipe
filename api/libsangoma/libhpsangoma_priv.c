/*****************************************************************************
 * libhpsangoma_priv.c:  Sangoma High Performance TDM API - Span Based Library
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
 *
 */

#include "libhpsangoma.h"
#include "libhpsangoma_priv.h"
#define DFT_CARD "wanpipe1"


/*!
  \brief Execute a management ioctl command
  \param span span object
  \return 0 = pass, non zero fail
 */
int do_wanpipemon_cmd(sangoma_hptdm_span_t *span)
{
	return ioctl(span->sock,WANPIPE_IOCTL_PIPEMON,&span->wan_udp);
}

/*!
  \brief Get full span hw configuration
  \param span span object
  \return 0 = pass, non zero fail
 */
unsigned char sangoma_get_cfg(sangoma_hptdm_span_t *span)
{
	span->wan_udp.wan_udphdr_command = WANPIPEMON_READ_CONFIGURATION;
	span->wan_udp.wan_udphdr_data_len = 0;
	span->wan_udp.wan_udphdr_return_code = 0xaa;
	do_wanpipemon_cmd(span);
	if (span->wan_udp.wan_udphdr_return_code != 0){
		lib_printf(0,NULL,"Error: SPAN %i management command failed 0x02Xs (%s)",
			span->span_no+1, span->wan_udp.wan_udphdr_return_code,strerror(errno));
		return -1;
	}

	memcpy(&span->span_cfg,&span->wan_udp.wan_udphdr_data[0],sizeof(span->span_cfg));

	return 0;
}


/*!
  \brief Open socket to span hw interface
  \param span span object
  \return 0 = pass, non zero fail
*/

int sangoma_hptdm_span_open(sangoma_hptdm_span_t *span)
{
	int sock=-1;
	int err;
	int status;

	sock =  sangoma_open_api_span(span->span_no);
   	if( sock < 0 ) {
      		perror("Socket");
      		return -1;
   	} /* if */

	span->sock=sock;

	span->waitobj = NULL;
	status = sangoma_wait_obj_create(&span->waitobj, span->sock, SANGOMA_DEVICE_WAIT_OBJ);
	if (status != SANG_STATUS_SUCCESS) {
		fprintf(stderr, "Error creating sangoma waitable object\n");
		err = -1;
		goto sangoma_hptdm_span_open_error;
	}

	err = sangoma_get_cfg(span);
	if (err) {
		goto sangoma_hptdm_span_open_error;
	}

	lib_printf(0,NULL,"libhp: span %i opened\n",
			span->span_no+1);

	return 0;

sangoma_hptdm_span_open_error:

	if (span->sock >= 0) {
		sangoma_close(&span->sock);
	}

	if (span->waitobj) {
		sangoma_wait_obj_delete(&span->waitobj);
	}

	return err;
}


/*!
  \brief Read oob event from hw and push oob event up to the user. Called after select()
  \param span span object
  \return 0 = pass, non zero fail
*/

int sangoma_hp_tdm_handle_oob_event(sangoma_hptdm_span_t *span)
{
	int err;
	hp_tdmapi_rx_event_t *rx_event;

	err = sangoma_read_event(span->sock, &span->tdm_api); recv(span->sock, span->rx_data, sizeof(span->rx_data), MSG_OOB);
	if (err > 0) {
		rx_event=(hp_tdmapi_rx_event_t*)&span->rx_data;
		if (span->span_reg.rx_event && span->span_reg.p) {
			span->span_reg.rx_event(span->span_reg.p, rx_event);
		}
	}
	/* For now treat all err as socket reload */
	return 1;
}

/*!
  \brief Multiplex span rx data and push it up to each channel
  \param span span object
  \return 0 = pass, non zero fail
*/

int sangoma_hp_tdm_push_rx_data(sangoma_hptdm_span_t *span)
{
	int i=0,x=0,err=0;
	sangoma_hptdm_chan_t *chan=NULL;
	hp_tmd_chunk_t *chunk=NULL;
	int chan_no_hw;
	char *rx_data = &span->rx_data[sizeof(wp_api_hdr_t)];


	for (i=0;i<SMG_HP_TDM_MAX_CHANS;i++) {
		chan = &span->chan_idx[i].chan;
		if (!chan->init) {
			continue;
		}
		chan_no_hw = span->chan_idx[i].chan_no_hw;

		lib_printf(15,NULL, "SPAN %i Chan %i Chunk %i Channelizing\n",
				span->span_no+1, chan->chan_no+1, span->chunk_sz);

		chunk = &chan->rx_chunk;

		for (x=0;x<span->chunk_sz;x++) {
			chunk->data[x] = rx_data[(span->max_chans*x)+chan_no_hw];
		}

		chunk->len = span->chunk_sz;
		if (chan->chan_reg.p && chan->chan_reg.rx_data) {
			err=chan->chan_reg.rx_data(chan->chan_reg.p,chunk->data,chunk->len);
		} else {
			err=1;
		}

		if (err) {
			chan->init=0;
			chan->chan_reg.p=NULL;
		}

	}

	return err;


}

/*!
  \brief Read data from hw interface and pass it up to the rxdata handler:  called by run_span()
  \param span span object
  \return 0 = pass, non zero fail
*/

int sangoma_hp_tdm_handle_read_event(sangoma_hptdm_span_t *span)
{
	int err=0;

#if 0
	hp_tdmapi_rx_event_t *rx_event = (hp_tdmapi_rx_event_t*)&span->rx_data[0];

	err = sangoma_readmsg(span->sock,
								&span->rx_data[0],sizeof(wp_api_hdr_t),
							&span->rx_data[sizeof(wp_api_hdr_t)],
							sizeof(span->rx_data) - sizeof(wp_api_hdr_t),
							0);

	if (err >  sizeof(wp_api_hdr_t)) {

		lib_printf(15,NULL, "SPAN %i Read Len = %i\n",span->span_no+1,err);

		if (rx_event->wp_api_rx_hdr_event_type) {

			if (span->span_reg.rx_event && span->span_reg.p) {
				span->span_reg.rx_event(span->span_reg.p, rx_event);
			}

		} else {

			if (err % span->chunk_sz) {
				lib_printf(0,NULL, "Error: SPAN %i Read Len = %i Block not chunk %i aligned! \n",span->span_no+1,err,span->chunk_sz);
				/* Received chunk size is not aligned drop it for now */
				return 0;
			}

			sangoma_hp_tdm_push_rx_data(span);
		}

		err=0;

	} else {
		if (errno == EAGAIN) {
			err = 0;
		} else {
			err=-1;
		}
	}
#endif

	return err;
}


/*!
  \brief Pull data from all chans and write data to hw interface: called by run_span()
  \param span span object
  \return 0 = pass, non zero fail
*/
int sangoma_hp_tdm_handle_write_event(sangoma_hptdm_span_t *span)
{
	int i=0,x=0,err=0;
	sangoma_hptdm_chan_t *chan;
	hp_tmd_chunk_t *chunk;
	int chan_no_hw=0;
	char *tx_data = &span->tx_data[sizeof(wp_api_hdr_t)];

	memset(&span->tx_data,span->idle,sizeof(span->tx_data));


	for (i=0;i<SMG_HP_TDM_MAX_CHANS;i++) {
		chan = &span->chan_idx[i].chan;
		if (!chan->init) {
			/* This channel is not open */
			continue;
		}
		chan_no_hw=span->chan_idx[i].chan_no_hw;

		chunk = &chan->tx_idx[chan->tx_idx_out];
		if (!chunk->init) {
			lib_printf(15,NULL,"span write s%ic%i tx chunk underrun out %i \n",
					chan->span_no+1,chan->chan_no+1,chan->tx_idx_out);
			/* There is no tx data for this channel */
			continue;
		}

		for (x=0;x<span->chunk_sz;x++) {

			tx_data[(span->max_chans*x)+chan_no_hw] = chunk->data[chunk->offset];
			chunk->offset++;

			if (chunk->offset >= chunk->len) {
				chunk->init=0;

				lib_printf(15,NULL,"span write s%ic%i tx chunk out %i \n",
					chan->span_no+1,chan->chan_no+1,chan->tx_idx_out);

				chan->tx_idx_out++;
				if (chan->tx_idx_out >= SMG_HP_TDM_CHUNK_IDX_SZ) {
					chan->tx_idx_out=0;
				}
				chunk=&chan->tx_idx[chan->tx_idx_out];
				if (!chunk->init) {
					/* We are out of tx data on this channel */
					break;
				}
			}
		}
	}


	err = sangoma_writemsg(span->sock,
				     span->tx_data,sizeof(wp_api_hdr_t),
				     tx_data, span->tx_size,
				     0);

	lib_printf(15, NULL, "SPAN %i TX Len %i Expecting %i\n",
			span->span_no+1, err, span->tx_size+sizeof(wp_api_hdr_t));

	if (err < span->tx_size) {
		if (errno == EAGAIN) {
			return 0;
		} else {
			return -1;
		}
	} else {
		err=0;
	}

	return err;

}
