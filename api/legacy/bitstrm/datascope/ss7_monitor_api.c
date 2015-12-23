/*****************************************************************************
* ss7_monitor_api.c
* 		
* 		SS7 Monitor API: Sample Module
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2000-2003 Sangoma Technologies Inc.
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
#include <time.h>
#include <linux/if.h>
#include <linux/wanpipe.h>
#include <linux/if_wanpipe.h>
#include "lib_api.h"

#define MAX_TX_DATA     5000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	5000

unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];

/* Prototypes */
int MakeConnection(void);
void handle_socket( void);
void sig_end(int sigid);

int sock;
FILE *tx_fd=NULL,*rx_fd=NULL;	

#if 0
void check_for_zeros(unsigned char *data, int len, int cnt, int bytes);
#endif

/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*       (Interface name is supplied by the user)
*/         

int MakeConnection(void) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return( WAN_FALSE );
   	} /* if */
  
	printf("\nConnecting to router %s, interface %s prot %x active ch %x\n", 
			card_name, 
			if_name,htons(SS7_MONITOR_PROT),
			tx_cnt);
	
	strcpy(sa.sll_device, if_name);
	strcpy(sa.sll_card, card_name);
	sa.sll_prot = ds_prot;
	sa.sll_prot_opt = ds_prot_opt;
	sa.sll_mult_cnt = ds_max_mult_cnt;
	sa.sll_active_ch = ds_active_ch;

	/* sa.sll_seven_bit_hdlc
	 * 0: Use 8 bit hdlc engine
	 * 1: Use 7 bit hdlc engine */
	sa.sll_seven_bit_hdlc = ds_7bit_hdlc;
	sa.sll_protocol = htons(SS7_MONITOR_PROT);

	sa.sll_family=AF_WANPIPE;
	
        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",if_name);
                exit(0);
        }

	printf("Socket bound to %s\n\n",if_name);
	
	return(WAN_TRUE);

}

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
*   o Data structures:
*
*	typedef struct {
*        	unsigned char   SIO      	PACKED;
*        	unsigned short  time_stamp      PACKED;
*        	unsigned char   reserved[13]    PACKED;
*	} api_rx_hdr_t; 
*
*	typedef struct {
*        	api_rx_hdr_t    api_rx_hdr      PACKED;
*        	unsigned char   data[1]         PACKED;
*	} api_rx_element_t;
*
*
*/

void handle_socket(void) 
{
	unsigned int Rx_count;
	wp_sock_rx_element_t* api_rx_el;
	fd_set 	ready, oob;
	int err,i;
	void *pRx_data;
	time_t time_stamp;
	struct timeval time_gtd;

	pRx_data = (void*)&Rx_data[sizeof(wp_sock_rx_hdr_t)];

        Rx_count = 0;
	
	printf("\n\nSocket Handler: Rx=%d RxCnt=%i\n",
			read_enable,rx_cnt);	
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  
	 */


	if (files_used & RX_FILE_USED){
		rx_fd=fopen(rx_file,"w");
		if (!rx_fd){
			printf("Failed to open file %s\n",rx_file);
			perror("File: ");
			return;
		}
		printf("Opening %s rx file\n",rx_file);
	}

	
	for(;;) {	
		FD_ZERO(&ready);
		FD_ZERO(&oob);
		FD_SET(sock,&oob);
		FD_SET(sock,&ready);

		fflush(stdout);
  		if (select(sock + 1,&ready, NULL, &oob, NULL)){
	
		   	if (FD_ISSET(sock,&oob)){
	
				err = recv(sock, Rx_data, MAX_RX_DATA, MSG_OOB);

				if(err < 0 ) {
					printf("Failed to receive OOB %i , %i\n", Rx_count, err);
					err = ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0);
					printf("Sock state is %s\n",
							(err == 0) ? "CONNECTED" : 
							(err == 1) ? "DISCONNECTED" :
							 "CONNECTING");
					break;
				}
					
				printf("GOT OOB EXCEPTION CMD Exiting\n");

				break;
			}
		  
			
                   	if (FD_ISSET(sock,&ready)){
try_rx_again:
				err = recv(sock, Rx_data, MAX_RX_DATA, 0);

				if (!read_enable){
					goto bitstrm_skip_read;
				}
				
				/* err indicates bytes received */
				if (err > 0){

					api_rx_el = (wp_sock_rx_element_t*)&Rx_data[0];

		                	/* Check the packet length */
                                	Rx_lgth = err - sizeof(wp_sock_rx_hdr_t);
                                	if(Rx_lgth<=0) {
                                        	printf("\nShort frame received (%d)\n",
                                                	Rx_lgth);
                                        	return;
					}


					if (direction != -1 && 
					    direction != api_rx_el->api_rx_hdr.direction){
						goto bitstrm_skip_read;
					}
				
					gettimeofday(&time_gtd,NULL);
					time(&time_stamp);
					
					if ((files_used & RX_FILE_USED) && rx_fd){
						fprintf(rx_fd,"%s: Len=%i : AbsTime(sec:usec): %lu:%lu : Time Stamp: %s",
								api_rx_el->api_rx_hdr.direction?"TX":"RX",
								Rx_lgth,
								time_gtd.tv_sec,
								time_gtd.tv_usec,
								ctime(&time_stamp));
						for(i=0; i<Rx_lgth; i++) {
							fprintf(rx_fd,"%02X ", api_rx_el->data[i]);
						} 
						fprintf(rx_fd,"\n\n");
					}
				
					++Rx_count;

					if (verbose){
						printf("%s: Len=%i Count=%i Time Stamp: %s",
							api_rx_el->api_rx_hdr.direction?"TX":"RX",
							Rx_lgth,Rx_count,
							ctime(&time_stamp));

						for(i=0;i<Rx_lgth; i ++) {
							printf("%02X ", api_rx_el->data[i]);
						} 
						printf("\n\n");

					}else{
						if (!(Rx_count % 10)){
							if (api_rx_el->api_rx_hdr.direction){
								putchar('T');
							}else{
								putchar('R');
							}
						}
					}
			
					if (rx_cnt == Rx_count){
						printf("Rxcnt %i == RxCount=%i\n",rx_cnt,Rx_count);
						break;
					}

					goto try_rx_again;
				}
bitstrm_skip_read:

		   	}
		}
	}

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}

	close (sock);
} 



/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call handle_socket() to read the socket 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int proceed;

	read_enable=1;

	proceed=init_args(argc,argv);
	if (proceed != WAN_TRUE){
		usage(argv[0]);
		return -1;
	}

	signal(SIGINT,&sig_end);
	
	proceed = MakeConnection();
	if( proceed == WAN_TRUE){
		handle_socket();
		return 0;
	}

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

	if (sock){
		close (sock);
	}

	exit(1);
}

#if 0
void check_for_zeros(unsigned char *data, int len, int cnt, int bytes)
{
	int i;
	int zero_cnt=0;

	for (i=0;i<len;i++){
		if (data[i]==0){
			if (++zero_cnt>1){
				printf("\nSENDING MULTIPLE ZEROS: Cnt %i, Len %i Bytes %i\n",
						cnt,len,bytes);
			}
		}else{
			zero_cnt=0;
		}
	}

}
#endif
