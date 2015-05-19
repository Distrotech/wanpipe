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
#include <linux/if.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_cfg.h>
#include <linux/wanpipe.h>
#include <linux/wanpipe_api.h>
#include "lib_api.h"

#define MAX_TX_DATA     1024*10	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	1024*10

unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];
unsigned char Tx_data[MAX_TX_DATA + sizeof(api_tx_hdr_t)];

/* Prototypes */
int MakeConnection(void);
void handle_socket( void);
void sig_end(int sigid);

int sock;
FILE *tx_fd=NULL,*rx_fd=NULL;	

/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*       (Interface name is supplied by the user)
*/         

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
  
	printf("\nConnecting to card %s, interface %s prot %x\n", card_name, if_name,htons(PVC_PROT));
	
	strcpy( sa.sll_device, if_name);
	strcpy( sa.sll_card, card_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;
	
        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",if_name);
                exit(0);
        }

	{
		unsigned int customer_id;
		int err = ioctl(sock,SIOC_AFT_CUSTOMER_ID,&customer_id);
		if (err){
			perror("Customer ID: ");
		}else{
			printf("Customer ID 0x%X\n",customer_id);
		}
	
		
	}
	printf("Socket bound to %s\n\n",if_name);
	
	return( WAN_TRUE );

}


int sangoma_read_socket(int sock, void *data, int len, int flag)
{
	return recv(sock,data,len,flag);
	
}

int sangoma_readmsg_socket(int sock, void *hdrbuf, int hdrlen, void *databuf, int datalen, int flag)
{
	struct msghdr msg;
	struct iovec iov[2];

	memset(&msg,0,sizeof(struct msghdr));

	iov[0].iov_len=hdrlen;
	iov[0].iov_base=hdrbuf;

	iov[1].iov_len=datalen;
	iov[1].iov_base=databuf;

	msg.msg_iovlen=2;
	msg.msg_iov=iov;

	return recvmsg(sock,&msg,flag);
}

int sangoma_send_socket(int sock, void *data, int len, int flag)
{
	return send(sock,data,len,flag);
	
}

int sangoma_sendmsg_socket(int sock, void *hdrbuf, int hdrlen, void *databuf, int datalen, int flag)
{
	struct msghdr msg;
	struct iovec iov[2];

	memset(&msg,0,sizeof(struct msghdr));

	iov[0].iov_len=hdrlen;
	iov[0].iov_base=hdrbuf;

	iov[1].iov_len=datalen;
	iov[1].iov_base=databuf;

	msg.msg_iovlen=2;
	msg.msg_iov=iov;

	return sendmsg(sock,&msg,flag);
}



