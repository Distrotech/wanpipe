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

#define MAX_TX_DATA     15000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	15000

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

void print_packet(unsigned char *buf, int len)
{
	int x;

    if (buf[0]==126 && buf[1]==126) {
	 	return;
	}

	printf("{  | ");
	for (x=0;x<len;x++){
		if (x && x%24 == 0){
			printf("\n  ");
		}
		if (x && x%8 == 0)
			printf(" | ");
		printf("%02X ",buf[x]);
	}
	printf("}\n");
}

static int sample_time_test(void){

	struct timeval last;
	static struct timeval tv_start = {0};
	static int elapsed_max=0;
	static int elapsed_min=0;
	int elapsed;
	int err;

	last=tv_start;
	gettimeofday(&tv_start, NULL);
	elapsed = abs((((last.tv_sec * 1000) + last.tv_usec / 1000) - ((tv_start.tv_sec * 1000) + tv_start.tv_usec / 1000)));
	err = abs(20-elapsed);
	if (err > 1 && err < 1000) {
		if (elapsed > elapsed_max) {
			elapsed_max = elapsed;
		}
		if (elapsed_min > elapsed) {
         	elapsed_min = elapsed;
		}
		printf("wanpipe: Elapsed %i  diff=%i max=%i min=%i\n", 
			elapsed,abs(20-elapsed),elapsed_max,elapsed_min);
	}

	return 0;
}


