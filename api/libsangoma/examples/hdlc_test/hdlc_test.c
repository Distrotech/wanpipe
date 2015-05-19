/*****************************************************************************
* hdlc_test.c:  Multiple HDLC Test Receive Module
*
* Author(s):    Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2011 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
*/

#include <libsangoma.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pthread.h>
#include <linux/wanpipe.h>
#include <linux/wanpipe_tdm_api.h>
#include <wanpipe_hdlc.h>

#define FALSE	0
#define TRUE	1

/* Enable/Disable tx of random frames */
#define RAND_FRAME 0

#define MAX_SPANS 80
#define MAX_CHANS 32
#define MAX_NUM_OF_TIMESLOTS  MAX_SPANS*MAX_CHANS

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     15000 //MAX_NUM_OF_TIMESLOTS*10	/* Size of tx data */
#define MAX_TX_FRAMES 	1000000     /* Number of frames to transmit */  

#define WRITE 1
#define MAX_IF_NAME 20
typedef struct {

	int sock;
	int rx_cnt;
	int tx_cnt;
	int data;
	int last_error;
	int frames;
	int chan;
	int span;
	int packets;
	int active;
	int running;
	char if_name[MAX_IF_NAME+1];
	wanpipe_hdlc_engine_t *hdlc_eng;
	wanpipe_tdm_api_t tdm_api;
	wanpipe_chan_stats_t stats;
	pthread_t thread;
	
} timeslot_t;


timeslot_t tslot_array[MAX_NUM_OF_TIMESLOTS];

//static int tx_change_data=0, tx_change_data_cnt=0;
static int app_end=0;
static int gerr=0;
static int verbose=0;
static int timeout=0;
static int err_limit=4;
pthread_mutex_t g_lock;

void *process_con_tx(void *obj); 

static int log_printf(char *fmt, ...)
{
	char *data;
	int ret = 0;
	va_list ap;
	char date[200] = "";
	struct tm now;
	time_t epoch;

	if (verbose < 0) {
		/* Continue through */
	} else if (!verbose) {
     		return 0;
	}
	
	if (time(&epoch) && localtime_r(&epoch, &now)) {
		strftime(date, sizeof(date), "%Y-%m-%d %T", &now);
	}

	va_start(ap, fmt);
	ret = vasprintf(&data, fmt, ap);
	if (ret == -1) {
		return ret;
	}

	pthread_mutex_lock(&g_lock);
	fprintf(stderr, "[%d] %s %s", getpid(), date, data);
	pthread_mutex_unlock(&g_lock);

	if (data) {
     		free(data);
	}

	return 0;
}


#define MGD_STACK_SIZE 1024*1024

static int launch_tx_hdlc_thread(timeslot_t *slot)
{
	pthread_attr_t attr;
	int result = -1;

	result = pthread_attr_init(&attr);
        //pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

	result = pthread_create(&slot->thread, &attr, process_con_tx, (void*)slot);
	if (result) {
		log_printf("%s: Error: Creating Thread! %s\n",
				 __FUNCTION__,strerror(errno));
		gerr++;
	} 
	pthread_attr_destroy(&attr);

   	return result;
}



