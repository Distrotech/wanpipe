/*****************************************************************************
* bstrm_hdlc_test_multi.c: Multiple Bstrm Test Receive Module
*
* Author(s):    Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2006 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
* 	The chdlc_api.c utility will bind to a socket to a chdlc network
* 	interface, and continously tx and rx packets to an from the sockets.
*
*	This example has been written for a single interface in mind, 
*	where the same process handles tx and rx data.
*
*	A real world example, should use different processes to handle
*	tx and rx spearately.  
*/



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
#include <libsangoma.h>
#include <wanpipe_hdlc.h>

#define FALSE	0
#define TRUE	1

/* Enable/Disable tx of random frames */
#define RAND_FRAME 1

#define MAX_NUM_OF_TIMESLOTS  31*16

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     1000 //MAX_NUM_OF_TIMESLOTS*10	/* Size of tx data */
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
	char if_name[MAX_IF_NAME+1];
	
} timeslot_t;


timeslot_t tslot_array[MAX_NUM_OF_TIMESLOTS];

int tx_change_data=0, tx_change_data_cnt=0;
int end=0;


void print_packet(unsigned char *buf, int len)
{
	int x;
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

void sig_end(int sigid)
{
	printf("%d: Got Signal %i\n",getpid(),sigid);
	end=1;
}        


static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
int myrand(int max_rand) {
next = next * 1103515245 + 12345;
return((unsigned)(next/65536) % max_rand);
}

void mysrand(unsigned seed) {
next = seed;
}


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

int MakeConnection(timeslot_t *slot, char *router_name ) 
{

	int span,chan;
	sangoma_span_chan_fromif(slot->if_name,&span,&chan);
	
	if (span > 0 && chan > 0) {
		wanpipe_tdm_api_t tdm_api;
             	slot->sock = sangoma_open_tdmapi_span_chan(span,chan);
	     	if( slot->sock < 0 ) {
      			perror("Open Span Chan: ");
      			return( FALSE );
		}

                sangoma_tdm_set_codec(slot->sock,&tdm_api,WP_NONE);
		 
		
		printf("Socket bound to Span=%i Chan=%i\n\n",span,chan);
		return (TRUE);
	}
	
	return(FALSE);
}

int  api_tdm_fe_alarms_callback(int fd, unsigned char alarm)
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
        	printf ("FE ALARMS Call back found device %i Alarm=%i \n", 
				fd,alarm);  	

	} else {
		printf ("FE ALARMS Call back no device Alarm=%i\n",alarm);
	}

	return 0;
}



#define ALL_OK  	0
#define	BOARD_UNDERRUN 	1
#define TX_ISR_UNDERRUN 2
#define UNKNOWN_ERROR 	4

void process_con_rx(void) 
{
	unsigned int Rx_count;
	fd_set 	oob,ready;
	int err,serr,i,slots=0;
	unsigned char Rx_data[1600];
	int error_bit=0, error_crc=0, error_abort=0, error_frm=0;
	struct timeval tv;
	int frame=0;
	int max_fd=0, packets=0;
	timeslot_t *slot=NULL;
	wanpipe_tdm_api_t tdm_api;

	memset(&tdm_api,0,sizeof(tdm_api));

	tdm_api.wp_tdm_event.wp_fe_alarm_event = &api_tdm_fe_alarms_callback;
	
	
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
	
	
	tv.tv_usec = 0;
	tv.tv_sec = 10;
	
        Rx_count = 0;

	for(;;) {
	
		FD_ZERO(&ready);
		FD_ZERO(&oob);
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
		
		tv.tv_usec = 0;
		tv.tv_sec = 10;

		if (end){
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
  		if((serr=select(max_fd + 1, &ready, NULL, &oob, &tv))){

		    for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
	
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

				/* err indicates bytes received */
				if(err > 0) {
#if 0
					sangoma_writemsg_tdm(tslot_array[i==1?2:1].sock, 
							 Rx_data,sizeof(wp_tdm_api_rx_hdr_t), 
							 &Rx_data[sizeof(wp_tdm_api_rx_hdr_t)],
							 err, 0);
#else
					sangoma_writemsg_tdm(tslot_array[i].sock, 
							 Rx_data,sizeof(wp_tdm_api_rx_hdr_t), 
							 &Rx_data[sizeof(wp_tdm_api_rx_hdr_t)],
							 err, 0);

#endif
				} else {
					printf("\n%s: Error receiving data\n",slot->if_name);
				}

			 } /* If rx */
			 
		    } /* for all slots */
		
		  
		} else {
			printf("\n: Error selecting rx socket rc=0x%x errno=0x%x\n",
				serr,errno);
			perror("Select: ");
			//break;
		}
	}
	

	printf("\nRx Unloading HDLC\n");
	
} 

 


/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call process_con() to read/write the socket 
 *
 **************************************************************/  

int main(int argc, char* argv[])
{
	int proceed;
	char router_name[20];	
	int x,i;
	
	if (argc < 1){
		printf("Usage: rec_wan_sock <router name> <interface name> ...\n");
		exit(0);
	}	
	
	nice(-11);
	
	signal(SIGINT,&sig_end);
	signal(SIGTERM,&sig_end);
	
	memset(&tslot_array,0,sizeof(tslot_array));
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
		tslot_array[i].sock=-1;
	}

	strncpy(router_name,argv[1],(sizeof(router_name)-1));	
	
	
	for (x=1;x<argc-1;x++) {	

		strncpy(tslot_array[x].if_name, argv[x+1], MAX_IF_NAME);		
		printf("Connecting to IF=%s\n",tslot_array[x].if_name);
	
#if 1		
		proceed = MakeConnection(&tslot_array[x],router_name);
		if( proceed == TRUE ){

			printf("Creating %s with tx data 0x%x : Sock=%i : x=%i\n",
				tslot_array[x].if_name,
				tslot_array[x].data,
				tslot_array[x].sock,
				x);
		}else{
			if (tslot_array[x].sock){
				close(tslot_array[x].sock);
			}
			tslot_array[x].sock=-1;
		}
#endif
	}
	
 	process_con_rx();
	
	for (x=0;x<MAX_NUM_OF_TIMESLOTS;x++){
		if (tslot_array[x].sock){
			close(tslot_array[x].sock);
			tslot_array[x].sock=0;
		}
	}
	
	/* Wait for the clear call children */
	return 0;
};
