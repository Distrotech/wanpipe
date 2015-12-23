/******************************************************************************//**
 * \file sample.c
 * \brief WANPIPE(tm) API C Sample Code
 *
 * Authors: David Rokhvarg <davidr@sangoma.com>
 *			Nenad Corbic <ncorbic@sangoma.com>
 *
 * Copyright (c) 2007 - 08, Sangoma Technologies
 * All rights reserved.
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sangoma Technologies nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ===============================================================================
 *
 */

#include "libsangoma.h"
#include "lib_api.h"		

static u_int32_t	poll_events_bitmap = 0;

/*!
  \def TEST_NUMBER_OF_OBJECTS
  \brief Number of wait objects to define in object array.

  Objects are used to wait on file descripotrs.
  Usually there is one wait object per file descriptor.

  In this example there is a single file descriptor and a
  single wait object.
*/
#define TEST_NUMBER_OF_OBJECTS 1

static sangoma_wait_obj_t *sangoma_wait_objects[TEST_NUMBER_OF_OBJECTS];

/* This example application has only a single execution thread - it is safe
 * to use a global buffer for received data and for data to be transmitted. */
static unsigned char rxdata[MAX_NO_DATA_BYTES_IN_FRAME];
static unsigned char txdata[MAX_NO_DATA_BYTES_IN_FRAME];

typedef struct sangoma_chan {
	int spanno;
	int channo;
} sangoma_chan_t;
sangoma_chan_t sangoma_channels[TEST_NUMBER_OF_OBJECTS];

/* Warning: non-thread safe globals. Ok for this single-thread example but not in production. */
unsigned char rx_rbs_bits = WAN_RBS_SIG_A;
FILE	*pRxFile;
int		application_termination_flag = 0;

/*****************************************************************
 * Prototypes
 *****************************************************************/

int __cdecl main(int argc, char* argv[]);
int open_sangoma_device(void);
void handle_span_chan(int open_device_counter);
int handle_tdm_event(uint32_t dev_index);
int handle_data(uint32_t dev_index, int flags_out);
int read_data(uint32_t dev_index, wp_api_hdr_t *rx_hdr, void *rx_buffer, int rx_buffer_length);
int write_data(uint32_t dev_index, wp_api_hdr_t *tx_hdr, void *tx_buffer, int tx_len);
int dtmf_event(sng_fd_t fd,unsigned char digit,unsigned char type,unsigned char port);
int rbs_event(sng_fd_t fd,unsigned char rbs_bits);
int rxhook_event(sng_fd_t fd,unsigned char hook_state);
int rxring_event(sng_fd_t fd,unsigned char ring_state);
int ringtrip_event (sng_fd_t fd, unsigned char ring_state);
int write_data_to_file(unsigned char *data,	unsigned int data_length);
int sangoma_print_stats(sng_fd_t sangoma_dev);
void cleanup(void);
int handle_fe_rw (void);

#ifdef __WINDOWS__
BOOL WINAPI TerminateHandler(DWORD dwCtrlType);
#else
void TerminateHandler(int);
#endif

/*****************************************************************
 * General Functions
 *****************************************************************/

/*!
  \fn void print_rx_data(unsigned char *data, int	datalen)
  \brief  Prints the contents of data packet
  \param data pointer to data buffer
  \param datalen size of data buffer
  \return void
*/
void print_rxdata(unsigned char *data, int	datalen, wp_api_hdr_t *hdr); /* dont remove prototype, gcc complains */
void print_rxdata(unsigned char *data, int	datalen, wp_api_hdr_t *hdr)
{
	int i;
	int err=0;

#ifdef WP_API_FEATURE_RX_TX_ERRS
	err=hdr->wp_api_rx_hdr_errors;
#endif
	printf("Data: (Len=%i,Errs=%i)\n",datalen,err);
	for(i = 0; i < datalen; i++) {
		if((i % 20 == 0)){
			if(i){
				printf("\n");
			}
		}
		printf("%02X ", data[i]);
#if 0
		/* don't print too much!! */
		if(i > 100){
			printf("...\n");
			break;
		}
#endif
	}
	printf("\n");
}