static void print_stats(timeslot_t *slot)
{
    wanpipe_chan_stats_t stats;

	memcpy(&stats,&slot->stats,sizeof(stats));

	printf("\nDriver Statistcs Span=%i Chan=%i\n",slot->span,slot->chan);
	
	printf("\trx_packets\t: %u\n",			stats.rx_packets);
	printf("\ttx_packets\t: %u\n",			stats.tx_packets);
	printf("\trx_bytes\t: %u\n",			stats.rx_bytes);
	printf("\ttx_bytes\t: %u\n",			stats.tx_bytes);
	printf("\trx_errors\t: %u\n",			stats.rx_errors);
	printf("\ttx_errors\t: %u\n",			stats.tx_errors);
	printf("\trx_dropped\t: %u\n",			stats.rx_dropped);
	printf("\ttx_dropped\t: %u\n",			stats.tx_dropped);
	printf("\tmulticast\t: %u\n",			stats.multicast);
	printf("\tcollisions\t: %u\n",			stats.collisions);

	printf("\trx_length_errors: %u\n",		stats.rx_length_errors);
	printf("\trx_over_errors\t: %u\n",		stats.rx_over_errors);
	printf("\trx_crc_errors\t: %u\n",		stats.rx_crc_errors);
	printf("\trx_frame_errors\t: %u\n",	stats.rx_frame_errors);
	printf("\trx_fifo_errors\t: %u\n",		stats.rx_fifo_errors);
	printf("\trx_missed_errors: %u\n",		stats.rx_missed_errors);

	/* Transmitter aborted frame transmission. Not an error. */
	printf("\trx_hdlc_abort_counter: %u\n",	stats.rx_hdlc_abort_counter);

	printf("\ttx_aborted_errors: %u\n",	stats.tx_aborted_errors);
	printf("\tTx Idle Data\t: %u\n",		stats.tx_carrier_errors);

	printf("\ttx_fifo_errors\t: %u\n",		stats.tx_fifo_errors);
	printf("\ttx_heartbeat_errors: %u\n",	stats.tx_heartbeat_errors);
	printf("\ttx_window_errors: %u\n",		stats.tx_window_errors);

	printf("\n\ttx_packets_in_q: %u\n",	stats.current_number_of_frames_in_tx_queue);
	printf("\ttx_queue_size: %u\n",		stats.max_tx_queue_length);

	printf("\n\trx_packets_in_q: %u\n",	stats.current_number_of_frames_in_rx_queue);
	printf("\trx_queue_size: %u\n",		stats.max_rx_queue_length);

	printf("\n\trx_events_in_q: %u\n",	stats.current_number_of_events_in_event_queue);
	printf("\trx_event_queue_size: %u\n",		stats.max_event_queue_length);
	printf("\trx_events: %u\n",	stats.rx_events);
	printf("\trx_events_dropped: %u\n",		stats.rx_events_dropped);

	printf("\tHWEC tone (DTMF) events counter: %u\n",		stats.rx_events_tone);

}
#if 0
static void print_packet(unsigned char *buf, int len)
{
	int x;

	if (!verbose) {
         return;
	}

	printf("{  | ");
	for (x=0;x<len;x++){
		if (x && x%24 == 0){
			printf("\n  ");
		}
		if (x && x%8 == 0)
			printf(" | ");
		printf("%02x ",buf[x]);
	}
	printf("}\n");
}
#endif

static void sig_end(int sigid)
{
	//printf("%d: Got Signal %i\n",getpid(),sigid);
	app_end=1;
}        


static unsigned long g_next_rand = 1;

/* RAND_MAX assumed to be 32767 */
static int myrand(int max_rand) {
	unsigned long tmp;
	pthread_mutex_lock(&g_lock);
	g_next_rand = g_next_rand * 1103515245 + 12345;
	tmp=((g_next_rand/65536) % max_rand);  
	pthread_mutex_unlock(&g_lock);
    return tmp;
}

static void mysrand(unsigned seed) {
	pthread_mutex_lock(&g_lock);
	g_next_rand = seed;
	pthread_mutex_unlock(&g_lock);
}


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

static int MakeConnection(timeslot_t *slot, char *router_name ) 
{

	int span,chan;
	wanpipe_api_t tdm_api;
	sangoma_span_chan_fromif(slot->if_name,&span,&chan);
	
	memset(&tdm_api,0,sizeof(tdm_api));

	if (span > 0 && chan > 0) {
    	slot->sock = sangoma_open_tdmapi_span_chan(span,chan);
	    if( slot->sock < 0 ) {
				printf("Error: Failed to open Span=%i Chan=%i\n",span,chan);
      			perror("Open Span Chan: ");
				gerr++;
      			return( FALSE );
		}

		sangoma_flush_stats(slot->sock,&tdm_api);
		sangoma_set_rx_queue_sz(slot->sock,&tdm_api,100);

#if 0
        sangoma_tdm_set_codec(slot->sock,&tdm_api,WP_NONE);
		err=sangoma_tdm_set_usr_period(slot->sock,&tdm_api,period);
		if (err) {
     		log_printf("Error: Failed to set period\n");
	    }
#endif

		log_printf("Socket bound to Span=%i Chan=%i\n\n",span,chan);
		slot->chan=chan;
		slot->span=span;
		return (TRUE);
	}


	gerr++;
	
	return(FALSE);
}

