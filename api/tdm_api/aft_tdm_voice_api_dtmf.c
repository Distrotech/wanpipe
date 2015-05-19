/*****************************************************************************
* aft_api.c	AFT T1/E1: HDLC API Sample Code
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_cfg.h>
#include <linux/wanpipe.h>
#include <libsangoma.h>
#include "lib_api.h"

#define MAX_TX_DATA     5000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	5000

unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];
unsigned char Tx_data[MAX_TX_DATA + sizeof(wp_tdm_api_rx_hdr_t)];

/* Prototypes */
int MakeConnection(void);
void handle_span_chan( void);
void sig_end(int sigid);

int dev_fd;
FILE *tx_fd=NULL,*rx_fd=NULL;	
wanpipe_tdm_api_t tdm_api; 


/***************************************************
* HANDLE SOCKET 
*
*   o Read a socket 
*   o Cast data received to api_rx_element_t data type 
*   o The received packet contains 16 bytes header 
*
*	------------------------------------------
*      |  16 bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
* o Data structures:
* ------------------
* typedef struct {
* 	union {
* 		struct {
* 			unsigned char	_event_type;
* 			unsigned char	_rbs_rx_bits;
* 			unsigned int	_time_stamp;
* 		}wp_event;
* 		struct {
* 			unsigned char	_rbs_rx_bits;
* 			unsigned int	_time_stamp;
* 		}wp_rx;
* 		unsigned char	reserved[16];
* 	}wp_rx_hdr_u;
* #define wp_api_event_type		wp_rx_hdr_u.wp_event._event_type
* #define wp_api_event_rbs_rx_bits 	wp_rx_hdr_u.wp_event._rbs_rx_bits
* #define wp_api_event_time_stamp 	wp_rx_hdr_u.wp_event._time_stamp
* } wp_tdm_api_rx_hdr_t;
* 
* typedef struct {
*         wp_tdm_api_rx_hdr_t	hdr;
*         unsigned char  		data[1];
* } wp_tdm_api_rx_element_t;
* 
* typedef struct {
* 	union {
* 		struct {
* 			unsigned char	_rbs_rx_bits;
* 			unsigned int	_time_stamp;
* 		}wp_tx;
* 		unsigned char	reserved[16];
* 	}wp_tx_hdr_u;
* #define wp_api_time_stamp 	wp_tx_hdr_u.wp_tx._time_stamp
* } wp_tdm_api_tx_hdr_t;
* 
* typedef struct {
*         wp_tdm_api_tx_hdr_t	hdr;
*         unsigned char  		data[1];
* } wp_tdm_api_tx_element_t;
*                     
* #define WPTDM_A_BIT 0x08
* #define WPTDM_B_BIT 0x04
* #define WPTDM_C_BIT 0x02
* #define WPTDM_D_BIT 0x01
*
*/

void handle_span_chan(void) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	int err;
	
#if 0
	int rlen;
	int stream_sync=0;
#endif
	
	Rx_count = 0;
	Tx_count = 0;

	if (tdm_api.wp_tdm_cmd.hdlc) {
                Tx_length = tx_size;
	} else {
		Tx_length = tdm_api.wp_tdm_cmd.usr_mtu_mru;
	}
	
	printf("\n\nSocket Handler: Rx=%d Tx=%i TxCnt=%i TxLen=%i TxDelay=%i\n",
			read_enable,write_enable,tx_cnt,tx_size,tx_delay);	

    	sangoma_tdm_enable_dtmf_events(dev_fd, &tdm_api);

	/* Main Rx Tx OOB routine */
	for(;;) {	

		err = sangoma_socket_waitfor(dev_fd, 1000, POLLPRI);
		printf("ret:%d\n", err);
		if (err){
			err=sangoma_tdm_read_event(dev_fd,&tdm_api);
			if(err < 0 ) {
				printf("Failed to receive EVENT %d\n", err);
				break;
			}
				
			printf("GOT OOB EXCEPTION CMD Exiting\n");
		}
		
		if (++Rx_count >= rx_cnt){
			break;
		}

	}

    	sangoma_tdm_disable_dtmf_events(dev_fd, &tdm_api);
	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}
	close (dev_fd);

	return;
} 

int dtmf_event (int fd, unsigned char digit, unsigned char type, unsigned char port)
{
	printf("%d: DTMV Event: %c (%s:%s)!\n",
			fd,
			digit,
			(port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
			(type == WAN_EC_TONE_PRESENT)?"PRESET":"STOP");		
	return 0;
}


/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call handle_span_chan() to read the socket 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int proceed;

	proceed=init_args(argc,argv);
	if (proceed != WAN_TRUE){
		usage(argv[0]);
		return -1;
	}

	signal(SIGINT,&sig_end);
	memset(&tdm_api,0,sizeof(tdm_api));
	tdm_api.wp_tdm_event.wp_dtmf_event = &dtmf_event;
	
	printf("TDM DTMF PTR = %p\n",tdm_api.wp_tdm_event.wp_dtmf_event);
	
	dev_fd =-1;

	dev_fd = sangoma_open_tdmapi_span_chan(atoi(card_name),atoi(if_name));
	if( dev_fd < 0){
		printf("Failed to open span chan(%s:%d,%s:%d)\n",
				card_name,atoi(card_name),if_name,atoi(if_name));
		exit (1);
	}
	printf("HANDLING SPAN %i CHAN %i FD=%i\n",
		atoi(card_name),atoi(if_name),dev_fd);
		
	sangoma_tdm_set_codec(dev_fd,&tdm_api,WP_NONE);
	sangoma_get_full_cfg(dev_fd, &tdm_api);
		
	handle_span_chan();
	close(dev_fd);
	return 0;

	return 0;
};


void sig_end(int sigid)
{

	printf("Got Signal %i\n",sigid);
    	
	sangoma_tdm_disable_dtmf_events(dev_fd, &tdm_api);

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}

	if (dev_fd){
		close (dev_fd);
	}

	exit(1);
}