int sangoma_print_stats(sng_fd_t sangoma_dev)
{
    int err;
	unsigned char firm_ver, cpld_ver;
	wanpipe_api_t wp_api;
	wanpipe_chan_stats_t stats_str;
	wanpipe_chan_stats_t *stats=&stats_str;
	memset(&wp_api,0,sizeof(wp_api));


  	err=sangoma_get_stats(sangoma_dev, &wp_api, stats);
	if (err) {
		printf("sangoma_get_stats(() failed (err: %d (0x%X))!\n", err, err);
		return 1;
	}

	printf( "******* OPERATIONAL_STATS *******\n");

	printf("\trx_packets\t: %u\n",			stats->rx_packets);
	printf("\ttx_packets\t: %u\n",			stats->tx_packets);
	printf("\trx_bytes\t: %u\n",			stats->rx_bytes);
	printf("\ttx_bytes\t: %u\n",			stats->tx_bytes);
	printf("\trx_errors\t: %u\n",			stats->rx_errors);	//Total number of Rx errors
	printf("\ttx_errors\t: %u\n",			stats->tx_errors);	//Total number of Tx errors
	printf("\trx_dropped\t: %u\n",			stats->rx_dropped);
	printf("\ttx_dropped\t: %u\n",			stats->tx_dropped);
	printf("\tmulticast\t: %u\n",			stats->multicast);
	printf("\tcollisions\t: %u\n",			stats->collisions);

	printf("\trx_length_errors: %u\n",		stats->rx_length_errors);
	printf("\trx_over_errors\t: %u\n",		stats->rx_over_errors);
	printf("\trx_crc_errors\t: %u\n",		stats->rx_crc_errors);	//HDLC CRC mismatch
	printf("\trx_frame_errors\t: %u\n",		stats->rx_frame_errors);//HDLC "abort" occured
	printf("\trx_fifo_errors\t: %u\n",		stats->rx_fifo_errors);
	printf("\trx_missed_errors: %u\n",		stats->rx_missed_errors);

	printf("\ttx_aborted_errors: %u\n",		stats->tx_aborted_errors);
	printf("\tTx Idle Data\t: %u\n",		stats->tx_carrier_errors);

	printf("\ttx_fifo_errors\t: %u\n",		stats->tx_fifo_errors);
	printf("\ttx_heartbeat_errors: %u\n",	stats->tx_heartbeat_errors);
	printf("\ttx_window_errors: %u\n",		stats->tx_window_errors);

	printf("\n\ttx_packets_in_q: %u\n",	stats->current_number_of_frames_in_tx_queue);
	printf("\ttx_queue_size: %u\n",		stats->max_tx_queue_length);

	printf("\n\trx_packets_in_q: %u\n",	stats->current_number_of_frames_in_rx_queue);
	printf("\trx_queue_size: %u\n",		stats->max_rx_queue_length);

	printf("\n\trx_events_in_q: %u\n",		stats->current_number_of_events_in_event_queue);
	printf("\trx_event_queue_size: %u\n",	stats->max_event_queue_length);
	printf("\trx_events: %u\n",				stats->rx_events);
	printf("\trx_events_dropped: %u\n",		stats->rx_events_dropped);

	printf("\tHWEC tone (DTMF) events counter: %u\n",	stats->rx_events_tone);
	printf( "*********************************\n");

	SANGOMA_INIT_TDM_API_CMD(wp_api);
	err=sangoma_get_driver_version(sangoma_dev,&wp_api, NULL);
	if (err) {
		return 1;
	}
	printf("\tDriver Version: %u.%u.%u.%u\n",
				wp_api.wp_cmd.version.major,
				wp_api.wp_cmd.version.minor,
				wp_api.wp_cmd.version.minor1,
				wp_api.wp_cmd.version.minor2);

	SANGOMA_INIT_TDM_API_CMD(wp_api);
	err=sangoma_get_firmware_version(sangoma_dev, &wp_api, &firm_ver);
	if (err) {
		return 1;
	}
	printf("\tFirmware Version: %X\n",
				firm_ver);

	SANGOMA_INIT_TDM_API_CMD(wp_api);
	err=sangoma_get_cpld_version(sangoma_dev, &wp_api, &cpld_ver);
	if (err) {
		return 1;
	}
	printf("\tCPLD Version: %X\n",
				cpld_ver);

	return 0;
}