static int api_tdm_fe_alarms_callback(sng_fd_t fd, unsigned int alarm)
{
	int fd_found=0;
	int i;
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
		if (tslot_array[i].sock == fd) {
			fd_found=1;
			break;		
		}	
	}          

	if (fd_found) {
        	log_printf ("FE ALARMS Call back found device %i Alarm=%i \n", 
				fd,alarm);  	

	} else {
		log_printf ("FE ALARMS Call back no device Alarm=%i\n",alarm);
	}

	return 0;
}



#define ALL_OK  	0
#define	BOARD_UNDERRUN 	1
#define TX_ISR_UNDERRUN 2
#define UNKNOWN_ERROR 	4

static void process_con_rx(void) 
{
	unsigned int Rx_count;
	fd_set 	oob,ready;
	int err,serr,i,slots=0;
	unsigned char Rx_data[15000];
	int error_bit=0, error_crc=0, error_abort=0, error_frm=0;
	struct timeval tv;
	int frame=0,print_status=0;
	int max_fd=-1, packets=0;
	timeslot_t *slot=NULL;
	wanpipe_tdm_api_t tdm_api;
	struct timeval started, ended;

	memset(&tdm_api,0,sizeof(tdm_api));

	tdm_api.wp_tdm_event.wp_fe_alarm_event = &api_tdm_fe_alarms_callback;

	gettimeofday(&started, NULL); 
	
	i=0;
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
	
		if (tslot_array[i].sock < 0) {
			continue;		
		}
		
		tslot_array[i].hdlc_eng = wanpipe_reg_hdlc_engine();
		if (!tslot_array[i].hdlc_eng){
			log_printf("ERROR: Failed to register HDLC Engine\n");
			gerr++;
			return;
		}

		if (tslot_array[i].sock > max_fd){
			max_fd=tslot_array[i].sock;
		}

		{
			wanpipe_tdm_api_t tdm_api;
			memset(&tdm_api,0,sizeof(tdm_api));
			sangoma_flush_stats(tslot_array[i].sock,&tdm_api);
		}
	}
	i=0;

	if (max_fd==-1) {
     	printf("Error: No device selected\n");
		gerr++;
		return;
	}
	

	
    Rx_count = 0;

	for(;;) {
	
		FD_ZERO(&ready);
		FD_ZERO(&oob);

		tv.tv_usec = 0;
		tv.tv_sec = 1;

		
		max_fd=0;
		for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
	
			if (tslot_array[i].sock < 0) {
				continue;		
			}	
			
			FD_SET(tslot_array[i].sock,&ready);
			FD_SET(tslot_array[i].sock,&oob);
			if (tslot_array[i].sock > max_fd){
				max_fd=tslot_array[i].sock;
			}
		}

		if (app_end){
			break;
		}

		if (max_fd > 1024) {
         	log_printf("Error: Select error fd >= 1024\n");
			gerr++;
			break;
		}
		
		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 
		slots=0;
  		if((serr=select(max_fd + 1, &ready, NULL, &oob, &tv)) >= 0){
					
			if (timeout) {
				int elapsed;
				gettimeofday(&ended, NULL);
				elapsed = (((ended.tv_sec * 1000) + ended.tv_usec / 1000) - ((started.tv_sec * 1000) + started.tv_usec / 1000)); 
				if (elapsed > timeout) {
					app_end=1;
					break;
				}
			}

		    for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++) {
	
	   	    	if (tslot_array[i].sock < 0) {
			    	continue;		
		        }	
		
		    	slots++;
		    	slot=&tslot_array[i];

				if (FD_ISSET(slot->sock,&oob)){ 
					sangoma_tdm_read_event(slot->sock,&tdm_api);
				}
		
			
				/* Check for rx packets */
				if (FD_ISSET(slot->sock,&ready)){
			

					err = sangoma_readmsg_tdm(slot->sock, 
							 Rx_data,sizeof(wp_tdm_api_rx_hdr_t), 
							 &Rx_data[sizeof(wp_tdm_api_rx_hdr_t)],
							 sizeof(Rx_data), 0);

		   		// log_printf("RX DATA HDLC: Len=%i err=%i\n",err-sizeof(wp_api_hdr_t),err);
        		//print_packet(&Rx_data[sizeof(wp_api_hdr_t)],err-sizeof(wp_api_hdr_t));
				/* err indicates bytes received */
				if(err > 0) {
					unsigned char *rx_frame = 
						(unsigned char *)&Rx_data[sizeof(wp_tdm_api_rx_hdr_t)];
					int len = err;
	
					
					/* Rx packet recevied OK
					 * Each rx packet will contain sizeof(wp_api_hdr_t) bytes of
					 * rx header, that must be removed.  The
					 * first byte of the sizeof(wp_api_hdr_t) byte header will
					 * indicate an error condition.
					 */
#if 0
					log_printf("RX DATA: Len=%i\n",len);
						print_packet(rx_frame,len);
					frame++;
					if ((frame % 100) == 0) {
						sangoma_get_full_cfg(slot->sock, &tdm_api);
					}
				
					continue;
#endif					
					frame++;
					print_status++;
					slot->frames++;	

#if 0
					if (slot->span == 1 && slot->chan == 3) {
                     	log_printf("%s Frames %i packets %i ger=%i\n",
								slot->if_name,slot->frames,slot->packets,gerr);
					}
#endif
					
					wanpipe_hdlc_decode(slot->hdlc_eng,rx_frame,len);
				
					slot->packets+=wanpipe_get_rx_hdlc_packets(slot->hdlc_eng);	
					packets+=wanpipe_get_rx_hdlc_packets(slot->hdlc_eng);	
					error_bit+=wanpipe_get_rx_hdlc_errors(slot->hdlc_eng);	
					error_crc+=slot->hdlc_eng->decoder.stats.crc;
					error_abort+=slot->hdlc_eng->decoder.stats.abort;
					error_frm+=slot->hdlc_eng->decoder.stats.frame_overflow;
				
#if 1	
					if (wanpipe_get_rx_hdlc_errors(slot->hdlc_eng) != slot->last_error){
						slot->last_error = wanpipe_get_rx_hdlc_errors(slot->hdlc_eng);
						if (slot->last_error > 1 && verbose){
							log_printf("%s: Errors = %i (crc=%i, abort=%i, frame=%i)\n",
								slot->if_name,
								wanpipe_get_rx_hdlc_errors(slot->hdlc_eng),
								slot->hdlc_eng->decoder.stats.crc,
								slot->hdlc_eng->decoder.stats.abort,
								slot->hdlc_eng->decoder.stats.frame_overflow);
								//sangoma_get_full_cfg(slot->sock, &tdm_api);
								//wanpipe_hdlc_dump_ring(slot->hdlc_eng);
						}
					}

					if ((slot->frames % 100) == 0 && slot->packets == 0){
                     	log_printf("%s: Frames=%i Rx Packets = %i (crc=%i, abort=%i, frame=%i)\n",
						slot->if_name,
						frame,
						wanpipe_get_rx_hdlc_packets(slot->hdlc_eng),
						slot->hdlc_eng->decoder.stats.crc,
						slot->hdlc_eng->decoder.stats.abort,
						slot->hdlc_eng->decoder.stats.frame_overflow);
					}
#endif
#if 0
					log_printf("%s: Received packet %i, length = %i : %s\n", 
							slot->if_name,++Rx_count, err,
							((error_bit == 0)? "USR DATA OK" :
							 (error_bit&BOARD_UNDERRUN)? "BRD_UNDR" : 
							 (error_bit&TX_ISR_UNDERRUN) ? "TX_UNDR" :
							 "UNKNOWN ERROR"));
#endif

				} else {
					if (app_end) {
             			break;
					}
					log_printf("\n%s: Error receiving data\n",slot->if_name);
					gerr++;
				}

			 } /* If rx */
			 
		    } /* for all slots */
		
		    if (verbose && print_status>100*slots){	 
				print_status=0;
				putchar('\r');
				log_printf("Slots=%04i frame=%04i packets=%04i errors=%04i (crc=%04i abort=%04i frm=%04i)",
					slots,
					frame, 
					packets,
					error_bit>slots?error_bit:0,
					error_crc,
					error_abort,
					error_frm);
		    
		    	fflush(stdout);
		    }
		    packets=0;	
		    error_bit=0;
		    error_crc=0;
		    error_abort=0;
		    error_frm=0;

		  
		} else {
			if (app_end) {
             	break;
			}
			log_printf("\n: Error selecting rx socket rc=0x%x errno=0x%x\n",
				serr,errno);
			perror("Select: ");
			gerr++;
			//break;
		}
	}


	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++) {

		if (tslot_array[i].sock < 0) {
			continue;		
		}	

		slot=&tslot_array[i];
		//printf("Get Stats for %i/%i\n",slot->span,slot->chan);

		while (1) {
			 err = sangoma_readmsg_tdm(slot->sock, 
					 Rx_data,sizeof(wp_tdm_api_rx_hdr_t), 
					 &Rx_data[sizeof(wp_tdm_api_rx_hdr_t)],
					 sizeof(Rx_data), 0);
			 if (err <= 0) {
              	break;
			 }
		}

		sangoma_get_stats(slot->sock,&slot->tdm_api,&slot->stats);

	}
	
	
	//log_printf("\nRx Unloading HDLC\n");
	
} 

 
void *process_con_tx(void *obj) 
{
	timeslot_t *slot = (timeslot_t *)obj;
	unsigned int Tx_count, max_tx_len, Tx_offset=0, Tx_length,Tx_hdlc_len;
	int Tx_encoded_hdlc_len;
	fd_set 	write;
	int err=0,i,tx_ok=1;
	unsigned char Tx_data[MAX_TX_DATA + sizeof(wp_api_hdr_t)];
	unsigned char Tx_hdlc_data[MAX_TX_DATA + sizeof(wp_api_hdr_t)];
	wanpipe_hdlc_engine_t *hdlc_eng;
	unsigned char next_idle;
	wanpipe_tdm_api_t tdm_api;
	struct timeval tv;
	//int txdata=0;

	memset(&tdm_api,0,sizeof(tdm_api));
	
	hdlc_eng = wanpipe_reg_hdlc_engine();
	if (!hdlc_eng){
		log_printf("ERROR: Failed to register HDLC Engine\n");
		gerr++;
		goto thread_exit;
	}
	
	Tx_count = 0;
	sangoma_get_full_cfg(slot->sock, &tdm_api);

	if (tdm_api.wp_tdm_cmd.hdlc) {
           Tx_length = max_tx_len =  100;          
	}else{
       	 	Tx_length = max_tx_len = sangoma_tdm_get_usr_mtu_mru(slot->sock, &tdm_api);
	}

	/* Send double of max so that a single big frame gets separated into two actual packets */
	Tx_hdlc_len=(max_tx_len*2);
	Tx_hdlc_len*= 0.75;

	log_printf("User MTU/MRU=%i  Tx Len = %i \n",max_tx_len,Tx_hdlc_len);
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  The CRC bytes will be appended as the 
	 * last two bytes in the rx packet.
	 */

	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(wp_api_hdr_t));
	slot->data=0x0;
	for (i=0;i<Tx_hdlc_len;i++){
		if (slot->data){
			Tx_data[i+sizeof(wp_api_hdr_t)] = slot->data;
		}else{
			Tx_data[i+sizeof(wp_api_hdr_t)] = myrand(255);
		}
	}
    mysrand(myrand(255));

	/* If drivers has been configured in HDLC_STREAMING
	 * mode, the CRC bytes will be included into the 
	 * rx packet.  Thus, the application should remove
	 * the last two bytes of the frame. 
	 * 
	 * The no_CRC_bytes_Rx will indicate to the application
	 * how many bytes to cut from the end of the rx frame.
	 *
	 */

	log_printf("%s: Tx Starting to write on sock %i data (0x%X)  f=%x l=%x hdr_sz=%i\n",
	 	slot->if_name,slot->sock,slot->data,Tx_data[sizeof(wp_api_hdr_t)],Tx_data[Tx_hdlc_len+sizeof(wp_api_hdr_t)-1],
		sizeof(wp_api_hdr_t));	

		
	//pause();
	
	for(;;) {	
		FD_ZERO(&write);
		FD_SET(slot->sock,&write);

		tv.tv_usec = 0;
		tv.tv_sec = 1;
		
		if (app_end){
			break;
		}
		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 
#if 1
		/* If we got busy on last frame repeat the frame */
		if (tx_ok == 1){
#if 0
			log_printf("TX DATA ORIG: Len=%i\n",Tx_hdlc_len);
	      		print_packet(&Tx_data[sizeof(wp_api_hdr_t)],Tx_hdlc_len);
#endif		
			wanpipe_hdlc_encode(hdlc_eng,&Tx_data[sizeof(wp_api_hdr_t)],Tx_hdlc_len,&Tx_hdlc_data[sizeof(wp_api_hdr_t)],&Tx_encoded_hdlc_len,&next_idle);
			if (Tx_encoded_hdlc_len < (max_tx_len*2)){
				int j;
				for (j=0;j<((max_tx_len*2) - Tx_encoded_hdlc_len);j++){
					Tx_hdlc_data[sizeof(wp_api_hdr_t)+Tx_encoded_hdlc_len+j]=next_idle;
				}
				Tx_encoded_hdlc_len+=j;
			}	
#if 0		
			log_printf("TX DATA HDLC: Olen=%i Len=%i\n",Tx_hdlc_len,Tx_encoded_hdlc_len);
        		print_packet(&Tx_hdlc_data[sizeof(wp_api_hdr_t)],Tx_encoded_hdlc_len);
#endif		
			if (Tx_encoded_hdlc_len > (max_tx_len*2)){
				log_printf("Tx hdlc len > max %i\n",Tx_encoded_hdlc_len);
				continue;
			}

			Tx_length=max_tx_len;
			Tx_offset=0;
			//log_printf("INITIAL Fragment Chunk tx! %i Tx_encoded =%i \n", Tx_offset,Tx_encoded_hdlc_len);
	
#if 0		
			if ((Tx_count % 60) == 0){
				Tx_hdlc_len++; /* Introduce Error */
			}
#endif
#if 0 
			log_printf("Data %i\n",Tx_hdlc_len);
			for (i=0;i<Tx_hdlc_len;i++){
				log_printf(" 0x%X",Tx_hdlc_data[sizeof(wp_api_hdr_t)+i]);
			}
			log_printf("\n");
#endif		
			tx_ok=0;
		} 
#endif		
  		if (select(slot->sock + 1,NULL, &write, NULL, &tv) >= 0){

			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(slot->sock,&write)){

#if 1
			        err=sangoma_writemsg_tdm(slot->sock, 
						     Tx_hdlc_data, sizeof(wp_api_hdr_t), 
						     &Tx_hdlc_data[sizeof(wp_api_hdr_t)+Tx_offset],
						     Tx_length,0);
#endif
				if (err > 0){
					/* Packet sent ok */
					//log_printf("\t\t%s:Packet sent: Len=%i Data=0x%x : %i\n",
					//		slot->if_name,err,slot->data,++Tx_count);
					//putchar('T');
						
					Tx_offset+=Tx_length;

					if (Tx_offset >= Tx_encoded_hdlc_len){ 
						//log_printf("LAST Chunk tx! %i \n", Tx_offset);
						Tx_offset=0;
						Tx_length=max_tx_len;
						/* Last fragment transmitted */
						/* pass throught */
					} else {
						Tx_length = Tx_encoded_hdlc_len - Tx_length;
						if (Tx_length > max_tx_len) {
							Tx_length=max_tx_len;
						}
	//					log_printf("MIDDLE Fragment Chunk tx! %i \n", Tx_offset);
						continue;
					}		

#if RAND_FRAME 	
					if (Tx_count%10 == 0){
						Tx_hdlc_len=myrand(max_tx_len*2*0.75);
						for (i=0;i<Tx_hdlc_len;i++){
							Tx_data[i+sizeof(wp_api_hdr_t)] = myrand(255);
						}
						mysrand(myrand(255));
					}
#endif					


#if 0
					if (Tx_count%100 == 0) {
						txdata = (txdata + 1) % 2;
						for (i=0;i<Tx_hdlc_len;i++){
							Tx_data[i+sizeof(wp_api_hdr_t)] = txdata;;
						}
	    			}
#endif
					Tx_count++;
					tx_ok=1;
				}else{
					if (errno != EBUSY){
						log_printf("Errno = %i EBUSY=%i\n",errno,-EBUSY);
						perror("Send Failed: ");
						break;
					}
					/* The driver is busy,  we must requeue this
					 * tx packet, and try resending it again 
					 * later. */
				}

		   	}else{
				log_printf("Error Tx nothing IFFSET\n");
			}

#if 0
			if (Tx_count > MAX_TX_FRAMES){
				break;
			}
#endif
#if 0
			if (Tx_count % 500){
				slot->data=slot->data+1;
				for (i=0;i<Tx_length;i++){
					Tx_data[i+sizeof(wp_api_hdr_t)] = slot->data;
				}
				tx_change_data=1;
				tx_change_data_cnt=0;
			}
#endif
		} else {
                	log_printf("\nError selecting socket\n");
			break;
		}
	}
	log_printf("\nTx Unloading HDLC  %p\n",slot);