/***************************************************
* HANDLE SOCKET 
*
*   o Read a socket 
*   o Cast data received to api_rx_element_t data type 
*   o The received packet contains 16 bytes header 
*
*	------------------------------------------
*      |  sizeof(wp_api_hdr_t) bytes      |        X bytes        ...
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
* 		unsigned char	reserved[sizeof(wp_api_hdr_t)];
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
* 		unsigned char	reserved[sizeof(wp_api_hdr_t)];
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
	wp_tdm_api_rx_element_t* api_rx_el;
	wp_tdm_api_tx_element_t * api_tx_el;
	fd_set 	ready,write,oob;
	int err,i;
	
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

	/* Initialize the Tx Data buffer */
	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(wp_tdm_api_rx_hdr_t));

	/* Cast the Tx data packet with the tx element
	 * structure.  We must insert a sizeof(wp_api_hdr_t) byte
	 * driver header, which driver will remove 
	 * before passing packet out the physical port */
	api_tx_el = (wp_tdm_api_tx_element_t*)&Tx_data[0];
	

	/* Create a Tx packet based on user info, or
	 * by deafult incrementing number starting from 0 */
	for (i=0;i<Tx_length;i++){
		if (tx_data == -1){
			api_tx_el->data[i] = (unsigned char)(i%24);
		}else{
#if 0
			api_tx_el->data[i] = (unsigned char)tx_data+(i%4);
#else
			api_tx_el->data[i] = (unsigned char)tx_data;
#endif
		}
	}

    print_packet(api_tx_el->data,Tx_length);

	/* Main Rx Tx OOB routine */
	for(;;) {	

		/* Initialize all select() descriptors */
		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_ZERO(&oob);
		FD_SET(dev_fd,&oob);
		FD_SET(dev_fd,&ready);

		if (write_enable){
		     FD_SET(dev_fd,&write);
		}

		/* Select will block, until:
		 * 	1: OOB event, link level change
		 * 	2: Rx data available
		 * 	3: Interface able to Tx */
		
  		if(select(dev_fd + 1,&ready, &write, &oob, NULL)){

			fflush(stdout);	
		   	if (FD_ISSET(dev_fd,&oob)){
		
				/* An OOB event is pending, usually indicating
				 * a link level change */
				
				err=sangoma_tdm_read_event(dev_fd,&tdm_api);

				if(err < 0 ) {
					printf("Failed to receive OOB %i , %i\n", Rx_count, err);
					err = ioctl(dev_fd,SIOC_WANPIPE_SOCK_STATE,0);
					printf("Sock state is %s\n",
							(err == 0) ? "CONNECTED" : 
							(err == 1) ? "DISCONNECTED" :
							 "CONNECTING");
					break;
				}
					
				printf("GOT OOB EXCEPTION CMD Exiting\n");
			}
		  
			
          	if (FD_ISSET(dev_fd,&ready)){

				/* An Rx packet is pending
				 * 	1: Read the rx packet into the Rx_data
				 * 	   buffer. Confirm len > 0
				 *
				 * 	2: Cast Rx_data to the api_rx_element.
				 * 	   Thus, removing a sizeof(wp_api_hdr_t) byte header
				 * 	   attached by the driver.
				 *
				 * 	3. Check error_flag:
				 * 		CRC,Abort..etc 
				 */

				memset(Rx_data, 0, sizeof(Rx_data));


				err = sangoma_readmsg_tdm(dev_fd,
							Rx_data, 
							sizeof(wp_tdm_api_rx_hdr_t),
							&Rx_data[sizeof(wp_tdm_api_rx_hdr_t)],
							MAX_RX_DATA, 0);   

				if (!read_enable){
					goto bitstrm_skip_read;
				}

				//sample_time_test();

				/* err indicates bytes received */
				if(err <= 0) {
					printf("\nError receiving data %s\n",strerror(errno));
					break;
				}

				api_rx_el = (wp_tdm_api_rx_element_t*)&Rx_data[0];

				/* Check the packet length */
				Rx_lgth = err;
				if(Rx_lgth<=0) {
					printf("\nShort frame received (%d)\n",
						Rx_lgth);
					return;
				}

#if 0
				if (api_rx_el->data[0] == tx_data && api_rx_el->data[1] == (tx_data+1)){
					if (!stream_sync){
						printf("GOT SYNC %x\n",api_rx_el->data[0]);
					}
					stream_sync=1;
				}else{
					if (stream_sync){
						printf("OUT OF SYNC: %x\n",api_rx_el->data[0]);
					}
				}
#endif					

				++Rx_count;

				if (verbose){
				   // printf("Received %i Length = %i\n", 
					 //   	Rx_count,Rx_lgth);
#if 1
     				print_packet(api_rx_el->data,Rx_lgth);
#else
					//printf("Data: \n");
					for(i=0;i<Rx_lgth; i ++) {
						if (i && i%24==0) {
							printf("\n");
						}
						printf("%02d ", api_rx_el->data[i]);
					}
					printf("\n");
#endif
				}else{
					//putchar('R');
				}


#if 0
				if (Rx_count == 100){
					sangoma_tdm_set_codec(dev_fd,&tdm_api,WP_SLINEAR);
					sangoma_get_full_cfg(dev_fd, &tdm_api);
				}   

				if (Rx_count == 200){
					sangoma_tdm_set_codec(dev_fd,&tdm_api,WP_NONE);
					sangoma_get_full_cfg(dev_fd, &tdm_api);
				}    

			   	if (Rx_count == 300){
					sangoma_tdm_set_usr_period(dev_fd,&tdm_api,20);
					sangoma_get_full_cfg(dev_fd, &tdm_api);
				}   

				if (Rx_count == 400){
					sangoma_tdm_set_codec(dev_fd,&tdm_api,WP_SLINEAR);
					sangoma_get_full_cfg(dev_fd, &tdm_api);
				}    
                
				if (Rx_count == 500){
					sangoma_tdm_set_usr_period(dev_fd,&tdm_api,10);
					sangoma_get_full_cfg(dev_fd, &tdm_api);
				}   
				
				memset(Rx_data,0,Rx_lgth+sizeof(wp_tdm_api_rx_hdr_t));
				sangoma_writemsg_tdm(dev_fd,
                                     Rx_data,sizeof(wp_api_hdr_t), 
					               	 &Rx_data[sizeof(wp_api_hdr_t)], Rx_lgth, 
						       		 0);

#endif				
#if 0
				if (Rx_count == 200){
					sangoma_tdm_set_codec(dev_fd,WP_SLINEAR);
					sangoma_get_full_cfg(dev_fd);
				}

				if (Rx_count == 400){
					sangoma_tdm_set_hw_period(dev_fd,20);
					sangoma_get_full_cfg(dev_fd);
				}

				if (Rx_count == 600){
					sangoma_tdm_set_codec(dev_fd,WP_NONE);
					sangoma_get_full_cfg(dev_fd);
				}
#endif				



				if (rx_cnt > 0  && Rx_count >= rx_cnt){
					break;
				}
bitstrm_skip_read:
;
		   	}
		
		   	if (FD_ISSET(dev_fd,&write)){
				wp_api_hdr_t *hdr=&Tx_data[0];

				err = sangoma_writemsg_tdm(dev_fd,
						       Tx_data,sizeof(wp_api_hdr_t), 
						       &Tx_data[sizeof(wp_api_hdr_t)], Tx_length, 
						       0);
				if (err <= 0){
					if (errno == EBUSY){
						if (verbose){
							printf("Sock busy try again!\n");
						}
						/* Socket busy try sending again !*/
					}else{
						printf("Faild to send %i \n",errno);
						perror("Send: ");
						break;
					}
				}else{

					++Tx_count;
					
					if (verbose){
						printf("Packet sent: Sent %i : %i  txq=%i txq_len=%i\n",
							err,Tx_count,hdr->wp_api_tx_hdr_max_queue_length, hdr->wp_api_tx_hdr_number_of_frames_in_queue);
					}else{
						//putchar('T');
					}
				}
		   	}

			if (tx_delay){
				usleep(tx_delay);
			}

			if (tx_cnt && tx_size && Tx_count >= tx_cnt && !(files_used & TX_FILE_USED)){
			
				write_enable=0;
				if (rx_cnt > 0){
					/* Dont break let rx finish */
				}else{
					break;
				}
			}
		}
	}

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}
	close (dev_fd);
} 

int rbs_event (int fd, unsigned char rbs_bits)
{
	printf("%d: GOT RBS EVENT: RBS BITS=0x%X\n",
			fd,rbs_bits);
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
	
	dev_fd =-1;
	  
	dev_fd = sangoma_open_tdmapi_span_chan(atoi(card_name),atoi(if_name));
	if( dev_fd < 0){
		printf("Failed to open span chan %i %i\n",atoi(card_name),atoi(if_name));
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