/*!
  \fn int read_data(uint32_t dev_index)
  \brief Read data buffer from a device
  \param dev_index device index number associated with device file descriptor
  \param rx_hdr pointer to api header
  \param rx_buffer pointer to a buffer where recived data will be stored
  \param rx_buffer_length maximum length of rx_buffer
  \return 0 - Ok otherwise Error
*/
int read_data(uint32_t dev_index, wp_api_hdr_t *rx_hdr, void *rx_buffer, int rx_buffer_length)
{
	sng_fd_t	dev_fd	 = sangoma_wait_obj_get_fd(sangoma_wait_objects[dev_index]);
	sangoma_chan_t *chan = sangoma_wait_obj_get_context(sangoma_wait_objects[dev_index]);		
	int			Rx_lgth = 0;
	static int	Rx_count= 0;
	wanpipe_api_t tdm_api; 

	memset(&tdm_api, 0x00, sizeof(tdm_api));
	memset(rx_hdr, 0, sizeof(wp_api_hdr_t));

	/* read the message */
	Rx_lgth = sangoma_readmsg(
					dev_fd,
					rx_hdr,					/* header buffer */
					sizeof(wp_api_hdr_t),	/* header size */
					rx_buffer,				/* data buffer */
					rx_buffer_length,		/* data BUFFER size */
					0);   
	if(Rx_lgth <= 0) {
		printf("Span: %d, Chan: %d: Error receiving data!\n", 
			chan->spanno, chan->channo);
		return 1;
	}


#ifdef WP_API_FEATURE_RX_TX_ERRS
	if (rx_hdr->wp_api_rx_hdr_error_map) {
     	if (rx_hdr->wp_api_rx_hdr_error_map & 1<<WP_FIFO_ERROR_BIT) {
         	printf("Span: %d, Chan: %d rx fifo error 0x%02X\n",chan->spanno,chan->channo,rx_hdr->wp_api_rx_hdr_error_map);
		}
     	if (rx_hdr->wp_api_rx_hdr_error_map & 1<<WP_CRC_ERROR_BIT) {
         	printf("Span: %d, Chan: %d rx crc error 0x%02X\n",chan->spanno,chan->channo,rx_hdr->wp_api_rx_hdr_error_map);
		}
     	if (rx_hdr->wp_api_rx_hdr_error_map & 1<<WP_ABORT_ERROR_BIT) {
         	printf("Span: %d, Chan: %d rx abort error 0x%02X\n",chan->spanno,chan->channo,rx_hdr->wp_api_rx_hdr_error_map);
		}
     	if (rx_hdr->wp_api_rx_hdr_error_map & 1<<WP_FRAME_ERROR_BIT) {
         	printf("Span: %d, Chan: %d rx framing error 0x%02X\n",chan->spanno,chan->channo,rx_hdr->wp_api_rx_hdr_error_map);
		}
     	if (rx_hdr->wp_api_rx_hdr_error_map & 1<<WP_DMA_ERROR_BIT) {
         	printf("Span: %d, Chan: %d rx dma error 0x%02X\n",chan->spanno,chan->channo,rx_hdr->wp_api_rx_hdr_error_map);
		}
		//return 1;
	}
#endif

	Rx_count++;

	if (verbose){
		print_rxdata(rx_buffer, Rx_lgth,rx_hdr);
	}

	if (stats_period && Rx_count % stats_period == 0) {
     	sangoma_print_stats(dev_fd);
	}
			
	/* use Rx_counter as "write" events trigger: */
	if(rbs_events == 1 && (Rx_count % 400) == 0){
		/*	bitmap - set as needed: WAN_RBS_SIG_A | WAN_RBS_SIG_B | WAN_RBS_SIG_C | WAN_RBS_SIG_D;
		
			In this example make bits A and B to change each time,
			so it's easy to see the change on the receiving side.
		*/
		if(rx_rbs_bits == WAN_RBS_SIG_A){
			rx_rbs_bits = WAN_RBS_SIG_B;
		}else{
			rx_rbs_bits = WAN_RBS_SIG_A;
		}
		printf("Writing RBS bits (0x%X)...\n", rx_rbs_bits);
		sangoma_tdm_write_rbs(dev_fd, &tdm_api, 
					chan->channo,
					rx_rbs_bits);
	}

	/* if user needs Rx data to be written into a file: */
	if(files_used & RX_FILE_USED){
		write_data_to_file(rx_buffer, Rx_lgth);
	}

	return 0;
}

/*!
  \fn int write_data(uint32_t dev_index, wp_api_hdr_t *tx_hdr, void *tx_data, int tx_len)
  \brief Transmit a data buffer to a device.
  \param dev_index device index number associated with device file descriptor
  \param tx_hdr pointer to a wp_api_hdr_t
  \param tx_data pointer to a data buffer 
  \param tx_len tx data buffer len
  \return 0 - Ok otherwise Error
*/
int write_data(uint32_t dev_index, wp_api_hdr_t *tx_hdr, void *tx_buffer, int tx_len)
{
	sng_fd_t	dev_fd	= sangoma_wait_obj_get_fd(sangoma_wait_objects[dev_index]);
	sangoma_chan_t *chan = sangoma_wait_obj_get_context(sangoma_wait_objects[dev_index]);
	int			err;
	static int	Tx_count = 0;

	if (hdlc_repeat) {
		if (verbose){
			printf("Repeating Frame\n");
		}
        tx_hdr->wp_api_tx_hdr_hdlc_rpt_len=4;
		memset(tx_hdr->wp_api_tx_hdr_hdlc_rpt_data,Tx_count,4);
	}

	/* write a message */
	err = sangoma_writemsg(
				dev_fd,
				tx_hdr,					/* header buffer */
				sizeof(wp_api_hdr_t),	/* header size */
				tx_buffer,				/* data buffer */
				tx_len,					/* DATA size */
				0);

	if (err <= 0){
		printf("Span: %d, Chan: %d: Failed to send %s!\n", 
			chan->spanno, 
			chan->channo, strerror(errno));
		return -1;
	}
	
	Tx_count++;
	if (verbose){
		printf("Packet sent: counter: %i, len: %i, errors %i\n", Tx_count, err, tx_hdr->wp_api_tx_hdr_errors);
	}else{
		if(Tx_count && (!(Tx_count % 1000))){
			printf("Packet sent: counter: %i, len: %i\n", Tx_count, err);
		}
	}

	if (tx_delay) {
     	sangoma_msleep(tx_delay);
	}

#if 1
	if(tx_cnt && Tx_count >= tx_cnt){
		write_enable=0;
		printf("Disabling POLLOUT...\n");
		/* No need for POLLOUT, turn it off!! If not turned off, and we
		 * have nothing for transmission, sangoma_socket_waitfor() will return
		 * immediately, creating a busy loop. */
		//sangoma_wait_objects[dev_index].flags_in &= (~POLLOUT);
		return 0;
	}
#endif
	return 0;
}