thread_exit:

	slot->running=0;
	pthread_exit(NULL);
} 



/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call process_con() to read/write the socket 
 *
 **************************************************************/  

int main (int argc, char* argv[])
{
	int proceed;
	char router_name[20];	
	int x,i;
	int scnt=0;
	int span,chan;
	int active_check=0;

    pthread_mutex_init(&g_lock, NULL);  
	
	if (argc < 2){
		printf("Usage: hdlc_test s1c1 s2c1 [ -verbose ] [ -timeout 0 ] [ -err_limit 5 ] \n");
		exit(0);
	}	
	
	nice(-11);
	
	signal(SIGINT,&sig_end);
	signal(SIGTERM,&sig_end);

	memset(&tslot_array,0,sizeof(tslot_array));

	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
		tslot_array[i].sock=-1;
	}
	
	strncpy(router_name,"wanpipe1",(sizeof(router_name)-1));	
	
	
	for (x=0;x<argc-1;x++) {	
	
		timeslot_t *slot;

		if (strcmp(argv[x+1], "-verbose") == 0) {
         	verbose=1;
			log_printf("Verbosity is on\n");
			continue;
		}
		if (strcmp(argv[x+1], "-timeout") == 0) {
			x++;
         	timeout=atoi(argv[x+1])*1000;
			log_printf("Timeout = %i\n",timeout);
			continue;
		}
		if (strcmp(argv[x+1], "-err_limit") == 0) {
			x++;
         	err_limit=atoi(argv[x+1]);
			log_printf("Error Limit = %i\n",err_limit);
			continue;
		}

		sangoma_span_chan_fromif(argv[x+1],&span,&chan);
		if (span > 0 && span <= MAX_SPANS && chan > 0 && chan < MAX_CHANS) {
			slot=&tslot_array[scnt++];
		} else {
			printf("Error: Invalid interface name %s\n",argv[x+1]);
			gerr++;
			continue;
		}


		strncpy(slot->if_name, argv[x+1], MAX_IF_NAME);		
		log_printf("Connecting to IF=%s\n",slot->if_name);
	
		proceed = MakeConnection(slot,router_name);
		if( proceed == TRUE ){
			int err;

			log_printf("Creating %s with tx data 0x%x : Sock=%i : x=%i [s%dc%i]\n",
				slot->if_name,
				slot->data,
				slot->sock,
				x,
				slot->span,slot->chan);

			slot->active=1;
			slot->running=1;
			
			err=launch_tx_hdlc_thread(slot);
			if (err) {
				slot->running=0;
				gerr++;
			}
		}else{
			gerr++;
			
			printf("Error: Failed to create %s with tx data 0x%x : Sock=%i : x=%i [s%dc%i]\n",
				slot->if_name,
				slot->data,
				slot->sock,
				x,
				slot->span,slot->chan);

			if (slot->sock){
				close(slot->sock);
			}
			slot->sock=-1;
		}
	}

