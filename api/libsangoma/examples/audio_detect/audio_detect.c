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

#define FALSE	0
#define TRUE	1

/* Enable/Disable tx of random frames */
#define RAND_FRAME 0

#define MAX_NUM_OF_TIMESLOTS  32*32

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
	int hdlc;
	char if_name[MAX_IF_NAME+1];
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
static int skip_data[100];
static int skip_byte_cnt=0;
pthread_mutex_t g_lock;


static int log_printf(char *fmt, ...)
{
	char *data;
	int ret = 0;
	va_list ap;
	char date[200] = "";
	struct tm now;
	time_t epoch;


	if (!verbose) {
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


#if 1
static void print_packet(timeslot_t *slot, unsigned char *buf, int len)
{
	int x;
	int print_packet=0;
	int i;
	unsigned char data[]={0x55,0xD5,0xFF};

	if (!len) {
     	return;
	}


	if (!verbose) {
    for (x=0;x<len;x++){         

        if (skip_byte_cnt) {
			for (i=0;i<skip_byte_cnt;i++) {
				if (skip_data[i] == buf[x]) {
					break;
				}
				print_packet++;
			}

			if (print_packet >= skip_byte_cnt) {
				break;
			}

		}  else {

			for (i=0;i<sizeof(data);i++) {
				if (data[i] == buf[x]) {
					break;
				}
				print_packet++;
			}

			if (print_packet >= sizeof(data)) {
				break;
			}
		}

		print_packet=0;
	}

	if (!print_packet) {
     	return;
	}
	}

	printf("|span=%i|chan=%i|len=%04i| ",slot->span,slot->chan,len);
	for (x=0;x<len;x++){
		printf("%02x",buf[x]);
	}
	printf("|\n");
}
#endif

static void sig_end(int sigid)
{
	//printf("%d: Got Signal %i\n",getpid(),sigid);
	app_end=1;
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
    	slot->sock = __sangoma_open_tdmapi_span_chan(span,chan);
	    if( slot->sock < 0 ) {
      			perror("Open Span Chan: ");
				gerr++;
      			return( FALSE );
		}
		
		sangoma_get_full_cfg(slot->sock, &tdm_api);


		slot->hdlc = tdm_api.wp_tdm_cmd.hdlc;

	    log_printf("Socket bound to Span=%i Chan=%i\n\n",span,chan);
		slot->chan=chan;
		slot->span=span;
		return (TRUE);
	}
	    
	printf("Error: Failed to bind If=%s Span=%i Chan=%i\n\n",slot->if_name,span,chan);


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
	struct timeval tv;
	int frame=0,print_status=0;
	int max_fd=-1;
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
		
		if (tslot_array[i].sock > max_fd){
			max_fd=tslot_array[i].sock;
		}
	}
	i=0;

	if (max_fd == -1) {
     	gerr++;
		printf("Error: No devices defined\n");
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
  		if((serr=select(max_fd + 1, &ready, NULL, &oob, &tv) >= 0)){
					
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

					print_packet(slot,rx_frame,len);           

				} else {
					if (app_end) {
             			break;
					}
					log_printf("\n%s: Error receiving data\n",slot->if_name);
					gerr++;
				}

			 } /* If rx */
			 
		    } /* for all slots */
		
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

    pthread_mutex_init(&g_lock, NULL);  
	
	if (argc < 2){
		printf("Usage: audio_detect  <dev name> ...\n");
		printf("    eg: ./audio_detect s1c1 s1c2 s1c3 ... -vebose -timeout <sec> -skip_byte 0xD5 -skip_byte 0x55 ..\n");
		exit(1);
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
		if (strcmp(argv[x+1], "-skip_byte") == 0) {
			x++;
			if (skip_byte_cnt < 100) {
         		sscanf(argv[x+1],"%x",&skip_data[skip_byte_cnt]);
				log_printf("Skip Byte = 0x%x\n",skip_data[skip_byte_cnt]);
            	skip_byte_cnt++;
			}
			continue;
		}

		sangoma_span_chan_fromif(argv[x+1],&span,&chan);
		if (span > 0 && span <= 32 && chan > 0 && chan < 32) {
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
			if (slot->hdlc) {
				log_printf("Skipping DCHAN %s x=%i [s%dc%i]\n",
					slot->if_name,
					x,
					slot->span,slot->chan);
             	close(slot->sock);
				slot->sock=-1;
			} else {
				log_printf("Creating %s with tx data 0x%x : Sock=%i : x=%i [s%dc%i]\n",
					slot->if_name,
					slot->data,
					slot->sock,
					x,
					slot->span,slot->chan);

				slot->active=1;
				slot->running=1;
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

	if (gerr==0) {
 		process_con_rx();
		app_end=1;
	} else {
     	app_end=1;
		sleep(1);
	}


	/* Wait for the clear call children */
	return (gerr != 0) ? -1 : 0;
};