/*!
  \fn int handle_data(uint32_t	dev_index, int flags_out)
  \brief Read data buffer from the device and transmit it back down.
  \param dev_index device index number associated with device file descriptor
  \return 0 - Ok  otherwise Error

   Read data buffer from a device.
*/
int handle_data(uint32_t dev_index, int flags_out)
{
	wp_api_hdr_t	rxhdr;
	int err=0;

	memset(&rxhdr, 0, sizeof(rxhdr));

#if 0
	printf("%s(): span: %d, chan: %d\n", __FUNCTION__,
		sangoma_wait_objects[dev_index].span, sangoma_wait_objects[dev_index].chan);
#endif

	if(flags_out & POLLIN){
		if(read_data(dev_index, &rxhdr, rxdata, MAX_NO_DATA_BYTES_IN_FRAME) == 0){
			if(rx2tx){
				/* Send back received data (create a "software loopback"), just a test. */
				return write_data(dev_index, &rxhdr, rxdata,rxhdr.data_length);
			}
		}
	}

	if( (flags_out & POLLOUT) && write_enable ){
	
		wp_api_hdr_t txhdr;
		static unsigned char tx_test_byte = 2;

		memset(&txhdr, 0, sizeof(txhdr));
		txhdr.data_length = (unsigned short)tx_size;/* use '-txsize' command line option to change 'tx_size' */

		/* set data which will be transmitted */
		memset(txdata, tx_test_byte, txhdr.data_length);
		
		err = write_data(dev_index, &txhdr, txdata, tx_size);
		if (err== 0) {
			//tx_test_byte++;
		}
	}
	return err;
}