#if 0
	if (verbose < 0) {
		verbose=0;
	}
#endif

	if (gerr==0) {
 		process_con_rx();
		app_end=1;
	} else {
     	app_end=1;
		sleep(1);
	}


	while(1) {
		active_check=0;
		for (x=0;x<MAX_NUM_OF_TIMESLOTS;x++){
			volatile timeslot_t *slot;
			slot=&tslot_array[x];

			if (slot->running) {
				active_check++;
			}
		}

		if (!active_check) {
			break;
		}
		
		log_printf("Waiting for all threads to stop %i!\n",active_check);
		sleep(1);
	}

    printf("\nProduction Test Results\n");
	for (x=0;x<MAX_NUM_OF_TIMESLOTS;x++){
		timeslot_t *slot;
		char str_status[100];
		slot=&tslot_array[x];

       	if (slot->active) { 
				int status=0;
				printf("\n");

				if (slot->frames && !slot->packets) {
                 	gerr++;
					status++;
				}
				if (!slot->frames) {
                 	gerr++;
					status++;
				}
				if (wanpipe_get_rx_hdlc_errors(slot->hdlc_eng) >= err_limit) {
                 	gerr++;
					status++;
				}

				if (slot->stats.errors >= err_limit) {
                 	gerr++;
					status++;
				}

				if (status) {
					sprintf(str_status,"failed");
				} else {
					sprintf(str_status,"passed");
				}
				
				printf("|name=%-05s|span=%02i|chan=%02i|rx_frm=%04i|rx_hdlc_pkt=%04i|hdlc_err=%04i|h_c=%04i|h_a=%04i|h_f=%04i|drv_err=%04i|d_fifo=%04i|status=%s|\n",
					slot->if_name,
					slot->span,slot->chan,
					slot->frames, 
					wanpipe_get_rx_hdlc_packets(slot->hdlc_eng),	
					wanpipe_get_rx_hdlc_errors(slot->hdlc_eng),	
					slot->hdlc_eng->decoder.stats.crc,
					slot->hdlc_eng->decoder.stats.abort,
					slot->hdlc_eng->decoder.stats.frame_overflow,
					slot->stats.errors,
					slot->stats.rx_fifo_errors,
					str_status);

				if (slot->stats.errors) {
					print_stats(slot);
				}
		}

		if (slot->sock){
			close(slot->sock);
			slot->sock=-1;
		}
	
		if (slot->hdlc_eng) {
			wanpipe_unreg_hdlc_engine(slot->hdlc_eng);
		}
	}

	printf("\n");
	if (gerr == 0) {
     	printf("|result=passed|\n");
	} else {
     	printf("|result=failed|\n");
	}

	/* Wait for the clear call children */
	return (gerr != 0) ? -1 : 0;
};
