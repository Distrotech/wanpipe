/*****************************************************************************
* bscstrm_api.c	BiSync Streaming API: Receive Module
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
* 	The bscstrm_api.c utility will bind to a socket to a bisync network
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
#include <linux/sdla_bscstrm.h>
#include "lib_api.h"

#define FALSE	0
#define TRUE	1

#define MAX_TX_DATA     5000	/* Size of tx data */  
#define MAX_RX_DATA 	MAX_TX_DATA

#define BSCSTRM_TX_TIMEOUT 10 

FILE *tx_fd=NULL,*rx_fd=NULL;	


/*===================================================
 * Golobal data
 *==================================================*/

unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];
unsigned char Tx_data[MAX_TX_DATA];
int 	sock;

unsigned char ebcdic_pkt[]={0x32,0x32,0x32,0x2D,0xFF};

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
 * 		typedef struct {
 *			unsigned char	station		PACKED;
 *			unsigned short	time_stamp	PACKED;
 *			unsigned char	reserved[13]	PACKED;
 * 		} api_rx_hdr_t;
 *
 * 		typedef struct {
 *       		api_rx_hdr_t	api_rx_hdr      PACKED;
 *        		unsigned char 	data[1]    	PACKED;
 * 		} api_rx_element_t;
 *
 * 	station:
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
 *	 	typedef struct {
 *			unsigned char 	station		PACKED;
 *  			unsigned char   misc_tx_rx_bits PACKED;
 * 			unsigned char  	reserved[14]	PACKED;
 * 		} api_tx_hdr_t;
 *
 * 		typedef struct {
 *			api_tx_hdr_t 	api_tx_hdr	PACKED;
 * 			unsigned char	data[1]		PACKED;
 * 		} api_tx_element_t;
 *
 *	Currently the bisync device driver doesn't use any of
 *	the above fields.  Thus, the user can set the 16 bytes
 *	to ZERO.
 * 
 */

void handle_socket(void) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	api_rx_element_t* api_rx_el;
	api_tx_element_t * api_tx_el;
	fd_set 	ready,write,oob;
	int err,i;
	int rlen;
	int ferr;
	int txfile_bytes=0;
	void *pRx_data,*pTx_data;


	pRx_data = (void*)&Rx_data[sizeof(api_rx_hdr_t)];
	pTx_data = (void*)&Tx_data[sizeof(api_tx_hdr_t)]; 

        Rx_count = 0;
	Tx_count = 0;
	Tx_length = tx_size;

	printf("\n\nSocket Handler: Rx=%d Tx=%i TxCnt=%i TxLen=%i TxDelay=%i RxCnt=%i\n",
			read_enable,write_enable,tx_cnt,tx_size,tx_delay,rx_cnt);	
	
	memset(&Tx_data[0],0,MAX_TX_DATA);

	if (files_used & RX_FILE_USED){
		rx_fd=fopen(rx_file,"wb");
		if (!rx_fd){
			printf("Failed to open file %s\n",rx_file);
			perror("File: ");
			return;
		}
		printf("Opening %s rx file\n",rx_file);
	}

	
	if (files_used & TX_FILE_USED){

		tx_fd=fopen(tx_file,"rb");
		if (!tx_fd){
			printf("Failed to open file %s\n",tx_file);
			perror("File: ");
			return;
		}

		printf("Opening %s tx file\n",tx_file);
		
		rlen=fread(pTx_data,
			   sizeof(char),
			   Tx_length,tx_fd);

		
		if (!rlen){
			printf("%s: File empty!\n",
				tx_file);
			return;
		}
	}else{
		api_tx_el = (api_tx_element_t*)&Tx_data[0];

		for (i=0;i<Tx_length;i++){
			if (tx_data == -1){
				api_tx_el->data[i] = (unsigned char)i;
			}else{
				api_tx_el->data[i] = (unsigned char)tx_data;
			}
		}

		/* Hardcode packet to EBCDIC frame */
		//memcpy(api_tx_el->data,ebcdic_pkt,sizeof(ebcdic_pkt));
		//Tx_length=sizeof(ebcdic_pkt);
	}


	for(;;) {	
		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_ZERO(&oob);
		FD_SET(sock,&oob);
		FD_SET(sock,&ready);

		if (write_enable){
		     FD_SET(sock,&write);
		}

		fflush(stdout);
  		if(select(sock + 1,&ready, &write, &oob, NULL)){
	
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

				if (!read_enable){
					goto bitstrm_skip_read;
				}
				
				/* err indicates bytes received */
				if (err > 0){

					api_rx_el = (api_rx_element_t*)&Rx_data[0];

		                	/* Check the packet length */
                                	Rx_lgth = err - sizeof(api_rx_hdr_t);
                                	if(Rx_lgth<=0) {
                                        	printf("\nShort frame received (%d)\n",
                                                	Rx_lgth);
                                        	return;
					}

					if ((files_used & RX_FILE_USED) && rx_fd){
						ferr=fwrite(pRx_data,
						       	    sizeof(char),
						            Rx_lgth,
						            rx_fd);	
						if (ferr != Rx_lgth){
							printf("Error: fwrite failed: written=%i should have %i\n",
									ferr,Rx_lgth);
						}
					}
				
					++Rx_count;

					if (verbose){
						printf("Received %i Olen=%i Length = %i\n", 
								Rx_count, err,Rx_lgth);
					}else{
						putchar('R');
					}
#if 1	
					if (verbose){
						printf("Data: ");
						for(i=0;(i<Rx_lgth); i ++) {
							printf("0x%02X ", api_rx_el->data[i]);
						} 
						printf("\n");
					}
#endif
			
					if (rx_cnt && rx_cnt == Rx_count){
						printf("Rxcnt %i == RxCount=%i\n",rx_cnt,Rx_count);
						break;
					}

				} else {
					printf("\nError receiving data\n");
					break;
				}

		   	}
		
bitstrm_skip_read:
		   	if (FD_ISSET(sock,&write)){

				err = send(sock,Tx_data, Tx_length + sizeof(api_tx_hdr_t), 0);

				if (err <= 0){
					if (errno == EBUSY){

						/* Important: The user must implement
						 * a delay, if the socket is busy */
						usleep(BSCSTRM_TX_TIMEOUT);
						
						if (verbose){
							printf("Sock busy try again!\n");
						}
						/* Socket busy try sending again !*/
					}else{
				 		/* Check socket state */
						err = ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0);
						printf("Sock state is %s\n",
							(err == 0) ? "CONNECTED" : 
							(err == 1) ? "DISCONNECTED" :
							 "CONNECTING");

						printf("Failed to send %i \n",errno);
						perror("Send: ");
						break;
					}
				}else{

					++Tx_count;
					
					if (verbose){
						printf("Packet sent: Sent %i : %i\n",
							err,Tx_count);
					}else{
						//putchar('T');
					}
					txfile_bytes+=Tx_length;

					if ((files_used & TX_FILE_USED) && tx_fd){
						rlen=fread(pTx_data,
							   sizeof(char),
							   Tx_length,tx_fd);

						if (!rlen){
							printf("\nTx of file %s is done %i bytes!\n",
								tx_file,txfile_bytes);	
							break;	
						}
						if (Tx_length != rlen){
							Tx_length = rlen;
						}
					}
				}
		   	}

			if (tx_delay){
				sleep(tx_delay);
			}

			if (tx_cnt && Tx_count == tx_cnt && !(files_used & TX_FILE_USED)){
				write_enable=0;
				if (!rx_cnt){
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