/***************************************************
* HANDLE SOCKET 
*
*   o Read a socket 
*   o Cast data received to api_rx_element_t data type 
*   o The received packet contains sizeof(wp_api_hdr_t) bytes header 
*
*	------------------------------------------
*      |  sizeof(wp_api_hdr_t) bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
*   o Data structures:
*
*   typedef struct {
*	unsigned char	wp_api_rx_hdr_error_flag;
*	unsigned short	time_stamp;
*	unsigned char	reserved[13];
*   } api_rx_hdr_t;
*
*   typedef struct {
*       api_rx_hdr_t	api_rx_hdr;
*       unsigned char  	data[1];
*   } api_rx_element_t;
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
#if 0
	int stream_sync=0;
#endif
        Rx_count = 0;
	Tx_count = 0;
	Tx_length = tx_size;

	printf("\n\nSocket Handler: Rx=%d Tx=%i TxCnt=%i TxLen=%i TxDelay=%i\n",
			read_enable,write_enable,tx_cnt,tx_size,tx_delay);	

	/* Initialize the Tx Data buffer */
	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(api_tx_hdr_t));

	/* If rx file is specified, open
	 * it and prepare it for writing */
	if (files_used & RX_FILE_USED){
		rx_fd=fopen(rx_file,"wb");
		if (!rx_fd){
			printf("Failed to open file %s\n",rx_file);
			perror("File: ");
			return;
		}
		printf("Opening %s rx file\n",rx_file);
	}

	/* Cast the Tx data packet with the tx element
	 * structure.  We must insert a sizeof(wp_api_hdr_t) byte
	 * driver header, which driver will remove 
	 * before passing packet out the physical port */
	api_tx_el = (api_tx_element_t*)&Tx_data[0];
	
	if (files_used & TX_FILE_USED){

		/* TX file is used to transmit data */
		
		tx_fd=fopen(tx_file,"rb");
		if (!tx_fd){
			printf("Failed to open file %s\n",tx_file);
			perror("File: ");
			return;
		}
		
		printf("Opening %s tx file\n",rx_file);
		
		rlen=fread((void*)&Tx_data[sizeof(api_tx_hdr_t)],
				   sizeof(char),
				   Tx_length,tx_fd);
		if (!rlen){
			printf("%s: File empty!\n",
				tx_file);
			return;
		}
	}else{

		/* Create a Tx packet based on user info, or
		 * by deafult incrementing number starting from 0 */
		for (i=0;i<Tx_length;i++){
			if (tx_data == -1){
				api_tx_el->data[i] = (unsigned char)i;
			}else{
#if 0
				api_tx_el->data[i] = (unsigned char)tx_data+(i%4);
#else
				api_tx_el->data[i] = (unsigned char)tx_data;
#endif
			}
		}
	}


	/* Main Rx Tx OOB routine */
	for(;;) {	

		/* Initialize all select() descriptors */
		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_ZERO(&oob);
		FD_SET(sock,&oob);
		FD_SET(sock,&ready);

		if (write_enable){
		     FD_SET(sock,&write);
		}

		/* Select will block, until:
		 * 	1: OOB event, link level change
		 * 	2: Rx data available
		 * 	3: Interface able to Tx */
		
  		if(select(sock + 1,&ready, &write, &oob, NULL)){

			fflush(stdout);	
		   	if (FD_ISSET(sock,&oob)){
		
				/* An OOB event is pending, usually indicating
				 * a link level change */
				
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

				/* An Rx packet is pending
				 * 	1: Read the rx packet into the Rx_data
				 * 	   buffer. Confirm len > 0
				 *
				 * 	2: Cast Rx_data to the api_rx_element.
				 * 	   Thus, removing a sizeof(wp_api_hdr_t) byte header
				 * 	   attached by the driver.
				 *
				 * 	3. Check wp_api_rx_hdr_error_flag:
				 * 		CRC,Abort..etc 
				 */

				memset(Rx_data, 0, sizeof(Rx_data));

#if 1
				err = sangoma_readmsg_socket(sock,
                                                             Rx_data,sizeof(wp_api_hdr_t), 
							     &Rx_data[sizeof(wp_api_hdr_t)], MAX_RX_DATA-sizeof(wp_api_hdr_t), 
							     0);
#else
				err = sangoma_read_socket(sock, Rx_data, MAX_RX_DATA, 0);
#endif
				if (!read_enable){
					goto bitstrm_skip_read;
				}

				
				/* err indicates bytes received */
				if(err <= 0) {
					printf("\nError receiving data\n");
					break;
				}

				api_rx_el = (api_rx_element_t*)&Rx_data[0];

				/* Check the packet length */
				Rx_lgth = err - sizeof(api_rx_hdr_t);
				if(Rx_lgth<=0) {
					printf("\nShort frame received (%d)\n",
						Rx_lgth);
					return;
				}

				if (api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag){
					printf("Data: ");
					for(i=0;i<Rx_lgth; i ++) {
						printf("0x%02X ", Rx_data[i+sizeof(wp_api_hdr_t)]);
					}
					printf("\n");
				}

				if (api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag & (1<<WP_FIFO_ERROR_BIT)){
					printf("\nPacket with fifo overflow err=0x%X len=%i\n",
						api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag,Rx_lgth);
					continue;
				}

				if (api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag & (1<<WP_CRC_ERROR_BIT)){
					printf("\nPacket with invalid crc!  err=0x%X len=%i\n",
						api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag,Rx_lgth);
					continue;
				}

				if (api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag & (1<<WP_ABORT_ERROR_BIT)){
					printf("\nPacket with abort!  err=0x%X len=%i\n",
						api_rx_el->api_rx_hdr.wp_api_rx_hdr_error_flag,Rx_lgth);
					continue;
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

				if ((files_used & RX_FILE_USED) && rx_fd){
					fwrite((void*)&Rx_data[sizeof(api_rx_hdr_t)],
					       sizeof(char),
					       Rx_lgth,
					       rx_fd);	
				}
			
				++Rx_count;

				//printf("RECEIVE:\n");
				//print_packet(&Rx_data[sizeof(wp_api_hdr_t)],Rx_lgth);


				if (verbose){
					printf("Received %i Olen=%i Length = %i\n", 
							Rx_count, err,Rx_lgth);
#if 1
					printf("Data: ");
					for(i=0;i<Rx_lgth; i ++) {
						printf("0x%02X ", api_rx_el->data[i]);
					}
					printf("\n");
#endif
				}else{
					//putchar('R');
				}

				if (rx_cnt > 0  && Rx_count >= rx_cnt){
					break;
				}
bitstrm_skip_read:
;
		   	}
		
		   	if (FD_ISSET(sock,&write)){


				if (Tx_count == 0){
					//printf("SEND: Len=%i\n",Tx_length);
					//print_packet(&Tx_data[sizeof(wp_api_hdr_t)],Tx_length);
				}

#if 1
				err = sangoma_sendmsg_socket(sock,
						       Tx_data,sizeof(wp_api_hdr_t), 
						       &Tx_data[sizeof(wp_api_hdr_t)], Tx_length, 
						       0);
#else	
				err = sangoma_send_socket(sock,Tx_data, 
                                                          Tx_length + sizeof(api_tx_hdr_t), 0);
#endif
				if (err <= 0){
					if (errno == EBUSY){
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

						printf("Failed to send errno=%i len=%i \n",errno,Tx_length);
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
					
					if ((files_used & TX_FILE_USED) && tx_fd){
						rlen=fread((void*)&Tx_data[sizeof(api_tx_hdr_t)],
							   sizeof(char),
							   Tx_length,tx_fd);
						if (!rlen){
							printf("\nTx of file %s is done!\n",
								tx_file);	
							break;	
						}
						if (Tx_length != rlen){
							Tx_length = rlen;
						}
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



