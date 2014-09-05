/*****************************************************************************
* chdlc_api.c	CHDLC API: Receive Module
*
* Author(s):	Gideon Hack & Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/wanpipe.h>
#include <linux/sdla_chdlc.h>
#include "lib_api.h"

#define FALSE	0
#define TRUE	1

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     5000	/* Size of tx data */  
#define MAX_RX_DATA 	MAX_TX_DATA


static unsigned char test_data[]={0x00,0x5A, 0xAA, 0xA5, 0xFF};
static unsigned char tx_index=0;

/*===================================================
 * Golobal data
 *==================================================*/

unsigned short no_CRC_bytes_Rx;
unsigned char HDLC_streaming = FALSE;
unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];
unsigned char Tx_data[MAX_TX_DATA];
int 	sock;


/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(void);
void handle_socket( void);


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

int MakeConnection(void) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return(FALSE);
   	} /* if */
  
	printf("\nConnecting to router %s, interface %s\n", card_name, if_name);

	strcpy( sa.sll_device, if_name);
	strcpy( sa.sll_card, card_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;

        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",if_name);
                exit(0);
        }
	printf("Socket bound to %s\n\n",if_name);

	return(TRUE);

}

/*===========================================================
 * handle_socket 
 *
 *   o Tx/Rx data to and from the socket 
 *   o Cast received data to an api_rx_element_t data type 
 *   o The received packet contains 16 bytes header 
 *
 *	------------------------------------------
 *      |  16 bytes      |        X bytes        ...
 *	------------------------------------------
 * 	   Header              Data
 *
 * RX DATA:
 * --------
 * 	Each rx data packet contains the 16 byte header!
 * 	
 *   	o Rx 16 byte data structure:
 *
 *		typedef struct {
 *       	 	unsigned char   error_flag      PACKED;
 *        		unsigned short  time_stamp      PACKED;
 *        		unsigned char   reserved[13]    PACKED;
 *		} api_rx_hdr_t; 
 *
 *		typedef struct {
 *        		api_rx_hdr_t    api_rx_hdr      PACKED;
 *        		void *          data            PACKED;
 *		} api_rx_element_t;
 *
 * 	error_flag:
 * 		bit 0: 	incoming frame was aborted
 * 		bit 1: 	incoming frame has a CRC error
 * 		bit 2:  incoming frame has an overrun eror 
 *
 * 	time_stamp:
 * 		absolute time value in ms.
 *
 * TX_DATA:
 * --------
 *	Each tx data packet MUST contain a 16 byte header!
 *
 * 	o Tx 16 byte data structure
 * 	
 *		typedef struct {
 *			unsigned char 	attr		PACKED;
 *			unsigned char  	reserved[15]	PACKED;
 *		} api_tx_hdr_t;
 *
 *		typedef struct {
 *			api_tx_hdr_t 	api_tx_hdr	PACKED;
 *			void *		data		PACKED;
 *		} api_tx_element_t;
 *
 *	Currently the chdlc device driver doesn't use any of
 *	the above fields.  Thus, the user can set the 16 bytes
 *	to ZERO.
 * 
 */

void handle_socket(void) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	api_rx_element_t* api_rx_el;
	fd_set 	ready,oob;
	int err,i;
	void *pRx_data,*pTx_data;
	int no_CRC_bytes_Rx = LGTH_CRC_BYTES;


	pRx_data = (void*)&Rx_data[sizeof(api_rx_hdr_t)];
	pTx_data = (void*)&Tx_data[sizeof(api_tx_hdr_t)]; 

        Rx_count = 0;
	Tx_count = 0;
	Tx_length = tx_size;

	printf("\n\nSocket Handler: Rx=%d Tx=%i TxCnt=%i TxLen=%i TxDelay=%i RxCnt=%i\n",
			read_enable,write_enable,tx_cnt,tx_size,tx_delay,rx_cnt);	
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  
	 */
	for(;;) {	
		FD_ZERO(&ready);
		FD_ZERO(&oob);
		FD_SET(sock,&oob);
		FD_SET(sock,&ready);

		fflush(stdout);
  		if(select(sock + 1,&ready, NULL, &oob, NULL)){
	
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
					
				printf("Got OOB exception: Link Down !\n");
				break;
			}
		  
			
                   	if (FD_ISSET(sock,&ready)){

				err = recv(sock, Rx_data, MAX_RX_DATA, 0);

				/* err indicates bytes received */
				if (err > 0){

					api_rx_el = (api_rx_element_t*)&Rx_data[0];

		                	/* Check the packet length */
                                	Rx_lgth = err - sizeof(api_rx_hdr_t)-no_CRC_bytes_Rx;
                                	if(Rx_lgth<=0) {
                                        	printf("\nShort frame received (%d)\n",
                                                	Rx_lgth);
                                        	return;
					}

					switch (api_rx_el->api_rx_hdr.wan_hdr_hdlc_error_flag){
				
					case 0:
						/* Rx packet is good */
						break;
						
					case RX_FRM_ABORT:
						/* Frame was aborted */
						break;
					case RX_FRM_CRC_ERROR:
						/* Frame has crc error */
						break;
					case RX_FRM_OVERRUN_ERROR:
						/* Frame has an overrun error */
						break;
					default:
						/* Error with the rx packet 
						 * handle it ... */
						break;
					}

					++Rx_count;

					for (i=0;i<Rx_lgth;i++){
						if (((unsigned char*)api_rx_el->data)[i] != 
						    test_data[tx_index]){
							printf("Rx Error cnt=%i: Packet corruption on offset %i\n",
						      		Rx_count,i);
							goto process_exit;
						}
					}
					
					if (Rx_count == tx_cnt){
						Rx_count=0;
						tx_index++;
						if (tx_index >=sizeof(test_data)){
							tx_index=0;
						}
					}	

					err = send(sock,Rx_data,err-2,0);

					if (err <= 0){
						printf("Failed to Tx Echo Data!\n");
					}

				} else {
					printf("\nError receiving data\n");
					break;
				}
		   	}
		}
	}
process_exit:
	close (sock);
} 

/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call handle_socket() to read/write the socket 
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

	proceed = MakeConnection();
	if(proceed == WAN_TRUE){
		handle_socket();
		return 0;
	}

	return 1;
};