/*!
  \fn int decode_api_event (wp_api_event_t *wp_tdm_api_event)
  \brief Handle API Event
  \param wp_tdm_api_event
*/
static void decode_api_event(wp_api_event_t *wp_tdm_api_event)
{ 
	printf("%s(): span: %d, chan: %d\n", __FUNCTION__,
		wp_tdm_api_event->span, wp_tdm_api_event->channel);

	switch(wp_tdm_api_event->wp_api_event_type)
	{
	case WP_API_EVENT_DTMF:/* DTMF detected by Hardware */
		printf("DTMF Event: Channel: %d, Digit: %c (Port: %s, Type:%s)!\n",
			wp_tdm_api_event->channel,
			wp_tdm_api_event->wp_api_event_dtmf_digit,
			(wp_tdm_api_event->wp_api_event_dtmf_port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
			(wp_tdm_api_event->wp_api_event_dtmf_type == WAN_EC_TONE_PRESENT)?"PRESENT":"STOP");
		break;

	case WP_API_EVENT_RXHOOK:
		printf("RXHOOK Event: Channel: %d, %s! (0x%X)\n", 
			wp_tdm_api_event->channel,
			WAN_EVENT_RXHOOK_DECODE(wp_tdm_api_event->wp_api_event_hook_state),
			wp_tdm_api_event->wp_api_event_hook_state);
		break;

	case WP_API_EVENT_RING_DETECT:
		printf("RING Event: %s! (0x%X)\n",
			WAN_EVENT_RING_DECODE(wp_tdm_api_event->wp_api_event_ring_state),
			wp_tdm_api_event->wp_api_event_ring_state);
		break;

	case WP_API_EVENT_RING_TRIP_DETECT:
		printf("RING TRIP Event: %s! (0x%X)\n", 
			WAN_EVENT_RING_TRIP_DECODE(wp_tdm_api_event->wp_api_event_ring_state),
			wp_tdm_api_event->wp_api_event_ring_state);
		break;

	case WP_API_EVENT_RBS:
		printf("RBS Event: Channel: %d, 0x%X!\n",
			wp_tdm_api_event->channel,
			wp_tdm_api_event->wp_api_event_rbs_bits);
		printf( "RX RBS: A:%1d B:%1d C:%1d D:%1d\n",
			(wp_tdm_api_event->wp_api_event_rbs_bits & WAN_RBS_SIG_A) ? 1 : 0,
			(wp_tdm_api_event->wp_api_event_rbs_bits & WAN_RBS_SIG_B) ? 1 : 0,
			(wp_tdm_api_event->wp_api_event_rbs_bits & WAN_RBS_SIG_C) ? 1 : 0,
			(wp_tdm_api_event->wp_api_event_rbs_bits & WAN_RBS_SIG_D) ? 1 : 0);
		break;

	case WP_API_EVENT_LINK_STATUS:
		printf("Link Status Event: %s! (0x%X)\n", 
			WAN_EVENT_LINK_STATUS_DECODE(wp_tdm_api_event->wp_api_event_link_status),
			wp_tdm_api_event->wp_api_event_link_status);
		break;

	case WP_API_EVENT_ALARM:
		printf("New Alarm State: %s! (0x%X)\n", (wp_tdm_api_event->wp_api_event_alarm == 0?"Off":"On"),
			wp_tdm_api_event->wp_api_event_alarm);
		break;

	case WP_API_EVENT_POLARITY_REVERSE:
		printf("Polarity Reversal Event : %s! (0x%X)\n", 
			WP_API_EVENT_POLARITY_REVERSE_DECODE(wp_tdm_api_event->wp_api_event_polarity_reverse),
			wp_tdm_api_event->wp_api_event_polarity_reverse);
		break;

	default:
		printf("Unknown TDM API Event: %d\n", wp_tdm_api_event->wp_api_event_type);
		break;
	}
}

/*!
  \fn int handle_tdm_event(uint32_t dev_index)
  \brief Read Event buffer from the device
  \param dev_index device index number associated with device file descriptor
  \return 0 - Ok  otherwise Error

   An EVENT has occoured. Execute a system call to read the EVENT
   on a device.
*/
int handle_tdm_event(uint32_t dev_index)
{
	wanpipe_api_t	tdm_api; 
	sng_fd_t		dev_fd = sangoma_wait_obj_get_fd(sangoma_wait_objects[dev_index]);

#if 0
	printf("%s(): dev_index: %d, dev_fd: 0x%p\n", __FUNCTION__, dev_index, dev_fd);
#endif

	memset(&tdm_api, 0x00, sizeof(tdm_api));

	if(sangoma_read_event(dev_fd, &tdm_api)){
		return 1;
	}

	decode_api_event(&tdm_api.wp_cmd.event);
	return 0;
}


int handle_fe_rw (void)
{
	sng_fd_t	dev_fd	 = sangoma_wait_obj_get_fd(sangoma_wait_objects[0]);
	uint8_t 	data=0;
	int err;

	if (fe_read_cmd) { 
		err=sangoma_fe_reg_read(dev_fd,fe_read_reg,&data);
		if (err) {
			fprintf(stderr,"Error, sangoma_fe_reg_read failed %i\n",err);
			return err;
		}

		printf("FE READ Reg=0x%04X Data=%02X\n", fe_read_reg,data);
	}

	if (fe_write_cmd) {

		printf("FE WRITE Reg=0x%04X Data=%02X\n", fe_write_reg,fe_write_data);
		
		err=sangoma_fe_reg_write(dev_fd,fe_write_reg,fe_write_data);
		if (err) {
			fprintf(stderr,"Error, sangoma_fe_reg_write failed %i\n",err);
			return err;
		}
		
	}
	
	return 0;
}


/*!
  \fn void handle_span_chan(int open_device_counter)
  \brief Write data buffer into a file
  \param open_device_counter number of opened devices
  \return void

  This function will wait on all opened devices.
  This example will wait for RX and EVENT signals.
  In case of POLLIN - rx data available
  In case of POLLPRI - event is available
*/
void handle_span_chan(int open_device_counter)
{
	int	iResult, i;
	u_int32_t input_flags[TEST_NUMBER_OF_OBJECTS];
	u_int32_t output_flags[TEST_NUMBER_OF_OBJECTS];

	printf("\n\nSpan/Chan Handler: RxEnable=%s, TxEnable=%s, TxCnt=%i, TxLen=%i, rx2tx=%s\n",
		(read_enable? "Yes":"No"), (write_enable?"Yes":"No"),tx_cnt,tx_size, (rx2tx?"Yes":"No"));	

	for (i = 0; i < open_device_counter; i++) {
		input_flags[i] = poll_events_bitmap;
	}

	/* Main Rx/Tx/Event loop */
	while(!application_termination_flag) 
	{	
		iResult = sangoma_waitfor_many(sangoma_wait_objects, 
					       input_flags,
					       output_flags,
				           open_device_counter /* number of wait objects */,
					       5000 /* wait timeout, in milliseconds */);
		switch(iResult)
		{
		case SANG_STATUS_APIPOLL_TIMEOUT:
			/* timeout (not an error) */
			{
			sng_fd_t dev_fd = sangoma_wait_obj_get_fd(sangoma_wait_objects[0]);
			sangoma_print_stats(dev_fd);
			}
			printf("Timeout\n");
			continue;

		case SANG_STATUS_SUCCESS:
			for(i = 0; i < open_device_counter; i++){

				/* a wait object was signaled */
				if(output_flags[i] & POLLPRI){
					/* got tdm api event */
					if(handle_tdm_event(i)){
						printf("Error in handle_tdm_event()!\n");
                        application_termination_flag=1;
						break;
					}
				}
					
				if(output_flags[i] & (POLLIN | POLLOUT)){
					/* rx data OR a free tx buffer available */
					if(handle_data(i, output_flags[i])){
						printf("Error in handle_data()!\n");
                        //application_termination_flag=1;
						break;
					}
				}
			}/* for() */
			break;

		default:
			/* error */
			printf("Error: iResult: %s (%d)\n", SDLA_DECODE_SANG_STATUS(iResult), iResult);
			return;
		}

	}/* while() */
}

/*!
  \fn int write_data_to_file(unsigned char *data, unsigned int data_length)
  \brief Write data buffer into a file
  \param data data buffer
  \param data_length length of a data buffer
  \return data_length = ok  otherwise error

*/
int write_data_to_file(unsigned char *data, unsigned int data_length)
{
	if(pRxFile == NULL){
		return 1;
	}

	return fwrite(data, 1, data_length, pRxFile);
}

#ifdef __WINDOWS__
/*
 * TerminateHandler() - this handler is called by the system whenever user tries to terminate
 *						the process with Ctrl+C, Ctrl+Break or closes the console window.
 *						Perform a clean-up here.
 */
BOOL TerminateHandler(DWORD dwCtrlType)
{
	printf("\nProcess terminated by user request.\n");
	application_termination_flag = 1;
	/* do the cleanup before exiting: */
	cleanup();
	/* return FALSE so the system will call the dafult handler which will terminate the process. */
	return FALSE;
}
#else
/*!
  \fn void TerminateHandler (int sig)
  \brief Signal handler for graceful shutdown
  \param sig signal 
*/
void TerminateHandler (int sig)
{
	printf("\nProcess terminated by user request.\n");
	application_termination_flag = 1;
	/* do the cleanup before exiting: */
	cleanup();
	return;
}

#endif

/*!
  \fn void cleanup()
  \brief Protperly shutdown single device
  \param dev_no device index number
  \return void

*/
void cleanup()
{
	int dev_no;
	sng_fd_t fd;
	sangoma_chan_t *chan;
	sangoma_wait_obj_t *sng_wait_object;
	wanpipe_api_t tdm_api; 

	/* do the cleanup before exiting: */
	for(dev_no = 0; dev_no < TEST_NUMBER_OF_OBJECTS; dev_no++){

		sng_wait_object = sangoma_wait_objects[dev_no];
		if(!sng_wait_object){
			continue;
		}
		chan = sangoma_wait_obj_get_context(sng_wait_object);
		printf("%s(): span: %d, chan: %d ...\n", __FUNCTION__, 
			chan->channo,
			chan->spanno);

		fd = sangoma_wait_obj_get_fd(sng_wait_object);
		memset(&tdm_api, 0x00, sizeof(tdm_api));

		if(dtmf_enable_octasic == 1){
			/* Disable dtmf detection on Octasic chip */
			sangoma_tdm_disable_dtmf_events(fd, &tdm_api);
		}

		if(dtmf_enable_remora == 1){
			/* Disable dtmf detection on Sangoma's Remora SLIC chip */
			sangoma_tdm_disable_rm_dtmf_events(fd, &tdm_api);
		}

		if(remora_hook == 1){
			sangoma_tdm_disable_rxhook_events(fd, &tdm_api);
		}

		if(rbs_events == 1){
			sangoma_tdm_disable_rbs_events(fd, &tdm_api);
		}

		sangoma_wait_obj_delete(&sng_wait_object);

		sangoma_close(&fd);

	}
}


/*!
  \fn int open_sangoma_device()
  \brief Open a single span chan device
  \return 0 ok otherise error.

  This function will open a single span chan.

  However it can be rewritten to iterate for all spans and chans and  try to
  open all existing wanpipe devices.

  For each opened device, a wait object will be initialized.
  For each device, configure the chunk size for tx/rx
                   enable events such as DTMF/RBS ...etc
*/
int open_sangoma_device()
{
	int span, chan, err = 0, open_dev_cnt = 0;
	sangoma_status_t status;
	sng_fd_t	dev_fd = INVALID_HANDLE_VALUE;
	wanpipe_api_t tdm_api;

	span = wanpipe_port_no;
	chan = wanpipe_if_no;

	/* span and chan are 1-based */
	if (force_open) {
		dev_fd = __sangoma_open_api_span_chan(span, chan);
	} else {
		dev_fd = sangoma_open_api_span_chan(span, chan);
	}
	if( dev_fd == INVALID_HANDLE_VALUE){
		printf("Warning: Failed to open span %d, chan %d\n", span , chan);
		return 1;
	}else{
		printf("Successfuly opened span %d, chan %d\n", span , chan);
	}

	memset(&tdm_api, 0x00, sizeof(tdm_api));

	status = sangoma_wait_obj_create(&sangoma_wait_objects[open_dev_cnt], dev_fd, SANGOMA_DEVICE_WAIT_OBJ);
	if(status != SANG_STATUS_SUCCESS){
		printf("Error: Failed to create 'sangoma_wait_object'!\n");
		return 1;
	}
	sangoma_channels[open_dev_cnt].channo = chan;
	sangoma_channels[open_dev_cnt].spanno = span;
	sangoma_wait_obj_set_context(sangoma_wait_objects[open_dev_cnt], &sangoma_channels[open_dev_cnt]);
	/* open_dev_cnt++; */

	if((err = sangoma_get_full_cfg(dev_fd, &tdm_api))){
		return 1;
	}

	if(set_codec_slinear){
		printf("Setting SLINEAR codec\n");
		if((err=sangoma_tdm_set_codec(dev_fd, &tdm_api, WP_SLINEAR))){
			return 1;
		}
	}

	if(set_codec_none){
		printf("Disabling codec\n");
		if((err=sangoma_tdm_set_codec(dev_fd, &tdm_api, WP_NONE))){
			return 1;
		}
	}

	if(usr_period){
		printf("Setting user period: %d\n", usr_period);
		if((err=sangoma_tdm_set_usr_period(dev_fd, &tdm_api, usr_period))){
			return 1;
		}
	}

	if (rx_gain_cmd) {
		printf("Setting rx gain : %f\n", rx_gain);
		if (sangoma_set_rx_gain(dev_fd,&tdm_api,rx_gain)) {
			return 1;
		}	
	}

	if (tx_gain_cmd) {
		printf("Setting tx gain : %f\n", rx_gain);
		if (sangoma_set_tx_gain(dev_fd,&tdm_api,tx_gain)) {
			return 1;
		}
	}

	if(set_codec_slinear || usr_period || set_codec_none){
		/* display new configuration AFTER it was changed */
		if((err=sangoma_get_full_cfg(dev_fd, &tdm_api))){
			return 1;
		}
	}

	if(dtmf_enable_octasic == 1){
		poll_events_bitmap |= POLLPRI;
		/* enable dtmf detection on Octasic chip */
		if((err=sangoma_tdm_enable_dtmf_events(dev_fd, &tdm_api))){
			return 1;
		}
	}

	if(dtmf_enable_remora == 1){
		poll_events_bitmap |= POLLPRI;
		/* enable dtmf detection on Sangoma's Remora SLIC chip (A200 ONLY) */
		if((err=sangoma_tdm_enable_rm_dtmf_events(dev_fd, &tdm_api))){
			return 1;
		}
	}

	if(remora_hook == 1){
		poll_events_bitmap |= POLLPRI;
		if((err=sangoma_tdm_enable_rxhook_events(dev_fd, &tdm_api))){
			return 1;
		}
	}

	if(rbs_events == 1){
		poll_events_bitmap |= POLLPRI;
		if((err=sangoma_tdm_enable_rbs_events(dev_fd, &tdm_api, 20))){
			return 1;
		}
	}
	if (buffer_multiplier) {
		printf("Setting buffer multiplier to %i\n",buffer_multiplier);
		err=sangoma_tdm_set_buffer_multiplier(dev_fd,&tdm_api,buffer_multiplier);
		if (err) {
			return 1;
		}
	}
	if (ss7_cfg_status) {
		wan_api_ss7_cfg_status_t ss7_hw_status;
     	printf("Getting ss7 cfg status\n");
		err=sangoma_ss7_get_cfg_status(dev_fd,&tdm_api,&ss7_hw_status);
		if (err) {
         	return 1;
		}
		printf("SS7 HW Configuratoin\n");
		printf("SS7 hw support   = %s\n",ss7_hw_status.ss7_hw_enable?"Enabled":"Disabled");
		printf("SS7 hw mode      = %s\n",ss7_hw_status.ss7_hw_mode?"4096":"128");
		printf("SS7 hw lssu size = %d\n",ss7_hw_status.ss7_hw_lssu_size);
		printf("SS7 driver repeat= %s\n",ss7_hw_status.ss7_driver_repeat?"Enabled":"Disabled");
	}

	if (stats_test) {
		sangoma_print_stats(dev_fd);
		exit(0);
	}
	if (flush_stats_test) {
  		sangoma_flush_stats(dev_fd, &tdm_api);
		sangoma_print_stats(dev_fd);
		exit(0);
	}

	if (fe_read_test) {
		uint8_t data=0;
		uint8_t prev_data=0;
		int i;
		int elapsed=0;
		int elapsed_cnt=0;


#if defined(__WINDOWS__)
		long started, ended;
		 started = GetTickCount();
#else
		struct timeval started, ended;

		gettimeofday(&started, NULL);
#endif		

		for (i=0;;i++) {
			elapsed_cnt++;
			sangoma_fe_reg_read(dev_fd, 0xF8, &data);	
			prev_data=data;
			if (data==0) {
				printf("Error: Bad FE Read 0x00 cnt=%i !!!!!!!!!!! \n",i);
				exit(1);
			} else {
				data=(data>>3)&0x1F;
				if (data != 0x0C) {
					printf("Error: Bad FE Read Prev Data = 0x%X cnt=%i !!!!!!!!!!!!\n",prev_data,i);
					exit(1);
				}
			}

#if defined(__WINDOWS__)
			ended = GetTickCount();
#else
			gettimeofday(&ended, NULL);
#endif

#if defined(__WINDOWS__)
			elapsed =  ended - started;
#else
			elapsed = (((ended.tv_sec * 1000) + ended.tv_usec / 1000) - ((started.tv_sec * 1000) + started.tv_usec / 1000));
#endif 

			if (elapsed > 5000) {
			  	printf("FE Reg 0xF8 = Prev Data = 0x%X int_cnt=%08i tot_cnt %08i\n",prev_data,elapsed_cnt,i);
#if defined(__WINDOWS__)
				started = GetTickCount();
#else
				gettimeofday(&started, NULL);
#endif

				elapsed_cnt=0;
			}
		}
		exit(0);
	}

	printf("Device Config RxQ=%i TxQ=%i \n",
		sangoma_get_rx_queue_sz(dev_fd,&tdm_api),
		sangoma_get_rx_queue_sz(dev_fd,&tdm_api));

#if 0
	sangoma_set_rx_queue_sz(dev_fd,&tdm_api,20);
	sangoma_set_tx_queue_sz(dev_fd,&tdm_api,30);
#endif

	printf("Device Config RxQ=%i TxQ=%i \n",
		sangoma_get_rx_queue_sz(dev_fd,&tdm_api),
		sangoma_get_tx_queue_sz(dev_fd,&tdm_api));
	
	sangoma_flush_bufs(dev_fd,&tdm_api);

	sangoma_print_stats(dev_fd);


	return err;
}


static int sangoma_hardware_rescan(void)
{

#ifdef  WP_API_FEATURE_HARDWARE_RESCAN 
	int err;
	int cnt;
 	port_management_struct_t port;
 	sng_fd_t fd = sangoma_open_driver_ctrl(1);
	if (fd == INVALID_HANDLE_VALUE) {
		printf("Hardware Rescan failed to open driver ctrl\n");
    	return -1;
	}

	memset(&port,0,sizeof(port));

	err=sangoma_driver_hw_rescan(fd,&port,&cnt);
	if (err < 0) {
		printf("Hardware Rescan error %i\n",err);
     	return err;
	}

	printf("Hardware Rescan: Detected Devices %i\n",cnt);

	sangoma_close(&fd);
	
#endif	

	return err;
}


/*!
  \fn int __cdecl main(int argc, char* argv[])
  \brief Main function that starts the sample code
  \param argc  number of arguments
  \param argv  argument list
*/
int __cdecl main(int argc, char* argv[])
{
	int proceed, i;


	proceed=init_args(argc,argv);
	if (proceed != WAN_TRUE){
		usage(argv[0]);
		return -1;
	}
	
	if (hw_pci_rescan) {
		/* Function used to rescan pci/usb bus.
		   added here as use example. Uncomment to use.
		   This function should only be used when all ports
		   are down. */
		return sangoma_hardware_rescan();
	}

	/* register Ctrl+C handler - we want a clean termination */
#if defined(__WINDOWS__)
	if (!SetConsoleCtrlHandler(TerminateHandler, TRUE)) {
		printf("ERROR : Unable to register terminate handler ( %d ).\nProcess terminated.\n", 
			GetLastError());
		return -1;
	}
#else
	signal(SIGHUP,TerminateHandler);
	signal(SIGTERM,TerminateHandler);
#endif
	
	for(i = 0; i < TEST_NUMBER_OF_OBJECTS; i++){
		sangoma_wait_objects[i] = NULL;
	}

	poll_events_bitmap = 0;
	if(read_enable	== 1){
		poll_events_bitmap |= POLLIN;
	}

	if(write_enable == 1 && rx2tx == 1){
		/* These two options are mutually exclusive because 'rx2tx' option
		 * indicates "use Reciever as the timing source for Transmitter". */
		write_enable = 0;
	}
	
	if(write_enable == 1){
		poll_events_bitmap |= POLLOUT;
	}

	/* Front End connect/disconnect, and other events, such as DTMF... */
	poll_events_bitmap |= (POLLHUP | POLLPRI);
#if defined(__WINDOWS__)
	printf("Enabling Poll Events:\n");
	print_poll_event_bitmap(poll_events_bitmap);
#endif
	printf("Connecting to Port/Span: %d, Interface/Chan: %d\n",
		wanpipe_port_no, wanpipe_if_no);


	if(open_sangoma_device()){
		return -1;
	}

	printf("********************************\n");
	printf("files_used: 0x%x\n", files_used);
	printf("********************************\n");
	if(files_used & RX_FILE_USED){
		pRxFile = fopen( (const char*)&rx_file[0], "wb" );
		if(pRxFile == NULL){
			printf("Can't open Rx file: [%s]!!\n", rx_file);
		}else{
			printf("Open Rx file: %s. OK.\n", rx_file);
		}
	}

	if (fe_read_cmd || fe_write_cmd) {
		handle_fe_rw();
		goto done;
	}
	
	handle_span_chan(1 /* handle a single device */);

done:
	/* returned from main loop, do the cleanup before exiting: */
	cleanup();

	printf("\nSample application exiting.(press any key)\n");
	_getch();
	return 0;
}
