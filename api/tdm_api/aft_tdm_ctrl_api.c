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

/* Prototypes */
void handle_span_chan(int);
void sig_end(int sigid);

int dev_fd;
FILE *tx_fd=NULL,*rx_fd=NULL;	
wanpipe_tdm_api_t tdm_api; 


/***************************************************
* HANDLE SOCKET 
*/

void handle_span_chan(int dev_fd) 
{
	fd_set 	oob;
	int err;

	/* Main Rx Tx OOB routine */
	for(;;) {	

		/* Initialize all select() descriptors */
		FD_ZERO(&oob);
		FD_SET(dev_fd,&oob);

		/* Select will block, until:
		 * 	1: OOB event, link level change
		 * 	2: Rx data available
		 * 	3: Interface able to Tx */
		err=select(dev_fd + 1,NULL, NULL, &oob, NULL);
			
  		if (err > 0){

			fflush(stdout);	
		   	if (FD_ISSET(dev_fd,&oob)){
				wp_tdm_api_event_t *rx_event;
		
				/* An OOB event is pending, usually indicating
				 * a link level change */
				
				err=sangoma_tdm_read_event(dev_fd,&tdm_api);

				if(err < 0 ) {
					printf("Failed to receive OOB %i\n",  err);
					break;
				} 

				rx_event = &tdm_api.wp_tdm_cmd.event;

				switch (rx_event->wp_tdm_api_event_type){

#if 0
				case WP_TDMAPI_EVENT_ALARM:
					printf("GOT ALARM EVENT 0x%0X Span=%i Chan=%i\n",
						rx_event->wp_tdm_api_event_alarm,rx_event->span,rx_event->channel);
					break;
#endif
				case WP_TDMAPI_EVENT_LINK_STATUS:
					printf("GOT LINK STATUS Span=%i Chan=%i State=%s\n",
						rx_event->span,rx_event->channel,
						rx_event->wp_tdm_api_event_link_status == WP_TDMAPI_EVENT_LINK_STATUS_CONNECTED ? "Connected" : "Disconnected");
					break;	
				default:			
					//printf("GOT UNKNOWN OOB %i \n",rx_event->wp_tdm_api_event_type);
					break;
				}
			}	
		}
		if (err < 0) {
			printf("Device Down!\n");
			fflush(stdout);	
			break;
		}
	}
} 

/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call handle_span_chan(int dev_fd) to read the socket 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int dev_fd;

	memset(&tdm_api,0,sizeof(tdm_api));
	
	dev_fd =-1;
	  
	dev_fd = sangoma_open_tdmapi_ctrl();
	if( dev_fd < 0){
		printf("Failed to open span chan\n");
		exit (1);
	}
		
	handle_span_chan(dev_fd);

	close(dev_fd);

	return 0;

};


